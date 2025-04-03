/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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
    > CP.Wang, 2012/09/27 12:19
        - create

==============================================================================
*/

/*
==============================================================================

    Include File

==============================================================================
*/
#include <stdlib.h>
#include "icli_api.h"
#include "icli_porting_trace.h"
#include "icli_cmd_func.h"

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
==============================================================================

    Static Function

==============================================================================
*/

/*
==============================================================================

    Public Function

==============================================================================
*/
BOOL icli_config_go_to_exec_mode(
    IN u32 session_id
)
{
    icli_cmd_mode_t     current_mode;
    u32                 i;

    for ( i = 0; i < ICLI_MODE_MAX_LEVEL; ++i ) {
        /* get current mode */
        if ( ICLI_MODE_GET(&current_mode) != ICLI_RC_OK ) {
            return FALSE;
        }

        /* check current mode */
        if ( current_mode == ICLI_CMD_MODE_EXEC ) {
            return TRUE;
        }

        /* exit current mode */
        (void)ICLI_MODE_EXIT();
    }

    return FALSE;
}

void icli_config_help_print(u32 session_id)
{
    ICLI_PRINTF("Help may be requested at any point in a command by entering\n");
    ICLI_PRINTF("a question mark '?'.  If nothing matches, the help list will\n");
    ICLI_PRINTF("be empty and you must backup until entering a '?' shows the\n");
    ICLI_PRINTF("available options.\n");
    ICLI_PRINTF("Two styles of help are provided:\n");
    ICLI_PRINTF("1. Full help is available when you are ready to enter a\n");
    ICLI_PRINTF("   command argument (e.g. 'show ?') and describes each possible\n");
    ICLI_PRINTF("   argument.\n");
    ICLI_PRINTF("2. Partial help is provided when an abbreviated argument is entered\n");
    ICLI_PRINTF("   and you want to know what arguments match the input\n");
    ICLI_PRINTF("   (e.g. 'show pr?'.)\n");
    ICLI_PRINTF("\n");
}

i32 icli_config_exec_do(u32 session_id, char *command)
{
    if (ICLI_MODE_ENTER(ICLI_CMD_MODE_EXEC) < 0) {
        ICLI_PRINTF("%% Fail to enter EXEC mode.\n\n");
        return ICLI_RC_ERROR;
    }

    if (ICLI_CMD_EXEC(command, TRUE) != ICLI_RC_OK) {
        ICLI_PRINTF("%% Fail to execute command in EXEC mode.\n\n");
    }

    if (ICLI_MODE_EXIT() < 0) {
        ICLI_PRINTF("%% Fail to exit EXEC mode.\n\n");
        return ICLI_RC_ERROR;
    }

    return ICLI_RC_OK;
}

BOOL icli_config_user_str_get(
    IN  u32     session_id,
    IN  i32     max_len,
    OUT char    *user_str

)
{
    char        *str;
    char        *c;
    i32         len;
    icli_rc_t   rc;
    BOOL        b_end;
    BOOL        b_loop = TRUE;

    if ( user_str == NULL ) {
        return FALSE;
    }

    // find the end char or EoS
    for ( c = user_str + 1; *c != *user_str && *c != 0; ++c ) {
        ;
    }

    // check if end char
    b_end = FALSE;
    if ( *c == *user_str ) {
        b_end = TRUE;
        *c = 0;
    }

    // check length
    // -1 is for the starting delimiter
    len = vtss_icli_str_len(user_str) - 1;
    if ( len >= max_len ) {
        user_str[max_len + 1] = 0;
        return TRUE;
    }

    // end delimiter is present
    if ( b_end ) {
        return TRUE;
    }

    // allocate memory
    str = (char *)icli_malloc(max_len + 1);
    if ( str == NULL ) {
        T_E("memory insufficient\n");
        return FALSE;
    }

    // prepare for next input
    *c     = '\n';
    *(c + 1) = 0;

    ICLI_PRINTF("Enter TEXT message.  End with the character '%c'.\n", *user_str);
    while ( b_loop ) {
        memset(str, 0, max_len + 1);
        len = max_len;
        rc = (icli_rc_t)ICLI_USR_STR_GET(ICLI_USR_INPUT_TYPE_NORMAL, str, &len, NULL);
        switch ( rc ) {
        case ICLI_RC_OK:
            // find the end char or EoS
            for ( c = str; *c != *user_str && *c != 0; ++c ) {
                ;
            }

            // check if end char
            b_end = FALSE;
            if ( *c == *user_str ) {
                b_end = TRUE;
                *c = 0;
            }

            // check length
            // -1 is for the starting delimiter
            len = vtss_icli_str_len(user_str) - 1 + vtss_icli_str_len(str);
            if ( len >= max_len ) {
                if ( vtss_icli_str_len(user_str) ) {
                    len = max_len - vtss_icli_str_len(user_str) + 1;
                } else {
                    len = max_len;
                }
                *(str + len) = 0;
                (void)vtss_icli_str_concat(user_str, str);
                // free memory
                icli_free(str);
                return TRUE;
            }

            // concat string
            (void)vtss_icli_str_concat(user_str, str);

            // end delimiter is present
            if ( b_end ) {
                // free memory
                icli_free(str);
                return TRUE;
            }

            // prepare for next input
            len = vtss_icli_str_len(user_str);
            user_str[len]   = '\n';
            user_str[len + 1] = 0;
            break;

        case ICLI_RC_ERR_EXPIRED:
            ICLI_PRINTF("\n%% timeout expired!\n");
            // free memory
            icli_free(str);
            return FALSE;

        default:
            ICLI_PRINTF("%% Fail to get from user input\n");
            // free memory
            icli_free(str);
            return FALSE;
        }
    }
    // free memory
    icli_free(str);
    return FALSE;
}
