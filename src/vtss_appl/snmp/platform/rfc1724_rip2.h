/*

 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

// Note: This file originally auto-generated by mib2c using vtss_mib2c_ucd_snmp.conf v3.40

#ifndef _RFC1724_RIP2_H_
#define _RFC1724_RIP2_H_


#define RIP2_STR_LEN_MAX    63  // FIXME: Redefine a sufficient value for saving the memory


/******************************************************************************/
//
// Data structure declarations
//
/******************************************************************************/

// The scalar data structure for rip2GlobalRouteChanges
typedef struct {
    u_long rip2GlobalRouteChanges;
    u_long rip2GlobalQueries;
} rip2Globals_scalar_t;


// The table entry data structure for rip2IfStatTable
typedef struct {
    // Entry keys
    in_addr_t   rip2IfStatAddress;

    // Entry columns
    u_long rip2IfStatRcvBadPackets;
    u_long rip2IfStatRcvBadRoutes;
    u_long rip2IfStatSentUpdates;
    long rip2IfStatStatus;
} rip2IfStatTable_entry_t;

// The table entry data structure for rip2IfConfTable
typedef struct {
    // Entry keys
    in_addr_t   rip2IfConfAddress;

    // Entry columns
    char rip2IfConfDomain[RIP2_STR_LEN_MAX + 1];
    size_t  rip2IfConfDomain_len;
    long rip2IfConfAuthType;
    char rip2IfConfAuthKey[RIP2_STR_LEN_MAX + 1];
    size_t  rip2IfConfAuthKey_len;
    long rip2IfConfSend;
    long rip2IfConfReceive;
    long rip2IfConfDefaultMetric;
    long rip2IfConfStatus;
    in_addr_t rip2IfConfSrcAddress;
} rip2IfConfTable_entry_t;

// The table entry data structure for rip2PeerTable
typedef struct {
    // Entry keys
    in_addr_t   rip2PeerAddress;
    char   rip2PeerDomain[RIP2_STR_LEN_MAX + 1];
    size_t  rip2PeerDomain_len;

    // Entry columns
    u_long rip2PeerLastUpdate;
    long rip2PeerVersion;
    u_long rip2PeerRcvBadPackets;
    u_long rip2PeerRcvBadRoutes;
} rip2PeerTable_entry_t;


/******************************************************************************/
//
// Initial function
//
/******************************************************************************/
/**
  * \brief Initializes the SNMP-part of the RIPv2-MIB:rip2.
  **/
void rfc1724_rip2_init(void);


/******************************************************************************/
//
// Scalar access function declarations
//
/******************************************************************************/
/**
  * \brief Get scalar data of rip2GlobalsScalar.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the scalar
  *                              entry to get the scalar data.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int rip2GlobalsScalar_get(rip2Globals_scalar_t *scalar_entry);

/**
  * \brief Set scalar data of rip2GlobalsScalar.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the scalar
  *                              entry to set the scalar data.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int rip2GlobalsScalar_set(rip2Globals_scalar_t *scalar_entry);


/******************************************************************************/
//
// Table entry access function declarations
//
/******************************************************************************/
/**
  * \brief Get first table entry of rip2IfStatTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get the first table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int rip2IfStatTableEntry_getfirst(rip2IfStatTable_entry_t *table_entry);

/**
  * \brief Get/Getnext table entry of rip2IfStatTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get/getnext the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int rip2IfStatTableEntry_get(rip2IfStatTable_entry_t *table_entry, int getnext);

/**
  * \brief Set table entry of rip2IfStatTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to set the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int rip2IfStatTableEntry_set(rip2IfStatTable_entry_t *table_entry);
/**
  * \brief Get first table entry of rip2IfConfTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get the first table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int rip2IfConfTableEntry_getfirst(rip2IfConfTable_entry_t *table_entry);

/**
  * \brief Get/Getnext table entry of rip2IfConfTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get/getnext the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int rip2IfConfTableEntry_get(rip2IfConfTable_entry_t *table_entry, int getnext);

/**
  * \brief Set table entry of rip2IfConfTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to set the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int rip2IfConfTableEntry_set(rip2IfConfTable_entry_t *table_entry);
/**
  * \brief Get first table entry of rip2PeerTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get the first table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int rip2PeerTableEntry_getfirst(rip2PeerTable_entry_t *table_entry);

/**
  * \brief Get/Getnext table entry of rip2PeerTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get/getnext the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int rip2PeerTableEntry_get(rip2PeerTable_entry_t *table_entry, int getnext);


#endif /* _RFC1724_RIP2_H_ */

