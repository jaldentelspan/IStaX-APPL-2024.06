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
/*
******************************************************************************

    Revision history
    > CP.Wang, 2012/09/03 10:33
        - create

******************************************************************************
*/

/*
******************************************************************************

    Include files

******************************************************************************
*/
#include <stdlib.h>
#include "icfg_api.h"
#include "icli_api.h"
#include "icli_porting_trace.h"

#ifdef __cplusplus
#include "enum_macros.hxx"
VTSS_ENUM_INC(icli_privilege_t);
#endif

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/
#define __VISIBLE_MIN_CHAR      33
#define __VISIBLE_MAX_CHAR      126
#define __VISIBLE_SIZE          (__VISIBLE_MAX_CHAR - __VISIBLE_MIN_CHAR + 1)
/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/
static BOOL                 g_banner_deli[__VISIBLE_SIZE];
static char                 g_banner[ICLI_BANNER_MAX_LEN + 1];
static char                 g_password[ICLI_PASSWORD_MAX_LEN + 1];
static char                 g_dev_name[ICLI_DEV_NAME_MAX_LEN + 1];
static icli_session_data_t  g_session_data;
static critd_t              g_mutex;

/*
******************************************************************************

    Static Function

******************************************************************************
*/
struct ICLILockScope {
    ICLILockScope(const char *file, int line)
        : file(file), line(line)
    {
        critd_enter(&g_mutex, file, line);
    }

    ~ICLILockScope(void)
    {
        critd_exit(&g_mutex, file, line);
    }
private:
    const char *file;
    const int  line;
};


static mesa_rc _banner_print(
    IN vtss_icfg_query_result_t     *result,
    IN const char                   *type
)
{
    mesa_rc     rc;
    char        *c;
    char        i;

    if ( g_banner[0] ) {
        /* find delimiter */
        memset(g_banner_deli, 0, sizeof(g_banner_deli));

        // the comment token can not be used for delimiter
        g_banner_deli['!' - __VISIBLE_MIN_CHAR] = TRUE;
        g_banner_deli['#' - __VISIBLE_MIN_CHAR] = TRUE;

        for ( c = g_banner; (*c) != 0; c++ ) {
            if ( (*c) >= __VISIBLE_MIN_CHAR && (*c) <= __VISIBLE_MAX_CHAR ) {
                g_banner_deli[(*c) - __VISIBLE_MIN_CHAR] = TRUE;
            }
        }
        for ( i = 0; i < __VISIBLE_SIZE; i++ ) {
            if ( g_banner_deli[(int)i] == FALSE ) {
                break;
            }
        }
        if ( i < __VISIBLE_SIZE ) {
            i += __VISIBLE_MIN_CHAR;
        } else {
            i = '#';
        }

        /*  */
        rc = vtss_icfg_printf(result, "banner %s %c%s%c\n", type, i, g_banner, i);
        if ( rc != VTSS_RC_OK ) {
            T_E("fail to print to icfg\n");
            return VTSS_RC_ERROR;
        }
    }
    return VTSS_RC_OK;
}

static mesa_rc _enable_password_print(
    IN vtss_icfg_query_result_t     *result,
    IN icli_privilege_t             priv
)
{
    mesa_rc     rc;

    if ( g_password[0] ) {
        if ( icli_enable_password_if_secret_get(priv) ) {
            rc = vtss_icfg_printf(result, "enable secret 5 level %d %s\n",
                                  priv, g_password);
            if ( rc != VTSS_RC_OK ) {
                T_E("fail to print to icfg\n");
                return VTSS_RC_ERROR;
            }
        } else {
            rc = vtss_icfg_printf(result, "enable password level %d %s\n",
                                  priv, g_password);
            if ( rc != VTSS_RC_OK ) {
                T_E("fail to print to icfg\n");
                return VTSS_RC_ERROR;
            }
        }
    }
    return VTSS_RC_OK;
}

/* ICFG callback functions */
static mesa_rc _icli_icfg(
    IN const vtss_icfg_query_request_t  *req,
    IN vtss_icfg_query_result_t         *result
)
{
    mesa_rc              rc;
    BOOL                 b_print, is_default;
    icli_privilege_t     priv;
    i32                  sec;
    icli_priv_cmd_conf_t cmd_conf;
    const char           *prompt;

    if ( req == NULL ) {
        T_E("req == NULL\n");
        return VTSS_RC_ERROR;
    }

    if ( result == NULL ) {
        T_E("result == NULL\n");
        return VTSS_RC_ERROR;
    }
    ICLILockScope lock(__FILE__, __LINE__);

    switch ( req->cmd_mode ) {
    case ICLI_CMD_MODE_GLOBAL_CONFIG:
        /* banner motd */
        if ( icli_banner_motd_get(g_banner) != ICLI_RC_OK ) {
            T_E("fail to get motd banner\n");
            return VTSS_RC_ERROR;
        }
        if ( _banner_print(result, "motd") != VTSS_RC_OK ) {
            return VTSS_RC_ERROR;
        }

        /* banner exec */
        if ( icli_banner_exec_get(g_banner) != ICLI_RC_OK ) {
            T_E("fail to get exec banner\n");
            return VTSS_RC_ERROR;
        }
        if ( _banner_print(result, "exec") != VTSS_RC_OK ) {
            return VTSS_RC_ERROR;
        }

        /* banner login */
        if ( icli_banner_login_get(g_banner) != ICLI_RC_OK ) {
            T_E("fail to get login banner\n");
            return VTSS_RC_ERROR;
        }
        if ( _banner_print(result, "login") != VTSS_RC_OK ) {
            return VTSS_RC_ERROR;
        }

        /* enable password */
        for (priv = (icli_privilege_t)0; priv < (icli_privilege_t)(ICLI_PRIVILEGE_MAX - 1); ++priv) {
            if ( icli_enable_password_get(priv, g_password) == FALSE ) {
                T_E("fail to get enable password at priv %d\n", priv);
                return VTSS_RC_ERROR;
            }
            if ( _enable_password_print(result, priv) != VTSS_RC_OK ) {
                return VTSS_RC_ERROR;
            }
        }
        // highest privilege has default value, so process individually
        if ( icli_enable_password_get(priv, g_password) == FALSE ) {
            T_E("fail to get enable password at priv %d\n", priv);
            return VTSS_RC_ERROR;
        }
        // check if print
        b_print = FALSE;
        if ( req->all_defaults ) {
            b_print = TRUE;
        } else if ( icli_str_cmp(g_password, ICLI_DEFAULT_ENABLE_PASSWORD) != 0 ) {
            // different with default
            b_print = TRUE;
        }
        if ( b_print ) {
            if ( _enable_password_print(result, priv) != VTSS_RC_OK ) {
                return VTSS_RC_ERROR;
            }
        }

        /* hostname */
        if ( icli_dev_name_get(g_dev_name) != ICLI_RC_OK ) {
            T_E("fail to get device name\n");
            return VTSS_RC_ERROR;
        }
        // check if print
        b_print = FALSE;
        if ( req->all_defaults ) {
            b_print = TRUE;
        } else if ( icli_str_cmp(g_dev_name, ICLI_DEFAULT_DEVICE_NAME) != 0 ) {
            // different with default
            b_print = TRUE;
        }
        if ( b_print ) {
            if ( g_dev_name[0] ) {
                rc = vtss_icfg_printf(result, "hostname %s\n", g_dev_name);
            } else {
                rc = vtss_icfg_printf(result, "no hostname\n");
            }
            if ( rc != VTSS_RC_OK ) {
                T_E("fail to print to icfg\n");
                return VTSS_RC_ERROR;
            }
        }

        // Prompt
        // This is a little special, because we need to call a function to figure out
        // whether the current prompt is default or not (ssssh, between us: it has two defaults).
        prompt = icli_prompt_get();
        if ((is_default = icli_prompt_is_default(prompt))) {
            // The default prompt to show in all-defaults need not be the same
            // as held in #prompt.
            prompt = icli_prompt_default_get();
        }

        if (req->all_defaults || !is_default) {
            // Cater for the case where someone changes the default prompt to an empty string
            if (strlen(prompt)) {
                (void)vtss_icfg_printf(result, "prompt %s\n", prompt);
            } else {
                (void)vtss_icfg_printf(result, "no prompt\n");
            }
        }

        /* command privilege */
        if ( icli_priv_get_first(&cmd_conf) == ICLI_RC_OK ) {
            rc = vtss_icfg_printf(result, "privilege %s level %u %s\n", icli_mode_name_get(cmd_conf.mode), cmd_conf.privilege, cmd_conf.cmd);
            if ( rc != VTSS_RC_OK ) {
                T_E("fail to print to icfg\n");
                return VTSS_RC_ERROR;
            }
            while ( icli_priv_get_next(&cmd_conf) == ICLI_RC_OK ) {
                rc = vtss_icfg_printf(result, "privilege %s level %u %s\n", icli_mode_name_get(cmd_conf.mode), cmd_conf.privilege, cmd_conf.cmd);
                if ( rc != VTSS_RC_OK ) {
                    T_E("fail to print to icfg\n");
                    return VTSS_RC_ERROR;
                }
            }
        }

        break;

    case ICLI_CMD_MODE_CONFIG_LINE:
        /* get session data */
        g_session_data.session_id = req->instance_id.line;
        if ( icli_session_data_get(&g_session_data) != ICLI_RC_OK ) {
            T_E("fail to get session data of session %d.\n", g_session_data.session_id);
            return VTSS_RC_ERROR;
        }

        /* editing */
        // check if print
        if ( req->all_defaults ) {
            if ( g_session_data.input_style == ICLI_INPUT_STYLE_SINGLE_LINE ) {
                rc = vtss_icfg_printf(result, " editing\n");
                if ( rc != VTSS_RC_OK ) {
                    T_E("fail to print to icfg\n");
                    return VTSS_RC_ERROR;
                }
            } else {
                rc = vtss_icfg_printf(result, " no editing\n");
                if ( rc != VTSS_RC_OK ) {
                    T_E("fail to print to icfg\n");
                    return VTSS_RC_ERROR;
                }
            }
        } else if ( g_session_data.input_style != ICLI_INPUT_STYLE_SINGLE_LINE ) {
            // different with default
            rc = vtss_icfg_printf(result, " no editing\n");
            if ( rc != VTSS_RC_OK ) {
                T_E("fail to print to icfg\n");
                return VTSS_RC_ERROR;
            }
        }

        /* exec-banner */
        b_print = FALSE;
        if ( req->all_defaults ) {
            if ( g_session_data.b_exec_banner ) {
                rc = vtss_icfg_printf(result, " exec-banner\n");
                if ( rc != VTSS_RC_OK ) {
                    T_E("fail to print to icfg\n");
                    return VTSS_RC_ERROR;
                }
            } else {
                b_print = TRUE;
            }
        } else if ( g_session_data.b_exec_banner != TRUE ) {
            // different with default
            b_print = TRUE;
        }
        if ( b_print ) {
            rc = vtss_icfg_printf(result, " no exec-banner\n");
            if ( rc != VTSS_RC_OK ) {
                T_E("fail to print to icfg\n");
                return VTSS_RC_ERROR;
            }
        }

        /* exec-timeout */
        sec = (g_session_data.wait_time <= 0) ? 0 : g_session_data.wait_time;
        b_print = FALSE;
        if ( req->all_defaults ) {
            b_print = TRUE;
        } else if ( sec != ICLI_DEFAULT_WAIT_TIME ) {
            // different with default
            b_print = TRUE;
        }
        if ( b_print ) {
            rc = vtss_icfg_printf(result, " exec-timeout %d %d\n", sec / 60, sec % 60);
            if ( rc != VTSS_RC_OK ) {
                T_E("fail to print to icfg\n");
                return VTSS_RC_ERROR;
            }
        }

        /* history size */
        b_print = FALSE;
        if ( req->all_defaults ) {
            b_print = TRUE;
        } else if ( g_session_data.history_size != ICLI_HISTORY_CMD_CNT ) {
            // different with default
            b_print = TRUE;
        }
        if ( b_print ) {
            rc = vtss_icfg_printf(result, " history size %u\n", g_session_data.history_size);
            if ( rc != VTSS_RC_OK ) {
                T_E("fail to print to icfg\n");
                return VTSS_RC_ERROR;
            }
        }

        /* length */
        b_print = FALSE;
        if ( req->all_defaults ) {
            b_print = TRUE;
        } else if ( g_session_data.lines != ICLI_DEFAULT_LINES ) {
            // different with default
            b_print = TRUE;
        }
        if ( b_print ) {
            rc = vtss_icfg_printf(result, " length %u\n", g_session_data.lines);
            if ( rc != VTSS_RC_OK ) {
                T_E("fail to print to icfg\n");
                return VTSS_RC_ERROR;
            }
        }

        /* location */
        if ( g_session_data.location[0] ) {
            rc = vtss_icfg_printf(result, " location %s\n", g_session_data.location);
            if ( rc != VTSS_RC_OK ) {
                T_E("fail to print to icfg\n");
                return VTSS_RC_ERROR;
            }
        }

        /* motd-banner */
        b_print = FALSE;
        if ( req->all_defaults ) {
            if ( g_session_data.b_motd_banner ) {
                rc = vtss_icfg_printf(result, " motd-banner\n");
                if ( rc != VTSS_RC_OK ) {
                    T_E("fail to print to icfg\n");
                    return VTSS_RC_ERROR;
                }
            } else {
                b_print = TRUE;
            }
        } else if ( g_session_data.b_motd_banner != TRUE ) {
            // different with default
            b_print = TRUE;
        }
        if ( b_print ) {
            rc = vtss_icfg_printf(result, " no motd-banner\n");
            if ( rc != VTSS_RC_OK ) {
                T_E("fail to print to icfg\n");
                return VTSS_RC_ERROR;
            }
        }


        /* privilege level */
        b_print = FALSE;
        if ( req->all_defaults ) {
            b_print = TRUE;
        } else if ( g_session_data.privileged_level != ICLI_DEFAULT_PRIVILEGED_LEVEL ) {
            // different with default
            b_print = TRUE;
        }
        if ( b_print ) {
            rc = vtss_icfg_printf(result, " privilege level %d\n", g_session_data.privileged_level);
            if ( rc != VTSS_RC_OK ) {
                T_E("fail to print to icfg\n");
                return VTSS_RC_ERROR;
            }
        }

        /* width */
        b_print = FALSE;
        if ( req->all_defaults ) {
            b_print = TRUE;
        } else if ( g_session_data.width != ICLI_DEFAULT_WIDTH ) {
            // different with default
            b_print = TRUE;
        }
        if ( b_print ) {
            rc = vtss_icfg_printf(result, " width %u\n", g_session_data.width);
            if ( rc != VTSS_RC_OK ) {
                T_E("fail to print to icfg\n");
                return VTSS_RC_ERROR;
            }
        }
        break;

    default:
        /* no config in other modes */
        break;
    }
    return VTSS_RC_OK;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
mesa_rc icli_icfg_init(void)
{
    mesa_rc rc;

    /*
        Register Global config callback functions to ICFG module.
    */
    rc = vtss_icfg_query_register(VTSS_ICFG_GLOBAL_ICLI, "icli", _icli_icfg);
    if ( rc != VTSS_RC_OK ) {
        return rc;
    }

    /*
        Register Line config callback functions to ICFG module.
    */
    rc = vtss_icfg_query_register(VTSS_ICFG_LINE_ICLI, "icli", _icli_icfg);
    if ( rc != VTSS_RC_OK ) {
        return rc;
    }

    /* init mutex */
    critd_init(&g_mutex, "icli_icfg", VTSS_MODULE_ID_ICLI, CRITD_TYPE_MUTEX);

    return VTSS_RC_OK;
}
