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

#include <vtss/appl/cfm.hxx>      /* For ourselves (public header)           */
#include "cfm_api.h"              /* For ourselves (semi-public header)      */
#include "cfm_trace.h"            /* For our own trace definitions           */
#include "cfm_expose.hxx"         /* For our own notifications               */
#include "cfm_base.hxx"           /* For ourselves (base-impl)               */
#include "cfm_lock.hxx"           /* For CFM_LOCK_SCOPE()                    */
#include "critd_api.h"            /* For mutex wrapper                       */
#include "ip_utils.hxx"           /* For tracing of vtss_appl_ip_if_status_t */
#include "misc_api.h"             /* For misc_mac_txt()                      */
#include "vlan_api.h"             /* For vlan_mgmt_XXX()                     */
#include "conf_api.h"             /* For conf_mgmt_mac_addr_get()            */
#include "port_api.h"             /* For port_change_register()              */
#include "l2proto_api.h"          /* For l2port2port()                       */
#include "interrupt_api.h"        /* For vtss_interrupt_source_hook_set()    */
#include "web_api.h"              /* For webCommonBufferHandler()            */
#include <vtss/appl/vlan.h>       /* For VTSS_APPL_VLAN_ID_MIN/MAX           */
#include <vtss/basics/stream.hxx> /* For vtss::ostream                       */
#include <vtss/basics/trace.hxx>  /* For VTSS_TRACE()                        */

//*****************************************************************************/
// Trace definitions
/******************************************************************************/
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "cfm", "Connectivity Fault Management"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [CFM_TRACE_GRP_BASE] = {
        "base",
        "Base",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [CFM_TRACE_GRP_CCM] = {
        "ccm",
        "CCM",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [CFM_TRACE_GRP_TIMER] = {
        "timers",
        "Timers",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [CFM_TRACE_GRP_CALLBACK] = {
        "callback",
        "Callbacks",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [CFM_TRACE_GRP_FRAME_RX] = {
        "rx",
        "Frame Rx",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [CFM_TRACE_GRP_FRAME_TX] = {
        "tx",
        "Frame Tx",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [CFM_TRACE_GRP_ICLI] = {
        "icli",
        "ICLI",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [CFM_TRACE_GRP_NOTIF] = {
        "notif",
        "Notifications",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/******************************************************************************/
// Global variables
/******************************************************************************/
critd_t CFM_crit;

// cfm_mep_notification_status holds the per-MEP state that one can get
// notifications on, that being SNMP traps or JSON notifications. Each row in
// this table is a struct of type vtss_appl_cfm_mep_notification_status_t.
cfm_mep_notification_status_t cfm_mep_notification_status("cfm_mep_notification_status", VTSS_MODULE_ID_CFM);

typedef vtss::Map<vtss_appl_cfm_mep_key_t, cfm_mep_conf_t> cfm_mep_conf_map_t;
typedef cfm_mep_conf_map_t::iterator cfm_mep_conf_itr_t;

typedef struct {
    vtss_appl_cfm_ma_conf_t ma_conf;
    cfm_mep_conf_map_t      mep_confs;
} cfm_ma_conf_t;

typedef vtss::Map<vtss_appl_cfm_ma_key_t, cfm_ma_conf_t> cfm_ma_conf_map_t;
typedef cfm_ma_conf_map_t::iterator cfm_ma_conf_itr_t;

typedef struct {
    vtss_appl_cfm_md_conf_t md_conf;
    cfm_ma_conf_map_t       ma_confs;
} cfm_md_conf_t;

typedef vtss::Map<vtss_appl_cfm_md_key_t, cfm_md_conf_t> cfm_md_conf_map_t;
typedef cfm_md_conf_map_t::iterator cfm_md_conf_itr_t;

static uint32_t                               CFM_cap_port_cnt;
static mesa_etype_t                           CFM_s_custom_tpid;
static vtss_appl_cfm_capabilities_t           CFM_cap;
static vtss_appl_cfm_global_conf_t            CFM_global_conf;
static cfm_md_conf_map_t                      CFM_md_conf_map;
cfm_mep_state_map_t                           CFM_mep_state_map; // Used by base as well
CapArray<cfm_port_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> CFM_port_state;    // Used by base as well
cfm_global_state_t                            CFM_global_state; // Used by base as well
static bool                                   CFM_started;

static struct {
    vtss_module_id_t module_id;
    void             (*callback)(void);
} CFM_conf_change_callbacks[3];

/******************************************************************************/
// CFM_css_display_none()
// Creates "display:none" variables used in HTML help.
/******************************************************************************/
static size_t CFM_css_display_none(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buf[512];

    if (CFM_global_state.has_vop_v0 || CFM_global_state.has_vop_v1) {
        // LAN966x, Serval-1, and Ocelot have Shared MEL, so hide independent
        // MEL classes.
        (void)snprintf(buf, sizeof(buf), ".cfm_has_independent_mel {display:none;}\r\n");
    } else {
        // All other chips have independent MEL, so hide shared MEL classes.
        (void)snprintf(buf, sizeof(buf), ".cfm_has_shared_mel {display:none;}\r\n");
    }

    return webCommonBufferHandler(base_ptr, cur_ptr, length, buf);
}

/******************************************************************************/
// Install our CSS function in the global array of CSS generators.
/******************************************************************************/
web_lib_filter_css_tab_entry(CFM_css_display_none);

/******************************************************************************/
// vtss_appl_cfm_md_key_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_cfm_md_key_t &md_key)
{
    o << md_key.md;
    return o;
}

/******************************************************************************/
// vtss_appl_cfm_md_key_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_cfm_md_key_t *md_key)
{
    o << *md_key;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_cfm_ma_key_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_cfm_ma_key_t &ma_key)
{
    o << ma_key.md << "::" << ma_key.ma;
    return o;
}

/******************************************************************************/
// vtss_appl_cfm_ma_key_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_cfm_ma_key_t *ma_key)
{
    o << *ma_key;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_cfm_mep_key_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_cfm_mep_key_t &mep_key)
{
    o << mep_key.md << "::" << mep_key.ma << "::" << mep_key.mepid;
    return o;
}

/******************************************************************************/
// vtss_appl_cfm_mep_key_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_cfm_mep_key_t *mep_key)
{
    o << *mep_key;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_cfm_rmep_key_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_cfm_rmep_key_t &rmep_key)
{
    o << rmep_key.md << "::" << rmep_key.ma << "::" << rmep_key.mepid << "::" << rmep_key.rmepid;
    return o;
}

/******************************************************************************/
// vtss_appl_cfm_rmep_key_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_cfm_rmep_key_t *rmep_key)
{
    o << *rmep_key;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_cfm_md_key_t::operator<
// Used for sorting the MD configuration map
/******************************************************************************/
static bool operator<(const vtss_appl_cfm_md_key_t &lhs, const vtss_appl_cfm_md_key_t &rhs)
{
    // Sort by md
    return lhs.md < rhs.md;
}

/******************************************************************************/
// vtss_appl_cfm_ma_key_t::operator<
// Used for sorting the MA configuration map
/******************************************************************************/
static bool operator<(const vtss_appl_cfm_ma_key_t &lhs, const vtss_appl_cfm_ma_key_t &rhs)
{
    // First sort by md
    if (lhs.md != rhs.md) {
        return lhs.md < rhs.md;
    }

    // Then by ma
    return lhs.ma < rhs.ma;
}

/******************************************************************************/
// vtss_appl_cfm_mep_key_t::operator<
// Used for sorting the MEP configuration and state maps
/******************************************************************************/
bool operator<(const vtss_appl_cfm_mep_key_t &lhs, const vtss_appl_cfm_mep_key_t &rhs)
{
    // First sort by md
    if (lhs.md != rhs.md) {
        return lhs.md < rhs.md;
    }

    // Then sort by ma
    if (lhs.ma != rhs.ma) {
        return lhs.ma < rhs.ma;
    }

    // Finally by mepid
    return lhs.mepid < rhs.mepid;
}

/******************************************************************************/
// vtss_appl_cfm_mep_key_t::operator==
// Used for figuring out whether to MEP keys are identical.
/******************************************************************************/
bool operator==(const vtss_appl_cfm_mep_key_t &lhs, const vtss_appl_cfm_mep_key_t &rhs)
{
    if (lhs.md != rhs.md) {
        return false;
    }

    if (lhs.ma != rhs.ma) {
        return false;
    }

    return lhs.mepid == rhs.mepid;
}

/******************************************************************************/
// vtss_appl_cfm_mep_key_t::operator!=
// Used for figuring out whether to MEP keys are different.
/******************************************************************************/
bool operator!=(const vtss_appl_cfm_mep_key_t &lhs, const vtss_appl_cfm_mep_key_t &rhs)
{
    return !(lhs == rhs);
}

/******************************************************************************/
// vtss_appl_cfm_rmep_key_t::operator<
// Used for sorting the RMEP configuration
/******************************************************************************/
static bool operator<(const vtss_appl_cfm_rmep_key_t &lhs, const vtss_appl_cfm_rmep_key_t &rhs)
{
    // First sort by md
    if (lhs.md != rhs.md) {
        return lhs.md < rhs.md;
    }

    // Then sort by ma
    if (lhs.ma != rhs.ma) {
        return lhs.ma < rhs.ma;
    }

    // Then by mepid
    if (lhs.mepid != rhs.mepid) {
        return lhs.mepid < rhs.mepid;
    }

    // Finally by rmepid
    return lhs.rmepid < rhs.rmepid;
}

/******************************************************************************/
// CFM_capabilities_set()
/******************************************************************************/
static void CFM_capabilities_set(void)
{
    uint32_t chip_family = fast_cap(MESA_CAP_MISC_CHIP_FAMILY);

    CFM_cap.md_cnt_max          = 50; // Arbitrarily chosen.
    CFM_cap.ma_cnt_max          = 10; // Arbitrarily chosen
    CFM_cap.rmep_cnt_max        =  1; // Currently we only support one single Remote MEP

    CFM_cap.mep_cnt_port_max    = MIN(fast_cap(MESA_CAP_VOP_PORT_VOE_CNT), CFM_cap_port_cnt);
    CFM_cap.mep_cnt_service_max = fast_cap(MESA_CAP_VOP_PATH_SERVICE_VOE_CNT);

    // LAN966x doesn't have VLAN Down-MEPs, but all other platforms do,
    // including those without a VOE.
    CFM_cap.has_vlan_meps       = !fast_cap(MESA_CAP_VOP_V0);

    if (CFM_cap.mep_cnt_port_max == 0) {
        // Caracal doesn't have H/W VOE-support, so we pick an (almost)
        // arbitrary number for the number of port MEPs that can be created.
        CFM_cap.mep_cnt_port_max = CFM_cap_port_cnt;
    }

    if (CFM_cap.mep_cnt_service_max == 0) {
        // Caracal doesn't have H/W VOE-support, so we pick an arbitrary number
        // for the number of service MEPs that can be created.
        CFM_cap.mep_cnt_service_max = 16;
    }

    switch (chip_family) {
    case MESA_CHIP_FAMILY_CARACAL:
        // Caracal doesn't have H/W-assisted CCM support, so everything needs to
        // be done in S/W, so we cannot support rates faster than 1 sec.
        CFM_cap.ccm_interval_min = VTSS_APPL_CFM_CCM_INTERVAL_1S;
        CFM_cap.ccm_interval_max = VTSS_APPL_CFM_CCM_INTERVAL_10MIN;
        break;

    case MESA_CHIP_FAMILY_OCELOT:
    case MESA_CHIP_FAMILY_LAN966X:
        // Serval-1 and Ocelot do have H/W-support, so it supports the fastest
        // rates. However, the H/W-support stops at 1 sec rates. In the future,
        // we might support slower rates through S/W-support, but for now, we
        // stop at 1sec rates.
        CFM_cap.ccm_interval_min = VTSS_APPL_CFM_CCM_INTERVAL_300HZ;
        CFM_cap.ccm_interval_max = VTSS_APPL_CFM_CCM_INTERVAL_1S;
        break;

    case MESA_CHIP_FAMILY_SERVALT:
    case MESA_CHIP_FAMILY_JAGUAR2:
    case MESA_CHIP_FAMILY_SPARX5:
    case MESA_CHIP_FAMILY_LAN969X:
        // JR2 and later also have H/W-support, so they support the fastest
        // rates as well. However, the H/W-support stops at 10 sec rates. In the
        // future, we might support slower rates through S/W-support, but for
        // now, we stop at 10sec rates.
        CFM_cap.ccm_interval_min = VTSS_APPL_CFM_CCM_INTERVAL_300HZ;
        CFM_cap.ccm_interval_max = VTSS_APPL_CFM_CCM_INTERVAL_10S;
        break;

    default:
        T_E("Unsupported chip family (%d)", chip_family);
        break;
    }

    CFM_cap.has_mips             = false;
    CFM_cap.has_up_meps          = false;
    CFM_cap.has_shared_meg_level = fast_cap(MESA_CAP_VOP_V1);

    T_D("md_cnt_max = %u, ma_cnt_max = %u, mep_cnt_port_max = %u, mep_cnt_service_max = %u, rmep_cnt_max = %u, ccm_interval_min = %s, ccm_interval_max = %s, has_mips = %d, has_up_meps = %d, has_shared_meg_level = %d",
        CFM_cap.md_cnt_max,
        CFM_cap.ma_cnt_max,
        CFM_cap.mep_cnt_port_max,
        CFM_cap.mep_cnt_service_max,
        CFM_cap.rmep_cnt_max,
        cfm_util_ccm_interval_to_str(CFM_cap.ccm_interval_min),
        cfm_util_ccm_interval_to_str(CFM_cap.ccm_interval_max),
        CFM_cap.has_mips,
        CFM_cap.has_up_meps,
        CFM_cap.has_shared_meg_level);
}

/******************************************************************************/
// CFM_ptr_check()
/******************************************************************************/
static mesa_rc CFM_ptr_check(const void *ptr)
{
    return ptr == nullptr ? (mesa_rc)VTSS_APPL_CFM_RC_INVALID_ARGUMENT : VTSS_RC_OK;
}

/******************************************************************************/
// CFM_key_check()
/******************************************************************************/
static mesa_rc CFM_key_check(const vtss_appl_cfm_name_key_t &key)
{
    size_t len, i;

    len = key.length();

    if (len < 1 || len > VTSS_APPL_CFM_KEY_LEN_MAX) {
        return VTSS_APPL_CFM_RC_INVALID_NAME_KEY_LENGTH;
    }

    // First character must be [a-zA-Z]
    if (!isalpha(key[0])) {
        return VTSS_APPL_CFM_RC_INVALID_NAME_KEY_CONTENTS;
    }

    for (i = 1; i < len; i++) {
        if (!isgraph(key[i])) {
            return VTSS_APPL_CFM_RC_INVALID_NAME_KEY_CONTENTS;
        }

        if (key[i] == ':') {
            return VTSS_APPL_CFM_RC_INVALID_NAME_KEY_CONTENTS_COLON;
        }
    }

    // "all" is reserved in ICLI. Using case-insensitive match, because ICLI is
    // case-insensitive.
    if (strcasecmp(key.c_str(), "all") == 0) {
        return VTSS_APPL_CFM_RC_INVALID_NAME_KEY_CONTENTS_ALL;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_mepid_check()
/******************************************************************************/
static mesa_rc CFM_mepid_check(vtss_appl_cfm_mepid_t mepid)
{
    if (mepid < 1 || mepid > 8191) {
        return VTSS_APPL_CFM_RC_MEP_INVALID_MEPID;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_key_check()
/******************************************************************************/
static mesa_rc CFM_key_check(const vtss_appl_cfm_md_key_t &key)
{
    return CFM_key_check(key.md);
}

/******************************************************************************/
// CFM_key_check()
/******************************************************************************/
static mesa_rc CFM_key_check(const vtss_appl_cfm_ma_key_t &key)
{
    VTSS_RC(CFM_key_check(static_cast<vtss_appl_cfm_md_key_t>(key)));
    return CFM_key_check(key.ma);
}

/******************************************************************************/
// CFM_key_check()
/******************************************************************************/
static mesa_rc CFM_key_check(const vtss_appl_cfm_mep_key_t &key)
{
    VTSS_RC(CFM_key_check(static_cast<vtss_appl_cfm_ma_key_t>(key)));
    return CFM_mepid_check(key.mepid);
}

/******************************************************************************/
// CFM_key_check()
/******************************************************************************/
static mesa_rc CFM_key_check(const vtss_appl_cfm_rmep_key_t &key)
{
    VTSS_RC(CFM_key_check(static_cast<vtss_appl_cfm_mep_key_t>(key)));
    return CFM_mepid_check(key.rmepid);
}

/******************************************************************************/
// CFM_maid_verify()
/******************************************************************************/
mesa_rc CFM_maid_verify(vtss_appl_cfm_md_conf_t *md_conf, vtss_appl_cfm_ma_conf_t *ma_conf, bool md_changed)
{
    uint32_t ma_len_max = 0, ma_len_req = 45;

    // The two Y.1731 formats can only be used with VTSS_APPL_CFM_MD_FORMAT_NONE
    if (ma_conf->format == VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC || ma_conf->format == VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC_CC) {
        if (md_conf->format != VTSS_APPL_CFM_MD_FORMAT_NONE) {
            return md_changed ? VTSS_APPL_CFM_RC_MD_Y1731_FORMAT : VTSS_APPL_CFM_RC_MA_Y1731_FORMAT;
        }
    }

    switch (md_conf->format) {
    case VTSS_APPL_CFM_MD_FORMAT_NONE:
        ma_len_max = 45;
        break;

    case VTSS_APPL_CFM_MD_FORMAT_STRING:
        ma_len_max = 44 - strlen(md_conf->name); // One less because of MD length field
        break;
    }

    switch (ma_conf->format) {
    case VTSS_APPL_CFM_MA_FORMAT_TWO_OCTET_INTEGER:
    case VTSS_APPL_CFM_MA_FORMAT_PRIMARY_VID:
        ma_len_req = 2;
        break;

    case VTSS_APPL_CFM_MA_FORMAT_STRING:
    case VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC:
    case VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC_CC:
        ma_len_req = strlen(ma_conf->name);
        break;
    }

    if (ma_len_req > ma_len_max) {
        return md_changed ? VTSS_APPL_CFM_RC_MD_MAID_TOO_LONG : VTSS_APPL_CFM_RC_MA_MAID_TOO_LONG;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_md_conf_t::operator!=
/******************************************************************************/
static bool operator!=(const vtss_appl_cfm_md_conf_t &lhs, const vtss_appl_cfm_md_conf_t &rhs)
{
    return lhs.format != rhs.format || lhs.level != rhs.level || strcmp(lhs.name, rhs.name) != 0;
}

/******************************************************************************/
// vtss_appl_cfm_ma_conf_t::operator!=
/******************************************************************************/
static bool operator!=(const vtss_appl_cfm_ma_conf_t &lhs, const vtss_appl_cfm_ma_conf_t &rhs)
{
    // This operator only checks the key-fields of the configuration, which are
    //   - format
    //   - name (which can be binary, so do not use strcmp())
    return lhs.format != rhs.format || memcmp(lhs.name, rhs.name, sizeof(lhs.name)) != 0;
}

/******************************************************************************/
// cfm_util_mep_state_change_to_str()
/******************************************************************************/
const char *cfm_util_mep_state_change_to_str(cfm_mep_state_change_t change)
{
    switch (change) {
    case CFM_MEP_STATE_CHANGE_CONF:
        return "Configuration";

    case CFM_MEP_STATE_CHANGE_CONF_NO_RESET:
        return "Configuration no reset";

    case CFM_MEP_STATE_CHANGE_CONF_RMEP:
        return "Remote MEP";

    case CFM_MEP_STATE_CHANGE_IP_ADDR:
        return "IP address";

    case CFM_MEP_STATE_CHANGE_TPID:
        return "TPID";

    case CFM_MEP_STATE_CHANGE_PORT_TYPE:
        return "VLAN Port Type";

    case CFM_MEP_STATE_CHANGE_PVID_OR_ACCEPTABLE_FRAME_TYPE:
        return "PVID or Acceptable Frame Type";

    case CFM_MEP_STATE_CHANGE_ENABLE_RMEP_DEFECT:
        return "enableRmepDefect change";

    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_PERIOD:
        return "CCM Period VOE Event";

    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_ZERO_PERIOD:
        return "CCM Zero Period VOE Event";

    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_PRIORITY:
        return "CCM Priority VOE Event";

    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_LOC:
        return "CCM LoC VOE Event";

    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_MEPID:
        return "CCM MEP-ID VOE Event";

    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_MAID:
        return "CCM MAID VOE Event";

    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_LEVEL:
        return "CCM Level VOE Event";

    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_RDI:
        return "CCM RDI VOE Event";

    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_SRC_PORT_MOVE:
        return "CCM Src Port Move VOE Event";

    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_TLV_PORT_STATUS:
        return "CCM Port Status TLV VOE Event";

    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_TLV_IF_STATUS:
        return "CCM IF Status TLV VOE Event";

    default:
        T_E("Internal error: Update %s for unknown change reason (%d)", __FUNCTION__, change);
        return "Unknown reason";
    }
}

/******************************************************************************/
// CFM_mep_update()
/******************************************************************************/
static void CFM_mep_update(cfm_mep_state_itr_t mep_state_itr, cfm_mep_state_change_t change)
{
    cfm_mep_state_t *mep_state     = &mep_state_itr->second;
    bool            old_mep_active = mep_state->status.mep_active;
    mesa_rc         rc;

    if (!CFM_started) {
        T_I("MEP %s: Deferring base update until INIT_CMD_ICFG_LOADING_POST event", mep_state->key);
        return;
    }

    rc = cfm_base_mep_update(mep_state, change);

    // Do not use T_E() if rc != VTSS_RC_OK, because cfm_base_mep_update()
    // uses the return code to say that not everything is OK, but that the real
    // problem can be found in mep_state_itr->status.errors.
    // Besides, if it's a code-problem, a trace error is issued inside the call
    // already.
    T_I("MEP %s: Update done because of change in %s. mep_active: %d -> %d, rc = %s",
        mep_state_itr->first,
        cfm_util_mep_state_change_to_str(change),
        old_mep_active,
        mep_state->status.mep_active,
        error_txt(rc));
}

/******************************************************************************/
// CFM_mep_config_error_update()
/******************************************************************************/
static void CFM_mep_config_error_update(cfm_mep_state_itr_t mep_state_itr, bool create_event = true)
{
    cfm_mep_state_t            *mep_state = &mep_state_itr->second;
    vtss_appl_cfm_mep_errors_t *errors    = &mep_state->status.errors;
    mesa_port_list_t           vlan_port_members;
    mesa_stp_state_t           stp_status;
    mesa_msti_t                msti;
    mesa_npi_conf_t            npi_conf;
    mesa_vid_t                 classified_vid;
    mesa_port_no_t             port_no, mirror_port_no;
    bool                       old_enableRmepDefect, old_mep_creatable;
    mesa_rc                    rc;

    if (!mep_state->mep_conf) {
        // The MEP is about to be deleted. Mark it as uncreatable before telling
        // the base about it.
        errors->mep_creatable = false;
        return;
    }

    // First figure out whether the MEP is creatable.
    classified_vid = mep_state->classified_vid_get();
    port_no        = mep_state->port_state->port_no;

    // If the MEP doesn't have any Remote MEPs created, the MEP is not
    // creatable.
    errors->no_rmeps = mep_state->mep_conf->rmep_confs.size() == 0;

    // If the MEP has direction up and the MA dictates port MEPs, then the MEP
    // is not creatable, because we cannot create port Up-MEPs according to
    // 802.1Q-2018, 22.2.2, table 22-1.
    errors->port_up_mep = mep_state->is_port_mep() && mep_state->mep_conf->mep_conf.direction == VTSS_APPL_CFM_DIRECTION_UP;

    // If the MEP has more than one RMEP and the CCM interval is faster than one
    // frame per second, we cannot handle it, because all CCM PDUs must be sent
    // to the CPU and handled by S/W, because the H/W only supports exactly
    // one RMEP.
    // Other cases like a particular platform not having H/W support at all,
    // will not allow the CCM interval to be configured faster than 1s in the
    // first place.
    errors->multiple_rmeps_ccm_interval = mep_state->mep_conf->rmep_confs.size() > 1 && mep_state->ma_conf->ccm_interval < VTSS_APPL_CFM_CCM_INTERVAL_1S;

    // We cannot create MEPs on mirror ports.
    // Had the mirror module had events for this, we would have hooked it. Now,
    // we can just sample it whenever this function is invoked for one or the
    // other reason.
    if ((rc = mesa_mirror_monitor_port_get(nullptr, &mirror_port_no)) != VTSS_RC_OK) {
        T_E("mesa_mirror_monitor_port_get() failed: %s", error_txt(rc));
    }

    errors->is_mirror_port = mirror_port_no == port_no;

    // We cannot create MEPs on NPI ports.
    // Normally, NPI configuration is a one-time configuration, so I think it's
    // good enough to sample it when the MEP is created.
    if ((rc = mesa_npi_conf_get(nullptr, &npi_conf)) != VTSS_RC_OK) {
        T_E("mesa_npi_conf_get() failed: %s", error_txt(rc));
    }

    errors->is_npi_port = npi_conf.port_no == port_no;

    old_mep_creatable = errors->mep_creatable;
    errors->mep_creatable = !errors->no_rmeps && !errors->port_up_mep && !errors->multiple_rmeps_ccm_interval && !errors->is_mirror_port && !errors->is_npi_port;

    if (mep_state->mep_conf->mep_conf.direction == VTSS_APPL_CFM_DIRECTION_UP) {
        T_E("Re-consider what to do for Up-MEPs.");
    }

    // For Down-MEPs, we need to determine whether service frames are discarded
    // or not when received on the port on which the Down-MEPs resides.

    // In principle, we could use mesa_packet_port_filter_get() for this, but
    // the granularity of its output is not good enough for populating the
    // configuration errors structure of this MEP, so we need to do it one by
    // one.

    // If there's not link on the port, the MEP may still be created, but we
    // won't be able to Rx (or Tx) frames.
    errors->no_link = !mep_state->port_state->link;

    // A VLAN MEP or a tagged port MEP on a VLAN unaware port is not a good
    // combination.
    errors->vlan_unaware = mep_state->port_state->vlan_conf.port_type == VTSS_APPL_VLAN_PORT_TYPE_UNAWARE && mep_state->is_tagged();

    // Ask the API rather than the VLAN module, because some (mis-behaving!)
    // modules do not use the VLAN module when changing membership.
    if ((rc = mesa_vlan_port_members_get(nullptr, classified_vid, &vlan_port_members)) != VTSS_RC_OK) {
        T_E("mesa_vlan_port_members_get(%u) failed: %s", classified_vid, error_txt(rc));
    }

    // Not forwarding due to not member of VLAN while ingress filtering is
    // enabled.
    errors->vlan_membership = mep_state->port_state->vlan_conf.ingress_filter && !vlan_port_members[port_no];

    // STP forwarding
    // Using the API directly, because using the MSTP module would require two
    // calls to figure out whether the port is discarding or disabled.
    if ((rc = mesa_stp_port_state_get(nullptr, port_no, &stp_status)) != VTSS_RC_OK) {
        T_E("mesa_stp_port_state_get(%u) failed: %s", port_no, error_txt(rc));
    }

    // Not forwarding because STP has marked the port as not forwarding.
    errors->stp_blocked = stp_status != MESA_STP_STATE_FORWARDING;

    // MSTI state for the MEP's VLAN.
    // First convert from VID to MSTI
    if ((rc = mesa_mstp_vlan_msti_get(nullptr, classified_vid, &msti)) != VTSS_RC_OK) {
        T_E("mesa_mstp_vlan_msti_get(%u) failed : %s", classified_vid, error_txt(rc));
    }

    // Then ask for MSTI state on port
    if ((rc = mesa_mstp_port_msti_state_get(nullptr, port_no, msti, &stp_status)) != VTSS_RC_OK) {
        T_E("mesa_mstp_port_msti_state_get(%u, %u) failed: %s", port_no, msti, error_txt(rc));
    }

    errors->mstp_blocked = stp_status != MESA_STP_STATE_FORWARDING;

    // Would have liked to check whether the <port_no, vlan> was discarding due
    // to ERPS, but there's no MESA API to get erps_discard_cnt.

    old_enableRmepDefect = errors->enableRmepDefect;

    // Aggregate results
    errors->enableRmepDefect = !errors->no_link && !errors->vlan_unaware && !errors->vlan_membership && !errors->stp_blocked && !errors->mstp_blocked;

    if (errors->enableRmepDefect != old_enableRmepDefect && !errors->enableRmepDefect) {
        // Create a notification right away if enableRmepDefect transitions to
        // false, indicating that at least one thing is preventing the port from
        // receiving CCMs.
        // If it transitions to true, the CFM_mep_update() call below causes it
        // to wait for valid CCMs.
        cfm_base_mep_ok_notification_update(mep_state);
    }

    T_D("MEP %s:\n"
        "mep_creatable = %d->%d (no_rmeps = %d, port_up_mep = %d, multiple_rmeps_ccm_interval = %d, is_mirror_port = %d, is_npi_port = %d)\n"
        "enableRmepDefect = %d->%d (no_link = %d, vlan_unaware = %d, vlan_membership = %d, stp_blocked = %d, mstp_blocked = %d)",
        mep_state->key,
        old_mep_creatable,
        errors->mep_creatable,
        errors->no_rmeps,
        errors->port_up_mep,
        errors->multiple_rmeps_ccm_interval,
        errors->is_mirror_port,
        errors->is_npi_port,
        old_enableRmepDefect,
        errors->enableRmepDefect,
        errors->no_link,
        errors->vlan_unaware,
        errors->vlan_membership,
        errors->stp_blocked,
        errors->mstp_blocked);

    if (create_event) {
        if (errors->mep_creatable != old_mep_creatable) {
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_CONF);
        } else if (errors->enableRmepDefect != old_enableRmepDefect) {
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_ENABLE_RMEP_DEFECT);
        }
    }
}

/******************************************************************************/
// CFM_mep_state_populate()
/******************************************************************************/
static mesa_rc CFM_mep_state_populate(cfm_mep_state_itr_t mep_state_itr, cfm_mep_conf_t *mep_conf, cfm_md_conf_itr_t md_conf_itr, cfm_ma_conf_itr_t ma_conf_itr)
{
    vtss_ifindex_elm_t ife;
    cfm_mep_state_t    *mep_state = &mep_state_itr->second;

    mep_state->global_conf  = &CFM_global_conf;
    mep_state->global_state = &CFM_global_state;
    mep_state->mep_conf     = mep_conf;

    if (mep_conf) {
        if (vtss_ifindex_decompose(mep_conf->mep_conf.ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= CFM_cap_port_cnt) {
            T_E("Internal error: ifindex (%u) is not a port or port number (%u) >= (%u) ", VTSS_IFINDEX_PRINTF_ARG(mep_conf->mep_conf.ifindex), ife.ordinal, CFM_cap_port_cnt);
            return VTSS_APPL_CFM_RC_MEP_INVALID_IFINDEX;
        }

        if (mep_state->port_state) {
            // Lu26 needs to know the old port number, so that it can back out
            // of old rules.
            mep_state->old_port_no = mep_state->port_state->port_no;
        }

        mep_state->port_state = &CFM_port_state[ife.ordinal];
        mep_state->ma_conf    = &ma_conf_itr->second.ma_conf;
        mep_state->md_conf    = &md_conf_itr->second.md_conf;

        CFM_mep_config_error_update(mep_state_itr, false);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_md_conf_change_get()
/******************************************************************************/
static cfm_mep_state_change_t CFM_md_conf_change_get(const vtss_appl_cfm_md_conf_t *old_conf, const vtss_appl_cfm_md_conf_t *new_conf)
{
    // Given the old and new configuration, figure out what has changed, so that
    // we can tell the base not to reset the entire MEP if we can help it.
    // Some parameters will, however, cause the entire MEP to be reset.
    if (!old_conf || *old_conf != *new_conf) {
        // operator!= compares format, name, and level
        // Reset everything
        return CFM_MEP_STATE_CHANGE_CONF;
    }

    // Changes in the remaining parameters do not require reset of MEP SMs.
    return CFM_MEP_STATE_CHANGE_CONF_NO_RESET;
}

/******************************************************************************/
// CFM_ma_conf_change_get()
/******************************************************************************/
static cfm_mep_state_change_t CFM_ma_conf_change_get(const vtss_appl_cfm_ma_conf_t *old_conf, const vtss_appl_cfm_ma_conf_t *new_conf)
{
    // Given the old and new configuration, figure out what has changed, so that
    // we can tell the base not to reset the entire MEP if we can help it.
    // Some parameters will, however, cause the entire MEP to be reset.
    if (!old_conf                                ||
        *old_conf != *new_conf                   || // operator!= compares format and name
        old_conf->vlan         != new_conf->vlan ||
        old_conf->ccm_interval != new_conf->ccm_interval) {
        // Reset everything
        return CFM_MEP_STATE_CHANGE_CONF;
    }

    // Changes in the remaining parameters do not require reset of MEP SMs.
    return CFM_MEP_STATE_CHANGE_CONF_NO_RESET;
}

/******************************************************************************/
// CFM_mep_conf_change_get()
/******************************************************************************/
static cfm_mep_state_change_t CFM_mep_conf_change_get(const vtss_appl_cfm_mep_conf_t *old_conf, const vtss_appl_cfm_mep_conf_t *new_conf)
{
    // Given the old and new configuration, figure out what has changed, so that
    // we can tell the base not to reset the entire MEP if we can help it.
    // Some parameters will, however, cause the entire MEP to be reset.
    if (!old_conf                                  ||
        !old_conf->admin_active                    ||
        !new_conf->admin_active                    ||
        old_conf->direction != new_conf->direction ||
        old_conf->ifindex   != new_conf->ifindex) {
        // Reset everything
        return CFM_MEP_STATE_CHANGE_CONF;
    }

    // Changes in the remaining parameters do not require reset of MEP SMs.
    return CFM_MEP_STATE_CHANGE_CONF_NO_RESET;
}

/******************************************************************************/
// CFM_mep_conf_update()
// Invoked when any configuration related to a particular MEP changes.
/******************************************************************************/
static mesa_rc CFM_mep_conf_update(const vtss_appl_cfm_mep_key_t &key, cfm_mep_state_change_t change)
{
    cfm_md_conf_itr_t   md_conf_itr;
    cfm_ma_conf_itr_t   ma_conf_itr;
    cfm_mep_conf_itr_t  mep_conf_itr;
    cfm_mep_state_itr_t mep_state_itr = CFM_mep_state_map.find(key);
    cfm_mep_conf_t      *mep_conf;
    bool                delete_state  = false;

    if ((md_conf_itr = CFM_md_conf_map.find(key)) == CFM_md_conf_map.end()) {
        T_E("%s: Unable to lookup MD", key);
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    if ((ma_conf_itr = md_conf_itr->second.ma_confs.find(key)) == md_conf_itr->second.ma_confs.end()) {
        T_E("%s: Unable to lookup MA", key);
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    mep_conf_itr = ma_conf_itr->second.mep_confs.find(key);
    mep_conf     = mep_conf_itr == ma_conf_itr->second.mep_confs.end() ? nullptr : &mep_conf_itr->second;

    if (!mep_conf) {
        // Current MEP is about to be deleted.
        T_I("%s: About to delete MEP", key);
        if (mep_state_itr == CFM_mep_state_map.end()) {
            // No assosicated state. Done.
            return VTSS_RC_OK;
        }

        delete_state = true;
    } else if (mep_state_itr == CFM_mep_state_map.end()) {
        // A new MEP is about to be created. Create the accompanying state.
        if ((mep_state_itr = CFM_mep_state_map.get(key)) == CFM_mep_state_map.end()) {
            return VTSS_APPL_CFM_RC_OUT_OF_MEMORY;
        }

        // Cannot memset(), because it contains structs (e.g. mesa_vce_t), which
        // contain mesa_port_list_t, which has a constructor.
        vtss_clear(mep_state_itr->second);
        mep_state_itr->second.key = key;
        VTSS_RC(cfm_base_mep_state_init(&mep_state_itr->second));
    } else {
        // A simple configuration change
    }

    VTSS_RC(CFM_mep_state_populate(mep_state_itr, mep_conf, md_conf_itr, ma_conf_itr));

    CFM_mep_update(mep_state_itr, change);

    if (delete_state) {
        CFM_mep_state_map.erase(mep_state_itr);
    } else if (mep_state_itr->second.port_state) {
        mep_state_itr->second.old_port_no = mep_state_itr->second.port_state->port_no;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_ma_conf_update()
/******************************************************************************/
static mesa_rc CFM_ma_conf_update(cfm_ma_conf_itr_t ma_conf_itr, cfm_mep_state_change_t change)
{
    cfm_mep_conf_itr_t mep_conf_itr;

    // Look through all MEPs using this MA
    for (mep_conf_itr = ma_conf_itr->second.mep_confs.begin(); mep_conf_itr != ma_conf_itr->second.mep_confs.end(); ++mep_conf_itr) {
        VTSS_RC(CFM_mep_conf_update(mep_conf_itr->first, change));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_md_conf_update()
/******************************************************************************/
static mesa_rc CFM_md_conf_update(cfm_md_conf_itr_t md_conf_itr, cfm_mep_state_change_t change)
{
    cfm_ma_conf_itr_t ma_conf_itr;

    // Look through all MAs using this MD
    for (ma_conf_itr = md_conf_itr->second.ma_confs.begin(); ma_conf_itr != md_conf_itr->second.ma_confs.end(); ++ma_conf_itr) {
        VTSS_RC(CFM_ma_conf_update(ma_conf_itr, change));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_ma_conf_del_meps()
// Delete all MEPs in this MA
/******************************************************************************/
static mesa_rc CFM_ma_conf_del_meps(cfm_ma_conf_itr_t ma_conf_itr)
{
    cfm_mep_conf_itr_t      mep_conf_itr;
    vtss_appl_cfm_mep_key_t key;

    mep_conf_itr = ma_conf_itr->second.mep_confs.begin();
    while (mep_conf_itr != ma_conf_itr->second.mep_confs.end()) {
        cfm_mep_conf_itr_t next_mep_conf_itr = mep_conf_itr;
        ++next_mep_conf_itr;

        // Delete MEP from API and from its map.
        key = mep_conf_itr->first;
        ma_conf_itr->second.mep_confs.erase(mep_conf_itr);
        VTSS_RC(CFM_mep_conf_update(key, CFM_MEP_STATE_CHANGE_CONF));

        mep_conf_itr = next_mep_conf_itr;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_ip_get()
/******************************************************************************/
static bool CFM_ip_get(vtss_appl_ip_if_status_type_t type, vtss_appl_ip_if_status_t &ip_status)
{
    vtss_ifindex_t prev_ifindex = VTSS_IFINDEX_NONE, next_ifindex;

    // Search for the first VLAN interface with a valid address of type "type".
    while (vtss_appl_ip_if_itr(&prev_ifindex, &next_ifindex, true /* only VLAN interfaces */) == VTSS_RC_OK) {
        prev_ifindex = next_ifindex;

        // Get status
        if (vtss_appl_ip_if_status_get(next_ifindex, type, 1, nullptr, &ip_status) != VTSS_RC_OK) {
            continue;
        }

        VTSS_TRACE(CFM_TRACE_GRP_CALLBACK, DEBUG) << "First IP of this type: " << ip_status;
        return true;
    }

    return false;
}

/******************************************************************************/
// CFM_ip_update()
// Returns whether it's changed from last time.
/******************************************************************************/
static bool CFM_ip_update(void)
{
    vtss_appl_ip_if_status_t new_ipv4_status, new_ipv6_status, *new_ip_status;
    bool                     ipv4_found, ipv6_found;

    // Search for the first VLAN interface with a valid IPv4 or IPv6 address.
    ipv4_found = CFM_ip_get(VTSS_APPL_IP_IF_STATUS_TYPE_IPV4, new_ipv4_status);
    ipv6_found = CFM_ip_get(VTSS_APPL_IP_IF_STATUS_TYPE_IPV6, new_ipv6_status);

    if (ipv4_found && ipv6_found) {
        // Both an IPv4 and an IPv6 were found. Choose the one with the lower
        // VLAN ID and IPv4 over IPv6 if they are identical.
        if (vtss_ifindex_as_vlan(new_ipv4_status.ifindex) <= vtss_ifindex_as_vlan(new_ipv6_status.ifindex)) {
            ipv6_found = false;
        } else {
            ipv4_found = false;
        }
    } else if (!ipv4_found && !ipv6_found) {
        // Transmit a NULL-IPv4 in Sender ID TLV.
        memset(&new_ipv4_status, 0, sizeof(new_ipv4_status));
        new_ipv4_status.type = VTSS_APPL_IP_IF_STATUS_TYPE_IPV4;
        ipv4_found = true;
    }

    new_ip_status = ipv4_found ? &new_ipv4_status : &new_ipv6_status;

    VTSS_TRACE(CFM_TRACE_GRP_CALLBACK, INFO) << "\n  Old IP: " << CFM_global_state.ip_status << "\n  New IP: " << *new_ip_status;

    if (memcmp(&CFM_global_state.ip_status, new_ip_status, sizeof(CFM_global_state.ip_status)) == 0) {
        // No change
        return false;
    }

    memcpy(&CFM_global_state.ip_status, new_ip_status, sizeof(CFM_global_state.ip_status));
    return true;
}

/******************************************************************************/
// CFM_ip_change_callback()
/******************************************************************************/
static void CFM_ip_change_callback(vtss_ifindex_t ifidx)
{
    cfm_mep_state_itr_t mep_state_itr;

    if (!vtss_ifindex_is_vlan(ifidx)) {
        T_EG(CFM_TRACE_GRP_CALLBACK, "Unsupported interface: %u", VTSS_IFINDEX_PRINTF_ARG(ifidx));
        return;
    }

    CFM_LOCK_SCOPE();

    // Only update IP address if the VLAN interface comes before the one we've
    // already cached, since we use the first in the Sender ID TLV.
    if (vtss_ifindex_is_none(CFM_global_state.ip_status.ifindex) ||
        CFM_global_state.ip_status.ifindex >= ifidx) {
        if (!CFM_ip_update()) {
            // The IP address hasn't changed.
            return;
        }
    } else {
        // We don't care about this VLAN interface, since it's higher numbered
        // than what we already have.
        return;
    }

    // Update all MEPs to use the new IP address as Sender ID TLV.
    for (mep_state_itr = CFM_mep_state_map.begin(); mep_state_itr != CFM_mep_state_map.end(); ++mep_state_itr) {
        CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_IP_ADDR);
    }
}

/******************************************************************************/
// CFM_global_state_init()
/******************************************************************************/
static void CFM_global_state_init(void)
{
    uint32_t chip_family = fast_cap(MESA_CAP_MISC_CHIP_FAMILY);

    CFM_LOCK_SCOPE();

    // This switch's MAC address
    (void)conf_mgmt_mac_addr_get(CFM_global_state.smac.addr, 0);

    // Get the IP address for the first VLAN interface containing one.
    CFM_ip_update();

    // Listen to changes to IPv4/IPv6 address.
    if (vtss_ip_if_callback_add(CFM_ip_change_callback) != VTSS_RC_OK) {
        T_E("Unable to listen to IP changes");
    }

    // Capabilities
    CFM_global_state.has_vop                = fast_cap(MESA_CAP_VOP);
    CFM_global_state.has_vop_v0             = fast_cap(MESA_CAP_VOP_V0);
    CFM_global_state.has_vop_v1             = fast_cap(MESA_CAP_VOP_V1);
    CFM_global_state.has_vop_v2             = fast_cap(MESA_CAP_VOP_V2);
    CFM_global_state.voe_event_support_mask = fast_cap(MESA_CAP_VOP_EVENT_SUPPORTED);

    // JR2 & SparX5: All CCM rates supported by AFI [VTSS_APPL_CFM_CCM_INTERVAL_300HZ; VTSS_APPL_CFM_CCM_INTERVAL_10MIN]
    // Serval1:      Only the fastest rates are supported by the AFI [VTSS_APPL_CFM_CCM_INTERVAL_1S; VTSS_APPL_CFM_CCM_INTERVAL_10MIN]
    // Caracal:      No CCM rates are supported by the AFI (since there is no AFI).
    switch (chip_family) {
    case MESA_CHIP_FAMILY_CARACAL:
        // AFI is not supported on Caracal, so all frames must be injected
        // manually by. CFM_cap.ccm_interval_min indicates the minimum supported
        // CCM interval to be 1s in that case.
        CFM_global_state.ccm_afi_interval_max = VTSS_APPL_CFM_CCM_INTERVAL_INVALID;
        break;

    case MESA_CHIP_FAMILY_OCELOT:
        // Serval-1's AFI cannot inject slower than 1fps. Any rate slower than
        // that will be handled by manual injection.
        CFM_global_state.ccm_afi_interval_max = VTSS_APPL_CFM_CCM_INTERVAL_1S;
        break;

    case MESA_CHIP_FAMILY_SERVALT:
    case MESA_CHIP_FAMILY_JAGUAR2:
    case MESA_CHIP_FAMILY_SPARX5:
    case MESA_CHIP_FAMILY_LAN969X:
    case MESA_CHIP_FAMILY_LAN966X:
        // JR2 and later chips' AFI supports all rates.
        CFM_global_state.ccm_afi_interval_max = VTSS_APPL_CFM_CCM_INTERVAL_10MIN;
        break;

    default:
        T_E("Unsupported chip family (%d)", chip_family);
        break;
    }

    T_I("Mgmt SMAC = %s, has_vop = %d, has_vop_v0 = %d, has_vop_v1 = %d, has_vop_v2 = %d, voe_event_support_mask = 0x%08x, ccm_afi_interval_max = %s",
        CFM_global_state.smac,
        CFM_global_state.has_vop,
        CFM_global_state.has_vop_v0,
        CFM_global_state.has_vop_v1,
        CFM_global_state.has_vop_v2,
        CFM_global_state.voe_event_support_mask,
        cfm_util_ccm_interval_to_str(CFM_global_state.ccm_afi_interval_max));
}

/******************************************************************************/
// CFM_vlan_custom_etype_change_callback()
/******************************************************************************/
static void CFM_vlan_custom_etype_change_callback(mesa_etype_t tpid)
{
    cfm_mep_state_itr_t mep_state_itr;
    mesa_port_no_t      port_no;

    T_IG(CFM_TRACE_GRP_CALLBACK, "S-Custom TPID: 0x%04x -> 0x%04x", CFM_s_custom_tpid, tpid);

    CFM_LOCK_SCOPE();

    if (CFM_s_custom_tpid == tpid) {
        return;
    }

    CFM_s_custom_tpid = tpid;

    for (port_no = 0; port_no < CFM_cap_port_cnt; port_no++) {
        cfm_port_state_t *port_state = &CFM_port_state[port_no];

        if (port_state->vlan_conf.port_type != VTSS_APPL_VLAN_PORT_TYPE_S_CUSTOM) {
            continue;
        }

        port_state->tpid = CFM_s_custom_tpid;

        // Loop across all MEPs and see if they use this port state.
        for (mep_state_itr = CFM_mep_state_map.begin(); mep_state_itr != CFM_mep_state_map.end(); ++mep_state_itr) {
            if (mep_state_itr->second.port_state != port_state) {
                continue;
            }

            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_TPID);
        }
    }
}

/******************************************************************************/
// CFM_vlan_port_conf_change_callback()
/******************************************************************************/
static void CFM_vlan_port_conf_change_callback(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_vlan_port_detailed_conf_t *new_vlan_conf)
{
    cfm_mep_state_itr_t                 mep_state_itr;
    cfm_port_state_t                    *port_state    = &CFM_port_state[port_no];
    vtss_appl_vlan_port_detailed_conf_t *cur_vlan_conf = &port_state->vlan_conf;
    mesa_etype_t                        new_tpid;
    bool                                port_type_changed, pvid_changed, frame_type_changed, tpid_changed, ingress_filtering_changed;

    CFM_LOCK_SCOPE();

    T_DG(CFM_TRACE_GRP_CALLBACK, "port_no = %u: pvid: %u -> %u, port_type: %s -> %s, ingress_filtering = %d -> %d, frame_type = %s -> %s",
         port_no,
         cur_vlan_conf->pvid,                                    new_vlan_conf->pvid,
         vlan_mgmt_port_type_to_txt(cur_vlan_conf->port_type),   vlan_mgmt_port_type_to_txt(new_vlan_conf->port_type),
         cur_vlan_conf->ingress_filter,                          new_vlan_conf->ingress_filter,
         vlan_mgmt_frame_type_to_txt(cur_vlan_conf->frame_type), vlan_mgmt_frame_type_to_txt(new_vlan_conf->frame_type));

    port_type_changed         = cur_vlan_conf->port_type      != new_vlan_conf->port_type;
    pvid_changed              = cur_vlan_conf->pvid           != new_vlan_conf->pvid;
    frame_type_changed        = cur_vlan_conf->frame_type     != new_vlan_conf->frame_type;
    ingress_filtering_changed = cur_vlan_conf->ingress_filter != new_vlan_conf->ingress_filter;

    if (!port_type_changed && !pvid_changed && !frame_type_changed && !ingress_filtering_changed) {
        // Nothing more to do. We got invoked because of another VLAN port
        // configuration change that we don't care about
        return;
    }

    *cur_vlan_conf = *new_vlan_conf;

    switch (cur_vlan_conf->port_type) {
    case VTSS_APPL_VLAN_PORT_TYPE_UNAWARE:
    case VTSS_APPL_VLAN_PORT_TYPE_C:
        new_tpid = VTSS_APPL_VLAN_C_TAG_ETHERTYPE;
        break;

    case VTSS_APPL_VLAN_PORT_TYPE_S:
        new_tpid = 0x88a8;
        break;

    case VTSS_APPL_VLAN_PORT_TYPE_S_CUSTOM:
        new_tpid = CFM_s_custom_tpid;
        break;

    default:
        T_EG(CFM_TRACE_GRP_CALLBACK, "Unknown port_type (%d)", cur_vlan_conf->port_type);
        new_tpid = port_state->tpid;
        break;
    }

    tpid_changed = port_state->tpid != new_tpid;
    port_state->tpid = new_tpid;

    for (mep_state_itr = CFM_mep_state_map.begin(); mep_state_itr != CFM_mep_state_map.end(); ++mep_state_itr) {
        if (mep_state_itr->second.port_state == port_state) {
            if (port_type_changed || pvid_changed || ingress_filtering_changed) {
                // Both a port-type change, a PVID change and an ingress
                // filtering change may may cause a change in the MEP's
                // configuration errors.
                CFM_mep_config_error_update(mep_state_itr);
            }
        }

        if (pvid_changed || frame_type_changed) {
            // All MEPs must be notified with a PVID or frame-type change,
            // because they create leaking rules on other ports than the MEP's
            // as well, and these leaking rules depend on the other port's VLAN
            // configuration.
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_PVID_OR_ACCEPTABLE_FRAME_TYPE);
        }

        if (mep_state_itr->second.port_state != port_state) {
            // Other VLAN configuration changes only affect MEPs configured on
            // the port on which the VLAN changes occurred.
            continue;
        }

        if (port_type_changed) {
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_PORT_TYPE);
        }

        if (tpid_changed) {
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_TPID);
        }
    }
}

/******************************************************************************/
// CFM_vlan_membership_change_callback()
/******************************************************************************/
static void CFM_vlan_membership_change_callback(vtss_isid_t isid, mesa_vid_t vid, vlan_membership_change_t *changes)
{
    cfm_mep_state_itr_t mep_state_itr;

    // Very disturbing during boot, because we get called back 4K times, so
    // using T_NG() rather than T_DG()
    T_NG(CFM_TRACE_GRP_CALLBACK, "VLAN membership change for VID = %u", vid);

    CFM_LOCK_SCOPE();

    // Loop through all MEPs to see if a MEP is using this VID and if the change
    // will cause changes to the MEP.
    for (mep_state_itr = CFM_mep_state_map.begin(); mep_state_itr != CFM_mep_state_map.end(); ++mep_state_itr) {
        if (mep_state_itr->second.classified_vid_get() == vid) {
            CFM_mep_config_error_update(mep_state_itr);
        }
    }
}

/******************************************************************************/
// CFM_link_state_change()
/******************************************************************************/
static void CFM_link_state_change(const char *func, mesa_port_no_t port_no, bool link)
{
    cfm_port_state_t    *port_state;
    cfm_mep_state_itr_t mep_state_itr;

    if (port_no >= CFM_cap_port_cnt) {
        T_EG(CFM_TRACE_GRP_CALLBACK, "Invalid port_no (%u). Max = %u", port_no, CFM_cap_port_cnt);
        return;
    }

    port_state = &CFM_port_state[port_no];

    T_DG(CFM_TRACE_GRP_CALLBACK, "%s: port_no = %u: Link state: %d -> %d", func, port_no, port_state->link, link);

    if (link == port_state->link) {
        return;
    }

    port_state->link = link;

    for (mep_state_itr = CFM_mep_state_map.begin(); mep_state_itr != CFM_mep_state_map.end(); ++mep_state_itr) {
        if (mep_state_itr->second.port_state != port_state) {
            continue;
        }

        CFM_mep_config_error_update(mep_state_itr);
    }
}

/******************************************************************************/
// CFM_port_link_state_change_callback()
/******************************************************************************/
static void CFM_port_link_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    CFM_LOCK_SCOPE();
    CFM_link_state_change(__FUNCTION__, port_no, status->link);
}

/******************************************************************************/
// CFM_port_shutdown_callback()
/******************************************************************************/
static void CFM_port_shutdown_callback(mesa_port_no_t port_no)
{
    CFM_LOCK_SCOPE();
    CFM_link_state_change(__FUNCTION__, port_no, false);
}

/******************************************************************************/
// CFM_port_interrupt_callback()
/******************************************************************************/
static void CFM_port_interrupt_callback(meba_event_t source_id, u32 port_no)
{
    mesa_rc rc;

    CFM_LOCK_SCOPE();

    // We need to re-register for link-state-changes
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_CFM, CFM_port_interrupt_callback, source_id, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_CALLBACK, "vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
    }

    CFM_link_state_change(__FUNCTION__, port_no, false);
}

#ifdef VTSS_SW_OPTION_MSTP
/******************************************************************************/
// CFM_port_stp_msti_state_change_callback()
/******************************************************************************/
static void CFM_port_stp_msti_state_change_callback(vtss_common_port_t l2port, u8 msti, vtss_common_stpstate_t new_state)
{
    vtss_isid_t         isid;
    mesa_port_no_t      port_no;
    cfm_mep_state_itr_t mep_state_itr;

    // Convert l2port to isid/iport
    if (!l2port2port(l2port, &isid, &port_no)) {
        T_IG(CFM_TRACE_GRP_CALLBACK, "l2port2port(%u) failed - probably because it's an aggr not a port", l2port);
        return;
    }

    T_DG(CFM_TRACE_GRP_CALLBACK, "MSTP change on port_no %u. new_state = %d", port_no, new_state);

    CFM_LOCK_SCOPE();

    // Update port/VLAN's forwarding state for the sake of Port Status TLVs
    for (mep_state_itr = CFM_mep_state_map.begin(); mep_state_itr != CFM_mep_state_map.end(); ++mep_state_itr) {
        if (mep_state_itr->second.port_state->port_no != port_no) {
            continue;
        }

        CFM_mep_config_error_update(mep_state_itr);
    }
}
#endif

/******************************************************************************/
// CFM_frame_rx_callback()
/******************************************************************************/
static BOOL CFM_frame_rx_callback(void *contxt, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    mesa_etype_t        etype;
    mesa_iflow_id_t     iflow_id;
    cfm_mep_state_itr_t mep_state_itr;
    cfm_mep_state_t     *mep_state;
    uint8_t             opcode;

    // As long as we can't receive a frame behind two tags, the frame is always
    // normalized, so that the CFM Ethertype comes at frm[12] and frm[13],
    // because the packet module strips the outer tag.
    etype  = frm[12] << 8 | frm[13];
    opcode = frm[15];

    T_DG(CFM_TRACE_GRP_FRAME_RX, "Rx frame on port_no = %u of length %u excl. FCS. EtherType = 0x%04x, opcode = %u, iflow_id = %u, hints = 0x%x, class-tag: tpid = 0x%04x, vid = %u, pcp = %u, dei = %u, stripped_tag: tpid = 0x%04x, vid = %u, pcp = %u, dei = %u",
         rx_info->port_no,
         rx_info->length,
         etype,
         opcode,
         rx_info->iflow_id,
         rx_info->hints,
         rx_info->tag.tpid,
         rx_info->tag.vid,
         rx_info->tag.pcp,
         rx_info->tag.dei,
         rx_info->stripped_tag.tpid,
         rx_info->stripped_tag.vid,
         rx_info->stripped_tag.pcp,
         rx_info->stripped_tag.dei);

    if (etype != CFM_ETYPE) {
        // Why did we receive this frame? Our Packet Rx Filter tells the packet
        // module only to send us frames with CFM EtherType.
        T_EG(CFM_TRACE_GRP_FRAME_RX, "Rx frame on port_no %u with etype = 0x%04x, which isn't the CFM EtherType", rx_info->port_no, etype);

        // Not consumed
        return FALSE;
    }

    // CFM_global_state will not change after initialization, so no need to have
    // the CFM mutex locked here.
    if (CFM_global_state.has_vop && !CFM_global_state.has_vop_v0) {
        if (CFM_global_state.has_vop_v1 || CFM_global_state.has_vop_v2) {
            iflow_id = rx_info->iflow_id;
        } else {
            T_EG(CFM_TRACE_GRP_FRAME_RX, "Chip has VOP, but it's neither v1 nor v2. Don't know how to handle frame");

            // Not consumed
            return FALSE;
        }

        if (iflow_id == MESA_IFLOW_ID_NONE) {
            // The frame must have been mapped to an IFLOW. If not, I don't know
            //  why we got it.
            T_DG(CFM_TRACE_GRP_FRAME_RX, "Rx CFM frame on port_no = %u, but with a IFLOW ID of %u. Not handling", rx_info->port_no, iflow_id);
            return TRUE;
        }
    } else {
        // On Caracal, there is no such thing as an iflow_id and on LAN966x, we
        // don't use iflow_id. Instead we need to loop through all MEPs to see
        // if Rx Port number and classified VLAN, etc. match.
    }

    CFM_LOCK_SCOPE();

    // Loop through all MEP states and find - amongst the active MEPs - the one
    // that matches this frame.
    for (mep_state_itr = CFM_mep_state_map.begin(); mep_state_itr != CFM_mep_state_map.end(); ++mep_state_itr) {
        mep_state = &mep_state_itr->second;

        if (!mep_state->status.mep_active) {
            continue;
        }

        if (CFM_global_state.has_vop && !CFM_global_state.has_vop_v0) {
            // The iflow_id must match
            if (mep_state->iflow_id != iflow_id) {
                continue;
            }
        } else {
            // Caracal && LAN966x: Match on ingress port and classified VLAN.
            if (mep_state->port_state->port_no != rx_info->port_no) {
                continue;
            }

            if (mep_state->classified_vid_get() != rx_info->tag.vid) {
                // The MEP's expected classified VID must match the frame's
                // classified VID, but it doesn't, so not for us.
                continue;
            }

            if (mep_state->is_tagged()) {
                // This is a tagged Port or VLAN MEP
                if (!rx_info->stripped_tag.tpid) {
                    // There must be a stripped tag in the frame, but there
                    // isn't.
                    continue;
                }
            } else {
                // This is a an untagged port MEP. We don't accept frames with
                // tags.
                if (rx_info->stripped_tag.tpid) {
                    continue;
                }
            }
        }

        T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s handles the frame", mep_state_itr->first);
        T_NG_HEX(CFM_TRACE_GRP_FRAME_RX, frm, rx_info->length);
        cfm_base_frame_rx(&mep_state_itr->second, frm, rx_info);

        // Consumed
        return TRUE;
    }

    // Got a frame (perhaps a CCM leakage PDU, that is, a VLAN/level-matching
    // CCM PDU received on another port than the port on which the MEP is
    // installed. Such CCM PDUs must be discarded) that we don't know how to
    // handle.
    T_DG(CFM_TRACE_GRP_FRAME_RX, "Dropping CFM frame received on classified VLAN %u, port_no = %u", rx_info->tag.vid, rx_info->port_no);

    // Consumed, since no-one else should handle CCM PDUs.
    return TRUE;
}

/******************************************************************************/
// CFM_voe_interrupt_callback()
/******************************************************************************/
static void CFM_voe_interrupt_callback(meba_event_t source_id, u32 instance_id)
{
    cfm_mep_state_itr_t mep_state_itr;
    uint32_t            pending_events;
    CapArray<uint32_t, MESA_CAP_VOP_EVENT_ARRAY_SIZE> active_voes;
    uint32_t            voe_idx;
    mesa_rc             rc;

    T_IG(CFM_TRACE_GRP_CALLBACK, "source_id = %d, instance_id = %d", source_id, instance_id);

    // Figure out which VOE(s) caused this interrupt
    if ((rc = mesa_voe_event_active_get(nullptr, active_voes.size(), active_voes.data())) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_CALLBACK, "mesa_voe_event_active_get(%u) failed: %s", active_voes.size(), error_txt(rc));
    }

    CFM_LOCK_SCOPE();

    // We need to re-register for VOE-interrupts
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_CFM, CFM_voe_interrupt_callback, source_id, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_CALLBACK, "vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
    }

    // Loop through all MEPs and check to see if their VOE has generated this
    // interrupt.
    for (mep_state_itr = CFM_mep_state_map.begin(); mep_state_itr != CFM_mep_state_map.end(); ++mep_state_itr) {
        voe_idx = mep_state_itr->second.voe_idx;
        if (voe_idx == MESA_VOE_IDX_NONE) {
            continue;
        }

        if (!(active_voes.data()[voe_idx / 32] & VTSS_BIT(voe_idx % 32))) {
            // Not this VOE that caused an interrupt
            continue;
        }

        if ((rc = mesa_voe_event_get(nullptr, voe_idx, &pending_events)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_CALLBACK, "MEP %s: mesa_voe_event_get(%u) failed: %s", mep_state_itr->first, voe_idx, error_txt(rc));
            continue;
        }

        T_IG(CFM_TRACE_GRP_CALLBACK, "MEP %s: voe_idx = %u: pending events before masking = 0x%08x, mask = 0x%08x", mep_state_itr->first, voe_idx, pending_events, mep_state_itr->second.voe_event_mask);

        // Mask out those events we don't care about.
        pending_events &= mep_state_itr->second.voe_event_mask;

        if (!pending_events) {
            continue;
        }

        if (pending_events & MESA_VOE_EVENT_MASK_CCM_PERIOD) {
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_PERIOD);
        }

        if (pending_events & MESA_VOE_EVENT_MASK_CCM_ZERO_PERIOD) {
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_ZERO_PERIOD);
        }

        if (pending_events & MESA_VOE_EVENT_MASK_CCM_PRIORITY) {
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_PRIORITY);
        }

        if (pending_events & MESA_VOE_EVENT_MASK_CCM_LOC) {
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_LOC);
        }

        if (pending_events & MESA_VOE_EVENT_MASK_CCM_MEP_ID) {
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_MEPID);
        }

        if (pending_events & MESA_VOE_EVENT_MASK_CCM_MEG_ID) {
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_MAID);
        }

        if (pending_events & MESA_VOE_EVENT_MASK_CCM_MEG_LEVEL) {
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_LEVEL);
        }

        if (pending_events & MESA_VOE_EVENT_MASK_CCM_RX_RDI) {
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_RDI);
        }

        if (pending_events & MESA_VOE_EVENT_MASK_CCM_SRC_PORT_MOVE) {
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_SRC_PORT_MOVE);
        }

        if (pending_events & MESA_VOE_EVENT_MASK_CCM_TLV_PORT_STATUS) {
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_TLV_PORT_STATUS);
        }

        if (pending_events & MESA_VOE_EVENT_MASK_CCM_TLV_IF_STATUS) {
            CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_TLV_IF_STATUS);
        }
    }
}

/******************************************************************************/
// CFM_port_state_init()
/******************************************************************************/
static void CFM_port_state_init(void)
{
    vtss_appl_vlan_port_conf_t vlan_conf;
    cfm_port_state_t           *port_state;
    mesa_port_no_t             port_no;
    mesa_rc                    rc;

    {
        CFM_LOCK_SCOPE();

        CFM_s_custom_tpid = VTSS_APPL_VLAN_CUSTOM_S_TAG_DEFAULT;

        for (port_no = 0; port_no < CFM_cap_port_cnt; port_no++) {
            // Get current VLAN configuration as configured in H/W. We could have
            // used vtss_appl_vlan_interface_conf_get(), but that would require us
            // to provide an ifindex rather than a port number, so we use the
            // internal API instead.
            // The function returns what we need in vlan_conf.hybrid.
            if ((rc = vlan_mgmt_port_conf_get(VTSS_ISID_LOCAL, port_no, &vlan_conf, VTSS_APPL_VLAN_USER_ALL, true)) != VTSS_RC_OK) {
                T_E("vlan_mgmt_port_conf_get(%u) failed: %s", port_no, error_txt(rc));
            }

            port_state            = &CFM_port_state[port_no];
            port_state->port_no   = port_no;
            port_state->tpid      = VTSS_APPL_VLAN_C_TAG_ETHERTYPE;
            port_state->vlan_conf = vlan_conf.hybrid;
            port_state->link      = false;
            (void)conf_mgmt_mac_addr_get(port_state->smac.addr, port_no + 1);
        }
    }

    // Do subscriptions without our mutex taken in order to avoid deadlocks in
    // case we make two or more subscriptions to the same module and an event
    // occurs in that module that requires callbacks in between the two calls.
    // As an example, suppose we did it as follows:
    // 1) Take CFM mutex
    // 2) port_change_register() is called and returns.
    // 3) A link-state change event occurs and callbacks get invoked by the
    //    port module with its port_cb mutex taken.
    // 4) CFM_port_link_state_change_callback() gets invoked. It attempts to
    //    take the CFM mutex, which is already taken, so that thread has to
    //    wait.
    // 5) port_shutdown_register() is called by us. This function attempts to
    //    take the port_cb mutex, but that is already taken.
    // 6) DEADLOCK!

    // Subscribe to Custom S-tag TPID changes in the VLAN module
    vlan_s_custom_etype_change_register(VTSS_MODULE_ID_CFM, CFM_vlan_custom_etype_change_callback);

    // Subscribe to VLAN port changes in the VLAN module
    vlan_port_conf_change_register(VTSS_MODULE_ID_CFM, CFM_vlan_port_conf_change_callback, TRUE);

    // Subscribe to VLAN membership changes in the VLAN module
    vlan_membership_change_register(VTSS_MODULE_ID_CFM, CFM_vlan_membership_change_callback);

    // Subscribe to link changes in the Port module
    if ((rc = port_change_register(VTSS_MODULE_ID_CFM, CFM_port_link_state_change_callback)) != VTSS_RC_OK) {
        T_E("port_change_register() failed: %s", error_txt(rc));
    }

    // Subscribe to 'shutdown' commands
    if ((rc = port_shutdown_register(VTSS_MODULE_ID_CFM, CFM_port_shutdown_callback)) != VTSS_RC_OK) {
        T_E("port_shutdown_register() failed: %s", error_txt(rc));
    }

    // Subscribe to FLNK and LOS interrupts
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_CFM, CFM_port_interrupt_callback, MEBA_EVENT_FLNK, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
        T_E("vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
    }

    // Subscribe to LoS interrupts
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_CFM, CFM_port_interrupt_callback, MEBA_EVENT_LOS, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
        T_E("vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
    }

#ifdef VTSS_SW_OPTION_MSTP
    // Subscribe to MSTP state changes
    if ((rc = l2_stp_msti_state_change_register(CFM_port_stp_msti_state_change_callback)) != VTSS_RC_OK) {
        T_E("l2_stp_msti_state_change_register() failed: %s", error_txt(rc));
    }
#endif
}

/******************************************************************************/
// CFM_frame_rx_register()
/******************************************************************************/
static void CFM_frame_rx_register(void)
{
    void               *filter_id;
    packet_rx_filter_t filter;
    mesa_rc            rc;

    packet_rx_filter_init(&filter);

    // We only need to install one packet filter, matching on the CFM ethertype.
    // The reason is that the packet module strips a possible outer tag before
    // it starts matching. This means that as long as we only support CFM behind
    // one tag, this is good enough. If we some day start supporting CFM behind
    // two (or more tags), we must also install filters for the inner tag and
    // let the CFM_frame_rx_callback() function look at the EtherType behind
    // that one tag.
    filter.modid = VTSS_MODULE_ID_CFM;
    filter.match = PACKET_RX_FILTER_MATCH_ETYPE;
    filter.cb    = CFM_frame_rx_callback;
    filter.prio  = PACKET_RX_FILTER_PRIO_NORMAL;
    filter.etype = CFM_ETYPE;
    if ((rc = packet_rx_filter_register(&filter, &filter_id)) != VTSS_RC_OK) {
        T_E("packet_rx_filter_register() failed: %s", error_txt(rc));
    }
}

/******************************************************************************/
// CFM_conf_change_callbacks_invoke()
/******************************************************************************/
static void CFM_conf_change_callbacks_invoke(void)
{
    int i;

    // Do not callback anyone until started, since that's a waste of time.
    // They will be called back once CFM_started gets set anyway.
    if (!CFM_started) {
        return;
    }

    // When this function is called, our mutex is NOT taken.
    // We should have taken it to obtain all the registrants, but I think we
    // are pretty safe without doing so.
    for (i = 0; i < ARRSZ(CFM_conf_change_callbacks); i++) {
        if (CFM_conf_change_callbacks[i].callback) {
            T_D("Invoking %p in module %s", CFM_conf_change_callbacks[i].callback, vtss_module_names[CFM_conf_change_callbacks[i].module_id]);
            CFM_conf_change_callbacks[i].callback();
        }
    }
}

/******************************************************************************/
// CFM_default()
/******************************************************************************/
static void CFM_default(void)
{
    cfm_md_conf_itr_t       md_conf_itr, next_md_conf_itr;
    cfm_ma_conf_itr_t       ma_conf_itr, next_ma_conf_itr;
    cfm_mep_conf_itr_t      mep_conf_itr, next_mep_conf_itr;
    vtss_appl_cfm_mep_key_t key;

    T_I("Creating defaults");

    {
        CFM_LOCK_SCOPE();

        // Loop through all MEPs and delete them
        md_conf_itr = CFM_md_conf_map.begin();
        while (md_conf_itr != CFM_md_conf_map.end()) {
            next_md_conf_itr = md_conf_itr;
            ++next_md_conf_itr;

            ma_conf_itr = md_conf_itr->second.ma_confs.begin();
            while (ma_conf_itr != md_conf_itr->second.ma_confs.end()) {
                next_ma_conf_itr = ma_conf_itr;
                ++next_ma_conf_itr;

                mep_conf_itr = ma_conf_itr->second.mep_confs.begin();
                while (mep_conf_itr != ma_conf_itr->second.mep_confs.end()) {
                    next_mep_conf_itr = mep_conf_itr;
                    ++next_mep_conf_itr;

                    key = mep_conf_itr->first;
                    ma_conf_itr->second.mep_confs.erase(mep_conf_itr);
                    T_I("%s: Invoking CFM_mep_conf_update()", key);
                    (void)CFM_mep_conf_update(key, CFM_MEP_STATE_CHANGE_CONF);

                    mep_conf_itr = next_mep_conf_itr;
                }

                md_conf_itr->second.ma_confs.erase(ma_conf_itr);
                ma_conf_itr = next_ma_conf_itr;
            }

            CFM_md_conf_map.erase(md_conf_itr);
            md_conf_itr = next_md_conf_itr;
        }

        (void)vtss_appl_cfm_global_conf_default_get(&CFM_global_conf);
    }

    CFM_conf_change_callbacks_invoke();
}

/******************************************************************************/
// CFM_mep_itr_get()
/******************************************************************************/
static mesa_rc CFM_mep_itr_get(const vtss_appl_cfm_mep_key_t &key, cfm_mep_conf_itr_t &mep_conf_itr)
{
    cfm_md_conf_itr_t md_conf_itr;
    cfm_ma_conf_itr_t ma_conf_itr;

    if ((md_conf_itr = CFM_md_conf_map.find(key)) == CFM_md_conf_map.end()) {
        return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
    }

    if ((ma_conf_itr = md_conf_itr->second.ma_confs.find(key)) == md_conf_itr->second.ma_confs.end()) {
        return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
    }

    if ((mep_conf_itr = ma_conf_itr->second.mep_confs.find(key)) == ma_conf_itr->second.mep_confs.end()) {
        return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_rmep_itr_get()
/******************************************************************************/
static mesa_rc CFM_rmep_itr_get(const vtss_appl_cfm_rmep_key_t &key, cfm_rmep_conf_itr_t &rmep_conf_itr)
{
    cfm_md_conf_itr_t  md_conf_itr;
    cfm_ma_conf_itr_t  ma_conf_itr;
    cfm_mep_conf_itr_t mep_conf_itr;

    if ((md_conf_itr = CFM_md_conf_map.find(key)) == CFM_md_conf_map.end()) {
        return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
    }

    if ((ma_conf_itr = md_conf_itr->second.ma_confs.find(key)) == md_conf_itr->second.ma_confs.end()) {
        return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
    }

    if ((mep_conf_itr = ma_conf_itr->second.mep_confs.find(key)) == ma_conf_itr->second.mep_confs.end()) {
        return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
    }

    if ((rmep_conf_itr = mep_conf_itr->second.rmep_confs.find(key)) == mep_conf_itr->second.rmep_confs.end()) {
        return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_port_or_service_mep_cnt_exceeded_chk()
/******************************************************************************/
static mesa_rc CFM_port_or_service_mep_cnt_exceeded_chk(cfm_ma_conf_t *dont_count_ma_conf, cfm_mep_conf_t *dont_count_mep_conf, uint32_t port_mep_initial_cnt, uint32_t service_mep_initial_cnt)
{
    cfm_md_conf_itr_t  md_conf_itr;
    cfm_ma_conf_itr_t  ma_conf_itr;
    cfm_mep_conf_itr_t mep_conf_itr;
    uint32_t           p = port_mep_initial_cnt, s = service_mep_initial_cnt, *cnt;

    for (md_conf_itr = CFM_md_conf_map.begin(); md_conf_itr != CFM_md_conf_map.end(); ++md_conf_itr) {
        for (ma_conf_itr = md_conf_itr->second.ma_confs.begin(); ma_conf_itr != md_conf_itr->second.ma_confs.end(); ++ma_conf_itr) {
            if (&ma_conf_itr->second == dont_count_ma_conf) {
                // Disregard this MA when counting, because it's currently being
                // configured, but not yet saved.
                continue;
            }

            // Increase the appropriate counter depending on whether the MA is
            // a Port MEP MA or a VLAN MEP MA.
            cnt = ma_conf_itr->second.ma_conf.vlan == 0 ? &p : &s;

            for (mep_conf_itr = ma_conf_itr->second.mep_confs.begin(); mep_conf_itr != ma_conf_itr->second.mep_confs.end(); ++mep_conf_itr) {
                if (&mep_conf_itr->second == dont_count_mep_conf) {
                    // Disregard this MEP when counting, because it's currently
                    // being configured, but not yet saved.
                    continue;
                }

                if (mep_conf_itr->second.mep_conf.admin_active) {
                    // This MEP is active, so count it in the approprite counter
                    (*cnt)++;
                }
            }
        }
    }

    // Check to see if the number of port/service instances are exceeded
    if (p > CFM_cap.mep_cnt_port_max) {
        // This function can be called from two places: When (re-)configuring an
        // MA and when (re-)configuring a MEP. When called from MA, the
        // dont_count_ma_conf is always non-NULL, so we know that if that one
        // is non-NULL, we return the MA error code, otherwise the MEP error
        // code.
        return dont_count_ma_conf ? VTSS_APPL_CFM_RC_MA_PORT_LIMIT_REACHED : VTSS_APPL_CFM_RC_MEP_PORT_LIMIT_REACHED;
    }

    if (s > CFM_cap.mep_cnt_service_max) {
        return dont_count_ma_conf ? VTSS_APPL_CFM_RC_MA_SERVICE_LIMIT_REACHED : VTSS_APPL_CFM_RC_MEP_SERVICE_LIMIT_REACHED;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_md_level_change_check()
// Invoked when the MD level is changing.
/******************************************************************************/
static mesa_rc CFM_md_level_change_check(cfm_md_conf_itr_t md_conf_itr, uint8_t new_level)
{
    cfm_md_conf_itr_t  md_conf_itr2;
    cfm_ma_conf_itr_t  ma_conf_itr, ma_conf_itr2;
    cfm_mep_conf_itr_t mep_conf_itr, mep_conf_itr2;
    mesa_vid_t         this_vlan, other_vlan;
    bool               new_is_port_mep, other_is_port_mep;

    // Loop over all MEPs in this MD.
    for (ma_conf_itr = md_conf_itr->second.ma_confs.begin(); ma_conf_itr != md_conf_itr->second.ma_confs.end(); ++ma_conf_itr) {
        for (mep_conf_itr = ma_conf_itr->second.mep_confs.begin(); mep_conf_itr != ma_conf_itr->second.mep_confs.end(); ++mep_conf_itr) {
            if (!mep_conf_itr->second.mep_conf.admin_active) {
                // This one doesn't contribute
                continue;
            }

            for (md_conf_itr2 = CFM_md_conf_map.begin(); md_conf_itr2 != CFM_md_conf_map.end(); ++md_conf_itr2) {
                if (md_conf_itr2 == md_conf_itr) {
                    // Don't check against MEPs in our own MD.
                    continue;
                }

                for (ma_conf_itr2 = md_conf_itr2->second.ma_confs.begin(); ma_conf_itr2 != md_conf_itr2->second.ma_confs.end(); ++ma_conf_itr2) {
                    for (mep_conf_itr2 = ma_conf_itr2->second.mep_confs.begin(); mep_conf_itr2 != ma_conf_itr2->second.mep_confs.end(); ++mep_conf_itr2) {
                        if (!mep_conf_itr2->second.mep_conf.admin_active) {
                            // The other MEP is not active and therefore doesn't
                            // contribute to errors.
                            continue;
                        }

                        new_is_port_mep   = ma_conf_itr->second.ma_conf.vlan  == 0;
                        other_is_port_mep = ma_conf_itr2->second.ma_conf.vlan == 0;

                        if (CFM_global_state.has_vop_v1) {
                            // On Serval-1, all Port MEPs must have same level,
                            // or we cannot avoid leaking.
                            if (new_is_port_mep && other_is_port_mep && new_level != md_conf_itr2->second.md_conf.level) {
                                return VTSS_APPL_CFM_RC_MD_ALL_PORT_MEPS_MUST_HAVE_SAME_LEVEL;
                            }

                            // On Serval-1, a Port MEP must have lower level
                            // than any VLAN MEP, or we cannot avoid leaking.
                            if (new_is_port_mep && !other_is_port_mep && new_level >= md_conf_itr2->second.md_conf.level) {
                                return VTSS_APPL_CFM_RC_MD_PORT_MEP_MUST_HAVE_LOWER_LEVEL_THAN_ANY_VLAN_MEP;
                            }

                            // On Serval-1, a VLAN MEP must have higher level
                            // than any Port MEP, or we cannot avoid leaking.
                            if (!new_is_port_mep && other_is_port_mep && new_level <= md_conf_itr2->second.md_conf.level) {
                                return VTSS_APPL_CFM_RC_MD_VLAN_MEP_MUST_HAVE_HIGHER_LEVEL_THAN_PORT_MEPS;
                            }
                        }

                        if (mep_conf_itr->second.mep_conf.ifindex != mep_conf_itr2->second.mep_conf.ifindex) {
                            // The two MEPs are not on the same port. Go on.
                            continue;
                        }

                        // There are further restrictions on two MEPs on the
                        // same port.
                        this_vlan  = mep_conf_itr->second.mep_conf.vlan  != 0 ? mep_conf_itr->second.mep_conf.vlan  : ma_conf_itr->second.ma_conf.vlan;
                        other_vlan = mep_conf_itr2->second.mep_conf.vlan != 0 ? mep_conf_itr2->second.mep_conf.vlan : ma_conf_itr2->second.ma_conf.vlan;

                        T_I("new = %s, other = %s, new_is_port_mep = %d, other_is_port_mep = %d, this_vlan = %u, other_vlan = %u",
                            mep_conf_itr->first, mep_conf_itr2->first, new_is_port_mep, other_is_port_mep, this_vlan, other_vlan);

                        if (this_vlan != other_vlan) {
                            continue;
                        }

                        if (new_is_port_mep && !other_is_port_mep) {
                            // New is a Port MEP and the other is a VLAN MEP.
                            // The level of the Port MEP must be lower than that
                            // of the VLAN MEP.
                            if (new_level >= md_conf_itr2->second.md_conf.level) {
                                return VTSS_APPL_CFM_RC_MD_LEVEL_OF_PORT_MEP_MUST_BE_LOWER_THAN_LEVEL_OF_VLAN_MEP;
                            }
                        } else if (!new_is_port_mep && other_is_port_mep) {
                            // New is a VLAN MEP and the other is a Port MEP.
                            // The level of the VLAN MEP must be higher than
                            // that of the Port MEP.
                            if (new_level <= md_conf_itr2->second.md_conf.level) {
                                return VTSS_APPL_CFM_RC_MD_LEVEL_OF_VLAN_MEP_MUST_BE_HIGHER_THAN_LEVEL_OF_PORT_MEP;
                            }
                        }
                    }
                }
            }
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_ma_type_change_check()
// Invoked when the MA is changing from containing Port MEPs to containing VLAN
// MEPs or vice versa.
// ma_conf_itr points to the old configuration.
/******************************************************************************/
static mesa_rc CFM_ma_type_change_check(cfm_md_conf_itr_t md_conf_itr, cfm_ma_conf_itr_t ma_conf_itr, mesa_vid_t new_ma_vlan)
{
    cfm_md_conf_itr_t  md_conf_itr2;
    cfm_ma_conf_itr_t  ma_conf_itr2;
    cfm_mep_conf_itr_t mep_conf_itr, mep_conf_itr2;
    uint32_t           port_mep_cnt = 0;
    uint32_t           vlan_mep_cnt = 0;
    mesa_vid_t         this_vlan, other_vlan;
    bool               contains_active_meps = false, new_is_port_mep, other_is_port_mep;

    new_is_port_mep = new_ma_vlan == 0;

    // If the MA is changed from containing Port MEPs to containing VLAN MEPs or
    // vice versa, it may happen that the total number of one or the other
    // exceeds their respective limits.
    // To get started, we have to count the number of port or service MEPs that
    // would result in using this new configuration (held in ma_conf_itr).
    for (mep_conf_itr = ma_conf_itr->second.mep_confs.begin(); mep_conf_itr != ma_conf_itr->second.mep_confs.end(); ++mep_conf_itr) {
        if (mep_conf_itr->second.mep_conf.admin_active) {
            contains_active_meps = true;
            if (new_is_port_mep) {
                port_mep_cnt++;
            } else {
                vlan_mep_cnt++;
            }
        }
    }

    // Then feed those initial counts into the following function, which
    // knows not to count this MA twice.
    VTSS_RC(CFM_port_or_service_mep_cnt_exceeded_chk(&ma_conf_itr->second, nullptr, port_mep_cnt, vlan_mep_cnt));

    if (!contains_active_meps) {
        // Nothing more to check
        return VTSS_RC_OK;
    }

    // Other checks across domains
    // Loop across all active MEPs in the MA we are trying to change.
    for (mep_conf_itr = ma_conf_itr->second.mep_confs.begin(); mep_conf_itr != ma_conf_itr->second.mep_confs.end(); ++mep_conf_itr) {
        if (!mep_conf_itr->second.mep_conf.admin_active) {
            // This one doesn't contribute
            continue;
        }

        for (md_conf_itr2 = CFM_md_conf_map.begin(); md_conf_itr2 != CFM_md_conf_map.end(); ++md_conf_itr2) {
            for (ma_conf_itr2 = md_conf_itr2->second.ma_confs.begin(); ma_conf_itr2 != md_conf_itr2->second.ma_confs.end(); ++ma_conf_itr2) {
                if (ma_conf_itr2 == ma_conf_itr) {
                    // Don't check against MEPs in our own MA.
                    continue;
                }

                for (mep_conf_itr2 = ma_conf_itr2->second.mep_confs.begin(); mep_conf_itr2 != ma_conf_itr2->second.mep_confs.end(); ++mep_conf_itr2) {
                    if (!mep_conf_itr2->second.mep_conf.admin_active) {
                        // The other MEP is not active and therefore doesn't
                        // contribute to errors.
                        continue;
                    }

                    other_is_port_mep = ma_conf_itr2->second.ma_conf.vlan == 0;

                    if (CFM_global_state.has_vop_v1) {
                        // On Serval-1, all Port MEPs must have same level, or
                        // we cannot avoid leaking.
                        if (new_is_port_mep && other_is_port_mep && md_conf_itr->second.md_conf.level != md_conf_itr2->second.md_conf.level) {
                            return VTSS_APPL_CFM_RC_MA_ALL_PORT_MEPS_MUST_HAVE_SAME_LEVEL;
                        }

                        // On Serval-1, a Port MEP must have lower level than
                        // any VLAN MEP, or we cannot avoid leaking.
                        if (new_is_port_mep && !other_is_port_mep && md_conf_itr->second.md_conf.level >= md_conf_itr2->second.md_conf.level) {
                            return VTSS_APPL_CFM_RC_MA_PORT_MEP_MUST_HAVE_LOWER_LEVEL_THAN_ANY_VLAN_MEP;
                        }

                        // On Serval-1, a VLAN MEP must have higher level than
                        // any Port MEP, or we cannot avoid leaking.
                        if (!new_is_port_mep && other_is_port_mep && md_conf_itr->second.md_conf.level <= md_conf_itr2->second.md_conf.level) {
                            return VTSS_APPL_CFM_RC_MA_VLAN_MEP_MUST_HAVE_HIGHER_LEVEL_THAN_PORT_MEPS;
                        }
                    }

                    if (mep_conf_itr->second.mep_conf.ifindex != mep_conf_itr2->second.mep_conf.ifindex) {
                        // The two MEPs are not on the same port. Go on.
                        continue;
                    }

                    // There are further restrictions on two MEPs on the same
                    // port.
                    this_vlan  = mep_conf_itr->second.mep_conf.vlan  != 0 ? mep_conf_itr->second.mep_conf.vlan  : new_ma_vlan;
                    other_vlan = mep_conf_itr2->second.mep_conf.vlan != 0 ? mep_conf_itr2->second.mep_conf.vlan : ma_conf_itr2->second.ma_conf.vlan;

                    T_I("new = %s, other = %s, new_is_port_mep = %d, other_is_port_mep = %d, this_vlan = %u, other_vlan = %u",
                        mep_conf_itr->first, mep_conf_itr2->first, new_is_port_mep, other_is_port_mep, this_vlan, other_vlan);

                    if (new_is_port_mep && other_is_port_mep) {
                        // This change will cause two Port MEPs to become active
                        // at the same time.
                        return VTSS_APPL_CFM_RC_MA_ONLY_ONE_PORT_MEP_PER_PORT;
                    } else if (this_vlan == other_vlan) {
                        if (new_is_port_mep) {
                            // New is a Port MEP and the other is a VLAN MEP.
                            // The level of the Port MEP must be lower than that
                            // of the VLAN MEP.
                            if (md_conf_itr->second.md_conf.level >= md_conf_itr2->second.md_conf.level) {
                                return VTSS_APPL_CFM_RC_MA_LEVEL_OF_PORT_MEP_MUST_BE_LOWER_THAN_LEVEL_OF_VLAN_MEP;
                            }
                        } else if (other_is_port_mep) {
                            // New is a VLAN MEP and the other is a Port MEP.
                            // The level of the VLAN MEP must be higher than
                            // that of the Port MEP.
                            if (md_conf_itr->second.md_conf.level <= md_conf_itr2->second.md_conf.level) {
                                return VTSS_APPL_CFM_RC_MA_LEVEL_OF_VLAN_MEP_MUST_BE_HIGHER_THAN_LEVEL_OF_PORT_MEP;
                            }
                        } else {
                            // Both are VLAN MEPs on the same port and VLAN.
                            // That's not supported.
                            return VTSS_APPL_CFM_RC_MA_ONLY_ONE_VLAN_MEP_PER_PORT_PER_VLAN;
                        }
                    }
                }
            }
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_capabilities_get()
/******************************************************************************/
mesa_rc vtss_appl_cfm_capabilities_get(vtss_appl_cfm_capabilities_t *cap)
{
    VTSS_RC(CFM_ptr_check(cap));
    *cap = CFM_cap;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_global_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_cfm_global_conf_default_get(vtss_appl_cfm_global_conf_t *conf)
{
    VTSS_RC(CFM_ptr_check(conf));

    memset(conf, 0, sizeof(*conf));

    conf->sender_id_tlv_option             = VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DISABLE;

    // By default, use the Port Status TLV option, because it will be able to
    // flag certain types of errors (when enableRMEPdefect is false)
    conf->port_status_tlv_option           = VTSS_APPL_CFM_TLV_OPTION_ENABLE;
    conf->interface_status_tlv_option      = VTSS_APPL_CFM_TLV_OPTION_DISABLE;
    conf->organization_specific_tlv_option = VTSS_APPL_CFM_TLV_OPTION_DISABLE;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_global_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_cfm_global_conf_get(vtss_appl_cfm_global_conf_t *conf)
{
    VTSS_RC(CFM_ptr_check(conf));

    T_N("Enter");

    CFM_LOCK_SCOPE();

    *conf = CFM_global_conf;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_global_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_cfm_global_conf_set(const vtss_appl_cfm_global_conf_t *conf)
{
    vtss_appl_cfm_global_conf_t                     local_conf;
    const vtss_appl_cfm_organization_specific_tlv_t *tlv;
    vtss_appl_cfm_organization_specific_tlv_t       *local_tlv;
    cfm_md_conf_itr_t                               md_conf_itr;
    cfm_ma_conf_itr_t                               ma_conf_itr;
    cfm_mep_conf_itr_t                              mep_conf_itr;
    uint32_t                                        len = 0;

    VTSS_RC(CFM_ptr_check(conf));

    tlv = &conf->organization_specific_tlv;

    T_D("Enter, sender_id_tlv_option = %s, port_status_tlv_option = %s, interface_status_tlv_option = %s, organization_specific_tlv_option = %s (oui = %02x-%02x-%02x, subtype = %u, value_len = %u, value = \"%s\")",
        cfm_util_sender_id_tlv_option_to_str(conf->sender_id_tlv_option, false),
        cfm_util_tlv_option_to_str(conf->port_status_tlv_option, false),
        cfm_util_tlv_option_to_str(conf->interface_status_tlv_option, false),
        cfm_util_tlv_option_to_str(conf->organization_specific_tlv_option, false),
        tlv->oui[0],
        tlv->oui[1],
        tlv->oui[1],
        tlv->subtype,
        tlv->value_len,
        tlv->value /* May print garbage after value_len, because it's not necessarily NULL-terminated */);

    CFM_LOCK_SCOPE();

    if (conf->sender_id_tlv_option <  VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DISABLE ||
        conf->sender_id_tlv_option >= VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DEFER) {
        return VTSS_APPL_CFM_RC_INVALID_SENDER_ID_TLV_OPTION;
    }

    if (conf->port_status_tlv_option <  VTSS_APPL_CFM_TLV_OPTION_DISABLE ||
        conf->port_status_tlv_option >= VTSS_APPL_CFM_TLV_OPTION_DEFER) {
        return VTSS_APPL_CFM_RC_INVALID_PORT_STATUS_TLV_OPTION;
    }

    if (conf->interface_status_tlv_option <  VTSS_APPL_CFM_TLV_OPTION_DISABLE ||
        conf->interface_status_tlv_option >= VTSS_APPL_CFM_TLV_OPTION_DEFER) {
        return VTSS_APPL_CFM_RC_INVALID_INTERFACE_STATUS_TLV_OPTION;
    }

    if (conf->organization_specific_tlv_option <  VTSS_APPL_CFM_TLV_OPTION_DISABLE ||
        conf->organization_specific_tlv_option >= VTSS_APPL_CFM_TLV_OPTION_DEFER) {
        return VTSS_APPL_CFM_RC_INVALID_ORGANIZATION_SPECIFIC_TLV_OPTION;
    }

    if (conf->organization_specific_tlv_option == VTSS_APPL_CFM_TLV_OPTION_ENABLE) {
        len = tlv->value_len;

        if (len > sizeof(tlv->value)) {
            return VTSS_APPL_CFM_RC_ORG_SPEC_TLV_VAL_TOO_LONG;
        }
    }

    // Gotta normalize user's conf, so that the strings are all-NULLs after
    // #value_len.
    local_conf = *conf;
    local_tlv = &local_conf.organization_specific_tlv;
    if (len < sizeof(local_tlv->value)) {
        memset(&local_tlv->value[len], 0, sizeof(local_tlv->value) - len);
    }

    if (conf->organization_specific_tlv_option != VTSS_APPL_CFM_TLV_OPTION_ENABLE) {
        // Also reset other fields if not used - all to be able to do a memcmp.
        memset(local_tlv->oui, 0, sizeof(local_tlv->oui));
        local_tlv->subtype   = 0;
        local_tlv->value_len = 0;
    }

    if (memcmp(&local_conf, &CFM_global_conf, sizeof(local_conf)) == 0) {
        return VTSS_RC_OK;
    }

    CFM_global_conf = local_conf;

    // Loop through all MEPs and update them
    for (md_conf_itr = CFM_md_conf_map.begin(); md_conf_itr != CFM_md_conf_map.end(); ++md_conf_itr) {
        for (ma_conf_itr = md_conf_itr->second.ma_confs.begin(); ma_conf_itr != md_conf_itr->second.ma_confs.end(); ++ma_conf_itr) {
            for (mep_conf_itr = ma_conf_itr->second.mep_confs.begin(); mep_conf_itr != ma_conf_itr->second.mep_confs.end(); ++mep_conf_itr) {
                // Changes to the global conf should not cause the MEPs' state
                // machines to reset, but the MEPs should indeed be updated.
                VTSS_RC(CFM_mep_conf_update(mep_conf_itr->first, CFM_MEP_STATE_CHANGE_CONF_NO_RESET));
            }
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_md_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_cfm_md_conf_default_get(vtss_appl_cfm_md_conf_t *conf)
{
    VTSS_RC(CFM_ptr_check(conf));

    memset(conf, 0, sizeof(*conf));

    // 12.14.1.2.2.a. Default format is type 4 (String) and default name is
    // "DEFAULT".
    conf->format                           = VTSS_APPL_CFM_MD_FORMAT_STRING;
    strcpy(conf->name, "DEFAULT");

    // 12.14.1.2.2.b. Default level is 0.
    conf->level                            = 0;
    conf->sender_id_tlv_option             = VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DEFER;
    conf->port_status_tlv_option           = VTSS_APPL_CFM_TLV_OPTION_DEFER;
    conf->interface_status_tlv_option      = VTSS_APPL_CFM_TLV_OPTION_DEFER;
    conf->organization_specific_tlv_option = VTSS_APPL_CFM_TLV_OPTION_DEFER;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_md_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_cfm_md_conf_get(const vtss_appl_cfm_md_key_t &key, vtss_appl_cfm_md_conf_t *conf)
{
    cfm_md_conf_itr_t md_conf_itr;

    VTSS_RC(CFM_key_check(key));
    VTSS_RC(CFM_ptr_check(conf));

    T_N("Enter, %s", key);

    CFM_LOCK_SCOPE();

    if ((md_conf_itr = CFM_md_conf_map.find(key)) == CFM_md_conf_map.end()) {
        return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
    }

    *conf = md_conf_itr->second.md_conf;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_md_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_cfm_md_conf_set(const vtss_appl_cfm_md_key_t &key, const vtss_appl_cfm_md_conf_t *conf)
{
    vtss_appl_cfm_md_conf_t local_conf;
    cfm_md_conf_itr_t       md_conf_itr;
    cfm_ma_conf_itr_t       ma_conf_itr;
    cfm_mep_state_change_t  change;
    const char              *p;
    size_t                  len = 0;
    mesa_rc                 rc;

    VTSS_RC(CFM_key_check(key));
    VTSS_RC(CFM_ptr_check(conf));

    T_D("Enter, key = %s, format = %s, name = \"%s\", level = %u, sender_id_tlv_option = %s, port_status_tlv_option = %s, interface_status_tlv_option = %s, organization_specific_tlv_option = %s",
        key, conf->format, conf->name, conf->level,
        cfm_util_sender_id_tlv_option_to_str(conf->sender_id_tlv_option, false),
        cfm_util_tlv_option_to_str(conf->port_status_tlv_option, false),
        cfm_util_tlv_option_to_str(conf->interface_status_tlv_option, false),
        cfm_util_tlv_option_to_str(conf->organization_specific_tlv_option, false));

    // Check configuration.
    if (conf->format != VTSS_APPL_CFM_MD_FORMAT_NONE &&
        conf->format != VTSS_APPL_CFM_MD_FORMAT_STRING) {
        return VTSS_APPL_CFM_RC_MD_UNSUPPORTED_FORMAT;
    }

    if (conf->format == VTSS_APPL_CFM_MD_FORMAT_STRING) {
        len = strlen(conf->name);

        if (len < 1 || len >= sizeof(conf->name)) {
            return VTSS_APPL_CFM_RC_MD_INVALID_NAME_LENGTH;
        }

        p = conf->name;
        while (*p != '\0') {
            // Must be in range [32; 126] == isprint()
            if (!isprint(*(p++))) {
                return VTSS_APPL_CFM_RC_MD_INVALID_NAME_CONTENTS;
            }
        }
    }

    if (conf->level > 7) {
        return VTSS_APPL_CFM_RC_MD_INVALID_LEVEL;
    }

    if (conf->sender_id_tlv_option < VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DISABLE ||
        conf->sender_id_tlv_option > VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DEFER) {
        return VTSS_APPL_CFM_RC_INVALID_SENDER_ID_TLV_OPTION;
    }

    if (conf->port_status_tlv_option < VTSS_APPL_CFM_TLV_OPTION_DISABLE ||
        conf->port_status_tlv_option > VTSS_APPL_CFM_TLV_OPTION_DEFER) {
        return VTSS_APPL_CFM_RC_INVALID_PORT_STATUS_TLV_OPTION;
    }

    if (conf->interface_status_tlv_option < VTSS_APPL_CFM_TLV_OPTION_DISABLE ||
        conf->interface_status_tlv_option > VTSS_APPL_CFM_TLV_OPTION_DEFER) {
        return VTSS_APPL_CFM_RC_INVALID_INTERFACE_STATUS_TLV_OPTION;
    }

    if (conf->organization_specific_tlv_option != VTSS_APPL_CFM_TLV_OPTION_DISABLE &&
        conf->organization_specific_tlv_option != VTSS_APPL_CFM_TLV_OPTION_DEFER) {
        // Cannot use VTSS_APPL_CFM_TLV_OPTION_EANBLE, because its contents
        // comes from the global configuration, where it may be disabled, so the
        // contents is invalid.
        return VTSS_APPL_CFM_RC_INVALID_ORGANIZATION_SPECIFIC_TLV_OPTION;
    }

    // Gotta normalize user's conf, so that the string is all-NULLs if format
    // is not using it and all-NULLs after terminating NULL if format is using
    // it.
    local_conf = *conf;
    memset(&local_conf.name[len], 0, sizeof(local_conf.name) - len);

    {
        CFM_LOCK_SCOPE();

        if ((md_conf_itr = CFM_md_conf_map.find(key)) != CFM_md_conf_map.end()) {
            if (memcmp(&local_conf, &md_conf_itr->second, sizeof(local_conf)) == 0) {
                // No changes.
                return VTSS_RC_OK;
            }
        }

        if (md_conf_itr == CFM_md_conf_map.end()) {
            // Check that we haven't created more MDs than we can allow
            if (CFM_md_conf_map.size() >= CFM_cap.md_cnt_max) {
                return VTSS_APPL_CFM_RC_MD_LIMIT_REACHED;
            }
        } else {
            // Modifying an existing. Check that it's possible to combine the MD
            // name with the MA name to form a valid MAID.
            for (ma_conf_itr = md_conf_itr->second.ma_confs.begin(); ma_conf_itr != md_conf_itr->second.ma_confs.end(); ++ma_conf_itr) {
                VTSS_RC(CFM_maid_verify(&local_conf, &ma_conf_itr->second.ma_conf, true));
            }

            if (md_conf_itr->second.md_conf.level != local_conf.level) {
                // Special things may happen when you change level. Check that
                // it's OK.
                VTSS_RC(CFM_md_level_change_check(md_conf_itr, local_conf.level));
            }
        }

        change = CFM_md_conf_change_get(md_conf_itr == CFM_md_conf_map.end() ? nullptr : &md_conf_itr->second.md_conf, &local_conf);

        // Create a new or update an existing entry
        if ((md_conf_itr = CFM_md_conf_map.get(key)) == CFM_md_conf_map.end()) {
            return VTSS_APPL_CFM_RC_OUT_OF_MEMORY;
        }

        md_conf_itr->second.md_conf = local_conf;

        rc = CFM_md_conf_update(md_conf_itr, change);
    }

    CFM_conf_change_callbacks_invoke();
    return rc;
}

/******************************************************************************/
// vtss_appl_cfm_md_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_cfm_md_conf_del(const vtss_appl_cfm_md_key_t &key)
{
    cfm_md_conf_itr_t md_conf_itr;
    cfm_ma_conf_itr_t ma_conf_itr;

    VTSS_RC(CFM_key_check(key));

    T_D("Enter, %s", key);

    {
        CFM_LOCK_SCOPE();

        if ((md_conf_itr = CFM_md_conf_map.find(key)) == CFM_md_conf_map.end()) {
            return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
        }

        // Find all MAs that belong to this MD
        ma_conf_itr = md_conf_itr->second.ma_confs.begin();
        while (ma_conf_itr != md_conf_itr->second.ma_confs.end()) {
            cfm_ma_conf_itr_t ma_conf_itr_next = ma_conf_itr;
            ++ma_conf_itr_next;

            // Delete the MA's MEPs
            VTSS_RC(CFM_ma_conf_del_meps(ma_conf_itr));

            // Delete the MA. No need to call CFM_ma_conf_update() afterwards,
            // because no MEPs exist in this MA anymore.
            md_conf_itr->second.ma_confs.erase(ma_conf_itr);

            ma_conf_itr = ma_conf_itr_next;
        }

        // Delete the MD. No need to call CFM_md_conf_update() afterwards,
        // because no MAs exist in this MD anymore.
        CFM_md_conf_map.erase(md_conf_itr);
    }

    CFM_conf_change_callbacks_invoke();
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_md_itr()
/******************************************************************************/
mesa_rc vtss_appl_cfm_md_itr(const vtss_appl_cfm_md_key_t *prev_key, vtss_appl_cfm_md_key_t *next_key)
{
    cfm_md_conf_itr_t md_conf_itr;

    VTSS_RC(CFM_ptr_check(next_key));

    CFM_LOCK_SCOPE();

    if (prev_key) {
        // Here we have a valid prev_key. Find the next from that one.
        md_conf_itr = CFM_md_conf_map.greater_than(*prev_key);
    } else {
        // We don't have a valid prev_key. Get the first.
        md_conf_itr = CFM_md_conf_map.begin();
    }

    if (md_conf_itr != CFM_md_conf_map.end()) {
        *next_key = md_conf_itr->first;
        return VTSS_RC_OK;
    }

    // No next
    return VTSS_APPL_CFM_RC_END_OF_LIST;
}

/******************************************************************************/
// vtss_appl_cfm_ma_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_cfm_ma_conf_default_get(vtss_appl_cfm_ma_conf_t *conf)
{
    VTSS_RC(CFM_ptr_check(conf));

    memset(conf, 0, sizeof(*conf));

    // 12.14.5.3.2.b: Use primary VID (underlying format is two-octet integer)
    conf->format = VTSS_APPL_CFM_MA_FORMAT_PRIMARY_VID;

    // 12.14.5.3.2.b: Should be the primary VID, but we don't know that yet.
    // RBNTBD: Make this an option when specifying TWO_OCTET_INTEGER as format
    conf->name[0] = '\0';
    conf->name[1] = '\0';

    conf->vlan                             = 0; // Port MEPs
    conf->ccm_interval                     = VTSS_APPL_CFM_CCM_INTERVAL_1S;
    conf->sender_id_tlv_option             = VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DEFER;
    conf->port_status_tlv_option           = VTSS_APPL_CFM_TLV_OPTION_DEFER;
    conf->interface_status_tlv_option      = VTSS_APPL_CFM_TLV_OPTION_DEFER;
    conf->organization_specific_tlv_option = VTSS_APPL_CFM_TLV_OPTION_DEFER;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_ma_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_cfm_ma_conf_get(const vtss_appl_cfm_ma_key_t &key, vtss_appl_cfm_ma_conf_t *conf)
{
    cfm_md_conf_itr_t md_conf_itr;
    cfm_ma_conf_itr_t ma_conf_itr;

    VTSS_RC(CFM_key_check(key));
    VTSS_RC(CFM_ptr_check(conf));

    T_N("Enter, %s", key);

    CFM_LOCK_SCOPE();

    if ((md_conf_itr = CFM_md_conf_map.find(key)) == CFM_md_conf_map.end()) {
        return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
    }

    if ((ma_conf_itr = md_conf_itr->second.ma_confs.find(key)) == md_conf_itr->second.ma_confs.end()) {
        return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
    }

    *conf = ma_conf_itr->second.ma_conf;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_ma_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_cfm_ma_conf_set(const vtss_appl_cfm_ma_key_t &key, const vtss_appl_cfm_ma_conf_t *conf)
{
    vtss_appl_cfm_ma_conf_t local_conf;
    cfm_md_conf_itr_t       md_conf_itr;
    cfm_ma_conf_itr_t       ma_conf_itr;
    cfm_mep_conf_itr_t      mep_conf_itr;
    cfm_mep_state_change_t  change;
    const char              *p;
    size_t                  len, cnt;
    bool                    slash_seen;
    mesa_rc                 rc;

    VTSS_RC(CFM_key_check(key));
    VTSS_RC(CFM_ptr_check(conf));

    T_D("Enter, %s, format = %s, name = \"%s\", vlan = %u, ccm interval = %u, sender_id_tlv_option = %s, port_status_tlv_option = %s, interface_status_tlv_option = %s, organization_specific_tlv_option = %s",
        key, cfm_util_ma_format_to_str(conf->format, false), conf->name, conf->vlan, conf->ccm_interval,
        cfm_util_sender_id_tlv_option_to_str(conf->sender_id_tlv_option, false),
        cfm_util_tlv_option_to_str(conf->port_status_tlv_option, false),
        cfm_util_tlv_option_to_str(conf->interface_status_tlv_option, false),
        cfm_util_tlv_option_to_str(conf->organization_specific_tlv_option, false));

    // Check configuration.
    len = strlen(conf->name);
    p   = conf->name;
    switch (conf->format) {
    case VTSS_APPL_CFM_MA_FORMAT_TWO_OCTET_INTEGER:
        // name[0] and name[1] used.
        len = 2;
        break;

    case VTSS_APPL_CFM_MA_FORMAT_PRIMARY_VID:
        // Primary VID is used, so no bytes from conf->name.
        len = 0;
        break;

    case VTSS_APPL_CFM_MA_FORMAT_STRING:
        if (len < 1 || len >= sizeof(conf->name)) {
            return VTSS_APPL_CFM_RC_MA_INVALID_NAME_STRING_LENGTH;
        }

        while (*p != '\0') {
            // Must be in range [32; 126] == isprint()
            if (!isprint(*(p++))) {
                return VTSS_APPL_CFM_RC_MA_INVALID_NAME_STRING_CONTENTS;
            }
        }

        break;

    case VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC:
        if (len != 13) {
            return VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_LENGTH;
        }

        while (*p != '\0') {
            // Must be in range [a-zA-Z0-9] == isalnum()
            if (!isalnum(*(p++))) {
                return VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_CONTENTS;
            }
        }

        break;

    case VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC_CC:
        if (len != 15) {
            return VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_CC_LENGTH;
        }

        cnt        = 0;
        slash_seen = false;
        while (*p != '\0') {
            if (cnt < 2) {
                // CC. Must be[A-Z] == isupper()
                if (!isupper(*p)) {
                    return VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_CC_CONTENTS_FIRST;
                }
            } else if (cnt < 8 && *p == '/') {
                // One '/' may be present in name[3]-name[7].
                if (slash_seen) {
                    return VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_CC_CONTENTS_SLASH;
                }

                slash_seen = true;
            } else if (!isalnum(*p)) {
                // Must be in range [a-zA-Z0-9] == isalnum()
                return VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_CC_CONTENTS_LAST;
            }

            p++;
            cnt++;
        }

        break;

    default:
        return VTSS_APPL_CFM_RC_MA_UNSUPPORTED_FORMAT;
    }

    if (conf->vlan != 0 && !CFM_cap.has_vlan_meps) {
        return VTSS_APPL_CFM_RC_MA_VLAN_MEPS_NOT_SUPPORTED;
    }

    if (conf->vlan != 0 && (conf->vlan < VTSS_APPL_VLAN_ID_MIN || conf->vlan > VTSS_APPL_VLAN_ID_MAX)) {
        // VID must be 0 or in range [VTSS_APPL_VLAN_ID_MIN; VTSS_APPL_VLAN_ID_MAX]
        return VTSS_APPL_CFM_RC_MA_INVALID_VLAN;
    }

    if (conf->ccm_interval < VTSS_APPL_CFM_CCM_INTERVAL_300HZ || conf->ccm_interval > VTSS_APPL_CFM_CCM_INTERVAL_10MIN) {
        return VTSS_APPL_CFM_RC_MA_INVALID_CCM_INTERVAL;
    }

    if (conf->ccm_interval < CFM_cap.ccm_interval_min || conf->ccm_interval > CFM_cap.ccm_interval_max) {
        return VTSS_APPL_CFM_RC_MA_CCM_INTERVAL_NOT_SUPPORTED;
    }

    if (conf->sender_id_tlv_option < VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DISABLE ||
        conf->sender_id_tlv_option > VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DEFER) {
        return VTSS_APPL_CFM_RC_INVALID_SENDER_ID_TLV_OPTION;
    }

    if (conf->port_status_tlv_option < VTSS_APPL_CFM_TLV_OPTION_DISABLE ||
        conf->port_status_tlv_option > VTSS_APPL_CFM_TLV_OPTION_DEFER) {
        return VTSS_APPL_CFM_RC_INVALID_PORT_STATUS_TLV_OPTION;
    }

    if (conf->interface_status_tlv_option < VTSS_APPL_CFM_TLV_OPTION_DISABLE ||
        conf->interface_status_tlv_option > VTSS_APPL_CFM_TLV_OPTION_DEFER) {
        return VTSS_APPL_CFM_RC_INVALID_INTERFACE_STATUS_TLV_OPTION;
    }

    if (conf->organization_specific_tlv_option != VTSS_APPL_CFM_TLV_OPTION_DISABLE &&
        conf->organization_specific_tlv_option != VTSS_APPL_CFM_TLV_OPTION_DEFER) {
        // Cannot use VTSS_APPL_CFM_TLV_OPTION_EANBLE, because its contents
        // comes from the global configuration, where it may be disabled, so the
        // contents is invalid.
        return VTSS_APPL_CFM_RC_INVALID_ORGANIZATION_SPECIFIC_TLV_OPTION;
    }

    // Gotta normalize user's conf, so that the string is all-NULLs if format
    // is not using it and all-NULLs after terminating NULL if format is using
    // it.
    local_conf = *conf;
    memset(&local_conf.name[len], 0, sizeof(local_conf.name) - len);

    {
        CFM_LOCK_SCOPE();

        // Check that the MD exists.
        if ((md_conf_itr = CFM_md_conf_map.find(key)) == CFM_md_conf_map.end()) {
            return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
        }

        // Check that it's possible to combine the MD name with the MA name to
        // form an MAID/MEGID
        VTSS_RC(CFM_maid_verify(&md_conf_itr->second.md_conf, &local_conf, false));

        if ((ma_conf_itr = md_conf_itr->second.ma_confs.find(key)) != md_conf_itr->second.ma_confs.end()) {
            // The MA already exists.
            if (memcmp(&local_conf, &ma_conf_itr->second, sizeof(local_conf)) == 0) {
                // No changes.
                return VTSS_RC_OK;
            }

            if ((ma_conf_itr->second.ma_conf.vlan == 0 && local_conf.vlan != 0) ||
                (ma_conf_itr->second.ma_conf.vlan != 0 && local_conf.vlan == 0)) {
                // The MA is changing from containing Port MEPs to containing
                // VLAN MEPs or vice versa. Several checks apply when doing so.
                VTSS_RC(CFM_ma_type_change_check(md_conf_itr, ma_conf_itr, local_conf.vlan));
            }
        }

        // Check that we haven't created more MAs than we support within one MD.
        if (ma_conf_itr == md_conf_itr->second.ma_confs.end()) {
            // We are adding a new, not changing an existing.
            if (md_conf_itr->second.ma_confs.size() >= CFM_cap.ma_cnt_max) {
                return VTSS_APPL_CFM_RC_MA_LIMIT_REACHED;
            }
        }

        change = CFM_ma_conf_change_get(ma_conf_itr == md_conf_itr->second.ma_confs.end() ? nullptr : &ma_conf_itr->second.ma_conf, &local_conf);

        // Create a new or update an existing entry
        if ((ma_conf_itr = md_conf_itr->second.ma_confs.get(key)) == md_conf_itr->second.ma_confs.end()) {
            return VTSS_APPL_CFM_RC_OUT_OF_MEMORY;
        }

        ma_conf_itr->second.ma_conf = local_conf;

        // Activate any existing MEPs on this MA.
        rc = CFM_ma_conf_update(ma_conf_itr, change);
    }

    CFM_conf_change_callbacks_invoke();
    return rc;
}

/******************************************************************************/
// vtss_appl_cfm_ma_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_cfm_ma_conf_del(const vtss_appl_cfm_ma_key_t &key)
{
    cfm_md_conf_itr_t md_conf_itr;
    cfm_ma_conf_itr_t ma_conf_itr;

    VTSS_RC(CFM_key_check(key));

    T_D("Enter, %s", key);

    {
        CFM_LOCK_SCOPE();

        if ((md_conf_itr = CFM_md_conf_map.find(key)) == CFM_md_conf_map.end()) {
            return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
        }

        if ((ma_conf_itr = md_conf_itr->second.ma_confs.find(key)) == md_conf_itr->second.ma_confs.end()) {
            return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
        }

        // Delete all MEPs belonging to this MA
        VTSS_RC(CFM_ma_conf_del_meps(ma_conf_itr));

        // Delete the MA. No need to call CFM_ma_conf_update() afterwards,
        // because no MEPs exist in this MA anymore.
        md_conf_itr->second.ma_confs.erase(ma_conf_itr);
    }

    CFM_conf_change_callbacks_invoke();
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_ma_itr()
/******************************************************************************/
mesa_rc vtss_appl_cfm_ma_itr(const vtss_appl_cfm_ma_key_t *prev_key, vtss_appl_cfm_ma_key_t *next_key, bool stay_in_this_md)
{
    cfm_md_conf_itr_t        md_conf_itr;
    cfm_ma_conf_itr_t        ma_conf_itr;
    vtss_appl_cfm_ma_key_t   key;

    VTSS_RC(CFM_ptr_check(next_key));

    key.md = prev_key ? prev_key->md : "";
    key.ma = prev_key ? prev_key->ma : "";

    CFM_LOCK_SCOPE();

    for (md_conf_itr = CFM_md_conf_map.greater_than_or_equal(key); md_conf_itr != CFM_md_conf_map.end(); ++md_conf_itr) {
        if (md_conf_itr->first.md != key.md) {
            if (stay_in_this_md) {
                // We have been asked not to enter another MD.
                return VTSS_APPL_CFM_RC_END_OF_LIST;
            }

            // Start over.
            key.ma = "";
        }

        if ((ma_conf_itr = md_conf_itr->second.ma_confs.greater_than(key)) != md_conf_itr->second.ma_confs.end()) {
            *next_key = ma_conf_itr->first;
            return VTSS_RC_OK;
        }
    }

    // No next
    return VTSS_APPL_CFM_RC_END_OF_LIST;
}

/******************************************************************************/
// vtss_appl_cfm_mep_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_cfm_mep_conf_default_get(vtss_appl_cfm_mep_conf_t *conf)
{
    VTSS_RC(CFM_ptr_check(conf));

    memset(conf, 0, sizeof(*conf));
    conf->direction             = VTSS_APPL_CFM_DIRECTION_DOWN;
    conf->vlan                  = 0; // Inherit from MA
    conf->pcp                   = 0;
    conf->admin_active          = false;
    conf->ccm_enable            = false;
    conf->alarm_level           = 2;
    conf->alarm_time_present_ms = 2500;
    conf->alarm_time_absent_ms  = 10000;

    if (vtss_ifindex_from_port(VTSS_ISID_LOCAL, VTSS_PORT_NO_START, &conf->ifindex) != VTSS_RC_OK) {
        T_E("Unable to convert <isid, port> = <%u, %u> to ifindex", VTSS_ISID_LOCAL, VTSS_PORT_NO_START);
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_mep_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_cfm_mep_conf_get(const vtss_appl_cfm_mep_key_t &key, vtss_appl_cfm_mep_conf_t *conf)
{
    cfm_mep_conf_itr_t mep_conf_itr;

    VTSS_RC(CFM_key_check(key));
    VTSS_RC(CFM_ptr_check(conf));

    T_N("Enter, %s", key);

    CFM_LOCK_SCOPE();

    VTSS_RC(CFM_mep_itr_get(key, mep_conf_itr));

    *conf = mep_conf_itr->second.mep_conf;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_mep_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_cfm_mep_conf_set(const vtss_appl_cfm_mep_key_t &key, const vtss_appl_cfm_mep_conf_t *conf)
{
    vtss_ifindex_elm_t     ife;
    cfm_md_conf_itr_t      md_conf_itr,  md_conf_itr2;
    cfm_ma_conf_itr_t      ma_conf_itr,  ma_conf_itr2;
    cfm_mep_conf_itr_t     mep_conf_itr, mep_conf_itr2;
    cfm_mep_state_change_t change;
    char                   mac_str[18];
    uint32_t               port_mep_cnt = 0;
    uint32_t               vlan_mep_cnt = 0;
    bool                   new_is_port_mep, other_is_port_mep;
    mesa_vid_t             this_vlan, other_vlan;
    mesa_rc                rc;

    VTSS_RC(CFM_key_check(key));
    VTSS_RC(CFM_ptr_check(conf));

    T_D("Enter, %s, direction = %s, ifindex = %u, vlan = %u, pcp = %u, smac = %s, ccm_enable = %d,"
        " alarm_level = %u, alarm_time_present_ms = %u, alarm_time_absent_ms = %u, admin_active = %d",
        key,
        cfm_util_direction_to_str(conf->direction, false),
        VTSS_IFINDEX_PRINTF_ARG(conf->ifindex),
        conf->vlan,
        conf->pcp,
        misc_mac_txt(conf->smac.addr, mac_str),
        conf->ccm_enable,
        conf->alarm_level,
        conf->alarm_time_present_ms,
        conf->alarm_time_absent_ms,
        conf->admin_active);

    // Check configuration.
    if (vtss_ifindex_decompose(conf->ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        return VTSS_APPL_CFM_RC_MEP_INVALID_IFINDEX;
    }

    if (conf->direction != VTSS_APPL_CFM_DIRECTION_DOWN && conf->direction != VTSS_APPL_CFM_DIRECTION_UP) {
        return VTSS_APPL_CFM_RC_MEP_INVALID_DIRECTION;
    }

    if (conf->direction == VTSS_APPL_CFM_DIRECTION_UP && !CFM_cap.has_up_meps) {
        return VTSS_APPL_CFM_RC_MEP_DIRECTION_NOT_SUPPORTED_ON_THIS_PLATFORM;
    }

    if (conf->vlan != 0 && (conf->vlan < VTSS_APPL_VLAN_ID_MIN || conf->vlan > VTSS_APPL_VLAN_ID_MAX)) {
        // VID must be 0 or in range
        // [VTSS_APPL_VLAN_ID_MIN; VTSS_APPL_VLAN_ID_MAX]
        return VTSS_APPL_CFM_RC_MEP_INVALID_VLAN;
    }

    if (conf->pcp > 7) {
        return VTSS_APPL_CFM_RC_MEP_INVALID_PCP;
    }

    if (conf->smac.addr[0] & 0x1) {
        // SMAC must not be a multicast SMAC
        return VTSS_APPL_CFM_RC_MEP_INVALID_SMAC;
    }

    if (conf->alarm_level < 1 || conf->alarm_level > 6) {
        return VTSS_APPL_CFM_RC_MEP_INVALID_ALARM_LEVEL;
    }

    if (conf->alarm_time_present_ms < 2500 || conf->alarm_time_present_ms > 10000) {
        return VTSS_APPL_CFM_RC_MEP_INVALID_ALARM_TIME_PRESENT;
    }

    if (conf->alarm_time_absent_ms < 2500 || conf->alarm_time_absent_ms > 10000) {
        return VTSS_APPL_CFM_RC_MEP_INVALID_ALARM_TIME_ABSENT;
    }

    {
        CFM_LOCK_SCOPE();

        if ((md_conf_itr = CFM_md_conf_map.find(key)) == CFM_md_conf_map.end()) {
            return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
        }

        if ((ma_conf_itr = md_conf_itr->second.ma_confs.find(key)) == md_conf_itr->second.ma_confs.end()) {
            return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
        }

        if ((mep_conf_itr = ma_conf_itr->second.mep_confs.find(key)) != ma_conf_itr->second.mep_confs.end()) {
            if (memcmp(conf, &mep_conf_itr->second, sizeof(*conf)) == 0) {
                // No changes
                return VTSS_RC_OK;
            }
        }

        if (conf->admin_active) {
            // Check to see if enabling this MEP causes the total number of port
            // or VLAN MEPs to be exceeded.
            // To get started, we have to feed the initial number of port and
            // VLAN MEPs given this new configuration into the function below.
            // Figure out whether it becomes a port or a VLAN MEP.
            if (ma_conf_itr->second.ma_conf.vlan == 0) {
                // Port MEP
                port_mep_cnt = 1;
            } else {
                // VLAN MEP
                vlan_mep_cnt = 1;
            }

            // Then call the common function passing the initial counts and ask
            // it not to cound the configuration we are currently changing.
            VTSS_RC(CFM_port_or_service_mep_cnt_exceeded_chk(nullptr, mep_conf_itr == ma_conf_itr->second.mep_confs.end() ? nullptr /* new MEP */ : &mep_conf_itr->second /* existing MEP */, port_mep_cnt, vlan_mep_cnt));
        }

        // Loop across all MDs and MAs and MEPs to see if this clashes with
        // another MEP.
        for (md_conf_itr2 = CFM_md_conf_map.begin(); md_conf_itr2 != CFM_md_conf_map.end(); ++md_conf_itr2) {
            for (ma_conf_itr2 = md_conf_itr2->second.ma_confs.begin(); ma_conf_itr2 != md_conf_itr2->second.ma_confs.end(); ++ma_conf_itr2) {
                for (mep_conf_itr2 = ma_conf_itr2->second.mep_confs.begin(); mep_conf_itr2 != ma_conf_itr2->second.mep_confs.end(); ++mep_conf_itr2) {
                    vtss_appl_cfm_mep_conf_t *other_conf;

                    if (mep_conf_itr2 == mep_conf_itr) {
                        // It's fine to change our own configuration
                        continue;
                    }

                    other_conf = &mep_conf_itr2->second.mep_conf;

                    if (!other_conf->admin_active) {
                        // Don't count it as long as it hasn't been put into
                        // use, because this function may be called many times
                        // (from e.g. ICLI) before a MEP is really becoming
                        // configured.
                        continue;
                    }

                    if (!conf->admin_active) {
                        // Nothing else to check
                        continue;
                    }

                    if (md_conf_itr2 == md_conf_itr && ma_conf_itr2 == ma_conf_itr) {
                        // All MEPs in the same MA must have the same direction.
                        // See clause 22.2.2 just below figure 22-9.
                        if (other_conf->direction != conf->direction) {
                            return VTSS_APPL_CFM_RC_MEP_ALL_MEPS_IN_MA_MUST_HAVE_SAME_DIRECTION;
                        }
                    }

                    new_is_port_mep   = ma_conf_itr->second.ma_conf.vlan  == 0;
                    other_is_port_mep = ma_conf_itr2->second.ma_conf.vlan == 0;

                    if (CFM_global_state.has_vop_v1) {
                        // On Serval-1, all Port MEPs must have same level, or
                        // we cannot avoid leaking.
                        if (new_is_port_mep && other_is_port_mep && md_conf_itr->second.md_conf.level != md_conf_itr2->second.md_conf.level) {
                            return VTSS_APPL_CFM_RC_MEP_ALL_PORT_MEPS_MUST_HAVE_SAME_LEVEL;
                        }

                        // On Serval-1, a Port MEP must have lower level than
                        // any VLAN MEP, or we cannot avoid leaking.
                        if (new_is_port_mep && !other_is_port_mep && md_conf_itr->second.md_conf.level >= md_conf_itr2->second.md_conf.level) {
                            return VTSS_APPL_CFM_RC_MEP_PORT_MEP_MUST_HAVE_LOWER_LEVEL_THAN_ANY_VLAN_MEP;
                        }

                        // On Serval-1, a VLAN MEP must have higher level than
                        // any Port MEP, or we cannot avoid leaking.
                        if (!new_is_port_mep && other_is_port_mep && md_conf_itr->second.md_conf.level <= md_conf_itr2->second.md_conf.level) {
                            return VTSS_APPL_CFM_RC_MEP_VLAN_MEP_MUST_HAVE_HIGHER_LEVEL_THAN_PORT_MEPS;
                        }
                    }

                    if (conf->ifindex != other_conf->ifindex) {
                        // The two MEPs are not on the same port. Go on.
                        continue;
                    }

                    // There are further restrictions on two MEPs on the same
                    // port.
                    this_vlan  = conf->vlan       != 0 ? conf->vlan       : ma_conf_itr->second.ma_conf.vlan;
                    other_vlan = other_conf->vlan != 0 ? other_conf->vlan : ma_conf_itr2->second.ma_conf.vlan;

                    T_I("new = %s, other = %s, new_is_port_mep = %d, other_is_port_mep = %d, this_vlan = %u, other_vlan = %u",
                        key, mep_conf_itr2->first, new_is_port_mep, other_is_port_mep, this_vlan, other_vlan);

                    // Check that only one port MEP per port is created
                    if (new_is_port_mep && other_is_port_mep) {
                        return VTSS_APPL_CFM_RC_MEP_ONLY_ONE_PORT_MEP_PER_PORT;
                    } else if (this_vlan == other_vlan) {
                        if (new_is_port_mep) {
                            // New is a Port MEP and the other is a VLAN MEP.
                            // The level of the Port MEP must be lower than that
                            // of the VLAN MEP.
                            if (md_conf_itr->second.md_conf.level >= md_conf_itr2->second.md_conf.level) {
                                return VTSS_APPL_CFM_RC_MEP_LEVEL_OF_PORT_MEP_MUST_BE_LOWER_THAN_LEVEL_OF_VLAN_MEP;
                            }
                        } else if (other_is_port_mep) {
                            // New is a VLAN MEP and the other is a Port MEP.
                            // The level of the VLAN MEP must be higher than
                            // that of the Port MEP.
                            if (md_conf_itr->second.md_conf.level <= md_conf_itr2->second.md_conf.level) {
                                return VTSS_APPL_CFM_RC_MEP_LEVEL_OF_VLAN_MEP_MUST_BE_HIGHER_THAN_LEVEL_OF_PORT_MEP;
                            }
                        } else {
                            // Both are VLAN MEPs on the same port and VLAN.
                            // That's not supported.
                            return VTSS_APPL_CFM_RC_MEP_ONLY_ONE_VLAN_MEP_PER_PORT_PER_VLAN;
                        }
                    }
                }
            }
        }

        change = CFM_mep_conf_change_get(mep_conf_itr == ma_conf_itr->second.mep_confs.end() ? nullptr : &mep_conf_itr->second.mep_conf, conf);

        if (mep_conf_itr == ma_conf_itr->second.mep_confs.end()) {
            // Create a new entry
            if ((mep_conf_itr = ma_conf_itr->second.mep_confs.get(key)) == ma_conf_itr->second.mep_confs.end()) {
                return VTSS_APPL_CFM_RC_OUT_OF_MEMORY;
            }
        }

        mep_conf_itr->second.mep_conf = *conf;

        // Update the MEP
        rc = CFM_mep_conf_update(mep_conf_itr->first, change);
    }

    CFM_conf_change_callbacks_invoke();
    return rc;
}

/******************************************************************************/
// vtss_appl_cfm_mep_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_cfm_mep_conf_del(const vtss_appl_cfm_mep_key_t &key)
{
    cfm_md_conf_itr_t  md_conf_itr;
    cfm_ma_conf_itr_t  ma_conf_itr;
    cfm_mep_conf_itr_t mep_conf_itr;
    mesa_rc            rc;

    VTSS_RC(CFM_key_check(key));

    T_D("Enter, %s", key);

    {
        CFM_LOCK_SCOPE();

        if ((md_conf_itr = CFM_md_conf_map.find(key)) == CFM_md_conf_map.end()) {
            return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
        }

        if ((ma_conf_itr = md_conf_itr->second.ma_confs.find(key)) == md_conf_itr->second.ma_confs.end()) {
            return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
        }

        if ((mep_conf_itr = ma_conf_itr->second.mep_confs.find(key)) == ma_conf_itr->second.mep_confs.end()) {
            return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
        }

        // Delete MEP from API and from map
        ma_conf_itr->second.mep_confs.erase(mep_conf_itr);
        rc = CFM_mep_conf_update(key, CFM_MEP_STATE_CHANGE_CONF);
    }

    CFM_conf_change_callbacks_invoke();
    return rc;
}

/******************************************************************************/
// vtss_appl_cfm_mep_itr()
/******************************************************************************/
mesa_rc vtss_appl_cfm_mep_itr(const vtss_appl_cfm_mep_key_t *prev_key, vtss_appl_cfm_mep_key_t *next_key, bool stay_in_this_ma)
{
    cfm_md_conf_itr_t       md_conf_itr;
    cfm_ma_conf_itr_t       ma_conf_itr;
    cfm_mep_conf_itr_t      mep_conf_itr;
    vtss_appl_cfm_mep_key_t key;

    VTSS_RC(CFM_ptr_check(next_key));

    key.md    = prev_key ? prev_key->md    : "";
    key.ma    = prev_key ? prev_key->ma    : "";
    key.mepid = prev_key ? prev_key->mepid : 0;

    CFM_LOCK_SCOPE();

    for (md_conf_itr = CFM_md_conf_map.greater_than_or_equal(key); md_conf_itr != CFM_md_conf_map.end(); ++md_conf_itr) {
        if (md_conf_itr->first.md != key.md) {
            if (stay_in_this_ma) {
                // We have been asked not to enter another MD
                return VTSS_APPL_CFM_RC_END_OF_LIST;
            }

            // Start over.
            key.ma = "";
        }

        for ((ma_conf_itr = md_conf_itr->second.ma_confs.greater_than_or_equal(key)); ma_conf_itr != md_conf_itr->second.ma_confs.end(); ++ma_conf_itr) {
            if (ma_conf_itr->first.ma != key.ma) {
                if (stay_in_this_ma) {
                    // We have been asked not to enter another MA
                    return VTSS_APPL_CFM_RC_END_OF_LIST;
                }

                // Start over.
                key.mepid = 0;
            }

            if ((mep_conf_itr = ma_conf_itr->second.mep_confs.greater_than(key)) != ma_conf_itr->second.mep_confs.end()) {
                *next_key = mep_conf_itr->first;
                return VTSS_RC_OK;
            }
        }
    }

    // No next
    return VTSS_APPL_CFM_RC_END_OF_LIST;
}

/******************************************************************************/
// vtss_appl_cfm_rmep_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_cfm_rmep_conf_default_get(vtss_appl_cfm_rmep_conf_t *conf)
{
    VTSS_RC(CFM_ptr_check(conf));
    memset(conf, 0, sizeof(*conf));

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_rmep_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_cfm_rmep_conf_get(const vtss_appl_cfm_rmep_key_t &key, vtss_appl_cfm_rmep_conf_t *conf)
{
    cfm_rmep_conf_itr_t rmep_conf_itr;

    VTSS_RC(CFM_key_check(key));
    VTSS_RC(CFM_ptr_check(conf));

    T_N("Enter, %s", key);

    CFM_LOCK_SCOPE();

    VTSS_RC(CFM_rmep_itr_get(key, rmep_conf_itr));

    *conf = rmep_conf_itr->second;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_rmep_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_cfm_rmep_conf_set(const vtss_appl_cfm_rmep_key_t &key, const vtss_appl_cfm_rmep_conf_t *conf)
{
    cfm_mep_conf_itr_t  mep_conf_itr;
    cfm_rmep_conf_itr_t rmep_conf_itr;

    VTSS_RC(CFM_key_check(key));
    VTSS_RC(CFM_ptr_check(conf));

    T_N("Enter, %s", key);

    {
        CFM_LOCK_SCOPE();

        VTSS_RC(CFM_mep_itr_get(key, mep_conf_itr));

        if (key.rmepid == mep_conf_itr->first.mepid) {
            // The Remote MEP-ID cannot be the same as this MEP's ID.
            return VTSS_APPL_CFM_RC_RMEP_RMEPID_SAME_AS_MEPID;
        }

        if ((rmep_conf_itr = mep_conf_itr->second.rmep_confs.find(key)) != mep_conf_itr->second.rmep_confs.end()) {
            rmep_conf_itr->second = *conf;
            return VTSS_RC_OK;
        }

        if (mep_conf_itr->second.rmep_confs.size() >= CFM_cap.rmep_cnt_max) {
            return VTSS_APPL_CFM_RC_RMEP_LIMIT_REACHED;
        }

        // Create a new entry.
        if ((rmep_conf_itr = mep_conf_itr->second.rmep_confs.get(key)) == mep_conf_itr->second.rmep_confs.end()) {
            return VTSS_APPL_CFM_RC_OUT_OF_MEMORY;
        }

        rmep_conf_itr->second = *conf;
        CFM_mep_conf_update(key, CFM_MEP_STATE_CHANGE_CONF_RMEP);
    }

    CFM_conf_change_callbacks_invoke();
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_rmep_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_cfm_rmep_conf_del(const vtss_appl_cfm_rmep_key_t &key)
{
    cfm_mep_conf_itr_t  mep_conf_itr;
    cfm_rmep_conf_itr_t rmep_conf_itr;

    VTSS_RC(CFM_key_check(key));

    T_N("Enter, %s", key);

    {
        CFM_LOCK_SCOPE();

        VTSS_RC(CFM_mep_itr_get(key, mep_conf_itr));

        if ((rmep_conf_itr = mep_conf_itr->second.rmep_confs.find(key)) == mep_conf_itr->second.rmep_confs.end()) {
            return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
        }

        mep_conf_itr->second.rmep_confs.erase(rmep_conf_itr);
        CFM_mep_conf_update(key, CFM_MEP_STATE_CHANGE_CONF_RMEP);
    }

    CFM_conf_change_callbacks_invoke();
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_cfm_rmep_itr()
/******************************************************************************/
mesa_rc vtss_appl_cfm_rmep_itr(const vtss_appl_cfm_rmep_key_t *prev_key, vtss_appl_cfm_rmep_key_t *next_key, bool stay_in_this_mep)
{
    cfm_md_conf_itr_t        md_conf_itr;
    cfm_ma_conf_itr_t        ma_conf_itr;
    cfm_mep_conf_itr_t       mep_conf_itr;
    cfm_rmep_conf_itr_t      rmep_conf_itr;
    vtss_appl_cfm_rmep_key_t key;

    VTSS_RC(CFM_ptr_check(next_key));

    key.md     = prev_key ? prev_key->md     : "";
    key.ma     = prev_key ? prev_key->ma     : "";
    key.mepid  = prev_key ? prev_key->mepid  : 0;
    key.rmepid = prev_key ? prev_key->rmepid : 0;

    CFM_LOCK_SCOPE();

    for (md_conf_itr = CFM_md_conf_map.greater_than_or_equal(key); md_conf_itr != CFM_md_conf_map.end(); ++md_conf_itr) {
        if (md_conf_itr->first.md != key.md) {
            if (stay_in_this_mep) {
                // We have been asked not to enter another MD.
                return VTSS_APPL_CFM_RC_END_OF_LIST;
            }

            // Start over.
            key.ma = "";
        }

        for ((ma_conf_itr = md_conf_itr->second.ma_confs.greater_than_or_equal(key)); ma_conf_itr != md_conf_itr->second.ma_confs.end(); ++ma_conf_itr) {
            if (ma_conf_itr->first.ma != key.ma) {
                if (stay_in_this_mep) {
                    // We have been asked not to enter another MA.
                    return VTSS_APPL_CFM_RC_END_OF_LIST;
                }

                // Start over.
                key.mepid = 0;
            }

            for ((mep_conf_itr = ma_conf_itr->second.mep_confs.greater_than_or_equal(key)); mep_conf_itr != ma_conf_itr->second.mep_confs.end(); ++mep_conf_itr) {
                if (mep_conf_itr->first.mepid  != key.mepid) {
                    if (stay_in_this_mep) {
                        // We have been asked not to enter another MA.
                        return VTSS_APPL_CFM_RC_END_OF_LIST;
                    }

                    // Start over
                    key.rmepid = 0;
                }

                // Search for a Remote MEP-ID > key.rmepid
                if ((rmep_conf_itr = mep_conf_itr->second.rmep_confs.greater_than(key)) != mep_conf_itr->second.rmep_confs.end()) {
                    *next_key = rmep_conf_itr->first;
                    return VTSS_RC_OK;
                }
            }
        }
    }

    // No next
    return VTSS_APPL_CFM_RC_END_OF_LIST;
}

//******************************************************************************
// vtss_appl_cfm_mep_status_get()
//******************************************************************************
mesa_rc vtss_appl_cfm_mep_status_get(const vtss_appl_cfm_mep_key_t &key, vtss_appl_cfm_mep_status_t *status)
{
    cfm_mep_state_itr_t mep_state_itr;

    VTSS_RC(CFM_key_check(key));
    VTSS_RC(CFM_ptr_check(status));

    T_N("Enter, %s", key);

    CFM_LOCK_SCOPE();

    if ((mep_state_itr = CFM_mep_state_map.find(key)) == CFM_mep_state_map.end()) {
        return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
    }

    *status = mep_state_itr->second.status;

    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_cfm_mep_statistics_clear()
//******************************************************************************
mesa_rc vtss_appl_cfm_mep_statistics_clear(const vtss_appl_cfm_mep_key_t &key)
{
    cfm_mep_state_itr_t mep_state_itr;

    VTSS_RC(CFM_key_check(key));

    T_N("Enter, %s", key);

    CFM_LOCK_SCOPE();

    if ((mep_state_itr = CFM_mep_state_map.find(key)) == CFM_mep_state_map.end()) {
        return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
    }

    return cfm_base_mep_statistics_clear(&mep_state_itr->second);
}

//******************************************************************************
// vtss_appl_cfm_mep_notification_status_get()
//******************************************************************************
mesa_rc vtss_appl_cfm_mep_notification_status_get(const vtss_appl_cfm_mep_key_t &key, vtss_appl_cfm_mep_notification_status_t *const notif_status)
{
    VTSS_RC(CFM_key_check(key));
    VTSS_RC(CFM_ptr_check(notif_status));

    T_D("Enter, %s", key);

    // No need to lock scope, because the .get() function is guaranteed to be atomic.
    return cfm_mep_notification_status.get(key, notif_status);
}

//******************************************************************************
// vtss_appl_cfm_rmep_status_get()
//******************************************************************************
mesa_rc vtss_appl_cfm_rmep_status_get(const vtss_appl_cfm_rmep_key_t &key, vtss_appl_cfm_rmep_status_t *status)
{
    cfm_mep_state_itr_t  mep_state_itr;
    cfm_rmep_state_itr_t rmep_state_itr;

    VTSS_RC(CFM_key_check(key));
    VTSS_RC(CFM_ptr_check(status));

    T_D("Enter, %s", key);

    CFM_LOCK_SCOPE();

    if ((mep_state_itr = CFM_mep_state_map.find(key)) == CFM_mep_state_map.end()) {
        return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
    }

    if ((rmep_state_itr = mep_state_itr->second.rmep_states.find(key.rmepid)) == mep_state_itr->second.rmep_states.end()) {
        return VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE;
    }

    *status = rmep_state_itr->second.status;
    return VTSS_RC_OK;
}

/******************************************************************************/
// cfm_util_sender_id_tlv_option_to_str()
/******************************************************************************/
const char *cfm_util_sender_id_tlv_option_to_str(vtss_appl_cfm_sender_id_tlv_option_t sender_id_tlv_option, bool use_in_show_cmd)
{
    switch (sender_id_tlv_option) {
    case VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DISABLE:
        return use_in_show_cmd ? "Disabled" : "disable";

    case VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_CHASSIS:
        return use_in_show_cmd ? "Chassis" : "chassis";

    case VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_MANAGE:
        return use_in_show_cmd ? "Management" : "management";

    case VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_CHASSIS_MANAGE:
        return use_in_show_cmd ? "Chassis + Management" : "chassis-management";

    case VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DEFER:
        return use_in_show_cmd ? "Deferred" : "defer";

    default:
        T_E("Invalid Sender ID TLV option (%d)", sender_id_tlv_option);
        return use_in_show_cmd ? "Unknown" : "unknown";
    }
}

/******************************************************************************/
// cfm_util_tlv_option_to_str()
/******************************************************************************/
const char *cfm_util_tlv_option_to_str(vtss_appl_cfm_tlv_option_t tlv_option, bool use_in_show_cmd)
{
    switch (tlv_option) {
    case VTSS_APPL_CFM_TLV_OPTION_DISABLE:
        return use_in_show_cmd ? "Disabled" : "disable";

    case VTSS_APPL_CFM_TLV_OPTION_ENABLE:
        return use_in_show_cmd ? "Enabled" : "enable";

    case VTSS_APPL_CFM_TLV_OPTION_DEFER:
        return use_in_show_cmd ? "Deferred" : "defer";

    default:
        T_E("Invalid TLV option selection (%d)", tlv_option);
        return use_in_show_cmd ? "Unknown" : "unknown";
    }
}

/******************************************************************************/
// cfm_util_md_format_to_str()
/******************************************************************************/
const char *cfm_util_md_format_to_str(vtss_appl_cfm_md_format_t format, bool capital_first_letter)
{
    switch (format) {
    case VTSS_APPL_CFM_MD_FORMAT_NONE:
        return capital_first_letter ? "None" : "none";

    case VTSS_APPL_CFM_MD_FORMAT_STRING:
        return capital_first_letter ? "String" : "string";

    default:
        T_E("Invalid MD format (%d)", format);
        return "unknown";
    }
}

/******************************************************************************/
// cfm_util_ma_format_to_str()
/******************************************************************************/
const char *cfm_util_ma_format_to_str(vtss_appl_cfm_ma_format_t format, bool use_in_show_cmd)
{
    switch (format) {
    case VTSS_APPL_CFM_MA_FORMAT_STRING:
        return use_in_show_cmd ? "string" : "String";

    case VTSS_APPL_CFM_MA_FORMAT_TWO_OCTET_INTEGER:
        return use_in_show_cmd ? "integer" : "Integer";

    case VTSS_APPL_CFM_MA_FORMAT_PRIMARY_VID:
        return use_in_show_cmd ? "primary-vid" : "Primary-VID";

    case VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC:
        return use_in_show_cmd ? "icc" : "ICC";

    case VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC_CC:
        return use_in_show_cmd ? "icc-cc" : "ICC-CC";

    default:
        T_E("Invalid Maintenance Association format (%d)", format);
        return use_in_show_cmd ? "unknown" : "Unknown";
    }
}

/******************************************************************************/
// cfm_util_ccm_interval_to_str()
/******************************************************************************/
const char *cfm_util_ccm_interval_to_str(vtss_appl_cfm_ccm_interval_t ccm_interval)
{
    switch (ccm_interval) {
    case VTSS_APPL_CFM_CCM_INTERVAL_INVALID:
        return "none";

    case VTSS_APPL_CFM_CCM_INTERVAL_300HZ:
        return "3.3ms";

    case VTSS_APPL_CFM_CCM_INTERVAL_10MS:
        return "10ms";

    case VTSS_APPL_CFM_CCM_INTERVAL_100MS:
        return "100ms";

    case VTSS_APPL_CFM_CCM_INTERVAL_1S:
        return "1s";

    case VTSS_APPL_CFM_CCM_INTERVAL_10S:
        return "10s";

    case VTSS_APPL_CFM_CCM_INTERVAL_1MIN:
        return "1min";

    case VTSS_APPL_CFM_CCM_INTERVAL_10MIN:
        return "10min";

    default:
        T_E("Invalid CCM interval (%d)", ccm_interval);
        return "unknown";
    }
}

/******************************************************************************/
// cfm_util_direction_to_str()
/******************************************************************************/
const char *cfm_util_direction_to_str(vtss_appl_cfm_direction_t direction, bool capital_first_letter)
{
    switch (direction) {
    case VTSS_APPL_CFM_DIRECTION_DOWN:
        return capital_first_letter ? "Down" : "down";

    case VTSS_APPL_CFM_DIRECTION_UP:
        return capital_first_letter ? "Up" : "up";

    default:
        T_E("Invalid direction (%d)", direction);
        return capital_first_letter ? "Unknown" : "unknown";
    }
}

/******************************************************************************/
// cfm_util_fng_state_to_str()
/******************************************************************************/
const char *cfm_util_fng_state_to_str(vtss_appl_cfm_fng_state_t fng_state)
{
    switch (fng_state) {
    case VTSS_APPL_CFM_FNG_STATE_RESET:
        return "FNG_RESET";

    case VTSS_APPL_CFM_FNG_STATE_DEFECT:
        return "FNG_DEFECT";

    case VTSS_APPL_CFM_FNG_STATE_REPORT_DEFECT:
        return "FNG_REPORT_DEFECT";

    case VTSS_APPL_CFM_FNG_STATE_DEFECT_REPORTED:
        return "FNG_DEFECT_REPORTED";

    case VTSS_APPL_CFM_FNG_STATE_DEFECT_CLEARING:
        return "FNG_DEFECT_CLEARING";

    default:
        T_E("Invalid fng_state (%d)", fng_state);
        return "Unknown";
    }
}

/******************************************************************************/
// cfm_util_mep_defect_to_str()
/******************************************************************************/
const char *cfm_util_mep_defect_to_str(vtss_appl_cfm_mep_defect_t mep_defect)
{
    switch (mep_defect) {
    case VTSS_APPL_CFM_MEP_DEFECT_NONE:
        return "No defects since FNG_RESET (DefNone)";

    case VTSS_APPL_CFM_MEP_DEFECT_RDI_CCM:
        return "someRDIdefect";

    case VTSS_APPL_CFM_MEP_DEFECT_MAC_STATUS:
        return "someMACstatusDefect";

    case VTSS_APPL_CFM_MEP_DEFECT_REMOTE_CCM:
        return "someRMEPCCMdefect";

    case VTSS_APPL_CFM_MEP_DEFECT_ERROR_CCM:
        return "errorCCMdefect";

    case VTSS_APPL_CFM_MEP_DEFECT_XCON_CCM:
        return "xconCCMdefect";

    default:
        T_E("Invalid mep_defect (%d)", mep_defect);
        return "Unknown";
    }
}

/******************************************************************************/
// cfm_util_mep_defects_to_str()
/******************************************************************************/
const char *cfm_util_mep_defects_to_str(uint8_t defects, char buf[6])
{
    buf[0] = defects & VTSS_APPL_CFM_MEP_DEFECT_MASK_RDI_CCM     ? 'R' : '-';
    buf[1] = defects & VTSS_APPL_CFM_MEP_DEFECT_MASK_MAC_STATUS  ? 'M' : '-';
    buf[2] = defects & VTSS_APPL_CFM_MEP_DEFECT_MASK_REMOTE_CCM  ? 'C' : '-';
    buf[3] = defects & VTSS_APPL_CFM_MEP_DEFECT_MASK_ERROR_CCM   ? 'E' : '-';
    buf[4] = defects & VTSS_APPL_CFM_MEP_DEFECT_MASK_XCON_CCM    ? 'X' : '-';
    buf[5] = '\0';

    return buf;
}

/******************************************************************************/
// cfm_util_rmep_state_to_str()
/******************************************************************************/
const char *cfm_util_rmep_state_to_str(vtss_appl_cfm_rmep_state_t rmep_state)
{
    switch (rmep_state) {
    case VTSS_APPL_CFM_RMEP_STATE_IDLE:
        return "RMEP_IDLE";

    case VTSS_APPL_CFM_RMEP_STATE_START:
        return "RMEP_START";

    case VTSS_APPL_CFM_RMEP_STATE_FAILED:
        return "RMEP_FAILED";

    case VTSS_APPL_CFM_RMEP_STATE_OK:
        return "RMEP_OK";

    default:
        T_E("Invalid rmep_state (%d)", rmep_state);
        return "Unknown";
    }
}

/******************************************************************************/
// cfm_util_mep_creatable_error_to_str()
/******************************************************************************/
const char *cfm_util_mep_creatable_error_to_str(const vtss_appl_cfm_mep_errors_t *errors)
{
    if (errors->no_rmeps) {
        return "The MEP does not have any remote MEPs configured";
    } else if (errors->port_up_mep) {
        return "MEPs created in this service are port MEPs, which cannot be configured as Up-MEPs";
    } else if (errors->multiple_rmeps_ccm_interval) {
        return "The MEP has more than one Remote MEP, but the service's CCM interval is faster than 1 second";
    } else if (errors->is_mirror_port) {
        return "The MEP's residence interface is used as mirror destination port";
    } else if (errors->is_npi_port) {
        return "The MEP's residence interface is used as NPI port";
    } else if (errors->hw_resources) {
        return "Out of hardware resources";
    } else if (errors->internal_error) {
        return "Internal error that requires a code update. Check console and/or crashlog";
    } else if (errors->mep_creatable) {
        return "";
    } else {
        // MEP is NOT creatable, but we haven't caught any or the errors that
        // can cause this in the if()s above.
        T_E("Internal error: MEP is NOT creatable, but the reason is not known");
        return "Internal error that requires a code update. Check console and/or crashlog";
    }
}

/******************************************************************************/
// cfm_util_mep_enableRmepDefect_error_to_str()
/******************************************************************************/
const char *cfm_util_mep_enableRmepDefect_error_to_str(const vtss_appl_cfm_ma_conf_t *ma_conf, const vtss_appl_cfm_mep_conf_t *mep_conf, vtss_appl_cfm_mep_errors_t *errors)
{
    if (errors->no_link) {
        return "No link on residence interface";
    } else if (errors->vlan_unaware) {
        return "The MEP is a VLAN MEP or a tagged Port MEP configured on a VLAN unaware residence interface";
    } else if (errors->vlan_membership) {
        if (ma_conf->vlan == 0 && mep_conf->vlan == 0) {
            // Untagged
            return "Ingress filtering is enabled on the untagged port MEP's residence interface, which is not a member of its own port VLAN ID";
        } else {
            return "Ingress filtering is enabled on the MEP's residence interface, which is not a member of the MEP's VLAN ID";
        }
    } else if (errors->stp_blocked) {
        return "The MEP's residence interface is blocked by the Spanning Tree Protocol";
    } else if (errors->mstp_blocked) {
        return "The MEP's residence interface is blocked on the MEP's interface by the Multiple Spanning Tree Protocol";
    } else if (errors->enableRmepDefect) {
        return "";
    } else {
        // RMEP SMs not running, but we haven't caught the reason in any or the
        // if()s above.
        T_E("Internal error: RMEP SM(s) not running, but the reason is not known");
        return "Internal error that requires a code update. Check console and/or crashlog";
    }
}

/******************************************************************************/
// cfm_util_port_status_to_str()
/******************************************************************************/
const char *cfm_util_port_status_to_str(vtss_appl_cfm_port_status_t port_status)
{
    switch (port_status) {
    case VTSS_APPL_CFM_PORT_STATUS_NOT_RECEIVED:
        return "Not received";

    case VTSS_APPL_CFM_PORT_STATUS_BLOCKED:
        return "Blocked";

    case VTSS_APPL_CFM_PORT_STATUS_UP:
        return "Up";

    default:
        // Do not throw an error, because it might be that H/W simply updates it
        // without checking it for validity.
        T_I("Invalid port_status (%d)", port_status);
        return "Invalid";
    }
}

/******************************************************************************/
// cfm_util_interface_status_to_str()
/******************************************************************************/
const char *cfm_util_interface_status_to_str(vtss_appl_cfm_interface_status_t interface_status)
{
    switch (interface_status) {
    case VTSS_APPL_CFM_INTERFACE_STATUS_NOT_RECEIVED:
        return "Not received";

    case VTSS_APPL_CFM_INTERFACE_STATUS_UP:
        return "Up";

    case VTSS_APPL_CFM_INTERFACE_STATUS_DOWN:
        return "Down";

    case VTSS_APPL_CFM_INTERFACE_STATUS_TESTING:
        return "Testing";

    case VTSS_APPL_CFM_INTERFACE_STATUS_UNKNOWN:
        return "Unknown";

    case VTSS_APPL_CFM_INTERFACE_STATUS_DORMANT:
        return "Dormant";

    case VTSS_APPL_CFM_INTERFACE_STATUS_NOT_PRESENT:
        return "Not present";

    case VTSS_APPL_CFM_INTERFACE_STATUS_LOWER_LAYER_DOWN:
        return "Lower layer down";

    default:
        // Do not throw an error, because it might be that H/W simply updates it
        // without checking it for validity.
        T_I("Invalid interface_status (%d)", interface_status);
        return "Invalid";
    }
}

/******************************************************************************/
// cfm_util_sender_id_chassis_id_subtype_to_str()
/******************************************************************************/
const char *cfm_util_sender_id_chassis_id_subtype_to_str(vtss_appl_cfm_chassis_id_subtype_t chassis_id_subtype)
{
    switch (chassis_id_subtype) {
    case VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_NOT_RECEIVED:
        return "Not received";

    case VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_CHASSIS_COMPONENT:
        return "chassisComponent";

    case VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_INTERFACE_ALIAS:
        return "interfaceAlias";

    case VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_PORT_COMPONENT:
        return "portComponent";

    case VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_MAC_ADDRESS:
        return "macAddress";

    case VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_NETWORK_ADDRESS:
        return "networkAddress";

    case VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_INTERFACE_NAME:
        return "interfaceName";

    case VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_LOCAL:
        return "local";

    default:
        // Do not throw an error, because we don't do validity checks on what we
        // receive of Chassis ID
        T_I("Unknown chassis_id_subtype (%d)", chassis_id_subtype);
        return "Unknown";
    }
}

/******************************************************************************/
// cfm_util_key_check()
/******************************************************************************/
mesa_rc cfm_util_key_check(const vtss_appl_cfm_mep_key_t &key, bool *empty)
{
    if (empty) {
        // User wants to know whether the key is empty or not. This cannot fail.
        *empty = key.md == "" || key.ma == "" || CFM_mepid_check(key.mepid) != VTSS_RC_OK;
        return VTSS_RC_OK;
    }

    return CFM_key_check(key);
}

/******************************************************************************/
// cfm_util_conf_change_callback_register()
/******************************************************************************/
mesa_rc cfm_util_conf_change_callback_register(vtss_module_id_t module_id, void (*callback)(void))
{
    int i;

    if (callback == nullptr) {
        return VTSS_APPL_CFM_RC_INVALID_ARGUMENT;
    }

    CFM_LOCK_SCOPE();

    for (i = 0; i < ARRSZ(CFM_conf_change_callbacks); i++) {
        if (CFM_conf_change_callbacks[i].callback) {
            continue;
        }

        CFM_conf_change_callbacks[i].callback  = callback;
        CFM_conf_change_callbacks[i].module_id = module_id;
        return VTSS_RC_OK;
    }

    T_E("No more free entries in CFM_conf_change_callbacks[]. Module = %s", vtss_module_names[module_id]);
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// cfm_error_txt()
/******************************************************************************/
const char *cfm_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_CFM_RC_INVALID_ARGUMENT:
        return "Invalid argument (typically a NULL pointer) pass to function";

    case VTSS_APPL_CFM_RC_INTERNAL_ERROR:
        return "Internal Error: A code-update is required. See console or crashlog for details";

    case VTSS_APPL_CFM_RC_OUT_OF_MEMORY:
        return "Out of memory";

    case VTSS_APPL_CFM_RC_END_OF_LIST:
        return "End-of-list (not a real error!)";

    case VTSS_APPL_CFM_RC_INVALID_NAME_KEY_LENGTH:
        return "Invalid key-name length. Must be a string of length 1 to " vtss_xstr(VTSS_APPL_CFM_KEY_LEN_MAX) " (excl. terminating NULL)";

    case VTSS_APPL_CFM_RC_INVALID_NAME_KEY_CONTENTS:
        return "Invalid key-name contents. First character must be from [a-zA-Z]. Remaining characters must be from a printable character excluding space and colon [33-126], except 58";

    case VTSS_APPL_CFM_RC_INVALID_NAME_KEY_CONTENTS_COLON:
        return "The key-name must not contain a colon. First character must be from [a-zA-Z]. Remaining characters must be from a printable character excluding space and colon [33-126], except 58";

    case VTSS_APPL_CFM_RC_INVALID_NAME_KEY_CONTENTS_ALL:
        return "The key-name 'all' is reserved";

    case VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE:
        return "Requested instance doesn't exist";

    case VTSS_APPL_CFM_RC_INVALID_SENDER_ID_TLV_OPTION:
        return "Invalid Sender ID TLV option";

    case VTSS_APPL_CFM_RC_INVALID_PORT_STATUS_TLV_OPTION:
        return "Invalid Port Status TLV option";

    case VTSS_APPL_CFM_RC_INVALID_INTERFACE_STATUS_TLV_OPTION:
        return "Invalid Interface Status TLV option";

    case VTSS_APPL_CFM_RC_ORG_SPEC_TLV_VAL_TOO_LONG:
        return "Value of organization-specific TLV is too long (must be from 0 to 64 characters long)";

    case VTSS_APPL_CFM_RC_MD_UNSUPPORTED_FORMAT:
        return "Unsupported domain format";

    case VTSS_APPL_CFM_RC_MD_INVALID_NAME_LENGTH:
        return "Invalid domain name length. It must be between 1 and 43 characters long";

    case VTSS_APPL_CFM_RC_MD_INVALID_NAME_CONTENTS:
        return "Invalid domain name. It must contain characters in ASCII range [32; 126]";

    case VTSS_APPL_CFM_RC_MD_INVALID_LEVEL:
        return "Invalid MD/MEG level. It must be a value in range [0; 7]";

    case VTSS_APPL_CFM_RC_MD_Y1731_FORMAT:
        return "One or more of the attached services (MAs) require Y.1731 MEG ID formats, so domain's format must be 'none'";

    case VTSS_APPL_CFM_RC_MD_MAID_TOO_LONG:
        return "One or more of the attached services (MAs) cause the resulting MAID to become too long (> 48 bytes)";

    case VTSS_APPL_CFM_RC_MD_LIMIT_REACHED:
        return "Cannot create another domain, because the limit is reached";

    case VTSS_APPL_CFM_RC_MD_ALL_PORT_MEPS_MUST_HAVE_SAME_LEVEL:
        return "Cannot change level for this domain, because that will result in Port MEPs with a different MD/MEG level than other Port MEPs";

    case VTSS_APPL_CFM_RC_MD_PORT_MEP_MUST_HAVE_LOWER_LEVEL_THAN_ANY_VLAN_MEP:
        return "Cannot change level for this domain, because that will result in Port MEPs with the same or higher MD/MEG level than the currently lowest level VLAN MEP";

    case VTSS_APPL_CFM_RC_MD_VLAN_MEP_MUST_HAVE_HIGHER_LEVEL_THAN_PORT_MEPS:
        return "Cannot change level for this domain, because that will result in VLAN MEPs with the same or lower MD/MEG level than the current Port MEP level";

    case VTSS_APPL_CFM_RC_MD_LEVEL_OF_PORT_MEP_MUST_BE_LOWER_THAN_LEVEL_OF_VLAN_MEP:
        return "Cannot change level for this domain, because a Port MEP within this domain then will have an MD/MEG level at of higher than a VLAN MEP on the same port and VLAN";

    case VTSS_APPL_CFM_RC_MD_LEVEL_OF_VLAN_MEP_MUST_BE_HIGHER_THAN_LEVEL_OF_PORT_MEP:
        return "Cannot change level for this domain, because a VLAN MEP within this domain then will have an MD/MEG level at or lower than a Port MEP on the same port and VLAN";

    case VTSS_APPL_CFM_RC_MA_UNSUPPORTED_FORMAT:
        return "Unsupported service (maintenance association) format";

    case VTSS_APPL_CFM_RC_MA_INVALID_NAME_STRING_LENGTH:
        return "Invalid MA name length. It must be between 1 and 45 characters long";

    case VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_LENGTH:
        return "Invalid MA name length. It must be exactly 13 characters long";

    case VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_CC_LENGTH:
        return "Invalid MA name length. It must be exactly 15 characters long";

    case VTSS_APPL_CFM_RC_MA_INVALID_NAME_STRING_CONTENTS:
        return "Invalid MA name. It must contain characters in ASCII range [32; 126]";

    case VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_CONTENTS:
        return "Invalid MA name. It must contain characters from [a-zA-Z0-9]";

    case VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_CC_CONTENTS_FIRST:
        return "Invalid MA name. The first two characters must be from [A-Z]";

    case VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_CC_CONTENTS_SLASH:
        return "Invalid MA name. Only one '/' may be present at position 4-8";

    case VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_CC_CONTENTS_LAST:
        return "Invalid MA name. The last thirteen characters must be from [a-zA-Z0-9] with a possible '/' at position 4-8";

    case VTSS_APPL_CFM_RC_MA_INVALID_VLAN:
        return "Invalid VLAN ID. It must be a value in range [0; 4095], 0 meaning creation of port/interface MEPs";

    case VTSS_APPL_CFM_RC_MA_INVALID_CCM_INTERVAL:
        return "Invalid CCM interval";

    case VTSS_APPL_CFM_RC_MA_CCM_INTERVAL_NOT_SUPPORTED:
        return "CCM interval not supported on this platform";

    case VTSS_APPL_CFM_RC_MA_Y1731_FORMAT:
        return "Y.1731 formats (icc and icc-cc) are only allowed if the domain's format is none";

    case VTSS_APPL_CFM_RC_MA_MAID_TOO_LONG:
        return "When combined with the domain, the resulting MAID will become too long (> 48 bytes)";

    case VTSS_APPL_CFM_RC_MA_PORT_LIMIT_REACHED:
        return "Cannot change type for this service, because that will cause the supported number of port MEPs to be exceeded";

    case VTSS_APPL_CFM_RC_MA_SERVICE_LIMIT_REACHED:
        return "Cannot change type for this service, because that will cause the supported number of VLAN MEPs to be exceeded";

    case VTSS_APPL_CFM_RC_MA_LIMIT_REACHED:
        return "Cannot create another service within this domain, because the limit is reached";

    case VTSS_APPL_CFM_RC_MA_ONLY_ONE_PORT_MEP_PER_PORT:
        return "Cannot change type for this service, because that will result in creating more than one Port MEP per port";

    case VTSS_APPL_CFM_RC_MA_ONLY_ONE_VLAN_MEP_PER_PORT_PER_VLAN:
        return "Cannot change type for this service, because that will result in creating more than one VLAN MEP per port in the same VLAN";

    case VTSS_APPL_CFM_RC_MA_LEVEL_OF_PORT_MEP_MUST_BE_LOWER_THAN_LEVEL_OF_VLAN_MEP:
        return "Cannot change service type to Port, because a Port MEP on this service will then will have an MD/MEG level at or higher than a VLAN MEP on the same port and VLAN";

    case VTSS_APPL_CFM_RC_MA_LEVEL_OF_VLAN_MEP_MUST_BE_HIGHER_THAN_LEVEL_OF_PORT_MEP:
        return "Cannot change service type to VLAN, because a VLAN MEP on this service will then will have an MD/MEG level at or lower than a Port MEP on the same port and VLAN";

    case VTSS_APPL_CFM_RC_MA_ALL_PORT_MEPS_MUST_HAVE_SAME_LEVEL:
        return "Cannot change type for this service, because that will result in Port MEPs with a different MD/MEG level than other Port MEPs";

    case VTSS_APPL_CFM_RC_MA_PORT_MEP_MUST_HAVE_LOWER_LEVEL_THAN_ANY_VLAN_MEP:
        return "Cannot change type for this service, because that will result in Port MEPs with the same or higher MD/MEG level than the currently lowest level VLAN MEP";

    case VTSS_APPL_CFM_RC_MA_VLAN_MEP_MUST_HAVE_HIGHER_LEVEL_THAN_PORT_MEPS:
        return "Cannot change type for this service, because that will result in VLAN MEPs with the same or lower MD/MEG level than the current Port MEP level";

    case VTSS_APPL_CFM_RC_MA_VLAN_MEPS_NOT_SUPPORTED:
        return "This platform does not support VLAN MEPs, only Port MEPs";

    case VTSS_APPL_CFM_RC_MEP_INVALID_MEPID:
        return "Invalid MEP ID. It must be in range [1; 8191]";

    case VTSS_APPL_CFM_RC_MEP_INVALID_IFINDEX:
        return "Invalid interface index. It must represent a port";

    case VTSS_APPL_CFM_RC_MEP_INVALID_DIRECTION:
        return "Invalid direction";

    case VTSS_APPL_CFM_RC_MEP_DIRECTION_NOT_SUPPORTED_ON_THIS_PLATFORM:
        return "Direction is not supported on this platform";

    case VTSS_APPL_CFM_RC_MEP_INVALID_VLAN:
        return "Invalid VLAN ID. It must be a value in range [0; 4095], 0 meaning use the service's primary VLAN, and if that is zero, use untagged";

    case VTSS_APPL_CFM_RC_MEP_INVALID_PCP:
        return "Invalid PCP value. It must be a value in range [0; 7].";

    case VTSS_APPL_CFM_RC_MEP_INVALID_SMAC:
        return "Source MAC address must be a unicast address";

    case VTSS_APPL_CFM_RC_MEP_INVALID_ALARM_LEVEL:
        return "Invalid Alarm Level. It must be a value in range [1; 6]";

    case VTSS_APPL_CFM_RC_MEP_INVALID_ALARM_TIME_PRESENT:
        return "Invalid alarm present time. It must be a value in range [2500; 10000] ms";

    case VTSS_APPL_CFM_RC_MEP_INVALID_ALARM_TIME_ABSENT:
        return "Invalid alarm absent time. It must be a value in range [2500; 10000] ms";

    case VTSS_APPL_CFM_RC_MEP_ALL_MEPS_IN_MA_MUST_HAVE_SAME_DIRECTION:
        return "MEPs in the same service (maintenance association) must all either be Up- or Down-MEPs";

    case VTSS_APPL_CFM_RC_MEP_ONLY_ONE_PORT_MEP_PER_PORT:
        return "There can only be one Port MEP per interface, and another Port MEP already exists on this one";

    case VTSS_APPL_CFM_RC_MEP_ONLY_ONE_VLAN_MEP_PER_PORT_PER_VLAN:
        return "Another VLAN Down-MEP on the same VLAN and interface already exists";

    case VTSS_APPL_CFM_RC_MEP_LEVEL_OF_PORT_MEP_MUST_BE_LOWER_THAN_LEVEL_OF_VLAN_MEP:
        return "A VLAN Down-MEP on the same interface and VLAN with an MD/MEG level at or lower than this Port MEP's MD/MEG level already exists";

    case VTSS_APPL_CFM_RC_MEP_LEVEL_OF_VLAN_MEP_MUST_BE_HIGHER_THAN_LEVEL_OF_PORT_MEP:
        return "A Port Down-MEP on the same interface and VLAN with an MD/MEG level at or higher than this VLAN MEP's MD/MEG level already exists";

    case VTSS_APPL_CFM_RC_MEP_ALL_PORT_MEPS_MUST_HAVE_SAME_LEVEL:
        return "All port MEPs must have same MD/MEG level";

    case VTSS_APPL_CFM_RC_MEP_PORT_MEP_MUST_HAVE_LOWER_LEVEL_THAN_ANY_VLAN_MEP:
        return "A port MEP must have lower MD/MEG level than any VLAN MEP";

    case VTSS_APPL_CFM_RC_MEP_VLAN_MEP_MUST_HAVE_HIGHER_LEVEL_THAN_PORT_MEPS:
        return "A VLAN MEP must have higher MD/MEG level than port MEPs";

    case VTSS_APPL_CFM_RC_MEP_PORT_LIMIT_REACHED:
        return "Cannot enable this MEP, since that will cause the supported number of port MEPs to be exceeded";

    case VTSS_APPL_CFM_RC_MEP_SERVICE_LIMIT_REACHED:
        return "Cannot enable this MEP, since that will cause the supported number of VLAN MEPs to be exceeded";

    case VTSS_APPL_CFM_RC_RMEP_RMEPID_SAME_AS_MEPID:
        return "A Remote MEP-ID cannot be identical to this MEP's MEP-ID";

    case VTSS_APPL_CFM_RC_RMEP_LIMIT_REACHED:
        return "Unable to add another Remote MEP, because the supported number is reached";

    case VTSS_APPL_CFM_RC_HW_RESOURCES:
        return "Out of H/W resources";

    default:
        return "CFM: Unknown error code";
    }
}

extern "C" int cfm_icli_cmd_register(void);

/******************************************************************************/
// Pre-declaration of JSON registration function.
/******************************************************************************/
#if defined(VTSS_SW_OPTION_JSON_RPC)
VTSS_PRE_DECLS void vtss_appl_cfm_json_init(void);
#endif /* defined(VTSS_SW_OPTION_JSON_RPC) */

/******************************************************************************/
// cfm_init()
/******************************************************************************/
mesa_rc cfm_init(vtss_init_data_t *data)
{
    cfm_mep_state_itr_t mep_state_itr;
    mesa_rc             rc;

    (void)cfm_timer_init(data);

    switch (data->cmd) {
    case INIT_CMD_INIT:
        // Do not use fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT), because that one
        // will return a number >= actual port count, and then some functions
        // may fail (e.g. vlan_mgmt_port_conf_get()).
        CFM_cap_port_cnt = port_count_max();

        CFM_capabilities_set();
        (void)vtss_appl_cfm_global_conf_default_get(&CFM_global_conf);
        critd_init(&CFM_crit, "cfm", VTSS_MODULE_ID_CFM, CRITD_TYPE_MUTEX);
        cfm_icli_cmd_register();

#if defined(VTSS_SW_OPTION_JSON_RPC)
        vtss_appl_cfm_json_init();
#endif /* defined(VTSS_SW_OPTION_JSON_RPC) */
        break;

    case INIT_CMD_START:
#ifdef VTSS_SW_OPTION_ICFG
        mesa_rc cfm_icfg_init(void);
        VTSS_RC(cfm_icfg_init()); // ICFG initialization (show running-config)
#endif
        break;

    case INIT_CMD_CONF_DEF:
        if (data->isid == VTSS_ISID_GLOBAL) {
            CFM_default();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        // Initialize the global state
        CFM_global_state_init();

        {
            CFM_LOCK_SCOPE();

            // Initialize the base library
            cfm_base_init(&CFM_global_state);
        }

        // Initialize the port-state array
        CFM_port_state_init();

        if (CFM_global_state.has_vop) {
            // Hook up for VOE events
            if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_CFM, CFM_voe_interrupt_callback, MEBA_EVENT_VOE, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
                T_E("vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
            }
        }

        CFM_default();

        // Register for CFM PDUs
        CFM_frame_rx_register();
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        // Now that ICLI has applied all configuration, start creating all the
        // MEPs. Only do this the very first time an INIT_CMD_ICFG_LOADING_POST
        // is issued.
        if (!CFM_started) {
            {
                CFM_LOCK_SCOPE();

                CFM_started = true;
                for (mep_state_itr = CFM_mep_state_map.begin(); mep_state_itr != CFM_mep_state_map.end(); ++mep_state_itr) {
                    CFM_mep_update(mep_state_itr, CFM_MEP_STATE_CHANGE_CONF);
                }
            }

            CFM_conf_change_callbacks_invoke();
        }

        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

