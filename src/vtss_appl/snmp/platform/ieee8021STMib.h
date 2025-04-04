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

#ifndef _IEEE8021STMIB_H_
#define _IEEE8021STMIB_H_


#define IEEE8021STMIB_STR_LEN_MAX    1796  // 256 * 7 + 4
#define IEEE8021STMIB_STR_LEN_GATE   1
#define IEEE8021STMIB_STR_LEN_TIME   10
#define IEEE8021STMIB_OID_LEN_MAX    16  // FIXME: Redefine a sufficient value for saving the memory
#define IEEE8021STMIB_BITS_LEN_MAX   4   // FIXME: Redefine a sufficient value for saving the memory


/******************************************************************************/
//
// Data structure declarations
//
/******************************************************************************/


// The table entry data structure for ieee8021STMaxSDUTable
typedef struct {
    // Entry keys
    u_long   ieee8021BridgeBaseComponentId;
    u_long   ieee8021BridgeBasePort;
    u_long   ieee8021STTrafficClass;

    // Entry columns
    u_long ieee8021STMaxSDU;
    struct counter64    ieee8021TransmissionOverrun;
} ieee8021STMaxSDUTable_entry_t;

// The table entry data structure for ieee8021STParametersTable
typedef struct {
    // Entry keys
    u_long   ieee8021BridgeBaseComponentId;
    u_long   ieee8021BridgeBasePort;

    // Entry columns
    long ieee8021STGateEnabled;
    char ieee8021STAdminGateStates[IEEE8021STMIB_STR_LEN_GATE + 1];
    size_t  ieee8021STAdminGateStates_len;
    char ieee8021STOperGateStates[IEEE8021STMIB_STR_LEN_GATE + 1];
    size_t  ieee8021STOperGateStates_len;
    u_long ieee8021STAdminControlListLength;
    u_long ieee8021STOperControlListLength;
    char ieee8021STAdminControlList[IEEE8021STMIB_STR_LEN_MAX + 1];
    size_t  ieee8021STAdminControlList_len;
    char ieee8021STOperControlList[IEEE8021STMIB_STR_LEN_MAX + 1];
    size_t  ieee8021STOperControlList_len;
    u_long ieee8021STAdminCycleTimeNumerator;
    u_long ieee8021STAdminCycleTimeDenominator;
    u_long ieee8021STOperCycleTimeNumerator;
    u_long ieee8021STOperCycleTimeDenominator;
    u_long ieee8021STAdminCycleTimeExtension;
    u_long ieee8021STOperCycleTimeExtension;
    char ieee8021STAdminBaseTime[IEEE8021STMIB_STR_LEN_TIME + 1];
    size_t  ieee8021STAdminBaseTime_len;
    char ieee8021STOperBaseTime[IEEE8021STMIB_STR_LEN_TIME + 1];
    size_t  ieee8021STOperBaseTime_len;
    long ieee8021STConfigChange;
    char ieee8021STConfigChangeTime[IEEE8021STMIB_STR_LEN_TIME + 1];
    size_t  ieee8021STConfigChangeTime_len;
    u_long ieee8021STTickGranularity;
    char ieee8021STCurrentTime[IEEE8021STMIB_STR_LEN_TIME + 1];
    size_t  ieee8021STCurrentTime_len;
    long ieee8021STConfigPending;
    struct counter64    ieee8021STConfigChangeError;
    u_long ieee8021STSupportedListMax;
} ieee8021STParametersTable_entry_t;

/**
 * \The following two enums defines the first two octet fields of
   each TLV inside ieee8021STAdminControlList.
   The first octet of each TLV is interpreted as an
   unsigned integer representing a gate operation name:
   0: SetGateStates
   1-255: Reserved for future gate operations
   The second octet of the TLV is the length field,
   interpreted as an unsigned integer, indicating the number of
   octets of the value that follows the length. A length of
   zero indicates that there is no value
   (i.e., the gate operation has no parameters).
  **/
typedef enum {
    ST_CONTROL_SET_GATE_STATES
} st_control_operations_t;

typedef enum {
    ST_SET_GATE_STATES_TLV_LENGTH = 5
} st_control_tlv_length_t;



/******************************************************************************/
//
// Initial function
//
/******************************************************************************/
/**
  * \brief Initializes the SNMP-part of the IEEE8021-ST-MIB:ieee8021STMib.
  **/
void ieee8021STMib_init(void);


/******************************************************************************/
//
// Scalar access function declarations
//
/******************************************************************************/


/******************************************************************************/
//
// Table entry access function declarations
//
/******************************************************************************/
/**
  * \brief Get first table entry of ieee8021STMaxSDUTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get the first table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int ieee8021STMaxSDUTableEntry_getfirst(ieee8021STMaxSDUTable_entry_t *table_entry);

/**
  * \brief Get/Getnext table entry of ieee8021STMaxSDUTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get/getnext the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int ieee8021STMaxSDUTableEntry_get(ieee8021STMaxSDUTable_entry_t *table_entry, int getnext);

/**
  * \brief Set table entry of ieee8021STMaxSDUTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to set the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int ieee8021STMaxSDUTableEntry_set(ieee8021STMaxSDUTable_entry_t *table_entry);
/**
  * \brief Get first table entry of ieee8021STParametersTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get the first table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int ieee8021STParametersTableEntry_getfirst(ieee8021STParametersTable_entry_t *table_entry);

/**
  * \brief Get/Getnext table entry of ieee8021STParametersTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get/getnext the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int ieee8021STParametersTableEntry_get(ieee8021STParametersTable_entry_t *table_entry, int getnext);

/**
  * \brief Set table entry of ieee8021STParametersTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to set the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int ieee8021STParametersTableEntry_set(ieee8021STParametersTable_entry_t *table_entry);

#endif /* _IEEE8021STMIB_H_ */

