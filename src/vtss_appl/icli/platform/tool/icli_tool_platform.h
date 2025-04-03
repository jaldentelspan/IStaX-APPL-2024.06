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
==============================================================================

    Revision history
    > CP.Wang, 05/29/2013 14:18
        - create

==============================================================================
*/
#ifndef __ICLI_TOOL_PLATFORM_H__
#define __ICLI_TOOL_PLATFORM_H__
//****************************************************************************
/*
==============================================================================

    Include File

==============================================================================
*/
#include "vtss_icli.h"
#include "icli_porting_trace.h"

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
    get command mode info by string

    INPUT
        mode_str : string of mode

    OUTPUT
        n/a

    RETURN
        icli_cmd_mode_info_t * : successful
        NULL                        : failed

    COMMENT
        n/a
*/
const icli_cmd_mode_info_t *icli_platform_cmd_mode_info_get_by_str(
    IN  char    *mode_str
);

/*
    get command mode info by command mode

    INPUT
        mode : command mode

    OUTPUT
        n/a

    RETURN
        icli_cmd_mode_info_t * : successful
        NULL                        : failed

    COMMENT
        n/a
*/
const icli_cmd_mode_info_t *icli_platform_cmd_mode_info_get_by_mode(
    IN  icli_cmd_mode_t     mode
);

/*
    get privilege by string

    INPUT
        priv_str : string of privilege

    OUTPUT
        n/a

    RETURN
        icli_privilege_t   : successful
        ICLI_PRIVILEGE_MAX : failed

    COMMENT
        n/a
*/
icli_privilege_t icli_platform_privilege_get_by_str(
    IN char     *priv_str
);

#ifdef __cplusplus
}
#endif
//****************************************************************************
#endif //__ICLI_TOOL_PLATFORM_H__

