/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "icli_api.h"
#include "vtss_icli.h"
#include "icli_console.h"
#include "icli_conf.h"
#include "icli_multiline.h"

#ifdef VTSS_SW_OPTION_ICFG
#include "icli_icfg.h"
#endif

#ifdef ICLI_STUB
#include "icli_stubs.h"
#endif

/*
==============================================================================

    Constant and Macro

==============================================================================
*/

/*
==============================================================================

    Type Definition

==============================================================================
*/
/*
    command register prototype
*/
BOOL icli_cmd_reg(void);

/*
==============================================================================

    Static Variable

==============================================================================
*/
/* Vitesse trace */
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "icli", "ICLI Engine"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

static char g_prompt[ICLI_PROMPT_UNINTERPRETED_MAX_LEN + 1];

/*
==============================================================================

    Static Function

==============================================================================
*/
/*
    get history command of the session

    INPUT
        session_id           : session ID
        history->history_pos : INDEX

    OUTPUT
        history_cmd : the history command of the session

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
static i32 _session_history_cmd_get(
    IN    icli_session_handle_t       *handle,
    INOUT icli_session_history_cmd_t  *history
)
{
    u32                     i;
    icli_history_cmd_t      *hcmd;

    if ( history == NULL ) {
        T_E("history == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    /* if history is empty */
    if ( vtss_icli_sutil_history_cmd_empty(handle) ) {
        return ICLI_RC_ERR_EMPTY;
    }

    /* check history_pos */
    if ( history->history_pos >= vtss_icli_sutil_history_cmd_cnt(handle) ) {
        return ICLI_RC_ERR_PARAMETER;
    }

    /* get that command */
    hcmd = handle->runtime_data.history_cmd_head->next;
    for ( i = 0; i < history->history_pos; i++ ) {
        ___NEXT( hcmd );
    }

    /* get history */
    (void)vtss_icli_str_cpy( history->history_cmd, hcmd->cmd );
    return ICLI_RC_OK;
}

/*
    execute a command on a session

    INPUT
        session              : session ID
        cmd                  : command to be executed
        b_exec               : TRUE  - execute the command function
                               FALSE - parse the command only to check if the command is legal or not,
                                       but not execute
        app_err_msg          : error message from application or config file name according to line_number
        line_number          : line number in the config file
                               if -1
                               then app_err_msg is application error message
                               else app_err_msg is config file name
        b_display_err_syntax : TRUE  - display error syntax
                               FALSE - not display

    OUTPUT
        exec_rc : result code returned by command callback after executing 'succesfully'

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
static i32 _session_cmd_exec(
    IN  icli_session_handle_t   *handle,
    IN  const char              *cmd,
    IN  BOOL                    b_exec,
    IN  const char              *app_err_msg,
    IN  i32                     line_number,
    IN  BOOL                    b_display_err_syntax,
    OUT i32                     *exec_rc
)
{
    icli_exec_type_t        orig_exec_type;
    i32                     rc;
    icli_parameter_t        *orig_cmd_var;
    u32                     cmd_len;

    /* session is alive or not */
    if ( handle->runtime_data.alive == FALSE ) {
        return ICLI_RC_ERR_PARAMETER;
    }

    /* skip space at beginning */
    ICLI_SPACE_SKIP(cmd, cmd);
    cmd_len = vtss_icli_str_len( cmd );

    /* pack cmd */
    // reset all cmd to be 0, otherwise, it will introduce issue for <line>
    memset(handle->runtime_data.cmd, 0, sizeof(handle->runtime_data.cmd));
    (void)vtss_icli_str_cpy(handle->runtime_data.cmd, cmd);
    handle->runtime_data.cmd_len   = cmd_len;
    handle->runtime_data.cmd_pos   = cmd_len;

    /* store original exec type */
    orig_exec_type = (icli_exec_type_t)(handle->runtime_data.exec_type);

    /* set corresponding exec type */
    if ( handle->runtime_data.b_in_parsing ) {
        handle->runtime_data.exec_type = ICLI_EXEC_TYPE_PARSING;
    } else {
        handle->runtime_data.exec_type = b_exec ? ICLI_EXEC_TYPE_CMD : ICLI_EXEC_TYPE_PARSING;
    }

    // store original command variables
    orig_cmd_var = handle->runtime_data.cmd_var;
    handle->runtime_data.cmd_var = NULL;

    // exec by API
    handle->runtime_data.b_exec_by_api = TRUE;

    // get application error message
    handle->runtime_data.app_err_msg = app_err_msg;
    handle->runtime_data.line_number = line_number;

    // display error syntax
    handle->runtime_data.err_display_mode = b_display_err_syntax ?
                                            ICLI_ERR_DISPLAY_MODE_PRINT : ICLI_ERR_DISPLAY_MODE_DROP;

    /* execute command */
    rc = vtss_icli_exec( handle );

    if ( exec_rc ) {
        *exec_rc = handle->runtime_data.exec_rc;
    }

    /* restore original exec type */
    handle->runtime_data.exec_type = orig_exec_type;

    // restore original command variables
    if ( orig_cmd_var ) {
        vtss_icli_exec_para_list_free( &(handle->runtime_data.cmd_var) );
        handle->runtime_data.cmd_var = orig_cmd_var;
    }

    // reset
    handle->runtime_data.b_exec_by_api        = FALSE;
    handle->runtime_data.app_err_msg          = NULL;
    handle->runtime_data.line_number          = 0;
    handle->runtime_data.err_display_mode     = ICLI_ERR_DISPLAY_MODE_PRINT;

    return rc;
}

extern "C" int icli_exec_icli_cmd_register();
extern "C" int icli_config_line_icli_cmd_register();
extern "C" int icli_debug_icli_cmd_register();
extern "C" int icli_config_icli_cmd_register();

/*
==============================================================================

    Public Function

==============================================================================
*/
#ifdef __cplusplus
extern "C"
#endif

/*
    initialize ICLI engine

    INPUT
        data : data for initialization

    OUTPUT
        n/a

    RETURN
        mesa_rc

    COMMENT
        n/a
*/
mesa_rc icli_init(vtss_init_data_t *data)
{
    icli_init_data_t init_data;
    BOOL             b;
#ifdef VTSS_SW_OPTION_ICFG
    mesa_rc          rc = VTSS_RC_OK;
#endif

    switch (data->cmd) {
    case INIT_CMD_EARLY_INIT:
        /* create semaphore */
        if (icli_os_init() == FALSE) {
            T_E("fail to initialize OS API's\n");
            return VTSS_RC_ERROR;
        }

        /* init engine */
        memset(&init_data, 0, sizeof(init_data));
        init_data.input_style    = ICLI_INPUT_STYLE_SINGLE_LINE;
        init_data.console_alive  = TRUE;
        init_data.case_sensitive = FALSE;

        if (vtss_icli_init(&init_data) != ICLI_RC_OK) {
            T_E("Failed to initialize ICLI engine");
            return VTSS_RC_ERROR;
        }

        /* port definition; start empty, fill in later */
        icli_port_range_reset();
        break;

    case INIT_CMD_INIT:
#ifdef VTSS_SW_OPTION_ICFG
        /* init I-configuration */
        rc = icli_icfg_init();
        if (rc != VTSS_RC_OK) {
            T_E("Failed to init icfg registration, rc = %s", error_txt(rc));
        }
#endif

#ifdef ICLI_STUB
        /* start stub server */
        if (icli_stubs_start() == FALSE) {
            T_E("Failed to start stub server");
            return VTSS_RC_ERROR;
        }

        T_D("ICLI Stub Server starts\n");
#endif

        icli_exec_icli_cmd_register();
        icli_config_line_icli_cmd_register();
        icli_debug_icli_cmd_register();
        icli_config_icli_cmd_register();
        break;

    case INIT_CMD_CONF_DEF:
        if (data->isid == VTSS_ISID_GLOBAL && !icli_conf_file_load(TRUE)) {
            T_E("Failed to load config");
            return VTSS_RC_ERROR;
        }

        if (VTSS_ISID_LEGAL(data->isid) && !data->switch_info[data->isid].configurable) {
            // A switch was deleted from the stack; we need to rebuild the port range
            _ICLI_SEMA_TAKE();
            b = icli_platform_port_range();
            _ICLI_SEMA_GIVE();

            if (!b) {
                T_E("Failed to build port range after reloading defaults");
                return VTSS_RC_ERROR;
            }
        }

        /* Clear all Telnet/SSH CLI sessions */
        icli_session_all_close(ICLI_SESSION_WAY_TELNET);
        icli_session_all_close(ICLI_SESSION_WAY_THREAD_TELNET);
        icli_session_all_close(ICLI_SESSION_WAY_SSH);
        icli_session_all_close(ICLI_SESSION_WAY_THREAD_SSH);
        break;

    case INIT_CMD_START:
#ifndef ICLI_STUB
        /* start console */
        if (!icli_console_start()) {
            T_E("Failed to start Console");
            return VTSS_RC_ERROR;
        }

        T_D("ICLI console started");
#endif

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        if (!icli_conf_file_load(FALSE)) {
            T_E("Failed to load config");
            return VTSS_RC_ERROR;
        }

    // Fall through

    case INIT_CMD_ICFG_LOADING_POST:
        _ICLI_SEMA_TAKE();
        b = icli_platform_port_range();
        _ICLI_SEMA_GIVE();

        if (!b) {
            T_E("Cmd = %d: Failed to build port range", data->cmd);
            return VTSS_RC_ERROR;
        }

        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

/*
    get text string for each icli_rc_t

    INPUT
        rc : icli_rc_t

    OUTPUT
        n/a

    RETURN
        text string

    COMMENT
        n/a
*/
const char *icli_error_txt(
    IN  mesa_rc   rc
)
{
    if ( rc ) {}
    return "";
}

/*
    get ICLI engine config data

    INPUT
        n/a

    OUTPUT
        conf - config data to get

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_conf_get(
    OUT icli_conf_data_t    *conf
)
{
    if ( conf == NULL ) {
        T_E("conf == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    memcpy(conf, vtss_icli_conf_get(), sizeof(icli_conf_data_t));

    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set config data to ICLI engine

    INPUT
        conf - config data to apply

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_conf_set(
    IN  icli_conf_data_t     *conf
)
{
    if ( conf == NULL ) {
        T_E("conf == NULL\n");
        return FALSE;
    }

    _ICLI_SEMA_TAKE();

    vtss_icli_conf_set( conf );

    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    reset config data to default

    INPUT
        n/a

    OUTPUT
        conf - config data to reset default

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_conf_default(
    void
)
{
    icli_rc_t   rc;

    _ICLI_SEMA_TAKE();

    g_prompt[0] = '\0';

    rc = (icli_rc_t)vtss_icli_conf_default();

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    get whick input_style the ICLI engine perform

    input -
        n/a

    output -
        input_style: current input style

    return -
        icli_rc_t

    comment -
        n/a
*/
i32 icli_input_style_get(
    OUT icli_input_style_t  *input_style
)
{
    if ( input_style == NULL ) {
        T_E("input_style == NULL\n");
        return ICLI_RC_ERROR;
    }

    _ICLI_SEMA_TAKE();

    *input_style = vtss_icli_input_style_get();

    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set whick input_style the ICLI engine perform

    input -
        input_style : input style

    output -
        n/a

    return -
        icli_rc_t

    comment -
        n/a
*/
i32 icli_input_style_set(
    IN icli_input_style_t   input_style
)
{
    icli_rc_t   rc;

    _ICLI_SEMA_TAKE();

    rc = (icli_rc_t)vtss_icli_input_style_set( input_style );

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    register ICLI command

    INPUT
        cmd_register : command data

    OUTPUT
        n/a

    RETURN
        >= 0 : command ID
        < 0  : icli_rc_t

    COMMENT
        n/a
*/
i32 icli_cmd_register(
    IN  icli_cmd_register_t     *cmd_register
)
{
    i32     cmd_id;

    _ICLI_SEMA_TAKE();
    cmd_id = vtss_icli_register_cmd( cmd_register );
    _ICLI_SEMA_GIVE();
    return cmd_id;
}

/*
    get if the command is enabled or disabled

    INPUT
        cmd_id : command ID

    OUTPUT
        enable : TRUE  - the command is enabled
                 FALSE - the command is disabled

    RETURN
        ICLI_RC_OK : get successfully
        ICLI_RC_ERR_PARAMETER : fail to get

    COMMENT
        n/a
*/
i32 icli_cmd_is_enable(
    IN  u32     cmd_id,
    OUT BOOL    *enable
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();
    rc = vtss_icli_register_cmd_is_enable( cmd_id, enable );
    _ICLI_SEMA_GIVE();
    return rc;
}

/*
    enable the command

    INPUT
        cmd_id : command ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_cmd_enable(
    IN  u32     cmd_id
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();
    rc = vtss_icli_register_cmd_enable( cmd_id );
    _ICLI_SEMA_GIVE();
    return rc;
}

/*
    disable the command

    INPUT
        cmd_id : command ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_cmd_disable(
    IN  u32     cmd_id
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();
    rc = vtss_icli_register_cmd_disable( cmd_id );
    _ICLI_SEMA_GIVE();
    return rc;
}

/*
    get if the command is visible or invisible

    INPUT
        cmd_id : command ID

    OUTPUT
        visible : TRUE  - the command is visible
                  FALSE - the command is invisible

    RETURN
        ICLI_RC_OK : get successfully
        ICLI_RC_ERR_PARAMETER : fail to get

    COMMENT
        n/a
*/
i32 icli_cmd_is_visible(
    IN  u32     cmd_id,
    OUT BOOL    *visible
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();
    rc = vtss_icli_register_cmd_is_visible( cmd_id, visible );
    _ICLI_SEMA_GIVE();
    return rc;
}

/*
    make the command visible to user

    INPUT
        cmd_id : command ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_cmd_visible(
    IN  u32     cmd_id
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();
    rc = vtss_icli_register_cmd_visible( cmd_id );
    _ICLI_SEMA_GIVE();
    return rc;
}

/*
    make the command invisible to user

    INPUT
        cmd_id : command ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_cmd_invisible(
    IN  u32     cmd_id
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();
    rc = vtss_icli_register_cmd_invisible( cmd_id );
    _ICLI_SEMA_GIVE();
    return rc;
}

/*
    get privilege of a command

    INPUT
        cmd_id : command ID

    OUTPUT
        privilege : session privilege

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_cmd_privilege_get(
    IN  u32     cmd_id,
    OUT u32     *privilege
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();
    rc = vtss_icli_register_cmd_privilege_get( cmd_id, privilege );
    _ICLI_SEMA_GIVE();
    return rc;
}

/*
    set privilege of a command

    INPUT
        cmd_id    : command ID
        privilege : session privilege

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_cmd_privilege_set(
    IN u32     cmd_id,
    IN u32     privilege
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();
    rc = vtss_icli_register_cmd_privilege_set( cmd_id, privilege );
    _ICLI_SEMA_GIVE();
    return rc;
}

/*
    ICLI engine for each session

    This API can be called only by the way of ICLI_SESSION_WAY_THREAD_CONSOLE,
    ICLI_SESSION_WAY_THREAD_TELNET, ICLI_SESSION_WAY_THREAD_SSH.

    The engine handles user input, execute command and display command output.
    User input includes input style, TAB key, ? key, comment, etc.
    And, it works for one command only. So you need to call it in a loop for
    the continuous service. For example,
        while( 1 ) {
            vtss_icli_session_engine( session_id );
        }

    INPUT
        session_id : session ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_engine(
    IN i32      session_id
)
{
    u32     i;

    _ICLI_SEMA_TAKE();
    i = vtss_icli_session_engine( session_id );

    _ICLI_SEMA_GIVE();

    return i;
}

/*
    open a ICLI session

    INPUT
        open_data : data for session open

    OUTPUT
        session_id : ID of session opened successfully

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_open(
    IN  icli_session_open_data_t    *open_data,
    OUT u32                         *session_id
)
{
    i32     rc;
    if ( open_data == NULL ) {
        T_E("open_data == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( session_id == NULL ) {
        T_E("session_id == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    switch ( open_data->way ) {
    case ICLI_SESSION_WAY_TELNET:
    case ICLI_SESSION_WAY_SSH:
    case ICLI_SESSION_WAY_CONSOLE:
    case ICLI_SESSION_WAY_THREAD_CONSOLE:
    case ICLI_SESSION_WAY_THREAD_TELNET:
    case ICLI_SESSION_WAY_THREAD_SSH:
        if ( open_data->char_get == NULL ) {
            T_E("open_data->char_get == NULL\n");
            return ICLI_RC_ERR_PARAMETER;
        }
#if 1 /* CP, 2013/04/15 10:51, APP to take output */
        if ( open_data->char_put == NULL && open_data->str_put == NULL ) {
            T_E("not output callback\n");
            return ICLI_RC_ERR_PARAMETER;
        }
#else
        if ( open_data->str_put == NULL ) {
            T_E("open_data->str_put == NULL\n");
            return ICLI_RC_ERR_PARAMETER;
        }
#endif
        break;

    case ICLI_SESSION_WAY_APP_EXEC:
#if 1 /* CP, 05/07/2013 19:05, Bugzilla#11725 - ICLI_PRINTF() to application output callback */
        if ( open_data->char_put || open_data->str_put ) {
            open_data->b_app_output = TRUE;
        } else {
            open_data->b_app_output = FALSE;
        }
#endif
        break;

    default:
        T_E("invalid session type = %d\n", open_data->way);
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();
    rc = vtss_icli_session_open( open_data, session_id );
    _ICLI_SEMA_GIVE();
    return rc;
}

i32 icli_session_begin(
    IN icli_session_open_data_t   *open_data,
    IN u32                         session_id
)
{
    i32     rc = VTSS_RC_OK;
    BOOL    b;

    if ( open_data == NULL ) {
        T_E("open_data == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( session_id < 0) {
        T_E("session_id < 0\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    b = TRUE;
    switch ( open_data->way ) {
    case ICLI_SESSION_WAY_CONSOLE:
        b = icli_os_thread_create( session_id,
                                   open_data->name,
                                   ICLI_THREAD_PRIORITY_HIGH,
                                   icli_session_engine,
                                   session_id );
        break;

    case ICLI_SESSION_WAY_TELNET:
    case ICLI_SESSION_WAY_SSH:
        b = icli_os_thread_create( session_id,
                                   open_data->name,
                                   ICLI_THREAD_PRIORITY_NORMAL,
                                   icli_session_engine,
                                   session_id );
        break;

    case ICLI_SESSION_WAY_APP_EXEC:
    case ICLI_SESSION_WAY_THREAD_CONSOLE:
    case ICLI_SESSION_WAY_THREAD_TELNET:
    case ICLI_SESSION_WAY_THREAD_SSH:
    default:
        break;
    }

    if ( b == FALSE ) {
        rc = ICLI_RC_ERROR;
    }

    _ICLI_SEMA_GIVE();
    return rc;
}

/*
    close a ICLI session

    INPUT
        session_id : session ID from icli_session_open()

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_close(
    IN u32  session_id
)
{
    i32    rc;

    _ICLI_SEMA_TAKE();
    rc = vtss_icli_session_close( session_id );
    _ICLI_SEMA_GIVE();
    return rc;
}

/*
    close ICLI sessions with the user name

    INPUT
        username : user name to clise session

    OUTPUT
        n/a

    RETURN
        n/a

    COMMENT
        n/a
*/
void icli_session_close_by_username(
    IN char     *username
)
{
    _ICLI_SEMA_TAKE();

    vtss_icli_session_close_by_username( username );

    _ICLI_SEMA_GIVE();
}

/*
    close all ICLI sessions

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        n/a

    COMMENT
        n/a
*/
void icli_session_all_close(icli_session_way_t way)
{
    u32     session_id;

    _ICLI_SEMA_TAKE();

    for ( session_id = 0; session_id < ICLI_SESSION_CNT; ++session_id ) {
        icli_session_handle_t *handle;

        if (!vtss_icli_session_alive(session_id) ||
            (handle = vtss_icli_session_handle_get(session_id)) == NULL ||
            handle->open_data.way != way) {
            continue;
        }

        (void)vtss_icli_session_close(session_id);
    }

    _ICLI_SEMA_GIVE();
}

/*
    get max number of sessions

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        max number of sessions

    COMMENT
        n/a
*/
u32 icli_session_max_get(void)
{
    u32     i;

    _ICLI_SEMA_TAKE();

    i = vtss_icli_session_max_cnt_get();

    _ICLI_SEMA_GIVE();

    return i;
}

/*
    set max number of sessions

    INPUT
        max_sessions : max number of sessions

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_max_set(
    IN u32 max_sessions
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    rc = vtss_icli_session_max_cnt_set( max_sessions );

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    execute or parse a command on a session

    If parsing commands
    before parsing, begin the transaction by ICLI_CMD_PARSING_BEGIN()
    and end the transaction after finishing the parsing by ICLI_CMD_PARSING_END()
    the flow is as follows.

        ICLI_CMD_PARSING_BEGIN();
        ICLI_CMD_EXEC(..., FALSE);
        ...
        ICLI_CMD_EXEC(..., FALSE);
        ICLI_CMD_PARSING_END();

    INPUT
        session : session ID
        cmd     : command to be executed
        b_exec  : TRUE  - execute the command function
                  FALSE - parse the command only to check if the command is legal or not,
                          but not execute

    OUTPUT
        exec_rc : result code returned by command callback after executing 'succesfully'
                  that is return ICLI_RC_OK.

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_cmd_exec_rc(
    IN  u32           session_id,
    IN  const char    *cmd,
    IN  BOOL          b_exec,
    OUT i32           *exec_rc
)
{
    icli_session_handle_t   *handle;
    i32                     rc;
    u32                     cmd_len;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %u\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( cmd == NULL ) {
        T_E("cmd == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    cmd_len = vtss_icli_str_len( cmd );

    // empty command
    if ( cmd_len == 0 ) {
        return ICLI_RC_OK;
    }

    // cmd is too long
    if ( cmd_len > ICLI_STR_MAX_LEN ) {
        T_E("cmd length %d is too long, must less than %d\n", cmd_len, ICLI_STR_MAX_LEN);
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    /* get handle */
    handle = vtss_icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = _session_cmd_exec( handle, cmd, b_exec, NULL, ICLI_INVALID_LINE_NUMBER, TRUE, exec_rc );

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    execute or parse a command on a session

    If parsing commands
    before parsing, begin the transaction by ICLI_CMD_PARSING_BEGIN()
    and end the transaction after finishing the parsing by ICLI_CMD_PARSING_END()
    the flow is as follows.

        ICLI_CMD_PARSING_BEGIN();
        ICLI_CMD_EXEC(..., FALSE);
        ...
        ICLI_CMD_EXEC(..., FALSE);
        ICLI_CMD_PARSING_END();

    INPUT
        session : session ID
        cmd     : command to be executed
        b_exec  : TRUE  - execute the command function
                  FALSE - parse the command only to check if the command is legal or not,
                          but not execute

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_cmd_exec(
    IN  u32           session_id,
    IN  const char    *cmd,
    IN  BOOL          b_exec
)
{
    return icli_session_cmd_exec_rc( session_id,
                                     cmd,
                                     b_exec,
                                     NULL );
}

/*
    execute or parsing a command on a session and display error message from application

    If parsing commands
    before parsing, begin the transaction by ICLI_CMD_PARSING_BEGIN()
    and end the transaction after finishing the parsing by ICLI_CMD_PARSING_END()
    the flow is as follows.

        ICLI_CMD_PARSING_BEGIN();
        ICLI_CMD_EXEC_ERR_DISPLAY(..., FALSE, ...);
        ...
        ICLI_CMD_EXEC_ERR_DISPLAY(..., FALSE, ...);
        ICLI_CMD_PARSING_END();

    INPUT
        session              : session ID
        cmd                  : command to be executed
        b_exec               : TRUE  - execute the command function
                               FALSE - parse the command only to
                                       check if the command is legal syntax,
                                       but not execute
        app_err_msg          : error message from application
        b_display_err_syntax : TRUE  - display error syntax
                               FALSE - not display

    OUTPUT
        exec_rc : result code returned by command callback after executing 'succesfully'
                  that is return ICLI_RC_OK.

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_cmd_exec_err_display_rc(
    IN  u32           session_id,
    IN  const char    *cmd,
    IN  BOOL          b_exec,
    IN  const char    *app_err_msg,
    IN  BOOL          b_display_err_syntax,
    OUT i32           *exec_rc
)
{
    icli_session_handle_t   *handle;
    i32                     rc;
    u32                     cmd_len;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %u\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( cmd == NULL ) {
        T_E("cmd == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    cmd_len = vtss_icli_str_len( cmd );

    // empty command
    if ( cmd_len == 0 ) {
        return ICLI_RC_OK;
    }

    // cmd is too long
    if ( cmd_len > ICLI_STR_MAX_LEN ) {
        T_E("cmd length %d is too long, must less than %d\n", cmd_len, ICLI_STR_MAX_LEN);
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    /* get handle */
    handle = vtss_icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = _session_cmd_exec( handle, cmd, b_exec, app_err_msg, ICLI_INVALID_LINE_NUMBER, b_display_err_syntax, exec_rc );

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    execute or parsing a command on a session and display error message from application

    If parsing commands
    before parsing, begin the transaction by ICLI_CMD_PARSING_BEGIN()
    and end the transaction after finishing the parsing by ICLI_CMD_PARSING_END()
    the flow is as follows.

        ICLI_CMD_PARSING_BEGIN();
        ICLI_CMD_EXEC_ERR_DISPLAY(..., FALSE, ...);
        ...
        ICLI_CMD_EXEC_ERR_DISPLAY(..., FALSE, ...);
        ICLI_CMD_PARSING_END();

    INPUT
        session              : session ID
        cmd                  : command to be executed
        b_exec               : TRUE  - execute the command function
                               FALSE - parse the command only to
                                       check if the command is legal syntax,
                                       but not execute
        app_err_msg          : error message from application
        b_display_err_syntax : TRUE  - display error syntax
                               FALSE - not display

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_cmd_exec_err_display(
    IN  u32           session_id,
    IN  const char    *cmd,
    IN  BOOL          b_exec,
    IN  const char    *app_err_msg,
    IN  BOOL          b_display_err_syntax
)
{
    return icli_session_cmd_exec_err_display_rc( session_id,
                                                 cmd,
                                                 b_exec,
                                                 app_err_msg,
                                                 b_display_err_syntax,
                                                 NULL );
}

/*
    execute or parsing a command on a session and display error message from application

    If parsing commands
    before parsing, begin the transaction by ICLI_CMD_PARSING_BEGIN()
    and end the transaction after finishing the parsing by ICLI_CMD_PARSING_END()
    the flow is as follows.

        ICLI_CMD_PARSING_BEGIN();
        ICLI_CMD_EXEC_ERR_DISPLAY(..., FALSE, ...);
        ...
        ICLI_CMD_EXEC_ERR_DISPLAY(..., FALSE, ...);
        ICLI_CMD_PARSING_END();

    INPUT
        session_id           : session ID
        cmd                  : command to be executed
        b_exec               : TRUE  - execute the command function
                               FALSE - parse the command only to
                                       check if the command is legal syntax,
                                       but not execute
        filename             : config file name of the command
        line_number          : the line number in the config file for the command
        b_display_err_syntax : TRUE  - display error syntax
                               FALSE - not display

    OUTPUT
        exec_rc : result code returned by command callback after executing 'succesfully'
                  that is return ICLI_RC_OK.

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_cmd_exec_err_file_line_rc(
    IN  u32     session_id,
    IN  char    *cmd,
    IN  BOOL    b_exec,
    IN  char    *filename,
    IN  u32     line_number,
    IN  BOOL    b_display_err_syntax,
    OUT i32     *exec_rc
)
{
    icli_session_handle_t   *handle;
    i32                     rc;
    u32                     cmd_len;
    icli_cmd_mode_t         mode;

    T_D("Entry: Session %d, %s:%d: %s", session_id, filename, line_number, cmd);

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %u\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( cmd == NULL ) {
        T_E("cmd == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    cmd_len = vtss_icli_str_len( cmd );

    // cmd is too long
    if ( cmd_len > ICLI_STR_MAX_LEN ) {
        T_E("cmd length %d is too long, must less than %d\n", cmd_len, ICLI_STR_MAX_LEN);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( line_number > ICLI_MAX_INT ) {
        T_E("invalid line number %u\n", line_number);
        return ICLI_RC_ERR_PARAMETER;
    }

    /* get handle */
    handle = vtss_icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        T_D("That didn't work out");
        return ICLI_RC_ERR_PARAMETER;
    }

    if (icli_session_mode_get(session_id, &mode) != ICLI_RC_OK) {
        return VTSS_RC_ERROR;
    }

    if (mode == ICLI_CMD_MODE_MULTILINE) {
        rc = icli_multiline_process_line(session_id, cmd);
        if (exec_rc) {
            *exec_rc = rc;
        }
    } else {
        const char *p = cmd;
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        // empty command, ICLI doesn't like it
        if ( *p == 0 ) {
            if (exec_rc) {
                *exec_rc = ICLI_RC_OK;
            }
            return ICLI_RC_OK;
        }
        if (b_exec || handle->runtime_data.current_parsing_mode != ICLI_CMD_MODE_MULTILINE) {
            if ((*p++ == 'e') && (*p++ == 'n') && (*p++ == 'd') &&
                (*p == '\0' || *p == '\n' || *p == '\r' || *p == ' ' || *p == '\t')) {
                if (exec_rc) {
                    *exec_rc = ICLI_RC_OK;
                }
                return ICLI_RC_ERR_PARSING_END;
            }
        }
        _ICLI_SEMA_TAKE();
        rc = _session_cmd_exec( handle, cmd, b_exec, filename, (i32)line_number, b_display_err_syntax, exec_rc );
        _ICLI_SEMA_GIVE();
    }

    T_D("All done, rc = %d", rc);
    return rc;
}

/*
    execute or parsing a command on a session and display error message from application

    If parsing commands
    before parsing, begin the transaction by ICLI_CMD_PARSING_BEGIN()
    and end the transaction after finishing the parsing by ICLI_CMD_PARSING_END()
    the flow is as follows.

        ICLI_CMD_PARSING_BEGIN();
        ICLI_CMD_EXEC_ERR_DISPLAY(..., FALSE, ...);
        ...
        ICLI_CMD_EXEC_ERR_DISPLAY(..., FALSE, ...);
        ICLI_CMD_PARSING_END();

    INPUT
        session_id           : session ID
        cmd                  : command to be executed
        b_exec               : TRUE  - execute the command function
                               FALSE - parse the command only to
                                       check if the command is legal syntax,
                                       but not execute
        filename             : config file name of the command
        line_number          : the line number in the config file for the command
        b_display_err_syntax : TRUE  - display error syntax
                               FALSE - not display

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_cmd_exec_err_file_line(
    IN  u32     session_id,
    IN  char    *cmd,
    IN  BOOL    b_exec,
    IN  char    *filename,
    IN  u32     line_number,
    IN  BOOL    b_display_err_syntax
)
{
    return icli_session_cmd_exec_err_file_line_rc( session_id,
                                                   cmd,
                                                   b_exec,
                                                   filename,
                                                   line_number,
                                                   b_display_err_syntax,
                                                   NULL );
}

/*
    begin transaction to parse a batch of commands on a session

    INPUT
        session : session ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_cmd_parsing_begin(
    IN u32      session_id
)
{
    icli_session_handle_t   *handle;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %u\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    /* get handle */
    handle = vtss_icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->runtime_data.b_in_parsing = TRUE;
    handle->runtime_data.current_parsing_mode =
        handle->runtime_data.mode_para[handle->runtime_data.mode_level].mode;

    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    end transaction to parse commands on a session

    INPUT
        session : session ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_cmd_parsing_end(
    IN u32      session_id
)
{
    icli_session_handle_t   *handle;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %u\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    /* get handle */
    handle = vtss_icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->runtime_data.b_in_parsing = FALSE;

    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    output formated string to a specific session
    the maximum length to print is (ICLI_STR_MAX_LEN + 64),
    where ICLI_STR_MAX_LEN depends on project.

    * if the length is > (ICLI_STR_MAX_LEN + 64),
      then please use icli_session_self_printf_lstr()

    INPUT
        session_id : the session ID
        format     : string format
        ...        : parameters of format

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_printf(
    IN  u32         session_id,
    IN  const char  *format,
    IN  ...
)
{
    va_list                 arglist;
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    if ( vtss_icli_session_alive(session_id) == FALSE ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    handle = vtss_icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("session %d handle is NULL\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    switch ( handle->open_data.way ) {
    case ICLI_SESSION_WAY_CONSOLE:
    case ICLI_SESSION_WAY_TELNET:
    case ICLI_SESSION_WAY_SSH:
    case ICLI_SESSION_WAY_THREAD_CONSOLE:
    case ICLI_SESSION_WAY_THREAD_TELNET:
    case ICLI_SESSION_WAY_THREAD_SSH:
        if ( handle->runtime_data.line_mode == ICLI_LINE_MODE_BYPASS ) {
            _ICLI_SEMA_GIVE();
            return ICLI_RC_ERR_BYPASS;
        }
        break;

    case ICLI_SESSION_WAY_APP_EXEC:
#if 1 /* CP, 05/07/2013 19:05, Bugzilla#11725 - ICLI_PRINTF() to application output callback */
        if ( handle->open_data.b_app_output ) {
            break;
        }
#endif
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_BYPASS;

    default:
        _ICLI_SEMA_GIVE();
        T_E("invalid session %d type %d\n", session_id, handle->open_data.way);
        return ICLI_RC_ERR_PARAMETER;
    }

    //format string buffer
    /*lint -e{530} ... 'arglist' is initialized by va_start() */
    va_start( arglist, format );

    (void)vtss_icli_session_va_str_put(session_id, format, arglist);

    va_end( arglist );

    _ICLI_SEMA_GIVE();
    return ICLI_RC_OK;
}

/*
    output long string to a specific session
    the length of long string is larger than (ICLI_STR_MAX_LEN),
    where ICLI_STR_MAX_LEN depends on project

    INPUT
        session_id : the session ID
        lstr       : long string to print
                     *** str must be a char array or dynamic allocated memory(write-able),
                         and must not a const string(read_only).

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_printf_lstr(
    IN  u32         session_id,
    IN  const char  *lstr
)
{
    if (lstr == NULL) {
        return ICLI_RC_OK;
    }

    size_t total_len = strlen(lstr);
    const char *remain_str = lstr;
    constexpr int bulk_output_len = ICLI_STR_MAX_LEN - 1;
    i32  rc = ICLI_RC_OK;

    while (total_len) {
        if (total_len >= bulk_output_len) {
            // Bulk output
            rc = icli_session_printf(session_id, "%.*s", bulk_output_len, remain_str);
            if (rc != ICLI_RC_OK) {
                return rc;
            }

            // Next bulk string
            remain_str += bulk_output_len;
            total_len -= bulk_output_len;
        } else {
            // Output the remaining string
            rc = icli_session_printf(session_id, "%s", remain_str);
            break; // End of the loop
        }
    }

    return rc;
}

/*
    output formated string to all sessions
    this does not check ICLI_LINE_MODE_BYPASS
    because this is used for system message

    INPUT
        format     : string format
        ...        : parameters of format

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_printf_to_all(
    IN  const char  *format,
    IN  ...
)
{
    u32                     session_id;
    va_list                 arglist;
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    /* find the first alive session */
    for ( session_id = 0; session_id < ICLI_SESSION_CNT; ++session_id ) {
        if ( ! vtss_icli_session_alive(session_id) ) {
            continue;
        }
        handle = vtss_icli_session_handle_get( session_id );
        if ( handle == NULL ) {
            _ICLI_SEMA_GIVE();
            T_E("session %d handle is NULL\n", session_id);
            return ICLI_RC_ERR_PARAMETER;
        }

        switch ( handle->open_data.way ) {
        case ICLI_SESSION_WAY_CONSOLE:
        case ICLI_SESSION_WAY_TELNET:
        case ICLI_SESSION_WAY_SSH:
        case ICLI_SESSION_WAY_THREAD_CONSOLE:
        case ICLI_SESSION_WAY_THREAD_TELNET:
        case ICLI_SESSION_WAY_THREAD_SSH:
            break;

        case ICLI_SESSION_WAY_APP_EXEC:
#if 1 /* CP, 05/07/2013 19:05, Bugzilla#11725 - ICLI_PRINTF() to application output callback */
            if ( handle->open_data.b_app_output ) {
                break;
            }
#endif
            continue;

        default:
            _ICLI_SEMA_GIVE();
            T_E("invalid session %u type %d\n", session_id, handle->open_data.way);
            return ICLI_RC_ERR_PARAMETER;
        }
        break;
    }

    /* no session alive */
    if ( session_id == ICLI_SESSION_CNT ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_OK;
    }

    /* format string from parameters */
    /*lint -e{530} ... 'arglist' is initialized by va_start() */
    va_start( arglist, format );

    (void)vtss_icli_session_va_str_put(session_id, format, arglist);

    va_end( arglist );

    /* print out to following alive sessions */
    for ( session_id++; session_id < ICLI_SESSION_CNT; ++session_id ) {
        if ( ! vtss_icli_session_alive(session_id) ) {
            continue;
        }
        (void)vtss_icli_session_va_str_put(session_id, format, arglist);
    }

    _ICLI_SEMA_GIVE();
    return ICLI_RC_OK;
}

/*
    get string from usr input

    INPUT
        session_id : the session ID
        type       : input type
                     NORMAL   - echo the input char
                     PASSWORD - echo '*'
        str_size   : the buffer size of str

    OUTPUT
        str        : user input string
        str_size   : the length of user input
        end_key    : the key to terminate the input,
                     Enter, New line, or Ctrl-C

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_usr_str_get(
    IN      u32                     session_id,
    IN      icli_usr_input_type_t   input_type,
    OUT     char                    *str,
    INOUT   i32                     *str_size,
    OUT     i32                     *end_key
)
{
    i32                     rc;
    icli_line_mode_t        old_line_mode;
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( vtss_icli_session_alive(session_id) == FALSE ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    /* get original line mode */
    old_line_mode = (icli_line_mode_t)(handle->runtime_data.line_mode);

    /* set FLOOD mode */
    handle->runtime_data.line_mode = ICLI_LINE_MODE_FLOOD;

    /* get user string */
    rc = vtss_icli_session_usr_str_get( session_id, input_type, str, str_size, end_key );

    /* set back to the original mode */
    handle->runtime_data.line_mode = old_line_mode;

    _ICLI_SEMA_GIVE();
    return rc;
}

/*
    get Ctrl-C from user

    INPUT
        session_id : the session ID
        wait_time  : time to wait in milli-seconds
                     must be 2147483647 >= wait_time > 0

    OUTPUT
        n/a

    RETURN
        ICLI_RC_OK            : yes, the user press Ctrl-C
        ICLI_RC_ERR_EXPIRED   : time expired
        ICLI_RC_ERR_PARAMETER : input paramter error

    COMMENT
        n/a
*/
i32 icli_session_ctrl_c_get(
    IN  u32     session_id,
    IN  u32     wait_time
)
{
    i32     rc;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( wait_time == 0 || wait_time > ICLI_MAX_INT) {
        T_E("invalid wait_time = %u\n", wait_time);
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    if ( vtss_icli_session_alive(session_id) == FALSE ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = vtss_icli_session_ctrl_c_get( session_id, wait_time );

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    get current command mode of the session

    INPUT
        session_id : the session ID

    OUTPUT
        mode : current command mode

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_mode_get(
    IN  u32                 session_id,
    OUT icli_cmd_mode_t     *mode
)
{
    i32     rc;

    if ( mode == NULL ) {
        T_E("mode == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    if ( vtss_icli_session_alive(session_id) == FALSE ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = vtss_icli_session_mode_get( session_id, mode );

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    make session entering the mode

    INPUT
        session_id : the session ID
        mode       : mode entering

    OUTPUT
        n/a

    RETURN
        >= 0 : successful and the return value is mode level
        -1   : failed

    COMMENT
        n/a
*/
i32 icli_session_mode_enter(
    IN  u32                 session_id,
    IN  icli_cmd_mode_t     mode
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    if ( vtss_icli_session_alive(session_id) == FALSE ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = vtss_icli_session_mode_enter( session_id, mode );

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    make session exit the top mode

    INPUT
        session_id : the session ID

    OUTPUT
        n/a

    RETURN
        >= 0 : successful and the return value is mode level
        -1   : failed

    COMMENT
        n/a
*/
i32 icli_session_mode_exit(
    IN  u32     session_id
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    if ( vtss_icli_session_alive(session_id) == FALSE ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = vtss_icli_session_mode_exit( session_id );

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    get configuration data of the session

    INPUT
        session_id : the session ID, INDEX

    OUTPUT
        data: data of the session

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_data_get(
    INOUT icli_session_data_t   *data
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    rc = vtss_icli_session_data_get( data );

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    get configuration data of the next session

    INPUT
        session_id : the next session of the session ID, INDEX

    OUTPUT
        data : data of the next session

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_data_get_next(
    INOUT icli_session_data_t   *data
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    rc = vtss_icli_session_data_get_next( data );

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    set history size of a session

    INPUT
        session_id   : session ID

    OUTPUT
        history_size : the size of history commands
                       0 means to disable history function

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_history_size_get(
    IN  u32     session_id,
    OUT u32     *history_size
)
{
    icli_session_handle_t   *handle;

    if ( history_size == NULL ) {
        T_E("invalid history_size = NULL, session_id = %u\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    *history_size = ICLI_HISTORY_MAX_CNT;

    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set history size of a session

    INPUT
        session_id   : session ID
        history_size : the size of history commands
                       0 means to disable history function

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_history_size_set(
    IN  u32     session_id,
    IN  u32     history_size
)
{
    icli_session_handle_t   *handle;
    u32                     n;

    if ( history_size > ICLI_HISTORY_CMD_CNT ) {
        T_E("invalid history_size = %u, session_id = %u\n", history_size, session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( ICLI_HISTORY_MAX_CNT == history_size ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_OK;
    }

    n = 0;
    if ( history_size == 0 ) {
        // free all
        n = vtss_icli_sutil_history_cmd_cnt( handle );
    } else if ( vtss_icli_sutil_history_cmd_cnt(handle) > history_size ) {
        // keep the recent histories
        n = vtss_icli_sutil_history_cmd_cnt( handle ) - history_size;
    }
    vtss_icli_sutil_history_cmd_free_n( handle, n );

    /* update history size */
    ICLI_HISTORY_MAX_CNT = history_size;

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return ICLI_RC_OK;
}

/*
    get the first history command of a session

    INPUT
        session_id  : session ID

    OUTPUT
        history_cmd : the first history command of the session

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_history_cmd_get_first(
    IN  u32                         session_id,
    OUT icli_session_history_cmd_t  *history
)
{
    icli_session_handle_t   *handle;
    i32                     rc;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( history == NULL ) {
        T_E("history == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    /* session alive */
    if ( vtss_icli_session_alive(session_id) == FALSE ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    handle = vtss_icli_session_handle_get(session_id);

    /* get first history */
    history->history_pos = 0;
    rc = _session_history_cmd_get(handle, history);

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    get history command of the session

    INPUT
        session_id           : session ID
        history->history_pos : INDEX

    OUTPUT
        history_cmd : the history command of the session

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_history_cmd_get(
    IN    u32                         session_id,
    INOUT icli_session_history_cmd_t  *history
)
{
    icli_session_handle_t   *handle;
    i32                     rc;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( history == NULL ) {
        T_E("history == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    /* session alive */
    if ( vtss_icli_session_alive(session_id) == FALSE ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    handle = vtss_icli_session_handle_get(session_id);

    /* get history */
    rc = _session_history_cmd_get(handle, history);

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    get the next history command of the session

    INPUT
        session_id           : session ID
        history->history_pos : INDEX

    OUTPUT
        history_cmd : the next history command of the session

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_history_cmd_get_next(
    IN    u32                         session_id,
    INOUT icli_session_history_cmd_t  *history
)
{
    icli_session_handle_t   *handle;
    i32                     rc;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( history == NULL ) {
        T_E("history == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    /* session alive */
    if ( vtss_icli_session_alive(session_id) == FALSE ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    /* get handle */
    handle = vtss_icli_session_handle_get(session_id);

    /* if history is empty */
    if ( vtss_icli_sutil_history_cmd_empty(handle) ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_EMPTY;
    }

    /* get next */
    ++( history->history_pos );
    rc = _session_history_cmd_get(handle, history);

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    get privilege of a session

    INPUT
        session_id : session ID

    OUTPUT
        privilege  : session privilege

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_privilege_get(
    IN  u32                 session_id,
    OUT icli_privilege_t    *privilege
)
{
    icli_session_handle_t   *handle;

    if ( privilege == NULL ) {
        T_E("privilege == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    /* get config data */
    *privilege = (icli_privilege_t)(handle->runtime_data.privilege);

    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set privilege of a session

    INPUT
        session_id : session ID
        privilege  : session privilege

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_privilege_set(
    IN  u32                 session_id,
    IN  icli_privilege_t    privilege
)
{
    icli_session_handle_t   *handle;

    if ( privilege >= ICLI_PRIVILEGE_MAX ) {
        T_E("invalid privilege %u\n", privilege);
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    /* set config data */
    handle->runtime_data.privilege = privilege;

    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    get agent id of a session

    INPUT
        session_id : session ID

    OUTPUT
        agent_id   : session agent ID

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_agent_id_get(
    IN  u32 session_id,
    OUT u16 *agent_id
)
{
    icli_session_handle_t   *handle;

    if ( agent_id == NULL ) {
        T_E("agent_id == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    /* get config data */
    *agent_id = (u16)(handle->runtime_data.agent_id);

    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set agent id of a session

    INPUT
        session_id : session ID
        agent_id   : session agent ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_agent_id_set(
    IN  u32 session_id,
    IN  u16 agent_id
)
{
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    /* set config data */
    handle->runtime_data.agent_id = agent_id;

    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set width of a session

    INPUT
        session_id : session ID
        width      : width (in number of characters) of the session

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_width_set(
    IN  u32     session_id,
    IN  u32     width
)
{
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->config_data->width = width;

    if ( width == 0 ) {
        handle->runtime_data.width = ICLI_MAX_WIDTH;
    } else if ( width < ICLI_MIN_WIDTH ) {
        handle->runtime_data.width = ICLI_MIN_WIDTH;
    } else {
        handle->runtime_data.width = width;
    }

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return ICLI_RC_OK;
}

/*
    set lines of a session

    INPUT
        session_id : session ID
        lines      : number of lines on a screen

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_lines_set(
    IN  u32     session_id,
    IN  u32     lines
)
{
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->config_data->lines = lines;

    if ( lines == 0 ) {
        handle->runtime_data.lines = 0;
    } else if ( lines < ICLI_MIN_LINES ) {
        handle->runtime_data.lines = ICLI_MIN_LINES;
    } else {
        handle->runtime_data.lines = lines;
    }

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return ICLI_RC_OK;
}

/*
    set waiting time of a session

    INPUT
        session_id : session ID
        wait_time  : time to wait user input, in seconds
                     = 0 means wait forever

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_wait_time_set(
    IN  u32     session_id,
    IN  u32     wait_time
)
{
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    // convert to msec
    handle->config_data->wait_time = wait_time;

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return ICLI_RC_OK;
}

/*
    get line mode of the session

    INPUT
        session_id : the session ID

    OUTPUT
        line_mode : line mode of the session

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_line_mode_get(
    IN  u32                 session_id,
    OUT icli_line_mode_t    *line_mode
)
{
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    *line_mode = (icli_line_mode_t)(handle->runtime_data.line_mode);

    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set line mode of the session

    INPUT
        session_id : the session ID
        line_mode  : line mode of the session

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_line_mode_set(
    IN  u32                 session_id,
    IN  icli_line_mode_t    line_mode
)
{
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->runtime_data.line_mode = line_mode;

    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    get input style of the session

    INPUT
        session_id : the session ID

    OUTPUT
        input_style : input style

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_input_style_get(
    IN  u32                 session_id,
    OUT icli_input_style_t  *input_style
)
{
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    *input_style = handle->config_data->input_style;

    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set input style of the session

    INPUT
        session_id  : the session ID
        input_style : input style of the session

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_input_style_set(
    IN  u32                 session_id,
    IN  icli_input_style_t  input_style
)
{
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->config_data->input_style = input_style;

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return ICLI_RC_OK;
}

/*
    enable/disable the display of EXEC banner of the session

    INPUT
        session_id    : the session ID
        b_exec_banner : TRUE - enable, FALSE - disable

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_exec_banner_enable(
    IN  u32     session_id,
    IN  BOOL    b_exec_banner
)
{
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->config_data->b_exec_banner = b_exec_banner;

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return ICLI_RC_OK;
}

/*
    enable/disable the display of Day banner of the session

    INPUT
        session_id    : the session ID
        b_motd_banner : TRUE - enable, FALSE - disable

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_motd_banner_enable(
    IN  u32     session_id,
    IN  BOOL    b_motd_banner
)
{
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->config_data->b_motd_banner = b_motd_banner;

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return ICLI_RC_OK;
}

/*
    set location of the session

    INPUT
        session_id : the session ID
        location   : where you are

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_location_set(
    IN  u32                 session_id,
    IN  char                *location
)
{
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( location == NULL ) {
        handle->config_data->location[0] = ICLI_EOS;
    } else {
        (void)vtss_icli_str_ncpy(handle->config_data->location, location, ICLI_LOCATION_MAX_LEN);
    }

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return ICLI_RC_OK;
}

/*
    set privileged level of the session

    INPUT
        session_id        : the session ID
        default_privilege : default privilege level

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_privileged_level_set(
    IN  u32                 session_id,
    IN  icli_privilege_t    privileged_level
)
{
    icli_session_handle_t   *handle;

    if ( privileged_level >= ICLI_PRIVILEGE_MAX ) {
        T_E("invalid privileged level %u\n", privileged_level);
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->config_data->privileged_level = privileged_level;

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return ICLI_RC_OK;
}

/*
    set user name of the session

    INPUT
        session_id : the session ID
        user_name  : who login

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_user_name_set(
    IN  u32     session_id,
    IN  char    *user_name
)
{
    icli_session_handle_t   *handle;

    if ( user_name == NULL ) {
        T_E("user_name == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    (void)vtss_icli_str_ncpy(handle->runtime_data.user_name, user_name, ICLI_USERNAME_MAX_LEN);

    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    output formated string to a specific session
    the maximum length to print is (ICLI_STR_MAX_LEN + 64),
    where ICLI_STR_MAX_LEN depends on project.

    * if the length is > (ICLI_STR_MAX_LEN + 64),
      then please use icli_session_self_printf_lstr()

    INPUT
        format     : string format
        ...        : parameters of format

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
int icli_session_self_printf(
    IN  const char  *format,
    IN  ...
)
{
    u32                     session_id;
    va_list                 arglist;
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    session_id = vtss_icli_session_self_id();
    if ( session_id >= vtss_icli_session_max_cnt_get() ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( vtss_icli_session_alive(session_id) == FALSE ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    handle = vtss_icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("session %d handle is NULL\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    switch ( handle->open_data.way ) {
    case ICLI_SESSION_WAY_CONSOLE:
    case ICLI_SESSION_WAY_TELNET:
    case ICLI_SESSION_WAY_SSH:
    case ICLI_SESSION_WAY_THREAD_CONSOLE:
    case ICLI_SESSION_WAY_THREAD_TELNET:
    case ICLI_SESSION_WAY_THREAD_SSH:
        if ( handle->runtime_data.line_mode == ICLI_LINE_MODE_BYPASS ) {
            _ICLI_SEMA_GIVE();
            return ICLI_RC_ERR_BYPASS;
        }
        break;

    case ICLI_SESSION_WAY_APP_EXEC:
        if ( handle->open_data.b_app_output ) {
            break;
        }
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_BYPASS;

    default:
        _ICLI_SEMA_GIVE();
        T_E("invalid session %d type %d\n", session_id, handle->open_data.way);
        return ICLI_RC_ERR_PARAMETER;
    }

    //format string buffer
    /*lint -e{530} ... 'arglist' is initialized by va_start() */
    va_start( arglist, format );

    (void)vtss_icli_session_va_str_put(session_id, format, arglist);

    va_end( arglist );

    _ICLI_SEMA_GIVE();
    return ICLI_RC_OK;
}

/*
    output long string to a specific session
    the length of long string is larger than (ICLI_STR_MAX_LEN),
    where ICLI_STR_MAX_LEN depends on project

    INPUT
        session_id : the session ID
        lstr       : long string to print
                     *** str must be a char array or dynamic allocated memory(write-able),
                         and must not a const string(read_only).

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_self_printf_lstr(
    IN  const char  *lstr
)
{
    i32     rc;
    u32     session_id;

    _ICLI_SEMA_TAKE();

    session_id = vtss_icli_session_self_id();

    if ( session_id >= vtss_icli_session_max_cnt_get() ) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_GIVE();

    rc = icli_session_printf_lstr(session_id, lstr);
    return rc;
}

/*
    set general error text string when error happen
    this error string will print out before error_txt(rc)

    INPUT
        session_id : ID of session
        msg        : general error message

    OUTPUT
        n/a

    RETURN
        n/a

    COMMENT
        n/a
*/
void icli_session_rc_check_setup(
    IN  u32         session_id,
    IN  const char  *msg
)
{
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    if ( session_id >= vtss_icli_session_max_cnt_get() ) {
        _ICLI_SEMA_GIVE();
        return;
    }

    if ( vtss_icli_session_alive(session_id) == FALSE ) {
        _ICLI_SEMA_GIVE();
        return;
    }

    handle = vtss_icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        return;
    }

    handle->runtime_data.rc_context_string = msg;
    _ICLI_SEMA_GIVE();
}

/*
    check if rc is ok or not.
    if rc is error, then print out sequence is as follows.
        1. msg (from this API)
        2. rc_context_string (from icli_session_rc_check_setup())
        3. error_txt(rc).

    INPUT
        session_id : ID of session
        rc         : error code
        msg        : general error message

    OUTPUT
        n/a

    RETURN
        n/a

    COMMENT
        n/a
*/
mesa_rc icli_session_rc_check(
    IN  u32         session_id,
    IN  mesa_rc     rc,
    IN  const char  *msg
)
{
    const char  *rc_context_string;

    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    if ( session_id >= vtss_icli_session_max_cnt_get() ) {
        _ICLI_SEMA_GIVE();
        return VTSS_RC_ERROR;
    }

    if ( vtss_icli_session_alive(session_id) == FALSE ) {
        _ICLI_SEMA_GIVE();
        return VTSS_RC_ERROR;
    }

    handle = vtss_icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        return VTSS_RC_ERROR;
    }

    rc_context_string = handle->runtime_data.rc_context_string;

    _ICLI_SEMA_GIVE();

    if ( rc != VTSS_RC_OK ) {
        if ( msg ) {
            (void)icli_session_printf(session_id, "%s", msg);
        } else if ( rc_context_string ) {
            (void)icli_session_printf(session_id, "%s", rc_context_string);
        }
        (void)icli_session_printf(session_id, "%% (%s)\n", error_txt(rc));
    }
    return rc;
}

/*
    get value of the specific command word with word_id
    where word_id is from 0.
    for example, the command,
                  show ip interface
        word_id = 0    1  2

    INPUT
        session_id : session ID
        word_id    : command word ID

    OUTPUT
        value : value resulting from user input

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_cmd_value_get(
    IN  u32                     session_id,
    IN  u32                     word_id,
    OUT icli_variable_value_t   *value
)
{
    icli_session_handle_t   *handle;
    icli_parameter_t        *cmd_var;
    i32                     rc;

    if ( value == NULL ) {
        T_E("invalid value == NULL, session_id = %u\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = ICLI_RC_ERROR;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    for ( cmd_var = handle->runtime_data.cmd_var; cmd_var != NULL; ___NEXT(cmd_var) ) {
        if ( cmd_var->word_id == word_id ) {
            *value = cmd_var->value;
            rc = ICLI_RC_OK;
            break;
        }
    }

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    Get command value of a specific mode at a specific position (0-based).
    Useful in nested command modes.
    For example, the command,
                  show ip interface
        word_id = 0    1  2

    INPUT
        session_id : session ID
        mode       : The ICLI command mode to look for
        word_id    : command word ID

    OUTPUT
        value : value resulting from user input

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_cmd_value_from_mode_get(
    IN  u32                     session_id,
    IN  u32                     word_id,
    IN  icli_cmd_mode_t         mode,
    OUT icli_variable_value_t   *value
)
{
    icli_session_handle_t *handle;
    icli_parameter_t      *cmd_var;
    icli_mode_para_t      *mode_para;
    i32                   rc, level;

    if (value == NULL) {
        T_E("session_id = %d: value == NULL", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = ICLI_RC_ERROR;

    _ICLI_SEMA_TAKE();

    if ((handle = vtss_icli_session_handle_get(session_id)) == NULL) {
        T_E("session_id = %d: handle == NULL", session_id);
        rc = ICLI_RC_ERR_PARAMETER;
        goto do_exit;
    }

    if ((level = handle->runtime_data.mode_level) >= ICLI_MODE_MAX_LEVEL) {
        T_E("session_id = %d: Internal error: Invalid level (%d)", session_id, level);
        return ICLI_RC_ERR_PARAMETER;
    }

    while (level >= 0) {
        mode_para = &handle->runtime_data.mode_para[level--];
        if (mode_para->mode != mode) {
            continue;
        }

        for (cmd_var = mode_para->cmd_var; cmd_var != NULL; ___NEXT(cmd_var) ) {
            if (cmd_var->word_id == word_id) {
                *value = cmd_var->value;
                rc = ICLI_RC_OK;
                goto do_exit;
            }
        }
    }

do_exit:
    _ICLI_SEMA_GIVE();

    return rc;
}


/*
    get session way

    INPUT
        session_id : session ID

    OUTPUT
        way : session way

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_way_get(
    IN  u32                     session_id,
    OUT icli_session_way_t      *way
)
{
    icli_session_handle_t   *handle;

    if ( way == NULL ) {
        T_E("invalid way == NULL, session_id = %u\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    *way = handle->open_data.way;

    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

#if 1 /* Bugzilla#14129 - remove debug level */
/*
    set allow or deny debug command per session

    INPUT
        session_id : session ID
        b_allow    : TRUE  - allow
                     FALSE - deny

    OUTPUT
        n/a

    RETURN
        n/a

    COMMENT
        n/a
*/
void icli_session_debug_cmd_allow_set(
    IN  u32     session_id,
    IN  BOOL    b_allow
)
{
    icli_session_handle_t   *handle;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return;
    }

    vtss_icli_session_debug_cmd_allow_set( handle, b_allow );

    _ICLI_SEMA_GIVE();
}

/*
    get debug command is allowed or denied per session

    INPUT
        session_id : session ID

    OUTPUT
        n/a

    RETURN
        TRUE  - allow
        FALSE - deny

    COMMENT
        n/a
*/
BOOL icli_session_debug_cmd_allow_get(
    IN  u32     session_id
)
{
    icli_session_handle_t   *handle;
    BOOL                    b_allow;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return FALSE;
    }

    b_allow = vtss_icli_session_debug_cmd_allow_get( handle );

    _ICLI_SEMA_GIVE();

    return b_allow;
}

/*
    get if the allow is the first time or not

    INPUT
        session_id : session ID

    OUTPUT
        n/a

    RETURN
        TRUE  - this is the first time
        FALSE - more times

    COMMENT
        n/a
*/
BOOL icli_session_debug_cmd_allow_first_time_get(
    IN  u32     session_id
)
{
    icli_session_handle_t   *handle;
    BOOL                    b_first_time;

    _ICLI_SEMA_TAKE();

    handle = vtss_icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _ICLI_SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return FALSE;
    }

    b_first_time = vtss_icli_session_debug_cmd_allow_first_time_get( handle );

    _ICLI_SEMA_GIVE();

    return b_first_time;
}
#endif

/*
    get current session command mode and the mode parameter value.
    parse value according to mode

    INPUT
        session_id : session ID

    OUTPUT
        mode  : current session command mode
        value : mode parameter value

    RETURN
        ICLI_RC_OK : successful
        others     : failed

    COMMENT
        mode and value are optional.
        If not needed, then put NULL in the API.
*/
i32 icli_session_mode_para_get(
    IN  u32                     session_id,
    OUT icli_cmd_mode_t         *mode,
    OUT icli_variable_value_t   *value
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    if (vtss_icli_session_alive(session_id) == FALSE) {
        _ICLI_SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = vtss_icli_session_mode_para_get(session_id, mode, value);

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    set mode name shown in command prompt

    INPUT
        mode : command mode
        name : mode name

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_mode_name_set(
    IN  icli_cmd_mode_t     mode,
    IN  char                *name
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    rc = vtss_icli_mode_prompt_set( mode, name );

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return rc;
}

/*
    set device name shown in command prompt

    INPUT
        dev_name : device name

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_dev_name_set(
    IN  char    *dev_name
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    rc = vtss_icli_dev_name_set( dev_name );

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return rc;
}

/*
    get device name

    INPUT
        n/a

    OUTPUT
        dev_name : device name

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_dev_name_get(
    OUT char    *dev_name
)
{
    char    *s;

    if ( dev_name == NULL ) {
        T_E("dev_name == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    s = vtss_icli_str_cpy(dev_name, vtss_icli_dev_name_get());

    _ICLI_SEMA_GIVE();

    if ( s ) {
        return ICLI_RC_OK;
    } else {
        return ICLI_RC_ERROR;
    }
}

/*
    get LOGIN banner.
    the maximum length is 255 chars, ICLI_BANNER_MAX_LEN.

    INPUT
        n/a

    OUTPUT
        banner_login : LOGIN banner

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_banner_login_get(
    OUT  char    *banner_login
)
{
    i32     rc = ICLI_RC_OK;

    if ( banner_login == NULL ) {
        T_E("banner_login == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    (void)vtss_icli_str_cpy(banner_login, vtss_icli_banner_login_get());

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    set LOGIN banner.
    the maximum length is 255 chars, ICLI_BANNER_MAX_LEN.

    INPUT
        banner_login : LOGIN banner

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_banner_login_set(
    IN  char    *banner_login
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    rc = vtss_icli_banner_login_set( banner_login );

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return rc;
}

/*
    get MOTD banner.
    the maximum length is 255 chars, ICLI_BANNER_MAX_LEN.

    INPUT
        n/a

    OUTPUT
        banner_motd : MOTD banner

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_banner_motd_get(
    OUT  char    *banner_motd
)
{
    i32     rc = ICLI_RC_OK;

    if ( banner_motd == NULL ) {
        T_E("banner_motd == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    (void)vtss_icli_str_cpy(banner_motd, vtss_icli_banner_motd_get());

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    set MOTD banner.
    the maximum length is 255 chars, ICLI_BANNER_MAX_LEN.

    INPUT
        banner_motd : MOTD banner

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_banner_motd_set(
    IN  char    *banner_motd
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    rc = vtss_icli_banner_motd_set( banner_motd );

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return rc;
}

/*
    get EXEC banner.
    the maximum length is 255 chars, ICLI_BANNER_MAX_LEN.

    INPUT
        n/a

    OUTPUT
        banner_exec : EXEC banner

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_banner_exec_get(
    OUT  char    *banner_exec
)
{
    i32     rc = ICLI_RC_OK;

    if ( banner_exec == NULL ) {
        T_E("banner_exec == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _ICLI_SEMA_TAKE();

    (void)vtss_icli_str_cpy(banner_exec, vtss_icli_banner_exec_get());

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    set EXEC banner.
    the maximum length is 255 chars, ICLI_BANNER_MAX_LEN.

    INPUT
        banner_exec : EXEC banner

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_banner_exec_set(
    IN  char    *banner_exec
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    rc = vtss_icli_banner_exec_set( banner_exec );

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return rc;
}

/*
    check if the port type is present in this device

    INPUT
        type : port type

    OUTPUT
        n/a

    RETURN
        TRUE  : yes, the device has ports belong to this port type
        FALSE : no

    COMMENT
        n/a
*/
BOOL icli_port_type_present(
    IN icli_port_type_t     type
)
{
    BOOL    b;

    _ICLI_SEMA_TAKE();

    b = vtss_icli_port_type_present( type );

    _ICLI_SEMA_GIVE();

    return b;
}

/*
    get port type for a specific switch port

    INPUT
        type : port type

    OUTPUT
        n/a

    RETURN
        name of the type
            if the type is invalid. then "Unknown" is return

    COMMENT
        n/a
*/
//lint -sem(icli_port_type_get_name, thread_protected)
char *icli_port_type_get_name(
    IN  u16    type
)
{
    return vtss_icli_variable_port_type_get_name((icli_port_type_t)type);
}

/*
    get short port type name for a specific port type

    INPUT
        type : port type

    OUTPUT
        n/a

    RETURN
        short name of the type
            if the type is invalid. then "Unkn" is returned

    COMMENT
        n/a
*/
//lint -sem(icli_port_type_get_short_name, thread_protected)
const char *icli_port_type_get_short_name(
    IN  icli_port_type_t    type
)
{
    return vtss_icli_variable_port_type_get_short_name(type);
}

/*
    reset port range

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        n/a

    COMMENT
        n/a
*/
void icli_port_range_reset( void )
{
    _ICLI_SEMA_TAKE();

    vtss_icli_port_range_reset();

    _ICLI_SEMA_GIVE();
}

/*
    get port range

    INPUT

    OUTPUT
        range : port range

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_port_range_get(
    OUT icli_stack_port_range_t  *range
)
{
    BOOL    b;

    _ICLI_SEMA_TAKE();

    b = vtss_icli_port_range_get( range );

    _ICLI_SEMA_GIVE();

    return b;
}

/*
    set port range on a specific port type

    INPUT
        range  : port range set on the port type

    OUTPUT
        n/a

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        CIRTICAL ==> before set, port range must be reset by icli_port_range_reset().
        otherwise, the set may be failed because this set will check the
        port definition to avoid duplicate definitions.
*/
BOOL icli_port_range_set(
    IN icli_stack_port_range_t  *range
)
{
    BOOL    b;

    _ICLI_SEMA_TAKE();

    b = vtss_icli_port_range_set(range);

    _ICLI_SEMA_GIVE();

    return b;
}

/*
    get switch range from usid and uport

    INPUT
        switch_range->usid        : usid
        switch_range->begin_uport : uport
        switch_range->port_cnt    : number of ports

    OUTPUT
        switch_range

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_port_from_usid_uport(
    INOUT icli_switch_port_range_t  *switch_range
)
{
    BOOL    b;

    _ICLI_SEMA_TAKE();

    b = vtss_icli_port_from_usid_uport( switch_range );

    _ICLI_SEMA_GIVE();

    return b;
}

/*
    get switch range from isid and iport

    INPUT
        switch_range->isid        : isid
        switch_range->begin_iport : iport
        switch_range->port_cnt    : number of ports

    OUTPUT
        switch_range

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_port_from_isid_iport(
    INOUT icli_switch_port_range_t  *switch_range
)
{
    BOOL    b;

    _ICLI_SEMA_TAKE();

    b = vtss_icli_port_from_isid_iport( switch_range );

    _ICLI_SEMA_GIVE();

    return b;
}

/*
    get first switch_id/type/port

    INPUT
        n/a

    OUTPUT
        switch_range->port_type   : first port type
        switch_range->switch_id   : first switch ID
        switch_range->begin_port  : first port ID
        switch_range->usid        : usid
        switch_range->begin_uport : uport
        switch_range->isid        : isid
        switch_range->begin_iport : iport

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_port_get_first(
    OUT icli_switch_port_range_t  *switch_range
)
{
    BOOL    b;

    _ICLI_SEMA_TAKE();

    b = vtss_icli_port_get_first( switch_range );

    _ICLI_SEMA_GIVE();

    return b;
}

/*
    get from switch_id/type/port

    INPUT
        index:
            switch_range->switch_id   : switch ID
            switch_range->port_type   : port type
            switch_range->begin_port  : port ID

    OUTPUT
        switch_range->usid        : usid
        switch_range->begin_uport : uport
        switch_range->isid        : isid
        switch_range->begin_iport : iport

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_port_get(
    INOUT icli_switch_port_range_t  *switch_range
)
{
    BOOL    b;

    _ICLI_SEMA_TAKE();

    b = vtss_icli_port_get( switch_range );

    _ICLI_SEMA_GIVE();

    return b;
}

/*
    get next switch_id/type/port

    INPUT
        index:
            switch_range->switch_id   : switch ID
            switch_range->port_type   : port type
            switch_range->begin_port  : port ID

    OUTPUT
        switch_range->switch_id   : next switch ID
        switch_range->port_type   : next port type
        switch_range->begin_port  : next port ID
        switch_range->usid        : usid
        switch_range->begin_uport : uport
        switch_range->isid        : isid
        switch_range->begin_iport : iport

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_port_get_next(
    INOUT icli_switch_port_range_t  *switch_range
)
{
    BOOL    b;

    _ICLI_SEMA_TAKE();

    b = vtss_icli_port_get_next( switch_range );

    _ICLI_SEMA_GIVE();

    return b;
}

/*
    check if the switch port range is valid or not

    INPUT
        switch_port : the followings are checking parameters
            switch_port->port_type
            switch_port->switch_id
            switch_port->begin_port
            switch_port->port_cnt

    OUTPUT
        switch_id, port_id : the port is not valid
                             put NULL if these are not needed

    RETURN
        TRUE  : all the switch ports are valid
        FALSE : at least one of switch port is not valid

    COMMENT
        n/a
*/
BOOL icli_port_switch_range_valid(
    IN  icli_switch_port_range_t    *switch_port,
    OUT u16                         *switch_id,
    OUT u16                         *port_id
)
{
    BOOL    b;

    _ICLI_SEMA_TAKE();

    b = vtss_icli_port_switch_range_valid( switch_port, switch_id, port_id );

    _ICLI_SEMA_GIVE();

    return b;
}

/*
    get switch_id by usid

    INPUT
        usid

    OUTPUT
        n/a

    RETURN
        switch ID

    COMMENT
        n/a
*/
u16 icli_usid2switchid(
    IN u16  usid
)
{
    return icli_platform_usid2switchid( usid );
}

/*
    get switch_id by isid

    INPUT
        isid

    OUTPUT
        n/a

    RETURN
        switch ID

    COMMENT
        n/a
*/
u16 icli_isid2switchid(
    IN u16  isid
)
{
    return icli_platform_isid2switchid( isid );
}

/*
    get switch_id

    INPUT
        index:
            switch_range->switch_id : switch ID

    OUTPUT
        switch_range->switch_id
        switch_range->usid
        switch_range->isid
        others = 0

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_switch_get(
    INOUT icli_switch_port_range_t  *switch_range
)
{
    BOOL    b;

    if ( switch_range == NULL ) {
        T_E("switch_range == NULL\n");
        return FALSE;
    }

    _ICLI_SEMA_TAKE();

    b = vtss_icli_switch_get( switch_range );

    _ICLI_SEMA_GIVE();

    return b;
}

/*
    get next switch_id

    INPUT
        index:
            switch_range->switch_id : switch ID

    OUTPUT
        switch_range->switch_id : next switch ID
        switch_range->usid      : next usid
        switch_range->isid      : next isid
        others = 0

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_switch_get_next(
    INOUT icli_switch_port_range_t  *switch_range
)
{
    BOOL    b;

    if ( switch_range == NULL ) {
        T_E("switch_range == NULL\n");
        return FALSE;
    }

    _ICLI_SEMA_TAKE();

    b = vtss_icli_switch_get_next( switch_range );

    _ICLI_SEMA_GIVE();

    return b;
}

/*
    get from switch_id/type/port

    INPUT
        index :
            switch_range->switch_id   : switch ID
            switch_range->port_type   : port type
            switch_range->begin_port  : port ID

    OUTPUT
        switch_range->usid        : usid
        switch_range->begin_uport : uport
        switch_range->isid        : isid
        switch_range->begin_iport : iport

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_switch_port_get(
    INOUT icli_switch_port_range_t  *switch_range
)
{
    BOOL    b;

    if ( switch_range == NULL ) {
        T_E("switch_range == NULL\n");
        return FALSE;
    }

    _ICLI_SEMA_TAKE();

    b = vtss_icli_switch_port_get( switch_range );

    _ICLI_SEMA_GIVE();

    return b;
}

/*
    get next switch_id/type/port

    INPUT
        index:
            switch_range->switch_id   : switch ID (not changed)
            switch_range->port_type   : port type
            switch_range->begin_port  : port ID

    OUTPUT
        switch_range->switch_id   : current switch ID (not changed)
        switch_range->port_type   : next port type
        switch_range->begin_port  : next port ID
        switch_range->usid        : usid
        switch_range->begin_uport : uport
        switch_range->isid        : isid
        switch_range->begin_iport : iport

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_switch_port_get_next(
    INOUT icli_switch_port_range_t  *switch_range
)
{
    BOOL    b;

    if ( switch_range == NULL ) {
        T_E("switch_range == NULL\n");
        return FALSE;
    }

    _ICLI_SEMA_TAKE();

    b = vtss_icli_switch_port_get_next( switch_range );

    _ICLI_SEMA_GIVE();

    return b;
}

/*
    get password of privilege level

    INPUT
        priv : the privilege level

    OUTPUT
        password : the corresponding password with size (ICLI_PASSWORD_MAX_LEN + 1)

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_enable_password_get(
    IN  icli_privilege_t    priv,
    OUT char                *password
)
{
    BOOL    b;

    _ICLI_SEMA_TAKE();

    b = vtss_icli_enable_password_get(priv, password);

    _ICLI_SEMA_GIVE();

    return b;
}

/*
    set password of privilege level

    INPUT
        priv     : the privilege level
        password : the corresponding password with size (ICLI_PASSWORD_MAX_LEN + 1)

    OUTPUT
        n/a

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_enable_password_set(
    IN  icli_privilege_t    priv,
    IN  const char          *password
)
{
    BOOL    b;

    _ICLI_SEMA_TAKE();

    b = vtss_icli_enable_password_set(priv, password);

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return b;
}

#if 1 /* CP, 2012/08/31 17:00, enable secret */
/*
    verify clear password of privilege level is correct or not
    according to password or secret

    INPUT
        priv           : the privilege level
        clear_password : the corresponding password with size (ICLI_PASSWORD_MAX_LEN + 1)

    OUTPUT
        n/a

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_enable_password_verify(
    IN  icli_privilege_t    priv,
    IN  char                *clear_password
)
{
    BOOL    b;

    _ICLI_SEMA_TAKE();

    b = vtss_icli_enable_password_verify(priv, clear_password);

    _ICLI_SEMA_GIVE();

    return b;
}

/*
    set secret of privilege level

    INPUT
        priv   : the privilege level
        secret : the corresponding secret with size (ICLI_PASSWORD_MAX_LEN + 1)

    OUTPUT
        n/a

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_enable_secret_set(
    IN  icli_privilege_t    priv,
    IN  const char          *secret
)
{
    BOOL    b;

    _ICLI_SEMA_TAKE();

    b = vtss_icli_enable_secret_set(priv, secret);

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return b;
}

/*
    translate clear password of privilege level to secret password

    INPUT
        priv           : the privilege level
        clear_password : the corresponding password with size (ICLI_PASSWORD_MAX_LEN + 1)

    OUTPUT
        n/a

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_enable_secret_clear_set(
    IN  icli_privilege_t    priv,
    IN  char                *clear_password
)
{
    BOOL    b;

    _ICLI_SEMA_TAKE();

    b = vtss_icli_enable_secret_set_clear(priv, clear_password);

    _ICLI_SEMA_GIVE();

    if ( icli_conf_file_write() == FALSE ) {
        T_W("fail to write conf\n");
    }

    return b;
}

/*
    get if the enable password is in secret or not

    INPUT
        priv : the privilege level

    OUTPUT
        n/a

    RETURN
        TRUE  : in secret
        FALSE : clear password

    COMMENT
        n/a
*/
BOOL icli_enable_password_if_secret_get(
    IN  icli_privilege_t    priv
)
{
    BOOL    b;

    _ICLI_SEMA_TAKE();

    b = vtss_icli_enable_password_if_secret_get( priv );

    _ICLI_SEMA_GIVE();

    return b;
}
#endif

static i32 prompt_verify(const char *new_prompt)
{
    const char *s;

    if (!new_prompt) {
        return ICLI_RC_ERR_PARAMETER;
    }

    if (strlen(new_prompt) > sizeof(g_prompt) - 1) {
        T_D("Prompt (%s) too long. Max size (without terminating null) is %zu bytes", new_prompt, sizeof(g_prompt) - 1);
        return ICLI_RC_ERROR;
    }

    s = new_prompt;
    while (*s) {
        if (*s == '%') {
            switch (*(s + 1)) {
            case 'h': // hostname (default)
            case '%': // A percent sign
            case 's': // space
            case 't': // tab
            case 'D': // Date only
            case 'T': // Time only
            case 'Z': // Date and Time (like %DT%T, but ensures atomicity)
                s++;
                break;

            case '\0':
                T_D("Prompt (%s) cannot end with a '%%'", new_prompt);
                return ICLI_RC_ERROR;

            default:
                T_D("Invalid character (%c) succeeding %% in prompt string (%s)", *(s + 1), new_prompt);
                return ICLI_RC_ERROR;
            }
        }

        s++;
    }

    return ICLI_RC_OK;
}

const char *icli_prompt_get(void)
{
    return g_prompt;
}

const char *icli_prompt_default_get(void)
{
    return "%h";
}

BOOL icli_prompt_is_default(const char *prompt)
{
    // It's considered default when it's an empty string or is "%h".
    return prompt && (prompt[0] == '\0' || strcmp(prompt, icli_prompt_default_get()) == 0);
}

i32 icli_prompt_set(const char *prompt)
{
    i32 rc;

    if ((rc = prompt_verify(prompt)) != ICLI_RC_OK) {
        return rc;
    }

    _ICLI_SEMA_TAKE();
    strncpy(g_prompt, prompt, sizeof(g_prompt) - 1);
    g_prompt[sizeof(g_prompt) - 1] = 0;
    _ICLI_SEMA_GIVE();

    return ICLI_RC_OK;
}

// Internal function that returns an interpreted prompt
// This function will NOT take the mutex, so the caller must
// have taken it already.
i32 icli_prompt_interpreted_get(char *interpreted_prompt, size_t input_len)
{
    char   temp_prompt[sizeof(g_prompt)], *s_in, *s_out, s_date_and_time[VTSS_APPL_RFC3339_TIME_STR_LEN];
    BOOL   overflow = FALSE;
    size_t len = input_len;

    if (!interpreted_prompt || input_len < 1) {
        return ICLI_RC_ERR_PARAMETER;
    }

    strcpy(temp_prompt, g_prompt);

    if (strlen(temp_prompt) == 0) {
        strncpy(temp_prompt, icli_prompt_default_get(), sizeof(g_prompt) - 1);
    }

    s_in  = temp_prompt;
    s_out = interpreted_prompt;

    // Reserve room for a terminating NULL.
    len--;

    while (*s_in && len > 0) {
        if (*s_in == '%') {
            BOOL print_date = FALSE;
            BOOL print_time = FALSE;
            const char *s_temp = NULL;

            switch (*(s_in + 1)) {
            case 'h': // hostname
                s_temp = vtss_icli_dev_name_get();
                break;

            case '%': // A percent sign
                *(s_out++) = '%';
                len--;
                break;

            case 's': // tab
                *(s_out++) = ' ';
                len--;
                break;

            case 't': // tab
                *(s_out++) = 9;
                len--;
                break;

            case 'D': // Date
                print_date = TRUE;
                break;

            case 'T': // Time
                print_time = TRUE;
                break;

            case 'Z': // Both Date and Time
                print_date = TRUE;
                print_time = TRUE;
                break;

            case '\0':
                T_E("Prompt (%s) cannot end with a '%%'", temp_prompt);
                return ICLI_RC_ERROR;

            default:
                T_E("Invalid character (%c) succeeding %% in prompt string (%s)", *(s_in + 1), temp_prompt);
                return ICLI_RC_ERROR;
            }

            if (print_date || print_time) {
                s_temp = misc_time2date_time_str_r(time(NULL), s_date_and_time, print_date, print_time);
            }

            if (s_temp) {
                size_t l = strlen(s_temp), b;
                strncpy(s_out, s_temp, len);

                // Figure out how many bytes were actually copied
                b = l > len ? len : l;
                overflow = l > len;
                len   -= b;
                s_out += b;
            }

            // Eat the % so that the s++ below can eat the modifier
            s_in++;
        } else {
            len--;
            *(s_out++) = *s_in;
        }

        s_in++;
    }

    // If still input and end of output is reached or could
    // have been overflown, end string with three dots (if input_len allows)
    if ((*s_in || overflow) && input_len > 3) {
        T_D("Changing end to dots");
        interpreted_prompt[input_len - 4] = '.';
        interpreted_prompt[input_len - 3] = '.';
        interpreted_prompt[input_len - 2] = '.';
        // Gets NULL-terminated below.
    }

    *s_out = '\0';

    return ICLI_RC_OK;
}

/*
    set privilege per command

    INPUT
        conf   : privilege command configuration

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_priv_set(
    IN  icli_priv_cmd_conf_t    *conf
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    rc = vtss_icli_priv_set( conf );

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    delete privilege per command

    INPUT
        conf : privilege command configuration, index - mode, cmd

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_priv_delete(
    IN icli_priv_cmd_conf_t    *conf
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    rc = vtss_icli_priv_delete( conf );

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    get first privilege per command

    INPUT
        n/a

    OUTPUT
        conf : first privilege command configuration, index - mode, cmd

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_priv_get_first(
    OUT icli_priv_cmd_conf_t    *conf
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    rc = vtss_icli_priv_get_first( conf );

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    get privilege per command

    INPUT
        conf : privilege command configuration, index - mode, cmd

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_priv_get(
    INOUT icli_priv_cmd_conf_t    *conf
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    rc = vtss_icli_priv_get( conf );

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    get next privilege per command
    use index - mode, cmd to find the current one and then get the next one
    of the current one. So, if the current one is not found, then this fails.

    INPUT
        conf : privilege command configuration, sorted by time

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_priv_get_next(
    INOUT icli_priv_cmd_conf_t    *conf
)
{
    i32     rc;

    _ICLI_SEMA_TAKE();

    rc = vtss_icli_priv_get_next( conf );

    _ICLI_SEMA_GIVE();

    return rc;
}

/*
    get mode name

    INPUT
        mode : command mode

    OUTPUT
        n/a

    RETURN
        mode name

    COMMENT
        n/a
*/

char *icli_mode_name_get(
    IN icli_cmd_mode_t  mode
)
{
    return icli_platform_mode_name_get( mode );
}

/*
    get mode by name

    INPUT
        name : name of command mode

    OUTPUT
        n/a

    RETURN
        mode : error if return ICLI_CMD_MODE_MAX

*/
icli_cmd_mode_t icli_mode_get_by_name(
    IN  char    *name
)
{
    return icli_platform_mode_get_by_name( name );
}

/*
    get mode string

    INPUT
        mode : command mode

    OUTPUT
        n/a

    RETURN
        mode string

    COMMENT
        n/a
*/
const char *icli_mode_str_get(
    IN icli_cmd_mode_t  mode
)
{
    return icli_platform_mode_str_get( mode );
}

/*
    check if a and b are sibling

    INPUT
        a : node
        b : node

    OUTPUT
        n/a

    RETURN
        TRUE  - yes
        FALSE - no

    COMMENT
        n/a
*/
BOOL icli_is_sibling(
    icli_parsing_node_t     *a,
    icli_parsing_node_t     *b
)
{
    icli_parsing_node_t     *t;

    if ( a == NULL || b == NULL ) {
        return FALSE;
    }

    for ( t = a; t; t = t->sibling ) {
        if ( t == b ) {
            return TRUE;
        }
    }

    for ( t = b; t; t = t->sibling ) {
        if ( t == a ) {
            return TRUE;
        }
    }

    return FALSE;
}

mesa_rc icli_multiline_data_get(u32 session_id, icli_session_multiline_data_t *const data)
{
    mesa_rc rc = VTSS_RC_OK;

    _ICLI_SEMA_TAKE();
    rc = vtss_icli_session_multiline_data_get(session_id, data);
    _ICLI_SEMA_GIVE();

    return rc;
}

mesa_rc icli_multiline_data_set(u32 session_id, const icli_session_multiline_data_t *data)
{
    mesa_rc rc = VTSS_RC_OK;

    _ICLI_SEMA_TAKE();
    rc = vtss_icli_session_multiline_data_set(session_id, data);
    _ICLI_SEMA_GIVE();

    return rc;
}

void icli_multiline_data_clear(u32 session_id)
{
    _ICLI_SEMA_TAKE();
    vtss_icli_session_multiline_data_clear(session_id);
    _ICLI_SEMA_GIVE();

    return;
}

BOOL icli_has_url_with_proto(u32 session_id, icli_url_proto_t req_proto)
{
    icli_session_handle_t *handle = vtss_icli_session_handle_get(session_id);
    if (handle == NULL) {
        return FALSE;
    }

    icli_parameter_t    *p;
    char temp_buf[ICLI_PUT_MAX_LEN + 4];

    // loop through all defined parameters
    for (p = handle->runtime_data.cmd_var; p; ___NEXT(p)) {
        // look for URL-type parameters only
        if (p->value.type != ICLI_VARIABLE_URL_FILE && p->value.type != ICLI_VARIABLE_URL) {
            continue;
        }
        // try to get the user-assigned value
        if (!vtss_icli_exec_para_word_get(p, temp_buf)) {
            continue;
        }

        // check if URL has requested protocol
        icli_url_proto_t proto = vtss_icli_variable_url_protocol_get(temp_buf);
        T_N("type: %d, value: %s\n", p->value.type, temp_buf);
        T_N("proto: %d, req_proto: %d\n", proto, req_proto);
        if (proto == req_proto) {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL icli_range_vtss_ports(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    if (ask != ICLI_ASK_RANGE) {
        return FALSE;
    }

    runtime->range.type = ICLI_RANGE_TYPE_UNSIGNED;
    runtime->range.u.ur.cnt = 1;
    runtime->range.u.ur.range[0].min = 1;
    runtime->range.u.ur.range[0].max = fast_cap(MEBA_CAP_BOARD_PORT_COUNT);
    return TRUE;
}

