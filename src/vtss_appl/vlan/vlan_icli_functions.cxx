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

#include "icli_api.h"
#include "icli_porting_util.h"
#include "vlan_api.h"
#include "msg.h"
#include "vlan_trace.h"
#include "vlan_icli_functions.h" /* Just to check syntax of .h file against the function signatures of the implementation */
#ifdef __cplusplus
#include "enum_macros.hxx"
#endif

#ifdef __cplusplus
VTSS_ENUM_INC(vtss_appl_vlan_user_t);
VTSS_ENUM_INC(vlan_port_flags_idx_t);
#endif /* #ifdef __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_VLAN

/******************************************************************************/
// VLAN_ICLI_port_error()
/******************************************************************************/
static void VLAN_ICLI_port_error(u32 session_id, vtss_usid_t usid, mesa_port_no_t uport, mesa_rc rc)
{
    char interface_str[ICLI_PORTING_STR_BUF_SIZE];
    ICLI_PRINTF("%% %s: %s", icli_port_info_txt(usid, uport, interface_str), error_txt(rc));
}

/****************************************************************************/
/*  Functions called by ICLI                                                */
/****************************************************************************/

const char *VLAN_ICLI_tx_tag_type_to_txt(vtss_appl_vlan_tx_tag_type_t tx_tag_type)
{
    if (tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS) {
        return "All except-native";
    } else if (tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL) {
        return "None";
    } else if (tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL) {
        return "All";
    }
    return "";
}

/******************************************************************************/
// VLAN_ICLI_show_status()
/******************************************************************************/
mesa_rc VLAN_ICLI_show_status(i32 session_id, icli_stack_port_range_t *plist, BOOL has_admin, BOOL has_combined, BOOL has_conflicts, BOOL has_erps, BOOL has_gvrp, BOOL has_mrp, BOOL has_mstp, BOOL has_mvr, BOOL has_nas, BOOL has_rmirror, BOOL has_vcl, BOOL has_voice_vlan)
{
    switch_iter_t         sit;
    port_iter_t           pit;
    vtss_appl_vlan_user_t usr;
    vlan_port_conflicts_t conflicts;
    u32                   all_usrs = 0;
    vlan_port_flags_idx_t temp;
    char                  buf[200];
    char                  interface_str[ICLI_PORTING_STR_BUF_SIZE];
    vtss_appl_vlan_user_t first_user = (vtss_appl_vlan_user_t)0;
    vtss_appl_vlan_user_t last_user  = (vtss_appl_vlan_user_t)(VTSS_APPL_VLAN_USER_LAST - 1);
    BOOL                  print_hdr  = TRUE;
    BOOL                  has_all    = FALSE;

    if (has_admin) {
        first_user = VTSS_APPL_VLAN_USER_STATIC;
    } else if (has_combined || has_conflicts) {
        first_user = VTSS_APPL_VLAN_USER_ALL;
#ifdef VTSS_SW_OPTION_ERPS
    } else if (has_erps) {
        first_user = VTSS_APPL_VLAN_USER_ERPS;
#endif
#if defined(VTSS_SW_OPTION_GVRP)
    } else if (has_gvrp) {
        first_user = VTSS_APPL_VLAN_USER_GVRP;
#endif
#ifdef VTSS_SW_OPTION_IEC_MRP
    } else if (has_mrp) {
        first_user = VTSS_APPL_VLAN_USER_IEC_MRP;
#endif
#ifdef VTSS_SW_OPTION_MSTP
    } else if (has_mstp) {
        first_user = VTSS_APPL_VLAN_USER_MSTP;
#endif
#ifdef VTSS_SW_OPTION_MVR
    } else if (has_mvr) {
        first_user = VTSS_APPL_VLAN_USER_MVR;
#endif
#ifdef VTSS_SW_OPTION_DOT1X
    } else if (has_nas) {
        first_user = VTSS_APPL_VLAN_USER_DOT1X;
#endif
#if defined(VTSS_SW_OPTION_RMIRROR)
    } else if (has_rmirror) {
        first_user = VTSS_APPL_VLAN_USER_RMIRROR;
#endif
#ifdef VTSS_SW_OPTION_VCL
    } else if (has_vcl) {
        first_user = VTSS_APPL_VLAN_USER_VCL;
#endif
#ifdef VTSS_SW_OPTION_VOICE_VLAN
    } else if (has_voice_vlan) {
        first_user = VTSS_APPL_VLAN_USER_VOICE_VLAN;
#endif
    } else {
        has_all = TRUE; // Default to show all if none of the others are selected
    }

    if (!has_all) {
        last_user = first_user;
    }

    // Loop through all switches in the stack
    VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID));
    while (icli_switch_iter_getnext(&sit, plist)) {
        VTSS_RC(port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL));
        while (icli_port_iter_getnext(&pit, plist)) {
            vtss_appl_vlan_port_conf_t conf;

            VTSS_RC(vlan_mgmt_port_conflicts_get(sit.isid, pit.iport, &conflicts));

            // Collect all conflicting VLAN users.
            for (temp = (vlan_port_flags_idx_t)0; temp < VLAN_PORT_FLAGS_IDX_CNT; temp++) {
                all_usrs |= conflicts.users[temp];
            }

            (void)icli_port_info_txt(sit.usid, pit.uport, interface_str);
            strcat(&interface_str[0], " :");
            icli_table_header(session_id, &interface_str[0]);

            print_hdr = TRUE;

            for (usr = first_user; usr <= last_user; usr++) {
                // Skip calling for those VLAN users that never override port configuration
                if (usr != VTSS_APPL_VLAN_USER_ALL && !vlan_mgmt_user_is_port_conf_changer(usr)) {
                    continue;
                }

                if (vlan_mgmt_port_conf_get(sit.isid, pit.iport, &conf, usr, TRUE) != VTSS_RC_OK) {
                    T_IG_PORT(VTSS_TRACE_GRP_CLI, pit.iport, "Skipping user: %d, isid: %u", usr, sit.isid);
                    continue;
                }

                if (print_hdr) {
                    (void)snprintf(&buf[0], sizeof(buf), "%s%-15s%-6s%-15s%s%-19s%-6s%s",
                                   "VLAN User   ",
                                   "PortType",
                                   "PVID",
                                   "Frame Type",
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
                                   "Ing Filter  ",
#else
                                   "",
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */
                                   "Tx Tag",
                                   "UVID",
                                   last_user != VTSS_APPL_VLAN_USER_STATIC ? "Conflicts" : "");

                    icli_table_header(session_id, buf);
                    print_hdr = FALSE;
                }

                T_NG(VTSS_TRACE_GRP_CLI, "has_conflicts: %d, conflicts.port_flags: %u", has_conflicts, conflicts.port_flags);
                if (!has_conflicts || conflicts.port_flags) {
                    ICLI_PRINTF("%-12s", vlan_mgmt_user_to_txt(usr));

                    ICLI_PRINTF("%-15s", conf.hybrid.flags & VTSS_APPL_VLAN_PORT_FLAGS_AWARE ? vlan_mgmt_port_type_to_txt(conf.hybrid.port_type) : "");

                    if (conf.hybrid.flags & VTSS_APPL_VLAN_PORT_FLAGS_PVID) {
                        ICLI_PRINTF("%-6d", conf.hybrid.pvid);
                    } else {
                        ICLI_PRINTF("%-6s", "");
                    }

                    ICLI_PRINTF("%-15s", (conf.hybrid.flags & VTSS_APPL_VLAN_PORT_FLAGS_RX_TAG_TYPE) ? vlan_mgmt_frame_type_to_txt(conf.hybrid.frame_type) : "");

#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
                    ICLI_PRINTF("%-12s", (conf.hybrid.flags & VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT) ? (conf.hybrid.ingress_filter ? "Enabled" : "Disabled") : "");
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */

                    ICLI_PRINTF("%-19s", (conf.hybrid.flags & VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE) ? VLAN_ICLI_tx_tag_type_to_txt(conf.hybrid.tx_tag_type) : "");

                    if (conf.hybrid.flags & VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE) {
                        ICLI_PRINTF("%-6d", conf.hybrid.untagged_vid);
                    } else {
                        ICLI_PRINTF("%-6s", "");
                    }

                    if (last_user != VTSS_APPL_VLAN_USER_STATIC) {
                        if (usr != VTSS_APPL_VLAN_USER_STATIC) {
                            if (conflicts.port_flags == 0) {
                                ICLI_PRINTF("%s", "No");
                            } else {
                                if (all_usrs & (1 << (u8)usr)) {
                                    ICLI_PRINTF("%s", "Yes");
                                } else {
                                    ICLI_PRINTF("%s", "No");
                                }
                            }
                        }
                    }
                    ICLI_PRINTF("\n");
                }
            }
            ICLI_PRINTF("\n");
        }
    }
    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_ICLI_runtime_nas()
/******************************************************************************/
BOOL VLAN_ICLI_runtime_nas(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
    case ICLI_ASK_PRESENT:
#if defined(VTSS_SW_OPTION_DOT1X)
        runtime->present = vlan_mgmt_user_is_port_conf_changer(VTSS_APPL_VLAN_USER_DOT1X);
#else
        runtime->present = FALSE;
#endif
        return TRUE;
    default:
        return FALSE;
    }
}

/******************************************************************************/
// VLAN_ICLI_runtime_mvr()
/******************************************************************************/
BOOL VLAN_ICLI_runtime_mvr(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
    case ICLI_ASK_PRESENT:
#if defined(VTSS_SW_OPTION_MVR)
        runtime->present = vlan_mgmt_user_is_port_conf_changer(VTSS_APPL_VLAN_USER_MVR);
#else
        runtime->present = FALSE;
#endif
        return TRUE;
    default:
        return FALSE;
    }
}

/******************************************************************************/
// VLAN_ICLI_runtime_voice_vlan()
/******************************************************************************/
BOOL VLAN_ICLI_runtime_voice_vlan(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
    case ICLI_ASK_PRESENT:
#if defined(VTSS_SW_OPTION_VOICE_VLAN)
        runtime->present = vlan_mgmt_user_is_port_conf_changer(VTSS_APPL_VLAN_USER_VOICE_VLAN);
#else
        runtime->present = FALSE;
#endif
        return TRUE;
    default:
        return FALSE;
    }
}

/******************************************************************************/
// VLAN_ICLI_runtime_mstp()
/******************************************************************************/
BOOL VLAN_ICLI_runtime_mstp(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
    case ICLI_ASK_PRESENT:
#if defined(VTSS_SW_OPTION_MSTP)
        runtime->present = vlan_mgmt_user_is_port_conf_changer(VTSS_APPL_VLAN_USER_MSTP);
#else
        runtime->present = FALSE;
#endif
        return TRUE;
    default:
        return FALSE;
    }
}

//******************************************************************************/
// VLAN_ICLI_runtime_erps()
/******************************************************************************/
BOOL VLAN_ICLI_runtime_erps(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
    case ICLI_ASK_PRESENT:
#if defined(VTSS_SW_OPTION_ERPS)
        runtime->present = vlan_mgmt_user_is_port_conf_changer(VTSS_APPL_VLAN_USER_ERPS);
#else
        runtime->present = FALSE;
#endif
        return TRUE;
    default:
        return FALSE;
    }
}

//******************************************************************************/
// VLAN_ICLI_runtime_mrp()
/******************************************************************************/
BOOL VLAN_ICLI_runtime_mrp(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
    case ICLI_ASK_PRESENT:
#if defined(VTSS_SW_OPTION_IEC_MRP)
        runtime->present = vlan_mgmt_user_is_port_conf_changer(VTSS_APPL_VLAN_USER_IEC_MRP);
#else
        runtime->present = FALSE;
#endif
        return TRUE;
    default:
        return FALSE;
    }
}

/******************************************************************************/
// VLAN_ICLI_runtime_vcl()
/******************************************************************************/
BOOL VLAN_ICLI_runtime_vcl(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
    case ICLI_ASK_PRESENT:
#if defined(VTSS_SW_OPTION_VCL)
        runtime->present = vlan_mgmt_user_is_port_conf_changer(VTSS_APPL_VLAN_USER_VCL);
#else
        runtime->present = FALSE;
#endif
        return TRUE;
    default:
        return FALSE;
    }
}

/******************************************************************************/
// VLAN_ICLI_runtime_gvrp()
/******************************************************************************/
BOOL VLAN_ICLI_runtime_gvrp(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
    case ICLI_ASK_PRESENT:
#if defined(VTSS_SW_OPTION_GVRP)
        runtime->present = vlan_mgmt_user_is_port_conf_changer(VTSS_APPL_VLAN_USER_GVRP);
#else
        runtime->present = FALSE;
#endif
        return TRUE;
    default:
        return FALSE;
    }
}

/******************************************************************************/
// VLAN_ICLI_runtime_rmirror()
/******************************************************************************/
BOOL VLAN_ICLI_runtime_rmirror(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
    case ICLI_ASK_PRESENT:
#if defined(VTSS_SW_OPTION_RMIRROR)
        runtime->present = vlan_mgmt_user_is_port_conf_changer(VTSS_APPL_VLAN_USER_RMIRROR);
#else
        runtime->present = FALSE;
#endif
        return TRUE;
    default:
        return FALSE;
    }
}

/******************************************************************************/
// VLAN_ICLI_hybrid_port_conf()
// PVID is configured through VLAN_ICLI_pvid_set()
/******************************************************************************/
mesa_rc VLAN_ICLI_hybrid_port_conf(u32 session_id, icli_stack_port_range_t *plist, vtss_appl_vlan_port_conf_t *new_conf, BOOL frametype, BOOL ingressfilter, BOOL porttype, BOOL tx_tag)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mesa_rc                    rc, return_rc = VTSS_RC_OK;
    vtss_appl_vlan_port_conf_t old_conf;

    // Loop through all configurable switches in stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop through all ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_NORMAL));
        while (icli_port_iter_getnext(&pit, plist)) {
            if ((rc = vlan_mgmt_port_conf_get(sit.isid, pit.iport, &old_conf, VTSS_APPL_VLAN_USER_STATIC, FALSE)) != VTSS_RC_OK) {
                VLAN_ICLI_port_error(session_id, sit.usid, pit.uport, rc);
                return_rc = rc;
                continue;
            }

            old_conf.hybrid.flags = VTSS_APPL_VLAN_PORT_FLAGS_ALL;

            if (frametype) {
                old_conf.hybrid.frame_type = new_conf->hybrid.frame_type;
            }

#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
            if (ingressfilter) {
                old_conf.hybrid.ingress_filter = new_conf->hybrid.ingress_filter;
            }
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */

            if (tx_tag) {
                if (old_conf.hybrid.tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS) {
                    // Make sure to always untag the PVID
                    old_conf.hybrid.untagged_vid = old_conf.hybrid.pvid;
                }

                old_conf.hybrid.tx_tag_type = new_conf->hybrid.tx_tag_type;
            }

            if (porttype) {
                old_conf.hybrid.port_type = new_conf->hybrid.port_type;
            }

            if ((rc = vlan_mgmt_port_conf_set(sit.isid, pit.iport, &old_conf, VTSS_APPL_VLAN_USER_STATIC)) != VTSS_RC_OK) {
                VLAN_ICLI_port_error(session_id, sit.usid, pit.uport, rc);
                return_rc = rc;
                continue;
            }
        }
    }

    return return_rc;
}

/******************************************************************************/
// VLAN_ICLI_pvid_set()
/******************************************************************************/
mesa_rc VLAN_ICLI_pvid_set(u32 session_id, icli_stack_port_range_t *plist, vtss_appl_vlan_port_mode_t port_mode, mesa_vid_t new_pvid)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mesa_rc                    rc, return_rc = VTSS_RC_OK;
    vtss_appl_vlan_port_conf_t conf;

    // Loop through all configurable switches in stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop through all ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_NORMAL));
        while (icli_port_iter_getnext(&pit, plist)) {
            if ((rc = vlan_mgmt_port_conf_get(sit.isid, pit.iport, &conf, VTSS_APPL_VLAN_USER_STATIC, FALSE)) != VTSS_RC_OK) {
                VLAN_ICLI_port_error(session_id, sit.usid, pit.uport, rc);
                return_rc = rc;
                continue;
            }

            switch (port_mode) {
            case VTSS_APPL_VLAN_PORT_MODE_ACCESS:
                if (conf.access_pvid == new_pvid) {
                    continue;
                }

                conf.access_pvid = new_pvid;
                break;

            case VTSS_APPL_VLAN_PORT_MODE_TRUNK:
                if (conf.trunk_pvid == new_pvid) {
                    continue;
                }

                conf.trunk_pvid = new_pvid;
                break;

            default:
                // Hybrid:
                if (conf.hybrid.pvid == new_pvid) {
                    continue;
                }

                conf.hybrid.pvid         = new_pvid;
                // If port is configured to "untag this", we must set
                // uvid = pvid. Administrative user cannot configure switch to
                // untag another VID than PVID, and it cannot configure
                // the switch to tag a specific VID, so it's safe
                // to always set untagged_vid to PVID here.
                conf.hybrid.untagged_vid = new_pvid;
                break;
            }

            if ((rc = vlan_mgmt_port_conf_set(sit.isid, pit.iport, &conf, VTSS_APPL_VLAN_USER_STATIC)) != VTSS_RC_OK) {
                VLAN_ICLI_port_error(session_id, sit.usid, pit.uport, rc);
                return_rc = rc;
                continue;
            }
        }
    }

    return return_rc;
}

/******************************************************************************/
// VLAN_ICLI_mode_set()
/******************************************************************************/
mesa_rc VLAN_ICLI_mode_set(u32 session_id, icli_stack_port_range_t *plist, vtss_appl_vlan_port_mode_t new_mode)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mesa_rc                    rc, return_rc = VTSS_RC_OK;
    vtss_appl_vlan_port_conf_t conf;

    // Loop over all configurable switches in usid order...
    (void)icli_switch_iter_init(&sit);
    // ...provided they're also in the plist.
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop over all ports in uport order...
        (void)icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_NORMAL);
        // ... provided they're also in the plist.
        while (icli_port_iter_getnext(&pit, plist)) {
            if ((rc = vlan_mgmt_port_conf_get(sit.isid, pit.iport, &conf, VTSS_APPL_VLAN_USER_STATIC, FALSE)) != VTSS_RC_OK) {
                VLAN_ICLI_port_error(session_id, sit.usid, pit.uport, rc);
                return_rc = rc;
                continue;
            }

            if (new_mode != conf.mode) {
                conf.mode = new_mode;
                if ((rc = vlan_mgmt_port_conf_set(sit.isid, pit.iport, &conf, VTSS_APPL_VLAN_USER_STATIC)) != VTSS_RC_OK) {
                    VLAN_ICLI_port_error(session_id, sit.usid, pit.uport, rc);
                    return_rc = rc;
                    continue;
                }
            }
        }
    }

    return return_rc;
}

/******************************************************************************/
// VLAN_ICLI_trunk_tag_pvid_set()
/******************************************************************************/
mesa_rc VLAN_ICLI_trunk_tag_pvid_set(u32 session_id, icli_stack_port_range_t *plist, BOOL trunk_tag_pvid)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mesa_rc                    rc, return_rc = VTSS_RC_OK;
    vtss_appl_vlan_port_conf_t conf;

    // Loop over all configurable switches in usid order...
    (void)icli_switch_iter_init(&sit);
    // ...provided they're also in the plist.
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop over all ports in uport order...
        (void)icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_NORMAL);
        // ... provided they're also in the plist.
        while (icli_port_iter_getnext(&pit, plist)) {
            if ((rc = vlan_mgmt_port_conf_get(sit.isid, pit.iport, &conf, VTSS_APPL_VLAN_USER_STATIC, FALSE)) != VTSS_RC_OK) {
                VLAN_ICLI_port_error(session_id, sit.usid, pit.uport, rc);
                return_rc = rc;
                continue;
            }

            if (trunk_tag_pvid != conf.trunk_tag_pvid) {
                conf.trunk_tag_pvid = trunk_tag_pvid;
                if ((rc = vlan_mgmt_port_conf_set(sit.isid, pit.iport, &conf, VTSS_APPL_VLAN_USER_STATIC)) != VTSS_RC_OK) {
                    VLAN_ICLI_port_error(session_id, sit.usid, pit.uport, rc);
                    return_rc = rc;
                    continue;
                }
            }
        }
    }

    return return_rc;
}

/******************************************************************************/
// VLAN_ICLI_port_mode_txt()
/******************************************************************************/
const char *VLAN_ICLI_port_mode_txt(vtss_appl_vlan_port_mode_t mode)
{
    if (mode == VTSS_APPL_VLAN_PORT_MODE_ACCESS) {
        return "access";
    } else if (mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK) {
        return "trunk";
    } else if (mode == VTSS_APPL_VLAN_PORT_MODE_HYBRID) {
        return "hybrid";
    }
    return "";
}

/******************************************************************************/
// VLAN_ICLI_vlan_print()
// Returns TRUE if further processing should be aborted. FALSE otherwise.
/******************************************************************************/
static BOOL VLAN_ICLI_vlan_print(u32 session_id, mesa_vid_t vid, BOOL *first, vtss_appl_vlan_user_t user)
{
    u8                     indent = 0; // For the first interface there is no indent, since it is printed at the same line as the VLAN
    char                   *str_buf = (char *)VTSS_MALLOC(ICLI_STR_MAX_LEN);
    BOOL                   name_indent, at_least_one_printed = FALSE;
    icli_line_mode_t       line_mode;
    vtss_appl_vlan_entry_t conf;
    mesa_rc                rc;
    switch_iter_t          sit;
#ifdef VTSS_SW_OPTION_VLAN_NAMING
    char                   vlan_name[VTSS_APPL_VLAN_NAME_MAX_LEN];

    if ((rc = vtss_appl_vlan_name_get(vid, vlan_name, NULL)) != VTSS_RC_OK) {
        T_E("Unable to obtain VLAN name for vid = %u. Error = %s", vid, error_txt(rc));
        vlan_name[0] = '\0';
    }
#endif

    if (str_buf == NULL) {
        return TRUE;
    }

    if (*first) { // Print header if this is the first VLAN found
#ifdef VTSS_SW_OPTION_VLAN_NAMING
        sprintf(str_buf, "%-4s  %-32s  %s", "VLAN", "Name", "Interfaces");
#else
        sprintf(str_buf, "%-4s  %s", "VLAN", "Interfaces");
#endif
        icli_table_header(session_id, str_buf);
        *first = FALSE;
    }

#ifdef VTSS_SW_OPTION_VLAN_NAMING
    ICLI_PRINTF("%-4d  %-32s  ", vid, vlan_name);
    name_indent = 40;
#else
    ICLI_PRINTF("%-4d  ", vid);
    name_indent = 6;
#endif

    // Find all switches/ports assigned to this VLAN
    rc = switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID_CFG);
    while (rc == VTSS_RC_OK && switch_iter_getnext(&sit)) {
        T_IG(VTSS_TRACE_GRP_CLI, "sid:%u, vid:%u", sit.isid, vid);

        // The VLAN may or may not exist depending on how this function is called,
        // so we don't care about the return value of vtss_appl_vlan_get().
        // vtss_appl_vlan_get() is guaranteed to memset(conf) to all-zeros if it doesn't
        // exist, so no need to do that ourselves.
        (void)vtss_appl_vlan_get(sit.isid, vid, &conf, FALSE, user);
        (void)icli_port_list_info_txt(sit.isid, conf.ports, str_buf, TRUE);
        if (strlen(str_buf) != 0) {
            ICLI_PRINTF("%*s%s\n", indent, "", str_buf);
            indent = name_indent; // Indent if multiple lines are needed to print interface from another switch.
            at_least_one_printed = TRUE;
        }
    }

    if (!at_least_one_printed) {
        // Gotta print a newline then.
        ICLI_PRINTF("\n");
    }

    VTSS_FREE(str_buf);

    // Check if user aborted output (^C or Q at -- more -- prompt)
    return (ICLI_LINE_MODE_GET(&line_mode) == ICLI_RC_OK && line_mode == ICLI_LINE_MODE_BYPASS);
}

/******************************************************************************/
// VLAN_ICLI_show_vlan()
/******************************************************************************/
mesa_rc VLAN_ICLI_show_vlan(u32 session_id, icli_unsigned_range_t *vlan_list, char *name, BOOL has_vid, BOOL has_name, BOOL has_all, BOOL has_forbidden)
{
    BOOL                  first = TRUE, show_defined_only = FALSE;
    icli_unsigned_range_t my_list;
    u32                   idx;
    mesa_vid_t            vid;
    mesa_rc               rc;
    vtss_appl_vlan_user_t user = VTSS_APPL_VLAN_USER_STATIC;

    if (has_forbidden) {
        user = VTSS_APPL_VLAN_USER_FORBIDDEN;
    } else if (has_all) {
        user = VTSS_APPL_VLAN_USER_ALL;
    }

    if (has_name) {
#if defined(VTSS_SW_OPTION_VLAN_NAMING)
        if (name == NULL || strlen(name) == 0) {
            T_E("Huh?");
            return VTSS_RC_ERROR;
        }

        if ((rc = vlan_mgmt_name_to_vid(name, &vid)) != VTSS_RC_OK) {
            ICLI_PRINTF("%% %s\n", error_txt(rc));
            return VTSS_RC_OK; // Not a programmatic error
        }

        my_list.cnt = 1;
        my_list.range[0].min = my_list.range[0].max = vid;
#else
        T_E("VLAN naming not compile-time enabled on this switch");
        return VTSS_RC_ERROR;
#endif
    } else if (has_vid) {
        if (vlan_list == NULL) {
            T_E("Huh?");
            return VTSS_RC_ERROR;
        }

        my_list = *vlan_list;
    } else {
        my_list.cnt = 1; // Make Lint happy
        show_defined_only = TRUE;
    }

    if (show_defined_only) {
        vtss_appl_vlan_entry_t conf;

        // If "all" keyword and "forbidden" is not specified, only access VLANs must be shown
        // otherwise, we show all VLANs with non-zero memberships.
        if (has_all || has_forbidden) {
            // Show all VLANs with non-zero memberships
            conf.vid = VTSS_VID_NULL;

            while (vtss_appl_vlan_get(VTSS_ISID_GLOBAL, conf.vid, &conf, TRUE, user) == VTSS_RC_OK) {
                if (VLAN_ICLI_vlan_print(session_id, conf.vid, &first, user)) {
                    // Aborted output
                    return VTSS_RC_OK;
                }
            }
        } else {
            u8 access_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
            // Only show access VLANs.
            if ((rc = vtss_appl_vlan_access_vids_get(access_vids)) != VTSS_RC_OK) {
                ICLI_PRINTF("%% %s\n", error_txt(rc));
                return rc;
            }

            for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
                if (!VTSS_BF_GET(access_vids, vid)) {
                    continue;
                }

                if (VLAN_ICLI_vlan_print(session_id, vid, &first, user)) {
                    // Aborted output
                    return VTSS_RC_OK;
                }
            }
        }
    } else {
        // User is asking for specific VLANs.
        for (idx = 0; idx < my_list.cnt; idx++) {
            for (vid = my_list.range[idx].min; vid <= my_list.range[idx].max; vid++) {
                if (VLAN_ICLI_vlan_print(session_id, vid, &first, user)) {
                    // Aborted output
                    return VTSS_RC_OK;
                }
            }
        }
    }

    if (first) {
        if (has_name || has_vid) {
            T_E("Huh?");
        } else {
            ICLI_PRINTF("%% No %sVLANs found", has_forbidden ? "forbidden " : "");
        }
    }

    ICLI_PRINTF("\n");

    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_ICLI_add_remove_forbidden()
/******************************************************************************/
mesa_rc VLAN_ICLI_add_remove_forbidden(icli_stack_port_range_t *plist, icli_unsigned_range_t *vlan_list, BOOL has_add)
{
    switch_iter_t          sit;
    port_iter_t            pit;
    u32                    idx;
    vtss_appl_vlan_entry_t conf;
    mesa_rc                rc;
    mesa_vid_t             vid;

    // Satisfy Lint:
    if (vlan_list == NULL) {
        T_E("Unexpected NULL");
        return VTSS_RC_ERROR;
    }

    for (idx = 0; idx < vlan_list->cnt; idx++) {
        for (vid = vlan_list->range[idx].min; vid <= vlan_list->range[idx].max; vid++) {

            VTSS_RC(icli_switch_iter_init(&sit));
            while (icli_switch_iter_getnext(&sit, plist)) {
                BOOL do_add = FALSE;

                if ((rc = vtss_appl_vlan_get(sit.isid, vid, &conf, FALSE, VTSS_APPL_VLAN_USER_FORBIDDEN)) != VTSS_RC_OK) {
                    T_D("%u: %s", sit.isid, error_txt(rc));

                    // Probably doesn't exist.
                    if (!has_add) {
                        // If we're removing ports from the existing list, there's nothing more to do for this switch.
                        continue;
                    }

                    // Otherwise, initialize all to non-forbidden.
                    memset(conf.ports, 0, sizeof(conf.ports));
                }

                conf.vid = vid;

                // Loop through all ports, and set forbidden/non-forbidden for the selected ports
                VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_NORMAL));
                while (icli_port_iter_getnext(&pit, plist)) {
                    conf.ports[pit.iport] = has_add;
                }

                // Gotta run through it once more to figure out whether to add or delete this forbidden VLAN.
                // This time, go through all known (non-stacking) ports on the switch.
                VTSS_RC(port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL));
                while (port_iter_getnext(&pit)) {
                    if (conf.ports[pit.iport]) {
                        do_add = TRUE;
                        break;
                    }
                }

                if (do_add) {
                    if ((rc = vlan_mgmt_vlan_add(sit.isid, &conf, VTSS_APPL_VLAN_USER_FORBIDDEN)) != VTSS_RC_OK) {
                        T_E("%u:%u: %s", sit.isid, vid, error_txt(rc));
                        return rc;
                    }
                } else {
                    if ((rc = vlan_mgmt_vlan_del(sit.isid, vid, VTSS_APPL_VLAN_USER_FORBIDDEN)) != VTSS_RC_OK) {
                        T_E("%u:%u: %s", sit.isid, vid, error_txt(rc));
                        return rc;
                    }
                }
            }
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_ICLI_bitmask_update()
/******************************************************************************/
static void VLAN_ICLI_bitmask_update(icli_unsigned_range_t *vlan_list, u8 *bitmask, BOOL set)
{
    mesa_vid_t vid;
    u32        idx;

    for (idx = 0; idx < vlan_list->cnt; idx++) {
        for (vid = vlan_list->range[idx].min; vid <= vlan_list->range[idx].max; vid++) {
            VTSS_BF_SET(bitmask, vid, set);
        }
    }
}

/******************************************************************************/
// VLAN_ICLI_allowed_vids_set()
/******************************************************************************/
mesa_rc VLAN_ICLI_allowed_vids_set(u32 session_id, icli_stack_port_range_t *plist, vtss_appl_vlan_port_mode_t port_mode, icli_unsigned_range_t *vlan_list, BOOL has_default, BOOL has_all, BOOL has_none, BOOL has_add, BOOL has_remove, BOOL has_except)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mesa_rc                    rc, return_rc = VTSS_RC_OK;
    u8                         allowed_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    vtss_appl_vlan_port_conf_t conf;

    // has_default, has_all, has_none, and has_except all use a fixed bitmask,
    // which can be obtained prior to going into the sit/pit loop below.

    // has_add, has_remove, and has_set use the currently defined bitmasks, which is per-port,
    // which means that we cannot compute it prior to entering the loop below.
    if (has_default) {
        // Reset to defaults.
        (void)vlan_mgmt_port_conf_default_get(&conf);
        memcpy(allowed_vids, port_mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK ? conf.trunk_allowed_vids : conf.hybrid_allowed_vids, sizeof(allowed_vids));
    } else if (has_all || has_except) {
        (void)vlan_mgmt_bitmask_all_ones_set(allowed_vids);

        if (has_except) {
            // Remove VIDs in the vlan_list from bitmask
            VLAN_ICLI_bitmask_update(vlan_list, allowed_vids, FALSE);
        }
    } else if (has_none || (!has_remove && !has_add)) {
        // Here, either the "none" keyword or no keywords at all were used on the command line.
        memset(allowed_vids, 0x0, sizeof(allowed_vids));

        if (!has_none) {
            // Overwrite (no keywords were used on the command line).
            VLAN_ICLI_bitmask_update(vlan_list, allowed_vids, TRUE);
        }
    } else {
        // has_add or has_remove is really handled below, but
        // in order to keep Lint satisfied, we must touch the array.
        allowed_vids[0] = 0;
    }

    // Loop through all configurable switches in stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop through all ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_NORMAL));
        while (icli_port_iter_getnext(&pit, plist)) {

            if ((rc = vlan_mgmt_port_conf_get(sit.isid, pit.iport, &conf, VTSS_APPL_VLAN_USER_STATIC, FALSE)) != VTSS_RC_OK) {
                VLAN_ICLI_port_error(session_id, sit.usid, pit.uport, rc);
                return_rc = rc;
                continue;
            }

            if (has_add || has_remove) {
                // add and remove are the only two ones that require the current list of VIDs to be updated.
                memcpy(allowed_vids, port_mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK ? conf.trunk_allowed_vids : conf.hybrid_allowed_vids, sizeof(allowed_vids));
                VLAN_ICLI_bitmask_update(vlan_list, allowed_vids, has_add);
            }

            memcpy(port_mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK ? conf.trunk_allowed_vids : conf.hybrid_allowed_vids, allowed_vids, sizeof(allowed_vids));
            if ((rc = vlan_mgmt_port_conf_set(sit.isid, pit.iport, &conf, VTSS_APPL_VLAN_USER_STATIC)) != VTSS_RC_OK) {
                VLAN_ICLI_port_error(session_id, sit.usid, pit.uport, rc);
                return_rc = rc;
                continue;
            }
        }
    }

    return return_rc;
}

/******************************************************************************/
// VLAN_ICLI_vlan_mode_enter()
/******************************************************************************/
void VLAN_ICLI_vlan_mode_enter(u32 session_id, icli_unsigned_range_t *vlan_list)
{
    u8         access_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    u32        idx;
    mesa_vid_t vid;
    mesa_rc    rc;

    // Get current list of access VLANs
    if ((rc = vtss_appl_vlan_access_vids_get(access_vids)) != VTSS_RC_OK) {
        T_E("%s\n",  error_txt(rc));
        ICLI_PRINTF("%% Unable to get current list of access VLANs. Error message: %s\n", error_txt(rc));
        return;
    }

    for (idx = 0; idx < vlan_list->cnt; idx++) {
        for (vid = vlan_list->range[idx].min; vid <= vlan_list->range[idx].max; vid++) {
            VTSS_BF_SET(access_vids, vid, 1);
        }
    }

    if ((rc = vtss_appl_vlan_access_vids_set(access_vids)) != VTSS_RC_OK) {
        T_E("%s\n", error_txt(rc));
        ICLI_PRINTF("%% Unable to set new list of access VLANs. Error Error message: %s\n", error_txt(rc));
    }
}

/******************************************************************************/
// VLAN_ICLI_runtime_vlan_name()
// Function for runtime asking if VLAN name is supported.
/******************************************************************************/
BOOL VLAN_ICLI_runtime_vlan_name(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
    case ICLI_ASK_PRESENT:
#if defined(VTSS_SW_OPTION_VLAN_NAMING)
        runtime->present = TRUE;
#else
        runtime->present = FALSE;
#endif
        return TRUE;
    default:
        break;
    }

    return FALSE;
}

#ifdef __cplusplus
}
#endif

