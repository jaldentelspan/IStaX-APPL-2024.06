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

#include "main.h"
#include "msg_api.h"
#include "critd_api.h"
#include "vlan_api.h"
#include "port_api.h" // For port_count_max()
#include "port_iter.hxx"
#include "misc_api.h"
#if defined(VTSS_SW_OPTION_SYSLOG)
#include "syslog_api.h"  /* For definition of S_E()  */
#endif /* defined(VTSS_SW_OPTION_SYSLOG) */
#include "mgmt_api.h"
#if defined(VTSS_SW_OPTION_ICFG)
#include "vlan_icfg.h"
#endif /* defined(VTSS_SW_OPTION_ICFG) */
#include "vlan_trace.h"

#ifdef __cplusplus
#include "enum_macros.hxx"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_VLAN

/**
 * This enum identifies VLAN users included in this build.
 *
 * Mappings between the external vtss_appl_vlan_user_t and
 * this enum are made with the VLAN_user_ext_to_int() and
 * VLAN_user_int_to_ext() functions.
 *
 * The reason for having two enums representing the same is
 * that the external enum must have all possible users defined
 * and new users must be added to the bottom of the enum to
 * get SNMP OIDs constant throughout a product's lifetime.
 *
 * The internal enum is used to iterate across internally and
 * to size various arrays. The members are compile-time included,
 * and except for VLAN_USER_INT_STATIC, they can have any enumaration
 * value.
 *
 * The users are prioritized in the following way:
 *   VLAN_USER_INT_STATIC has least priority.
 *   Enumeration #1 has highest priority
 *   Enumeration #last has least priority.
 */
typedef enum {
    VLAN_USER_INT_STATIC = 0, /**< End-user. Do not change this position */
#if defined(VTSS_SW_OPTION_DOT1X)
    VLAN_USER_INT_DOT1X,      /**< 802.1X/NAS */
#endif
#if defined(VTSS_SW_OPTION_MVRP)
    VLAN_USER_INT_MVRP,       /**< MVRP */
#endif
#if defined(VTSS_SW_OPTION_GVRP)
    VLAN_USER_INT_GVRP,       /**< GVRP */
#endif
#if defined(VTSS_SW_OPTION_MVR)
    VLAN_USER_INT_MVR,        /**< MVR */
#endif
#if defined(VTSS_SW_OPTION_VOICE_VLAN)
    VLAN_USER_INT_VOICE_VLAN, /**< Voice VLAN */
#endif
#if defined(VTSS_SW_OPTION_MSTP)
    VLAN_USER_INT_MSTP,       /**< MSTP */
#endif
#if defined(VTSS_SW_OPTION_ERPS)
    VLAN_USER_INT_ERPS,       /**< ERPS */
#endif
#if defined(VTSS_SW_OPTION_IEC_MRP)
    VLAN_USER_INT_IEC_MRP,    /**< MRP */
#endif
#if defined(VTSS_SW_OPTION_VCL)
    VLAN_USER_INT_VCL,        /**< VCL */
#endif
#if defined(VTSS_SW_OPTION_RMIRROR)
    VLAN_USER_INT_RMIRROR,    /**< Remote Mirroring */
#endif
    VLAN_USER_INT_FORBIDDEN,  /**< Not a real user, but easiest to deal with in terms of management, when forbidden VLAN is enumerated as a user. Must come just before VLAN_USER_INT_ALL. */
    VLAN_USER_INT_ALL,        /**< Used in XXX_get() functions to get the current configuration. Cannot be used in XXX_set() functions. Do not change this position. */
    VLAN_USER_INT_CNT         /**< Used to size various structures and iterate. Do not change this position. */
} vlan_user_int_t;

#undef VLAN_VOLATILE_USER_PRESENT /* Prevent it from being set from outside */
#if defined(VTSS_SW_OPTION_DOT1X)      || defined(VTSS_SW_OPTION_MSTP)    || \
    defined(VTSS_SW_OPTION_MVR)        || defined(VTSS_SW_OPTION_MVRP)    || \
    defined(VTSS_SW_OPTION_VOICE_VLAN) || defined(VTSS_SW_OPTION_ERPS)    || \
    defined(VTSS_SW_OPTION_IEC_MRP)    || defined(VTSS_SW_OPTION_VCL)     || \
    defined(VTSS_SW_OPTION_RMIRROR)
#define VLAN_VOLATILE_USER_PRESENT
#endif /* defined(...) || defined(...) */

#undef VLAN_SAME_USER_SUPPORT /* Prevent it from being set from outside */
#if defined(VTSS_SW_OPTION_VOICE_VLAN)
#define VLAN_SAME_USER_SUPPORT
#endif

#undef VLAN_SINGLE_USER_SUPPORT /* Prevent it from being set from outside */
#if defined(VTSS_SW_OPTION_DOT1X)
#define VLAN_SINGLE_USER_SUPPORT
#endif /* VTSS_SW_OPTION_DOT1X */

#ifdef __cplusplus
VTSS_ENUM_INC(vlan_user_int_t);
#endif /* #ifdef __cplusplus */

// Enumerate the modules requiring this feature.
// Multi VLAN users can add multiple VLANs and specify
// exactly which ports are members of which VLANs.
typedef enum {
    VLAN_MULTI_STATIC,
#ifdef VTSS_SW_OPTION_MVRP
    VLAN_MULTI_MVRP,
#endif
#ifdef VTSS_SW_OPTION_GVRP
    VLAN_MULTI_GVRP,
#endif
#ifdef VTSS_SW_OPTION_MVR
    VLAN_MULTI_MVR,
#endif
#ifdef VTSS_SW_OPTION_RMIRROR
    VLAN_MULTI_RMIRROR,
#endif
    VLAN_MULTI_CNT  /**< This must come last */
} multi_user_index_t;

// "Same" VLAN users can configure ports to be members of at most one VLAN.
typedef enum {
#ifdef VTSS_SW_OPTION_VOICE_VLAN
    VLAN_SAME_VOICE_VLAN,
#endif
#ifdef VTSS_SW_OPTION_RMIRROR
    VLAN_SAME_RMIRROR,
#endif
    VLAN_SAME_CNT
} same_user_index_t;

// "Single" VLAN users can configure different ports to be members
// of different VLANs, but at most one VLAN per port.
typedef enum {
#ifdef VTSS_SW_OPTION_DOT1X
    VLAN_SINGLE_DOT1X,
#endif
    VLAN_SINGLE_CNT
} single_user_index_t;

typedef struct {
    mesa_vid_t   vid;   /* VLAN ID   */
    vlan_ports_t entry; /* Port mask */
} vlan_entry_single_switch_with_vid_t;

/* VLAN single membership configuration table */
typedef struct {
    // A VID = VTSS_VID_NULL indicates that this [isid][port] is not a member of any VLAN.
    // The [isid] index is zero-based (i.e. idx 0 == VTSS_ISID_START).
    CapArray<mesa_vid_t, VTSS_APPL_CAP_ISID_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> vid;
} vlan_single_membership_entry_t;

typedef struct {
    // Zero-based ISIDs.
    mesa_port_list_t ports[VTSS_ISID_CNT];
} vlan_entry_t;

typedef struct {
    mesa_vid_t vid;
    vlan_entry_t entry;
} vlan_entry_with_vid_t;

/* VLAN entry */
typedef struct vlan_list_entry_t {
    struct vlan_list_entry_t *next;
    vlan_entry_t user_entries[VLAN_MULTI_CNT];
} vlan_list_entry_t;

/****************************************************************************/
// Global variables
/****************************************************************************/

// Structure holding port-conf-change callback registrants
typedef struct {
    vtss_module_id_t                 modid;         // Identifies module (debug purposes)
    vlan_port_conf_change_callback_t cb;            // Callback function
    BOOL                             cb_on_primary; // Callback on local or on primary switch?
} vlan_port_conf_change_cb_conf_t;

// Global structure
static vlan_port_conf_change_cb_conf_t VLAN_pcc_cb[10]; // Shorthand for "port-conf-change-callback"

// Structure holding svl-conf-change callback registrants
typedef struct {
    vtss_module_id_t                modid; // Identifies module (debug purposes)
    vlan_svl_conf_change_callback_t cb;    // Callback function
} vlan_svl_conf_change_cb_conf_t;

static vlan_svl_conf_change_cb_conf_t VLAN_svl_cb[1];

typedef struct {
    vtss_module_id_t modid; // Identifies module (debug purposes)

    union {
        vlan_membership_change_callback_t        cb; // Non-bulk callback function
        vlan_membership_bulk_change_callback_t  bcb; // Bulk callback function
        vlan_s_custom_etype_change_callback_t   scb; // S-custom EtherType change callback function
    } u;
} vlan_change_cb_conf_t;

static vlan_change_cb_conf_t VLAN_mc_cb[10];             // Shorthand for "membership-change-callback"
static vlan_change_cb_conf_t VLAN_mc_bcb[10];            // Shorthand for "membership-change-bulk-callback"
static vlan_change_cb_conf_t VLAN_s_custom_etype_cb[10];

/**
 * Points to a list of unused vlan_list_entry_t items.
 */
static vlan_list_entry_t *VLAN_free_list;

/**
 * Storage area for the VLANs that can be configured.
 * There are VLAN_ENTRY_CNT such VLANs, not to be confused
 * with the VLAN IDs that can be configured.
 * During initialization, it is stiched together and
 * a pointer to the first item is stored in #VLAN_free_list,
 * and this table is therefore not directly referred to anymore.
 *
 * The entries are moved back and forth between VLAN_free_list
 * and VLAN_multi_table.
 */
vlan_list_entry_t VLAN_table[VLAN_ENTRY_CNT];
static vlan_list_entry_t *VLAN_multi_table[VTSS_APPL_VLAN_ID_MAX + 1];

// All VIDs (not just VLAN_ENTRY_CNT) must be available in the forbidden list.
static vlan_entry_t VLAN_forbidden_table[VTSS_APPL_VLAN_ID_MAX + 1];

// All VIDs (not just VLAN_ENTRY_CNT) must be available in the combined list.
static vlan_entry_t VLAN_combined_table[VTSS_APPL_VLAN_ID_MAX + 1];

#ifdef VLAN_SAME_USER_SUPPORT
// "Same" VLAN users can configure exactly one VLAN.
static vlan_entry_with_vid_t VLAN_same_table[VLAN_SAME_CNT];
#endif /* VLAN_SAME_USER_SUPPORT */

#ifdef VLAN_SINGLE_USER_SUPPORT
static vlan_single_membership_entry_t VLAN_single_table[VLAN_SINGLE_CNT];
#endif /* VLAN_SINGLE_USER_SUPPORT */

static mesa_vid_t vlan_fid_table[VLAN_ENTRY_CNT];
#define VLAN_FID_GET(_vid_)        vlan_fid_table[(_vid_) - VTSS_APPL_VLAN_ID_MIN]
#define VLAN_FID_SET(_vid_, _fid_) vlan_fid_table[(_vid_) - VTSS_APPL_VLAN_ID_MIN] = (_fid_)

/**
  * This array will store all the VLAN Users' port
  * configuration. Notice that one extra VLAN user entry is added here:
  * currently configured port properties are stored in the
  * VLAN_port_detailed_conf[VLAN_USER_INT_ALL] entry.
  */
static CapArray<vtss_appl_vlan_port_detailed_conf_t, VTSS_APPL_CAP_VLAN_USER_INT_CNT, VTSS_APPL_CAP_ISID_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> VLAN_port_detailed_conf;

// Static VLAN user configuration.
static CapArray<vtss_appl_vlan_port_conf_t, VTSS_APPL_CAP_ISID_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> VLAN_port_conf;

static mesa_etype_t VLAN_tpid_s_custom_port; /* EtherType for Custom S-ports */

#ifdef VTSS_SW_OPTION_VLAN_NAMING
/**
 * VLAN names.
 *
 * By default, only VLAN_name_conf[VTSS_APPL_VLAN_ID_DEFAULT] has a non-standard name ("default").
 *
 * All other VLANs have default names, which are "VLANxxxx", where
 * "xxxx" represent four numeric digits (with leading zeroes) equal to the VLAN ID.
 *
 * An empty string indicates that the VLAN has its default name.
 *
 * The code takes care of disallowing changing the name of e.g VID 7 to VLAN0003
 */
static char VLAN_name_conf[VTSS_APPL_VLAN_ID_MAX + 1][VTSS_APPL_VLAN_NAME_MAX_LEN];
#endif

/**
 * This array holds which VLANs the static user has enabled.
 * It's in order to be able to distinguish automatically
 * added memberships due to trunk/hybrid ports from
 * statically added (ICLI: vlan <vlan_list>).
 */
static u8 VLAN_end_user_enabled_vlans[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];

/**
 * This array holds which VLANs has flooding enabled.
 */
static u8 VLAN_flooding_vlans[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];

/**
 * The following array tells whether a given user has
 * added non-zero memberships to a VID on a given switch.
 * Use VTSS_BF_GET()/VTSS_BF_SET() operations on the array.
 */
static u8 VLAN_non_zero_membership[VTSS_APPL_VLAN_ID_MAX + 1][VLAN_USER_INT_CNT][VTSS_BF_SIZE(VTSS_ISID_CNT)];

/**
 * This structure is used to capture multiple changes
 * to VLANs in order to minimize the number of calls
 * into VLAN membership subscribers and to msg_tx()
 * with new VLAN memberships.
 *
 * The structure is protected by VLAN_crit.
 */
static struct {
    /**
     * Reference counts VLAN_bulk_begin()/VLAN_bulk_end() calls.
     * When non-zero, all updates are cached.
     * When it goes from 1 to 0, changes are applied and subscribers are called back.
     */
    u32 ref_cnt;

    /**
     * Zero-based indexed per-switch array, holding infor
     * about changes and who to send them to.
     */
    struct {
        /**
         * This tells whether at least one VLAN has changed for this switch.
         * Used when notifying subscribers.
         */
        BOOL dirty;

        /**
         * The following is used when notifying subscribers.
         * It contains a bit for every *possibly* changed VLAN.
         * Due to the re-entrancy nature of the VLAN module, it might be
         * that no changes have really occurred even when a bit in this array is 1.
         */
        u8 dirty_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];

        /**
         * The following is used when notifying subscribers.
         * It contains the membership as they were last time
         * the subscribers were called back.
         *
         * Index 0 == VLAN_USER_INT_STATIC, index 1 == VLAN_USER_INT_FORBIDDEN
         */
        vlan_ports_t old_members[2][VTSS_APPL_VLAN_ID_MAX + 1]; // Ditto

        /**
         * The following are used when figuring out whether to transmit configuration to given switches
         * tx_conf[isid][VTSS_VID_NULL] == 1 indicates that at least one bit is set in the remainder.
         */
        u8 tx_conf[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];

    } s[VTSS_ISID_CNT]; // Zero-based.

    /**
     * The following tells a given switch whether it should flush all entries
     * before applying new. Will be TRUE only upon INIT_CMD_ICFG_LOADING_POST
     * events.
     */
    BOOL flush[VTSS_ISID_CNT];

} VLAN_bulk;

typedef enum {
    VLAN_BIT_OPERATION_OVERWRITE,
    VLAN_BIT_OPERATION_ADD,
    VLAN_BIT_OPERATION_DEL,
} vlan_bit_operation_t;

static critd_t VLAN_crit;
static critd_t VLAN_cb_crit;

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "vlan", "VLAN table"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_CB] = {
        "callback",
        "VLAN Callback",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_CLI] = {
        "CLI",
        "Command line interface",
        VTSS_TRACE_LVL_WARNING
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define VLAN_CRIT_ENTER()            critd_enter(        &VLAN_crit,    __FUNCTION__, __LINE__)
#define VLAN_CRIT_EXIT()             critd_exit(         &VLAN_crit,    __FUNCTION__, __LINE__)
#define VLAN_CRIT_ASSERT_LOCKED()    critd_assert_locked(&VLAN_crit,    __FUNCTION__, __LINE__)
#define VLAN_CB_CRIT_ENTER()         critd_enter(        &VLAN_cb_crit, __FUNCTION__, __LINE__)
#define VLAN_CB_CRIT_EXIT()          critd_exit(         &VLAN_cb_crit, __FUNCTION__, __LINE__)
#define VLAN_CB_CRIT_ASSERT_LOCKED() critd_assert_locked(&VLAN_cb_crit, __FUNCTION__, __LINE__)

// VLAN_IN_USE_ON_SWITCH_GET() may be invoked with #vid == VTSS_VID_NULL, but
// VLAN_IN_USE_ON_SWITCH_SET() MUST NOT be invoked with #vid == VTSS_VID_NULL.
#define VLAN_IN_USE_ON_SWITCH_GET(_isid_, _vid_, _user_)        VTSS_BF_GET(VLAN_non_zero_membership[_vid_][_user_], (_isid_) - VTSS_ISID_START)
#define VLAN_IN_USE_ON_SWITCH_SET(_isid_, _vid_, _user_, _val_) VTSS_BF_SET(VLAN_non_zero_membership[_vid_][_user_], (_isid_) - VTSS_ISID_START, _val_)

/******************************************************************************/
// Various local functions
/******************************************************************************/

/******************************************************************************/
// VLAN_user_int_to_ext()
// Converts an internal VLAN user enum to an external VLAN user enum.
// It cannot return errors, because the external enumeration is always present
// regardless of compile-time defines.
/******************************************************************************/
static vtss_appl_vlan_user_t VLAN_user_int_to_ext(vlan_user_int_t user)
{
    switch (user) {
    case VLAN_USER_INT_STATIC:
        return VTSS_APPL_VLAN_USER_STATIC;
#if defined(VTSS_SW_OPTION_DOT1X)
    case VLAN_USER_INT_DOT1X:
        return VTSS_APPL_VLAN_USER_DOT1X;
#endif
#if defined(VTSS_SW_OPTION_MVRP)
    case VLAN_USER_INT_MVRP:
        return VTSS_APPL_VLAN_USER_MVRP;
#endif
#if defined(VTSS_SW_OPTION_GVRP)
    case VLAN_USER_INT_GVRP:
        return VTSS_APPL_VLAN_USER_GVRP;
#endif
#if defined(VTSS_SW_OPTION_MVR)
    case VLAN_USER_INT_MVR:
        return VTSS_APPL_VLAN_USER_MVR;
#endif
#if defined(VTSS_SW_OPTION_VOICE_VLAN)
    case VLAN_USER_INT_VOICE_VLAN:
        return VTSS_APPL_VLAN_USER_VOICE_VLAN;
#endif
#if defined(VTSS_SW_OPTION_MSTP)
    case VLAN_USER_INT_MSTP:
        return VTSS_APPL_VLAN_USER_MSTP;
#endif
#if defined(VTSS_SW_OPTION_ERPS)
    case VLAN_USER_INT_ERPS:
        return VTSS_APPL_VLAN_USER_ERPS;
#endif
#if defined(VTSS_SW_OPTION_IEC_MRP)
    case VLAN_USER_INT_IEC_MRP:
        return VTSS_APPL_VLAN_USER_IEC_MRP;
#endif
#if defined(VTSS_SW_OPTION_VCL)
    case VLAN_USER_INT_VCL:
        return VTSS_APPL_VLAN_USER_VCL;
#endif
#if defined(VTSS_SW_OPTION_RMIRROR)
    case VLAN_USER_INT_RMIRROR:
        return VTSS_APPL_VLAN_USER_RMIRROR;
#endif
    case VLAN_USER_INT_FORBIDDEN:
        return VTSS_APPL_VLAN_USER_FORBIDDEN;
    case VLAN_USER_INT_ALL:
        return VTSS_APPL_VLAN_USER_ALL;
    default:
        T_E("Invoked with unknown user: %d", user);
        return VTSS_APPL_VLAN_USER_ALL; /* Whatever */
    }
}

/******************************************************************************/
// VLAN_user_ext_to_int()
// Converts an external VLAN user enum to an internal VLAN user enum.
// Since external enums always exist independent of compile-time defines,
// it could be that public VLAN functions get invoked with what looks as a
// valid external enum, but what happens to be invalid in a particular product
// because the corresponding module is not enabled.
//
// If this happens, the function returns VLAN_USER_INT_CNT. No trace error is
// produced.
// If the value passed is not a valid ext_user enum, the function returns
// VLAN_USER_INT_CNT and a trace error is produced.
/******************************************************************************/
static vlan_user_int_t VLAN_user_ext_to_int(vtss_appl_vlan_user_t ext_user)
{
    switch (ext_user) {
    case VTSS_APPL_VLAN_USER_ALL:
        return VLAN_USER_INT_ALL;
    case VTSS_APPL_VLAN_USER_STATIC:
        return VLAN_USER_INT_STATIC;
    case VTSS_APPL_VLAN_USER_FORBIDDEN:
        return VLAN_USER_INT_FORBIDDEN;
    case VTSS_APPL_VLAN_USER_DOT1X:
#if defined(VTSS_SW_OPTION_DOT1X)
        return VLAN_USER_INT_DOT1X;
#else
        return VLAN_USER_INT_CNT;
#endif
    case VTSS_APPL_VLAN_USER_MVRP:
#if defined(VTSS_SW_OPTION_MVRP)
        return VLAN_USER_INT_MVRP;
#else
        return VLAN_USER_INT_CNT;
#endif
    case VTSS_APPL_VLAN_USER_GVRP:
#if defined(VTSS_SW_OPTION_GVRP)
        return VLAN_USER_INT_GVRP;
#else
        return VLAN_USER_INT_CNT;
#endif
    case VTSS_APPL_VLAN_USER_MVR:
#if defined(VTSS_SW_OPTION_MVR)
        return VLAN_USER_INT_MVR;
#else
        return VLAN_USER_INT_CNT;
#endif
    case VTSS_APPL_VLAN_USER_VOICE_VLAN:
#if defined(VTSS_SW_OPTION_VOICE_VLAN)
        return VLAN_USER_INT_VOICE_VLAN;
#else
        return VLAN_USER_INT_CNT;
#endif
    case VTSS_APPL_VLAN_USER_MSTP:
#if defined(VTSS_SW_OPTION_MSTP)
        return VLAN_USER_INT_MSTP;
#else
        return VLAN_USER_INT_CNT;
#endif
    case VTSS_APPL_VLAN_USER_ERPS:
#if defined(VTSS_SW_OPTION_ERPS)
        return VLAN_USER_INT_ERPS;
#else
        return VLAN_USER_INT_CNT;
#endif
    case VTSS_APPL_VLAN_USER_IEC_MRP:
#if defined(VTSS_SW_OPTION_IEC_MRP)
        return VLAN_USER_INT_IEC_MRP;
#else
        return VLAN_USER_INT_CNT;
#endif
    case VTSS_APPL_VLAN_USER_MEP_OBSOLETE:
        return VLAN_USER_INT_CNT;
    case VTSS_APPL_VLAN_USER_EVC_OBSOLETE:
        return VLAN_USER_INT_CNT;
    case VTSS_APPL_VLAN_USER_VCL:
#if defined(VTSS_SW_OPTION_VCL)
        return VLAN_USER_INT_VCL;
#else
        return VLAN_USER_INT_CNT;
#endif
    case VTSS_APPL_VLAN_USER_RMIRROR:
#if defined(VTSS_SW_OPTION_RMIRROR)
        return VLAN_USER_INT_RMIRROR;
#else
        return VLAN_USER_INT_CNT;
#endif
    case VTSS_APPL_VLAN_USER_TT_LOOP_OBSOLETE:
        return VLAN_USER_INT_CNT;
    default:
        T_E("Invoked with unknown user: %d", ext_user);
        return VLAN_USER_INT_CNT;
    }
}

/******************************************************************************/
// VLAN_user_to_multi_idx()
// If not a valid multi-VLAN user, returns VLAN_MULTI_CNT.
/******************************************************************************/
static inline multi_user_index_t VLAN_user_to_multi_idx(vlan_user_int_t user)
{
    switch (user) {
    case VLAN_USER_INT_STATIC:
        return VLAN_MULTI_STATIC;
#if defined(VTSS_SW_OPTION_MVRP)
    case VLAN_USER_INT_MVRP:
        return VLAN_MULTI_MVRP;
#endif /* defined(VTSS_SW_OPTION_MVRP) */
#if defined(VTSS_SW_OPTION_GVRP)
    case VLAN_USER_INT_GVRP:
        return VLAN_MULTI_GVRP;
#endif /* defined(VTSS_SW_OPTION_GVRP) */
#if defined(VTSS_SW_OPTION_MVR)
    case VLAN_USER_INT_MVR:
        return VLAN_MULTI_MVR;
#endif /* defined(VTSS_SW_OPTION_MVR) */
#if defined(VTSS_SW_OPTION_RMIRROR)
    case VLAN_USER_INT_RMIRROR:
        return VLAN_MULTI_RMIRROR;
#endif /* defined(VTSS_SW_OPTION_RMIRROR) */
    default:
        return VLAN_MULTI_CNT;
    }
}

/******************************************************************************/
// VLAN_multi_idx_to_user()
/******************************************************************************/
static inline vlan_user_int_t VLAN_multi_idx_to_user(u32 multi_idx)
{
    switch (multi_idx) {
    case VLAN_MULTI_STATIC:
        return VLAN_USER_INT_STATIC;
#if defined(VTSS_SW_OPTION_MVRP)
    case VLAN_MULTI_MVRP:
        return VLAN_USER_INT_MVRP;
#endif /* defined(VTSS_SW_OPTION_MVRP) */
#if defined(VTSS_SW_OPTION_GVRP)
    case VLAN_MULTI_GVRP:
        return VLAN_USER_INT_GVRP;
#endif /* defined(VTSS_SW_OPTION_GVRP) */
#if defined(VTSS_SW_OPTION_MVR)
    case VLAN_MULTI_MVR:
        return VLAN_USER_INT_MVR;
#endif /* defined(VTSS_SW_OPTION_MVR) */
#if defined(VTSS_SW_OPTION_RMIRROR)
    case VLAN_MULTI_RMIRROR:
        return VLAN_USER_INT_RMIRROR;
#endif /* defined(VTSS_SW_OPTION_RMIRROR) */
    default:
        return VLAN_USER_INT_CNT;
    }
}

#if defined(VLAN_SAME_USER_SUPPORT)
/******************************************************************************/
// VLAN_user_to_same_idx()
// If not a valid same-VLAN user, returns VLAN_SAME_CNT.
/******************************************************************************/
static inline same_user_index_t VLAN_user_to_same_idx(vlan_user_int_t user)
{
    switch (user) {
#if defined(VTSS_SW_OPTION_VOICE_VLAN)
    case VLAN_USER_INT_VOICE_VLAN:
        return VLAN_SAME_VOICE_VLAN;
#endif /* defined(VTSS_SW_OPTION_VOICE_VLAN) */
    default:
        return VLAN_SAME_CNT;
    }
}
#endif /* defined(VLAN_SAME_USER_SUPPORT) */

#if defined(VLAN_SINGLE_USER_SUPPORT)
/******************************************************************************/
// VLAN_user_to_single_idx()
// If not a valid single-VLAN user, returns VLAN_SINGLE_CNT.
/******************************************************************************/
static inline single_user_index_t VLAN_user_to_single_idx(vlan_user_int_t user)
{
    switch (user) {
#if defined(VTSS_SW_OPTION_DOT1X)
    case VLAN_USER_INT_DOT1X:
        return VLAN_SINGLE_DOT1X;
#endif /* defined(VTSS_SW_OPTION_DOT1X) */
    default:
        return VLAN_SINGLE_CNT;
    }
}
#endif /* defined(VLAN_SINGLE_USER_SUPPORT) */

/******************************************************************************/
// VLAN_bit_to_bool()
// Converts a bit-array of ports to a boolean array of ports.
// The number of bits to convert depends on #isid.
// You may call this with VTSS_ISID_LOCAL or a legal ISID.
// Any stack ports are lost in this conversion.
// Returns TRUE if at least one port is set in the result, FALSE otherwise.
/******************************************************************************/
static BOOL VLAN_bit_to_bool(vtss_isid_t isid, vlan_ports_t *src, vtss_appl_vlan_entry_t *dst, u64 *port_mask)
{
    port_iter_t pit;
    BOOL        at_least_one_bit_set = FALSE;

    memset(dst->ports, 0, sizeof(dst->ports));

    *port_mask = 0;

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        if (src->ports[pit.iport]) {
            dst->ports[pit.iport] = TRUE;
            at_least_one_bit_set = TRUE;
            *port_mask |= VTSS_BIT64(pit.iport);
        }
    }

    return at_least_one_bit_set;
}

/******************************************************************************/
// VLAN_bool_to_bit()
// Converts a boolean array of ports to a bit-array of ports.
// The number of entries to convert depends on #isid.
// You may call this with VTSS_ISID_LOCAL or a legal ISID.
// Any stack ports are lost in this conversion.
/******************************************************************************/
static void VLAN_bool_to_bit(vtss_isid_t isid, vtss_appl_vlan_entry_t *src, vlan_ports_t *dst)
{
    port_iter_t pit;
    vtss_clear(*dst);

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        dst->ports[pit.iport] = src->ports[pit.iport];
    }
}

/******************************************************************************/
// VLAN_port_conf_change_callback()
/******************************************************************************/
static void VLAN_port_conf_change_callback(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_vlan_port_detailed_conf_t *conf)
{
    vlan_port_conf_change_cb_conf_t local_cb[ARRSZ(VLAN_pcc_cb)];
    u32                             i;

    VLAN_CB_CRIT_ENTER();
    // Take a copy in order to avoid deadlock issues
    memcpy(local_cb, VLAN_pcc_cb, sizeof(local_cb));
    VLAN_CB_CRIT_EXIT();

    for (i = 0; i < ARRSZ(local_cb); i++) {
        if (local_cb[i].cb == NULL) {
            // Since there is no un-register support,
            // we know that there are no more registrants
            // when we meet a NULL pointer.
            break;
        }

        if (isid == VTSS_ISID_LOCAL) {
            // Local switch change
            if (local_cb[i].cb_on_primary) {
                // Not interested in getting called back
                continue;
            }
        } else {
            // Primary switch change
            if (!local_cb[i].cb_on_primary) {
                // Not interested in getting called back
                continue;
            }
        }

        T_DG(TRACE_GRP_CB, "%u:%u: Calling back %s", isid, port_no, vtss_module_names[local_cb[i].modid]);
        local_cb[i].cb(isid, port_no, conf);
    }
}

/******************************************************************************/
// VLAN_get()
//
// Returns a specific user's (next) VLAN membership/VID.
//
// #isid:   Legal ISID or VTSS_ISID_GLOBAL. If legal ISID, also get membership, otherwise only get (next) VID.
// #vid:    [0; VTSS_APPL_VLAN_ID_MAX], 0 only if #next == TRUE.
// #next:   TRUE if find a VID > #vid. FALSE if getting membership-enabledness for #vid.
// #user:   Can be anything from [VLAN_USER_INT_STATIC; VLAN_USER_INT_ALL]
// #result: May be NULL if not interested in memberships.
//
// Returns VTSS_VID_NULL if no such (next) entry was found, and a valid VID if it
// was found. In that case, #result is updated if isid != VTSS_ISID_GLOBAL.
/******************************************************************************/
static mesa_vid_t VLAN_get(vtss_isid_t isid, mesa_vid_t vid, vlan_user_int_t user, BOOL next, vlan_ports_t *result)
{
    BOOL        found = FALSE;
    vtss_isid_t isid_iter, isid_min, isid_max;
    mesa_vid_t  vid_min, vid_max;
    u32         multi_user_idx = VLAN_user_to_multi_idx(user);
#if defined(VLAN_SAME_USER_SUPPORT)
    u32         same_user_idx  = VLAN_user_to_same_idx(user);
#endif /* defined(VLAN_SAME_USER_SUPPORT) */
#if defined(VLAN_SINGLE_USER_SUPPORT)
    u32         single_user_idx = VLAN_user_to_single_idx(user);
#endif /* defined(VLAN_SINGLE_USER_SUPPORT) */

    VLAN_CRIT_ASSERT_LOCKED();

    T_D("isid %d, vid %d, next %d, user %s", isid, vid, next, vlan_mgmt_user_to_txt(VLAN_user_int_to_ext(user)));

    // Zero it out to start with
    if (result) {
        vtss_clear(*result);
    }

    if (next == FALSE && (vid < VTSS_APPL_VLAN_ID_MIN || vid > VTSS_APPL_VLAN_ID_MAX)) {
        T_E("Ehh? (%d)", vid);
        return VTSS_VID_NULL;
    }

    if (user > VLAN_USER_INT_ALL) {
        T_E("Invalid user %d", user);
        return VTSS_VID_NULL;
    }

    if (isid == VTSS_ISID_GLOBAL) {
        isid_min = VTSS_ISID_START;
        isid_max = VTSS_ISID_END - 1;
    } else {
        isid_min = isid_max = isid;
    }

    if (next) {
        vid_min = vid + 1;
        vid_max = VTSS_APPL_VLAN_ID_MAX;
    } else {
        vid_min = vid_max = vid;
    }

    // Loop through to find the (next) valid VID.
    for (vid = vid_min; vid <= vid_max; vid++) {
        for (isid_iter = isid_min; isid_iter <= isid_max; isid_iter++) {
            if (VLAN_IN_USE_ON_SWITCH_GET(isid_iter, vid, user)) {
                found = TRUE;
                break;
            }
        }

        if (found) {
            break;
        }
    }

    // Now, if #found is TRUE, #vid points to the VID we're looking for.

    if (result == NULL || isid == VTSS_ISID_GLOBAL || !found) {
        // If #isid is not a legal ISID, the caller only wants to know
        // whether the entry exists, so we can exit now.
        // Also, if an entry was not found, we can exit.
        return found ? vid : VTSS_VID_NULL;
    }

    // If we get here, the caller wants us to fill in the membership info as well for a specific ISID.

    // Forbidden, Combined, Multi-, and Same- VLAN users all use the same structure.
    if (user == VLAN_USER_INT_FORBIDDEN || user == VLAN_USER_INT_ALL || multi_user_idx != VLAN_MULTI_CNT
#if defined(VLAN_SAME_USER_SUPPORT)
        || same_user_idx != VLAN_SAME_CNT
#endif /* defined(VLAN_SAME_USER_SUPPORT) */
       ) {
        // Caller is interested in getting forbidden, combined, multi- or same-user port membership for VID #vid.
        vlan_entry_t *entry;

        if (user == VLAN_USER_INT_FORBIDDEN) {
            entry = &VLAN_forbidden_table[vid];
        } else if (user == VLAN_USER_INT_ALL) {
            entry = &VLAN_combined_table[vid];
        } else if (multi_user_idx != VLAN_MULTI_CNT) {
            if (VLAN_multi_table[vid] == NULL) {
                // in_use table and membership table are out of sync.
                // This should not be possible.
                T_E("Internal error");
                return VTSS_VID_NULL;
            }

            entry = &VLAN_multi_table[vid]->user_entries[multi_user_idx];
        }

#if defined(VLAN_SAME_USER_SUPPORT)
        else if (same_user_idx != VLAN_SAME_CNT) {
            // "Same" user. Such users can only configure one single VLAN.
            if (VLAN_same_table[same_user_idx].vid != vid) {
                // in_use table and configured VID are out of sync.
                // This should not be possible.
                T_E("Internal error (vid=%d, %d). User %s", VLAN_same_table[same_user_idx].vid, vid, vlan_mgmt_user_to_txt(VLAN_user_int_to_ext(user)));
                return VTSS_VID_NULL;
            }

            entry = &VLAN_same_table[same_user_idx].entry;
        }
#endif /* defined(VLAN_SAME_USER_SUPPORT) */

        else {
            T_E("How did this happen?");
            return VTSS_VID_NULL;
        }

        result->ports = entry->ports[isid - VTSS_ISID_START];
    }

#if defined(VLAN_SINGLE_USER_SUPPORT)
    else if (single_user_idx != VLAN_SINGLE_CNT) {
        // A "single" user is a user that can configure different ports to be members of different VLANs,
        // but at most one VLAN per port.
        vlan_single_membership_entry_t *entry = &VLAN_single_table[single_user_idx];
        port_iter_t                    pit;

        (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (entry->vid[isid - VTSS_ISID_START][pit.iport] == vid) {
                result->ports[pit.iport] = TRUE;
            }
        }

        // It is not mandated that any ports are now members, because it's possible to create an empty VLAN.
    }
#endif /* defined(VLAN_SINGLE_USER_SUPPORT) */

    else {
        // Here, we should have handled the user already.
        T_E("Invalid VLAN User %d", user);
    }

    return vid;
}

/******************************************************************************/
// VLAN_membership_api_set()
// Add/Delete VLAN entry to/from switch API.
/******************************************************************************/
static mesa_rc VLAN_membership_api_set(mesa_vid_t vid, vlan_ports_t *conf)
{
    mesa_rc                rc;
    vtss_appl_vlan_entry_t dst;
    mesa_port_list_t       port_list;
    mesa_port_no_t         iport;

    // Figure out if user is adding or deleting VID
    if (conf) {
        u64 port_mask;
        (void)VLAN_bit_to_bool(VTSS_ISID_LOCAL, conf, &dst, &port_mask);
        T_D("VLAN Add: vid %u port_mask = 0x%08" PRIx64, vid, port_mask);
    } else {
        vtss_clear(dst);
        T_D("VLAN Delete: vid %u", vid);
    }

    for (iport = 0; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
        port_list[iport] = dst.ports[iport];
    }
    if ((rc = mesa_vlan_port_members_set(NULL, vid, &port_list)) != VTSS_RC_OK) {
        T_E("vtss_vlan_port_member_set(): %s", error_txt(rc));
        return rc;
    }

    // Enable port isolation for this VLAN
    return mesa_isolated_vlan_set(NULL, vid, conf != NULL);
}

/******************************************************************************/
// VLAN_bulk_begin()
/******************************************************************************/
static void VLAN_bulk_begin(void)
{
    VLAN_CRIT_ASSERT_LOCKED();

    // These days, VLAN configuration change commands can be quite powerful,
    // and cause a lot of changes with just a few simple clicks with the mouse
    // or ICLI commands.
    // In order to minimize the number of calls to the VLAN membership API
    // function and to the callback registrants, we do a reference counting
    // mechanism so that when e.g. Web or CLI know that a lot of changes are to
    // be applied to the VLAN module, it calls vlan_bulk_update_begin(), which
    // increases the reference count, and when it's done it calls
    // vlan_bulk_update_end(), which decreases the reference count. Once
    // the reference count reaches 0, any changes applied while the ref.
    // count was greater than 0 are now applied to subscribers and to H/W.
    //
    // Now, the problem is that we cannot hold the VLAN_crit while calling
    // back subscribers, because they might need to call back into the
    // VLAN module. This means that we have to let go of VLAN_crit everytime
    // we call back. This, in turn, means that we must never clear the dirty
    // arrays when ref_cnt increases from 0 to 1. Instead we need to re-run
    // all dirty arrays in the called context whenever ref-count decreases
    // to zero.
    VLAN_bulk.ref_cnt++;
    T_D("New ref. count = %u", VLAN_bulk.ref_cnt);
}

/******************************************************************************/
// VLAN_bulk_end()
/******************************************************************************/
static void VLAN_bulk_end(void)
{
    mesa_vid_t            vid;
    vtss_isid_t           isid, zisid;
    BOOL                  local_flush[VTSS_ISID_CNT];
    BOOL                  at_least_one_change = FALSE;
    vlan_change_cb_conf_t local_vid_cb[ARRSZ(VLAN_mc_cb)], local_bulk_cb[ARRSZ(VLAN_mc_bcb)];
    int                   i;
    mesa_port_no_t        iport;
    mesa_rc               rc;
    vlan_ports_t          entry;
    u32                   port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    // It confuses Lint that we exit then enter instead of enter then exit.
    //lint --e{454,455,456}

    VLAN_CB_CRIT_ENTER();
    // Take a copy in order to avoid deadlock issues
    memcpy(local_vid_cb,  VLAN_mc_cb,  sizeof(local_vid_cb));
    memcpy(local_bulk_cb, VLAN_mc_bcb, sizeof(local_bulk_cb));
    VLAN_CB_CRIT_EXIT();

    VLAN_CRIT_ASSERT_LOCKED();

    if (VLAN_bulk.ref_cnt == 0) {
        T_E("Bulk reference count is already 0. Can't decrease it further");
        return;
    } else {
        VLAN_bulk.ref_cnt--;

        T_D("New ref. count = %u", VLAN_bulk.ref_cnt);

        if (VLAN_bulk.ref_cnt > 0) {
            return;
        }
    }

    T_D("New ref. count = %u", VLAN_bulk.ref_cnt);

    // Take a snapshot of the flush-array, because we need to clear it again
    // and because we need the info when sending notifications back to registrants.
    memcpy(local_flush, VLAN_bulk.flush, sizeof(local_flush));

    // ------------------------oOo------------------------
    // Update H/W
    // ------------------------oOo------------------------
    // Send new configuration to switches in question.
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        zisid = isid - VTSS_ISID_START;

        VLAN_bulk.flush[zisid] = FALSE;

        // Bit 0 (VTSS_VID_NULL) in the array tells whether we should transmit a message or not.
        if (VTSS_BF_GET(VLAN_bulk.s[zisid].tx_conf, VTSS_VID_NULL)) {
            VTSS_BF_SET(VLAN_bulk.s[zisid].tx_conf, VTSS_VID_NULL, FALSE);

            if (!msg_switch_exists(isid)) {
                // Nothing to do, since it doesn't exist.
                continue;
            }

            if (local_flush[zisid]) {
                // We need to delete all currently existing VLANs.
                // This will shortly give rise to no configured VLANs, but that's ignorable.
                T_D("VLAN Table flush");

                for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
                    if ((rc = VLAN_membership_api_set(vid, NULL)) != VTSS_RC_OK) {
                        T_E("VLAN_membership_api_set(del, %u): %d", vid, rc);
                        break;
                    }
                }
            }

            for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
                if (VTSS_BF_GET(VLAN_bulk.s[zisid].tx_conf, vid)) {
                    VTSS_BF_SET(VLAN_bulk.s[zisid].tx_conf, vid, FALSE);

                    // Get this VID's configuration. This may be an empty configuration, so don't
                    // check the return value, which just tells you whether the VID exists or not.
                    (void)VLAN_get(isid, vid, VLAN_USER_INT_ALL, FALSE, &entry);
                    T_D("VLAN Add: vid = %u", vid);
                    if ((rc = VLAN_membership_api_set(vid, &entry)) != VTSS_RC_OK) {
                        T_E("VLAN_membership_api_set(add, %u): %d", vid, rc);
                        break;
                    }
                }
            }

            // VLAN bulk subscribers must be invoked whenever a VLAN changes, whether it's due
            // to VLAN_USER_INT_STATIC or some other user (non-bulk-subscribers only get called
            // back if VLAN_USER_INT_STATIC or VLAN_USER_INT_FORBIDDEN changes).
            at_least_one_change = TRUE;
        }
    }

    // ------------------------oOo------------------------
    // Update subscribers
    // ------------------------oOo------------------------
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        vlan_membership_change_t changes;

        zisid = isid - VTSS_ISID_START;

        if (!VLAN_bulk.s[zisid].dirty) {
            // Nothing to do for this switch.
            continue;
        }

        VLAN_bulk.s[zisid].dirty = FALSE;

        if (local_flush[zisid]) {
            // Pretend that all ports have changed for this VID
            changes.changed_ports.ports.set_all();
        }

        for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
            BOOL notify;

            if (!VTSS_BF_GET(VLAN_bulk.s[zisid].dirty_vids, vid)) {
                continue;
            }

            VTSS_BF_SET(VLAN_bulk.s[zisid].dirty_vids, vid, FALSE);

            // We always notify if flushing remote VLAN table, because that
            // means that all possible old VLANs are getting changed.
            notify = local_flush[zisid];

            changes.static_vlan_exists = VTSS_BF_GET(VLAN_end_user_enabled_vlans, vid);
            (void)VLAN_get(isid, vid, VLAN_USER_INT_STATIC,    FALSE, &changes.static_ports);
            (void)VLAN_get(isid, vid, VLAN_USER_INT_FORBIDDEN, FALSE, &changes.forbidden_ports);

            if (!local_flush[zisid]) {
                // Gotta compute a change mask
                for (iport = 0; iport < port_cnt; iport++) {
                    if (VLAN_bulk.s[zisid].old_members[0][vid].ports[iport] != changes.static_ports.ports[iport] ||
                        VLAN_bulk.s[zisid].old_members[1][vid].ports[iport] != changes.forbidden_ports.ports[iport]) {
                        changes.changed_ports.ports[iport] = TRUE;
                        notify = TRUE;
                    } else {
                        changes.changed_ports.ports[iport] = FALSE;
                    }
                }
            }

            // Call back per-VID subscribers.
            if (notify) {
                // Temporarily exit our crit while calling back in order for the call back functions to
                // be able to call into the VLAN module again.
                VLAN_CRIT_EXIT();
                for (i = 0; i < ARRSZ(local_vid_cb); i++) {
                    if (local_vid_cb[i].u.cb == NULL) {
                        // Since there is no un-register support,
                        // we know that there are no more registrants
                        // when we meet a NULL pointer.
                        break;
                    }

                    T_DG(TRACE_GRP_CB, "%u:%u: Calling back %s", isid, vid, vtss_module_names[local_vid_cb[i].modid]);
                    local_vid_cb[i].u.cb(isid, vid, &changes);
                }

                VLAN_CRIT_ENTER();
            }
        }
    }

    if (at_least_one_change) {
        VLAN_CRIT_EXIT();
        // Call back bulk subscribers.
        for (i = 0; i < ARRSZ(local_bulk_cb); i++) {
            if (local_bulk_cb[i].u.bcb == NULL) {
                // Since there is no un-register support,
                // we know that there are no more registrants
                // when we meet a NULL pointer.
                break;
            }

            T_DG(TRACE_GRP_CB, "Calling back %s", vtss_module_names[local_bulk_cb[i].modid]);
            local_bulk_cb[i].u.bcb();
        }

        VLAN_CRIT_ENTER();
    }

    VLAN_CRIT_ASSERT_LOCKED();
}

/******************************************************************************/
// VLAN_s_custom_etype_change_callback()
/******************************************************************************/
static void VLAN_s_custom_etype_change_callback(mesa_etype_t tpid)
{
    vlan_change_cb_conf_t local_cb[ARRSZ(VLAN_s_custom_etype_cb)];
    int                   i;

    VLAN_CB_CRIT_ENTER();
    // Take a copy in order to avoid deadlock issues
    memcpy(local_cb, VLAN_s_custom_etype_cb, sizeof(local_cb));
    VLAN_CB_CRIT_EXIT();

    for (i = 0; i < ARRSZ(local_cb); i++) {
        if (local_cb[i].u.scb == NULL) {
            // Since there is no un-register support,
            // we know that there are no more registrants
            // when we meet a NULL pointer.
            break;
        }

        T_IG(TRACE_GRP_CB, "Calling back %s with EtherType 0x%04x", vtss_module_names[local_cb[i].modid], tpid);
        local_cb[i].u.scb(tpid);
    }
}

/******************************************************************************/
// VLAN_port_conf_api_set()
// Setup VLAN port configuration via switch API.
/******************************************************************************/
static void VLAN_port_conf_api_set(mesa_port_no_t port_no, vtss_appl_vlan_port_detailed_conf_t *conf)
{
    mesa_rc               rc;
    mesa_vlan_port_conf_t api_conf;

    if (conf->pvid < VTSS_APPL_VLAN_ID_MIN || conf->pvid > VTSS_APPL_VLAN_ID_MAX) {
        T_E("%u: Invalid PVID (%u). Setting to default", port_no, conf->pvid);
        conf->pvid = VTSS_APPL_VLAN_ID_DEFAULT;
    }

    if (conf->tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS || conf->tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS) {
        if (conf->untagged_vid < VTSS_APPL_VLAN_ID_MIN || conf->untagged_vid > VTSS_APPL_VLAN_ID_MAX) {
            T_E("Invalid UVID (%d)", conf->untagged_vid);
        }
    }

    // To be able to seamlessly support future API enhancements,
    // we need to get the current configuration prior to setting
    // a new.
    if ((rc = mesa_vlan_port_conf_get(NULL, port_no, &api_conf)) != VTSS_RC_OK) {
        T_E("Huh (%d)?", port_no);
        memset(&api_conf, 0, sizeof(api_conf)); // What else can we do than resetting to all-zeros?
    }

    switch (conf->port_type) {
    case VTSS_APPL_VLAN_PORT_TYPE_UNAWARE:
        api_conf.port_type = MESA_VLAN_PORT_TYPE_UNAWARE;
        break;

    case VTSS_APPL_VLAN_PORT_TYPE_C:
        api_conf.port_type = MESA_VLAN_PORT_TYPE_C;
        break;

    case VTSS_APPL_VLAN_PORT_TYPE_S:
        api_conf.port_type = MESA_VLAN_PORT_TYPE_S;
        break;

    case VTSS_APPL_VLAN_PORT_TYPE_S_CUSTOM:
        api_conf.port_type = MESA_VLAN_PORT_TYPE_S_CUSTOM;
        break;

    default:
        T_E("Invalid port type %d", conf->port_type);
        break;
    }

    api_conf.pvid = conf->pvid;

    switch (conf->tx_tag_type) {
    case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS:
        api_conf.untagged_vid = conf->untagged_vid;
        break;

    case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS:
        // This is the least prioritized tag-type. If PVID must be tagged, we can never end here.
        // #conf->untagged_vid contains a VLAN ID that we must tag (SIC!).
        // If #conf->untagged_vid == #conf->pvid, then we tag everything by setting #api_conf.untagged_vid to VTSS_VID_NULL.
        // Otherwise we set it to #conf->pvid.
        api_conf.untagged_vid = conf->untagged_vid == conf->pvid ? VTSS_VID_NULL : conf->pvid;
        break;

    case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL:
        api_conf.untagged_vid = VTSS_VID_NULL;
        break;

    case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL:
        api_conf.untagged_vid = VTSS_VID_ALL;
        break;

    default:
        T_E("Invalid tx_tag_type (%d)", conf->tx_tag_type);
        return;
    }

    api_conf.frame_type = conf->frame_type;
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
    // Option available & controllable
    api_conf.ingress_filter = conf->ingress_filter;
#else
    api_conf.ingress_filter = TRUE; // Always enable
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */

    T_D("VLAN Port change port %d, pvid %d", port_no, api_conf.pvid);
    if ((rc = mesa_vlan_port_conf_set(NULL, port_no, &api_conf)) != VTSS_RC_OK) {
        T_E("%u: %s", iport2uport(port_no), error_txt(rc));
        return;
    }

    // Call-back those modules interested in configuration changes on the local switch.
    VLAN_port_conf_change_callback(VTSS_ISID_LOCAL, port_no, conf);
}

/******************************************************************************/
// VLAN_port_conf_api_get()
// Get VLAN port configuration directly from switch API.
/******************************************************************************/
static mesa_rc VLAN_port_conf_api_get(mesa_port_no_t port_no, vtss_appl_vlan_port_detailed_conf_t *conf)
{
    mesa_rc               rc;
    mesa_vlan_port_conf_t api_conf;

    memset(conf, 0, sizeof(*conf));
    rc = mesa_vlan_port_conf_get(NULL, port_no, &api_conf);

    switch (api_conf.port_type) {
    case MESA_VLAN_PORT_TYPE_UNAWARE:
        conf->port_type = VTSS_APPL_VLAN_PORT_TYPE_UNAWARE;
        break;

    case MESA_VLAN_PORT_TYPE_C:
        conf->port_type = VTSS_APPL_VLAN_PORT_TYPE_C;
        break;

    case MESA_VLAN_PORT_TYPE_S:
        conf->port_type = VTSS_APPL_VLAN_PORT_TYPE_S;
        break;

    case MESA_VLAN_PORT_TYPE_S_CUSTOM:
        conf->port_type = VTSS_APPL_VLAN_PORT_TYPE_S_CUSTOM;
        break;

    default:
        T_E("Invalid API port type %d", api_conf.port_type);
        break;
    }

    conf->pvid = api_conf.pvid;
    conf->untagged_vid = api_conf.untagged_vid;
    if (conf->untagged_vid == VTSS_VID_NULL) {
        conf->tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL;
    } else if (conf->untagged_vid == VTSS_VID_ALL) {
        conf->tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL;
    } else if (conf->pvid == conf->untagged_vid) {
        conf->tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS;
    } else {
        conf->tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS;
    }

    conf->frame_type = api_conf.frame_type;
#if defined(VTSS_SW_OPTION_VLAN_INGRESS_FILTERING)
    conf->ingress_filter = api_conf.ingress_filter;
#endif  /* defined(VTSS_SW_OPTION_VLAN_INGRESS_FILTERING) */

    return rc;
}

/******************************************************************************/
// VLAN_api_get()
// Retrieve (next) VID and memberships directly from H/W.
// This is as opposed to VLAN_get(), which works on the software state.
//
// Returns VTSS_VID_NULL if no such (next) entry was found, and a valid VID if
// it was found. In that case, #result is updated.
/******************************************************************************/
static mesa_vid_t VLAN_api_get(mesa_vid_t vid, BOOL next, vlan_ports_t *result)
{
    mesa_rc          rc = VTSS_RC_ERROR;
    mesa_vid_t       vid_min, vid_max;
    mesa_port_list_t ports;
    mesa_port_list_t empty_member;
    BOOL             found = FALSE;

    if (next) {
        vid_min = vid + 1;
        vid_max = VTSS_APPL_VLAN_ID_MAX;
    } else {
        vid_min = vid_max = vid;
    }

    vtss_clear(*result);

    for (vid = vid_min; vid <= vid_max; vid++) {
        if ((rc = mesa_vlan_port_members_get(NULL, vid, &ports)) != VTSS_RC_OK) {
            T_E("VLAN_membership_api_get(): %s", error_txt(rc));
            return VTSS_VID_NULL;
        }

        if (ports != empty_member) {
            found = TRUE;
            result->ports = ports;
            break;
        }
    }

    return found ? vid : VTSS_VID_NULL;
}

/******************************************************************************/
// VLAN_combined_update()
// #isid must be a legal ISID.
// #vid must be a legal VID ([VTSS_APPL_VLAN_ID_MIN; VTSS_APPL_VLAN_ID_MAX]).
/******************************************************************************/
static void VLAN_combined_update(vtss_isid_t isid, mesa_vid_t vid)
{
    vlan_ports_t    combined;
    vtss_isid_t     zisid = isid - VTSS_ISID_START;
    vlan_entry_t    *entry = &VLAN_combined_table[vid];
    BOOL            found = FALSE;
    vlan_user_int_t user;
    mesa_port_no_t  iport;
    u32             port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    VLAN_CRIT_ASSERT_LOCKED();

    for (user = VLAN_USER_INT_STATIC; user < VLAN_USER_INT_ALL; user++) {
        vlan_ports_t user_contrib;

        if (!VLAN_IN_USE_ON_SWITCH_GET(isid, vid, user)) {
            // This user is not contributing to final VLAN.
            continue;
        }

        if (VLAN_get(isid, vid, user, FALSE, &user_contrib) == VTSS_VID_NULL) {
            T_E("Odd. User %s is contributing to VID %u, but VLAN_get() says it's not", vlan_mgmt_user_to_txt(VLAN_user_int_to_ext(user)), vid);
            continue;
        }

        if (user == VLAN_USER_INT_FORBIDDEN) {
            // Forbidden VLANs override everything. The "VLAN_USER_INT_FORBIDDEN" enumeration must
            // be the last in the iteration.
            for (iport = 0; iport < port_cnt; iport++) {
                if (user_contrib.ports[iport]) {
                    combined.ports[iport] = FALSE;
                }
            }
        } else {
            // A forbidden VLAN does not contribute to whether the VLAN exists or not,
            // so only set #found to TRUE on non-forbidden users.
            found = TRUE;
            for (iport = 0; iport < port_cnt; iport++) {
                if (user_contrib.ports[iport]) {
                    combined.ports[iport] = TRUE;
                }
            }
        }
    }

    // Gotta update the bulk changes for later membership subscriber callback and H/W update.
    if (memcmp(entry->ports[zisid], combined.ports, sizeof(entry->ports[zisid])) != 0) {
        VTSS_BF_SET(VLAN_bulk.s[zisid].tx_conf, vid, TRUE);

        // Use vid == VTSS_VID_NULL to indicate that there's something to transmit for this ISID.
        VTSS_BF_SET(VLAN_bulk.s[zisid].tx_conf, VTSS_VID_NULL, TRUE);
    }

    if (found) {
        VLAN_combined_table[vid].ports[zisid] = combined.ports;
    } else {
        VLAN_combined_table[vid].ports[zisid].clear_all();
    }

    VLAN_IN_USE_ON_SWITCH_SET(isid, vid, VLAN_USER_INT_ALL, found);
}

/******************************************************************************/
// VLAN_add()
// Adds or overwrites a VLAN to #isid.
// #isid must be a legal ISID.
// #vid must be a valid VID.
// #user must be in range [VLAN_USER_INT_STATIC; VLAN_USER_INT_ALL[.
//
// If #user == VLAN_USER_INT_STATIC, #entry may contain the empty port set.
// If #user != VLAN_USER_INT_STATIC, #entry must have at least one port set.
//
// This function automatically updates VLAN_USER_INT_ALL with new combined state.
/******************************************************************************/
static mesa_rc VLAN_add(vtss_isid_t isid, mesa_vid_t vid, vlan_user_int_t user, vlan_ports_t *entry)
{
    mesa_rc               rc = VTSS_RC_OK;
    u32                   user_idx;
    vtss_isid_t           zisid = isid - VTSS_ISID_START;
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
    vtss_appl_vlan_user_t ext_user = VLAN_user_int_to_ext(user);
#endif

    VLAN_CRIT_ASSERT_LOCKED();

    T_D("VLAN Add: isid %u vid %u user %s", isid, vid, vlan_mgmt_user_to_txt(ext_user));

    // Various updates for notifications to subscribers and switch updates.
    if (user == VLAN_USER_INT_STATIC || user == VLAN_USER_INT_FORBIDDEN) {
        if (VTSS_BF_GET(VLAN_bulk.s[zisid].dirty_vids, vid) == FALSE) {
            // This VLAN ID has not been touched before on this switch,
            // so we better take a snapshot of the current configuration, so that
            // we can send out any changes to subscribers.
            // The following will also clear the returned port array if the VLAN is currently not existing.
            (void)VLAN_get(isid, vid, VLAN_USER_INT_STATIC,    FALSE, &VLAN_bulk.s[zisid].old_members[0][vid]);
            (void)VLAN_get(isid, vid, VLAN_USER_INT_FORBIDDEN, FALSE, &VLAN_bulk.s[zisid].old_members[1][vid]);

            // Now, this VID is going to be changed and hence dirty
            VTSS_BF_SET(VLAN_bulk.s[zisid].dirty_vids, vid, TRUE);

            // And also tell that this switch is now dirty.
            VLAN_bulk.s[zisid].dirty = TRUE;
        }
    }

    if (user == VLAN_USER_INT_FORBIDDEN) {
        VLAN_forbidden_table[vid].ports[zisid] = entry->ports;
    } else if ((user_idx = VLAN_user_to_multi_idx(user)) != VLAN_MULTI_CNT) {
        // A multi-user is a VLAN user that is allowed to set membership of all ports
        // on all VIDs (as long as there are free entries).
        vlan_list_entry_t *the_vlan;

        if ((the_vlan = VLAN_multi_table[vid]) == NULL) {

            if (VLAN_free_list == NULL) {
                rc = VLAN_ERROR_VLAN_TABLE_FULL;
                goto exit_func;
            }

            // Pick the first item from the free list.
            the_vlan = VLAN_free_list;
            VLAN_free_list = VLAN_free_list->next;
            vtss_clear(*the_vlan);
            VLAN_multi_table[vid] = the_vlan;
        }

        the_vlan->user_entries[user_idx].ports[zisid] = entry->ports;
    }

#if defined(VLAN_SAME_USER_SUPPORT)
    else if ((user_idx = VLAN_user_to_same_idx(user)) != VLAN_SAME_CNT) {
        // "Same" VLAN users can configure one single VLAN.
        vlan_entry_with_vid_t *same_member_entry = &VLAN_same_table[user_idx];

        // Check if the user entry is previously configured
        if (same_member_entry->vid != VTSS_VID_NULL && same_member_entry->vid != vid) {
            T_E("\"Same\" user %s has already configured a VLAN (%d). Attempting to configure %d", vlan_mgmt_user_to_txt(ext_user), same_member_entry->vid, vid);
            rc = VLAN_ERROR_USER_PREVIOUSLY_CONFIGURED;
            goto exit_func;
        }

        // Update the user database
        same_member_entry->vid = vid;

        // Overwrite the entire portion for this ISID
        same_member_entry->entry.ports[zisid] = entry->ports;
    }
#endif /* defined(VLAN_SAME_USER_SUPPORT) */

#if defined(VLAN_SINGLE_USER_SUPPORT)
    else if ((user_idx = VLAN_user_to_single_idx(user)) != VLAN_SINGLE_CNT) {
        // A "single" user is a user that can configure different ports to be members of different VLANs,
        // but at most one VLAN per port.
        vlan_single_membership_entry_t *single_member_entry = &VLAN_single_table[user_idx];
        port_iter_t                    pit;

        // Loop through and set our database's VID for ports set in the
        // users port list. Before setting, check that the entry is not
        // currently used, since that's not allowed for SINGLE users.
        // Also remember to clear out ports that match the user request's
        // VID, but are not set in the user's request (anymore).

        (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            mesa_vid_t *vid_ptr = &single_member_entry->vid[zisid][pit.iport];
            if (entry->ports[pit.iport]) {
                // User wants to make this port member of #vid. Check if it's OK.
                if (*vid_ptr != VTSS_VID_NULL && *vid_ptr != vid) {
                    T_E("User %s has already a VID (%d) configured on port %u:%u. New vid = %u", vlan_mgmt_user_to_txt(ext_user), *vid_ptr, isid, pit.uport, vid);
                    rc = VLAN_ERROR_USER_PREVIOUSLY_CONFIGURED;
                    goto exit_func;
                }

                *vid_ptr = vid;
            } else if (*vid_ptr == vid) {
                // User no longer wants this port to be member of the VLAN ID.
                *vid_ptr = VTSS_VID_NULL;
            }
        }
    }
#endif /* defined(VLAN_SINGLE_USER_SUPPORT) */
    else {
        T_E("Ehh (user = %d)", user);
        rc = VLAN_ERROR_USER;
    }

exit_func:
    if (rc == VTSS_RC_OK) {
        // Success. Update this user's in-use flag.
        VLAN_IN_USE_ON_SWITCH_SET(isid, vid, user, TRUE);

        // And go on and update the combined user's membership and in-use flag.
        // Unfortunately, it's not good enough to simply OR the new user membership
        // onto the current combined memership, because the new membership may
        // have cleared some ports while adding others on this ISID. And there is
        // no guarantee that just because this user clears a port, that the port
        // should be cleared in the combined membership (another user may have it set).
        VLAN_combined_update(isid, vid);
    }

    return rc;
}

/******************************************************************************/
// VLAN_del()
// #isid must be a legal ISID.
// #vid  must be a legal VID.
// #user must be in range [VLAN_USER_INT_STATIC; VLAN_USER_INT_ALL[
/******************************************************************************/
static void VLAN_del(vtss_isid_t isid, mesa_vid_t vid, vlan_user_int_t user)
{
    u32             user_idx;
    BOOL            in_use;
    vtss_isid_t     isid_iter;
    vlan_user_int_t user_iter;
    vtss_isid_t     zisid = isid - VTSS_ISID_START;

    VLAN_CRIT_ASSERT_LOCKED();

    if (!VLAN_IN_USE_ON_SWITCH_GET(isid, vid, user)) {
        // #user does not have a share on the final VID on this switch.
        return;
    }

    // Various updates for notifications to subscribers.
    if (user == VLAN_USER_INT_STATIC || user == VLAN_USER_INT_FORBIDDEN) {
        if (VTSS_BF_GET(VLAN_bulk.s[zisid].dirty_vids, vid) == FALSE) {
            // Get the old members before it was deleted, so that we can tell subscribers about changes.
            (void)VLAN_get(isid, vid, VLAN_USER_INT_STATIC,    FALSE, &VLAN_bulk.s[zisid].old_members[0][vid]);
            (void)VLAN_get(isid, vid, VLAN_USER_INT_FORBIDDEN, FALSE, &VLAN_bulk.s[zisid].old_members[1][vid]);

            // Now, this VID is going to be changed and hence dirty
            VTSS_BF_SET(VLAN_bulk.s[zisid].dirty_vids, vid, TRUE);

            // And also tell that this switch is now dirty.
            VLAN_bulk.s[zisid].dirty = TRUE;
        }
    }

    // #user has a share in VLAN membership's final value.
    // Let's see if it changes anything to back out of this share.
    VLAN_IN_USE_ON_SWITCH_SET(isid, vid, user, FALSE);

    // If forbidden user, clear from forbidden table for this switch
    // (forbidden user is not part of any of the other VLAN user types).
    if (user == VLAN_USER_INT_FORBIDDEN) {
        VLAN_forbidden_table[vid].ports[zisid].clear_all();
    }

#if defined(VLAN_SINGLE_USER_SUPPORT)
    if ((user_idx = VLAN_user_to_single_idx(user)) != VLAN_SINGLE_CNT) {
        // A "single" user is a user that can configure different ports
        // to be members of different VLANs, but at most one VLAN per port.
        // If he backs out of a certain VLAN on a given switch, we need to
        // clear our status for all ports that are members of this VID on
        // the switch in question.
        mesa_port_no_t iport;
        for (iport = 0; iport < VLAN_single_table[user_idx].vid[zisid].size(); iport++) {
            if (VLAN_single_table[user_idx].vid[zisid][iport] == vid) {
                VLAN_single_table[user_idx].vid[zisid][iport] = VTSS_VID_NULL;
            }
        }
    }
#endif

    in_use = FALSE;
    for (isid_iter = VTSS_ISID_START; isid_iter < VTSS_ISID_END; isid_iter++) {
        if (VLAN_IN_USE_ON_SWITCH_GET(isid_iter, vid, user)) {
            // Still in use on another switch.
            in_use = TRUE;
            break;
        }
    }

    if (!in_use) {
        // The VID is no longer in use. Do various updates.
        if (VLAN_user_to_multi_idx(user) != VLAN_MULTI_CNT) {
            // If #user is a multi-user, it could be that we should move the entry to the free list.

            if (VLAN_multi_table[vid] == NULL) {
                // Must not be possible to have a NULL entry in this table
                // by now (since VLAN_IN_USE_ON_SWITCH_GET() above returned TRUE).
                T_E("Internal error");
                return;
            }

            in_use = FALSE;
            for (user_idx = 0; user_idx < VLAN_MULTI_CNT; user_idx++) {
                user_iter = VLAN_multi_idx_to_user(user_idx);
                if (user_iter == user) {
                    // We already know that #user is no longer in use. Only check other multi-users
                    continue;
                }

                for (isid_iter = VTSS_ISID_START; isid_iter < VTSS_ISID_END; isid_iter++) {
                    if (VLAN_IN_USE_ON_SWITCH_GET(isid_iter, vid, user_iter)) {
                        // At least one other multi-user is currently using this entry. Can't free it.
                        in_use = TRUE;
                        break;
                    }
                }

                if (in_use) {
                    // No need to iterate any further
                    break;
                }
            }

            if (!in_use) {
                // Free it.
                VLAN_multi_table[vid]->next = VLAN_free_list;
                VLAN_free_list = VLAN_multi_table[vid];
                VLAN_multi_table[vid] = NULL;
            }

#if defined(VLAN_SAME_USER_SUPPORT)
        } else if ((user_idx = VLAN_user_to_same_idx(user)) != VLAN_SAME_CNT) {
            // If #user is a same-user, we must reset the VID to VTSS_VID_NULL if this was the last switch he deleted the VLAN for.
            VLAN_same_table[user_idx].vid = VTSS_VID_NULL;
#endif /* defined(VLAN_SAME_USER_SUPPORT) */
        }
    }

    // Update the new final membership value.
    VLAN_combined_update(isid, vid);
}

/******************************************************************************/
// VLAN_msg_tx_port_conf_all()
// Transmit whole switch's port configuration.
/******************************************************************************/
static void VLAN_msg_tx_port_conf_all(vtss_isid_t isid)
{
    CapArray<vtss_appl_vlan_port_detailed_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> conf;
    port_iter_t pit;

    // In order not to iteratively take VLAN_CRIT_ENTER() for each and every port,
    // we take a copy of the port configuration
    VLAN_CRIT_ENTER();
    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        conf[pit.iport] = VLAN_port_detailed_conf[VLAN_USER_INT_ALL][isid - VTSS_ISID_START][pit.iport];
    }
    VLAN_CRIT_EXIT();

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        // Call-back those modules interested in configuration changes on the primary switch.
        VLAN_port_conf_change_callback(isid, pit.iport, &conf[pit.iport]);

        // Apply configuration
        VLAN_port_conf_api_set(pit.iport, &conf[pit.iport]);
    }
}

/******************************************************************************/
// VLAN_msg_tx_port_conf_single()
// Transmit single port's configuration.
/******************************************************************************/
static void VLAN_msg_tx_port_conf_single(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_vlan_port_detailed_conf_t *conf)
{
    // Only send to existing switches
    if (!msg_switch_exists(isid)) {
        return;
    }

    // Only send to existing ports
    if (port_no >= port_count_max()) {
        return;
    }

    // Apply configuration
    VLAN_port_conf_api_set(port_no, conf);

    // Call-back those modules interested in configuration changes on the primary switch.
    VLAN_port_conf_change_callback(isid, port_no, conf);
}

/******************************************************************************/
// VLAN_msg_tx_tpid_conf()
// Transmit TPID configuraiton to all existing switches in the stack.
/******************************************************************************/
static void VLAN_msg_tx_tpid_conf(mesa_etype_t tpid)
{
    mesa_vlan_conf_t conf;

    conf.s_etype = tpid;
    T_D("EType = 0x%x", conf.s_etype);
    if (mesa_vlan_conf_set(NULL, &conf) != VTSS_RC_OK) {
        T_E("Setting TPID Configuration failed");
    }
}

/******************************************************************************/
// VLAN_svl_conf_change_callback()
/******************************************************************************/
static void VLAN_svl_conf_change_callback(mesa_vid_t vid, mesa_vid_t fid)
{
    u32 i;

    for (i = 0; i < ARRSZ(VLAN_svl_cb); i++) {
        if (VLAN_svl_cb[i].cb == NULL) {
            // Since there is no un-register support,
            // we know that there are no more registrants
            // when we meet a NULL pointer.
            break;
        }

        T_IG(TRACE_GRP_CB, "Calling back %s with vid = %u => fid = %u", vtss_module_names[VLAN_svl_cb[i].modid], vid, fid);
        VLAN_svl_cb[i].cb(vid, fid);
    }
}

/******************************************************************************/
// VLAN_msg_tx_vid_to_fid()
/******************************************************************************/
static void VLAN_msg_tx_vid_to_fid(vtss_isid_t isid, mesa_vid_t vid, mesa_vid_t fid)
{
    mesa_rc              rc;
    mesa_vlan_vid_conf_t vid_conf;

    if ((rc = mesa_vlan_vid_conf_get(NULL, vid, &vid_conf)) != VTSS_RC_OK) {
        T_E("Getting VID conf for VID = %u failed. rc = %u", vid, rc);
    } else if (vid_conf.fid != fid) {
        T_D("Changing FID from %u to %u for VID = %u", vid_conf.fid, fid, vid);
        vid_conf.fid = fid;

        if ((rc = mesa_vlan_vid_conf_set(NULL, vid, &vid_conf)) != VTSS_RC_OK) {
            T_E("Setting VID conf for VID = %u to FID = %u failed. rc = %u", vid, fid, rc);
        }

        VLAN_svl_conf_change_callback(vid, fid);
    }
}

/******************************************************************************/
// VLAN_msg_tx_fid_table_reset()
/******************************************************************************/
static void VLAN_msg_tx_fid_table_reset(vtss_isid_t isid)
{
    mesa_vid_t vid;

    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
        VLAN_msg_tx_vid_to_fid(isid, vid, vid);
    }
}

/******************************************************************************/
// VLAN_msg_tx_membership_all()
// Transmit all VLAN memberships to switch pointed to by #isid,
// which is a legal ISID.
/******************************************************************************/
static void VLAN_msg_tx_membership_all(vtss_isid_t isid)
{
    vlan_ports_t dummy;
    mesa_vid_t   vid = VTSS_VID_NULL;
    vtss_isid_t  zisid = isid - VTSS_ISID_START;

    T_D("enter, isid: %d", isid);

    VLAN_CRIT_ENTER();

    // Start capturing changes.
    VLAN_bulk_begin();

    // The following causes VLAN_bulk_end() to send notifications to all subscribers
    // about all ports (when VLAN_bulk.flush is also set).
    (void)vlan_mgmt_bitmask_all_ones_set(VLAN_bulk.s[zisid].dirty_vids);

    // And then the global one for fast iterations in VLAN_bulk_end();
    VLAN_bulk.s[zisid].dirty = TRUE;

    // Since the new switch may have old VLANs installed, we must ask it to flush
    // prior to adding new memberships.
    VLAN_bulk.flush[zisid] = TRUE;

    while ((vid = VLAN_get(isid, vid, VLAN_USER_INT_ALL, TRUE, &dummy)) != VTSS_VID_NULL) {
        // This tells VLAN_bulk_end() to transmit new configuration to #isid.
        VTSS_BF_SET(VLAN_bulk.s[zisid].tx_conf, vid, TRUE);
    }

    // Use vid == VTSS_VID_NULL to indicate that there's something to transmit for this ISID.
    // Whether or not the switch in question has a VID to add, we need to
    // enforce VLAN_bulk_end() to transmit a message so that it gets old VLANs cleared.
    VTSS_BF_SET(VLAN_bulk.s[zisid].tx_conf, VTSS_VID_NULL, TRUE);

    // This is where the real work happens
    VLAN_bulk_end();
    VLAN_CRIT_EXIT();

    T_D("exit, isid: %d", isid);
}

/******************************************************************************/
// VLAN_isid_port_check()
/******************************************************************************/
static mesa_rc VLAN_isid_port_check(vtss_isid_t isid, mesa_port_no_t port_no, BOOL allow_local, BOOL check_port)
{
    BOOL configurable = VTSS_ISID_LEGAL(isid) && msg_switch_configurable(isid);

    if ((isid == VTSS_ISID_LOCAL && allow_local) || (isid != VTSS_ISID_LOCAL && configurable)) {
        // Easier to express in positive logic.
    } else {
        T_D("Invalid ISID (%u). Allow local = %d", isid, allow_local);
        return VLAN_ERROR_ISID;
    }

    if (isid != VTSS_ISID_LOCAL && !msg_switch_is_primary()) {
        // Possibly transient phenomenon
        T_D("Not primary switch");
        return VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if (check_port && port_no >= port_count_max()) {
        T_D("Invalid port %u", port_no);
        return VLAN_ERROR_PORT;
    }

    return VTSS_RC_OK;
}

#if defined(VLAN_VOLATILE_USER_PRESENT)
/******************************************************************************/
// VLAN_conflict_check()
/******************************************************************************/
static void VLAN_conflict_check(vtss_appl_vlan_user_t ext_user, vlan_port_conflicts_t *conflicts, vtss_appl_vlan_port_detailed_conf_t *resulting_conf, const vtss_appl_vlan_port_detailed_conf_t *second_conf, u8 flag)
{
    if (flag == VTSS_APPL_VLAN_PORT_FLAGS_PVID) {
        if (second_conf->flags & VTSS_APPL_VLAN_PORT_FLAGS_PVID) {
            // This VLAN user wants to control the port's pvid
            if (resulting_conf->flags & VTSS_APPL_VLAN_PORT_FLAGS_PVID) {
                // So did a previous user.
                if (resulting_conf->pvid != second_conf->pvid) {
                    // And the previous module wanted another value of the pvid
                    // flag than we do.  But since he came before us in the
                    // vlan_user_int_t enumeration, he wins.
                    conflicts->port_flags |= VTSS_APPL_VLAN_PORT_FLAGS_PVID;
                    conflicts->users[VLAN_PORT_FLAGS_IDX_PVID] |= (1 << (u8)ext_user);
                    T_D("VLAN PVID Conflict");
                } else {
                    // Luckily, the previous module wants to set pvid to the
                    // same value as us. No conflict.
                }
            } else {
                // VLAN pvid not previously overridden, but this user wants
                // to override it.
                resulting_conf->pvid = second_conf->pvid;
                resulting_conf->flags |= VTSS_APPL_VLAN_PORT_FLAGS_PVID; // Overridden
            }
        }
    } else if (flag == VTSS_APPL_VLAN_PORT_FLAGS_AWARE) {
        if (second_conf->flags & VTSS_APPL_VLAN_PORT_FLAGS_AWARE) {
            // This VLAN user wants to control the port's VLAN awareness
            if (resulting_conf->flags & VTSS_APPL_VLAN_PORT_FLAGS_AWARE) {
                // So did a previous user.
                if (resulting_conf->port_type != second_conf->port_type) {
                    // And the previous module wanted another value of the
                    // awareness flag than we do. But since he came before
                    // us in the vlan_user_int_t enumeration, he wins.
                    conflicts->port_flags |= VTSS_APPL_VLAN_PORT_FLAGS_AWARE;
                    conflicts->users[VLAN_PORT_FLAGS_IDX_AWARE] |= (1 << (u8)ext_user);
                    T_D("VLAN Awareness Conflict");
                } else {
                    // Luckily, the previous module wants to set awareness
                    // to the same value as us. No conflict.
                }
            } else {
                // VLAN awareness not previously overridden, but this user
                // wants to override it.
                resulting_conf->port_type = second_conf->port_type;
                resulting_conf->flags |= VTSS_APPL_VLAN_PORT_FLAGS_AWARE; // Overridden
            }
        }
    } else if (flag == VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT) {
        if (second_conf->flags & VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT) {
            // This VLAN user wants to control the port's VLAN ingress_filter
            if (resulting_conf->flags & VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT) {
                // So did a previous user.
                if (resulting_conf->ingress_filter != second_conf->ingress_filter) {
                    // And the previous module wanted another value of the
                    // ingress_filter flag than we do. But since he came
                    // before us in the vlan_user_int_t enumeration, he wins.
                    conflicts->port_flags |= VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT;
                    conflicts->users[VLAN_PORT_FLAGS_IDX_INGR_FILT] |= (1 << (u8)ext_user);
                    T_D("VLAN Ingress Filter Conflict");
                } else {
                    // Luckily, the previous module wants to set ingress_filter
                    // to the same value as us. No conflict.
                }
            } else {
                // VLAN ingress_filter not previously overridden, but this
                // user wants to override it.
                resulting_conf->ingress_filter = second_conf->ingress_filter;
                resulting_conf->flags |= VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT; // Overridden
            }
        }
    } else if (flag == VTSS_APPL_VLAN_PORT_FLAGS_RX_TAG_TYPE) {
        if (second_conf->flags & VTSS_APPL_VLAN_PORT_FLAGS_RX_TAG_TYPE) {
            // This VLAN user wants to control the port's ingress frame_type
            if (resulting_conf->flags & VTSS_APPL_VLAN_PORT_FLAGS_RX_TAG_TYPE) {
                // So did a previous user.
                if (resulting_conf->frame_type != second_conf->frame_type) {
                    // And the previous module wanted another value of the
                    // ingress frame_type flag than we do. But since he came
                    // before us in the vlan_user_int_t enumeration, he wins.
                    conflicts->port_flags |= VTSS_APPL_VLAN_PORT_FLAGS_RX_TAG_TYPE;
                    conflicts->users[VLAN_PORT_FLAGS_IDX_RX_TAG_TYPE] |= (1 << (u8)ext_user);
                    T_D("VLAN Frame_Type Conflict");
                } else {
                    // Luckily, the previous module wants to set ingress
                    // frame_type to the same value as us. No conflict.
                }
            } else {
                // VLAN ingress frame_type not previously overridden,
                // but this user wants to override it.
                resulting_conf->frame_type = second_conf->frame_type;
                resulting_conf->flags |= VTSS_APPL_VLAN_PORT_FLAGS_RX_TAG_TYPE; // Overridden
            }
        }
    } else {
        T_E("Invalid input flag");
    }
}
#endif /* defined(VLAN_VOLATILE_USER_PRESENT) */

#if defined(VLAN_VOLATILE_USER_PRESENT)
/******************************************************************************/
// VLAN_conflict_check_uvid_inner()
/******************************************************************************/
static BOOL VLAN_conflict_check_uvid_inner(vtss_appl_vlan_port_detailed_conf_t *resulting_conf, vtss_appl_vlan_port_detailed_conf_t *temp_conf, BOOL *at_least_one_module_wants_to_send_a_specific_vid_tagged)
{
    BOOL conflict = FALSE;

    if (temp_conf->flags & VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE) {
        // This VLAN user wants to control the Tx Tagging on this port
        if (resulting_conf->flags & VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE) {
            // So did a higher-prioritized user. Let's see if this gives
            // rise to conflicts.
            switch (resulting_conf->tx_tag_type) {
            case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS:
                switch (temp_conf->tx_tag_type) {
                case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS:
                    // A previous module wants to send untagged, and so do we. If the .uvid of
                    // the previous one is the same as ours, we're OK with it.
                    if (resulting_conf->untagged_vid != temp_conf->untagged_vid) {
                        // The previous module wants to untag a specific VLAN ID,
                        // which is not the same as the one we want to untag.
                        if (*at_least_one_module_wants_to_send_a_specific_vid_tagged) {
                            // At the same time, another user module wants to send a specific VID
                            // tagged. This is not possible, because we want to resolve the
                            // "two-different-untagged-VIDs" conflict by sending all frames untagged.
                            conflict = TRUE;
                        } else {
                            resulting_conf->tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL;
                        }
                    }
                    break;

                case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL:
                    if (*at_least_one_module_wants_to_send_a_specific_vid_tagged) {
                        // At the same time, another user module wants to send
                        // a specific VID tagged. This is not possible.
                        conflict = TRUE;
                    } else {
                        resulting_conf->tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL;
                    }
                    break;

                case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS:
                    // A previous module wants to send untagged, but we want to send tagged. This is possible if
                    // not two or more previous modules want to send untagged (indicated by .uvid = VTSS_VID_ALL),
                    // and if the .uvid we want to send tagged is different from the .uvid that a previous
                    // user wants to send untagged.
                    if (resulting_conf->untagged_vid == temp_conf->untagged_vid) {
                        conflict = TRUE;
                    } else {
                        // Do not overwrite resulting_conf->uvid, but keep track of the fact that at least one
                        // module wants to send a specific VID tagged.
                        *at_least_one_module_wants_to_send_a_specific_vid_tagged = TRUE;
                    }
                    break;

                case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL:
                    // A previous module wants to untag a specific VID, but we want to tag all.
                    // This is impossible.
                    conflict = TRUE;
                    break;

                default:
                    T_E("Actual config tx untag this, current config Tx Tag error");
                    return conflict;
                }
                break;

            case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL:
                switch (temp_conf->tx_tag_type) {
                case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS:
                case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL:
                    // A previous user wants to send all vids untagged and current user either wants to untag one vid or all vids.
                    break;
                case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS:
                case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL:
                    // A previous user wants to send all vids untagged. Hence current user cannot tag a vid. Impossible.
                    conflict = TRUE;
                    break;
                }
                break;

            case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS:
                switch (temp_conf->tx_tag_type) {
                case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS:
                    // A previous module wants to send a specific VID tagged, and we want to send a specific VID untagged.
                    // This is possible if the .uvids are not the same.
                    if (resulting_conf->untagged_vid != temp_conf->untagged_vid) {
                        // Here we have to change the tag-type to "untag", and set the uvid to the untagged uvid.
                        resulting_conf->tx_tag_type   = VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS;
                        resulting_conf->untagged_vid  = temp_conf->untagged_vid;
                        // And remember that at least one module wanted to send a specific VID tagged.
                        *at_least_one_module_wants_to_send_a_specific_vid_tagged = TRUE;
                    } else {
                        // Cannot send the same VID tagged and untagged at the same time.
                        conflict = TRUE;
                    }
                    break;

                case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL:
                    // Cannot send the untag_all and tag a vid at the same time.
                    conflict = TRUE;
                    break;

                case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS:
                    if (resulting_conf->untagged_vid != temp_conf->untagged_vid) {
                        // Tell the API to send all frames tagged.
                        resulting_conf->tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL;
                    } else {
                        // Both a previous module and we want to tag a specific VID. No problem.
                    }
                    break;

                case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL:
                    // A previous user wants to tag a specific VID, and we want to tag all.
                    // No problem, but change the tx_tag_type to this more restrictive.
                    resulting_conf->tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL;
                    break;

                default:
                    T_E("Actual config tx tag this, current config Tx Tag error");
                    return conflict;
                }
                break;

            case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL:
                switch (temp_conf->tx_tag_type) {
                case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS:
                case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL:
                    // A previous module wants all VIDs to be tagged, but this module wants a specific VID
                    // or all vids to be untagged. Impossible.
                    conflict = TRUE;
                    break;

                case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS:
                    // A previous module wants all VIDs to be tagged, and we want to tag a specific. No problem.
                    break;

                case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL:
                    // Both a previous module and we want to tag all frames. No problem.
                    break;

                default:
                    T_E("Actual config tx tag all, current config Tx Tag error");
                    return conflict;
                }
                break;

            default:
                T_E("actual config tx tag error");
                return conflict;
            }
        } else {
            // We're the first user module with Tx tagging requirements
            resulting_conf->tx_tag_type    = temp_conf->tx_tag_type;
            resulting_conf->untagged_vid   = temp_conf->untagged_vid;
            resulting_conf->flags         |= VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE;
            if (temp_conf->tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS) {
                *at_least_one_module_wants_to_send_a_specific_vid_tagged = TRUE;
            }
        }
    }

    return conflict;
}
#endif /* defined(VLAN_VOLATILE_USER_PRESENT) */

#if defined(VLAN_VOLATILE_USER_PRESENT)
/******************************************************************************/
// VLAN_conflict_check_uvid()
/******************************************************************************/
static void VLAN_conflict_check_uvid(vtss_isid_t isid, mesa_port_no_t port, vlan_port_conflicts_t *conflicts, vtss_appl_vlan_port_detailed_conf_t *resulting_conf)
{
    BOOL                                at_least_one_module_wants_to_send_a_specific_vid_tagged = FALSE; // This one is used to figure out whether we're allowed to send all frames untagged or not.
    vlan_user_int_t                     user;
    vtss_appl_vlan_port_detailed_conf_t temp_conf;

    // Check for .tx_tag_type. In the loop below, resulting_conf->pvid must
    // be resolved already . That's the reason for iterating over the VLAN users
    // in two tempi.
    for (user = (vlan_user_int_t)(VLAN_USER_INT_STATIC + 1); user <= VLAN_USER_INT_ALL; user++) {

        // The static user has least priority, but should still have a chance to
        // add a word to the final Tx tagging, even if another module has overridden
        // it. Suppose the static user has configured the port to tag all and
        // a volatile user configures a port to tag a particular VID. In this case,
        // the final tx tagging should be tag all with no conflicts.
        // Therefore we iterate over the static user as well here.
        if (user == VLAN_USER_INT_ALL) {
            temp_conf = VLAN_port_detailed_conf[VLAN_USER_INT_STATIC][isid - VTSS_ISID_START][port];
        } else {
            temp_conf = VLAN_port_detailed_conf[user][isid - VTSS_ISID_START][port];
        }

        if (VLAN_conflict_check_uvid_inner(resulting_conf, &temp_conf, &at_least_one_module_wants_to_send_a_specific_vid_tagged) && user != VLAN_USER_INT_ALL) {
            // If a conflict occurs, only report it when it's not due to the static user (since he is always overridable).
            conflicts->port_flags |= VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE;
            conflicts->users[VLAN_PORT_FLAGS_IDX_TX_TAG_TYPE] |= (1 << (u8)VLAN_user_int_to_ext(user));
        }
    }
}
#endif /* defined(VLAN_VOLATILE_USER_PRESENT) */

/******************************************************************************/
// VLAN_port_conflict_resolver()
/******************************************************************************/
static BOOL VLAN_port_conflict_resolver(vtss_isid_t isid, mesa_port_no_t port, vlan_port_conflicts_t *conflicts, vtss_appl_vlan_port_detailed_conf_t *resulting_conf)
{
    BOOL            changed;
#if defined(VLAN_VOLATILE_USER_PRESENT)
    vlan_user_int_t user;
#endif /* defined(VLAN_VOLATILE_PRESENT) */

    // Critical section should have been taken by now.
    VLAN_CRIT_ASSERT_LOCKED();

    memset(conflicts, 0, sizeof(*conflicts));

    // Start out with the static user's configuration
    *resulting_conf = VLAN_port_detailed_conf[VLAN_USER_INT_STATIC][isid - VTSS_ISID_START][port];

    // All the static configuration can be overridden
    resulting_conf->flags = 0;

#if defined(VLAN_VOLATILE_USER_PRESENT)
    // This code is assuming that VLAN_USER_INT_STATIC is the very first in the vlan_user_int_t enum.
    for (user = (vlan_user_int_t)(VLAN_USER_INT_STATIC + 1); user < VLAN_USER_INT_ALL; user++) {
        vtss_appl_vlan_port_detailed_conf_t *temp_conf = &VLAN_port_detailed_conf[user][isid - VTSS_ISID_START][port];
        vtss_appl_vlan_user_t ext_user = VLAN_user_int_to_ext(user);

        // Check for aware conflicts.
        VLAN_conflict_check(ext_user, conflicts, resulting_conf, temp_conf, VTSS_APPL_VLAN_PORT_FLAGS_AWARE);

        // Check for ingress_filter conflicts.
        VLAN_conflict_check(ext_user, conflicts, resulting_conf, temp_conf, VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT);

        // Check for pvid conflicts.
        VLAN_conflict_check(ext_user, conflicts, resulting_conf, temp_conf, VTSS_APPL_VLAN_PORT_FLAGS_PVID);

        // Check for acceptable frame type conflicts.
        VLAN_conflict_check(ext_user, conflicts, resulting_conf, temp_conf, VTSS_APPL_VLAN_PORT_FLAGS_RX_TAG_TYPE);
    }

    // Finally check untagged_vid and tx_tag_type (VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE)
    VLAN_conflict_check_uvid(isid, port, conflicts, resulting_conf);
#endif /* defined(VLAN_VOLATILE_USER_PRESENT) */

    // The final port configuration is now held in resulting_conf.
    // It should always have all the port flags set.
    resulting_conf->flags = VTSS_APPL_VLAN_PORT_FLAGS_ALL;

    changed = memcmp(resulting_conf, &VLAN_port_detailed_conf[VLAN_USER_INT_ALL][isid - VTSS_ISID_START][port], sizeof(*resulting_conf)) ? TRUE : FALSE;

    // Save it back into the port_conf array as VLAN_USER_INT_ALL.
    VLAN_port_detailed_conf[VLAN_USER_INT_ALL][isid - VTSS_ISID_START][port] = *resulting_conf;

    return changed;
}

#if defined(VLAN_VOLATILE_USER_PRESENT) && defined(VTSS_SW_OPTION_SYSLOG)
/******************************************************************************/
// VLAN_conflict_log()
// This function compares the previous and present conflicts and logs possible
// conflicts to RAM-based syslog.
/******************************************************************************/
static void VLAN_conflict_log(vlan_port_conflicts_t *prev_conflicts, vlan_port_conflicts_t *current_conflicts)
{
    vlan_user_int_t user;

    for (user = (vlan_user_int_t)(VLAN_USER_INT_STATIC + 1); user < VLAN_USER_INT_ALL; user++) {
        vtss_appl_vlan_user_t ext_user = VLAN_user_int_to_ext(user);

        // This condition checks if there are any PVID conflicts.
        if (current_conflicts->port_flags & VTSS_APPL_VLAN_PORT_FLAGS_PVID) {
            if ((current_conflicts->users[VLAN_PORT_FLAGS_IDX_PVID] & (1 << (u8)ext_user))  && !(prev_conflicts->users[VLAN_PORT_FLAGS_IDX_PVID] & (1 << (u8)ext_user))) {
                S_W("VLAN-CONF-CONFLICT: VLAN Port Configuration PVID Conflict - %s", vlan_mgmt_user_to_txt(ext_user));
            }
        }

        // This condition checks if there are any Aware conflicts.
        if (current_conflicts->port_flags & VTSS_APPL_VLAN_PORT_FLAGS_AWARE) {
            if ((current_conflicts->users[VLAN_PORT_FLAGS_IDX_AWARE] & (1 << (u8)ext_user))  && !(prev_conflicts->users[VLAN_PORT_FLAGS_IDX_AWARE] & (1 << (u8)ext_user))) {
                S_W("VLAN-CONF-CONFLICT: VLAN Port Configuration Awareness Conflict - %s", vlan_mgmt_user_to_txt(ext_user));
            }
        }

        // This condition checks if there are any Ingr Filter conflicts.
        if (current_conflicts->port_flags & VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT) {
            if ((current_conflicts->users[VLAN_PORT_FLAGS_IDX_INGR_FILT] & (1 << (u8)ext_user))  && !(prev_conflicts->users[VLAN_PORT_FLAGS_IDX_INGR_FILT] & (1 << (u8)ext_user))) {
                S_W("VLAN-CONF-CONFLICT: VLAN Port Configuration Ingress Filter Conflict - %s", vlan_mgmt_user_to_txt(ext_user));
            }
        }

        // This condition checks if there are any Frame Type conflicts.
        if (current_conflicts->port_flags & VTSS_APPL_VLAN_PORT_FLAGS_RX_TAG_TYPE) {
            if ((current_conflicts->users[VLAN_PORT_FLAGS_IDX_RX_TAG_TYPE] & (1 << (u8)ext_user))  && !(prev_conflicts->users[VLAN_PORT_FLAGS_IDX_RX_TAG_TYPE] & (1 << (u8)ext_user))) {
                S_W("VLAN-CONF-CONFLICT: VLAN Port Configuration Frame Type Conflict - %s", vlan_mgmt_user_to_txt(ext_user));
            }
        }

        // This condition checks if there are any UVID conflicts.
        if (current_conflicts->port_flags & VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE) {
            if ((current_conflicts->users[VLAN_PORT_FLAGS_IDX_TX_TAG_TYPE] & (1 << (u8)ext_user))  && !(prev_conflicts->users[VLAN_PORT_FLAGS_IDX_TX_TAG_TYPE] & (1 << (u8)ext_user))) {
                S_W("VLAN-CONF-CONFLICT: VLAN Port Configuration UVID Conflict - %s", vlan_mgmt_user_to_txt(ext_user));
            }
        }
    }
}
#endif /* defined(VLAN_VOLATILE_USER_PRESENT) && defined(VTSS_SW_OPTION_SYSLOG) */

/******************************************************************************/
// VLAN_flooding_set()
/******************************************************************************/
static mesa_rc VLAN_flooding_set(mesa_vid_t vid, mesa_bool_t flooding)
{
    mesa_vlan_vid_conf_t vid_conf;
    mesa_rc rc;

    if ((rc = mesa_vlan_vid_conf_get(NULL, vid, &vid_conf)) != MESA_RC_OK) {
        return rc;
    }

    vid_conf.flooding = flooding;

    if ((rc = mesa_vlan_vid_conf_set(NULL, vid, &vid_conf)) != MESA_RC_OK) {
        return rc;
    }

    VTSS_BF_SET(VLAN_flooding_vlans, vid, flooding);

    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_flooding_default()
/******************************************************************************/
static void VLAN_flooding_default(void)
{
    mesa_rc rc;

    if (!fast_cap(VTSS_APPL_CAP_VLAN_FLOODING)) {
        return;
    }

    VLAN_CRIT_ENTER();
    for (int vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++ ) {
        if ((rc = VLAN_flooding_set(vid, TRUE)) != VTSS_RC_OK) {
            T_E("Failed setting vlan flooding to default. vid %u rc %u", vid, rc);
        }
    }

    VLAN_CRIT_EXIT();
}

#if defined(VTSS_SW_OPTION_VLAN_NAMING)
/******************************************************************************/
// VLAN_name_default()
/******************************************************************************/
static void VLAN_name_default(void)
{
    VLAN_CRIT_ENTER();
    memset(VLAN_name_conf, 0, sizeof(VLAN_name_conf));
    VLAN_CRIT_EXIT();
}
#endif /* defined(VTSS_SW_OPTION_VLAN_NAMING) */

/******************************************************************************/
// VLAN_port_to_detailed_conf()
/******************************************************************************/
static mesa_rc VLAN_port_to_detailed_conf(vtss_appl_vlan_port_conf_t *conf, vtss_appl_vlan_port_detailed_conf_t *port_detailed_conf)
{
    switch (conf->mode) {
    case VTSS_APPL_VLAN_PORT_MODE_ACCESS:
        // UNTAG_ALL is better than UNTAG_THIS, because then it has correct properties if used as an rmirror reflector port.
        // It will then be up to the end-user to configure the reflector port as hybrid and change the egress tagging to
        // tag all if he wants all frames to get tagged with their classified VID.
        port_detailed_conf->tx_tag_type    = VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL;
        port_detailed_conf->frame_type     = MESA_VLAN_FRAME_ALL;
        port_detailed_conf->ingress_filter = TRUE;
        port_detailed_conf->port_type      = VTSS_APPL_VLAN_PORT_TYPE_C;
        port_detailed_conf->pvid           = conf->access_pvid;
        port_detailed_conf->untagged_vid   = conf->access_pvid;
        break;

    case VTSS_APPL_VLAN_PORT_MODE_TRUNK:
        port_detailed_conf->pvid            = conf->trunk_pvid;
        port_detailed_conf->untagged_vid    = port_detailed_conf->pvid;
        port_detailed_conf->port_type       = VTSS_APPL_VLAN_PORT_TYPE_C;
        port_detailed_conf->ingress_filter  = TRUE;
        if (conf->trunk_tag_pvid) {
            port_detailed_conf->tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL;
            port_detailed_conf->frame_type  = MESA_VLAN_FRAME_TAGGED;
        } else {
            port_detailed_conf->tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS;
            port_detailed_conf->frame_type  = MESA_VLAN_FRAME_ALL;
        }
        break;

    case VTSS_APPL_VLAN_PORT_MODE_HYBRID:
        *port_detailed_conf                 = conf->hybrid;
        break;

    default:
        T_E("Que? %d)", conf->mode);
        return VTSS_RC_ERROR;
    }

    port_detailed_conf->flags = VTSS_APPL_VLAN_PORT_FLAGS_ALL;

    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_add_del_core()
// #isid: Legal ISID
// #vid:  Legal VID
// #user: [VLAN_USER_INT_STATIC; VLAN_USER_INT_ALL[
/******************************************************************************/
static mesa_rc VLAN_add_del_core(vtss_isid_t isid, mesa_vid_t vid, vlan_user_int_t user, const vlan_ports_t *const ports, vlan_bit_operation_t operation)
{
    mesa_rc      rc;
    vlan_ports_t resulting_ports; // Don't overwrite caller's ports
    BOOL         delete_rather_than_add = TRUE;
    BOOL         force_delete = ports == NULL;
    u32          iport, port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    VLAN_CRIT_ASSERT_LOCKED();

    resulting_ports.ports[0] = 0; // Satisfy Lint

    if (!force_delete) {
        switch (operation) {
        case VLAN_BIT_OPERATION_OVERWRITE:
            // What's in #ports is what must be written to state.
            resulting_ports = *ports;
            break;

        case VLAN_BIT_OPERATION_ADD:
            // What's in #ports must be added to the current setting.
            (void)VLAN_get(isid, vid, user, FALSE, &resulting_ports);
            for (iport = 0; iport < port_cnt; iport++) {
                if (ports->ports[iport]) {
                    resulting_ports.ports[iport] = TRUE;
                }
            }

            delete_rather_than_add = FALSE;
            break;

        case VLAN_BIT_OPERATION_DEL:
            // What's in #ports must be removed from the current settings.
            (void)VLAN_get(isid, vid, user, FALSE, &resulting_ports);
            for (iport = 0; iport < port_cnt; iport++) {
                if (ports->ports[iport]) {
                    resulting_ports.ports[iport] = FALSE;
                }
            }
            break;

        default:
            T_E("Huh?");
            return VTSS_RC_ERROR;
        }
    }

    if (!force_delete && delete_rather_than_add) {
        // It might now be that the VLAN is empty. If the VLAN is not enabled
        // by the end-user, it is because it has been auto-created by the fact
        // that one or more ports are in trunk or hybrid mode and therefore
        // members of all allowed VLANs.
        if (user == VLAN_USER_INT_STATIC && VTSS_BF_GET(VLAN_end_user_enabled_vlans, vid)) {
            // Nope. This is an end-user-enabled VLAN. Keep adding.
            delete_rather_than_add = FALSE;
        } else {
            // It's not an end-user-enabled VLAN. Check to see if there
            // are any member ports.
            for (iport = 0; iport < port_cnt; iport++) {
                if (resulting_ports.ports[iport]) {
                    // There is at least one member port. Keep adding.
                    delete_rather_than_add = FALSE;
                    break;
                }
            }
        }
    }

    if (force_delete || delete_rather_than_add) {
        VLAN_del(isid, vid, user);
    } else {
        if ((rc = VLAN_add(isid, vid, user, &resulting_ports)) != VTSS_RC_OK) {
            T_E("Huh? %s", error_txt(rc));
            // Shouldn't be possible
            return rc;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_membership_update()
/******************************************************************************/
static mesa_rc VLAN_membership_update(vtss_isid_t isid, vlan_ports_t *ports, mesa_vid_t old_vid, mesa_vid_t new_vid, u8 old_allowed_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES], u8 new_allowed_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES])
{
    mesa_vid_t           vid, check_vid;
    u8                   *allowed_vids;
    vlan_bit_operation_t oper;

    VLAN_CRIT_ASSERT_LOCKED();

    T_I("old_vid = %u, new_vid = %u, old_allowed_vids = %p, new_allowed_vids = %p", old_vid, new_vid, old_allowed_vids, new_allowed_vids);

    if ((old_allowed_vids == NULL && new_allowed_vids != NULL) ||
        (old_allowed_vids != NULL && new_allowed_vids == NULL)) {
        // Either going from a single-VID port to a multi-VID port or vice versa.
        BOOL going_to_multi_vid = new_allowed_vids != NULL;
        BOOL check_vid_enabled;

        allowed_vids = old_allowed_vids ? old_allowed_vids : new_allowed_vids;
        check_vid    = old_vid          ? old_vid          : new_vid;
        check_vid_enabled = VTSS_BF_GET(VLAN_end_user_enabled_vlans, check_vid);

        if (allowed_vids == NULL) {
            // Satisfy Lint
            T_E("Huh?");
            return VTSS_RC_ERROR;
        }

        // Gotta traverse all VIDs and:
        // 1) if going to a multi-VID port, remove old single-VID and add all new multi-VIDs.
        // 2) if going to a single-VID port, remove old multi-VIDs and possibly add single-VID.
        for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
            BOOL multi_vid_enabled = VTSS_BF_GET(allowed_vids, vid);
            if (vid == check_vid) {
                if (multi_vid_enabled != check_vid_enabled) {
                    oper = (going_to_multi_vid && multi_vid_enabled) || (!going_to_multi_vid && check_vid_enabled) ? VLAN_BIT_OPERATION_ADD : VLAN_BIT_OPERATION_DEL;
                } else {
                    continue;
                }
            } else if (multi_vid_enabled) {
                oper = going_to_multi_vid ? VLAN_BIT_OPERATION_ADD : VLAN_BIT_OPERATION_DEL;
            } else {
                continue;
            }

            VTSS_RC(VLAN_add_del_core(isid, vid, VLAN_USER_INT_STATIC, ports, oper));
        }
    } else if (old_allowed_vids == NULL && new_allowed_vids == NULL) {
        // Going from one single-VID to another single-VID mode.
        if (old_vid != new_vid) {
            if (VTSS_BF_GET(VLAN_end_user_enabled_vlans, old_vid)) {
                // Remove membership from old VID.
                VTSS_RC(VLAN_add_del_core(isid, old_vid, VLAN_USER_INT_STATIC, ports, VLAN_BIT_OPERATION_DEL));
            }
            if (VTSS_BF_GET(VLAN_end_user_enabled_vlans, new_vid)) {
                // Add membership of new VID.
                VTSS_RC(VLAN_add_del_core(isid, new_vid, VLAN_USER_INT_STATIC, ports, VLAN_BIT_OPERATION_ADD));
            }
        }
    } else {
        // Last case is when we go from one multi-VID to another multi-VID.
        if (old_allowed_vids == NULL || new_allowed_vids == NULL) {
            // Satisfy Lint
            T_E("Huh?");
            return VTSS_RC_ERROR;
        }

        for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
            BOOL was_member, must_be_member;

            was_member     = VTSS_BF_GET(old_allowed_vids, vid);
            must_be_member = VTSS_BF_GET(new_allowed_vids, vid);

            if (was_member != must_be_member) {
                VTSS_RC(VLAN_add_del_core(isid, vid, VLAN_USER_INT_STATIC, ports, was_member ? VLAN_BIT_OPERATION_DEL : VLAN_BIT_OPERATION_ADD));
            }
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_port_detailed_conf_get()
// This function doesn't check isid:port_no on purpose.
/******************************************************************************/
static mesa_rc VLAN_port_detailed_conf_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_vlan_port_detailed_conf_t *conf, vlan_user_int_t user)
{
    vtss_appl_vlan_user_t ext_user = VLAN_user_int_to_ext(user);

    if (conf == NULL) {
        return VLAN_ERROR_PARM;
    }

    if (user > VLAN_USER_INT_ALL) {
        return VLAN_ERROR_USER;
    }

    if (user != VLAN_USER_INT_ALL && !vlan_mgmt_user_is_port_conf_changer(ext_user)) {
        T_I("Unexpected user calling this function: %s. Will get allowed, but code should be updated", vlan_mgmt_user_to_txt(ext_user));
        // Continue execution
    }

    // Secondary switches do not store VLAN port configuration for each VLAN User.
    // Hence, combined information can only be fetched on the secondary switches from the VTSS API.
    if (isid == VTSS_ISID_LOCAL) {
        if (user != VLAN_USER_INT_ALL) {
            T_E("VLAN User should always be VLAN_USER_INT_ALL when isid is VTSS_ISID_LOCAL");
            return VLAN_ERROR_USER;
        }

        // Get port configuration directly from API.
        if (VLAN_port_conf_api_get(port_no, conf) != VTSS_RC_OK) {
            return VTSS_RC_ERROR;
        }
        return VTSS_RC_OK;
    }

    VLAN_CRIT_ENTER();
    *conf = VLAN_port_detailed_conf[user][isid - VTSS_ISID_START][port_no];
    VLAN_CRIT_EXIT();

    return VTSS_RC_OK;
}

/******************************************************************************/
// vlan_port_conf_check()
/******************************************************************************/
static mesa_rc VLAN_port_conf_check(const vtss_appl_vlan_port_detailed_conf_t *conf, vlan_user_int_t user)
{
    if (user == VLAN_USER_INT_STATIC && (conf->flags & VTSS_APPL_VLAN_PORT_FLAGS_ALL) != VTSS_APPL_VLAN_PORT_FLAGS_ALL) {
        // The static VLAN user must set all fields in one chunk, because
        // the whole #conf structure is copied to running config in one go.
        // Other VLAN users may have all flags cleared, indicating that they
        // no longer wish to override a particular port feature.
        return VLAN_ERROR_FLAGS;
    }

    if ((conf->flags & VTSS_APPL_VLAN_PORT_FLAGS_PVID) && (conf->pvid < VTSS_APPL_VLAN_ID_MIN || conf->pvid > VTSS_APPL_VLAN_ID_MAX)) {
        return VLAN_ERROR_PVID;
    }

    if (conf->flags & VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT) {
        // Nothing to do
    }

    if ((conf->flags & VTSS_APPL_VLAN_PORT_FLAGS_RX_TAG_TYPE) && conf->frame_type > MESA_VLAN_FRAME_UNTAGGED) {
        return VLAN_ERROR_FRAME_TYPE;
    }

    if (conf->flags & VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE) {
        if (conf->tx_tag_type > VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL) {
            return VLAN_ERROR_TX_TAG_TYPE;
        }

        // The static user cannot configure egress tagging as "tag this"
        if (user == VLAN_USER_INT_STATIC && conf->tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS) {
            return VLAN_ERROR_TX_TAG_TYPE;
        }

        if ((conf->tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS || conf->tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS) && (conf->untagged_vid < VTSS_APPL_VLAN_ID_MIN || conf->untagged_vid > VTSS_APPL_VLAN_ID_MAX)) {
            return VLAN_ERROR_UVID;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_port_detailed_conf_set()
// This function doesn't check isid:port_no on purpose.
/******************************************************************************/
static mesa_rc VLAN_port_detailed_conf_set(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_vlan_port_detailed_conf_t *new_conf, vlan_user_int_t user)
{
    vtss_appl_vlan_port_detailed_conf_t *old_conf;
    vtss_appl_vlan_port_detailed_conf_t resulting_conf;
    vlan_port_conflicts_t               new_conflicts;
    BOOL                                changed;
    vtss_appl_vlan_user_t               ext_user = VLAN_user_int_to_ext(user);
#if defined(VLAN_VOLATILE_USER_PRESENT) && defined(VTSS_SW_OPTION_SYSLOG)
    vlan_port_conflicts_t               old_conflicts;
#endif /* defined(VLAN_VOLATILE_USER_PRESENT) */

    T_D("Enter. %d:%d:%d", isid, port_no, user);

    if (user >= VLAN_USER_INT_ALL) {
        return VLAN_ERROR_USER;
    }

    if (!vlan_mgmt_user_is_port_conf_changer(ext_user)) {
        T_W("Unexpected user calling this function: %s. Will get allowed, but code should be updated", vlan_mgmt_user_to_txt(ext_user));
        // Continue execution
    }

    if (new_conf == NULL) {
        return VLAN_ERROR_PARM;
    }

    // Check contents of new_conf
    VTSS_RC(VLAN_port_conf_check(new_conf, user));

    VLAN_CRIT_ENTER();

    // Get the current VLAN port configuration.
    old_conf = &VLAN_port_detailed_conf[user][isid - VTSS_ISID_START][port_no];

    if (memcmp(new_conf, old_conf, sizeof(*new_conf)) == 0) {
        VLAN_CRIT_EXIT();
        return VTSS_RC_OK;
    }

#if defined(VLAN_VOLATILE_USER_PRESENT) && defined(VTSS_SW_OPTION_SYSLOG)
    // Get old port conflicts for syslog purposes
    (void)VLAN_port_conflict_resolver(isid, port_no, &old_conflicts, &resulting_conf);
#endif /* defined(VLAN_VOLATILE_USER_PRESENT) */

    // Time to update our internal state
    *old_conf = *new_conf;

    // Get new port conflicts
    changed = VLAN_port_conflict_resolver(isid, port_no, &new_conflicts, &resulting_conf);

    VLAN_CRIT_EXIT();

    if (changed) {
        // Pass on this port configuration on the stack.
        VLAN_msg_tx_port_conf_single(isid, port_no, &resulting_conf);
    }

#if defined(VLAN_VOLATILE_USER_PRESENT) && defined(VTSS_SW_OPTION_SYSLOG)
    // This function will dump the current conflicts to the RAM based syslog.
    VLAN_conflict_log(&old_conflicts, &new_conflicts);
#endif /* defined(VLAN_VOLATILE_USER_PRESENT) */

    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_port_conf_get()
// This function doesn't check for isid:port_no on purpose.
/******************************************************************************/
static mesa_rc VLAN_port_conf_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_vlan_port_conf_t *conf)
{
    VLAN_CRIT_ENTER();
    *conf = VLAN_port_conf[isid - VTSS_ISID_START][port_no];
    VLAN_CRIT_EXIT();

    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_port_conf_set()
// This function doesn't check for isid:port_no on purpose.
// Used by static user.
/******************************************************************************/
static mesa_rc VLAN_port_conf_set(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_vlan_port_conf_t *new_conf)
{
    vtss_appl_vlan_port_conf_t          *old_conf;
    vlan_ports_t                        ports;
    vtss_isid_t                         zisid = isid - VTSS_ISID_START;
    mesa_rc                             rc    = VTSS_RC_OK;
    vtss_appl_vlan_port_detailed_conf_t port_detailed_conf;
    mesa_vid_t                          vid;

#define EXIT_RC(expr)   { rc = (expr); if (rc != VTSS_RC_OK) {goto do_exit;}}

    if (new_conf == NULL) {
        return VLAN_ERROR_PARM;
    }

    if (new_conf->mode != VTSS_APPL_VLAN_PORT_MODE_ACCESS &&
        new_conf->mode != VTSS_APPL_VLAN_PORT_MODE_TRUNK  &&
        new_conf->mode != VTSS_APPL_VLAN_PORT_MODE_HYBRID) {
        return VLAN_ERROR_PORT_MODE;
    }

    if (new_conf->access_pvid < VTSS_APPL_VLAN_ID_MIN || new_conf->access_pvid > VTSS_APPL_VLAN_ID_MAX || new_conf->trunk_pvid < VTSS_APPL_VLAN_ID_MIN || new_conf->trunk_pvid > VTSS_APPL_VLAN_ID_MAX) {
        return VLAN_ERROR_VID;
    }

    // Check the leading bits in allowed VLANs for zeroness.
    for (vid = VTSS_VID_NULL; vid < VTSS_APPL_VLAN_ID_MIN; vid++) {
        if (VTSS_BF_GET(new_conf->hybrid_allowed_vids, vid) != 0) {
            return VLAN_ERROR_ALLOWED_HYBRID_VID;
        }

        if (VTSS_BF_GET(new_conf->trunk_allowed_vids, vid) != 0) {
            return VLAN_ERROR_ALLOWED_TRUNK_VID;
        }
    }

    // Check the trailing bits in allowed VLANs for zeroness.
    for (vid = VTSS_APPL_VLAN_ID_MAX + 1; vid < VTSS_APPL_VLAN_BITMASK_LEN_BYTES; vid++) {
        if (VTSS_BF_GET(new_conf->hybrid_allowed_vids, vid) != 0) {
            return VLAN_ERROR_ALLOWED_HYBRID_VID;
        }

        if (VTSS_BF_GET(new_conf->trunk_allowed_vids, vid) != 0) {
            return VLAN_ERROR_ALLOWED_TRUNK_VID;
        }
    }

    // Better set these flags on behalf of caller (static user), since he shouldn't bother.
    new_conf->hybrid.flags = VTSS_APPL_VLAN_PORT_FLAGS_ALL;

    // The static user cannot change the untagged VID.
    // It must always be the same as the PVID.
    new_conf->hybrid.untagged_vid = new_conf->hybrid.pvid;

    // Check contents of hybrid port configuration
    VTSS_RC(VLAN_port_conf_check(&new_conf->hybrid, VLAN_USER_INT_STATIC));

    VLAN_CRIT_ENTER();

    old_conf = &VLAN_port_conf[zisid][port_no];

    if (memcmp(old_conf, new_conf, sizeof(*old_conf)) == 0) {
        VLAN_CRIT_EXIT();
        return VTSS_RC_OK; // Do *NOT* goto do_exit, since that part of the code also applies the configuration.
    }

    // Start capturing changes
    VLAN_bulk_begin();

    // Configuration that is not directly related to a specific VID
    // Snoop into the normal underlying port configuration.
    // Doing it this early avoids a Lint warning that port_detailed_conf may not be initialized.
    port_detailed_conf       = VLAN_port_detailed_conf[VLAN_USER_INT_STATIC][zisid][port_no];
    port_detailed_conf.flags = VTSS_APPL_VLAN_PORT_FLAGS_ALL;

    // Prepare the port array with ourselves as the only bit set.
    vtss_clear(ports);
    ports.ports[port_no] = TRUE;

    // Gotta investigate changes one by one, because not only may the
    // port configuration change, but also the VLAN memberships.
    if (old_conf->mode != new_conf->mode) {
        if (old_conf->mode == VTSS_APPL_VLAN_PORT_MODE_ACCESS) {
            // Going from a single-VID mode to a multi-VID mode.
            // Gotta remove membership from the single-VID mode and add membership of all defined VLANs that are allowed in the multi-VID mode in question.
            EXIT_RC(VLAN_membership_update(isid, &ports,
                                           old_conf->access_pvid,                                                                                             // Old VID
                                           VTSS_VID_NULL,
                                           NULL,
                                           new_conf->mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK ? new_conf->trunk_allowed_vids : new_conf->hybrid_allowed_vids)); // New allowed VIDs
        } else {
            if (new_conf->mode == VTSS_APPL_VLAN_PORT_MODE_ACCESS) {
                // Going from a multi-VID mode to a single-VID mode.
                // Gotta remove membership from all defined VLANs in the multi-VID mode question and add membership of the single-VID.
                EXIT_RC(VLAN_membership_update(isid, &ports,
                                               VTSS_VID_NULL,
                                               new_conf->access_pvid,                                                                                           // New VID
                                               old_conf->mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK ? old_conf->trunk_allowed_vids : old_conf->hybrid_allowed_vids, // Old allowed VIDs
                                               NULL));
            } else {
                // Going from one multi-VID mode to another multi-VID mode.
                // Gotta remove membership from all defined VLANs in the old multi-VID mode and add membership to all defined VLANs in the new multi-VID mode.
                EXIT_RC(VLAN_membership_update(isid, &ports,
                                               VTSS_VID_NULL,
                                               VTSS_VID_NULL,
                                               old_conf->mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK ? old_conf->trunk_allowed_vids : old_conf->hybrid_allowed_vids,   // Old allowed VIDs
                                               new_conf->mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK ? new_conf->trunk_allowed_vids : new_conf->hybrid_allowed_vids)); // New allowed VIDs
            }
        }
    } else {
        // We're staying in the same mode, but it could be that the access VID is changing or the allowed VIDs have changed
        if (new_conf->mode == VTSS_APPL_VLAN_PORT_MODE_ACCESS) {
            if (old_conf->access_pvid != new_conf->access_pvid) {
                // It is. Gotta change membeship
                EXIT_RC(VLAN_membership_update(isid, &ports,
                                               old_conf->access_pvid, // Old VID
                                               new_conf->access_pvid, // New VID
                                               NULL,
                                               NULL));
            }
        } else {
            // Perhaps an allowed VID mask has changed for the non-access mode
            EXIT_RC(VLAN_membership_update(isid, &ports,
                                           VTSS_VID_NULL,
                                           VTSS_VID_NULL,
                                           old_conf->mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK ? old_conf->trunk_allowed_vids : old_conf->hybrid_allowed_vids,   // Old allowed VIDs
                                           new_conf->mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK ? new_conf->trunk_allowed_vids : new_conf->hybrid_allowed_vids)); // New allowed VIDs
        }
    }

    // Time to update the port configuration

    EXIT_RC(VLAN_port_to_detailed_conf(new_conf, &port_detailed_conf));

    // And then - finally - time to update our state.
    *old_conf = *new_conf;

#undef EXIT_RC
do_exit:
    VLAN_bulk_end();
    VLAN_CRIT_EXIT();

    if (rc != VTSS_RC_OK) {
        return rc;
    }

    return VLAN_port_detailed_conf_set(isid, port_no, &port_detailed_conf, VLAN_USER_INT_STATIC);
}

/******************************************************************************/
// VLAN_port_default()
/******************************************************************************/
static void VLAN_port_default(vtss_isid_t isid)
{
    vtss_appl_vlan_port_conf_t conf;
    switch_iter_t              sit;
    mesa_port_no_t             iport;
    mesa_rc                     rc;

    (void)vlan_mgmt_port_conf_default_get(&conf);

    // Default all switches pointed out by #isid, configurable as well as non-configurable
    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID_ALL);
    while (switch_iter_getnext(&sit)) {
        // Configure all ports, including stack ports.
        // We cannot use the port iterator unless we are the primary switch,
        // which we aren't during INIT_CMD_START.
        for (iport = 0; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
            // Set default conf
            if ((rc = VLAN_port_conf_set(sit.isid, iport, &conf)) != VTSS_RC_OK) {
                T_E("%u:%u: Say what? %s", sit.isid, iport, error_txt(rc));
            }
        }
    }
}

/******************************************************************************/
// VLAN_default()
// If VTSS_ISID_LEGAL(isid), default port configuration.
// If isid == VTSS_ISID_GLOBAL, default all ports' configuration,
// VLAN memberships and VLAN names.
/******************************************************************************/
static void VLAN_default(vtss_isid_t isid)
{
    mesa_rc                             rc;
#if defined(VLAN_VOLATILE_USER_PRESENT)
    switch_iter_t                       sit;
    port_iter_t                         pit;
    vlan_user_int_t                     user;
    vtss_appl_vlan_port_detailed_conf_t new_conf;
#endif /* defined(VLAN_VOLATILE_USER_PRESENT) */

    vlan_bulk_update_begin();

#if defined(VLAN_VOLATILE_USER_PRESENT)
    // Whether we create defaults or read from flash, start by defaulting all VLAN users' port configuration.
    // The reason we do it through the VLAN_port_detailed_conf_set() function is that we want possible
    // change-subscribers to get notified about any changes that may occur.
    memset(&new_conf, 0, sizeof(new_conf)); // Ensure that also padding fields are initialized to 0
    new_conf.flags = 0; // This causes the VLAN module to forget everything about a particular user's override.

    // Here, we only "un"-configure configurable switches, because these are the only ones that can
    // have volatile configuration attached.
    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            // Static user is handled separately, because it must use VLAN_port_conf_set().
            for (user = (vlan_user_int_t)(VLAN_USER_INT_STATIC + 1); user < VLAN_USER_INT_ALL; user++) {
                if (!vlan_mgmt_user_is_port_conf_changer(VLAN_user_int_to_ext(user))) {
                    continue;
                }

                if ((rc = VLAN_port_detailed_conf_set(sit.isid, pit.iport, &new_conf, user)) != VTSS_RC_OK) {
                    T_E("%u:%u: Say what? %s", sit.usid, pit.uport, error_txt(rc));
                }
            }
        }
    }
#endif /* defined(VLAN_VOLATILE_USER_PRESENT) */

    if (VTSS_APPL_VLAN_FID_CNT && isid == VTSS_ISID_GLOBAL) {
        memset(vlan_fid_table, 0, sizeof(vlan_fid_table));
        VLAN_msg_tx_fid_table_reset(VTSS_ISID_GLOBAL);
    }

    // Create defaults for static user.
    VLAN_port_default(isid);

    if (isid == VTSS_ISID_GLOBAL) {
        u32                    i;
        vtss_appl_vlan_entry_t membership;

        // Also default memberships, custom S-tag, and VLAN names.

        // We need to remove all existing VLANs and notify all modules about the removal.
        // Volatile VLAN users expect us to also reset their volatile configuration.
        // The reason we do it through the vlan_mgmt_vlan_del() function is that we want possible
        // change-subscribers to get notified about any changes that may occur.

        for (i = 0; i < 2; i++) {
            // Forbidden VLANs are not removed by removing VLAN_USER_INT_ALL, so we need to do this in two tempi.
            vtss_appl_vlan_user_t ext_user = i == 0 ? VTSS_APPL_VLAN_USER_ALL : VTSS_APPL_VLAN_USER_FORBIDDEN;

            membership.vid = VTSS_VID_NULL;

            while (vtss_appl_vlan_get(VTSS_ISID_GLOBAL, membership.vid, &membership, TRUE, ext_user) == VTSS_RC_OK) {
                if ((rc = vlan_mgmt_vlan_del(VTSS_ISID_GLOBAL, membership.vid, ext_user)) != VTSS_RC_OK) {
                    T_E("Say what? %u, %s, %s", membership.vid, vlan_mgmt_user_to_txt(ext_user), error_txt(rc));
                }
            }
        }

        // Do create membership defaults.
        membership.vid = VTSS_APPL_VLAN_ID_DEFAULT;
        if ((rc = vlan_mgmt_vlan_add(VTSS_ISID_GLOBAL, &membership, VTSS_APPL_VLAN_USER_STATIC)) != VTSS_RC_OK) {
            T_E("Say what? %s", error_txt(rc));
        }

        // And default etype.
        if ((rc = vtss_appl_vlan_s_custom_etype_set(VTSS_APPL_VLAN_CUSTOM_S_TAG_DEFAULT)) != VTSS_RC_OK) {
            T_E("Say what? %s", error_txt(rc));
        }

        VLAN_flooding_default();

        // By default, there are no forbidden VLANs, so nothing to do in that respect.

#if defined(VTSS_SW_OPTION_VLAN_NAMING)
        // VLAN naming defaults
        VLAN_name_default();
#endif /* defined(VTSS_SW_OPTION_VLAN_NAMING) */
    }

    vlan_bulk_update_end();
}

/******************************************************************************/
// VLAN_table_init()
// Stitches together the linked list of free VLAN multi-membership entries.
/******************************************************************************/
static void VLAN_table_init(void)
{
    u32 i;

    VLAN_CRIT_ENTER();

    vtss_clear(VLAN_table);
    for (i = 0; i < ARRSZ(VLAN_table) - 1; i++) {
        VLAN_table[i].next = &VLAN_table[i + 1];
    }

    VLAN_free_list = &VLAN_table[0];
    VLAN_table[ARRSZ(VLAN_table) - 1].next = NULL;

    VLAN_CRIT_EXIT();
}

#if defined(VTSS_SW_OPTION_VLAN_NAMING)
/******************************************************************************/
// VLAN_reserved_name_to_vid()
// Returns the VLAN ID corresponding to #name if #name is one of the reserved
// names (VTSS_APPL_VLAN_NAME_DEFAULT or matches "VLANxxxx" template).
// Otherwise returns VTSS_VID_NULL.
/******************************************************************************/
static mesa_vid_t VLAN_reserved_name_to_vid(const char name[VTSS_APPL_VLAN_NAME_MAX_LEN])
{
    if (name[0] == '\0') {
        return VTSS_VID_NULL;
    }

    // Check to see if #name is either VTSS_APPL_VLAN_NAME_DEFAULT or on the form "VLANxxxx".
    // If so, return the VLAN ID corresponding to these.
    // Otherwise return VTSS_VID_NULL.
    if (strcmp(name, VTSS_APPL_VLAN_NAME_DEFAULT) == 0) {
        return VTSS_APPL_VLAN_ID_DEFAULT;
    }

    // Check against "VLANxxxx" template.
    if (strlen(name) == 8 && strncmp(name, "VLAN", 4) == 0) {
        mesa_vid_t v;
        int        i;

        v = 0;
        for (i = 0; i < 4; i++) {
            char c = name[4 + i];

            if (c < '0' || c > '9') {
                // Doesn't match template
                return VTSS_VID_NULL;
            }

            v *= 10;
            v += c - '0';;
        }

        // The name might be reserved, because it adheres to the "VLANxxxx" template.
        if (v >= VTSS_APPL_VLAN_ID_MIN && v <= VTSS_APPL_VLAN_ID_MAX) {
            // It's a valid VLAN ID
            return v;
        }
    }

    return VTSS_VID_NULL;
}
#endif /* defined(VTSS_SW_OPTION_VLAN_NAMING) */

#if defined(VTSS_SW_OPTION_VLAN_NAMING)
/******************************************************************************/
// VLAN_name_lookup()
/******************************************************************************/
static mesa_vid_t VLAN_name_lookup(const char name[VTSS_APPL_VLAN_NAME_MAX_LEN])
{
    mesa_vid_t v;

    VLAN_CRIT_ASSERT_LOCKED();

    if (name[0] != '\0') {
        for (v = VTSS_APPL_VLAN_ID_MIN; v <= VTSS_APPL_VLAN_ID_MAX; v++) {
            if (strcmp(VLAN_name_conf[v], name) == 0) {
                return v;
            }
        }
    }

    return VTSS_VID_NULL;
}
#endif /* defined(VTSS_SW_OPTION_VLAN_NAMING) */

/******************************************************************************/
//
// Public functions
//
/******************************************************************************/

/******************************************************************************/
// vlan_error_txt()
/******************************************************************************/
const char *vlan_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VLAN_ERROR_GEN:
        return "VLAN generic error";

    case VLAN_ERROR_PARM:
        return "VLAN parameter error";

    case VLAN_ERROR_ISID:
        return "Invalid Switch ID";

    case VLAN_ERROR_PORT:
        return "Invalid port number";

    case VLAN_ERROR_IFINDEX:
        return "Invalid ifindex";

    case VLAN_ERROR_MUST_BE_PRIMARY_SWITCH:
        return "Operation only valid on primary switch";

    case VLAN_ERROR_NOT_CONFIGURABLE:
        return "Switch not configurable";

    case VLAN_ERROR_USER:
        return "Invalid VLAN User";

    case VLAN_ERROR_VID:
        return "Invalid VLAN ID";

    case VLAN_ERROR_FID:
        return "Invalid Filter ID (FID). Valid range is 0 to " vtss_xstr(VTSS_APPL_VLAN_FID_CNT);

    case VLAN_ERROR_ALLOWED_HYBRID_VID:
        return "Invalid VLAN ID selected in allowed hybrid VLANs";

    case VLAN_ERROR_ALLOWED_TRUNK_VID:
        return "Invalid VLAN ID selected in allowed trunk VLANs";

    case VLAN_ERROR_FLAGS:
        return "At least one field to override must be set";

    case VLAN_ERROR_PVID:
        return "Invalid Port VLAN ID";

    case VLAN_ERROR_FRAME_TYPE:
        return "Invalid frame type";

    case VLAN_ERROR_TX_TAG_TYPE:
        return "Invalid Tx tag type";

    case VLAN_ERROR_UVID:
        return "Invalid untagged VID";

    case VLAN_ERROR_TPID:
        return "Invalid Custom-S TPID";

    case VLAN_ERROR_PORT_MODE:
        return "Invalid port mode";

    case VLAN_ERROR_ENTRY_NOT_FOUND:
        return "Entry not found";

    case VLAN_ERROR_VLAN_TABLE_FULL:
        return "VLAN table full";

    case VLAN_ERROR_USER_PREVIOUSLY_CONFIGURED:
        return "Previously configured";

    case VLAN_ERROR_FLOODING_NOT_MANAGEABLE:
        return "Flooding can not be managed";

#if defined(VTSS_SW_OPTION_VLAN_NAMING)
    case VLAN_ERROR_NAME_ALREADY_EXISTS:
        return "A VLAN with that name already exists";

    case VLAN_ERROR_NAME_RESERVED:
        return "The VLAN name is reserved for another VLAN ID";

    case VLAN_ERROR_NAME_DEFAULT_VLAN:
        return "The default VLAN's name cannot be changed";

    case VLAN_ERROR_NAME_DOES_NOT_EXIST:
        return "VLAN name does not exist";

    case VLAN_ERROR_NAME_VLAN_NOT_CREATED:
        return "Cannot change name of VLANs that are not created";

#endif /* defined(VTSS_SW_OPTION_VLAN_NAMING) */

    default:
        return "Unknown VLAN error";
    }
}

/******************************************************************************/
// vlan_mgmt_port_conf_default_get()
// Returns a default VLAN configuration.
/******************************************************************************/
mesa_rc vlan_mgmt_port_conf_default_get(vtss_appl_vlan_port_conf_t *conf)
{
    if (conf == NULL) {
        return VLAN_ERROR_PARM;
    }

    // These are the default port mode values
    memset(conf, 0, sizeof(*conf));
    conf->mode                  = VTSS_APPL_VLAN_PORT_MODE_ACCESS;
    conf->access_pvid           = VTSS_APPL_VLAN_ID_DEFAULT;
    conf->trunk_pvid            = VTSS_APPL_VLAN_ID_DEFAULT;
    conf->trunk_tag_pvid        = FALSE;
    conf->hybrid.pvid           = VTSS_APPL_VLAN_ID_DEFAULT;
    conf->hybrid.untagged_vid   = conf->hybrid.pvid;
    conf->hybrid.frame_type     = MESA_VLAN_FRAME_ALL;
    conf->hybrid.ingress_filter = FALSE;
    conf->hybrid.tx_tag_type    = VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS;
    conf->hybrid.port_type      = VTSS_APPL_VLAN_PORT_TYPE_C;
    conf->hybrid.flags          = VTSS_APPL_VLAN_PORT_FLAGS_ALL;

    // By default trunk and hybrid ports are members of all VLANs.
    (void)vlan_mgmt_bitmask_all_ones_set(conf->trunk_allowed_vids);
    (void)vlan_mgmt_bitmask_all_ones_set(conf->hybrid_allowed_vids);

    return VTSS_RC_OK;
}

/******************************************************************************/
// vlan_port_conf_change_register()
// Some modules need to know when vlan configuration changes. For
// doing this a callback function is provided. At the time of
// writing there is no need for being able to unregister.
/******************************************************************************/
void vlan_port_conf_change_register(vtss_module_id_t modid, vlan_port_conf_change_callback_t cb, BOOL cb_on_primary)
{
    u32 i;

    if (cb == NULL) {
        T_EG(TRACE_GRP_CB, "Invalid parameter, cb");
        return;
    }

    if (modid < 0 || modid >= VTSS_MODULE_ID_NONE) {
        T_EG(TRACE_GRP_CB, "Invalid module ID, %d", modid);
        return;
    }

    T_DG(TRACE_GRP_CB, "%s: Registering for %s VLAN port configuration changes", vtss_module_names[modid], cb_on_primary ? "primary" : "local switch");

    VLAN_CB_CRIT_ENTER();
    for (i = 0; i < ARRSZ(VLAN_pcc_cb); i++) {
        if (VLAN_pcc_cb[i].cb == NULL) {
            VLAN_pcc_cb[i].modid        = modid;
            VLAN_pcc_cb[i].cb           = cb;
            VLAN_pcc_cb[i].cb_on_primary = cb_on_primary;
            break;
        }
    }
    VLAN_CB_CRIT_EXIT();

    if (i == ARRSZ(VLAN_pcc_cb)) {
        T_EG(TRACE_GRP_CB, "Ran out of registration entries");
    }
}

/******************************************************************************/
// vlan_svl_conf_change_register()
/******************************************************************************/
void vlan_svl_conf_change_register(vtss_module_id_t modid, vlan_svl_conf_change_callback_t cb)
{
    u32 i;

    if (cb == NULL) {
        T_EG(TRACE_GRP_CB, "Invalid parameter, cb");
        return;
    }

    if (modid < 0 || modid >= VTSS_MODULE_ID_NONE) {
        T_EG(TRACE_GRP_CB, "Invalid module ID, %d", modid);
        return;
    }

    T_DG(TRACE_GRP_CB, "%s: Registering for SVL configuration changes", vtss_module_names[modid]);

    VLAN_CB_CRIT_ENTER();
    for (i = 0; i < ARRSZ(VLAN_svl_cb); i++) {
        if (VLAN_svl_cb[i].cb == NULL) {
            VLAN_svl_cb[i].modid = modid;
            VLAN_svl_cb[i].cb    = cb;
            break;
        }
    }
    VLAN_CB_CRIT_EXIT();

    if (i == ARRSZ(VLAN_svl_cb)) {
        T_EG(TRACE_GRP_CB, "Ran out of registration entries when trying to add %s", vtss_module_names[modid]);
    }
}

/******************************************************************************/
// vlan_membership_change_register()
/******************************************************************************/
void vlan_membership_change_register(vtss_module_id_t modid, vlan_membership_change_callback_t cb)
{
    u32 i;

    if (cb == NULL) {
        T_EG(TRACE_GRP_CB, "Invalid parameter, cb");
        return;
    }

    if (modid < 0 || modid >= VTSS_MODULE_ID_NONE) {
        T_EG(TRACE_GRP_CB, "Invalid module ID, %d", modid);
        return;
    }

    T_DG(TRACE_GRP_CB, "%s: Registering for VLAN membership changes", vtss_module_names[modid]);

    VLAN_CB_CRIT_ENTER();
    for (i = 0; i < ARRSZ(VLAN_mc_cb); i++) {
        if (VLAN_mc_cb[i].u.cb == NULL) {
            VLAN_mc_cb[i].modid = modid;
            VLAN_mc_cb[i].u.cb   = cb;
            break;
        }
    }
    VLAN_CB_CRIT_EXIT();

    if (i == ARRSZ(VLAN_mc_cb)) {
        T_EG(TRACE_GRP_CB, "Ran out of registration entries");
    }
}

/******************************************************************************/
// vlan_membership_bulk_change_register()
/******************************************************************************/
void vlan_membership_bulk_change_register(vtss_module_id_t modid, vlan_membership_bulk_change_callback_t cb)
{
    u32 i;

    if (cb == NULL) {
        T_EG(TRACE_GRP_CB, "Invalid parameter, cb");
        return;
    }

    if (modid < 0 || modid >= VTSS_MODULE_ID_NONE) {
        T_EG(TRACE_GRP_CB, "Invalid module ID, %d", modid);
        return;
    }

    T_DG(TRACE_GRP_CB, "%s: Registering for *bulk* VLAN membership changes", vtss_module_names[modid]);

    VLAN_CB_CRIT_ENTER();
    for (i = 0; i < ARRSZ(VLAN_mc_bcb); i++) {
        if (VLAN_mc_bcb[i].u.bcb == NULL) {
            VLAN_mc_bcb[i].modid = modid;
            VLAN_mc_bcb[i].u.bcb = cb;
            break;
        }
    }
    VLAN_CB_CRIT_EXIT();

    if (i == ARRSZ(VLAN_mc_bcb)) {
        T_EG(TRACE_GRP_CB, "Ran out of registration entries");
    }
}

/******************************************************************************/
// vlan_bulk_update_begin()
/******************************************************************************/
void vlan_bulk_update_begin(void)
{
    VLAN_CRIT_ENTER();
    VLAN_bulk_begin();
    VLAN_CRIT_EXIT();
}

/******************************************************************************/
// vlan_bulk_update_end()
/******************************************************************************/
void vlan_bulk_update_end(void)
{
    VLAN_CRIT_ENTER();
    VLAN_bulk_end();
    VLAN_CRIT_EXIT();
}

/******************************************************************************/
// vlan_bulk_update_ref_cnt_get()
/******************************************************************************/
u32 vlan_bulk_update_ref_cnt_get(void)
{
    u32 result;
    VLAN_CRIT_ENTER();
    result = VLAN_bulk.ref_cnt;
    VLAN_CRIT_EXIT();

    return result;
}

/******************************************************************************/
// vlan_s_custom_etype_change_register()
/******************************************************************************/
void vlan_s_custom_etype_change_register(vtss_module_id_t modid, vlan_s_custom_etype_change_callback_t cb)
{
    u32 i;

    if (cb == NULL) {
        T_EG(TRACE_GRP_CB, "Invalid parameter, cb");
        return;
    }

    if (modid < 0 || modid >= VTSS_MODULE_ID_NONE) {
        T_EG(TRACE_GRP_CB, "Invalid module ID, %d", modid);
        return;
    }

    T_DG(TRACE_GRP_CB, "%s: Registering for S-Custom Ethertype changes", vtss_module_names[modid]);

    VLAN_CB_CRIT_ENTER();
    for (i = 0; i < ARRSZ(VLAN_s_custom_etype_cb); i++) {
        if (VLAN_s_custom_etype_cb[i].u.scb == NULL) {
            VLAN_s_custom_etype_cb[i].modid = modid;
            VLAN_s_custom_etype_cb[i].u.scb = cb;
            break;
        }
    }
    VLAN_CB_CRIT_EXIT();

    if (i == ARRSZ(VLAN_s_custom_etype_cb)) {
        T_EG(TRACE_GRP_CB, "Ran out of registration entries");
    }
}

/******************************************************************************/
// vlan_mgmt_port_conf_get()
/******************************************************************************/
mesa_rc vlan_mgmt_port_conf_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_vlan_port_conf_t *conf, vtss_appl_vlan_user_t ext_user, BOOL details)
{
    BOOL            get_details = ext_user != VTSS_APPL_VLAN_USER_STATIC || details;
    vlan_user_int_t user;

    if ((user = VLAN_user_ext_to_int(ext_user)) == VLAN_USER_INT_CNT) {
        return VLAN_ERROR_USER;
    }

    VTSS_RC(VLAN_isid_port_check(isid, port_no, get_details /* allow isid == VTSS_ISID_LOCAL when getting details */, TRUE));

    if (conf == NULL) {
        return VLAN_ERROR_PARM;
    }

    if (get_details) {
        // Always pretend it's a hybrid port then, because only conf->hybrid is valid when requesting details.
        conf->mode = VTSS_APPL_VLAN_PORT_MODE_HYBRID;
        return VLAN_port_detailed_conf_get(isid, port_no, &conf->hybrid, user);
    } else {
        // Asking for the composite mode.
        return VLAN_port_conf_get(isid, port_no, conf);
    }
}

/******************************************************************************/
// vtss_appl_vlan_interface_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_vlan_interface_conf_get(vtss_ifindex_t ifindex, vtss_appl_vlan_user_t ext_user, vtss_appl_vlan_port_conf_t *conf, BOOL details)
{
    vtss_ifindex_elm_t ife;

    if (!vtss_ifindex_is_port(ifindex)) {
        return VLAN_ERROR_IFINDEX;
    }

    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    VTSS_RC(vlan_mgmt_port_conf_get(ife.isid, ife.ordinal, conf, ext_user, details));

    if (!details) {
        // vlan_mgmt_port_conf_get() doesn't fill in forbidden_vids (since it's an extension
        // for SNMP to get all in one table, for now), so we better help it. With time,
        // this will become the only interface to get/set forbidden VLANs.
        VTSS_RC(vlan_mgmt_membership_per_port_get(ife.isid, ife.ordinal, VTSS_APPL_VLAN_USER_FORBIDDEN, conf->forbidden_vids));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vlan_mgmt_port_conf_set()
/******************************************************************************/
mesa_rc vlan_mgmt_port_conf_set(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_vlan_port_conf_t *conf, vtss_appl_vlan_user_t ext_user)
{
    vlan_user_int_t user;

    VTSS_RC(VLAN_isid_port_check(isid, port_no, FALSE, TRUE));

    if ((user = VLAN_user_ext_to_int(ext_user)) == VLAN_USER_INT_CNT) {
        return VLAN_ERROR_USER;
    }

    if (!msg_switch_configurable(isid)) {
        return VLAN_ERROR_NOT_CONFIGURABLE;
    }

    if (user == VLAN_USER_INT_STATIC) {
        return VLAN_port_conf_set(isid, port_no, (vtss_appl_vlan_port_conf_t * /* it's const */)conf);
    } else {
        return VLAN_port_detailed_conf_set(isid, port_no, &conf->hybrid, user);
    }
}

/******************************************************************************/
// vtss_appl_vlan_interface_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_vlan_interface_conf_set(vtss_ifindex_t ifindex, const vtss_appl_vlan_port_conf_t *conf)
{
    vtss_ifindex_elm_t ife;
    vlan_ports_t       forbidden_ports;
    mesa_vid_t         vid;
    vtss_isid_t        zisid;
    mesa_rc            rc;

    if (!vtss_ifindex_is_port(ifindex)) {
        return VLAN_ERROR_IFINDEX;
    }

    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    vlan_bulk_update_begin();

    T_D("port-conf-set: %u, %u", ife.isid, ife.ordinal);
    if ((rc = vlan_mgmt_port_conf_set(ife.isid, ife.ordinal, conf, VTSS_APPL_VLAN_USER_STATIC)) != VTSS_RC_OK) {
        goto do_exit;
    }

    // vlan_mgmt_port_conf_set() doesn't use conf->forbidden_vids (since it's an extension
    // for SNMP to get all port-related information in one table, for now), so we better help it.
    // With time, this will become the only interface to get/set forbidden VLANs.
    zisid = ife.isid - VTSS_ISID_START;

    VLAN_CRIT_ENTER();

    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
        BOOL new_is_forbidden = VTSS_BF_GET(conf->forbidden_vids, vid);
        if (VLAN_forbidden_table[vid].ports[zisid][ife.ordinal] != new_is_forbidden) {
            port_iter_t pit;
            BOOL        do_add = FALSE;

            // Seems like we're crossing the river to get water, but what can we do, when the interface
            // is per-vid rather than per-port?!?
            (void)VLAN_get(ife.isid, vid, VLAN_USER_INT_FORBIDDEN, FALSE, &forbidden_ports);

            forbidden_ports.ports[ife.ordinal] = new_is_forbidden;

            // Gotta run through it once more to figure out whether to add or delete this forbidden VLAN.
            // This time, go through all known (non-stacking) ports on the switch.
            VTSS_RC(port_iter_init(&pit, NULL, ife.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL));
            while (port_iter_getnext(&pit)) {
                if (forbidden_ports.ports[pit.iport]) {
                    do_add = TRUE;
                    break;
                }
            }

            if (do_add) {
                if ((rc = VLAN_add(ife.isid, vid, VLAN_USER_INT_FORBIDDEN, &forbidden_ports)) != VTSS_RC_OK) {
                    goto do_exit_with_crit_exit;
                }
            } else {
                VLAN_del(ife.isid, vid, VLAN_USER_INT_FORBIDDEN);
            }
        }
    }

do_exit_with_crit_exit:
    VLAN_CRIT_EXIT();

do_exit:
    vlan_bulk_update_end();
    return rc;
}

/******************************************************************************/
// vtss_appl_vlan_s_custom_etype_get()
/******************************************************************************/
mesa_rc vtss_appl_vlan_s_custom_etype_get(mesa_etype_t *tpid)
{
    if (tpid == NULL) {
        return VLAN_ERROR_PARM;
    }

    VLAN_CRIT_ENTER();
    *tpid = VLAN_tpid_s_custom_port;
    VLAN_CRIT_EXIT();
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_vlan_s_custom_etype_set()
/******************************************************************************/
mesa_rc vtss_appl_vlan_s_custom_etype_set(mesa_etype_t tpid)
{
    BOOL update = FALSE;

    T_D("TPID = 0x%x", tpid);

    if (tpid < 0x600) {
        T_I("EtherType should always be greater than or equal to 0x0600");
        return VLAN_ERROR_TPID;
    }

    VLAN_CRIT_ENTER();
    if (tpid != VLAN_tpid_s_custom_port) {
        VLAN_tpid_s_custom_port = tpid;
        update = TRUE;
    }
    VLAN_CRIT_EXIT();

    if (update) {
        // Send the configuration to all switches in the stack
        VLAN_msg_tx_tpid_conf(tpid);
        VLAN_s_custom_etype_change_callback(tpid);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_vlan_get()
/******************************************************************************/
mesa_rc vtss_appl_vlan_get(vtss_isid_t isid, mesa_vid_t vid, vtss_appl_vlan_entry_t *membership, BOOL next, vtss_appl_vlan_user_t ext_user)
{
    vlan_ports_t    entry;
    mesa_rc         rc = VTSS_RC_OK;
    vlan_user_int_t user;

    if ((user = VLAN_user_ext_to_int(ext_user)) == VLAN_USER_INT_CNT) {
        return VLAN_ERROR_USER;
    }

    if (isid == VTSS_ISID_GLOBAL) {
        // Allow VTSS_ISID_GLOBAL, but only on primary switch
        if (!msg_switch_is_primary()) {
            return VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
        }
    } else {
        VTSS_RC(VLAN_isid_port_check(isid, 0, TRUE, FALSE));
    }

    if (next) {
        // When asking for next VID, allow all positive VIDs,
        // but shortcut the function if it's at or greater than the
        // maximum possible VID.
        if (vid >= VTSS_APPL_VLAN_ID_MAX) {
            return VLAN_ERROR_ENTRY_NOT_FOUND;
        }
    } else if (vid < VTSS_APPL_VLAN_ID_MIN || vid > VTSS_APPL_VLAN_ID_MAX) {
        // If asking for a specific VID, only allow VIDs in range.
        return VLAN_ERROR_VID;
    }

    if (membership == NULL || user > VLAN_USER_INT_ALL) {
        return VLAN_ERROR_PARM;
    }

    if (isid == VTSS_ISID_LOCAL && user != VLAN_USER_INT_ALL) {
        // When requesting what's in H/W, #user must be VLAN_USER_INT_ALL
        T_E("When called with VTSS_ISID_LOCAL, the user must be VTSS_APPL_VLAN_USER_ALL and not user = %s", vlan_mgmt_user_to_txt(ext_user));
        return VLAN_ERROR_PARM;
    }

    VLAN_CRIT_ENTER();

    if (isid == VTSS_ISID_LOCAL) {
        // Read the H/W.
        if ((vid = VLAN_api_get(vid, next, &entry)) == VTSS_VID_NULL) {
            rc = VLAN_ERROR_ENTRY_NOT_FOUND;
        }
    } else {
        // Now handle legal ISIDs and VTSS_ISID_GLOBAL.
        if ((vid = VLAN_get(isid, vid, user, next, &entry)) == VTSS_VID_NULL) {
            rc = VLAN_ERROR_ENTRY_NOT_FOUND;
        }
    }

    VLAN_CRIT_EXIT();

    membership->vid = vid;

    if (isid == VTSS_ISID_GLOBAL) {
        // Entries are not filled in when invoked with this ISID.
        memset(membership->ports, 0, sizeof(membership->ports));
    } else {
        u64 dummy;
        (void)VLAN_bit_to_bool(isid, &entry, membership, &dummy);
    }

    return rc;
}

/******************************************************************************/
// vtss_appl_vlan_access_vids_get()
/******************************************************************************/
mesa_rc vtss_appl_vlan_access_vids_get(u8 access_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES])
{
    VLAN_CRIT_ENTER();
    memcpy(access_vids, VLAN_end_user_enabled_vlans, sizeof(VLAN_end_user_enabled_vlans));
    VLAN_CRIT_EXIT();

    return VTSS_RC_OK;
}

/******************************************************************************/
// vlan_mgmt_vlan_add()
// VLAN members as boolean port list.
// #isid must be legal or VTSS_ISID_GLOBAL.
//
// The VLAN will be added to all switches, but on some switches it may be
// with zero member ports.
/******************************************************************************/
mesa_rc vlan_mgmt_vlan_add(vtss_isid_t isid, vtss_appl_vlan_entry_t *membership, vtss_appl_vlan_user_t ext_user)
{
    switch_iter_t   sit;
    mesa_vid_t      vid;
    mesa_rc         rc = VTSS_RC_OK;
    vlan_user_int_t user;

    if ((user = VLAN_user_ext_to_int(ext_user)) == VLAN_USER_INT_CNT) {
        return VLAN_ERROR_USER;
    }

    if (membership == NULL) {
        return VLAN_ERROR_PARM;
    }

    // If #user != VLAN_USER_INT_STATIC, an empty port set actually means "delete VLAN".
    // In this case, we only "delete" the VLAN globally if the user doesn't have shares
    // on other switches.
    if (user != VLAN_USER_INT_STATIC) {
        port_iter_t pit;
        BOOL        at_least_one_port_included = FALSE;

        if (isid == VTSS_ISID_GLOBAL) {
            mesa_port_no_t iport;
            u32            port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

            // Don't know which switch the user means, so we have to traverse all
            // ports, and not just those that are present on the switch in question.
            for (iport = 0; iport < port_cnt; iport++) {
                if (membership->ports[iport]) {
                    at_least_one_port_included = TRUE;
                    break;
                }
            }
        } else {
            (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (membership->ports[pit.iport]) {
                    at_least_one_port_included = TRUE;
                    break;
                }
            }
        }

        if (!at_least_one_port_included) {
            // The non-static user no longer wants to add membership on this switch.
            return vlan_mgmt_vlan_del(isid, membership->vid, ext_user);
        }
    }

    if (user == VLAN_USER_INT_STATIC) {
        // Gotta add on all switches according to the ports' configuration.
        isid = VTSS_ISID_GLOBAL;
    }

    if (isid == VTSS_ISID_GLOBAL) {
        // Allow VTSS_ISID_GLOBAL, but only on primary switch
        if (!msg_switch_is_primary()) {
            return VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
        }
    } else {
        VTSS_RC(VLAN_isid_port_check(isid, 0, FALSE, FALSE));
    }

    if (user >= VLAN_USER_INT_ALL) {
        return VLAN_ERROR_USER;
    }

    if (!vlan_mgmt_user_is_membership_changer(ext_user)) {
        T_I("Unexpected user calling this function: %s. Will get allowed, but code should be updated", vlan_mgmt_user_to_txt(ext_user));
        // Continue execution
    }

    vid = membership->vid;
    if (vid < VTSS_APPL_VLAN_ID_MIN || vid > VTSS_APPL_VLAN_ID_MAX) {
        return VLAN_ERROR_VID;
    }

    T_D("isid %u, vid %u, user %s", isid, membership->vid, vlan_mgmt_user_to_txt(ext_user));

    VLAN_CRIT_ENTER();
    VLAN_bulk_begin();

    if (user == VLAN_USER_INT_STATIC) {
        // In order to be able to distinguish end-user-enabled VLANs
        // from auto-added VLANs (due to trunking), we must keep track
        // of the end-user-enabled.
        VTSS_BF_SET(VLAN_end_user_enabled_vlans, vid, TRUE);
    }

    // When adding a VLAN, it must be added to all switches, even if there are no
    // members on a particular switch.
    // So we iterate over VTSS_ISID_GLOBAL and *all* ISIDs, so that when a brand-new
    // switch enters the stack, it will automatically receive the required VLANs
    // and memberships.
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
    while (switch_iter_getnext(&sit)) {
        vlan_ports_t ports;

        // If it's the static user that is adding VLANs, we need to auto-add
        // ports depending on their port mode.
        // The value of #membership->ports when this function got invoked is not
        // used for anything in that case, because we know the whole story already.
        if (user == VLAN_USER_INT_STATIC) {
            port_iter_t pit;

            // Loop over *all* ports (not just those existing on the switch). The
            // reason for this is that we must also cater for switches that have
            // never been seen in the stack in which case we don't know how many ports it has
            // or even where the stack ports are located.
            (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT_ALL, PORT_ITER_FLAGS_ALL);
            while (port_iter_getnext(&pit)) {
                vtss_appl_vlan_port_conf_t *conf = &VLAN_port_conf[sit.isid - VTSS_ISID_START][pit.iport];

                if (conf->mode == VTSS_APPL_VLAN_PORT_MODE_ACCESS) {
                    membership->ports[pit.iport] = vid == conf->access_pvid;
                } else {
                    // Trunk or hybrid. Membership depends on allowed VIDs.
                    membership->ports[pit.iport] = VTSS_BF_GET(conf->mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK ? conf->trunk_allowed_vids : conf->hybrid_allowed_vids, vid);
                }
            }
        }

        // Figure out if memberships are empty for this particular switch.
        // Notice that we need to convert the array for each and every switch,
        // because the resulting #ports may differ depending on number of
        // ports and location of stack ports on switch in question.
        if (isid == VTSS_ISID_GLOBAL || isid == sit.isid) {
            // Either all switches must have this configuration (isid == VTSS_ISID_GLOBAL)
            // or we're currently updating the ISID that the user has requested.
            VLAN_bool_to_bit(sit.isid, membership, &ports);

            // Now that we have updated the memberships for VLAN_USER_INT_STATIC,
            // do the actual updating of the VLANs. This is handled in a
            // separate function, because it is also required by the functions
            // that can change port mode and allowed VIDs.
            if ((rc = VLAN_add_del_core(sit.isid, vid, user, &ports, VLAN_BIT_OPERATION_OVERWRITE)) != VTSS_RC_OK) {
                // Shouldn't be possible to return here.
                break;
            }
        }
    }

    VLAN_CRIT_EXIT();
    vlan_bulk_update_end();

    return rc;
}

/******************************************************************************/
// vlan_mgmt_vlan_del()
// #isid must be legal or VTSS_ISID_GLOBAL.
// #user must be in range [VLAN_USER_INT_STATIC; VLAN_USER_INT_ALL], i.e. VLAN_USER_INT_ALL
// is also valid! If VLAN_USER_INT_ALL, #isid must be VTSS_ISID_GLOBAL.
// The function returns VTSS_RC_OK even if deleting VLANs that don't exist beforehand.
//
// The VLAN will not be marked as deleted until the last switch has gotten the
// VLAN deleted. Listeners will only get called back when either
// 1) The last switch gets the VLAN deleted, or
// 2) at least one port was member on the switch in question.
/******************************************************************************/
mesa_rc vlan_mgmt_vlan_del(vtss_isid_t isid, mesa_vid_t vid, vtss_appl_vlan_user_t ext_user)
{
    switch_iter_t   sit;
    vlan_user_int_t user, user_iter, user_min, user_max;
    BOOL            was_end_user_enabled = FALSE;

    if ((user = VLAN_user_ext_to_int(ext_user)) == VLAN_USER_INT_CNT) {
        return VLAN_ERROR_USER;
    }

    if (isid == VTSS_ISID_GLOBAL) {
        // Allow VTSS_ISID_GLOBAL, but only on primary switch
        if (!msg_switch_is_primary()) {
            // Possibly a transient phenomenon.
            T_D("Not primary switch");
            return VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
        }
    } else {
        VTSS_RC(VLAN_isid_port_check(isid, 0, FALSE, FALSE));
    }

    if (vid < VTSS_APPL_VLAN_ID_MIN || vid > VTSS_APPL_VLAN_ID_MAX) {
        T_E("Invalid VID (%u)", vid);
        return VLAN_ERROR_VID;
    }

    // Allow VLAN_USER_INT_ALL
    if (user > VLAN_USER_INT_ALL) {
        T_E("Invalid user (%d)", user);
        return VLAN_ERROR_USER;
    }

    if (user != VLAN_USER_INT_ALL && !vlan_mgmt_user_is_membership_changer(ext_user)) {
        T_I("Unexpected user calling this function: %s. Will get allowed, but code should be updated", vlan_mgmt_user_to_txt(ext_user));
        // Continue execution
    }

    if (user == VLAN_USER_INT_ALL) {
        user_min = VLAN_USER_INT_STATIC;
        user_max = (vlan_user_int_t)(VLAN_USER_INT_ALL - 1);
        // Since the static user requires #isid to be VTSS_ISID_GLOBAL,
        // we also need it here, because all users include the static user.
        isid = VTSS_ISID_GLOBAL;
    } else {
        user_min = user_max = user;
    }

    if (user == VLAN_USER_INT_STATIC) {
        // Always remove everything.
        isid = VTSS_ISID_GLOBAL;
    }

    T_D("isid %u, vid %d, user %s)", isid, vid, vlan_mgmt_user_to_txt(ext_user));

    VLAN_CRIT_ENTER();
    VLAN_bulk_begin();

    if (user_min == VLAN_USER_INT_STATIC) {
        // In order to be able to distinguish end-user-enabled VLANs
        // from auto-added VLANs (due to trunking), we must keep track
        // of the end-user-enabled.
        // We must set it prior to entering the loop below, because
        // VLAN_add_del_core() checks it.
        was_end_user_enabled = VTSS_BF_GET(VLAN_end_user_enabled_vlans, vid);
        if (was_end_user_enabled) {
            VTSS_BF_SET(VLAN_end_user_enabled_vlans, vid, FALSE);
        }
    }

    // Make sure to get all in-use bits cleared in case we've been called with VTSS_ISID_GLOBAL.
    // This is ensured by iterating over *all* switches (if #isid == VTSS_ISID_GLOBAL).
    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID_ALL);
    while (switch_iter_getnext(&sit)) {

        // Iterate over all requested users.
        for (user_iter = user_min; user_iter <= user_max; user_iter++) {
            BOOL         at_least_one_change = TRUE, force_delete = TRUE;
            vlan_ports_t ports;

            if (user != VLAN_USER_INT_FORBIDDEN && user_iter == VLAN_USER_INT_FORBIDDEN) {
                // We must not remove forbidden membership unless we're invoked
                // with #ext_user == VTSS_APPL_VLAN_USER_FORBIDDEN.
                continue;
            }

            ports.ports[0] = 0; // Satisfy Lint

            if (user_iter == VLAN_USER_INT_STATIC) {
                // Static user is pretty special, because deleting a VLAN
                // doesn't necessarily mean that all ports belonging to that VLAN
                // should have their membership removed.
                // Hybrid and Trunk ports should retain their current membership,
                // whereas access ports whose PVID is the removed VLAN should
                // be taken out. Therefore, we need to traverse the current
                // port configuration and take proper action to only update
                // access ports.
                at_least_one_change = was_end_user_enabled; // Only call VLAN_add_del_core() if this is an administrator-added access VLAN.
                force_delete        = FALSE;                // If set to TRUE, all ports will get removed from the VLAN. Here, we select which ports to take out by setting it to FALSE.

                if (was_end_user_enabled) {
                    // Only take out access ports if the VLAN had been added previously by the administrator with a "vlan XXX" command.
                    mesa_port_no_t port;

                    vtss_clear(ports);
                    for (port = 0; port < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port++) {
                        vtss_appl_vlan_port_conf_t *port_conf = &VLAN_port_conf[sit.isid - VTSS_ISID_START][port];
                        if (port_conf->mode == VTSS_APPL_VLAN_PORT_MODE_ACCESS && vid == port_conf->access_pvid) {
                            // Gotta remove this ports from the VLAN.
                            ports.ports[port] = TRUE;
                        }
                    }
                }
            }

            if (at_least_one_change) {
                mesa_rc rc;
                if ((rc = VLAN_add_del_core(sit.isid, vid, user_iter, force_delete ? NULL : &ports, VLAN_BIT_OPERATION_DEL)) != VTSS_RC_OK) {
                    T_E("What happened? %s", error_txt(rc));
                }
            }
        }
    }

    VLAN_CRIT_EXIT();

    // This will cause subscribers and switch(es) to get updated.
    vlan_bulk_update_end();

    // Takes some serious effort to make this function fail.
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_vlan_access_vids_set()
/******************************************************************************/
mesa_rc vtss_appl_vlan_access_vids_set(const u8 access_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES])
{
    u8                     old_access_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    vtss_appl_vlan_entry_t membership;
    mesa_vid_t             vid;
    mesa_rc                rc = VTSS_RC_OK;

    VTSS_RC(vtss_appl_vlan_access_vids_get(old_access_vids));

    (void)vlan_bulk_update_begin();

    // Create or delete VIDs as requested.
    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
        BOOL old_existed = VTSS_BF_GET(old_access_vids, vid);
        if (old_existed != VTSS_BF_GET(access_vids, vid)) {
            if (old_existed) {
                // Used to exist, but no longer does.
                if ((rc = vlan_mgmt_vlan_del(VTSS_ISID_GLOBAL, vid, VTSS_APPL_VLAN_USER_STATIC)) != VTSS_RC_OK) {
                    goto do_exit;
                }
            } else {
                // Didn't exist before. Create it.
                // It doesn't matter to fill in the actual membership ports, because the function
                // does that all by itself.
                membership.vid = vid;
                if ((rc = vlan_mgmt_vlan_add(VTSS_ISID_GLOBAL, &membership, VTSS_APPL_VLAN_USER_STATIC)) != VTSS_RC_OK) {
                    goto do_exit;
                }
            }
        }
    }

do_exit:
    (void)vlan_bulk_update_end();

    return rc;
}

/******************************************************************************/
// vlan_mgmt_membership_per_port_get()
/******************************************************************************/
mesa_rc vlan_mgmt_membership_per_port_get(vtss_isid_t isid, mesa_port_no_t port, vtss_appl_vlan_user_t ext_user, u8 vid_mask[VTSS_APPL_VLAN_BITMASK_LEN_BYTES])
{
    vtss_isid_t     zisid = isid - VTSS_ISID_START;
    mesa_vid_t      vid;
    vlan_user_int_t user;
    u32             multi_user_idx;
#if defined(VLAN_SAME_USER_SUPPORT)
    u32             same_user_idx;
#endif /* defined(VLAN_SAME_USER_SUPPORT) */
#if defined(VLAN_SINGLE_USER_SUPPORT)
    u32             single_user_idx;
#endif /* defined(VLAN_SINGLE_USER_SUPPORT) */

    if ((user = VLAN_user_ext_to_int(ext_user)) == VLAN_USER_INT_CNT) {
        return VLAN_ERROR_USER;
    }

    if (user > VLAN_USER_INT_ALL) {
        return VLAN_ERROR_USER;
    }

    VTSS_RC(VLAN_isid_port_check(isid, port, FALSE, TRUE));

    multi_user_idx  = VLAN_user_to_multi_idx(user);
#if defined(VLAN_SAME_USER_SUPPORT)
    same_user_idx   = VLAN_user_to_same_idx(user);
#endif /* defined(VLAN_SAME_USER_SUPPORT) */
#if defined(VLAN_SINGLE_USER_SUPPORT)
    single_user_idx = VLAN_user_to_single_idx(user);
#endif /* defined(VLAN_SINGLE_USER_SUPPORT) */

    memset(vid_mask, 0, VTSS_APPL_VLAN_BITMASK_LEN_BYTES);

    // Same and Single VLAN users can only have one bit set in the resulting array.
#if defined(VLAN_SAME_USER_SUPPORT)
    if (same_user_idx != VLAN_SAME_CNT) {
        VLAN_CRIT_ENTER();

        // "Same" users can only configure one single VLAN.
        vid = VLAN_same_table[same_user_idx].vid;

        // Always check in-use array, because the contents of VLAN_same_table[] may
        // not have been updated when a user unregistered himself.
        if (VLAN_IN_USE_ON_SWITCH_GET(isid, vid, user)) {
            VTSS_BF_SET(vid_mask, vid, TRUE);
        }

        VLAN_CRIT_EXIT();

        return VTSS_RC_OK;
    }
#endif /* defined(VLAN_SAME_USER_SUPPORT) */

#if defined(VLAN_SINGLE_USER_SUPPORT)
    if (single_user_idx != VLAN_SINGLE_CNT) {
        VLAN_CRIT_ENTER();

        // A "single" user is a user that can configure different ports to be members of different VLANs,
        // but at most one VLAN per port.
        vid = VLAN_single_table[single_user_idx].vid[zisid][port];

        // Always check in-use array, because the contents of VLAN_single_table[] may
        // not have been updated when a user unregistered himself.
        if (VLAN_IN_USE_ON_SWITCH_GET(isid, vid, user)) {
            VTSS_BF_SET(vid_mask, vid, TRUE);
        }

        VLAN_CRIT_EXIT();

        return VTSS_RC_OK;
    }
#endif /* defined(VLAN_SINGLE_USER_SUPPORT) */

    // If we get here, #user is either forbidden or multi-vlan-user
    VLAN_CRIT_ENTER();

    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
        if (!VLAN_IN_USE_ON_SWITCH_GET(isid, vid, user)) {
            continue;
        }

        if (user == VLAN_USER_INT_FORBIDDEN) {
            VTSS_BF_SET(vid_mask, vid, VLAN_forbidden_table[vid].ports[zisid][port]);
        } else if (multi_user_idx != VLAN_MULTI_CNT) {
            VTSS_BF_SET(vid_mask, vid, VLAN_multi_table[vid]->user_entries[multi_user_idx].ports[zisid][port]);
        } else {
            // VLAN_USER_INT_ALL
            VTSS_BF_SET(vid_mask, vid, VLAN_combined_table[vid].ports[zisid][port]);
        }
    }

    VLAN_CRIT_EXIT();

    return VTSS_RC_OK;
}

/******************************************************************************/
// vlan_mgmt_registration_per_port_get()
/******************************************************************************/
mesa_rc vlan_mgmt_registration_per_port_get(vtss_isid_t isid, mesa_port_no_t port, vlan_registration_type_t reg[VTSS_APPL_VLAN_ID_MAX + 1])
{
    mesa_rc    rc;
    u8         static_vid_mask[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    u8         forbidden_vid_mask[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    mesa_vid_t vid;

    // Let vlan_mgmt_membership_per_port_get() verify the arguments to this function.

    if ((rc = vlan_mgmt_membership_per_port_get(isid, port, VTSS_APPL_VLAN_USER_STATIC, static_vid_mask)) != VTSS_RC_OK) {
        return rc;
    }

    if ((rc = vlan_mgmt_membership_per_port_get(isid, port, VTSS_APPL_VLAN_USER_FORBIDDEN, forbidden_vid_mask)) != VTSS_RC_OK) {
        return rc;
    }

    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
        if (VTSS_BF_GET(forbidden_vid_mask, vid)) {
            reg[vid] = VLAN_REGISTRATION_TYPE_FORBIDDEN;
        } else {
            reg[vid] = VTSS_BF_GET(static_vid_mask, vid) ? VLAN_REGISTRATION_TYPE_FIXED : VLAN_REGISTRATION_TYPE_NORMAL;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_vlan_flooding_get()
/******************************************************************************/
mesa_rc vtss_appl_vlan_flooding_get(mesa_vid_t vid, mesa_bool_t *flooding)
{

    if (!fast_cap(VTSS_APPL_CAP_VLAN_FLOODING)) {
        return VLAN_ERROR_FLOODING_NOT_MANAGEABLE;
    }

    if (vid < VTSS_APPL_VLAN_ID_MIN || vid > VTSS_APPL_VLAN_ID_MAX) {
        return VLAN_ERROR_VID;
    }

    if (flooding == NULL) {
        return VLAN_ERROR_PARM;
    }

    VLAN_CRIT_ENTER();
    *flooding = VTSS_BF_GET(VLAN_flooding_vlans, vid);
    VLAN_CRIT_EXIT();

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_vlan_flooding_set()
/******************************************************************************/
mesa_rc vtss_appl_vlan_flooding_set(mesa_vid_t vid, mesa_bool_t flooding)
{
    mesa_rc rc;

    if (!fast_cap(VTSS_APPL_CAP_VLAN_FLOODING)) {
        return VLAN_ERROR_FLOODING_NOT_MANAGEABLE;
    }

    if (vid < VTSS_APPL_VLAN_ID_MIN || vid > VTSS_APPL_VLAN_ID_MAX) {
        return VLAN_ERROR_VID;
    }

    VLAN_CRIT_ENTER();
    rc = VLAN_flooding_set(vid, flooding);
    VLAN_CRIT_EXIT();

    return rc;
}

#if defined(VTSS_SW_OPTION_VLAN_NAMING)
/******************************************************************************/
// vtss_appl_vlan_name_get()
/******************************************************************************/
mesa_rc vtss_appl_vlan_name_get(mesa_vid_t vid, char name[VTSS_APPL_VLAN_NAME_MAX_LEN], BOOL *is_default)
{
    if (vid < VTSS_APPL_VLAN_ID_MIN || vid > VTSS_APPL_VLAN_ID_MAX) {
        return VLAN_ERROR_VID;
    }

    if (name == NULL) {
        return VLAN_ERROR_PARM;
    }

    VLAN_CRIT_ENTER();

    if (is_default) {
        // #is_default is optional and may be NULL.
        *is_default = VLAN_name_conf[vid][0] == '\0';
    }

    if (VLAN_name_conf[vid][0] == '\0') {
        if (vid == VTSS_APPL_VLAN_ID_DEFAULT) {
            strcpy(name, VTSS_APPL_VLAN_NAME_DEFAULT);
        } else {
            sprintf(name, "VLAN%04d", vid);
        }
    } else {
        strcpy(name, VLAN_name_conf[vid]);
    }

    VLAN_CRIT_EXIT();

    return VTSS_RC_OK;
}
#endif /* defined(VTSS_SW_OPTION_VLAN_NAMING) */

#if defined(VTSS_SW_OPTION_VLAN_NAMING)
/******************************************************************************/
// vtss_appl_vlan_name_set()
/******************************************************************************/
mesa_rc vtss_appl_vlan_name_set(mesa_vid_t vid, const char name[VTSS_APPL_VLAN_NAME_MAX_LEN])
{
    mesa_rc    rc        = VTSS_RC_OK;
    BOOL       set_empty = FALSE;
    mesa_vid_t v;
    u32        i, len;

    if (vid < VTSS_APPL_VLAN_ID_MIN || vid > VTSS_APPL_VLAN_ID_MAX) {
        return VLAN_ERROR_VID;
    }

    if (name == NULL) {
        return VLAN_ERROR_PARM;
    }

    len = strlen(name);
    if (len >= VTSS_APPL_VLAN_NAME_MAX_LEN) {
        return VLAN_ERROR_PARM;
    }

    for (i = 0; i < len; i++) {
        if (name[i] < 33 || name[i] > 126) {
            return VLAN_ERROR_NAME_INVALID;
        }
    }

    // Try to convert #name to a VID using default name rules.
    if ((v = VLAN_reserved_name_to_vid(name)) != VTSS_VID_NULL) {
        // #name matches a reserved name.
        if (v != vid) {
            // The VLAN name is reserved for another VID.
            return VLAN_ERROR_NAME_RESERVED;
        } else {
            // The name that is attempted set, corresponds to the default name of this VLAN.
            // Clear it.
            set_empty = TRUE;
        }
    }

    if (name[0] == '\0') {
        set_empty = TRUE;
    }

    if (!set_empty && vid == VTSS_APPL_VLAN_ID_DEFAULT) {
        // It's illegal to change the name of the default VLAN.
        // The default VLAN supports two "set"-names, namely "default" and "VLAN0001",
        // but it only supports one "get"-name, namely "default".
        return VLAN_ERROR_NAME_DEFAULT_VLAN;
    }

    VLAN_CRIT_ENTER();

    // It is only allowed to change the VLAN name for created access VLANs, but we allow
    // setting the VLAN's default name even if not created.
    if (set_empty) {
        // Back to default name
        VLAN_name_conf[vid][0] = '\0';
    } else if (VLAN_get(VTSS_ISID_GLOBAL, vid, VLAN_USER_INT_STATIC, FALSE, NULL) == vid) {
        // The VLAN is created as an access VLAN. Allow changing it.
        // Look through the VLAN_name_conf[] table to see if another VLAN has this name already.
        if ((v = VLAN_name_lookup(name)) == VTSS_VID_NULL) {
            // Not already found.
            strcpy(VLAN_name_conf[vid], name);
        } else if (v != vid) {
            // Already exists under another VLAN ID.
            rc = VLAN_ERROR_NAME_ALREADY_EXISTS;
        }
    } else {
        rc = VLAN_ERROR_NAME_VLAN_NOT_CREATED;
    }

    VLAN_CRIT_EXIT();

    return rc;
}
#endif /* defined(VTSS_SW_OPTION_VLAN_NAMING) */

#if defined(VTSS_SW_OPTION_VLAN_NAMING)
/******************************************************************************/
// vlan_mgmt_name_to_vid()
/******************************************************************************/
mesa_rc vlan_mgmt_name_to_vid(const char name[VTSS_APPL_VLAN_NAME_MAX_LEN], mesa_vid_t *vid)
{
    if (name == NULL || vid == NULL) {
        return VLAN_ERROR_PARM;
    }

    // Try to convert #name to a VID using reserved name rules.
    if ((*vid = VLAN_reserved_name_to_vid(name)) != VTSS_VID_NULL) {
        // It matched the reserved name for a VLAN.
        return VTSS_RC_OK;
    }

    // No match so far. Gotta look it up in the array of VLAN names.

    VLAN_CRIT_ENTER();
    *vid = VLAN_name_lookup(name);
    VLAN_CRIT_EXIT();

    return *vid == VTSS_VID_NULL ? (mesa_rc)VLAN_ERROR_NAME_DOES_NOT_EXIST : VTSS_RC_OK;
}
#endif /* defined(VTSS_SW_OPTION_VLAN_NAMING) */

/******************************************************************************/
// vlan_mgmt_port_conflicts_get()
// Get the current conflicts from VLAN port configuration database.
/******************************************************************************/
mesa_rc vlan_mgmt_port_conflicts_get(vtss_isid_t isid, mesa_port_no_t port, vlan_port_conflicts_t *conflicts)
{
    vtss_appl_vlan_port_detailed_conf_t dummy;

    VTSS_RC(VLAN_isid_port_check(isid, port, FALSE, TRUE));

    if (conflicts == NULL) {
        return VLAN_ERROR_PARM;
    }

    VLAN_CRIT_ENTER();
    (void)VLAN_port_conflict_resolver(isid, port, conflicts, &dummy);
    VLAN_CRIT_EXIT();

    return VTSS_RC_OK;
}

/******************************************************************************/
//
// Utility functions
//
/******************************************************************************/

/******************************************************************************/
// vlan_mgmt_vid_gets_tagged()
/******************************************************************************/
BOOL vlan_mgmt_vid_gets_tagged(vtss_appl_vlan_port_detailed_conf_t *p, mesa_vid_t vid)
{
    if (p == NULL || vid < VTSS_APPL_VLAN_ID_MIN || vid > VTSS_APPL_VLAN_ID_MAX) {
        T_E("Invalid params");
        return FALSE;
    }

    if (p->tx_tag_type  == VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL                             ||
        (p->tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS && p->untagged_vid == vid) ||
        (p->tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS   && p->pvid         == vid && p->untagged_vid != p->pvid)) {
        // Port is configured to either
        // 1) Untag all frames,
        // 2) Untag a particular VID, which happens to be the VID we're currently looking at.
        // 3) Tag a particular VID. Two cases to consider:
        //      If the VID that is requested tagged is the PVID, all frames are tagged.
        //      If the VID that is requested tagged is NOT the PVID, all frames but PVID are tagged.
        //      Therefore, if the VID we're looking at now is the PVID and the VID to tag is not the PVID,
        //      the VID we're looking at right now (the PVID) gets untagged.
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************/
// vlan_mgmt_vid_bitmask_to_txt()
// Inspired by mgmt_list2txt()
/******************************************************************************/
char *vlan_mgmt_vid_bitmask_to_txt(const u8 bitmask[VTSS_APPL_VLAN_BITMASK_LEN_BYTES], char *txt)
{
    mesa_vid_t vid;
    BOOL       member, first = TRUE;
    u32        count = 0;
    char       *p = txt;

    txt[0] = '\0';

    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
        member = VTSS_BF_GET(bitmask, vid);

        if ((member && (count == 0 || vid == VTSS_APPL_VLAN_ID_MAX)) || (!member && count > 1)) {
            p += sprintf(p, "%s%d",
                         first ? "" : count > (member ? 1 : 2) ? "-" : ",",
                         member ? vid : vid - 1);
            first = FALSE;
        }

        count = member ? count + 1 : 0;
    }

    return txt;
}

/******************************************************************************/
// vlan_mgmt_bitmasks_identical()
// This function relies heavily on the fact that VTSS_BF_GET()/VTSS_BF_SET()
// macros place e.g. index 0, 8, 16, 24 into bit 0 of a given byte,
// index 1, 9, 17, 25, etc. into bit 1 of a given byte, and so on.
/******************************************************************************/
BOOL vlan_mgmt_bitmasks_identical(u8 *bitmask1, u8 *bitmask2)
{
    mesa_vid_t vid;

    if (bitmask1 == NULL || bitmask2 == NULL) {
        T_E("Invalid argument(s)");
        return FALSE;
    }

    // Now be careful in the comparison.
    //
    // Bits from [0 to VTSS_APPL_VLAN_ID_MIN[ are not used, and should not
    // be part of the comparison. The following computes the number
    // of bytes to skip at the beginning of the bit array.
#define VLAN_HEAD_BYTES_TO_SKIP ((VTSS_APPL_VLAN_ID_MIN + 7) / 8)
    //
    // Also, bits from ]VTSS_APPL_VLAN_ID_MAX; next-multiple-of-8[ are also
    // not used, and should therefore not be part of the comparison.
    // If not all bits are used in the last byte, we skip the last byte
    // alltogether.
#if ((VTSS_APPL_VLAN_ID_MAX + 1) % 8) == 0
#define VLAN_TAIL_BYTES_TO_SKIP 0
#else
#define VLAN_TAIL_BYTES_TO_SKIP 1
#endif

#if VTSS_APPL_VLAN_BITMASK_LEN_BYTES > VLAN_HEAD_BYTES_TO_SKIP + VLAN_TAIL_BYTES_TO_SKIP
    // Only call memcmp() if the number of fully valid bytes is greater than zero.
    if (memcmp(bitmask1 + VLAN_HEAD_BYTES_TO_SKIP, bitmask2 + VLAN_HEAD_BYTES_TO_SKIP, VTSS_APPL_VLAN_BITMASK_LEN_BYTES - VLAN_HEAD_BYTES_TO_SKIP - VLAN_TAIL_BYTES_TO_SKIP) != 0) {
        return FALSE;
    }
#endif /* VTSS_APPL_VLAN_BITMASK_LEN_BYTES > VLAN_HEAD_BYTES_TO_SKIP + VLAN_TAIL_BYTES_TO_SKIP */

#if (VTSS_APPL_VLAN_ID_MIN % 8) != 0
    // VTSS_APPL_VLAN_ID_MIN is not located on bit 0 of a given byte.
    // Gotta check from [VTSS_APPL_VLAN_ID_MIN; next-byte-boundary[ manually
    for (vid = VTSS_APPL_VLAN_ID_MIN; vid < 8 * ((VTSS_APPL_VLAN_ID_MIN + 7) / 8); vid++) {
        if (VTSS_BF_GET(bitmask1, vid) != VTSS_BF_GET(bitmask2, vid)) {
            return FALSE;
        }
    }
#endif /* (VTSS_APPL_VLAN_ID_MIN % 8) != 0 */

#if VLAN_TAIL_BYTES_TO_SKIP != 0
    // VTSS_APPL_VLAN_ID_MAX is not located on bit 7 of a given byte.
    // Gotta check from ]prev-byte_boundary; VTSS_APPL_VLAN_ID_MAX] manually.
    for (vid = 8 * (VTSS_APPL_VLAN_ID_MAX / 8); vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
        if (VTSS_BF_GET(bitmask1, vid) != VTSS_BF_GET(bitmask2, vid)) {
            return FALSE;
        }
    }
#endif /* VLAN_TAIL_BYTES_TO_SKIP != 0 */

    return TRUE;
}

/******************************************************************************/
// vlan_mgmt_bitmask_all_ones_set()
/******************************************************************************/
mesa_rc vlan_mgmt_bitmask_all_ones_set(u8 bitmask[VTSS_APPL_VLAN_BITMASK_LEN_BYTES])
{
    mesa_vid_t vid;

    if (bitmask == NULL) {
        T_E("Invalid argument");
        return VLAN_ERROR_PARM;
    }

    memset(bitmask, 0xFF, VTSS_APPL_VLAN_BITMASK_LEN_BYTES);

    // Make sure we only set bits for valid VLANs, or e.g. JSON will
    // show an invalid range of VLANs.

    // Clear the leading bits in allowed VLANs.
    for (vid = VTSS_VID_NULL; vid < VTSS_APPL_VLAN_ID_MIN; vid++) {
        VTSS_BF_SET(bitmask, vid, 0);
    }

    // Clear the trailing bits in allowed VLANs.
    for (vid = VTSS_APPL_VLAN_ID_MAX + 1; vid < 8 * VTSS_APPL_VLAN_BITMASK_LEN_BYTES; vid++) {
        VTSS_BF_SET(bitmask, vid, 0);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vlan_mgmt_user_to_txt()
/******************************************************************************/
const char *vlan_mgmt_user_to_txt(vtss_appl_vlan_user_t ext_user)
{
    switch (ext_user) {
    case VTSS_APPL_VLAN_USER_STATIC:
        return "Admin";
#if defined(VTSS_SW_OPTION_DOT1X)
    case VTSS_APPL_VLAN_USER_DOT1X:
        return "NAS";
#endif /* defined(VTSS_SW_OPTION_DOT1X) */
#if defined(VTSS_SW_OPTION_MSTP)
    case VTSS_APPL_VLAN_USER_MSTP:
        return "MSTP";
#endif /* defined(VTSS_SW_OPTION_MSTP) */
#if defined(VTSS_SW_OPTION_MVRP)
    case VTSS_APPL_VLAN_USER_MVRP:
        return "MVRP";
#endif /* defined(VTSS_SW_OPTION_MVRP) */
#if defined(VTSS_SW_OPTION_GVRP)
    case VTSS_APPL_VLAN_USER_GVRP:
        return "GVRP";
#endif /* defined(VTSS_SW_OPTION_GVRP) */
#if defined(VTSS_SW_OPTION_MVR)
    case VTSS_APPL_VLAN_USER_MVR:
        return "MVR";
#endif /* defined(VTSS_SW_OPTION_MVR) */
#if defined(VTSS_SW_OPTION_VOICE_VLAN)
    case VTSS_APPL_VLAN_USER_VOICE_VLAN:
        return "Voice VLAN";
#endif /* defined(VTSS_SW_OPTION_VOICE_VLAN) */
#if defined(VTSS_SW_OPTION_ERPS)
    case VTSS_APPL_VLAN_USER_ERPS:
        return "ERPS";
#endif /* defined(VTSS_SW_OPTION_ERPS) */
#if defined(VTSS_SW_OPTION_IEC_MRP)
    case VTSS_APPL_VLAN_USER_IEC_MRP:
        return "MRP";
#endif /* defined(VTSS_SW_OPTION_IEC_MRP) */
#if defined(VTSS_SW_OPTION_VCL)
    case VTSS_APPL_VLAN_USER_VCL:
        return "VCL";
#endif /* defined(VTSS_SW_OPTION_VCL) */
#if defined(VTSS_SW_OPTION_RMIRROR)
    case VTSS_APPL_VLAN_USER_RMIRROR:
        return "RMirror";
#endif /* defined(VTSS_SW_OPTION_RMIRROR) */
    case VTSS_APPL_VLAN_USER_FORBIDDEN:
        return "Forbidden VLANs";
    case VTSS_APPL_VLAN_USER_ALL:
        return "Combined";
    case VTSS_APPL_VLAN_USER_LAST:
    default:
        T_E("Invoked with user = %u", ext_user);
        return "Unknown";
    }
}

/******************************************************************************/
// vlan_mgmt_port_type_to_txt()
/******************************************************************************/
const char *vlan_mgmt_port_type_to_txt(vtss_appl_vlan_port_type_t port_type)
{
    switch (port_type) {
    case VTSS_APPL_VLAN_PORT_TYPE_UNAWARE:
        return "Unaware";
    case VTSS_APPL_VLAN_PORT_TYPE_C:
        return "C-Port";
    case VTSS_APPL_VLAN_PORT_TYPE_S:
        return "S-Port";
    case VTSS_APPL_VLAN_PORT_TYPE_S_CUSTOM:
        return "S-Custom-Port";
    default:
        return "Unknown";
    }
}

/******************************************************************************/
// vlan_mgmt_frame_type_to_txt()
// Acceptable frame type.
/******************************************************************************/
const char *vlan_mgmt_frame_type_to_txt(mesa_vlan_frame_t frame_type)
{
    switch (frame_type) {
    case MESA_VLAN_FRAME_ALL:
        return "All";
    case MESA_VLAN_FRAME_TAGGED:
        return "Tagged";
    case MESA_VLAN_FRAME_UNTAGGED:
        return "Untagged";
    default:
        return "Unknown";
    }
}

/******************************************************************************/
// vlan_mgmt_tx_tag_type_to_txt()
/******************************************************************************/
const char *vlan_mgmt_tx_tag_type_to_txt(vtss_appl_vlan_tx_tag_type_t tx_tag_type, BOOL can_be_any_uvid)
{
    switch (tx_tag_type) {
    case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS:
        return can_be_any_uvid ? "Untag UVID" : "Untag PVID";
    case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS:
        return can_be_any_uvid ? "Tag UVID"   : "Tag PVID";
    case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL:
        return "Untag All";
    case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL:
        return "Tag All";
    }
    return "Unknown";
}

/******************************************************************************/
// vlan_mgmt_user_is_port_conf_changer()
/******************************************************************************/
BOOL vlan_mgmt_user_is_port_conf_changer(vtss_appl_vlan_user_t ext_user)
{
    switch (ext_user) {
    case VTSS_APPL_VLAN_USER_STATIC:
#if defined(VTSS_SW_OPTION_DOT1X)
    case VTSS_APPL_VLAN_USER_DOT1X:
#endif /* defined(VTSS_SW_OPTION_DOT1X) */
#if defined(VTSS_SW_OPTION_MVR)
    case VTSS_APPL_VLAN_USER_MVR:
#endif /* defined(VTSS_SW_OPTION_MVR) */
#if defined(VTSS_SW_OPTION_VOICE_VLAN)
    case VTSS_APPL_VLAN_USER_VOICE_VLAN:
#endif /* defined(VTSS_SW_OPTION_VOICE_VLAN) */
#if defined(VTSS_SW_OPTION_MSTP)
    case VTSS_APPL_VLAN_USER_MSTP:
#endif /* defined(VTSS_SW_OPTION_MSTP) */
#if defined(VTSS_SW_OPTION_ERPS)
    case VTSS_APPL_VLAN_USER_ERPS:
#endif /* defined(VTSS_SW_OPTION_ERPS) */
#if defined(VTSS_SW_OPTION_IEC_MRP)
    case VTSS_APPL_VLAN_USER_IEC_MRP:
#endif /* defined(VTSS_SW_OPTION_IEC_MRP) */
#if defined(VTSS_SW_OPTION_VCL)
    case VTSS_APPL_VLAN_USER_VCL:
#endif /* defined(VTSS_SW_OPTION_VCL) */
#if defined(VTSS_SW_OPTION_GVRP)
    case VTSS_APPL_VLAN_USER_GVRP:
#endif /* defined(VTSS_SW_OPTION_GVRP) */
#if defined(VTSS_SW_OPTION_RMIRROR)
    case VTSS_APPL_VLAN_USER_RMIRROR:
#endif /* defined(VTSS_SW_OPTION_RMIRROR) */
        return TRUE;

    default:
        return FALSE;
    }
}

/******************************************************************************/
// vlan_mgmt_is_membership_changer()
/******************************************************************************/
BOOL vlan_mgmt_user_is_membership_changer(vtss_appl_vlan_user_t ext_user)
{
    vlan_user_int_t user;

    if (ext_user == VTSS_APPL_VLAN_USER_FORBIDDEN) {
        return TRUE; // Allowed.
    }

    if ((user = VLAN_user_ext_to_int(ext_user)) == VLAN_USER_INT_CNT) {
        return FALSE;
    }

    if (VLAN_user_to_multi_idx(user) != VLAN_MULTI_CNT) {
        return TRUE; // Allowed.
    }

#if defined(VLAN_SAME_USER_SUPPORT)
    if (VLAN_user_to_same_idx(user) != VLAN_SAME_CNT) {
        return TRUE; // Allowed.
    }
#endif /* defined(VLAN_SAME_USER_SUPPORT) */

#if defined(VLAN_SINGLE_USER_SUPPORT)
    if (VLAN_user_to_single_idx(user) != VLAN_SINGLE_CNT) {
        return TRUE; // Allowed.
    }
#endif /* defined(VLAN_SINGLE_USER_SUPPORT) */

    // Wasn't any of the allowed users.
    return FALSE;
}

/******************************************************************************/
// vtss_appl_vlan_fid_get()
/******************************************************************************/
mesa_rc vtss_appl_vlan_fid_get(mesa_vid_t vid, mesa_vid_t *fid)
{
    if (vid < VTSS_APPL_VLAN_ID_MIN || vid > VTSS_APPL_VLAN_ID_MAX) {
        return VLAN_ERROR_VID;
    }

    if (fid == NULL) {
        return VLAN_ERROR_PARM;
    }

    if (!msg_switch_is_primary()) {
        return VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if (VTSS_APPL_VLAN_FID_CNT) {
        VLAN_CRIT_ENTER();
        *fid = VLAN_FID_GET(vid);
        VLAN_CRIT_EXIT();
        return VTSS_RC_OK;
    } else {
        // Not supported.
        *fid = 0;
        return VTSS_RC_ERROR;
    }
}

/******************************************************************************/
// vtss_appl_vlan_fid_set()
/******************************************************************************/
mesa_rc vtss_appl_vlan_fid_set(mesa_vid_t vid, mesa_vid_t fid)
{
    if (vid < VTSS_APPL_VLAN_ID_MIN || vid > VTSS_APPL_VLAN_ID_MAX) {
        return VLAN_ERROR_VID;
    }

    if (fid > VTSS_APPL_VLAN_FID_CNT) {
        return VLAN_ERROR_FID;
    }

    if (!msg_switch_is_primary()) {
        return VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if (VTSS_APPL_VLAN_FID_CNT) {
        BOOL update = FALSE;

        if (fid == vid) {
            // Special case. When the user wants a one-to-one correspondance
            // between VID and FID, we use 0. This is useful if the number
            // of FIDs on this platform is lower than the number of VIDs.
            fid = 0;
        }

        VLAN_CRIT_ENTER();

        if (VLAN_FID_GET(vid) != fid) {
            VLAN_FID_SET(vid, fid);
            update = TRUE;
        }

        VLAN_CRIT_EXIT();

        if (update) {
            // Send the configuration to all switches in the stack
            VLAN_msg_tx_vid_to_fid(VTSS_ISID_GLOBAL, vid, fid);
        }
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

/******************************************************************************/
// vtss_appl_vlan_capabilities_get()
/******************************************************************************/
mesa_rc vtss_appl_vlan_capabilities_get(vtss_appl_vlan_capabilities_t *cap)
{
    if (!cap) {
        return VLAN_ERROR_PARM;
    }

    cap->vlan_id_min  = VTSS_APPL_VLAN_ID_MIN;
    cap->vlan_id_max  = VTSS_APPL_VLAN_ID_MAX;
    cap->fid_cnt      = VTSS_APPL_VLAN_FID_CNT;
    cap->has_flooding = fast_cap(VTSS_APPL_CAP_VLAN_FLOODING);
    return VTSS_RC_OK;
}

u32 vlan_user_int_cnt(void)
{
    return VLAN_USER_INT_CNT;
}

/******************************************************************************/
// vtss_appl_vlan_port_detailed_conf_t::operator<<()
// Used for tracing.
// Traces vtss_appl_vlan_port_conf_t::hybrid.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_vlan_port_detailed_conf_t &conf)
{
    o << "{pvid = "            << conf.pvid
      << ", untagged_vid = "   << conf.untagged_vid
      << ", port_type = "      << conf.port_type
      << ", ingress_filter = " << conf.ingress_filter
      << ", frame_type = "     << conf.frame_type
      << ", tx_tag_type = "    << conf.tx_tag_type
      << ", flags = 0x"        << vtss::hex(conf.flags)
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_vlan_port_detailed_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_vlan_port_detailed_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}


/******************************************************************************/
// Pre-declaration of MIB registration function.
/******************************************************************************/
#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
VTSS_PRE_DECLS void vlan_mib_init(void);
#endif /* defined(VTSS_SW_OPTION_PRIVATE_MIB) */
#if defined(VTSS_SW_OPTION_JSON_RPC)
VTSS_PRE_DECLS void vtss_appl_vlan_json_init(void);
#endif /* defined(VTSS_SW_OPTION_PRIVATE_MIB) */
extern "C" int vlan_icli_cmd_register();

/******************************************************************************/
// vlan_init()
// Initialize module.
/******************************************************************************/
mesa_rc vlan_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
    mesa_rc     rc   = VTSS_RC_OK;

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);

    switch (data->cmd) {
    case INIT_CMD_INIT:

        // Create mutexes for critical regions
        critd_init(&VLAN_crit,    "vlan",    VTSS_MODULE_ID_VLAN, CRITD_TYPE_MUTEX);
        critd_init(&VLAN_cb_crit, "vlan.cb", VTSS_MODULE_ID_VLAN, CRITD_TYPE_MUTEX);

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
        vlan_mib_init();
#endif /* defined(VTSS_SW_OPTION_PRIVATE_MIB) */
#if defined(VTSS_SW_OPTION_JSON_RPC)
        vtss_appl_vlan_json_init();
#endif /* defined(VTSS_SW_OPTION_PRIVATE_MIB) */
        vlan_icli_cmd_register();

#if defined(VTSS_SW_OPTION_ICFG)
        if ((rc = VLAN_icfg_init()) != VTSS_RC_OK) {
            T_D("Calling vlan_icfg_init() failed rc = %s", error_txt(rc));
        }
#endif /* defined(VTSS_SW_OPTION_ICFG) */
        break;

    case INIT_CMD_START:

        // Stitch together the VLAN table.
        VLAN_table_init();

        // Initialize VLAN port configuration. This one takes the VLAN crit itself.
        VLAN_port_default(VTSS_ISID_GLOBAL);

        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (VTSS_ISID_LEGAL(isid) || isid == VTSS_ISID_GLOBAL) {
            VLAN_default(isid);
        }
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");
        VLAN_default(VTSS_ISID_GLOBAL);
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");

        // Apply all configuration to switch.
        VLAN_msg_tx_port_conf_all(isid);
        VLAN_msg_tx_membership_all(isid);
        {
            mesa_etype_t tpid;

            VLAN_CRIT_ENTER();
            tpid = VLAN_tpid_s_custom_port;
            VLAN_CRIT_EXIT();

            VLAN_msg_tx_tpid_conf(tpid);
        }

        if (VTSS_APPL_VLAN_FID_CNT) {
            mesa_vid_t vid, fid;

            VLAN_CRIT_ENTER();
            VLAN_msg_tx_fid_table_reset(isid);
            for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
                if ((fid = VLAN_FID_GET(vid)) != 0) {
                    VLAN_msg_tx_vid_to_fid(isid, vid, fid);
                }
            }
            VLAN_CRIT_EXIT();
        }
        break;

    default:
        break;
    }

    return rc;
}

