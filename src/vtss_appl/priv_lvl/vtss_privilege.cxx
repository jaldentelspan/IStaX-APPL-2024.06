/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "misc_api.h"
#include "vtss_privilege_api.h"
#include "vtss_privilege.h"
#include "sysutil_api.h"
#include "vtss_users_api.h"

#ifdef VTSS_SW_OPTION_ICFG
#include "vtss_privilege_icfg.h"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_PRIV_LVL

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Global structure */
static privilege_global_t VTSS_PRIVILEGE_global;

static vtss_trace_reg_t VTSS_PRIVILEGE_trace_reg = {
    VTSS_TRACE_MODULE_ID, "priv_lvl", "Privilege level"
};

static vtss_trace_grp_t VTSS_PRIVILEGE_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&VTSS_PRIVILEGE_trace_reg, VTSS_PRIVILEGE_trace_grps);

#define PRIVILEGE_CRIT_ENTER() critd_enter(&VTSS_PRIVILEGE_global.crit, __FILE__, __LINE__)
#define PRIVILEGE_CRIT_EXIT()  critd_exit( &VTSS_PRIVILEGE_global.crit, __FILE__, __LINE__)

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/

/* Set privilege defaults */
void VTSS_PRIVILEGE_default_get(vtss_priv_conf_t *conf)
{
    int i;

    for (i = 0; i < VTSS_MODULE_ID_NONE; i++) {
        switch (i) {
        case VTSS_MODULE_ID_SYSTEM:
        case VTSS_MODULE_ID_PORT:
        case VTSS_MODULE_ID_TOPO:
            conf->privilege_level[i].configRoPriv = 5;
            conf->privilege_level[i].configRwPriv = 10;
            conf->privilege_level[i].statusRoPriv = 1;
            conf->privilege_level[i].statusRwPriv = 10;
            break;
        case VTSS_MODULE_ID_MISC:
        case VTSS_MODULE_ID_DEBUG:
            conf->privilege_level[i].configRoPriv = 15;
            conf->privilege_level[i].configRwPriv = 15;
            conf->privilege_level[i].statusRoPriv = 15;
            conf->privilege_level[i].statusRwPriv = 15;
            break;
        case VTSS_MODULE_ID_SECURITY:
            conf->privilege_level[i].configRoPriv = 10;
            conf->privilege_level[i].configRwPriv = 10;
            conf->privilege_level[i].statusRoPriv = 5;
            conf->privilege_level[i].statusRwPriv = 10;
            break;
        default:
            conf->privilege_level[i].configRoPriv = 5;
            conf->privilege_level[i].configRwPriv = 10;
            conf->privilege_level[i].statusRoPriv = 5;
            conf->privilege_level[i].statusRwPriv = 10;
            break;
        }
    }
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/**
  * \brief Retrieve an error string based on a return code
  *        from one of the Privilege Level API functions.
  *
  * \param rc [IN]: Error code that must be in the VTSS_PRIV_ERROR_xxx range.
  */
const char *vtss_privilege_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_PRIV_ERROR_MUST_BE_PRIMARY_SWITCH:
        return "Operation only valid on primary switch switch";

    case VTSS_PRIV_ERROR_ISID:
        return "Invalid Switch ID";

    case VTSS_PRIV_ERROR_INV_PARAM:
        return "Invalid parameter supplied to function";

    default:
        return "Privilege Level: Unknown error code";
    }
}

/**
  * \brief Get the global Privilege Level configuration.
  *
  * \param glbl_cfg [OUT]: Pointer to structure that receives
  *                        the current configuration.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VTSS_PRIV_ERROR_INV_PARAM if glbl_cfg is NULL.\n
  *    VTSS_PRIV_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  */
mesa_rc vtss_priv_mgmt_conf_get(vtss_priv_conf_t *glbl_cfg)
{
    T_D("enter");
    if (glbl_cfg == NULL) {
        T_W("not primary switch");
        T_D("exit");
        return VTSS_PRIV_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return VTSS_PRIV_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    PRIVILEGE_CRIT_ENTER();
    *glbl_cfg = VTSS_PRIVILEGE_global.privilege_conf;
    PRIVILEGE_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/**
  * \brief Set the global Privilege Level configuration.
  *
  * \param glbl_cfg [IN]: Pointer to structure that contains the
  *                       global configuration to apply to the
  *                       voice VLAN module.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VTSS_PRIV_ERROR_INV_PARAM if glbl_cfg is NULL or parameters error.\n
  *    VTSS_PRIV_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    Others value is caused form other modules.\n
  */
mesa_rc vtss_priv_mgmt_conf_set(vtss_priv_conf_t *glbl_cfg)
{
    mesa_rc                 rc = VTSS_RC_OK;
    int                     i;
    users_conf_t            users_conf;

    T_D("enter");

    if (glbl_cfg == NULL) {
        T_W("not primary switch");
        T_D("exit");
        return VTSS_PRIV_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return VTSS_PRIV_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    /* Check illegal parameter */
    for (i = 0; i < VTSS_MODULE_ID_NONE; i++) {
        /* The privilege level of 'Read-only' should be less or equal 'Read-write' */
        if (glbl_cfg->privilege_level[i].configRoPriv > VTSS_APPL_PRIVILEGE_LVL_MAX ||
            glbl_cfg->privilege_level[i].configRwPriv > VTSS_APPL_PRIVILEGE_LVL_MAX ||
            glbl_cfg->privilege_level[i].statusRoPriv > VTSS_APPL_PRIVILEGE_LVL_MAX ||
            glbl_cfg->privilege_level[i].statusRwPriv > VTSS_APPL_PRIVILEGE_LVL_MAX) {
            T_D("exit");
            return VTSS_PRIV_ERROR_INV_PARAM;
        }

        /* The privilege level of 'Read-only' should be less or equal 'Read-write' */
        if (glbl_cfg->privilege_level[i].configRoPriv > glbl_cfg->privilege_level[i].configRwPriv || glbl_cfg->privilege_level[i].statusRoPriv > glbl_cfg->privilege_level[i].statusRwPriv) {
            T_D("exit");
            return VTSS_PRIV_ERROR_INV_PARAM;
        }

        /* The privilege level of 'Configuration/Execute Read-write' should be great or equal 'Status/Statistics Read-only' */
        if (glbl_cfg->privilege_level[i].configRwPriv < glbl_cfg->privilege_level[i].statusRoPriv) {
            T_D("exit");
            return VTSS_PRIV_ERROR_INV_PARAM;
        }
    }

    /* Change to lower privilege level will lock yourself out */
    strcpy(users_conf.username, VTSS_SYS_ADMIN_NAME);
    if (vtss_users_mgmt_conf_get(&users_conf, FALSE) != VTSS_RC_OK) {
        T_D("exit");
        return VTSS_PRIV_ERROR_INV_PARAM;
    }
    if ((int)(glbl_cfg->privilege_level[VTSS_MODULE_ID_MISC].configRwPriv) > users_conf.privilege_level) {
        T_D("exit");
        return VTSS_PRIV_ERROR_INV_PARAM;
    }

    PRIVILEGE_CRIT_ENTER();
    VTSS_PRIVILEGE_global.privilege_conf = *glbl_cfg;
    PRIVILEGE_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/**
  * \brief Verify privilege level is allowed for 'Configuration Read-only'
  *
  * Used by CLI and Web module in order to get the configured privilege
  * levels for a specific privilege level group.
  *
  * \param id               [IN]: The module ID.
  * \param current_level    [IN]: The current privilege level.
  *
  * \return
  *    TRUE if the module name is found.\n
  *    FALSE otherwise.\n
  */
BOOL vtss_priv_is_allowed_cro(vtss_module_id_t id, int current_level)
{
    T_D("enter");
    PRIVILEGE_CRIT_ENTER();
    if (current_level >= (int)(VTSS_PRIVILEGE_global.privilege_conf.privilege_level[id].configRoPriv)) {
        PRIVILEGE_CRIT_EXIT();
        T_D("exit");
        return TRUE;
    } else {
        PRIVILEGE_CRIT_EXIT();
        T_D("exit");
        return FALSE;
    }
}

/**
  * \brief Verify privilege level is allowed for 'Configuration Read-write'
  *
  * Used by CLI and Web module in order to get the configured privilege
  * levels for a specific privilege level group.
  *
  * \param id               [IN]: The module ID.
  * \param current_level    [IN]: The current privilege level.
  *
  * \return
  *    TRUE if the module name is found.\n
  *    FALSE otherwise.\n
  */
BOOL vtss_priv_is_allowed_crw(vtss_module_id_t id, int current_level)
{
    T_D("enter: module_id=%d, current_level = %d", id, current_level);
    PRIVILEGE_CRIT_ENTER();
    if (current_level >= (int)(VTSS_PRIVILEGE_global.privilege_conf.privilege_level[id].configRwPriv)) {
        T_D("exit: Allowed, current level is greater than Config Read-Write privilege level %d", VTSS_PRIVILEGE_global.privilege_conf.privilege_level[id].configRwPriv);
        PRIVILEGE_CRIT_EXIT();
        return TRUE;
    } else {
        T_D("exit: Not allowed, current level is smaller than Config Read-Write privilege level %d", VTSS_PRIVILEGE_global.privilege_conf.privilege_level[id].configRwPriv);
        PRIVILEGE_CRIT_EXIT();
        return FALSE;
    }
}

/**
  * \brief Verify privilege level is allowed for ''Status/Statistics Read-only'
  *
  * Used by CLI and Web module in order to get the configured privilege
  * levels for a specific privilege level group.
  *
  * \param id               [IN]: The module ID.
  * \param current_level    [IN]: The current privilege level.
  *
  * \return
  *    TRUE if the module name is found.\n
  *    FALSE otherwise.\n
  */
BOOL vtss_priv_is_allowed_sro(vtss_module_id_t id, int current_level)
{
    T_D("exit");
    PRIVILEGE_CRIT_ENTER();
    if (current_level >= (int)(VTSS_PRIVILEGE_global.privilege_conf.privilege_level[id].statusRoPriv)) {
        PRIVILEGE_CRIT_EXIT();
        T_D("exit");
        return TRUE;
    } else {
        PRIVILEGE_CRIT_EXIT();
        T_D("exit");
        return FALSE;
    }
}

/**
  * \brief Verify privilege level is allowed for ''Status/Statistics Read-write'
  *
  * Used by CLI and Web module in order to get the configured privilege
  * levels for a specific privilege level group.
  *
  * \param id               [IN]: The module ID.
  * \param current_level    [IN]: The current privilege level.
  *
  * \return
  *    TRUE if the module name is found.\n
  *    FALSE otherwise.\n
  */
BOOL vtss_priv_is_allowed_srw(vtss_module_id_t id, int current_level)
{
    T_D("enter");
    PRIVILEGE_CRIT_ENTER();
    if (current_level >= (int)(VTSS_PRIVILEGE_global.privilege_conf.privilege_level[id].statusRwPriv)) {
        PRIVILEGE_CRIT_EXIT();
        T_D("exit");
        return TRUE;
    } else {
        PRIVILEGE_CRIT_EXIT();
        T_D("exit");
        return FALSE;
    }
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

/* Read/create privilege stack configuration */
static void VTSS_PRIVILEGE_conf_read_stack(BOOL create)
{
    T_D("enter, create: %d", create);

    PRIVILEGE_CRIT_ENTER();
    /* Use default values */
    VTSS_PRIVILEGE_default_get(&VTSS_PRIVILEGE_global.privilege_conf);
    PRIVILEGE_CRIT_EXIT();

    T_D("exit");
}

/* Module start */
static void VTSS_PRIVILEGE_start(void)
{
    vtss_priv_conf_t    *conf_p;

    T_D("enter");

    /* Initialize privilege configuration */
    conf_p = &VTSS_PRIVILEGE_global.privilege_conf;
    VTSS_PRIVILEGE_default_get(conf_p);

    /* Create semaphore for critical regions */
    critd_init(&VTSS_PRIVILEGE_global.crit, "Privilege", VTSS_MODULE_ID_PRIV_LVL, CRITD_TYPE_MUTEX);

    T_D("exit");
}

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize private mib */
VTSS_PRE_DECLS void privilege_mib_init(void);
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_privilege_json_init(void);
#endif

extern "C" int vtss_privilege_icli_cmd_register();

/**
  * \brief Initialize the Privilege Level module
  *
  * \param cmd [IN]: Reason why calling this function.
  * \param p1  [IN]: Parameter 1. Usage varies with cmd.
  * \param p2  [IN]: Parameter 2. Usage varies with cmd.
  *
  * \return
  *    VTSS_RC_OK.
  */
mesa_rc vtss_priv_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
#ifdef VTSS_SW_OPTION_ICFG
    mesa_rc     rc = VTSS_RC_OK;
#endif

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");

        VTSS_PRIVILEGE_start();
#ifdef VTSS_SW_OPTION_ICFG
        if ((rc = priv_icfg_init()) != VTSS_RC_OK) {
            T_D("Calling priv_icfg_init() failed rc = %s", error_txt(rc));
        }
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register private mib */
        privilege_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_privilege_json_init();
#endif
        vtss_privilege_icli_cmd_register();
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            VTSS_PRIVILEGE_conf_read_stack(1);
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");

        /* Read stack and switch configuration */
        VTSS_PRIVILEGE_conf_read_stack(0);
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

static void VTSS_PRIVILEGE_strn_tolower(char *str, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        str[i] = tolower(str[i]);
    }
} /* str_tolower */

/**
  * \brief Get module ID by privilege level group name
  *
  * \param name         [IN]: The privilege level group name.
  * \param module_id_p  [OUT]: The module ID.
  *
  * \return
  *    TRUE if the module name is found.\n
  *    FALSE otherwise.\n
  */
BOOL vtss_privilege_module_to_val(const char *name, vtss_module_id_t *module_id_p)
{
    int i;
    char name_lc[VTSS_PRIV_LVL_NAME_LEN_MAX], module_name_lc[VTSS_PRIV_LVL_NAME_LEN_MAX];
    int  mid_match = -1;

    T_D("enter");

    /* Convert name to lowercase */
    strncpy(name_lc, name, VTSS_PRIV_LVL_NAME_LEN_MAX - 1);
    name_lc[VTSS_PRIV_LVL_NAME_LEN_MAX - 1] = 0;
    VTSS_PRIVILEGE_strn_tolower(name_lc, strlen(name_lc));

    for (i = 0; i < VTSS_MODULE_ID_NONE; i++) {
        if (vtss_priv_lvl_groups_filter[i]) {
            continue;
        }
        strncpy(module_name_lc, vtss_module_names[i], VTSS_PRIV_LVL_NAME_LEN_MAX - 1);
        module_name_lc[VTSS_PRIV_LVL_NAME_LEN_MAX - 1] = 0;
        VTSS_PRIVILEGE_strn_tolower(module_name_lc, strlen(module_name_lc));
        if (strncmp(name_lc, module_name_lc, strlen(name_lc)) == 0) {
            if (strlen(module_name_lc) == strlen(name_lc)) {
                /* Exact match found */
                mid_match = i;
                break;
            }
            if (mid_match == -1) {
                /* First match found */
                mid_match = i;
            } else {
                /* >1 match found */
                T_D("exit");
                return FALSE;
            }
        }
    }

    if (mid_match != -1) {
        *module_id_p = (vtss_module_id_t) mid_match;
        T_D("exit");
        return TRUE;
    }

    T_D("exit");
    return FALSE;
}

/**
  * \brief Get privilege group name
  *
  * \param name         [OUT]: The privilege level group name.
  * \param module_id_p  [OUT]: The module ID.
  * \param next         [IN]: is getnext operation.
  *
  * \return
  *    TRUE if the module name is found.\n
  *    FALSE otherwise.\n
  */
BOOL vtss_privilege_group_name_get(char *name, vtss_module_id_t *module_id_p, BOOL next)
{
    int i, found = 0;
    char upper_name[VTSS_PRIV_LVL_NAME_LEN_MAX] = "";
    char name_lc[VTSS_PRIV_LVL_NAME_LEN_MAX], module_name_lc[VTSS_PRIV_LVL_NAME_LEN_MAX];

    /* Convert name to lowercase */
    strncpy(name_lc, name, VTSS_PRIV_LVL_NAME_LEN_MAX - 1);
    name_lc[VTSS_PRIV_LVL_NAME_LEN_MAX - 1] = 0;
    VTSS_PRIVILEGE_strn_tolower(name_lc, strlen(name_lc));

    T_D("enter");
    for (i = 0; i < VTSS_MODULE_ID_NONE; i++) {
        if (vtss_priv_lvl_groups_filter[i]) {
            continue;
        }
        if (vtss_module_names[i] == NULL) {
            T_W("Cannot find the module name. module ID = %d", i);
            continue;
        }
        strncpy(module_name_lc, vtss_module_names[i], VTSS_PRIV_LVL_NAME_LEN_MAX - 1);
        module_name_lc[VTSS_PRIV_LVL_NAME_LEN_MAX - 1] = 0;
        VTSS_PRIVILEGE_strn_tolower(module_name_lc, strlen(module_name_lc));
        if (next) {
            if ((found && (strcmp(module_name_lc, name_lc) > 0 && strcmp(module_name_lc, upper_name) < 0)) ||
                (!found && strcmp(module_name_lc, name_lc) > 0)) {
                strcpy(upper_name, module_name_lc);
                *module_id_p = (vtss_module_id_t) i;
                if (!found && strcmp(module_name_lc, name_lc) > 0) {
                    found = 1;
                }
            }
        } else if (strcmp(module_name_lc, name_lc) == 0) {
            *module_id_p = (vtss_module_id_t) i;
            found = 1;
            break;
        }
    }

    if (found) {
        strcpy(name, vtss_module_names[*module_id_p]);
        T_D("exit");
        return TRUE;
    } else {
        T_D("exit");
        return FALSE;
    }
}

/**
  * \brief Get privilege group name list
  *
  * \param max_cnt  [IN]: The maximum count of privilege group name.
  * \param list_p   [OUT]: The point list of  privilege group name.
  *
  * \return
  *    TRUE if the module name is found.\n
  *    FALSE otherwise.\n
  */
void vtss_privilege_group_name_list_get(const u32 max_cnt, const char *list_p[])
{
    u32 i, n;

    // Insert unique entries into list. We do that by searching for existing
    // matching entries. That's expensive, of course, but it's a rare operation
    // so we live with it.

    T_D("enter");
    for (i = n = 0; i < VTSS_MODULE_ID_NONE  &&  n < max_cnt - 1; i++) {
        if (vtss_priv_lvl_groups_filter[i] || vtss_module_names[i] == NULL) {
            continue;
        }
        list_p[n++] = vtss_module_names[i];
    }

    if (i < VTSS_MODULE_ID_NONE  &&  n == max_cnt - 1) {
        T_E("web privilege group name list full; truncating. i = %d, n = %d", i, n);
    }
    list_p[n] = NULL;
}

/*
==============================================================================

    Public APIs in vtss_appl\include\vtss\appl\privilege.h

==============================================================================
*/
/*
    compare string length first, then compare string

    Return:
            <  0 : s1 < s2
            == 0 : s1 == s2
            >  0 : s1 > s2
*/
static int _mib_str_cmp(
    const char     *const s1,
    const char     *const s2
)
{
    size_t  s1_len = strlen(s1);
    size_t  s2_len = strlen(s2);

    if (s1_len < s2_len) {
        return -1;
    }

    if (s1_len > s2_len) {
        return 1;
    }

    return strcmp(s1, s2);
}

/**
 * \brief Iterate function of Web Privilege Configuration
 *
 * To get first and get next indexes.
 *
 * \param prev_moduleName [IN]  previous module name.
 * \param next_moduleName [OUT] next module name.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_privilege_config_web_itr(
    const vtss_appl_privilege_module_name_t     *const prev_moduleName,
    vtss_appl_privilege_module_name_t           *const next_moduleName
)
{
    u32     i;
    char    current_moduleName[VTSS_APPL_PRIVILEGE_NAME_MAX_LEN + 1];
    u32     next_moduleId;

    if (next_moduleName == NULL) {
        T_E("next_moduleName == NULL\n");
        return VTSS_RC_ERROR;
    }

    if (prev_moduleName && strlen(prev_moduleName->name) > VTSS_APPL_PRIVILEGE_NAME_MAX_LEN) {
        return VTSS_RC_ERROR;
    }

    if (prev_moduleName) {
        strcpy(current_moduleName, prev_moduleName->name);
    } else {
        memset(current_moduleName, 0, sizeof(current_moduleName));
    }

    /*
        find next of current_moduleName
        compare string length then string
    */
    next_moduleId = VTSS_MODULE_ID_NONE;
    for (i = 0; i < VTSS_MODULE_ID_NONE; ++i) {
        if (vtss_priv_lvl_groups_filter[i] || vtss_module_names[i] == NULL) {
            continue;
        }

        /*
            check if this module is larger than current and less than next
        */

        /* compare current */
        if (_mib_str_cmp(vtss_module_names[i], current_moduleName) <= 0) {
            // this module is less than or equal to current
            continue;
        }

        /* compare next */
        if (next_moduleId == VTSS_MODULE_ID_NONE) {
            next_moduleId = i;
        } else {
            // if this module less than next, then this is next
            if (_mib_str_cmp(vtss_module_names[i], vtss_module_names[next_moduleId]) < 0) {
                next_moduleId = i;
            }
        }
    }

    if (next_moduleId == VTSS_MODULE_ID_NONE) {
        return VTSS_RC_ERROR;
    }

    memset(next_moduleName, 0, sizeof(vtss_appl_privilege_module_name_t));
    strcpy(next_moduleName->name, vtss_module_names[next_moduleId]);

    return VTSS_RC_OK;
}

/**
 * \brief Get Web Privilege Configuration
 *
 * To read configuration of web privilege.
 *
 * \param moduleName [IN]  (key) Module name.
 * \param conf       [OUT] The configuration of web privilege
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_privilege_config_web_get(
    vtss_appl_privilege_module_name_t   moduleName,
    vtss_appl_privilege_config_web_t    *const conf
)
{
    vtss_module_id_t    moduleId;
    vtss_priv_conf_t    *priv_conf;

    if (vtss_privilege_module_to_val(moduleName.name, &moduleId) == FALSE) {
        return VTSS_RC_ERROR;
    }

    if (conf == NULL) {
        T_E("conf == NULL\n");
        return VTSS_RC_ERROR;
    }

    PRIVILEGE_CRIT_ENTER();

    priv_conf = &(VTSS_PRIVILEGE_global.privilege_conf);

    conf->configRoPriv = priv_conf->privilege_level[moduleId].configRoPriv;
    conf->configRwPriv = priv_conf->privilege_level[moduleId].configRwPriv;
    conf->statusRoPriv = priv_conf->privilege_level[moduleId].statusRoPriv;
    conf->statusRwPriv = priv_conf->privilege_level[moduleId].statusRwPriv;

    PRIVILEGE_CRIT_EXIT();

    return VTSS_RC_OK;
}

/**
 * \brief Set Web Privilege Configuration
 *
 * To modify configuration of web privilege.
 *
 * \param moduleName [IN] (key) Module name.
 * \param conf       [IN] The configuration of web privilege
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_privilege_config_web_set(
    vtss_appl_privilege_module_name_t           moduleName,
    const vtss_appl_privilege_config_web_t      *const conf
)
{
    vtss_module_id_t    moduleId;
    vtss_priv_conf_t    *priv_conf;
    BOOL                b_change;
    users_conf_t        users_conf;

    T_D("enter: module name: %s", moduleName.name);

    if (vtss_privilege_module_to_val(moduleName.name, &moduleId) == FALSE) {
        T_D("exit: Cannot find the privilege value mapping for module name: %s", moduleName.name);
        return VTSS_RC_ERROR;
    }

    if (conf == NULL) {
        T_E("exit: conf == NULL\n");
        return VTSS_RC_ERROR;
    }

    if (conf->configRoPriv > VTSS_APPL_PRIVILEGE_LVL_MAX) {
        T_D("exit: Config Read-Only privilege level %d is greater than %d", conf->configRoPriv, VTSS_APPL_PRIVILEGE_LVL_MAX);
        return VTSS_RC_ERROR;
    }

    if (conf->configRwPriv > VTSS_APPL_PRIVILEGE_LVL_MAX) {
        T_D("exit: Config Read-Write privilege level %d is greater than %d", conf->configRwPriv, VTSS_APPL_PRIVILEGE_LVL_MAX);
        return VTSS_RC_ERROR;
    }

    if (conf->statusRoPriv > VTSS_APPL_PRIVILEGE_LVL_MAX) {
        T_D("exit: Status Read-Only privilege level %d is greater than %d", conf->statusRoPriv, VTSS_APPL_PRIVILEGE_LVL_MAX);
        return VTSS_RC_ERROR;
    }

    if (conf->statusRwPriv > VTSS_APPL_PRIVILEGE_LVL_MAX) {
        T_D("exit: Status Read-Write privilege level %d is greater than %d", conf->statusRwPriv, VTSS_APPL_PRIVILEGE_LVL_MAX);
        return VTSS_RC_ERROR;
    }

    /* The privilege level of 'Read-only' should be less or equal 'Read-write' */
    if (conf->configRoPriv > conf->configRwPriv) {
        T_D("exit: Config Read-Only privilege level %d is greater than Config Read-Write privilege level %d", conf->configRoPriv, conf->configRwPriv);
        return VTSS_RC_ERROR;
    }
    if (conf->statusRoPriv > conf->statusRwPriv) {
        T_D("exit: Status Read-Only privilege level %d is greater than Status Read-Write privilege level %d", conf->configRoPriv, conf->configRwPriv);
        return VTSS_RC_ERROR;
    }

    /* The privilege level of 'Configuration/Execute Read-write' should be great or equal 'Status/Statistics Read-only' */
    if (conf->configRwPriv < conf->statusRoPriv) {
        T_D("exit: Config Read-Write privilege level %d is smaller than Status Read-Only privilege level %d", conf->configRwPriv, conf->statusRoPriv);
        return VTSS_RC_ERROR;
    }

    /* Change to lower privilege level will lock yourself out */
    if (moduleId == VTSS_MODULE_ID_MISC) {
        strcpy(users_conf.username, VTSS_SYS_ADMIN_NAME);
        if (vtss_users_mgmt_conf_get(&users_conf, FALSE) == VTSS_RC_OK) {
            if ((int)(conf->configRwPriv) > users_conf.privilege_level) {
                T_D("exit: Change to lower privilege level %d will lock yourself out(%d)", conf->configRwPriv, users_conf.privilege_level);
                return VTSS_RC_ERROR;
            }
        }
    }

    PRIVILEGE_CRIT_ENTER();

    priv_conf = &(VTSS_PRIVILEGE_global.privilege_conf);

    b_change = FALSE;
    if (conf->configRoPriv != priv_conf->privilege_level[moduleId].configRoPriv ||
        conf->configRwPriv != priv_conf->privilege_level[moduleId].configRwPriv ||
        conf->statusRoPriv != priv_conf->privilege_level[moduleId].statusRoPriv ||
        conf->statusRwPriv != priv_conf->privilege_level[moduleId].statusRwPriv  ) {
        b_change = TRUE;
    }

    if (b_change == FALSE) {
        PRIVILEGE_CRIT_EXIT();
        T_D("exit: New configuration is the same as original one");
        return VTSS_RC_OK;
    }

    priv_conf->privilege_level[moduleId].configRoPriv = conf->configRoPriv;
    priv_conf->privilege_level[moduleId].configRwPriv = conf->configRwPriv;
    priv_conf->privilege_level[moduleId].statusRoPriv = conf->statusRoPriv;
    priv_conf->privilege_level[moduleId].statusRwPriv = conf->statusRwPriv;

    PRIVILEGE_CRIT_EXIT();

    T_D("exit: SET operation success");
    return VTSS_RC_OK;
}

