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

#include "main.h"
#include "msg_api.h"
#include "critd_api.h"
#include "misc_api.h"
#include "vtss_users_api.h"
#include "vtss_users.h"
#include "sysutil_api.h"
#include "conf_api.h"
#if defined(VTSS_SW_OPTION_FAST_CGI)
#include "fast_cgi_api.hxx"
#endif

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#endif

#ifdef VTSS_SW_OPTION_AUTH
#include "vtss_auth_api.h"
#endif /* VTSS_SW_OPTION_AUTH */

#ifdef VTSS_SW_OPTION_ICFG
#include "vtss_users_icfg.h"
#endif

/* Verify some constants until iCLI uses common header files */
#if VTSS_APPL_USERS_NAME_LEN != VTSS_SYS_INPUT_USERNAME_LEN
#error VTSS_APPL_USERS_NAME_LEN != VTSS_SYS_INPUT_USERNAME_LEN
#endif
#if VTSS_APPL_USERS_PASSWORD_LEN != (VTSS_SYS_PASSWD_ENCRYPTED_LEN - 1)
#error VTSS_APPL_USERS_PASSWORD_LEN != (VTSS_SYS_PASSWD_ENCRYPTED_LEN - 1)
#endif
#if VTSS_APPL_PRIVILEGE_LEVEL_MAX != VTSS_USERS_MAX_PRIV_LEVEL
#error VTSS_APPL_PRIVILEGE_LEVEL_MAX != VTSS_USERS_MAX_PRIV_LEVEL
#endif

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Global structure */
static users_global_t VTSS_USERS_global;

static vtss_trace_reg_t VTSS_USERS_trace_reg = {
    VTSS_TRACE_MODULE_ID, "users", "Users module"
};

static vtss_trace_grp_t VTSS_USERS_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&VTSS_USERS_trace_reg, VTSS_USERS_trace_grps);

#define USERS_CRIT_ENTER() critd_enter(&VTSS_USERS_global.crit, __FILE__, __LINE__)
#define USERS_CRIT_EXIT()  critd_exit( &VTSS_USERS_global.crit, __FILE__, __LINE__)

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/

/* Encode the password.
 * Parameter:
 * conf - The point of users configuration.
 * salt_padding - The salt for the hash function.
 *
 * Notes that the hash process only affect when the input parameter 'conf->encrypted == FALSE',
 * and it will generate a random salt padding when the input parameter 'salt_padding == NULL'.
 */
static void VTSS_USERS_password_encode(users_conf_t *conf, char salt_padding[VTSS_USERS_HASH_DIGEST_LEN * 2 + 1])
{
    if (!conf->encrypted) {
        char clear_pwd[VTSS_USERS_PASSWORD_MAX_LEN];

        misc_strncpyz(clear_pwd, conf->password, sizeof(clear_pwd));
        sysutil_password_encode(conf->username, clear_pwd, salt_padding, conf->password);
        conf->encrypted = TRUE;
    }
}

/* Determine if users configuration has changed */
static int VTSS_USERS_conf_changed(u32 old_num, users_conf_t old[VTSS_USERS_NUMBER_OF_USERS], u32 new_num, users_conf_t new_[VTSS_USERS_NUMBER_OF_USERS])
{
    return (old_num != new_num ||
            memcmp(new_, old, sizeof(users_conf_t) * VTSS_USERS_NUMBER_OF_USERS));
}

/* Set users defaults */
static void VTSS_USERS_default_set(u32 *users_conf_num, users_conf_t conf[VTSS_USERS_NUMBER_OF_USERS])
{
    /* A least one administor user as default setting: For example:
       user name        : admin
       password         :
       privilege level  : 15 */
    conf_board_t board;
    *users_conf_num = 1;
    memset(conf, 0x0, sizeof(users_conf_t) * VTSS_USERS_NUMBER_OF_USERS);
    conf[0].valid = 1;
    strcpy(conf[0].username, VTSS_SYS_ADMIN_NAME);     /* Refer to \webstax2\vtss_appl\sysutil\sysutil_api.h */
    conf_mgmt_board_get(&board);
    strcpy(conf[0].password, board.default_password); /* Refer to \webstax2\vtss_appl\sysutil\sysutil_api.h */
    conf[0].privilege_level = VTSS_USERS_MAX_PRIV_LEVEL;

    // The password always be saved as encrypted password
    VTSS_USERS_password_encode(&conf[0], NULL);
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/**
  * \brief Retrieve an error string based on a return code
  *        from one of the Users API functions.
  *
  * \param rc [IN]: Error code that must be in the VTSS_USERS_ERROR_xxx range.
  */
const char *vtss_users_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_USERS_ERROR_MUST_BE_PRIMARY_SWITCH:
        return "Operation only valid on primary switch switch";

    case VTSS_USERS_ERROR_ISID:
        return "Invalid Switch ID";

    case VTSS_USERS_ERROR_INV_PARAM:
        return "Illegal parameter";

    case VTSS_USERS_ERROR_REJECT:
        return "Username and password combination not found";

    case VTSS_USERS_ERROR_CFG_INVALID_USERNAME:
        return "Invalid username";

    case VTSS_USERS_ERROR_CFG_INVALID_PASSWORD:
        return "Invalid password";

    case VTSS_USERS_ERROR_USERS_TABLE_FULL:
        return "Users table full";

    case VTSS_USERS_ERROR_USERS_DEL_ADMIN:
        return "Cannot delete system default administrator";

    case VTSS_USERS_ERROR_LOWER_PRIV_LVL_LOCK_YOUSELF:
        return "Change to lower privilege level will lock yourself out";

    case VTSS_USERS_ERROR_USERNAME_NOT_EXISTING:
        return "Username is not existing";

    default:
        return "Users: Unknown error code";
    }
}

/* Get users configuration (only for local using) */
static mesa_rc VTSS_USERS_conf_get(u32 *users_conf_num, users_conf_t conf[VTSS_USERS_NUMBER_OF_USERS])
{
    T_D("enter");
    USERS_CRIT_ENTER();

    *users_conf_num = VTSS_USERS_global.users_conf_num;
    memcpy(conf, VTSS_USERS_global.users_conf, sizeof(users_conf_t) * VTSS_USERS_NUMBER_OF_USERS);

    USERS_CRIT_EXIT();
    T_D("exit");

    return VTSS_RC_OK;
}

/* Set users configuration (only for local using) */
static mesa_rc VTSS_USERS_conf_set(u32 users_conf_num, users_conf_t conf[VTSS_USERS_NUMBER_OF_USERS])
{
    mesa_rc             rc      = VTSS_RC_OK;
    int                 changed = 0;

    T_D("enter");

    USERS_CRIT_ENTER();
    if (msg_switch_is_primary()) {
        changed = VTSS_USERS_conf_changed(VTSS_USERS_global.users_conf_num, VTSS_USERS_global.users_conf, users_conf_num, conf);
        VTSS_USERS_global.users_conf_num = users_conf_num;
        memcpy(VTSS_USERS_global.users_conf, conf, sizeof(users_conf_t) * VTSS_USERS_NUMBER_OF_USERS);
    } else {
        T_W("not primary switch");
        rc = VTSS_USERS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    USERS_CRIT_EXIT();

    if (changed) {
#ifdef VTSS_SW_OPTION_AUTH
        vtss_auth_mgmt_httpd_cache_expire(); // Clear httpd cache
#endif /* VTSS_SW_OPTION_AUTH */
    }

    T_D("exit");

    return rc;
}

/**
  * \brief Get the global Users configuration.
  *
  * \param glbl_cfg [OUT]: Pointer to structure that receives
  *                        the current configuration.
  * \param next     [IN]:  Getnext?
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VTSS_USERS_ERROR_INV_PARAM if glbl_cfg is NULL.\n
  *    VTSS_USERS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    VTSS_USERS_ERROR_REJECT if get fail.\n
  */
mesa_rc vtss_users_mgmt_conf_get(users_conf_t *glbl_cfg, BOOL next)
{
    u32 i, num, found = 0;
    users_conf_t *tmp;
#if defined(VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR)
    users_conf_t blank_user;
#endif /* VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR */

    T_D("enter");

    if (glbl_cfg == NULL) {
        T_W("not primary switch");
        T_D("exit");
        return VTSS_USERS_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return VTSS_USERS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    tmp = NULL;

    USERS_CRIT_ENTER();
#if defined(VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR)
    memset(&blank_user, 0, sizeof(blank_user));
    if (next && !memcmp(glbl_cfg, &blank_user, sizeof(blank_user))) {
        // GetFirst
        found = 1;
        tmp = &VTSS_USERS_global.users_conf[0];
    } else
#endif /* VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR */
    {
        for (i = 0, num = 0;
             i < VTSS_USERS_NUMBER_OF_USERS && num < VTSS_USERS_global.users_conf_num;
             i++) {

            if ( VTSS_USERS_global.users_conf[i].valid == FALSE ) {
                continue;
            }

            num++;

            T_D("i:%d, username:%s", i, VTSS_USERS_global.users_conf[i].username);
            if ( next ) {
                /* first index is string length, then alphabet order */
                if ( strlen(VTSS_USERS_global.users_conf[i].username) > strlen(glbl_cfg->username) ||
                     ( strlen(VTSS_USERS_global.users_conf[i].username) == strlen(glbl_cfg->username) &&
                       strcmp(VTSS_USERS_global.users_conf[i].username, glbl_cfg->username) > 0 ) ) {
                    if ( tmp ) {
                        if ( strlen(VTSS_USERS_global.users_conf[i].username) > strlen(tmp->username) ||
                             ( strlen(VTSS_USERS_global.users_conf[i].username) == strlen(tmp->username) &&
                               strcmp(VTSS_USERS_global.users_conf[i].username, tmp->username) >= 0 ) ) {
                            continue;
                        }
                    }
                    tmp = &VTSS_USERS_global.users_conf[i];
                    found = 1;
                }
            } else {
                T_D("Looking for:%s, found:%s", glbl_cfg->username, VTSS_USERS_global.users_conf[i].username);
                if ( strcmp(VTSS_USERS_global.users_conf[i].username, glbl_cfg->username) == 0 ) {
                    found = 1;
                    tmp = &VTSS_USERS_global.users_conf[i];
                    break;
                }
            }
        }
    }
    USERS_CRIT_EXIT();

    T_D("exit");
    if ( found && tmp ) {
        *glbl_cfg = *tmp;
        return VTSS_RC_OK;
    } else {
        return VTSS_USERS_ERROR_REJECT;
    }
}

/**
  * \brief Vereify the clear password if valid.
  *
  * \param username [IN]: Pointer to username.
  * \param password [IN]: Pointer to password.
  *
  * \return
  *    TRUE when the username/password is valid, FALSE otherwise.
  */
BOOL vtss_users_mgmt_verify_clear_password(const char *username, const char *password)
{
    BOOL            is_valid = FALSE;
    users_conf_t    search_conf, *search_conf_p = &search_conf;
    u32             i, num;
    char            salt_apdding[VTSS_SYS_PASSWD_SALT_PADDING_LEN * 2 + 1];

    T_D("enter: username=%s, password=%s", username, password);

    /* Parameter checking */
    if (!username || !password) {
        T_E("exit: Null point");
        return is_valid;
    }
    if (!msg_switch_is_primary()) {
        T_W("exit: Not primary switch");
        return is_valid;
    }

    // Copy to a local variable since we always save the encrypted passward in database
    memset(&search_conf, 0, sizeof(search_conf));
    misc_strncpyz(search_conf.username, username, sizeof(search_conf.username));

    /* lookup users database */
    USERS_CRIT_ENTER();
    for (i = 0, num = 0;
         i < VTSS_USERS_NUMBER_OF_USERS && num < VTSS_USERS_global.users_conf_num;
         i++) {
        if (!VTSS_USERS_global.users_conf[i].valid) {
            continue;
        }
        num++;

        T_D("Compare: username=%s, encrypted=%s", VTSS_USERS_global.users_conf[i].username, VTSS_USERS_global.users_conf[i].encrypted ? "T" : "F");
        if (!strcmp(VTSS_USERS_global.users_conf[i].username, search_conf_p->username)) {
            // Encrypted password
            misc_strncpyz(search_conf.password, password, sizeof(search_conf.password));
            search_conf.encrypted = FALSE;
            misc_strncpyz(salt_apdding, &VTSS_USERS_global.users_conf[i].password[VTSS_USERS_HASH_DIGEST_LEN * 2], sizeof(salt_apdding));
            VTSS_USERS_password_encode(&search_conf, salt_apdding);

            if (sysutil_slow_equals((u8 *)VTSS_USERS_global.users_conf[i].password, VTSS_SYS_PASSWD_ENCRYPTED_LEN - 1, (u8 *)search_conf_p->password, VTSS_SYS_PASSWD_ENCRYPTED_LEN - 1)) {
                // The username/password are matched
                is_valid = TRUE;
                break;
            }
        }
    }
    USERS_CRIT_EXIT();

    T_D("exit: is_valid=%s", is_valid ? "T" : "F");
    return is_valid;
}

/**
  * \brief Set the global Users configuration.
  *
  * \param glbl_cfg [IN]: Pointer to structure that contains the
  *                       global configuration to apply to the
  *                       voice VLAN module.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VTSS_USERS_ERROR_INV_PARAM if glbl_cfg is NULL or parameters error.\n
  *    VTSS_USERS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    VTSS_USERS_ERROR_CFG_INVALID_USERNAME if user name is null string.\n
  *    VTSS_USERS_ERROR_USERS_TABLE_FULL if users table is full.\n
  *    Others value is caused form other modules.\n
  */
mesa_rc vtss_users_mgmt_conf_set(users_conf_t *glbl_cfg)
{
    mesa_rc         rc = VTSS_RC_OK;
    int             changed = 0, found = 0;
    u32             i, num, users_conf_num;
    users_conf_t    set_conf, *set_conf_p = &set_conf, users_conf[VTSS_USERS_NUMBER_OF_USERS];
    char            changed_username[VTSS_SYS_USERNAME_LEN];

    T_D("enter: username=%s, encrypted=%s, privilege_level=%d", glbl_cfg->username, glbl_cfg->encrypted ? "T" : "F", glbl_cfg->privilege_level);

    if (!glbl_cfg) {
        T_E("exit: Null point of users configuration");
        return VTSS_USERS_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("exit: Not primary switch");
        return VTSS_USERS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (glbl_cfg->encrypted != TRUE && glbl_cfg->encrypted != FALSE) {
        T_W("exit: encrypted not equal TRUE or FALSE");
        return VTSS_USERS_ERROR_INV_PARAM;
    }

    /* Check illegal parameter */
    if (strcmp(glbl_cfg->username, VTSS_SYS_ADMIN_NAME) &&
        !vtss_users_mgmt_is_valid_username(glbl_cfg->username)) {
        T_D("exit: Invalid username");
        return VTSS_USERS_ERROR_CFG_INVALID_USERNAME;
    }

#if !defined(VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR)
    if (!strcmp(glbl_cfg->username, "")) {
        T_D("exit: Invalid username (empty string)");
        return VTSS_USERS_ERROR_CFG_INVALID_USERNAME;
    }
#endif /* !VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR */

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (glbl_cfg->privilege_level < VTSS_USERS_MIN_PRIV_LEVEL ||
        glbl_cfg->privilege_level > VTSS_USERS_MAX_PRIV_LEVEL) {
        T_D("exit: Invalid privilege level=%d", glbl_cfg->privilege_level);
        return VTSS_USERS_ERROR_INV_PARAM;
    }
    if ((!strcmp(glbl_cfg->username, VTSS_SYS_ADMIN_NAME)) &&
        (!vtss_priv_is_allowed_crw(VTSS_MODULE_ID_MISC, glbl_cfg->privilege_level))) {
        /* Change to lower privilege level will lock yourself out */
        T_D("exit: Invalid configuation, change to lower privilege level will lock yourself out");
        return VTSS_USERS_ERROR_LOWER_PRIV_LVL_LOCK_YOUSELF;
    }
#endif

    if ((glbl_cfg->encrypted && strlen(glbl_cfg->password) != (VTSS_SYS_PASSWD_ENCRYPTED_LEN - 1)) ||
        (!glbl_cfg->encrypted && strlen(glbl_cfg->password) > VTSS_SYS_INPUT_PASSWD_LEN)) {
        T_W("exit: Encrypted parameter not equal TRUE or FALSE");
        return VTSS_USERS_ERROR_CFG_INVALID_PASSWORD;
    }

    // Check format
    if (glbl_cfg->encrypted) {
        if (!sysutil_encrypted_password_fomat_is_valid(glbl_cfg->username, glbl_cfg->password)) {
            T_D("exit: Calling sysutil_encrypted_password_fomat_is_valid() failed");
            return VTSS_USERS_ERROR_CFG_INVALID_PASSWORD;
        }
    }

    /* Get local users configuration */
    if ((rc = VTSS_USERS_conf_get(&users_conf_num, users_conf)) != VTSS_RC_OK) {
        T_D("exit: Calling VTSS_USERS_conf_get() failed");
        return rc;
    }

    if (glbl_cfg->encrypted) {
        set_conf_p = glbl_cfg;
    } else {
        // Copy to a local variable since we always save the encrypted passward in database
        set_conf = *glbl_cfg;
        VTSS_USERS_password_encode(&set_conf, NULL);
    }

    USERS_CRIT_ENTER();
    for (i = 0, num = 0;
         i < VTSS_USERS_NUMBER_OF_USERS && num < users_conf_num;
         i++) {
        if (!users_conf[i].valid) {
            continue;
        }
        num++;
        if (!strcmp(users_conf[i].username, set_conf_p->username)) {
            T_D("Found the configration in existing database");
            found = 1;
            break;
        }
    }

    if (i < VTSS_USERS_NUMBER_OF_USERS && found) {
        /* Modify exist entry */
        if (memcmp(&users_conf[i], set_conf_p, sizeof(users_conf_t))) {
            strcpy(changed_username, users_conf[i].username);
            users_conf[i] = *set_conf_p;
            users_conf[i].valid = 1;
            changed = 1;
        }
    } else {
        /* Add new entry */
        for (i = 0; i < VTSS_USERS_NUMBER_OF_USERS; i++) {
            if (users_conf[i].valid) {
                continue;
            }
            users_conf_num++;
            users_conf[i] = *set_conf_p;
            users_conf[i].valid = 1;
            break;
        }
        if (i < VTSS_USERS_NUMBER_OF_USERS) {
            changed = 1;
        } else {
            T_D("Table full");
            rc = VTSS_USERS_ERROR_USERS_TABLE_FULL;
        }
    }
    USERS_CRIT_EXIT();

    if (changed) {
        /* Save changed configuration */
        rc = VTSS_USERS_conf_set(users_conf_num, users_conf);
    }

    T_D("exit, rc:%d", rc);
    return rc;
}

/**
 * Delete the Users configuration.
 * \param user_name [IN] The user name
 * \return : VTSS_RC_OK or one of the following
 *  VTSS_USERS_ERROR_GEN (conf is a null pointer)
 *  VTSS_USERS_ERROR_MUST_BE_PRIMARY_SWITCH
 */
mesa_rc vtss_users_mgmt_conf_del(char *username)
{
    mesa_rc         rc = VTSS_RC_OK;
    int             changed = 0, found = 0;
    u32             i, num, users_conf_num;
    users_conf_t    users_conf[VTSS_USERS_NUMBER_OF_USERS];
    char            changed_username[VTSS_SYS_USERNAME_LEN];

    T_D("enter");

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return VTSS_USERS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    /* Check illegal parameter */
    if (username == NULL || !strcmp(username, VTSS_SYS_ADMIN_NAME)) {
        T_D("exit");
        return rc;
    }

    if ((rc = VTSS_USERS_conf_get(&users_conf_num, users_conf)) != VTSS_RC_OK) {
        T_D("exit");
        return rc;
    }

    USERS_CRIT_ENTER();

    if (msg_switch_is_primary()) {
        for (i = 0, num = 0;
             i < VTSS_USERS_NUMBER_OF_USERS && num < users_conf_num;
             i++) {
            if (!users_conf[i].valid) {
                continue;
            }
            num++;
            if (!strcmp(users_conf[i].username, username)) {
                found = 1;
                break;
            }
        }

        if (!found) {
            rc = VTSS_USERS_ERROR_USERNAME_NOT_EXISTING;
        } else if (i < VTSS_USERS_NUMBER_OF_USERS && found) {
            users_conf_num--;
            strcpy(changed_username, users_conf[i].username);
            memset(&users_conf[i], 0x0, sizeof(users_conf_t));
            changed = 1;
        }
    } else {
        T_W("not primary switch");
        T_D("exit");
        rc = VTSS_USERS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    USERS_CRIT_EXIT();

    if (changed) {
        /* Save changed configuration */
        rc = VTSS_USERS_conf_set(users_conf_num, users_conf);
    }

    T_D("exit, rc:%d", rc);
    return rc;
}

/**
 * Clear the Users configuration.
 */
mesa_rc vtss_users_mgmt_conf_clear(void)
{
    users_conf_t conf;
    int          admin_cnt = 0;

    T_D("enter");

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return VTSS_USERS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    memset(&conf, 0x0, sizeof(conf));
    while (vtss_users_mgmt_conf_get(&conf, 1) == VTSS_RC_OK) {
        if (strcmp(conf.username, VTSS_SYS_ADMIN_NAME)) {
            if (vtss_users_mgmt_conf_del(conf.username) != VTSS_RC_OK) {
                T_D("Calling vtss_users_mgmt_conf_del(%s) failed\n", conf.username);
            }
        } else if (admin_cnt == 0) {
            admin_cnt++;
        } else {
            break;
        }
    }

    T_D("exit");
    return VTSS_RC_OK;
}


/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

/* Read/create users stack configuration */
static void VTSS_USERS_conf_read_stack(BOOL create)
{
    u32                 new_users_conf_num = 0;
    users_conf_t        new_users_conf[VTSS_USERS_NUMBER_OF_USERS];

    T_D("enter, create: %d", create);

    USERS_CRIT_ENTER();
    /* Use default values first. (Quiet lint/Coverity) */
    VTSS_USERS_default_set(&new_users_conf_num, new_users_conf);

    VTSS_USERS_global.users_conf_num = new_users_conf_num;
    memcpy(VTSS_USERS_global.users_conf, new_users_conf, sizeof(users_conf_t) * VTSS_USERS_NUMBER_OF_USERS);
    USERS_CRIT_EXIT();

    T_D("exit");
}

/* Module start */
static void VTSS_USERS_start(void)
{
    T_D("enter");

    /* Initialize users configuration */
    VTSS_USERS_default_set(&VTSS_USERS_global.users_conf_num, VTSS_USERS_global.users_conf);

    /* Create semaphore for critical regions */
    critd_init(&VTSS_USERS_global.crit, "users", VTSS_MODULE_ID_USERS, CRITD_TYPE_MUTEX);

    T_D("exit");
}

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize our private mib */
VTSS_PRE_DECLS void users_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_users_json_init(void);
#endif
extern "C" int vtss_users_icli_cmd_register();

/**
  * \brief Initialize the Users module
  *
  * \param cmd [IN]: Reason why calling this function.
  * \param p1  [IN]: Parameter 1. Usage varies with cmd.
  * \param p2  [IN]: Parameter 2. Usage varies with cmd.
  *
  * \return
  *    VTSS_RC_OK.
  */
mesa_rc vtss_users_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
#ifdef VTSS_SW_OPTION_ICFG
    mesa_rc     rc;
#endif

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
        VTSS_USERS_start();

#ifdef VTSS_SW_OPTION_ICFG
        rc = vtss_users_icfg_init();
        if (rc != VTSS_RC_OK) {
            T_D("fail to init icfg registration, rc = %s", error_txt(rc));
        }
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register our private mib */
        users_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_users_json_init();
#endif
        vtss_users_icli_cmd_register();
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            VTSS_USERS_conf_read_stack(1);
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
        }
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");

        /* Read stack and switch configuration */
        VTSS_USERS_conf_read_stack(0);
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

/* Check if user name string */
BOOL vtss_users_mgmt_is_valid_username(const char *str)
{
    int idx, len = strlen(str);

#if !defined(VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR)
    if (!len) {
        return FALSE;
    }
#endif /* !VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR */

    for (idx = 0; idx < len; idx++) {
        if ((str[idx] >= '0' && str[idx] <= '9') ||
            (str[idx] >= 'A' && str[idx] <= 'Z') ||
            (str[idx] >= 'a' && str[idx] <= 'z') ||
            str[idx] == '_') {
            continue;
        } else {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL vtss_users_mgmt_is_printable_string(char *encry_password)
{
    int idx, len = strlen(encry_password);
    for (idx = 0; idx < len; idx++) {
        if (encry_password[idx] < 32 || encry_password[idx] > 126) {
            return FALSE;
        }
    }
    return TRUE;
}

/*
==============================================================================

    Public APIs in vtss_appl\include\vtss\appl\users.h

==============================================================================
*/
#include "vtss_os_wrapper.h"

/**
 * \brief Iterate function of Users Configuration
 *
 * To get first and get next indexes.
 *
 * \param prev_username [IN]  previous user name.
 * \param next_username [OUT] next user name.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_users_config_itr(
    const vtss_appl_users_username_t    *const prev_username,
    vtss_appl_users_username_t          *const next_username
)
{
    users_conf_t    uconf;

    /* check parameter */
    if ( next_username == NULL ) {
        return VTSS_RC_ERROR;
    }

    memset(&uconf, 0, sizeof(uconf));

    if ( prev_username ) {
        /* get next */
#if defined(VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR)
        // Since default username is a null string,
        // so the function vtss_users_mgmt_conf_get(&uconf, TRUE) uses empty username
        // configuration for GetFirst operation.
        // For GetNext operation, set any field to noe-zero value e.g. "uconf.valid=1"
        // in order to make something different with empty username configuration.
        uconf.valid = 1;
#endif /* VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR */
        strncpy(uconf.username, prev_username->username, VTSS_APPL_USERS_NAME_LEN);
#if defined(VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR)
    } else { // else get first
        // Since default username is a null string,
        // so the function vtss_users_mgmt_conf_get(&uconf, TRUE) uses empty username
        // configuration for GetFirst operation.
        // For GetNext operation, set any field to noe-zero value e.g. "uconf.valid=1"
        // in order to make something different with empty username configuration.
        uconf.valid = 1;
        strncpy(uconf.username, VTSS_SYS_ADMIN_NAME, VTSS_APPL_USERS_NAME_LEN);
#endif /* VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR */
    }

    VTSS_RC(vtss_users_mgmt_conf_get(&uconf, TRUE));

    /* get next */
    memset(next_username->username, 0, sizeof(vtss_appl_users_username_t));
    strncpy(next_username->username, uconf.username, VTSS_APPL_USERS_NAME_LEN);
    next_username->username[VTSS_APPL_USERS_NAME_LEN] = 0;

    return VTSS_RC_OK;
}

/**
 * \brief Get Users Configuration
 *
 * To read configuration of Users.
 *
 * \param username [IN]  (key) User name.
 * \param conf     [OUT] The configuration of the user
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_users_config_get(
    vtss_appl_users_username_t  username,
    vtss_appl_users_config_t    *const conf
)
{
    users_conf_t    uconf;

    /* check parameter */
    if ( conf == NULL ) {
        T_E("conf == NULL\n");
        return VTSS_RC_ERROR;
    }

    memset(&uconf, 0, sizeof(uconf));
    strncpy(uconf.username, username.username, VTSS_APPL_USERS_NAME_LEN);

    /* get */
    VTSS_RC(vtss_users_mgmt_conf_get(&uconf, FALSE))

    memset(conf, 0, sizeof(vtss_appl_users_config_t));

    conf->privilege = (u32)( uconf.privilege_level );

    // Always encode password for get function
    conf->encrypted = TRUE;
    strcpy(conf->password, uconf.password);

    return VTSS_RC_OK;
}

/**
 * \brief Set Users Configuration
 *
 * To add or modify configuration of Users.
 *
 * \param username [IN] (key) User name.
 * \param conf     [IN] The configuration of the user
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_users_config_set(
    vtss_appl_users_username_t      username,
    const vtss_appl_users_config_t  *const conf
)
{
    users_conf_t uconf;

    /* Check parameter */
    if (conf == NULL) {
        return VTSS_USERS_ERROR_INV_PARAM;
    }
    if (conf->privilege > VTSS_APPL_PRIVILEGE_LEVEL_MAX) {
        return VTSS_USERS_ERROR_INV_PARAM;
    }
    if (conf->encrypted != TRUE && conf->encrypted != FALSE) {
        return VTSS_USERS_ERROR_INV_PARAM;
    }

    // Check password length
    if ((conf->encrypted && strlen(conf->password) != VTSS_APPL_USERS_PASSWORD_LEN) ||
        (!conf->encrypted && strlen(conf->password) > VTSS_APPL_USERS_UNENCRYPTED_PASSWORD_LEN)) {
        return VTSS_USERS_ERROR_INV_PARAM;
    }

    // Fill user configuration
    memset(&uconf, 0, sizeof(uconf));
    uconf.valid = 1;
    strcpy(uconf.username, username.username);
    uconf.privilege_level = (int)( conf->privilege );
    uconf.encrypted = conf->encrypted;
    strcpy(uconf.password, conf->password);

    /* set */
    return vtss_users_mgmt_conf_set(&uconf);
}

/**
 * \brief Delete Users Configuration
 *
 * To delete configuration of Users.
 *
 * \param username [IN]  (key) User name.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_users_config_del(
    vtss_appl_users_username_t      username
)
{
    /* check parameter */
    if ( strcmp(username.username, VTSS_SYS_ADMIN_NAME) == 0 ) {
        return VTSS_USERS_ERROR_USERS_DEL_ADMIN;
    }

    /* delete */
    VTSS_RC(vtss_users_mgmt_conf_del(username.username));

    return VTSS_RC_OK;
}

/**
 * \brief Get myself username and privilege.
 *
 * \param info [OUT]: myself user information.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_users_whoami(
    vtss_appl_users_info_t    *const info
)
{
    /* check parameter */
    if ( info == NULL ) {
        T_E("conf == NULL\n");
        return VTSS_RC_ERROR;
    }

#if defined(VTSS_SW_OPTION_FAST_CGI)
    cyg_httpd_current_username(info->username, VTSS_APPL_USERS_NAME_LEN);
    info->privilege = (u32)cyg_httpd_current_privilege_level();
    return VTSS_RC_OK;
#else
    return VTSS_RC_ERROR;
#endif
}

