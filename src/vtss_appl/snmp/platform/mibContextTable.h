/*
 *
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
 *
 */

#ifndef _MIBCONTEXTTABLE_H_
#define _MIBCONTEXTTABLE_H_


#include "vtss/basics/expose/snmp/vtss_oid.hxx"  //oid

#define MIBCONTEXTTABLE_STR_LEN_MAX    63
#define MIBCONTEXTTABLE_OID_LEN_MAX    32


#define MIBCONTEXTTABLE_MAX_ENTRY_CNT    256


/******************************************************************************/
//
// Data structure declarations
//
/******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// The table entry data structure for mibContextTable
typedef struct {
    // Entry keys
    char            mib_name[MIBCONTEXTTABLE_STR_LEN_MAX + 1];
    vtss_oid        oid_[MIBCONTEXTTABLE_OID_LEN_MAX + 1];
    size_t          oid_len;

    // Entry columns
    char            descr[MIBCONTEXTTABLE_STR_LEN_MAX + 1];
    size_t          descr_len;
} mibContextTable_entry_t;


/******************************************************************************/
//
// Initial function
//
/******************************************************************************/
/**
  * \brief Initialize mibContextTable semaphore for critical regions.
  **/
void            mibContextTable_init(void);


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
void mibContextTable_register(vtss_oid *oidin, size_t oidlen, const char *descr);


/******************************************************************************/
//
// Table entry access function declarations
//
/******************************************************************************/
/**
  * \brief Getnext table entry by MIB name of mibContextTableEntry
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to getnext the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int
mibContextTableEntry_getnext_by_mib_name(mibContextTable_entry_t *table_entry);

/**
  * \brief Getnext table entry by description of mibContextTableEntry
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to getnext the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int
mibPrefixTableEntry_getnext_by_descr(mibContextTable_entry_t *table_entry);

/**
  * \brief Get table entry by description of mibContextTableEntry
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int
mibPrefixTableEntry_get_by_descr(mibContextTable_entry_t *table_entry);


#ifdef __cplusplus
}
#endif
#endif                          /* _MIBCONTEXTTABLE_H_ */
