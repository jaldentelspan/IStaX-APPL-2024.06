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
    > CP.Wang, 2012/08/09 13:53
        - revise

******************************************************************************
*/
/*
******************************************************************************

    Include File

******************************************************************************
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vtss_free_list_api.h"
#include "vtss_free_list.h"

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
static u32 _node_index_get(
    IN vtss_free_list_t     *flist,
    IN void                 *user_data
)
{
    u64     i, j;
    u32     r;

    i = (u64)(user_data);
    j = (u64)(flist->user_data);
    r = ( i - j ) / flist->user_data_size;
    return r;
}

/*
******************************************************************************

    Public Function

******************************************************************************
*/
/*
    initialize free list

    INPUT
        flist - free list

    OUTPUT
        n/a

    RETURN
        TRUE  - successful
        FALSE - failed
*/
BOOL vtss_free_list_init(
    IN vtss_free_list_t     *flist
)
{
    char                *c;
    u32                 i;
    vtss_free_node_t    *node;
    u32                 size;
    u32                 max_node_cnt;

    if ( flist == NULL ) {
        _FLIST_T_E("flist == NULL\n");
        return FALSE;
    }

    if ( flist->user_data == NULL ) {
        _FLIST_T_E("flist->user_data == NULL\n");
        return FALSE;
    }

    if ( flist->user_data_size == 0 ) {
        _FLIST_T_E("flist->user_data_size == 0\n");
        return FALSE;
    }

    if ( flist->max_node_cnt == 0 ) {
        _FLIST_T_E("flist->max_node_cnt == 0\n");
        return FALSE;
    }

    if ( flist->free_nodes == NULL ) {
        _FLIST_T_E("flist->free_nodes == NULL\n");
        return FALSE;
    }

    /* chain free node and link user data */
    size = flist->user_data_size;
    c = (char *)flist->user_data;
    max_node_cnt = flist->max_node_cnt - 1;
    node = flist->free_nodes;
    for ( i = 0; i < max_node_cnt; i++, node++ ) {
        node->user_data = c + (i * size);
        node->next = node + 1;
    }
    node->next = NULL;
    node->user_data = c + (i * size);

    /* get free_list */
    flist->free_list  = flist->free_nodes;
    flist->free_count = flist->max_node_cnt;

    return TRUE;
}

/*
    allocate a free memory from user data

    INPUT
        flist - free list

    OUTPUT
        n/a

    RETURN
        not NULL - available user_data
        NULL     - memory insufficient
*/
void *vtss_free_list_malloc(
    IN vtss_free_list_t     *flist
)
{
    vtss_free_node_t    *node;

    if ( flist == NULL ) {
        _FLIST_T_E("flist == NULL\n");
        return NULL;
    }

    if ( flist->free_list == NULL ) {
        return NULL;
    }

    /* catch from free list */
    node = flist->free_list;
    flist->free_list = node->next;
    (flist->free_count)--;

    /* update counter */
    return node->user_data;
}

/*
    free user data back

    INPUT
        flist     - free list
        user_data - the memory for free

    OUTPUT
        n/a

    RETURN
        n/a
*/
void vtss_free_list_free(
    IN vtss_free_list_t     *flist,
    IN void                 *user_data
)
{
    u32                 i;
    vtss_free_node_t    *node;

    if ( flist == NULL ) {
        _FLIST_T_E("flist == NULL\n");
        return;
    }

    if ( user_data == NULL ) {
        _FLIST_T_E("user_data == NULL\n");
        return;
    }

    // get free node index
    i = _node_index_get(flist, user_data);
    node = flist->free_nodes + i;

    // consistency check
    if ( node->user_data != user_data ) {
        _FLIST_T_E("user_data in invalid\n");
        return;
    }

    /* put to free list */
    node->next = flist->free_list;
    flist->free_list = node;

    /* update counter */
    (flist->free_count)++;
}

