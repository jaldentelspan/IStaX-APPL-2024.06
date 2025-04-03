/*

 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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
    > CP.Wang, 05/29/2013 14:18
        - create

==============================================================================
*/
#ifndef __VTSS_ICLI_PLATFORM_H__
#define __VTSS_ICLI_PLATFORM_H__
//****************************************************************************
/*
==============================================================================

    Include File

==============================================================================
*/
#include "icli_porting_trace.h"
#include "icli_os.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
==============================================================================

    Constant

==============================================================================
*/

/*
==============================================================================

    Type

==============================================================================
*/
typedef struct {
    icli_cmd_mode_t     mode;
    const char          *str;
    const char          *desc;
    const char          *prompt;
    char                *name;
    const char          *cmd;
    BOOL                b_mode_var;
} icli_cmd_mode_info_t;

typedef struct {
    BOOL    b_primary;
    u32     usid;
} icli_switch_info_t;

/*
==============================================================================

    Macro Definition

==============================================================================
*/

/*
==============================================================================

    Public Function

==============================================================================
*/
/*
    calculate MD5
*/
void icli_platform_hmac_md5(
    IN  const unsigned char *key,
    IN  size_t              key_len,
    IN  const unsigned char *data,
    IN  size_t              data_len,
    OUT unsigned char       *mac
);

/*
    cursor up
*/
void icli_platform_cursor_up(
    IN icli_session_handle_t    *handle
);

/*
    cursor down
*/
void icli_platform_cursor_down(
    IN icli_session_handle_t    *handle
);

/*
    cursor forward
*/
void icli_platform_cursor_forward(
    IN icli_session_handle_t    *handle
);

/*
    cursor backward
*/
void icli_platform_cursor_backward(
    IN icli_session_handle_t    *handle
);

/*
    cursor go to ( current_x + offser_x, current_y + offset_y )
*/
void icli_platform_cursor_offset(
    IN icli_session_handle_t    *handle,
    IN i32                      offset_x,
    IN i32                      offset_y
);

/*
    cursor backward and delete that char
    update cursor_pos, but not cmd_pos and cmd_len
*/
void icli_platform_cursor_backspace(
    IN  icli_session_handle_t   *handle
);

/*
    get switch information
*/
void icli_platform_switch_info_get(
    OUT icli_switch_info_t  *switch_info
);

/*
    check if auth callback exists

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        TRUE  - yes, the callback exists
        FALSE - no, it is NULL

*/
BOOL icli_platform_has_user_auth(
    void
);

/*
    authenticate user

    INPUT
        session_way  : way to access session
        username     : user name
        password     : password for the user

    OUTPUT
        privilege    : the privilege level for the authenticated user
        agent_id     : agent id assigned to the session

    RETURN
        icli_rc_t
*/
i32 icli_platform_user_auth(
    IN  icli_session_way_t  session_way,
    IN  char                *hostname,
    IN  char                *username,
    IN  char                *password,
    OUT u32                 *privilege,
    OUT u32                 *agent_id
);

/*
    authorize command

    INPUT
        handle    : session handle
        command   : full command string
        cmd_priv  : highest command privilege
        b_execute : TRUE if <enter>, FALSE if <tab>, '?' or '??'

    OUTPUT
        n/a

    RETURN
        icli_rc_t
*/
i32 icli_platform_cmd_authorize(
    IN icli_session_handle_t    *handle,
    IN char                     *command,
    IN u32                      cmd_priv,
    IN BOOL                     b_execute
);

/*
    user logout

    INPUT
        handle    : session handle

    OUTPUT
        n/a

    RETURN
        icli_rc_t
*/
i32 icli_platform_user_logout(
    IN icli_session_handle_t    *handle
);

u16 icli_platform_isid2usid(
    IN u16  isid
);

u16 icli_platform_iport2uport(
    IN u16  iport
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
u16 icli_platform_usid2switchid(
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
u16 icli_platform_isid2switchid(
    IN u16  isid
);

/*
    get mode prompt

    INPUT
        mode : command mode

    OUTPUT
        n/a

    RETURN
        mode prompt
*/
const char *icli_platform_mode_prompt_get(
    IN  icli_cmd_mode_t     mode
);

/*
    get mode name

    INPUT
        mode : command mode

    OUTPUT
        n/a

    RETURN
        mode name
*/
char *icli_platform_mode_name_get(
    IN  icli_cmd_mode_t     mode
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
icli_cmd_mode_t icli_platform_mode_get_by_name(
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
*/
const char *icli_platform_mode_str_get(
    IN  icli_cmd_mode_t     mode
);

/*
    configure current port ranges including stacking into ICLI

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        TRUE  - successful
        FALSE - failed

    COMMENT
        n/a
*/
BOOL icli_platform_port_range(
    void
);

#ifdef __cplusplus
}
#endif

//****************************************************************************
#endif //__VTSS_ICLI_PLATFORM_H__

