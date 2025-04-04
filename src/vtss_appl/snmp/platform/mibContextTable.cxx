/*
 *
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 *
 */

// Note: This file originally auto-generated by mib2c using vtss_mib2c_ucd_snmp.conf v3.40
//       Revise the "FIXME" parts to make it as a completed code.

#include <main.h>
#include "vtss_os_wrapper_snmp.h"
#include "vtss_snmp_api.h"
#include "mibContextTable.h"
#include "vtss_avl_tree_api.h"

/* Trace module */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID        VTSS_MODULE_ID_SNMP
#define VTSS_TRACE_GRP_DEFAULT      0

/* mutex for global protection */
#include "critd_api.h"
static critd_t mibContextTable_global_critd_mutex;

#define MIBCONTEXTTABLE_CRIT_ENTER() critd_enter(&mibContextTable_global_critd_mutex, __FILE__, __LINE__)
#define MIBCONTEXTTABLE_CRIT_EXIT()  critd_exit( &mibContextTable_global_critd_mutex, __FILE__, __LINE__)

//static mibContextTable_entry_t mibContextTable[MIBCONTEXTTABLE_MAX_ENTRY_CNT];
static BOOL mibContextTable_global_init_flag = FALSE;

/* AVL Tree utility library */
static i32 MIBCONTEXTTABLE_entry_compare_func(void *elm1, void *elm2);
static i32 MIBPREFIXTABLE_entry_compare_func(void *elm1, void *elm2);

// Create ACL tree to maintain the same database(mibContextTable) with different entry key
VTSS_AVL_TREE(MIBCONTEXTTABLE_global_table, "SNMP_MIBCONTEXTTABLE", VTSS_MODULE_ID_SNMP, MIBCONTEXTTABLE_entry_compare_func, MIBCONTEXTTABLE_MAX_ENTRY_CNT)

// Create ACL tree to maintain the mibPrefixTable) with different entry key
VTSS_AVL_TREE(MIBPREFIXTABLE_global_table, "SNMP_MIBPREFIXTABLE", VTSS_MODULE_ID_SNMP, MIBPREFIXTABLE_entry_compare_func, MIBCONTEXTTABLE_MAX_ENTRY_CNT)

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_SNMP    // VTSS_MALLOC()


/******************************************************************************/
//
// Local functions
//
/******************************************************************************/

static i32 MIBCONTEXTTABLE_entry_compare_func(void *element1, void *element2)
{
    mibContextTable_entry_t *elm1 = (mibContextTable_entry_t *)element1;
    mibContextTable_entry_t *elm2 = (mibContextTable_entry_t *)element2;
    int                 strcmp_rc, oid_cmp_rc;

    if ((strcmp_rc = strcmp(elm1->mib_name, elm2->mib_name)) > 0) {
        return 1;
    } else if (strcmp_rc < 0) {
        return -1;
    } else if ((oid_cmp_rc = snmp_oid_compare(elm1->oid_, elm1->oid_len, elm2->oid_, elm2->oid_len)) > 0) {
        return 1;
    } else if (oid_cmp_rc < 0) {
        return -1;
    } else {
        return 0;
    }
}

static i32 MIBPREFIXTABLE_entry_compare_func(void *element1, void *element2)
{
    mibContextTable_entry_t *elm1 = (mibContextTable_entry_t *)element1;
    mibContextTable_entry_t *elm2 = (mibContextTable_entry_t *)element2;
    int                 rc;

    if ((rc = strlen(elm1->descr) - strlen(elm2->descr)) > 0) {
        return 1;
    } else if (rc < 0) {
        return -1;
    } else if ((rc = strcmp(elm1->descr, elm2->descr)) > 0) {
        return 1;
    } else if (rc < 0) {
        return -1;
    } else {
        return 0;
    }
}


/******************************************************************************/
// Initial function
/******************************************************************************/
/**
  * \brief Initialize mibContextTable semaphore for critical regions.
  **/
void mibContextTable_init(void)
{
    T_D("enter");

    if (mibContextTable_global_init_flag) {
        return;
    }
    mibContextTable_global_init_flag = TRUE;

    /* Create semaphore for critical regions */
    critd_init(&mibContextTable_global_critd_mutex, "mibContextTable", VTSS_MODULE_ID_SNMP, CRITD_TYPE_MUTEX);

    MIBCONTEXTTABLE_CRIT_ENTER();

    if (vtss_avl_tree_init(&MIBCONTEXTTABLE_global_table) != TRUE) {
        T_E("mibContextTable - Initialize fail");
    }

    if (vtss_avl_tree_init(&MIBPREFIXTABLE_global_table) != TRUE) {
        T_E("mibPrefixTable - Initialize fail");
    }

    MIBCONTEXTTABLE_CRIT_EXIT();

    T_D("exit");
}


/******************************************************************************/
//
// mibContextTable entry register
//
/******************************************************************************/
/**
  * \brief Register table entry of mibContextTable
  *
  * \param oidin  [IN]: The MIB OID which will register to mibContextTable
  * \param oidlen [IN]: The OID length of input parameter "oidin"
  * \param descr  [IN]: The desciption of MIB node. Format: <MIB_File_Name> : <Scalar_or_Table_Name>
  **/
void mibContextTable_register(oid *oidin, size_t oidlen, const char *descr)
{
    mibContextTable_entry_t tmp_entry, *entry_p = &tmp_entry;
    char                    mib_name[MIBCONTEXTTABLE_STR_LEN_MAX + 1];
    const char              *delimit = ":", *ch_p;

    /* Check input parameters */
    if (oidlen > MIBCONTEXTTABLE_OID_LEN_MAX ||
        strlen(descr) >= MIBCONTEXTTABLE_STR_LEN_MAX) {
        T_E("mibContextTable - Input parameter out of valid range");
        return;
    }

    memcpy(entry_p->oid_, oidin, sizeof(oid) * oidlen);
    entry_p->oid_len = oidlen;
    strcpy(mib_name, descr);
    char *saveptr; // Local strtok_r() context
    if ((ch_p = strtok_r(mib_name, delimit, &saveptr)) != NULL) {
        while (isspace(*ch_p)) {
            ch_p++;
        }
        strcpy(entry_p->mib_name, ch_p);
    }

    MIBCONTEXTTABLE_CRIT_ENTER();
    if (vtss_avl_tree_get(&MIBCONTEXTTABLE_global_table, (void **) &entry_p, VTSS_AVL_TREE_GET) == TRUE) {
        // Already registed
        MIBCONTEXTTABLE_CRIT_EXIT();
        return;
    }
    MIBCONTEXTTABLE_CRIT_EXIT();

    /* Allocate memory for table entry */
    if ((entry_p = (mibContextTable_entry_t *)VTSS_MALLOC(sizeof(*entry_p))) == NULL) {
        T_E("mibContextTable - Memory allocated fail");
        return;
    }

    /* Fill table entry elements */
    memset(entry_p, 0, sizeof(*entry_p));
    memcpy(entry_p->oid_, oidin, sizeof(oid) * oidlen);
    entry_p->oid_len = oidlen;
    if ((ch_p = strtok_r(mib_name, delimit, &saveptr)) != NULL) {
        strcpy(entry_p->mib_name, ch_p);
        ch_p += strlen(ch_p) + 1;
        while (isspace(*ch_p)) {
            ch_p++;
        }
        strcpy(entry_p->descr, ch_p);
        entry_p->descr_len = strlen(entry_p->descr);
    }

    /* Add to AVL tree */
    MIBCONTEXTTABLE_CRIT_ENTER();
    if (vtss_avl_tree_add(&MIBCONTEXTTABLE_global_table, entry_p) != TRUE) {
        VTSS_FREE(entry_p);
        entry_p = NULL;
        T_W("Register SNMP supported MIBs failed");
    }

    if ( entry_p && vtss_avl_tree_add(&MIBPREFIXTABLE_global_table, entry_p) != TRUE) {
        T_E("mibPrefixTable is not synced with mibContextTable. "
            "Check the syntax of description -> <MIB_Name> %s  : <Supported_OID_Name> %s", entry_p->mib_name, entry_p->descr);
    }

    MIBCONTEXTTABLE_CRIT_EXIT();
}


/******************************************************************************/
//
// Table entry access functions
//
/******************************************************************************/
/**
  * \brief Getnext table entry by MIB name of mibContextTableEntry
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get/getnext the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int
mibContextTableEntry_getnext_by_mib_name(mibContextTable_entry_t *table_entry)
{
    int                 rc = -1;
    mibContextTable_entry_t  *entry_p = table_entry;

    T_D("enter");

    MIBCONTEXTTABLE_CRIT_ENTER();
    if (vtss_avl_tree_get(&MIBCONTEXTTABLE_global_table, (void **) &entry_p, VTSS_AVL_TREE_GET_NEXT) == TRUE) {
        rc = 0;
        *table_entry = *entry_p;
    }
    MIBCONTEXTTABLE_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/**
  * \brief Getnext table entry by description of mibContextTableEntry
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get/getnext the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int
mibPrefixTableEntry_getnext_by_descr(mibContextTable_entry_t *table_entry)
{
    int                 rc = -1;
    mibContextTable_entry_t  *entry_p = table_entry;

    T_D("enter");

    MIBCONTEXTTABLE_CRIT_ENTER();
    if (vtss_avl_tree_get(&MIBPREFIXTABLE_global_table, (void **) &entry_p, VTSS_AVL_TREE_GET_NEXT) == TRUE) {
        rc = 0;
        *table_entry = *entry_p;
    }
    MIBCONTEXTTABLE_CRIT_EXIT();

    T_D("exit");
    return rc;

}

/**
  * \brief Get table entry by description of mibContextTableEntry
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int
mibPrefixTableEntry_get_by_descr(mibContextTable_entry_t *table_entry)
{
    int                 rc = -1;
    mibContextTable_entry_t  *entry_p = table_entry;

    T_D("enter");

    MIBCONTEXTTABLE_CRIT_ENTER();
    if (vtss_avl_tree_get(&MIBPREFIXTABLE_global_table, (void **) &entry_p, VTSS_AVL_TREE_GET) == TRUE) {
        rc = 0;
        *table_entry = *entry_p;
    }
    MIBCONTEXTTABLE_CRIT_EXIT();

    T_D("exit");
    return rc;

}

