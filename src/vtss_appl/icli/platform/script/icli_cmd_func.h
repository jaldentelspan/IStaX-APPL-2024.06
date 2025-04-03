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
******************************************************************************

    Revision history
    > CP.Wang, 2012/09/27 12:19
        - create

******************************************************************************
*/
#ifndef __ICLI_CMD_FUNC_H__
#define __ICLI_CMD_FUNC_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
******************************************************************************

    Public Function

******************************************************************************
*/
BOOL icli_config_go_to_exec_mode(
    IN u32 session_id
);

void icli_config_help_print(u32 session_id);
i32 icli_config_exec_do(u32 session_id, char *command);

BOOL icli_config_user_str_get(
    IN  u32     session_id,
    IN  i32     max_len,
    OUT char    *user_str

);

//****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //__ICLI_CMD_FUNC_H__
