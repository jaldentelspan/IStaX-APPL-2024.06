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
    > CP.Wang, 05/29/2013 11:38
        - create

==============================================================================
*/
#ifndef __VTSS_ICLI_PARSING_H__
#define __VTSS_ICLI_PARSING_H__
//****************************************************************************

/*
==============================================================================

    Include File

==============================================================================
*/

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

    Public Function

==============================================================================
*/
/*
    initialization

    input -
        n/a
    output -
        n/a
    return -
        icli_rc_t
    comment -
        n/a
*/
i32 vtss_icli_parsing_init(
    void
);

/*
    build parsing tree

    input -
        cmd_register : command data, string with word ID
    output -
        n/a
    return -
        icli_rc_t
    comment -
        n/a
*/
i32 vtss_icli_parsing_build(
    IN  icli_cmd_register_t     *cmd_register
);

/*
    get parsing tree according to the mode

    input -
        mode : command mode
    output -
        n/a
    return -
        not NULL : successful, tree
        NULL     : failed
    comment -
        n/a
*/
icli_parsing_node_t *vtss_icli_parsing_tree_get(
    IN  icli_cmd_mode_t         mode
);

/*
    check if head is the random optiona head of node

    INPUT
        head : random optional head
        node : the node to be checked

    OUTPUT
        n/a

    RETURN
        TRUE  - yes, it is
        FALSE - no

    COMMENT
        n/a
*/
BOOL vtss_icli_parsing_random_head(
    IN  icli_parsing_node_t     *head,
    IN  icli_parsing_node_t     *node,
    OUT u32                     *optional_level
);

//****************************************************************************
#endif //__VTSS_ICLI_PARSING_H__

