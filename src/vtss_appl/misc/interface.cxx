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

#include <stdio.h>  /* snprintf */
#include "vtss/appl/interface.h"
#include "port_api.h"         // For port_count_max()
#include "vtss_common_iterator.hxx"
#include "standalone_api.h"   // topo_usid2isid(), topo_isid2usid()
#include "vtss_trace_api.h"
#include "misc.h"
#include "misc_api.h" //iport2uport()
#include "msg_api.h"
#include "mgmt_api.h"
#include "vlan_api.h"
#include "vtss_appl_formatting_tags.hxx"
#include "vtss_avl_tree_api.h"
#include <net/if.h> /* For if_indextoname() */
#include <vtss/basics/parser_impl.hxx>

#ifdef VTSS_ALLOC_MODULE_ID
    #undef VTSS_ALLOC_MODULE_ID
#endif
#define VTSS_ALLOC_MODULE_ID    VTSS_MODULE_ID_MISC

#define VTSS_IFINDEX_BITMASK(x)               ((1 << (x)) - 1)
#define VTSS_IFINDEX_TYPE_CHK(_f, _t)         (_f & (1<<_t))

#ifdef VTSS_SW_OPTION_AGGR
#define VTSS_AGGR_GROUP_NO_END aggr_mgmt_group_no_end()
#else
#define VTSS_AGGR_GROUP_NO_END (fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)/2)
#endif

#define VTSS_APPL_CPU_MAX       100
#define VTSS_APPL_FOREIGN_MAX   (100000000 - 1) // minus 1 since the ID is One-based
#define VTSS_APPL_FRR_VLINK_MAX (100000000 - 1) // minus 1 since the ID is One-based

#define IF_T_D                              T_D
#define IF_T_E                              T_E

size_t fmt(vtss::ostream &stream, const vtss::Fmt &fmt, const vtss_ifindex_t *p) {
    switch(fmt.encoding_flag) {
        case 'd':
        case 'u':
        case 'x':
            return vtss::fmt(stream, fmt, &VTSS_IFINDEX_PRINTF_ARG(*p));
        case 's':
        default: {
            char buf[64];
            vtss_ifindex2str(buf, sizeof(buf), *p);
            return stream.write(buf, buf + strnlen(buf, sizeof(buf)));
        }
    }
}

vtss_ifindex_t vtss_ifindex_cast_from_u32_0(u32 i) {
    return vtss_ifindex_t{i};
}

vtss_ifindex_t vtss_ifindex_cast_from_u32(u32 i, vtss_ifindex_type_t a) {
    vtss_ifindex_t x{i};
    vtss_ifindex_elm_t elm;
    vtss_ifindex_type_t t = VTSS_IFINDEX_TYPE_ILLEGAL;
    BOOL decompose_ok = 0;

    if (vtss_ifindex_decompose(x, &elm) == VTSS_RC_OK) {
        t = elm.iftype;
        decompose_ok = 1;
    }

    if (t != a) {
        T_E("Unexpected ifindex type. i: %u, type: %u%s, expected type: %u", i,
            t, (decompose_ok ? "":"*"), a);
    }

    return x;
}

vtss_ifindex_t vtss_ifindex_cast_from_u32_2(u32 i, vtss_ifindex_type_t a,
                                            vtss_ifindex_type_t b) {
    vtss_ifindex_t x{i};
    vtss_ifindex_elm_t elm;
    vtss_ifindex_type_t t = VTSS_IFINDEX_TYPE_ILLEGAL;
    BOOL decompose_ok = 0;

    if (vtss_ifindex_decompose(x, &elm) == VTSS_RC_OK) {
        t = elm.iftype;
        decompose_ok = 1;
    }

    if (t != a && t != b) {
        T_E("Unexpected ifindex type. i: %u, type: %u%s, expected type: "
            "%u or %u", i, t, (decompose_ok ? "":"*"), a, b);
    }

    return x;
}

vtss_ifindex_t vtss_ifindex_cast_from_u32_3(u32 i, vtss_ifindex_type_t a,
                                            vtss_ifindex_type_t b,
                                            vtss_ifindex_type_t c) {
    vtss_ifindex_t x{i};
    vtss_ifindex_elm_t elm;
    vtss_ifindex_type_t t = VTSS_IFINDEX_TYPE_ILLEGAL;
    BOOL decompose_ok = 0;

    if (vtss_ifindex_decompose(x, &elm) == VTSS_RC_OK) {
        t = elm.iftype;
        decompose_ok = 1;
    }

    if (t != a && t != b && t != c) {
        T_E("Unexpected ifindex type. i: %u, type: %u%s, expected type: "
            "%u, %u or %u", i, t, (decompose_ok ? "":"*"), a, b, c);
    }

    return x;
}

vtss_ifindex_t vtss_ifindex_cast_from_u32_4(u32 i, vtss_ifindex_type_t a,
                                            vtss_ifindex_type_t b,
                                            vtss_ifindex_type_t c,
                                            vtss_ifindex_type_t d) {
    vtss_ifindex_t x{i};
    vtss_ifindex_elm_t elm;
    vtss_ifindex_type_t t = VTSS_IFINDEX_TYPE_ILLEGAL;
    BOOL decompose_ok = 0;

    if (vtss_ifindex_decompose(x, &elm) == VTSS_RC_OK) {
        t = elm.iftype;
        decompose_ok = 1;
    }

    if (t != a && t != b && t != c && t != d) {
        T_E("Unexpected ifindex type. i: %u, type: %u%s, expected type: "
            "%u, %u, %u or %u", i, t, (decompose_ok ? "":"*"), a, b, c, d);
    }

    return x;
}

/*
 * iterator function prototype
 */
typedef mesa_rc  (vtss_ifindex_iterator_cb_t) (vtss_ifindex_t startIfindex, const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex, BOOL check_exist);

/*
 * interface index iterator callback entry data structure.
 */
typedef struct vtss_ifindex_iterator_cb_ent_s {
    //vtss_ifindex_iterator_cb_ent_s  *next;
    vtss_ifindex_t                  startIfindex;
    u32                             type;
    vtss_ifindex_iterator_cb_t      *iterator_cb_fn;
} vtss_ifindex_iterator_cb_ent_t;


static i32 _iterator_cb_cmp_func(void    *data_a, void    *data_b);
/*lint -esym(459,ifindex_iterator_cb_avlt)*/
VTSS_AVL_TREE(ifindex_iterator_cb_avlt, "ifIndex_iterator_fn", 0, _iterator_cb_cmp_func, ((VTSS_USID_END*2)+VTSS_IFINDEX_TYPE_ILLEGAL))


static i32 _iterator_cb_cmp_func(void *data_a, void *data_b)
{
    vtss_ifindex_iterator_cb_ent_t  *a;
    vtss_ifindex_iterator_cb_ent_t  *b;

    a = (vtss_ifindex_iterator_cb_ent_t *)data_a;
    b = (vtss_ifindex_iterator_cb_ent_t *)data_b;

    if (a->startIfindex > b->startIfindex) {
        return 1;
    }

    if (a->startIfindex < b->startIfindex) {
        return -1;
    }

    return 0;
}

/*
 * Insert ifindex get next callback.
 */
static mesa_rc vtss_ifindex_add_iterator_cb(vtss_ifindex_iterator_cb_ent_t   *info)
{
    vtss_ifindex_iterator_cb_ent_t      *new_ent;

    /* ifindex_iterator_cb_avlt is initialed at system init and will not add or delete at runtime.
     * So, there is no semaphore protection here.
     * If the ifindex_iterator_cb_avlt changes design to dynamically update at runtime,
     * semaphore protect must be implement here!
     */

    new_ent = (vtss_ifindex_iterator_cb_ent_t *) VTSS_MALLOC(sizeof(vtss_ifindex_iterator_cb_ent_t));
    if (new_ent == NULL)
    {
        IF_T_E("Out of memory! start %d type %d", VTSS_IFINDEX_PRINTF_ARG(info->startIfindex), info->type);
        return VTSS_RC_ERROR;
    }

    memcpy(new_ent, info, sizeof(vtss_ifindex_iterator_cb_ent_t));
    if ( vtss_avl_tree_add(&ifindex_iterator_cb_avlt, (void *)new_ent) == FALSE ) {
        IF_T_E("Add fail! start %d type %d", VTSS_IFINDEX_PRINTF_ARG(info->startIfindex), info->type);
        return VTSS_RC_ERROR;
    } else {
        return VTSS_RC_OK;
    }
}

/*
 * Front port iterator function.
 * This function is per-USID bases.
 */
static mesa_rc vtss_ifindex_iterator_front_port_per_usid(vtss_ifindex_t startIfindex,
                                                         const vtss_ifindex_t *prev_ifindex,
                                                         vtss_ifindex_t *next_ifindex,
                                                         BOOL check_exist)
{
    vtss_usid_t         usid;
    vtss_isid_t         isid;
    vtss_uport_no_t     uport;
    mesa_rc             rc = VTSS_RC_ERROR;

    usid = vtss_ifindex_cast_to_u32(startIfindex) / VTSS_IFINDEX_UNIT_MULTP_;
    if (check_exist) {
        isid = topo_usid2isid(usid);
        if ( !msg_switch_is_local(isid) && !msg_switch_exists(isid)) {
            return VTSS_RC_ERROR;
        }
    }

    if (prev_ifindex == NULL || *prev_ifindex < (VTSS_IFINDEX_PORT_UNIT_OFFSET_ + (usid * VTSS_IFINDEX_UNIT_MULTP_))) {
        /* first */
        uport = iport2uport(VTSS_PORT_NO_START);
    } else {
        /* next */
        uport = ((vtss_ifindex_cast_to_u32(*prev_ifindex) + 1) % VTSS_IFINDEX_UNIT_MULTP_) - VTSS_IFINDEX_PORT_UNIT_OFFSET_;
    }

    rc = vtss_ifindex_from_usid_uport(usid, uport,  next_ifindex);

    return rc;
}



/*
 * LLAG iterator function.
 * This function is per-USID bases.
 */
static mesa_rc vtss_ifindex_iterator_llag_per_usid(vtss_ifindex_t startIfindex,
                                                   const vtss_ifindex_t *prev_ifindex,
                                                   vtss_ifindex_t *next_ifindex,
                                                   BOOL check_exist)
{
    vtss_ifindex_t              ifindex;
    vtss_usid_t                 usid;
    vtss_isid_t                 isid;
    u32                         ordinal;
    mesa_rc                     rc = VTSS_RC_ERROR;
#ifdef VTSS_SW_OPTION_AGGR
    aggr_mgmt_group_no_t        aggr_id = 0;
    aggr_mgmt_group_member_t    aggr_members;
#endif

    usid = vtss_ifindex_cast_to_u32(startIfindex) / VTSS_IFINDEX_UNIT_MULTP_;
    isid = topo_usid2isid(usid);
    if (check_exist) {
        if (!msg_switch_configurable(isid)) {
            return VTSS_RC_ERROR;
        }
    }

    if (prev_ifindex == NULL || vtss_ifindex_cast_to_u32(*prev_ifindex) < (VTSS_IFINDEX_LLAG_UNIT_OFFSET_ + usid * VTSS_IFINDEX_UNIT_MULTP_)) {
        /* first */
        ordinal = VTSS_AGGR_NO_START + 1;
    } else {
        /* next */
        ordinal = (vtss_ifindex_cast_to_u32(*prev_ifindex) + 1) % VTSS_IFINDEX_UNIT_MULTP_ - VTSS_IFINDEX_LLAG_UNIT_OFFSET_;
    }

    for (; ordinal < VTSS_AGGR_GROUP_NO_END + 1; ordinal++) {
        if (vtss_ifindex_from_usid_llag(usid, ordinal, &ifindex) == VTSS_RC_OK) {
            if (check_exist) {
#ifdef VTSS_SW_OPTION_AGGR
                aggr_id = ordinal;
                if ((aggr_mgmt_port_members_get(isid, aggr_id >= MGMT_AGGR_ID_START ? mgmt_aggr_id2no(aggr_id) : (MGMT_AGGR_ID_START - 1), &aggr_members, FALSE) != VTSS_RC_OK)
#ifdef VTSS_SW_OPTION_LACP
                    && (aggr_mgmt_lacp_members_get(isid, aggr_id >= MGMT_AGGR_ID_START ? mgmt_aggr_id2no(aggr_id) : (MGMT_AGGR_ID_START - 1), &aggr_members, FALSE) != VTSS_RC_OK)
#endif /* VTSS_SW_OPTION_LACP */
                   ) {
                    continue;
                }
#else
                continue;
#endif/* VTSS_SW_OPTION_AGGR */
            }
            *next_ifindex = ifindex;
            rc = VTSS_RC_OK;

            break;
        }
    }/* for */

    return rc;
}


#if defined(VTSS_GLAGS)
/*
 * Check if a GLAG is exist or not
 */
static BOOL _vtss_ifindex_is_glag_exist(u32 ordinal)
{
#ifdef VTSS_SW_OPTION_AGGR
    BOOL                        found = FALSE;
    vtss_isid_t                 isid = 0;
    aggr_mgmt_group_no_t        aggr_id;
    aggr_mgmt_group_member_t    aggr_members;

    aggr_id = AGGR_MGMT_GLAG_START_ + ordinal - AGGR_MGMT_GROUP_NO_START;
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (!msg_switch_configurable(isid)) {
            continue;
        }

        if ((aggr_mgmt_port_members_get(isid, aggr_id, &aggr_members, FALSE) != VTSS_RC_OK)
#ifdef VTSS_SW_OPTION_LACP
            && (aggr_mgmt_lacp_members_get(isid, aggr_id, &aggr_members, FALSE) != VTSS_RC_OK)
#endif /* VTSS_SW_OPTION_LACP */
           ) {
            continue;
        }

        found = TRUE;
        break;
    }/* for */
    return found;
#else
    return FALSE;
#endif/* VTSS_SW_OPTION_AGGR */
}


/*
 * GLAG iterator function.
 */
static mesa_rc vtss_ifindex_iterator_glag(vtss_ifindex_t startIfindex,
                                          const vtss_ifindex_t *prev_ifindex,
                                          vtss_ifindex_t *next_ifindex,
                                          BOOL check_exist)
{
    vtss_ifindex_t              ifindex;
    u32                         ordinal;
    mesa_rc                     rc = VTSS_RC_ERROR;


    if (prev_ifindex == NULL || *prev_ifindex < VTSS_IFINDEX_GLAG_OFFSET) {
        /* first */
        ordinal = VTSS_GLAG_NO_START;
    } else {
        /* next */
        ordinal = (vtss_ifindex_cast_to_u32(*prev_ifindex) - VTSS_IFINDEX_START_ + 1) - VTSS_IFINDEX_GLAG_OFFSET_;
    }

    for (; ordinal < VTSS_AGGR_GROUP_NO_END; ordinal++) {
        if (vtss_ifindex_from_glag(ordinal, &ifindex) == VTSS_RC_OK) {
            if (check_exist) {
                if (_vtss_ifindex_is_glag_exist(ordinal) == FALSE) {
                    continue;
                }
            }
            *next_ifindex = ifindex;
            rc = VTSS_RC_OK;
            break;
        }
    }/* for */

    return rc;
}
#endif

/*
 * VLAN iterator function.
 */
static mesa_rc vtss_ifindex_iterator_vlan(vtss_ifindex_t startIfindex,
                                          const vtss_ifindex_t *prev_ifindex,
                                          vtss_ifindex_t *next_ifindex,
                                          BOOL check_exist)
{
    vtss_ifindex_t          ifindex;
    u32                     ordinal;
    mesa_rc                 rc = VTSS_RC_ERROR;
    vtss_appl_vlan_entry_t  vlan_mgmt_entry;

    if (prev_ifindex == NULL || *prev_ifindex < VTSS_IFINDEX_VLAN_OFFSET) {
        /* first */
        ordinal = VTSS_VID_NULL + 1;
    } else {
        /* next */
        ordinal = (vtss_ifindex_cast_to_u32(*prev_ifindex) + 1) - VTSS_IFINDEX_VLAN_OFFSET_;
    }

    if (check_exist == FALSE) {
        for (; ordinal < VTSS_VIDS; ordinal++) {
            if (vtss_ifindex_from_vlan(ordinal, &ifindex) == VTSS_RC_OK) {
                *next_ifindex = ifindex;
                rc = VTSS_RC_OK;
                break;
            }
        }/* for */
    } else {
        vlan_mgmt_entry.vid = ordinal - 1;
        if (vtss_appl_vlan_get(VTSS_ISID_GLOBAL, vlan_mgmt_entry.vid, &vlan_mgmt_entry, TRUE, VTSS_APPL_VLAN_USER_ALL) == VTSS_RC_OK) {
            if (vtss_ifindex_from_vlan(vlan_mgmt_entry.vid, &ifindex) == VTSS_RC_OK) {
                *next_ifindex = ifindex;
                rc = VTSS_RC_OK;
            }
        }
    }

    return rc;
}

/*
 * Foreign iterator function.
 */
static mesa_rc vtss_ifindex_iterator_foreign(vtss_ifindex_t startIfindex,
                                             const vtss_ifindex_t *prev_ifindex,
                                             vtss_ifindex_t *next_ifindex,
                                             BOOL check_exist)
{
    vtss_ifindex_t      ifindex;
    u32                 ordinal;
    mesa_rc             rc = VTSS_RC_ERROR;

    if (prev_ifindex == NULL || *prev_ifindex < VTSS_IFINDEX_FOREIGN_OFFSET) {
        /* first */
        ordinal = 0;
    } else {
        ordinal = (vtss_ifindex_cast_to_u32(*prev_ifindex) + 1) - VTSS_IFINDEX_FOREIGN_OFFSET_;
    }

    if (check_exist == FALSE) {
        for (; ordinal < VTSS_APPL_FOREIGN_MAX; ++ordinal) {
            if (vtss_ifindex_from_foreign(ordinal, &ifindex) == VTSS_RC_OK) {
                *next_ifindex = ifindex;
                rc = VTSS_RC_OK;
                break;
            }
        }
    } else {
        /* TODO */
    }
    return rc;
}

/*
 * Frr vlink interator function
 */
static mesa_rc vtss_ifindex_iterator_frr_vlink(vtss_ifindex_t startIfindex,
                                               const vtss_ifindex_t *prev_ifindex,
                                               vtss_ifindex_t *next_ifindex,
                                               BOOL check_exist)
{
    vtss_ifindex_t      ifindex;
    u32                 ordinal;
    mesa_rc             rc = VTSS_RC_ERROR;

    if (prev_ifindex == NULL || *prev_ifindex < VTSS_IFINDEX_FRR_VLINK_OFFSET) {
        /* first */
        ordinal = 1;
    } else {
        ordinal = (vtss_ifindex_cast_to_u32(*prev_ifindex) + 1) - VTSS_IFINDEX_FRR_VLINK_OFFSET_;
    }

    if (check_exist == FALSE) {
        for (; ordinal < VTSS_APPL_FRR_VLINK_MAX; ++ordinal) {
            if (vtss_ifindex_from_frr_vlink(ordinal, &ifindex) == VTSS_RC_OK) {
                *next_ifindex = ifindex;
                rc = VTSS_RC_OK;
                break;
            }
        }
    } else {
        /* TODO */
    }
    return rc;
}

#ifdef VTSS_SW_OPTION_CPUPORT
/*
 * Cpu iterator function.
 */
static mesa_rc vtss_ifindex_iterator_cpu(vtss_ifindex_t startIfindex,
                                             const vtss_ifindex_t *prev_ifindex,
                                             vtss_ifindex_t *next_ifindex,
                                             BOOL check_exist)
{
    return vtss_ifindex_iterator_cpu(prev_ifindex, next_ifindex);
}
#endif

/** Interface iterator function.
 *
 * \param prev_ifindex [IN] Previous interface index
 *
 * \param next_ifindex [OUT] next interface index
 *
 * \param enumerate_types [IN] Specify VTSS_IFINDEX_GETNEXT_xxx to filter by interface type when iterates interface index.
 *
 * \param check_exist [IN] TRUE - will verify the interface index is currently present or not.
 *                         The interface index will be skipped if it is not present in the system.
 *
 * \return VTSS_RC_OK if the next interface index is found.
 */
mesa_rc vtss_ifindex_iterator(const vtss_ifindex_t  *prev_ifindex,
                              vtss_ifindex_t        *next_ifindex,
                              u32                   enumerate_types,
                              BOOL                  check_exist)
{
    vtss_ifindex_iterator_cb_ent_t      tmp_ent, *cb_ent;
    mesa_rc rc = VTSS_RC_ERROR;

    /* ifindex_iterator_cb_avlt is initialed at system init and will not add or delete at runtime.
     * So, there is no semaphore protection here.
     * If the ifindex_iterator_cb_avlt changes design to dynamically update at runtime,
     * semaphore protect must be implement here!
     */
    /* Critical-Section-Lock */

    if (prev_ifindex == NULL || (*prev_ifindex < VTSS_IFINDEX_START)) {
        if (vtss_avl_tree_get(&ifindex_iterator_cb_avlt, (void **)&cb_ent, VTSS_AVL_TREE_GET_FIRST) == FALSE) {
            /* Critical-Section-Unlock */
            return VTSS_RC_ERROR;
        }
    } else {
        tmp_ent.startIfindex = *prev_ifindex + 1;
        cb_ent = &tmp_ent;
        if (vtss_avl_tree_get(&ifindex_iterator_cb_avlt, (void **)&cb_ent, VTSS_AVL_TREE_GET_PREV) == FALSE) {
            /* Critical-Section-Unlock */
            return VTSS_RC_ERROR;
        }
    }

    do {
        IF_T_D("cb(%d,%d)", VTSS_IFINDEX_PRINTF_ARG(cb_ent->startIfindex), cb_ent->type);

        if ( !VTSS_IFINDEX_TYPE_CHK(enumerate_types, cb_ent->type) ) {
            IF_T_D("cb(%d,%d) type dismatch", VTSS_IFINDEX_PRINTF_ARG(cb_ent->startIfindex), cb_ent->type);
            continue;
        }
        rc = cb_ent->iterator_cb_fn(cb_ent->startIfindex, prev_ifindex, next_ifindex, check_exist);
        if (rc == VTSS_RC_OK) {
            break;
        }
    } while(vtss_avl_tree_get(&ifindex_iterator_cb_avlt, (void **)&cb_ent, VTSS_AVL_TREE_GET_NEXT));

    /* Critical-Section-Unlock */

    return rc;
}


static inline BOOL vtss_ifindex_is_valid_isid(vtss_isid_t isid)
{
    return VTSS_ISID_LEGAL(isid);
}

static inline BOOL vtss_ifindex_is_valid_usid(vtss_usid_t usid)
{
    return VTSS_USID_LEGAL(usid);
}


static inline BOOL vtss_ifindex_is_valid_ordinal(u32 ordinal, u32 offset, u32 range) {
    return (ordinal >= offset) && (ordinal < (offset + range));
}

static inline BOOL vtss_ifindex_is_valid_ordinal(vtss_ifindex_t ordinal, vtss_ifindex_t offset, u32 range) {
    return vtss_ifindex_is_valid_ordinal(vtss_ifindex_cast_to_u32(ordinal), vtss_ifindex_cast_to_u32(offset), range);
}

/*
 * Transfer USID/Port to ifIndex. port_no is uport
 */
static void _vtss_ifindex_from_usid_uport(const vtss_usid_t usid, const vtss_uport_no_t port_no,
                                          vtss_ifindex_t *ifindex)
{
    *ifindex = vtss_ifindex_t{(usid * VTSS_IFINDEX_UNIT_MULTP_) + VTSS_IFINDEX_PORT_UNIT_OFFSET_ + port_no};
}

BOOL vtss_ifindex_is_none(vtss_ifindex_t ifindex) {
    return ifindex == VTSS_IFINDEX_NONE;
}

BOOL vtss_ifindex_is_port(vtss_ifindex_t ifindex)
{
    vtss_usid_t usid = vtss_ifindex_cast_to_u32(ifindex) / VTSS_IFINDEX_UNIT_MULTP_;
    vtss_uport_no_t  uport = (vtss_ifindex_cast_to_u32(ifindex) % VTSS_IFINDEX_UNIT_MULTP_) - VTSS_IFINDEX_PORT_UNIT_OFFSET_;
    u32 ordinal = uport2iport(uport);
    return vtss_ifindex_is_valid_usid(usid) &&
            vtss_ifindex_is_valid_ordinal(ordinal, VTSS_PORT_NO_START, fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT));
}

BOOL vtss_ifindex_is_llag(vtss_ifindex_t ifindex)
{
    vtss_isid_t usid = (vtss_ifindex_cast_to_u32(ifindex) - VTSS_IFINDEX_START_) / VTSS_IFINDEX_UNIT_MULTP_;
    u32 ordinal = (vtss_ifindex_cast_to_u32(ifindex) - VTSS_IFINDEX_START_) % VTSS_IFINDEX_UNIT_MULTP_;
    return vtss_ifindex_is_valid_usid(usid) &&
            vtss_ifindex_is_valid_ordinal(ordinal, VTSS_IFINDEX_LLAG_UNIT_OFFSET_, VTSS_AGGR_GROUP_NO_END + 1);
}

BOOL vtss_ifindex_is_glag(vtss_ifindex_t ifindex)
{
#if defined(VTSS_GLAGS)
    return vtss_ifindex_is_valid_ordinal(ifindex - VTSS_IFINDEX_START, VTSS_IFINDEX_GLAG_OFFSET, VTSS_AGGR_GROUP_NO_END + 1);
#else
    return FALSE;
#endif
}

BOOL vtss_ifindex_is_vlan(vtss_ifindex_t ifindex)
{
    return vtss_ifindex_is_valid_ordinal(ifindex - VTSS_IFINDEX_START, VTSS_IFINDEX_VLAN_OFFSET, VTSS_VIDS);
}

BOOL vtss_ifindex_is_redbox_neighbor(vtss_ifindex_t ifindex) {
    return ifindex == VTSS_IFINDEX_REDBOX_NEIGHBOR;
}

BOOL vtss_ifindex_is_foreign(vtss_ifindex_t ifindex)
{
    return vtss_ifindex_is_valid_ordinal(ifindex - VTSS_IFINDEX_START, VTSS_IFINDEX_FOREIGN_OFFSET, VTSS_APPL_FOREIGN_MAX);
}

BOOL vtss_ifindex_is_frr_vlink(vtss_ifindex_t ifindex)
{
    return vtss_ifindex_is_valid_ordinal(ifindex - VTSS_IFINDEX_START, VTSS_IFINDEX_FRR_VLINK_OFFSET, VTSS_APPL_FRR_VLINK_MAX);
}

BOOL vtss_ifindex_is_cpu(vtss_ifindex_t ifindex)
{
    return vtss_ifindex_is_valid_ordinal(ifindex, VTSS_IFINDEX_CPU_OFFSET, VTSS_APPL_CPU_MAX);
}

static mesa_rc isid_local_compensate(vtss_isid_t &isid) {
    // return if we have no problem.
    if (VTSS_ISID_LEGAL(isid))
        return VTSS_RC_OK;

    // If the isid is not valid, and it is not "local", then we have an error
    if (isid != VTSS_ISID_LOCAL)
        return VTSS_RC_ERROR;

    isid = VTSS_ISID_START;
    return VTSS_RC_OK;
}

static mesa_rc usid_local_compensate(vtss_usid_t &usid) {
    // return if we have no problem.
    if (VTSS_USID_LEGAL(usid))
        return VTSS_RC_OK;

    // If the usid is not valid, and it is not "local", then we have an error
    if (usid != VTSS_USID_LOCAL)
        return VTSS_RC_ERROR;

    // usid local is the same as isid local!
    vtss_isid_t isid = VTSS_ISID_LOCAL;
    VTSS_RC(isid_local_compensate(isid));

    // convert back to usid
    usid = topo_isid2usid(isid);
    return VTSS_RC_OK;
}

/*
 * Port ifindex is composed from USID and Port number.
 *  usid    [IN]    USID value of the switch in which the port resides on.
 *  uport   [IN]    User view port number
 *  ifindex [OUT]   interface index
 */
mesa_rc vtss_ifindex_from_usid_uport(vtss_usid_t usid, vtss_uport_no_t uport, vtss_ifindex_t *ifindex)
{
    mesa_rc        rc = VTSS_RC_ERROR;
    mesa_port_no_t port_no;

    VTSS_RC(usid_local_compensate(usid));

    port_no = uport2iport(uport);
    if (vtss_ifindex_is_valid_ordinal(port_no, VTSS_PORT_NO_START, port_count_max())) {
        _vtss_ifindex_from_usid_uport(usid, uport, ifindex);
        rc = VTSS_RC_OK;
    }

    return rc;
}

/*
 * Port ifindex is composed from USID and Port number + 1.
 *  isid    [IN]    ISID value of the switch in which the port resides on.
 *  port_no [IN]    Physical port number (0 based)
 *  ifindex [OUT]   interface index
 */
mesa_rc vtss_ifindex_from_port(vtss_isid_t isid, mesa_port_no_t port_no, vtss_ifindex_t *ifindex)
{
    mesa_rc rc = VTSS_RC_ERROR;
    vtss_usid_t     usid;

    VTSS_RC(isid_local_compensate(isid));

    if (vtss_ifindex_is_valid_ordinal(port_no, VTSS_PORT_NO_START, port_count_max())) {
        usid = topo_isid2usid(isid);
        _vtss_ifindex_from_usid_uport(usid, iport2uport(port_no), ifindex);
        IF_T_D("isid=%d iport=%d usid=%d --> ifindex %u", isid, port_no, usid, VTSS_IFINDEX_PRINTF_ARG(*ifindex));
        rc = VTSS_RC_OK;
    } else if (vtss_ifindex_is_valid_ordinal(port_no, port_count_max(), fast_cap(MEBA_CAP_CPU_PORTS_COUNT))) {
        rc = vtss_ifindex_from_cpu(port_no - port_count_max(), ifindex);
    }
    return rc;
}

/*
 * LLAG ifindex is composed from USID and LLAG number + 1.
 *  usid    [IN]    USID value of the switch in which the LLAG resides on.
 *  llag_no [IN]    Physical LLAG number (0 based)
 *  ifindex [OUT]   interface index
 */
mesa_rc vtss_ifindex_from_usid_llag(vtss_usid_t usid, u32 llag_no, vtss_ifindex_t *ifindex)
{
    mesa_rc rc = VTSS_RC_ERROR;

    VTSS_RC(usid_local_compensate(usid));

    if (vtss_ifindex_is_valid_ordinal(llag_no, VTSS_AGGR_NO_START + 1, VTSS_AGGR_GROUP_NO_END + 1)) {
        *ifindex = vtss_ifindex_t{(usid * VTSS_IFINDEX_UNIT_MULTP_) + VTSS_IFINDEX_LLAG_UNIT_OFFSET_ + llag_no};
        rc = VTSS_RC_OK;
    }

    return rc;
}

/*
 * LLAG ifindex is composed from USID and LLAG number.
 *  isid    [IN]    ISID value of the switch in which the LLAG resides on.
 *  llag_no [IN]    Physical LLAG number (start from 1)
 *  ifindex [OUT]   interface index
 */

mesa_rc vtss_ifindex_from_llag(vtss_isid_t isid, u32 llag_no, vtss_ifindex_t *ifindex)
{
    mesa_rc rc = VTSS_RC_ERROR;
#if defined(VTSS_SW_OPTION_AGGR)
    vtss_usid_t     usid;

    VTSS_RC(isid_local_compensate(isid));

    if (vtss_ifindex_is_valid_ordinal(llag_no, AGGR_MGMT_GROUP_NO_START, AGGR_MGMT_GROUP_NO_END_)) {
        usid = topo_isid2usid(isid);
        *ifindex = vtss_ifindex_t{(usid * VTSS_IFINDEX_UNIT_MULTP_) + VTSS_IFINDEX_LLAG_UNIT_OFFSET_ + llag_no};
        rc = VTSS_RC_OK;
    }
#endif
    return rc;
}

mesa_rc vtss_ifindex_from_glag(u32 glag_no, vtss_ifindex_t *ifindex)
{
    mesa_rc rc = VTSS_RC_ERROR;
#if defined(VTSS_GLAGS)
    if (vtss_ifindex_is_valid_ordinal(glag_no, VTSS_GLAG_NO_START, VTSS_AGGR_GROUP_NO_END + 1)) {
        *ifindex = VTSS_IFINDEX_START + VTSS_IFINDEX_GLAG_OFFSET + glag_no;
        rc = VTSS_RC_OK;
    }
#endif
    return rc;
}

mesa_rc vtss_ifindex_from_vlan(mesa_vid_t vlan_no, vtss_ifindex_t *ifindex)
{
    mesa_rc rc = VTSS_RC_ERROR;
    if (vtss_ifindex_is_valid_ordinal(vlan_no, VTSS_VID_NULL, VTSS_VIDS)) {
        *ifindex = VTSS_IFINDEX_VLAN_OFFSET + vlan_no;
        rc = VTSS_RC_OK;
    }
    return rc;
}

mesa_rc vtss_ifindex_from_foreign(u32 foreign_no, vtss_ifindex_t *ifindex)
{
    mesa_rc rc = VTSS_RC_ERROR;
    if (vtss_ifindex_is_valid_ordinal(foreign_no, 1, VTSS_APPL_FOREIGN_MAX)) {
        *ifindex = VTSS_IFINDEX_FOREIGN_OFFSET + foreign_no;
        rc = VTSS_RC_OK;
    }
    return rc;
}

mesa_rc vtss_ifindex_from_frr_vlink(u32 frr_vlink_no, vtss_ifindex_t *ifindex)
{
    mesa_rc rc = VTSS_RC_ERROR;
    if (vtss_ifindex_is_valid_ordinal(frr_vlink_no, 1, VTSS_APPL_FRR_VLINK_MAX)) {
        *ifindex = VTSS_IFINDEX_FRR_VLINK_OFFSET + frr_vlink_no;
        rc = VTSS_RC_OK;
    }
    return rc;
}

mesa_rc vtss_ifindex_from_cpu(u32 idx, vtss_ifindex_t *ifindex)
{
    mesa_rc rc = VTSS_RC_ERROR;
    if (vtss_ifindex_is_valid_ordinal(idx, 0, fast_cap(MEBA_CAP_BOARD_PORT_COUNT))) {
        *ifindex = VTSS_IFINDEX_CPU_OFFSET + idx;
        rc = VTSS_RC_OK;
    }
    return rc;
}

vtss_ifindex_type_t vtss_ifindex_type(vtss_ifindex_t ifindex)
{
    if (vtss_ifindex_is_none(ifindex)) {
        return VTSS_IFINDEX_TYPE_NONE;
    } else if (vtss_ifindex_is_port(ifindex)) {
        return VTSS_IFINDEX_TYPE_PORT;
    } else if (vtss_ifindex_is_llag(ifindex)) {
        return VTSS_IFINDEX_TYPE_LLAG;
    } else if (vtss_ifindex_is_glag(ifindex)) {
        return VTSS_IFINDEX_TYPE_GLAG;
    } else if (vtss_ifindex_is_vlan(ifindex)) {
        return VTSS_IFINDEX_TYPE_VLAN;
    } else if (vtss_ifindex_is_redbox_neighbor(ifindex)) {
        return VTSS_IFINDEX_TYPE_REDBOX_NEIGHBOR;
    } else if (vtss_ifindex_is_foreign(ifindex)) {
        return VTSS_IFINDEX_TYPE_FOREIGN;
    } else if (vtss_ifindex_is_frr_vlink(ifindex)) {
        return VTSS_IFINDEX_TYPE_FRR_VLINK;
    } else if (vtss_ifindex_is_cpu(ifindex)) {
        return VTSS_IFINDEX_TYPE_CPU;
    }

    return VTSS_IFINDEX_TYPE_ILLEGAL;
}


mesa_rc vtss_ifindex_decompose(vtss_ifindex_t ifindex_, vtss_ifindex_elm_t *ife)
{
    u32 ifindex = vtss_ifindex_cast_to_u32(ifindex_);
    vtss_ifindex_type_t iftype = vtss_ifindex_type(ifindex_);
    switch (iftype) {
        case VTSS_IFINDEX_TYPE_NONE:
        case VTSS_IFINDEX_TYPE_REDBOX_NEIGHBOR:
            ife->isid = 0;
            ife->usid = 0;
            ife->ordinal = 0;
            break;
        case VTSS_IFINDEX_TYPE_PORT:
            ife->usid = (ifindex / VTSS_IFINDEX_UNIT_MULTP_);
            ife->isid = topo_usid2isid(ife->usid);
            ife->ordinal = uport2iport((ifindex % VTSS_IFINDEX_UNIT_MULTP_) - VTSS_IFINDEX_PORT_UNIT_OFFSET_);
            break;
        case VTSS_IFINDEX_TYPE_LLAG:
            ife->usid = (ifindex / VTSS_IFINDEX_UNIT_MULTP_);
            ife->isid = topo_usid2isid(ife->usid);
            ife->ordinal = (ifindex % VTSS_IFINDEX_UNIT_MULTP_) - VTSS_IFINDEX_LLAG_UNIT_OFFSET_;
            break;
#if defined(VTSS_GLAGS)
        case VTSS_IFINDEX_TYPE_GLAG:
            ife->usid = VTSS_USID_START;
            ife->isid = VTSS_ISID_START;
            ife->ordinal = ifindex - VTSS_IFINDEX_START_ - VTSS_IFINDEX_GLAG_OFFSET_;
            break;
#endif
        case VTSS_IFINDEX_TYPE_VLAN:
            ife->usid = VTSS_USID_START;
            ife->isid = VTSS_ISID_START;
            ife->ordinal = ifindex - VTSS_IFINDEX_VLAN_OFFSET_;
            break;
        case VTSS_IFINDEX_TYPE_FOREIGN:
            ife->usid = VTSS_USID_START;
            ife->isid = VTSS_ISID_START;
            ife->ordinal = ifindex - VTSS_IFINDEX_FOREIGN_OFFSET_;
            break;
        case VTSS_IFINDEX_TYPE_FRR_VLINK:
            ife->usid = VTSS_USID_START;
            ife->isid = VTSS_ISID_START;
            ife->ordinal = ifindex - VTSS_IFINDEX_FRR_VLINK_OFFSET_;
            break;
        case VTSS_IFINDEX_TYPE_CPU:
            ife->usid = VTSS_USID_START;
            ife->isid = VTSS_ISID_START;
            ife->ordinal = ifindex - VTSS_IFINDEX_CPU_OFFSET_;
            break;
        default:
            return VTSS_RC_ERROR;  /* Illegal ifindex */
    }
    /* All well, return decomposed ifindex */
    ife->iftype = iftype;
    return VTSS_RC_OK;
}

mesa_vid_t vtss_ifindex_as_vlan(vtss_ifindex_t ifindex)
{
    if (!vtss_ifindex_is_vlan(ifindex))
        return 0;

    u32 ifidx = vtss_ifindex_cast_to_u32(ifindex);
    return ifidx - VTSS_IFINDEX_VLAN_OFFSET_;
}


/*
 * Getnext interface index
 */
mesa_rc vtss_ifindex_getnext(const vtss_ifindex_t   *prev_ifindex,
                             vtss_ifindex_t         *next_ifindex,
                             BOOL                   check_exist)
{
    return vtss_ifindex_iterator(prev_ifindex, next_ifindex, VTSS_IFINDEX_GETNEXT_ALL, check_exist);
}


/*
 * Getnext interface index by specified interface type(s)
 */
mesa_rc vtss_ifindex_getnext_by_type(const vtss_ifindex_t *prev_ifindex,
                                     vtss_ifindex_t *next_ifindex,
                                     u32 enumerate_types)
{
    return vtss_ifindex_iterator(prev_ifindex, next_ifindex, enumerate_types, FALSE);
}

char *vtss_ifindex2str(char *buffer,
                       size_t size,
                       vtss_ifindex_t ifindex)
{
    vtss_ifindex_elm_t ife;
    mesa_rc rc;
    if ((rc = vtss_ifindex_decompose(ifindex, &ife)) == VTSS_RC_OK) {
        switch(ife.iftype) {
            case VTSS_IFINDEX_TYPE_NONE:
                snprintf(buffer, size, "NONE");
                break;
            case VTSS_IFINDEX_TYPE_PORT:
                snprintf(buffer, size, "Port %d/%d", ife.usid, iport2uport(ife.ordinal));
                break;
            case VTSS_IFINDEX_TYPE_LLAG:
                snprintf(buffer, size, "LLAG %d/%d", ife.usid, ife.ordinal + 1);
                break;
#if defined(VTSS_GLAGS)
            case VTSS_IFINDEX_TYPE_GLAG:
                snprintf(buffer, size, "GLAG %d (%d)", ife.ordinal + 1, ife.ordinal);
                break;
#endif
            case VTSS_IFINDEX_TYPE_VLAN:
                snprintf(buffer, size, "VLAN %d", ife.ordinal);
                break;
            case VTSS_IFINDEX_TYPE_REDBOX_NEIGHBOR:
                snprintf(buffer, size, "RedBox-Neighbor");
                break;
            case VTSS_IFINDEX_TYPE_CPU:
                snprintf(buffer, size, "CPU %d/%d", ife.usid, iport2uport(ife.ordinal));
                break;
            case VTSS_IFINDEX_TYPE_FRR_VLINK:
                snprintf(buffer, size, "OSPF VLINK %d", ife.ordinal);
                break;
            default:
                snprintf(buffer, size, "%d: Illegal interface index", VTSS_IFINDEX_PRINTF_ARG(ifindex));
        }
    }
    return buffer;
}

#ifdef VTSS_SW_OPTION_WEB
mesa_rc vtss_str2ifindex(const char *buffer, int len, vtss_ifindex_t *ifindex)
{
    vtss::AsInterfaceIndex as_ifidx(*ifindex);
    if (len == 0) {
        len = strlen(buffer);
    }

    return parse(buffer, buffer+len, as_ifidx) ? VTSS_RC_OK : VTSS_RC_ERROR;
};
#endif //VTSS_SW_OPTION_WEB

/*
 * Getnext port interface index
 */
mesa_rc vtss_ifindex_getnext_port(const vtss_ifindex_t *previous,
                                  vtss_ifindex_t *next)
{
     return vtss_ifindex_iterator(previous, next, VTSS_IFINDEX_GETNEXT_PORTS | VTSS_IFINDEX_GETNEXT_CPU, FALSE);
};


/*
 * Getnext exist port interface index
 */
mesa_rc vtss_ifindex_getnext_port_exist(const vtss_ifindex_t *previous,
                                        vtss_ifindex_t *next)
{
    return vtss_ifindex_iterator(previous, next, VTSS_IFINDEX_GETNEXT_PORTS | VTSS_IFINDEX_GETNEXT_CPU, TRUE);
};


/*
 * Getnext configureable port interface index
 */
mesa_rc vtss_ifindex_getnext_port_configurable(const vtss_ifindex_t *previous,
                                               vtss_ifindex_t *next)
{
    vtss_ifindex_elm_t  ife;
    vtss_ifindex_t      prev_tmp = VTSS_IFINDEX_NONE;

    if (previous) {
        prev_tmp = *previous;
    }

    while (vtss_ifindex_iterator(&prev_tmp, next, VTSS_IFINDEX_GETNEXT_PORTS, FALSE) == VTSS_RC_OK) {
        prev_tmp = *next;
        if (vtss_ifindex_decompose(*next, &ife) != VTSS_RC_OK) {
            continue;
        }

        if (ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
            continue;
        }

        if (msg_switch_configurable(ife.isid) == FALSE) {
            prev_tmp = vtss_ifindex_t{(ife.usid + 1) * VTSS_IFINDEX_UNIT_MULTP_ + VTSS_IFINDEX_PORT_UNIT_OFFSET_};
        } else {
            return VTSS_RC_OK;
        }

    }/* while */

    while (vtss_ifindex_iterator(&prev_tmp, next, VTSS_IFINDEX_GETNEXT_CPU, FALSE) == VTSS_RC_OK) {
        prev_tmp = *next;
        if (vtss_ifindex_decompose(*next, &ife) != VTSS_RC_OK) {
            continue;
        }

        if (ife.iftype != VTSS_IFINDEX_TYPE_CPU) {
            continue;
        }

        if (msg_switch_configurable(ife.isid) == FALSE) {
            prev_tmp = vtss_ifindex_t{(ife.usid + 1) * VTSS_IFINDEX_UNIT_MULTP_ + VTSS_IFINDEX_CPU_OFFSET_};
        } else {
            return VTSS_RC_OK;
        }

    }/* while */

    return VTSS_RC_ERROR;
}

// Generic iterator for ifIndex,queue
mesa_rc vtss_ifindex_getnext_port_queue(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                        const mesa_prio_t    *prev_queue,   mesa_prio_t    *next_queue)
{
    if (!prev_queue) {
        *next_queue = 0;
    } else {
        if (*prev_queue >= (VTSS_PRIOS - 1)) {
            *next_queue = 0;
        } else {
            *next_ifindex = *prev_ifindex;
            *next_queue   = *prev_queue + 1;
            return VTSS_RC_OK;
        }
    }

    return vtss_appl_iterator_ifindex_front_port_exist(prev_ifindex, next_ifindex);
}

#if defined(VTSS_BASICS_STANDALONE)
/* We declare the following definition for the unit-test only.
 * Since the unit-test compiler environment is a pure software process,
 * it does not include the hardware declared header files.
 */
// Refer to vtss/appl/vlan.h
#define VTSS_APPL_VLAN_ID_MAX 4095

// Refer to vtss/appl/interface.h
#define VTSS_IFINDEX_FRR_VLINK_OFFSET_ 800000000
#define VTSS_IFINDEX_FRR_VLINK_MAX_ 100000000

/* (For unit-test purpose)
 * Parse the OS interface name.
 * For VLAN interface e.g. vtss.vlan.2, the ifIndex is 2
 * For VLINK interface e.g. VLINK2, the ifIndex is 800000002
 * default ifIndex is 0.
 */
vtss_ifindex_t vtss_ifindex_from_os_ifname(const std::string &os_ifname)
{
    vtss::parser::Lit lit_vlan("vtss.vlan.");
    vtss::parser::IntUnsignedBase10<uint32_t, 1, 4> index;
    vtss::parser::Lit lit_vlink("VLINK");
    const char *b = &*os_ifname.begin();
    const char *e = &*os_ifname.end();
    vtss_ifindex_t vtss_ifindex;

    IF_T_D("Converting '%s' to ifIndex", os_ifname.c_str());

    /* VTSS_IFINDEX_NONE is returned for empty 'os_ifname'. */
    if (!os_ifname.size()) {
        return VTSS_IFINDEX_NONE;
    }

    if (vtss::parser::Group(b, e, lit_vlan, index)) {
        return vtss_ifindex_t{index.get()};
    } else if (vtss::parser::Group(b, e, lit_vlink, index)) {
        return vtss_ifindex_t{VTSS_IFINDEX_FRR_VLINK_OFFSET_ + index.get()};
    }

    return vtss_ifindex_t{0};
}
#else
/**
 * \brief Convert vtss.vlan.<vid> into vtss_ifindex.
 * \param os_ifname     [IN] The ifname in FRR layer(e.g., vtss.vlan.<vid>,
 * VLINK0, etc.).
 * \return VTSS ifindex. The foreign ifindex is invalid for FRR, the caller
 * should use frr_vtss_ifindex_isvalid() to check if the returned ifindex is
 * valid or not. If the foreign ifindex is returned, the caller can identify
 * the failed reason if
 * ifindex == 709004096 - The failure due to unknown OS interface name, else if
 * ifindex >= 702000000 - The failure due to VLINK type conversion, else
 * ifindex >= 701000000 - The failure due to VLAN type conversion.
 */
vtss_ifindex_t vtss_ifindex_from_os_ifname(const std::string &os_ifname)
{
    vtss::parser::Lit lit_vlan("vtss.vlan.");
    vtss::parser::IntUnsignedBase10<uint32_t, 1, 4> index;
    vtss::parser::Lit lit_vlink("VLINK");
    const char *b = &*os_ifname.begin();
    const char *e = os_ifname.c_str() + os_ifname.size();
    vtss_ifindex_t vtss_ifindex;

    IF_T_D("Converting %s to ifIndex", os_ifname.c_str());

    /* VTSS_IFINDEX_NONE is returned for empty 'os_ifname'. */
    if (!os_ifname.size()) {
        return VTSS_IFINDEX_NONE;
    }

    if (vtss::parser::Group(b, e, lit_vlan, index)) {
        if (vtss_ifindex_from_vlan(index.get(), &vtss_ifindex) != MESA_RC_OK) {
            // The specific foreign ID to indicate failed conversion,
            // (index + 1000000) are chosen as the input.
            // Since it doesn't exceed VTSS_IFINDEX_FOREIGN_OFFSET_, the error
            // handler can be ignored.
            (void)vtss_ifindex_from_foreign(index.get() + VTSS_IFINDEX_CONVERT_VLAN_FAILED_, &vtss_ifindex);
            IF_T_E("VLAN %u convert failed", index.get());
        }
    } else if (vtss::parser::Group(b, e, lit_vlink, index)) {
        if (vtss_ifindex_from_frr_vlink(index.get() + 1, &vtss_ifindex)) {
            // To distinguish from the above error, the ifindex are converted
            // from (index + 1 + 2000000).
            // Since it doesn't exceed VTSS_IFINDEX_FOREIGN_OFFSET_, the error
            // handler can be ignored.
            (void)vtss_ifindex_from_foreign(index.get() + VTSS_IFINDEX_CONVERT_VLINK_FAILED_, &vtss_ifindex);
            IF_T_E("VLINK %u convert failed", index.get());
        }
    } else {
        // The specific foreign ID to indicate failed conversion,
        // (VTSS_APPL_VLAN_ID_MAX + 1 + 9000000) are choosed as the input.
        // Since it doesn't exceed VTSS_IFINDEX_FOREIGN_OFFSET_, the error
        // handler can be ignored.
        (void)vtss_ifindex_from_foreign(VTSS_IFINDEX_CONVERT_OS_IF_NAME_FAILED_, &vtss_ifindex);
        IF_T_E("%s convert failed", os_ifname.c_str());
    }

    return vtss_ifindex;
}
#endif /* VTSS_BASICS_STANDALONE */

#if defined(VTSS_BASICS_STANDALONE)
vtss_ifindex_t vtss_ifindex_from_os_ifindex(unsigned int os_ifindex) {
    return vtss_ifindex_t{0};
}
#else
vtss_ifindex_t vtss_ifindex_from_os_ifindex(unsigned int os_ifindex)
{
    char buf[4096];

    // os_ifindex is the number you get if you do an "ip link" in a shell. E.g.
    // 1, 2, 3, and 4 in the following:
    // $ ip link
    // 1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue qlen 1000
    //     link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    // 2: vtss.ifh: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1526 qdisc pfifo_fast qlen 1000
    //     link/ether da:ab:48:69:b8:b4 brd ff:ff:ff:ff:ff:ff
    // 3: sit0@NONE: <NOARP> mtu 1480 qdisc noop qlen 1000
    //     link/sit 0.0.0.0 brd 0.0.0.0
    // 4: vtss.vlan.1: <BROADCAST,MULTICAST> mtu 1500 qdisc noop qlen 1000
    //     link/ether 00:01:c1:01:c7:b0 brd ff:ff:ff:ff:ff:ff

    // If os_ifindex is a known interface (e.g. vtss.vlan.1), it will be
    // converted to a VLAN interface.
    // If os_ifindex is NOT known, it will be converted to an ifindex of type
    // VTSS_IFINDEX_TYPE_FOREIGN.
    if (os_ifindex == 0) {
        return vtss_ifindex_cast_from_u32(0, VTSS_IFINDEX_TYPE_NONE);
    }

    if (if_indextoname(os_ifindex, buf) == NULL) {
        vtss_ifindex_t vtss_ifindex;
        // To distinguish from the errors reported in
        // vtss_ifindex_from_os_ifname(), the ifindex is converted from
        // (VTSS_APPL_VLAN_ID_MAX +1 + 3000000).
        // Since it doesn't exceed VTSS_IFINDEX_FOREIGN_OFFSET_, the error
        // handler can be ignored.
        (void)vtss_ifindex_from_foreign(VTSS_IFINDEX_CONVERT_OS_IF_ID_FAILED_, &vtss_ifindex);
        return vtss_ifindex;
    }

    std::string os_ifname(buf);
    return vtss_ifindex_from_os_ifname(os_ifname);
}
#endif /* VTSS_BASICS_STANDALONE */

/*
 * Interface ifindex initial
 */
void vtss_ifindex_init(void)
{
    u32                             usid;
    vtss_ifindex_iterator_cb_ent_t  cb_info;

    if ( vtss_avl_tree_init(&ifindex_iterator_cb_avlt) == FALSE ) {
        IF_T_E("Fail to create AVL tree for ifindex_iterator_cb_avlt");
        return;
    }

    memset(&cb_info, 0, sizeof(vtss_ifindex_iterator_cb_ent_t));

    cb_info.startIfindex    = VTSS_IFINDEX_VLAN_OFFSET;
    cb_info.type            = VTSS_IFINDEX_TYPE_VLAN;
    cb_info.iterator_cb_fn  = vtss_ifindex_iterator_vlan;
    if (vtss_ifindex_add_iterator_cb(&cb_info) != VTSS_RC_OK) {
        IF_T_E("vlan insert iterator cb error");
    }

    cb_info.startIfindex    = VTSS_IFINDEX_FOREIGN_OFFSET;
    cb_info.type            = VTSS_IFINDEX_TYPE_FOREIGN;
    cb_info.iterator_cb_fn  = vtss_ifindex_iterator_foreign;
    if (vtss_ifindex_add_iterator_cb(&cb_info) != VTSS_RC_OK) {
        IF_T_E("foreign insert iterator cb error");
    }

    cb_info.startIfindex    = VTSS_IFINDEX_FRR_VLINK_OFFSET;
    cb_info.type            = VTSS_IFINDEX_TYPE_FRR_VLINK;
    cb_info.iterator_cb_fn  = vtss_ifindex_iterator_frr_vlink;
    if (vtss_ifindex_add_iterator_cb(&cb_info) != VTSS_RC_OK) {
        IF_T_E("frr vlink intert iterator cb error");
    }

#ifdef VTSS_SW_OPTION_CPUPORT
    cb_info.startIfindex    = VTSS_IFINDEX_CPU_OFFSET;
    cb_info.type            = VTSS_IFINDEX_TYPE_CPU;
    cb_info.iterator_cb_fn  = vtss_ifindex_iterator_cpu;
    if (vtss_ifindex_add_iterator_cb(&cb_info) != VTSS_RC_OK) {
        IF_T_E("cpu insert iterator cb error");
    }
#endif

#if defined(VTSS_GLAGS)
    cb_info.startIfindex    = VTSS_IFINDEX_GLAG_OFFSET;
    cb_info.type            = VTSS_IFINDEX_TYPE_GLAG;
    cb_info.iterator_cb_fn  = vtss_ifindex_iterator_glag;
    if (vtss_ifindex_add_iterator_cb(&cb_info) != VTSS_RC_OK) {
        IF_T_E("glag insert iterator cb error");
    }
#endif

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        cb_info.startIfindex    = vtss_ifindex_t{VTSS_IFINDEX_PORT_UNIT_OFFSET_ + usid * VTSS_IFINDEX_UNIT_MULTP_};
        cb_info.type            = VTSS_IFINDEX_TYPE_PORT;
        cb_info.iterator_cb_fn  = vtss_ifindex_iterator_front_port_per_usid;
        if (vtss_ifindex_add_iterator_cb(&cb_info) != VTSS_RC_OK) {
            IF_T_E("port/llag insert iterator cb error");
        }
        cb_info.startIfindex    = vtss_ifindex_t{VTSS_IFINDEX_LLAG_UNIT_OFFSET_ + usid * VTSS_IFINDEX_UNIT_MULTP_};
        cb_info.type            = VTSS_IFINDEX_TYPE_LLAG;
        cb_info.iterator_cb_fn  = vtss_ifindex_iterator_llag_per_usid;
        if (vtss_ifindex_add_iterator_cb(&cb_info) != VTSS_RC_OK) {
            IF_T_E("port/llag insert iterator cb error");
        }
    }/* for */

    return;
}

