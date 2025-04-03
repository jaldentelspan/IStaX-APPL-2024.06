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

#include <vtss/appl/mvr.h>
#include "port_api.h" /* For port_count_max() */

#ifndef _MVR_EXPOSE_HXX_
#define _MVR_EXPOSE_HXX_

typedef struct {
    vtss_appl_ipmc_lib_vlan_status_t     status;
    vtss_appl_ipmc_lib_vlan_statistics_t statistics;
} mvr_expose_igmp_vlan_status_t;

typedef struct {
    vtss_appl_ipmc_lib_vlan_status_t     status;
    vtss_appl_ipmc_lib_vlan_statistics_t statistics;
} mvr_expose_mld_vlan_status_t;

typedef struct {
    vtss_appl_ipmc_lib_grp_status_t status;

    // status::port_list[] converted to the stackable version.-
    vtss_port_list_stackable_t port_list_stackable;
} mvr_expose_grp_status_t;

// These are defined in vtss_appl/mac/mac.cxx
BOOL portlist_index_set(uint32_t i, vtss_port_list_stackable_t *pl);
uint32_t isid_port_to_index(vtss_isid_t i, mesa_port_no_t p);

/******************************************************************************/
// MVR_EXPOSE_key_get()
/******************************************************************************/
static inline vtss_appl_ipmc_lib_key_t MVR_EXPOSE_key_get(bool is_ipv4)
{
    vtss_appl_ipmc_lib_key_t key = {};

    key.is_mvr  = true;
    key.is_ipv4 = is_ipv4;

    return key;
}

/******************************************************************************/
// mvr_expose_global_conf_get()
/******************************************************************************/
static inline mesa_rc mvr_expose_global_conf_get(vtss_appl_ipmc_lib_global_conf_t *global_conf)
{
    return vtss_appl_ipmc_lib_global_conf_get(MVR_EXPOSE_key_get(true), global_conf);
}

/******************************************************************************/
// mvr_expose_global_conf_default_get()
/******************************************************************************/
static inline mesa_rc mvr_expose_global_conf_default_get(vtss_appl_ipmc_lib_global_conf_t *global_conf)
{
    return vtss_appl_ipmc_lib_global_conf_default_get(MVR_EXPOSE_key_get(true), global_conf);
}

/******************************************************************************/
// MVR_EXPOSE_ifindex_to_port_get(
/******************************************************************************/
static inline mesa_rc MVR_EXPOSE_ifindex_to_port_get(const vtss_ifindex_t *ifindex, mesa_port_no_t &port_no)
{
    vtss_ifindex_elm_t ife;

    if (!ifindex) {
        return VTSS_RC_ERROR;
    }

    if (vtss_ifindex_decompose(*ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        return VTSS_RC_ERROR;
    }

    port_no = ife.ordinal;
    return VTSS_RC_OK;
}

/******************************************************************************/
// mvr_expose_port_conf_get()
/******************************************************************************/
static inline mesa_rc mvr_expose_port_conf_get(const vtss_ifindex_t *ifindex, vtss_appl_ipmc_lib_port_conf_t *conf)
{
    mesa_port_no_t port_no;

    VTSS_RC(MVR_EXPOSE_ifindex_to_port_get(ifindex, port_no));
    return vtss_appl_ipmc_lib_port_conf_get(MVR_EXPOSE_key_get(true), port_no, conf);
}

/******************************************************************************/
// mvr_expose_port_conf_set()
/******************************************************************************/
static inline mesa_rc mvr_expose_port_conf_set(const vtss_ifindex_t *ifindex, const vtss_appl_ipmc_lib_port_conf_t *conf)
{
    mesa_port_no_t port_no;

    VTSS_RC(MVR_EXPOSE_ifindex_to_port_get(ifindex, port_no));
    return vtss_appl_mvr_port_conf_set(port_no, conf);
}

/******************************************************************************/
// mvr_expose_port_conf_default_get()
/******************************************************************************/
static inline mesa_rc mvr_expose_port_conf_default_get(vtss_ifindex_t *ifindex, vtss_appl_ipmc_lib_port_conf_t *conf)
{
    return vtss_appl_ipmc_lib_port_conf_default_get(MVR_EXPOSE_key_get(true), conf);
}

/******************************************************************************/
// mvr_expose_vlan_conf_default_get()
/******************************************************************************/
static inline mesa_rc mvr_expose_vlan_conf_default_get(vtss_ifindex_t *ifindex, vtss_appl_ipmc_lib_vlan_conf_t *conf)
{
    return vtss_appl_ipmc_lib_vlan_conf_default_get(MVR_EXPOSE_key_get(true), conf);
}

/******************************************************************************/
// MVR_EXPOSE_ifindex_to_vlan_get()
/******************************************************************************/
static inline mesa_rc MVR_EXPOSE_ifindex_to_vlan_get(const vtss_ifindex_t *ifindex, mesa_vid_t &vid)
{
    vtss_ifindex_elm_t ife;

    if (!ifindex) {
        return VTSS_RC_ERROR;
    }

    if (vtss_ifindex_decompose(*ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_VLAN) {
        return VTSS_RC_ERROR;
    }

    vid = ife.ordinal;
    return VTSS_RC_OK;
}

/******************************************************************************/
// MVR_EXPOSE_vlan_key_get()
/******************************************************************************/
static inline vtss_appl_ipmc_lib_vlan_key_t MVR_EXPOSE_vlan_key_get(mesa_vid_t vid, bool is_ipv4)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key = {};

    vlan_key.is_mvr  = true;
    vlan_key.is_ipv4 = is_ipv4;
    vlan_key.vid     = vid;

    return vlan_key;
}

/******************************************************************************/
// mvr_expose_vlan_conf_get()
/******************************************************************************/
static inline mesa_rc mvr_expose_vlan_conf_get(const vtss_ifindex_t *ifindex, vtss_appl_ipmc_lib_vlan_conf_t *conf)
{
    mesa_vid_t vid;

    VTSS_RC(MVR_EXPOSE_ifindex_to_vlan_get(ifindex, vid));
    return vtss_appl_ipmc_lib_vlan_conf_get(MVR_EXPOSE_vlan_key_get(vid, true), conf);
}

/******************************************************************************/
// mvr_expose_vlan_conf_set()
/******************************************************************************/
static inline mesa_rc mvr_expose_vlan_conf_set(const vtss_ifindex_t *ifindex, const vtss_appl_ipmc_lib_vlan_conf_t *conf)
{
    mesa_vid_t vid;

    VTSS_RC(MVR_EXPOSE_ifindex_to_vlan_get(ifindex, vid));
    return vtss_appl_mvr_vlan_conf_set(vid, conf);
}

/******************************************************************************/
// mvr_expose_vlan_conf_del()
/******************************************************************************/
static inline mesa_rc mvr_expose_vlan_conf_del(const vtss_ifindex_t *ifindex)
{
    mesa_vid_t vid;

    VTSS_RC(MVR_EXPOSE_ifindex_to_vlan_get(ifindex, vid));
    return vtss_appl_mvr_vlan_conf_del(vid);
}

/******************************************************************************/
// mvr_expose_vlan_itr()
/******************************************************************************/
static inline mesa_rc mvr_expose_vlan_itr(const vtss_ifindex_t *vid_ifindex_prev, vtss_ifindex_t *vid_ifindex_next)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key;
    mesa_vid_t                    vid;

    if (!vid_ifindex_next) {
        return VTSS_RC_ERROR;
    }

    if (vid_ifindex_prev) {
        VTSS_RC(MVR_EXPOSE_ifindex_to_vlan_get(vid_ifindex_prev, vid));
    } else {
        vid = 0;
    }

    vlan_key = MVR_EXPOSE_vlan_key_get(vid, true);

    VTSS_RC(vtss_appl_ipmc_lib_vlan_itr(&vlan_key, &vlan_key, true /* Don't mix MVR/IPMC and IGMP/MLD */));

    // Convert back to ifindex
    return vtss_ifindex_from_vlan(vlan_key.vid, vid_ifindex_next);
}

/******************************************************************************/
// mvr_expose_vlan_port_conf_default_get()
/******************************************************************************/
static inline mesa_rc mvr_expose_vlan_port_conf_default_get(vtss_ifindex_t *vid_ifindex, vtss_ifindex_t *port_ifindex, vtss_appl_ipmc_lib_vlan_port_conf_t *vlan_port_conf)
{
    vtss_appl_ipmc_lib_key_t key;

    key = MVR_EXPOSE_key_get(true);
    return vtss_appl_ipmc_lib_vlan_port_conf_default_get(key, vlan_port_conf);
}

/******************************************************************************/
// mvr_expose_vlan_port_conf_get()
/******************************************************************************/
static inline mesa_rc mvr_expose_vlan_port_conf_get(const vtss_ifindex_t *vid_ifindex, const vtss_ifindex_t *port_ifindex, vtss_appl_ipmc_lib_vlan_port_conf_t *vlan_port_conf)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key;
    mesa_vid_t                    vid;
    mesa_port_no_t                port_no;

    VTSS_RC(MVR_EXPOSE_ifindex_to_vlan_get(vid_ifindex,  vid));
    VTSS_RC(MVR_EXPOSE_ifindex_to_port_get(port_ifindex, port_no));

    vlan_key = MVR_EXPOSE_vlan_key_get(vid, true);

    return vtss_appl_ipmc_lib_vlan_port_conf_get(vlan_key, port_no, vlan_port_conf);
}

/******************************************************************************/
// mvr_expose_vlan_port_itr()
/******************************************************************************/
static inline mesa_rc mvr_expose_vlan_port_itr(const vtss_ifindex_t *vid_ifindex_prev, vtss_ifindex_t *vid_ifindex_next, const vtss_ifindex_t *port_ifindex_prev, vtss_ifindex_t *port_ifindex_next)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key;
    mesa_vid_t                    vid;
    mesa_port_no_t                port_no;

    if (!vid_ifindex_next || !port_ifindex_next) {
        return VTSS_RC_ERROR;
    }

    if (vid_ifindex_prev) {
        VTSS_RC(MVR_EXPOSE_ifindex_to_vlan_get(vid_ifindex_prev, vid));
    } else {
        vid = 0;
    }

    if (port_ifindex_prev) {
        VTSS_RC(MVR_EXPOSE_ifindex_to_port_get(port_ifindex_prev, port_no));
    } else {
        port_no = 0;
    }

    vlan_key = MVR_EXPOSE_vlan_key_get(vid, true);

    VTSS_RC(vtss_appl_ipmc_lib_vlan_port_itr(&vlan_key, &vlan_key, &port_no, &port_no, true /* Don't mix MVR/IPMC and IGMP/MLD */));

    // Convert back to ifindex
    VTSS_RC(vtss_ifindex_from_vlan(vlan_key.vid, vid_ifindex_next));
    VTSS_RC(vtss_ifindex_from_port(VTSS_ISID_START, port_no, port_ifindex_next));

    return VTSS_RC_OK;
}

/******************************************************************************/
// mvr_expose_vlan_port_conf_set()
/******************************************************************************/
static inline mesa_rc mvr_expose_vlan_port_conf_set(const vtss_ifindex_t *vid_ifindex, const vtss_ifindex_t *port_ifindex, const vtss_appl_ipmc_lib_vlan_port_conf_t *vlan_port_conf)
{
    mesa_vid_t     vid;
    mesa_port_no_t port_no;

    VTSS_RC(MVR_EXPOSE_ifindex_to_vlan_get(vid_ifindex,  vid));
    VTSS_RC(MVR_EXPOSE_ifindex_to_port_get(port_ifindex, port_no));

    return vtss_appl_mvr_vlan_port_conf_set(vid, port_no, vlan_port_conf);
}

/******************************************************************************/
// MVR_EXPOSE_vlan_status_get()
/******************************************************************************/
static inline mesa_rc MVR_EXPOSE_vlan_status_get(const vtss_ifindex_t *vid_ifindex, vtss_appl_ipmc_lib_vlan_status_t &vlan_status, vtss_appl_ipmc_lib_vlan_statistics_t &vlan_statistics, bool is_ipv4)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key;
    mesa_vid_t                    vid;

    VTSS_RC(MVR_EXPOSE_ifindex_to_vlan_get(vid_ifindex, vid));

    vlan_key = MVR_EXPOSE_vlan_key_get(vid, is_ipv4);

    VTSS_RC(vtss_appl_ipmc_lib_vlan_status_get(vlan_key, &vlan_status));
    return vtss_appl_ipmc_lib_vlan_statistics_get(vlan_key, &vlan_statistics);
}

/******************************************************************************/
// mvr_expose_igmp_vlan_status_get()
/******************************************************************************/
static inline mesa_rc mvr_expose_igmp_vlan_status_get(const vtss_ifindex_t *vid_ifindex, mvr_expose_igmp_vlan_status_t *status)
{
    if (!status) {
        return VTSS_RC_ERROR;
    }

    return MVR_EXPOSE_vlan_status_get(vid_ifindex, status->status, status->statistics, true);
}

/******************************************************************************/
// mvr_expose_mld_vlan_status_get()
/******************************************************************************/
static inline mesa_rc mvr_expose_mld_vlan_status_get(const vtss_ifindex_t *vid_ifindex, mvr_expose_mld_vlan_status_t *status)
{
    if (!status) {
        return VTSS_RC_ERROR;
    }

    return MVR_EXPOSE_vlan_status_get(vid_ifindex, status->status, status->statistics, false);
}

/******************************************************************************/
// MVR_EXPOSE_grp_itr()
/******************************************************************************/
static inline mesa_rc MVR_EXPOSE_grp_itr(const vtss_ifindex_t *vid_ifindex_prev, vtss_ifindex_t *vid_ifindex_next, vtss_appl_ipmc_lib_ip_t &grp)
{
    vtss_appl_ipmc_lib_vlan_key_t key;
    mesa_vid_t                    vid;

    if (!vid_ifindex_next) {
        return VTSS_RC_ERROR;
    }

    if (vid_ifindex_prev) {
        VTSS_RC(MVR_EXPOSE_ifindex_to_vlan_get(vid_ifindex_prev, vid));
    } else {
        vid = 0;
    }

    key = MVR_EXPOSE_vlan_key_get(vid, grp.is_ipv4);

    VTSS_RC(vtss_appl_ipmc_lib_grp_itr(&key, &key, &grp, &grp, true /* Don't mix IPMC/MVR and IGMP/MLD */));

    // Convert back to ifindex
    VTSS_RC(vtss_ifindex_from_vlan(key.vid, vid_ifindex_next));

    return VTSS_RC_OK;
}

/******************************************************************************/
// mvr_expose_igmp_grp_itr()
/******************************************************************************/
static inline mesa_rc mvr_expose_igmp_grp_itr(const vtss_ifindex_t *vid_ifindex_prev, vtss_ifindex_t *vid_ifindex_next, const mesa_ipv4_t *grp_ipv4_prev, mesa_ipv4_t *grp_ipv4_next)
{
    vtss_appl_ipmc_lib_ip_t grp = {};

    if (!grp_ipv4_next) {
        return VTSS_RC_ERROR;
    }

    grp.is_ipv4 = true;
    if (grp_ipv4_prev) {
        grp.ipv4 = *grp_ipv4_prev;
    }

    VTSS_RC(MVR_EXPOSE_grp_itr(vid_ifindex_prev, vid_ifindex_next, grp));

    *grp_ipv4_next = grp.ipv4;
    return VTSS_RC_OK;
}

/******************************************************************************/
// mvr_expose_mld_grp_itr()
/******************************************************************************/
static inline mesa_rc mvr_expose_mld_grp_itr(const vtss_ifindex_t *vid_ifindex_prev, vtss_ifindex_t *vid_ifindex_next, const mesa_ipv6_t *grp_ipv6_prev, mesa_ipv6_t *grp_ipv6_next)
{
    vtss_appl_ipmc_lib_ip_t grp = {};

    if (!grp_ipv6_next) {
        return VTSS_RC_ERROR;
    }

    grp.is_ipv4 = false;
    if (grp_ipv6_prev) {
        grp.ipv6 = *grp_ipv6_prev;
    }

    VTSS_RC(MVR_EXPOSE_grp_itr(vid_ifindex_prev, vid_ifindex_next, grp));

    *grp_ipv6_next = grp.ipv6;
    return VTSS_RC_OK;
}


/******************************************************************************/
// MVR_EXPOSE_grp_status_get()
/******************************************************************************/
static inline mesa_rc MVR_EXPOSE_grp_status_get(const vtss_ifindex_t *vid_ifindex, const vtss_appl_ipmc_lib_ip_t &grp, mvr_expose_grp_status_t *grp_status)
{
    mesa_vid_t     vid;
    mesa_port_no_t port_no;
    uint32_t       port_cnt;

    if (!grp_status) {
        return VTSS_RC_ERROR;
    }

    VTSS_RC(MVR_EXPOSE_ifindex_to_vlan_get(vid_ifindex, vid));
    VTSS_RC(vtss_appl_ipmc_lib_grp_status_get(MVR_EXPOSE_vlan_key_get(vid, grp.is_ipv4), &grp, &grp_status->status));

    // Convert grp_status.port_list to grp_status->port_list_stackable
    vtss_clear(grp_status->port_list_stackable);
    port_cnt = port_count_max();
    for (port_no = 0; port_no < port_cnt; port_no++) {
        if (grp_status->status.port_list[port_no]) {
            (void)portlist_index_set(isid_port_to_index(VTSS_ISID_START, port_no), &grp_status->port_list_stackable);
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// mvr_expose_igmp_grp_status_get()
/******************************************************************************/
static inline mesa_rc mvr_expose_igmp_grp_status_get(const vtss_ifindex_t *vid_ifindex, const mesa_ipv4_t *grp_ipv4, mvr_expose_grp_status_t *grp_status)
{
    vtss_appl_ipmc_lib_ip_t grp = {};

    if (!grp_ipv4) {
        return VTSS_RC_ERROR;
    }

    grp.is_ipv4 = true;
    grp.ipv4    = *grp_ipv4;

    return MVR_EXPOSE_grp_status_get(vid_ifindex, grp, grp_status);
}

/******************************************************************************/
// mvr_expose_mld_grp_status_get()
/******************************************************************************/
static inline mesa_rc mvr_expose_mld_grp_status_get(const vtss_ifindex_t *vid_ifindex, const mesa_ipv6_t *grp_ipv6, mvr_expose_grp_status_t *grp_status)
{
    vtss_appl_ipmc_lib_ip_t grp = {};

    if (!grp_ipv6) {
        return VTSS_RC_ERROR;
    }

    grp.is_ipv4 = false;
    grp.ipv6    = *grp_ipv6;

    return MVR_EXPOSE_grp_status_get(vid_ifindex, grp, grp_status);
}

/******************************************************************************/
// MVR_EXPOSE_src_itr()
/******************************************************************************/
static inline mesa_rc MVR_EXPOSE_src_itr(const vtss_ifindex_t *vid_ifindex_prev, vtss_ifindex_t *vid_ifindex_next, vtss_appl_ipmc_lib_ip_t &grp, const vtss_ifindex_t *port_ifindex_prev, vtss_ifindex_t *port_ifindex_next, vtss_appl_ipmc_lib_ip_t &src)
{
    vtss_appl_ipmc_lib_vlan_key_t key;
    mesa_vid_t                    vid;
    mesa_port_no_t                port_no;

    if (!vid_ifindex_next) {
        return VTSS_RC_ERROR;
    }

    if (vid_ifindex_prev) {
        VTSS_RC(MVR_EXPOSE_ifindex_to_vlan_get(vid_ifindex_prev, vid));
    } else {
        vid = 0;
    }

    if (port_ifindex_prev) {
        VTSS_RC(MVR_EXPOSE_ifindex_to_port_get(port_ifindex_prev, port_no));
    } else {
        port_no = 0; // Doesn't matter
    }

    key = MVR_EXPOSE_vlan_key_get(vid, grp.is_ipv4);

    VTSS_RC(vtss_appl_ipmc_lib_src_itr(&key, &key, &grp, &grp, &port_no, &port_no, &src, &src, true /* Don't mix IPMC/MVR and IGMP/MLD */));

    // Convert back to ifindex
    VTSS_RC(vtss_ifindex_from_vlan(key.vid, vid_ifindex_next));
    VTSS_RC(vtss_ifindex_from_port(VTSS_ISID_START, port_no, port_ifindex_next));

    return VTSS_RC_OK;
}

/******************************************************************************/
// mvr_expose_igmp_src_itr()
/******************************************************************************/
static inline mesa_rc mvr_expose_igmp_src_itr(const vtss_ifindex_t *vid_ifindex_prev, vtss_ifindex_t *vid_ifindex_next, const mesa_ipv4_t *grp_ipv4_prev, mesa_ipv4_t *grp_ipv4_next, const vtss_ifindex_t *port_ifindex_prev, vtss_ifindex_t *port_ifindex_next, const mesa_ipv4_t *src_ipv4_prev, mesa_ipv4_t *src_ipv4_next)
{
    vtss_appl_ipmc_lib_ip_t grp = {}, src = {};

    if (!grp_ipv4_next || !src_ipv4_next) {
        return VTSS_RC_ERROR;
    }

    grp.is_ipv4 = true;
    if (grp_ipv4_prev) {
        grp.ipv4 = *grp_ipv4_prev;
    }

    src.is_ipv4 = true;
    if (src_ipv4_prev) {
        src.ipv4 = *src_ipv4_prev;
    }

    VTSS_RC(MVR_EXPOSE_src_itr(vid_ifindex_prev, vid_ifindex_next, grp, port_ifindex_prev, port_ifindex_next, src));

    *grp_ipv4_next = grp.ipv4;
    *src_ipv4_next = src.ipv4;
    return VTSS_RC_OK;
}

/******************************************************************************/
// mvr_expose_mld_src_itr()
/******************************************************************************/
static inline mesa_rc mvr_expose_mld_src_itr(const vtss_ifindex_t *vid_ifindex_prev, vtss_ifindex_t *vid_ifindex_next, const mesa_ipv6_t *grp_ipv6_prev, mesa_ipv6_t *grp_ipv6_next, const vtss_ifindex_t *port_ifindex_prev, vtss_ifindex_t *port_ifindex_next, const mesa_ipv6_t *src_ipv6_prev, mesa_ipv6_t *src_ipv6_next)
{
    vtss_appl_ipmc_lib_ip_t grp = {}, src = {};

    if (!grp_ipv6_next || !src_ipv6_next) {
        return VTSS_RC_ERROR;
    }

    grp.is_ipv4 = false;
    if (grp_ipv6_prev) {
        grp.ipv6 = *grp_ipv6_prev;
    }

    src.is_ipv4 = false;
    if (src_ipv6_prev) {
        src.ipv6 = *src_ipv6_prev;
    }

    VTSS_RC(MVR_EXPOSE_src_itr(vid_ifindex_prev, vid_ifindex_next, grp, port_ifindex_prev, port_ifindex_next, src));

    *grp_ipv6_next = grp.ipv6;
    *src_ipv6_next = src.ipv6;
    return VTSS_RC_OK;
}

/******************************************************************************/
// MVR_EXPOSE_src_status_get()
/******************************************************************************/
static inline mesa_rc MVR_EXPOSE_src_status_get(const vtss_ifindex_t *vid_ifindex, vtss_appl_ipmc_lib_ip_t &grp, const vtss_ifindex_t *port_ifindex, const vtss_appl_ipmc_lib_ip_t &src, vtss_appl_ipmc_lib_src_status_t *src_status)
{
    mesa_vid_t              vid;
    mesa_port_no_t          port_no;

    if (!src_status) {
        return VTSS_RC_ERROR;
    }

    VTSS_RC(MVR_EXPOSE_ifindex_to_vlan_get(vid_ifindex, vid));
    VTSS_RC(MVR_EXPOSE_ifindex_to_port_get(port_ifindex, port_no));

    return vtss_appl_ipmc_lib_src_status_get(MVR_EXPOSE_vlan_key_get(vid, grp.is_ipv4), &grp, port_no, &src, src_status);
}

/******************************************************************************/
// mvr_expose_igmp_src_status_get()
/******************************************************************************/
static inline mesa_rc mvr_expose_igmp_src_status_get(const vtss_ifindex_t *vid_ifindex, const mesa_ipv4_t *grp_ipv4, const vtss_ifindex_t *port_ifindex, const mesa_ipv4_t *src_ipv4, vtss_appl_ipmc_lib_src_status_t *src_status)
{
    vtss_appl_ipmc_lib_ip_t grp = {}, src = {};

    if (!grp_ipv4 || !src_ipv4) {
        return VTSS_RC_ERROR;
    }

    grp.is_ipv4 = true;
    grp.ipv4    = *grp_ipv4;

    src.is_ipv4 = true;
    src.ipv4    = *src_ipv4;

    return MVR_EXPOSE_src_status_get(vid_ifindex, grp, port_ifindex, src, src_status);
}

/******************************************************************************/
// mvr_expose_mld_src_status_get()
/******************************************************************************/
static inline mesa_rc mvr_expose_mld_src_status_get(const vtss_ifindex_t *vid_ifindex, const mesa_ipv6_t *grp_ipv6, const vtss_ifindex_t *port_ifindex, const mesa_ipv6_t *src_ipv6, vtss_appl_ipmc_lib_src_status_t *src_status)
{
    vtss_appl_ipmc_lib_ip_t grp = {}, src = {};

    if (!grp_ipv6 || !src_ipv6) {
        return VTSS_RC_ERROR;
    }

    grp.is_ipv4 = false;
    grp.ipv6    = *grp_ipv6;

    src.is_ipv4 = false;
    src.ipv6    = *src_ipv6;

    return MVR_EXPOSE_src_status_get(vid_ifindex, grp, port_ifindex, src, src_status);
}

/******************************************************************************/
// mvr_expose_vlan_statistics_dummy_get()
/******************************************************************************/
static inline mesa_rc mvr_expose_vlan_statistics_dummy_get(const vtss_ifindex_t *vid_ifindex, BOOL *clear)
{
    if (clear && *clear) {
        *clear = FALSE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// mvr_expose_vlan_statistics_clear()
/******************************************************************************/
static inline mesa_rc mvr_expose_vlan_statistics_clear(const vtss_ifindex_t *vid_ifindex, const BOOL *clear)
{
    mesa_vid_t vid;

    if (clear && *clear) {
        VTSS_RC(MVR_EXPOSE_ifindex_to_vlan_get(vid_ifindex, vid));
        return vtss_appl_mvr_vlan_statistics_clear(vid);

    }

    return VTSS_RC_OK;
}

#endif // _MVR_EXPOSE_HXX_

