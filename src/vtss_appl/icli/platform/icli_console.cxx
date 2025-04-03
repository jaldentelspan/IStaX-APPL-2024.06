/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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
    > CP.Wang, 2011/04/14 10:00
        - create

******************************************************************************
*/

/*
******************************************************************************

    Include File

******************************************************************************
*/
#include "icli_api.h"

#ifndef VTSS_SW_OPTION_CLI
#ifdef VTSS_SW_OPTION_CONSOLE
#include <string.h>
#include "console_api.h"
#endif
#endif /* VTSS_SW_OPTION_CLI */

/*
******************************************************************************

    Constant and Macro

******************************************************************************
*/

/*
******************************************************************************

    Type Definition

******************************************************************************
*/

/*
******************************************************************************

    Static Variable

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/

/*
******************************************************************************

    Public Function

******************************************************************************
*/
BOOL icli_console_start(void)
{
#ifndef VTSS_SW_OPTION_CLI
#ifdef VTSS_SW_OPTION_CONSOLE
    i32                         rc;
    u32                         session_id;
    icli_session_open_data_t    open_data;

    /* open a Console session */
    if ( console_session_open("ICLI", &session_id) == FALSE ) {
        T_E("fail to open a Console session\n");
        return FALSE;
    }

    /* Open ICLI session */
    memset(&open_data, 0, sizeof(open_data));

    open_data.name      = "CONSOLE";
    open_data.way       = ICLI_SESSION_WAY_CONSOLE;
    open_data.app_id    = session_id;

    /* I/O callback */
    open_data.char_get  = &console_getc;
    open_data.char_put  = &console_putc;
    open_data.str_put   = &console_puts;

    /* APP session callback */
    open_data.app_init  = NULL;
    open_data.app_close = NULL;

    rc = icli_session_open(&open_data, &session_id);
    if ( rc != ICLI_RC_OK ) {
        T_E("fail to open a session for console\n");
        return FALSE;
    }
#endif
#endif /* VTSS_SW_OPTION_CLI */
    return TRUE;
}

