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
    > CP.Wang, 2012/07/20 12:54
        - Create

******************************************************************************
*/
/*******************************************************
*
* Project: avl-tree
*
* Description: Generic implementation of AVL tree
*
*******************************************************/
#ifndef __VTSS_AVL_TREE_H__
#define __VTSS_AVL_TREE_H__

/*
******************************************************************************

    Include File

******************************************************************************
*/

/*
******************************************************************************

    Constant and Macro

******************************************************************************
*/
#define LCHILD              0
#define RCHILD              1

/* For debugging only */
// #define DEBUG_AVL_TREE

#if defined(DEBUG_AVL_TREE)
#define _AVLT_DEBUG(args)   { DBG_PRINTF args; }
#if defined(WIN32)
#define DBG_PRINTF          printf
#define _AVLT_SNPRINTF      _snprintf
#else
#error "Unsupported OS"
#endif /* defined(VTSS_OPSYS_WIN32) */
#else
#define _AVLT_DEBUG(args)
#endif /* defined(DEBUG_AVL_TREE) */

#define VTSS_AVL_TREE_T_E(...)  _AVLT_DEBUG(("Error: %s, %s, %d, ", __FILE__, __FUNCTION__, __LINE__)); \
                                _AVLT_DEBUG((__VA_ARGS__));

/*
******************************************************************************

    Type Definition

******************************************************************************
*/
typedef enum {
    VTSS_AVL_TREE_NODE_UNKNOWN,
    VTSS_AVL_TREE_NODE_ROOT,
    VTSS_AVL_TREE_NODE_LCHILD,
    VTSS_AVL_TREE_NODE_RCHILD
} vtss_avl_tree_node_type_t;

#define SET_ROOT(ptreenode) { \
    ptreenode->nodetype = VTSS_AVL_TREE_NODE_ROOT; \
}

#define SET_LCHILD(ptreenode) { \
    ptreenode->nodetype = VTSS_AVL_TREE_NODE_LCHILD; \
}

#define SET_RCHILD(ptreenode) { \
    ptreenode->nodetype = VTSS_AVL_TREE_NODE_RCHILD; \
}

#define IS_UNKNOWN(ptreenode)       (ptreenode->nodetype == VTSS_AVL_TREE_NODE_UNKNOWN)
#define IS_ROOT(ptreenode)          (ptreenode->nodetype == VTSS_AVL_TREE_NODE_ROOT)
#define IS_LCHILD(ptreenode)        (ptreenode->nodetype == VTSS_AVL_TREE_NODE_LCHILD)
#define IS_RCHILD(ptreenode)        (ptreenode->nodetype == VTSS_AVL_TREE_NODE_RCHILD)
#define NO_LCHILD(ptreenode)        (ptreenode->pchild[LCHILD] == NULL)
#define NO_RCHILD(ptreenode)        (ptreenode->pchild[RCHILD] == NULL)
#define HAS_LCHILD(ptreenode)       (ptreenode->pchild[LCHILD] != NULL)
#define HAS_RCHILD(ptreenode)       (ptreenode->pchild[RCHILD] != NULL)
#define GET_LCHILD(ptreenode)       (ptreenode->pchild[LCHILD])
#define GET_RCHILD(ptreenode)       (ptreenode->pchild[RCHILD])
#define GET_PARENT(ptreenode)       (ptreenode->pparent)

#define LCHILD_HEIGHT(ptreenode)    (ptreenode->pchild[LCHILD] ? ptreenode->pchild[LCHILD]->height : 0)
#define RCHILD_HEIGHT(ptreenode)    (ptreenode->pchild[RCHILD] ? ptreenode->pchild[RCHILD]->height : 0)
#define NO_CHILD(ptreenode)         (NO_LCHILD(ptreenode) && NO_RCHILD(ptreenode))

#define NODE_NEXT(n)                ((n)->pparent)

//****************************************************************************
#endif /* __VTSS_AVL_TREE_H__ */
