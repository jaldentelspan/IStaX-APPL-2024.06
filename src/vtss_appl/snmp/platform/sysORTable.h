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

// Note: This file originally auto-generated by mib2c using vtss_mib2c_ucd_snmp.conf v3.40

#ifndef _SYSORTABLE_H_
#define _SYSORTABLE_H_

#define SYSORTABLE_STR_LEN_MAX      63
#define SYSORTABLE_OID_LEN_MAX      32
#define SYSORTABLE_MAX_ENTRY_CNT    256

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
//
// Data structure declarations
//
/******************************************************************************/


// The table entry data structure for sysORTable
typedef struct {
    // Entry keys
    long            sysORIndex;

    // Entry columns
    oid             sysORID[SYSORTABLE_OID_LEN_MAX + 1];
    size_t          sysORID_len;
    char            sysORDescr[SYSORTABLE_STR_LEN_MAX + 1];
    size_t          sysORDescr_len;
    u_long          sysORUpTime;
    char            mib_name[SYSORTABLE_STR_LEN_MAX + 1];
} sysORTable_entry_t;


/******************************************************************************/
//
// Initial function
//
/******************************************************************************/
/**
  * \brief Initializes the SNMP-part of the SNMPv2-MIB:sysORTable.
  **/
void            sysORTable_init(void);


/******************************************************************************/
//
// SysORTable entry register
//
/******************************************************************************/
/**
  * \brief Register table entry of sysORTable
  *
  * \param oidin  [IN]: The MIB OID which will register to sysORTable
  * \param oidlen [IN]: The OID length of input parameter "oidin"
  * \param descr  [IN]: The desciption of MIB node. Format: <MIB_File_Name> : <Scalar_or_Table_Name>
  **/
void sysORTable_register(oid *oidin, size_t oidlen, const char *descr);

/**
  * \brief Get the value of sysUpTime at the time of the most recent change in state or value of any instance of sysORID.
  *
  * \param last_change  [OUT]: The last change time of sysORTable entry
  **/
void sysORTable_LastChange_get(u_long *last_change);


/******************************************************************************/
//
// Table entry access function declarations
//
/******************************************************************************/
/**
  * \brief Get first table entry of sysORTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get the first table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int             sysORTableEntry_getfirst(sysORTable_entry_t *table_entry);

/**
  * \brief Get/Getnext table entry of sysORTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get/getnext the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int             sysORTableEntry_get(sysORTable_entry_t *table_entry,
                                    int getnext);

#ifdef __cplusplus
}
#endif
#endif                          /* _SYSORTABLE_H_ */
