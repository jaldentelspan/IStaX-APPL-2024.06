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

#include "vlan_serializer.hxx"
#include <vtss/appl/vlan.h>
#include "vlan_trace.h"
#include "vtss_common_iterator.hxx" /* For vtss_appl_iterator_ifindex_front_port() */
#include "port_iter.hxx"

//******************************************************************************
// Mapping between enum and text.
//******************************************************************************
vtss_enum_descriptor_t vlan_user_txt[] = {
    {VTSS_APPL_VLAN_USER_ALL,               "combined"   },
    {VTSS_APPL_VLAN_USER_STATIC,            "admin"      },
    {VTSS_APPL_VLAN_USER_DOT1X,             "dot1x"      },
    {VTSS_APPL_VLAN_USER_MVRP,              "mvrp"       },
    {VTSS_APPL_VLAN_USER_GVRP,              "gvrp"       },
    {VTSS_APPL_VLAN_USER_MVR,               "mvr"        },
    {VTSS_APPL_VLAN_USER_VOICE_VLAN,        "voiceVlan"  },
    {VTSS_APPL_VLAN_USER_MSTP,              "mstp"       },
    {VTSS_APPL_VLAN_USER_ERPS,              "erps"       },
    {VTSS_APPL_VLAN_USER_IEC_MRP,           "mrp"        },
    {VTSS_APPL_VLAN_USER_MEP_OBSOLETE,      "mepObsolete"},
    {VTSS_APPL_VLAN_USER_EVC_OBSOLETE,      "evcObsolete"},
    {VTSS_APPL_VLAN_USER_VCL,               "vcl"        },
    {VTSS_APPL_VLAN_USER_RMIRROR,           "rmirror"    },
    {0, 0}
};

vtss_enum_descriptor_t vlan_tx_tag_type_txt[] = {
    {VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS, "untagThis"},
    {VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS,   "tagThis"  },
    {VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL,    "tagAll"   },
    {VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL,  "untagAll" },
    {0, 0}
};

// Awareness, and if aware, which tag to react on
vtss_enum_descriptor_t vlan_port_type_txt[] = {
    {VTSS_APPL_VLAN_PORT_TYPE_UNAWARE,      "unaware"  },
    {VTSS_APPL_VLAN_PORT_TYPE_C,            "c"        },
    {VTSS_APPL_VLAN_PORT_TYPE_S,            "s"        },
    {VTSS_APPL_VLAN_PORT_TYPE_S_CUSTOM,     "sCustom"  },
    {0, 0}
};

// Port mode
vtss_enum_descriptor_t vlan_port_mode_txt[] = {
    {VTSS_APPL_VLAN_PORT_MODE_ACCESS,       "access"   },
    {VTSS_APPL_VLAN_PORT_MODE_TRUNK,        "trunk"    },
    {VTSS_APPL_VLAN_PORT_MODE_HYBRID,       "hybrid"   },
    {0, 0}
};

// Ingress acceptance
vtss_enum_descriptor_t vlan_frame_txt[] = {
    {MESA_VLAN_FRAME_ALL,                   "all"      },
    {MESA_VLAN_FRAME_TAGGED,                "tagged"   },
    {MESA_VLAN_FRAME_UNTAGGED,              "untagged" },
    {0, 0}
};

/******************************************************************************/
// VLAN_MIB_name_get()
/******************************************************************************/
mesa_rc VLAN_MIB_name_get(mesa_vid_t vid, VLAN_MIB_name_t *const name)
{
    BOOL dummy;

    return vtss_appl_vlan_name_get(vid, name->name, &dummy);
}

/******************************************************************************/
// VLAN_MIB_name_set()
/******************************************************************************/
mesa_rc VLAN_MIB_name_set(mesa_vid_t vid, const VLAN_MIB_name_t *const name)
{
    return vtss_appl_vlan_name_set(vid, name->name);
}

/******************************************************************************/
// VLAN_MIB_vid_all_iter()
// Allows for iterating across all VLAN IDs
/******************************************************************************/
mesa_rc VLAN_MIB_vid_all_iter(const mesa_vid_t *prev, mesa_vid_t *next)
{
    if (prev == NULL || *prev < VTSS_APPL_VLAN_ID_MIN) {
        *next = VTSS_APPL_VLAN_ID_MIN;
        return VTSS_RC_OK;
    }

    if (*prev > VTSS_APPL_VLAN_ID_MAX) {
        return VTSS_RC_ERROR;
    }

    *next = *prev + 1;
    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_MIB_config_globals_main_get()
/******************************************************************************/
mesa_rc VLAN_MIB_config_globals_main_get(VLAN_MIB_config_globals_main_t *const g)
{
    VTSS_RC(vtss_appl_vlan_s_custom_etype_get(&g->custom_s_port_ether_type));
    return vtss_appl_vlan_access_vids_get(g->access_vids);
}

/******************************************************************************/
// VLAN_MIB_config_globals_main_set()
/******************************************************************************/
mesa_rc VLAN_MIB_config_globals_main_set(const VLAN_MIB_config_globals_main_t *g)
{
    VTSS_RC(vtss_appl_vlan_s_custom_etype_set(g->custom_s_port_ether_type));
    return vtss_appl_vlan_access_vids_set(g->access_vids);
}

/******************************************************************************/
// VLAN_MIB_interface_conf_get()
/******************************************************************************/
mesa_rc VLAN_MIB_interface_conf_get(vtss_ifindex_t ifindex, vtss_appl_vlan_port_conf_t *const conf)
{
    return vtss_appl_vlan_interface_conf_get(ifindex, VTSS_APPL_VLAN_USER_STATIC, conf, FALSE);
}

/******************************************************************************/
// VLAN_MIB_user_iter()
/******************************************************************************/
static vtss::expose::snmp::IteratorComposeRange<vtss_appl_vlan_user_t> VLAN_MIB_user_iter((vtss_appl_vlan_user_t)0, (vtss_appl_vlan_user_t)(VTSS_APPL_VLAN_USER_LAST - 1));

/******************************************************************************/
// VLAN_MIB_ifindex_user_iter()
/******************************************************************************/
mesa_rc VLAN_MIB_ifindex_user_iter(const vtss_ifindex_t        *prev_ifindex, vtss_ifindex_t        *next_ifindex,
                                   const vtss_appl_vlan_user_t *prev_user,    vtss_appl_vlan_user_t *next_user)
{
    mesa_rc rc;
    vtss::IteratorComposeN<vtss_ifindex_t, vtss_appl_vlan_user_t> itr(vtss_appl_iterator_ifindex_front_port, VLAN_MIB_user_iter);

    T_D("Enter: prev_ifindex = %d, prev_user = %d", prev_ifindex ? vtss_ifindex_cast_to_u32(*prev_ifindex) : -1, prev_user ? *prev_user : -1);
    rc = itr(prev_ifindex, next_ifindex, prev_user, next_user);
    T_D("Exit (rc = %d): *prev_ifindex = %d, vtss_ifindex_cast_to_u32(*next_ifindex) = %u, *prev_user = %d *next_user = %u", rc, prev_ifindex ? vtss_ifindex_cast_to_u32(*prev_ifindex) : -1, vtss_ifindex_cast_to_u32(*next_ifindex), prev_user ? *prev_user : -1, *next_user);

    return rc;
}

//******************************************************************************/
// VLAN_MIB_vid_user_iter()
// Could have used vtss::IteratorComposeN(), but that turns out to be too
// slow because of the multitude of failing calls into vtss_appl_vlan_get(),
// because the #next argument had to be FALSE. Therefore implementing it
// by hand and using #next argument to let vlan.c find the next defined VID
// for a particular user.
//******************************************************************************/
mesa_rc VLAN_MIB_vid_user_iter(const mesa_vid_t            *prev_vid,  mesa_vid_t            *next_vid,
                               const vtss_appl_vlan_user_t *prev_user, vtss_appl_vlan_user_t *next_user)
{
    mesa_vid_t             start_vid  = prev_vid ? *prev_vid : VTSS_VID_NULL, vid = VTSS_APPL_VLAN_ID_MAX + 1;
    vtss_appl_vlan_user_t  start_user, user_iter, user = VTSS_APPL_VLAN_USER_LAST /* Whatever */;
    vtss_appl_vlan_entry_t membership;
    int                    i;
    BOOL                   found = FALSE;
    mesa_rc                rc;

    T_D("Enter: prev_vid = %d, prev_user = %d", prev_vid ? *prev_vid : -1, prev_user ? *prev_user : -1);

    if (!prev_user || *prev_user < 0) {
        start_user = (vtss_appl_vlan_user_t)0;
    } else if (*prev_user < VTSS_APPL_VLAN_USER_LAST - 1) {
        start_user = (vtss_appl_vlan_user_t)(*prev_user + 1);
    } else {
        // Overflow.
        start_user = (vtss_appl_vlan_user_t)0;
        start_vid++;
    }

    if (start_vid > VTSS_APPL_VLAN_ID_MAX) {
        // Overflow.
        return VTSS_RC_ERROR;
    }

    for (i = 0 ; i < 2; i++) {
        if (i == 0) {
            // In first iteration, we check the remaining users on a particular VID.
            if (start_user == (vtss_appl_vlan_user_t)0) {
                 // Nothing to do in first iteration, since we're about to find
                 // the next, lowest VID for any user.
                 continue;
            }
        } else {
            // In second iteration, we iterate over all users and find the
            // user with the next, smallest VID.
            start_user = (vtss_appl_vlan_user_t)0;
        }

        for (user_iter = start_user; user_iter < VTSS_APPL_VLAN_USER_LAST; user_iter = (vtss_appl_vlan_user_t)(user_iter + 1)) {
            // If we're in the first iteration, we're checking if more users are on this VID, so
            // #next parameter is FALSE. Otherwise, we attempt to get the next VID for a particular user.
            if (vtss_appl_vlan_get(VTSS_ISID_GLOBAL, start_vid, &membership, i == 0 ? FALSE : TRUE, user_iter) == VTSS_RC_OK) {
                if (membership.vid < vid) {
                    vid   = membership.vid;
                    user  = user_iter;
                    found = TRUE;
                    if (i == 0) {
                        // Return the first hit if we're searching for a user on a particular VID.
                        break;
                    }
                }
            }
        }

        if (found) {
            break;
        }
    }

    if (found) {
        *next_vid = vid;
        *next_user = user;
        rc = VTSS_RC_OK;
    } else {
        // No more VLANs.
        rc = VTSS_RC_ERROR;
    }

    T_D("Exit (rc = %d): *prev_vid = %d, *next_vid = %u, *prev_user = %d *next_user = %u", rc, prev_vid ? *prev_vid : -1, *next_vid, prev_user ? *prev_user : -1, *next_user);

    return rc;
}

/******************************************************************************/
// VLAN_MIB_interface_detailed_conf_get()
/******************************************************************************/
mesa_rc VLAN_MIB_interface_detailed_conf_get(vtss_ifindex_t ifindex, vtss_appl_vlan_user_t user, vtss_appl_vlan_port_detailed_conf_t *const conf)
{
    vtss_appl_vlan_port_conf_t full_conf;

    T_I("Enter: ifindex = %u, user = %u", vtss_ifindex_cast_to_u32(ifindex), user);

    VTSS_RC(vtss_appl_vlan_interface_conf_get(ifindex, user, &full_conf, TRUE));

    if (full_conf.hybrid.flags == 0) {
        T_D("No overrides");
        return VTSS_RC_ERROR;
    }

    *conf = full_conf.hybrid;

    T_I("Exit: ifindex = %u, user = %u", vtss_ifindex_cast_to_u32(ifindex), user);

    return VTSS_RC_OK;
}

//******************************************************************************/
// VLAN_MIB_membership_vid_user_get()
//******************************************************************************/
mesa_rc VLAN_MIB_membership_vid_user_get(mesa_vid_t vid, vtss_appl_vlan_user_t user, vtss::PortListStackable *s)
{
    vtss_isid_t            isid;
    vtss_appl_vlan_entry_t membership;
    BOOL                   found = FALSE;

    s->clear_all();

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (vtss_appl_vlan_get(isid, vid, &membership, FALSE, user) == VTSS_RC_OK) {
            port_iter_t pit;

            found = TRUE;

            (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (membership.ports[pit.iport]) {
                    (void)s->set(isid, pit.iport);
                }
            }
        }
    }

    return found ? VTSS_RC_OK : VTSS_RC_ERROR;
}

//******************************************************************************/
// VLAN_MIB_svl_conf_get()
//******************************************************************************/
mesa_rc VLAN_MIB_svl_conf_get(mesa_vid_t vid, mesa_vid_t *fid)
{
    VTSS_RC(vtss_appl_vlan_fid_get(vid, fid));

    // We have a sparsely populated table, so if a VLAN is not mapped, skip it.
    return *fid == 0 ? VTSS_RC_ERROR : VTSS_RC_OK;
}

//******************************************************************************/
// VLAN_MIB_svl_exists()
//******************************************************************************/
static BOOL VLAN_MIB_svl_exists(mesa_vid_t vid)
{
   mesa_vid_t fid;

   return VLAN_MIB_svl_conf_get(vid, &fid) == VTSS_RC_OK;
}

//******************************************************************************/
// VLAN_MIB_svl_conf_itr()
//******************************************************************************/
mesa_rc VLAN_MIB_svl_conf_itr(const mesa_vid_t *prev, mesa_vid_t *next)
{
    vtss::expose::snmp::IteratorComposeRange<mesa_vid_t> vid_itr(VTSS_APPL_VLAN_ID_MIN, VTSS_APPL_VLAN_ID_MAX);
    return vtss::expose::snmp::IteratorComposeFilter(vid_itr, prev, next, VLAN_MIB_svl_exists);
}

//******************************************************************************/
// VLAN_MIB_svl_conf_del()
//******************************************************************************/
mesa_rc VLAN_MIB_svl_conf_del(mesa_vid_t vid)
{
    return vtss_appl_vlan_fid_set(vid, 0);
}

//******************************************************************************/
// VLAN_MIB_svl_conf_default()
//******************************************************************************/
mesa_rc VLAN_MIB_svl_conf_default(mesa_vid_t *vid, mesa_vid_t *fid)
{
    *vid = MESA_VID_DEFAULT;
    return VTSS_RC_OK;
}

