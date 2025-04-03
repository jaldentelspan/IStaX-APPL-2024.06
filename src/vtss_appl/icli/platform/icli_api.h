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

/*
==============================================================================

    Revision history
    > CP.Wang, 2011/04/12 14:45
        - create

==============================================================================
*/
#ifndef __ICLI_API_H__
#define __ICLI_API_H__
//****************************************************************************

/*
==============================================================================

    Include File

==============================================================================
*/
#include "vtss_icli_type.h"
#include "vtss_icli_util.h"
#include "icli_os_misc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
==============================================================================

    Macro

==============================================================================
*/
#define ICLI_VTSS_RC(expr)   { i32 __rc__ = (expr); if (__rc__ != ICLI_RC_OK) return __rc__; }

#define ICLI_CLOSE() \
    icli_session_close(session_id)

#define ICLI_CMD_EXEC_RC(cmd, b_exec, exec_rc) \
    icli_session_cmd_exec_rc(session_id, cmd, b_exec, exec_rc)

#define ICLI_CMD_EXEC(cmd, b_exec) \
    icli_session_cmd_exec(session_id, cmd, b_exec)

#define ICLI_CMD_EXEC_ERR_DISPLAY_RC(cmd, b_exec, app_err_msg, b_display_err_syntax, exec_rc) \
    icli_session_cmd_exec_err_display_rc(session_id, cmd, b_exec, app_err_msg, b_display_err_syntax, exec_rc)

#define ICLI_CMD_EXEC_ERR_DISPLAY(cmd, b_exec, app_err_msg, b_display_err_syntax) \
    icli_session_cmd_exec_err_display(session_id, cmd, b_exec, app_err_msg, b_display_err_syntax)

#define ICLI_CMD_EXEC_ERR_FILE_LINE_RC(cmd, b_exec, filename, line_number, b_display_err_syntax, exec_rc) \
    icli_session_cmd_exec_err_file_line_rc(session_id, cmd, b_exec, filename, line_number, b_display_err_syntax, exec_rc)

#define ICLI_CMD_EXEC_ERR_FILE_LINE(cmd, b_exec, filename, line_number, b_display_err_syntax) \
    icli_session_cmd_exec_err_file_line(session_id, cmd, b_exec, filename, line_number, b_display_err_syntax)

#define ICLI_CMD_PARSING_BEGIN() \
    icli_session_cmd_parsing_begin(session_id);

#define ICLI_CMD_PARSING_END() \
    icli_session_cmd_parsing_end(session_id);

#define ICLI_PRINTF(...) \
    (void)icli_session_printf(session_id, __VA_ARGS__)

#define ICLI_PRINTF_LSTR(lstr) \
    (void)icli_session_printf_lstr(session_id, lstr)

#define ICLI_USR_STR_GET(input_type, str, str_size, end_key) \
    icli_session_usr_str_get(session_id, input_type, str, str_size, end_key)

#define ICLI_CTRL_C_GET(wait_time) \
    icli_session_ctrl_c_get(session_id, wait_time)

#define ICLI_MODE_GET(mode) \
    icli_session_mode_get(session_id, mode)

#define ICLI_MODE_ENTER(mode) \
    icli_session_mode_enter(session_id, mode)

#define ICLI_MODE_EXIT() \
    icli_session_mode_exit(session_id)

#define ICLI_PRIVILEGE_GET(privilege) \
    icli_session_privilege_get(session_id, privilege)

#define ICLI_PRIVILEGE_SET(privilege) \
    icli_session_privilege_set(session_id, privilege)

#define ICLI_TIMEOUT_SET(timeout) \
    icli_session_wait_time_set(session_id, timeout)

#define ICLI_WIDTH_SET(width) \
    icli_session_width_set(session_id, width)

#define ICLI_LINES_SET(lines) \
    icli_session_lines_set(session_id, lines)

#define ICLI_LINE_MODE_GET(line_mode) \
    icli_session_line_mode_get(session_id, line_mode)

#define ICLI_LINE_MODE_SET(line_mode) \
    icli_session_line_mode_set(session_id, line_mode)

#define ICLI_INPUT_STYLE_GET(input_style) \
    icli_session_input_style_get(session_id, input_style)

#define ICLI_INPUT_STYLE_SET(input_style) \
    icli_session_input_style_set(session_id, input_style)

#define ICLI_HISTORY_SIZE_GET(history_size) \
    icli_session_history_size_get(session_id, history_size)

#define ICLI_HISTORY_SIZE_SET(history_size) \
    icli_session_history_size_set(session_id, history_size)

#define ICLI_EXEC_BANNER_ENABLE(b_exec_banner) \
    icli_session_exec_banner_enable(session_id, b_exec_banner)

#define ICLI_MOTD_BANNER_ENABLE(b_exec_banner) \
    icli_session_motd_banner_enable(session_id, b_exec_banner)

#define ICLI_LOCATION_SET(location) \
    icli_session_location_set(session_id, location)

#define ICLI_PRIVILEGED_LEVEL_SET(privileged_level) \
    icli_session_privileged_level_set(session_id, privileged_level)

#define ICLI_SELF_PRINTF(...) \
    (void)icli_session_self_printf(__VA_ARGS__)

#define ICLI_SELF_PRINTF_LSTR(lstr) \
    (void)icli_session_self_printf_lstr(lstr)

#define ICLI_RC_CHECK_SETUP(msg) \
    icli_session_rc_check_setup(session_id, msg)

#define ICLI_RC_CHECK(rc, msg) \
    if ( icli_session_rc_check(session_id, rc, msg) != VTSS_RC_OK ) { \
        return ICLI_RC_ERROR; \
    }

// Same as ICLI_RC_CHECK, but will be printing the message based upon rc using error_txt() function.
#define ICLI_RC_CHECK_PRINT_RC(rc) \
    {                                                                                       \
        mesa_rc __rc__ = (rc);                                                              \
        if ( icli_session_rc_check(session_id, __rc__, NULL) != VTSS_RC_OK ) { \
           return ICLI_RC_ERROR;                                                            \
        }                                                                                   \
    }

#define ICLI_CMD_VALUE_GET(word_id, value) \
    icli_session_cmd_value_get(session_id, word_id, value)

#define ICLI_WAY_GET(way) \
    icli_session_way_get(session_id, way)

#define ICLI_DEBUG_CMD_ALLOW_SET(b_allow) \
    icli_session_debug_cmd_allow_set(session_id, b_allow)

#define ICLI_DEBUG_CMD_ALLOW_GET() \
    icli_session_debug_cmd_allow_get(session_id)

#define ICLI_DEBUG_CMD_ALLOW_FIRST_TIME_GET() \
    icli_session_debug_cmd_allow_first_time_get(session_id)

#define ICLI_MODE_PARA_GET(mode, value) \
    icli_session_mode_para_get(session_id, mode, value)

/*
==============================================================================

    Util Definition

==============================================================================
*/
#define icli_to_upper_case              vtss_icli_to_upper_case
#define icli_to_lower_case              vtss_icli_to_lower_case
#define icli_str_len                    vtss_icli_str_len
#define icli_str_cpy                    vtss_icli_str_cpy
#define icli_str_ncpy                   vtss_icli_str_ncpy
#define icli_str_cmp                    vtss_icli_str_cmp
#define icli_str_sub                    vtss_icli_str_sub
#define icli_str_concat                 vtss_icli_str_concat
#define icli_str_str                    vtss_icli_str_str
#define icli_ipv4_to_str                vtss_icli_ipv4_to_str
#define icli_ipv4_class_get             vtss_icli_ipv4_class_get
#define icli_ipv4_prefix_to_netmask     vtss_icli_ipv4_prefix_to_netmask
#define icli_ipv4_netmask_to_prefix     vtss_icli_ipv4_netmask_to_prefix
#define icli_ipv6_netmask_to_prefix     vtss_icli_ipv6_netmask_to_prefix
#define icli_ipv6_to_str                vtss_icli_ipv6_to_str
#define icli_mac_to_str                 vtss_icli_mac_to_str
#define icli_str_to_int                 vtss_icli_str_to_int
#define icli_str_prefix                 vtss_icli_str_prefix
#define icli_switch_port_range_to_str   vtss_icli_switch_port_range_to_str
#define icli_stack_port_range_to_str    vtss_icli_stack_port_range_to_str
#define icli_unsigned_range_to_str      vtss_icli_unsigned_range_to_str
#define icli_signed_range_to_str        vtss_icli_signed_range_to_str
#define icli_range_to_str               vtss_icli_range_to_str
#define icli_oui_to_str                 vtss_icli_oui_to_str
#define icli_clock_id_to_str            vtss_icli_clock_id_to_str
#define icli_vcap_vr_to_str             vtss_icli_vcap_vr_to_str
#define icli_hexval_to_str              vtss_icli_hexval_to_str
#define icli_file_name_check            vtss_icli_file_name_check

/*
==============================================================================

    Public Function

==============================================================================
*/
/*
    Initialize the ICLI module

    return
       VTSS_RC_OK.
*/
mesa_rc icli_init(
    IN vtss_init_data_t     *data
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
    IN i32   session_id
);

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
);

/*
    Start thread to handle the session

    INPUT
        open_data : data for session open

    OUTPUT
        session_id : ID of session opened successfully

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_begin(
    IN icli_session_open_data_t   *open_data,
    IN u32                         session_id
);

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
);

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
);

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
void icli_session_all_close(icli_session_way_t way);

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
u32 icli_session_max_get(void);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
    IN  ...) __attribute__((format (__printf__, 2, 3)));

/*
    output long string to a specific session,
    where the length of long string is larger than (ICLI_STR_MAX_LEN),
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
);

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
    IN  ...) __attribute__((format (__printf__, 1, 2)));

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
);

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
);

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
);

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
);

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
);

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
);

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
);

#if 1 /* CP, 2012/08/29 09:25, history max count is configurable */
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
);

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
);
#endif /* CP, 2012/08/29 09:25, history max count is configurable */

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

#if 1 /* CP, 2012/08/31 07:51, enable/disable banner per line */
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
);

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
);
#endif

#if 1 /* CP, 2012/08/31 09:31, location and default privilege */
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
);

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
);
#endif

#if 1 /* CP, 2012/09/04 16:46, session user name */
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
);
#endif

#if 1 /* CP, 2012/09/11 14:08, Use thread information to get session ID */
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
    IN  ...) __attribute__((format (__printf__, 1, 2)));

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
);
#endif /* CP, 2012/09/11 14:08, Use thread information to get session ID */

#if 1 /* CP, 2012/10/16 17:02, ICLI_RC_CHECK */
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
);

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
);
#endif

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
);

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
);

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
);

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
);

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
);

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
);
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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
char *icli_port_type_get_name(
    IN  u16    type
);

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
const char *icli_port_type_get_short_name(
    IN  icli_port_type_t    type
);

/*
    reset port setting

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        n/a

    COMMENT
        n/a
*/
void icli_port_range_reset( void );

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);
#endif

const char *icli_prompt_get(void);
const char *icli_prompt_default_get(void);
BOOL icli_prompt_is_default(const char *prompt);
i32 icli_prompt_set(const char *prompt);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

mesa_rc icli_multiline_data_get(u32 session_id, icli_session_multiline_data_t *const data);

mesa_rc icli_multiline_data_set(u32 session_id, const icli_session_multiline_data_t *data);

void icli_multiline_data_clear(u32 session_id);

/*
 * Check if an already given parameter value is a URL with the given protocol.
 *
 * Return TRUE if a parameter of type ICLI_VARIABLE_URL or ICLI_VARIABLE_URL_FILE
 * has a value with a protocol that equals req_proto.
 *
 * Return FALSE if no such parameter is found.
 *
 * This function is intended to be used in a RUNTIME ICLI_ASK_PRESENT situation
 * where the presence of a parameter depends on the protocol given earlier.
 */
BOOL icli_has_url_with_proto(u32 session_id, icli_url_proto_t req_proto);

BOOL icli_range_vtss_ports(u32 session_id,
                           icli_runtime_ask_t ask,
                           icli_runtime_t *runtime);

//****************************************************************************
#ifdef __cplusplus
}
#endif

//****************************************************************************

#endif //__ICLI_API_H__
