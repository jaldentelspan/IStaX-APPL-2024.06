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
//       Revise the "FIXME" parts to make it as a completed code.

#include <main.h>

#include "vtss_os_wrapper_snmp.h"
#include "vtss_snmp_api.h"
#include "ucd_snmp_ieee8021PreemptionMib.h"
#include "ieee8021PreemptionMib.h"
#include "mibContextTable.h" // mibContextTable_register()
#include "snmp_mib_redefine.h"  // snmp_mib_redefine_register()
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#include <vtss/appl/tsn.h>
#include "dot1Port_api.h"

// Trace module ID
#define VTSS_TRACE_MODULE_ID    VTSS_MODULE_ID_SNMP
#define IEEE8021BRIDGECOMPONENTID       1
#define IEEE8021BRIDGECOMPONENT_CNT     1

/******************************************************************************/
//
// Initial function
//
/******************************************************************************/
/**
  * \brief Initializes the SNMP-part of the IEEE8021-Preemption-MIB:ieee8021PreemptionMib.
  **/
void ieee8021PreemptionMib_init(void)
{
    T_D("enter");
    ucd_snmp_init_ieee8021PreemptionMib();
    oid             ieee8021PreemptionMib_oid[] = { 1, 3, 111, 2, 802, 1, 1, 29 };

    mibContextTable_register(ieee8021PreemptionMib_oid,
                             sizeof(ieee8021PreemptionMib_oid) / sizeof(oid),
                             "IEEE8021-PREEMPTION-MIB : ieee8021PreemptionMib");

    // ieee8021FramePreemptionAdminStatus
    oid ieee8021FramePreemptionAdminStatus_variables_oid[] = { 1, 3, 111, 2, 802, 1, 1, 29, 1, 1, 1, 1, 2 };
    snmp_mib_redefine_register(ieee8021FramePreemptionAdminStatus_variables_oid,
                               sizeof(ieee8021FramePreemptionAdminStatus_variables_oid) / sizeof(oid),
                               "IEEE8021-Preemption-MIB : ieee8021FramePreemptionAdminStatus",
                               "INTEGER",
                               SNMP_MIB_ACCESS_TYPE_RWRITE,
                               SNMP_MIB_ACCESS_TYPE_RWRITE,
                               FALSE,
                               "\
{0 Disable} \
{1 Preemption} \
");

    // ieee8021FramePreemptionHoldAdvance
    oid ieee8021FramePreemptionHoldAdvance_variables_oid[] = { 1, 3, 111, 2, 802, 1, 1, 29, 1, 1, 2, 1, 1 };
    snmp_mib_redefine_register(ieee8021FramePreemptionHoldAdvance_variables_oid,
                               sizeof(ieee8021FramePreemptionHoldAdvance_variables_oid) / sizeof(oid),
                               "IEEE8021-Preemption-MIB : ieee8021FramePreemptionHoldAdvance",
                               "UNSIGNED32",
                               SNMP_MIB_ACCESS_TYPE_RONLY,
                               SNMP_MIB_ACCESS_TYPE_RONLY,
                               FALSE,
                               "");

    // ieee8021FramePreemptionReleaseAdvance
    oid ieee8021FramePreemptionReleaseAdvance_variables_oid[] = { 1, 3, 111, 2, 802, 1, 1, 29, 1, 1, 2, 1, 2 };
    snmp_mib_redefine_register(ieee8021FramePreemptionReleaseAdvance_variables_oid,
                               sizeof(ieee8021FramePreemptionReleaseAdvance_variables_oid) / sizeof(oid),
                               "IEEE8021-Preemption-MIB : ieee8021FramePreemptionReleaseAdvance",
                               "UNSIGNED32",
                               SNMP_MIB_ACCESS_TYPE_RONLY,
                               SNMP_MIB_ACCESS_TYPE_RONLY,
                               FALSE,
                               "");

    // ieee8021FramePreemptionActive
    oid ieee8021FramePreemptionActive_variables_oid[] = { 1, 3, 111, 2, 802, 1, 1, 29, 1, 1, 2, 1, 3 };
    snmp_mib_redefine_register(ieee8021FramePreemptionActive_variables_oid,
                               sizeof(ieee8021FramePreemptionActive_variables_oid) / sizeof(oid),
                               "IEEE8021-Preemption-MIB : ieee8021FramePreemptionActive",
                               "INTEGER",
                               SNMP_MIB_ACCESS_TYPE_RONLY,
                               SNMP_MIB_ACCESS_TYPE_RONLY,
                               FALSE,
                               "\
{1 idle} \
{2 active} \
");

    // ieee8021FramePreemptionHoldRequest
    oid ieee8021FramePreemptionHoldRequest_variables_oid[] = { 1, 3, 111, 2, 802, 1, 1, 29, 1, 1, 2, 1, 4 };
    snmp_mib_redefine_register(ieee8021FramePreemptionHoldRequest_variables_oid,
                               sizeof(ieee8021FramePreemptionHoldRequest_variables_oid) / sizeof(oid),
                               "IEEE8021-Preemption-MIB : ieee8021FramePreemptionHoldRequest",
                               "INTEGER",
                               SNMP_MIB_ACCESS_TYPE_RONLY,
                               SNMP_MIB_ACCESS_TYPE_RONLY,
                               FALSE,
                               "\
{1 hold} \
{2 release} \
");

    T_D("exit");
}

/******************************************************************************/
//
// Local functions
//
/******************************************************************************/
static int IEEE8021BRIDGE_BasePortTableEntryKey_getnext(u_long *componentId, u_long *basePort)
{
    u_long tmp_componentId;
    dot1Port_info_t info;
    if ( *componentId > IEEE8021BRIDGECOMPONENT_CNT ) {
        return VTSS_RC_ERROR;
    }

    if ( *componentId  < IEEE8021BRIDGECOMPONENTID) {
        tmp_componentId = 1;
        info.dot1port = DOT1PORT_NO_NONE;
    } else {
        tmp_componentId = *componentId;
        info.dot1port = *basePort;
    }

    if (FALSE == dot1Port_get_next(&info)) {
        return VTSS_RC_ERROR;
    }
    *componentId = tmp_componentId;
    *basePort = info.dot1port;
    return VTSS_RC_OK;

}

static int IEEE8021BRIDGE_BasePortTableEntryKey_get(u_long *componentId, u_long *basePort)
{
    u_long tmp_componentId;
    u_long tmp_basePort;

    if ( *componentId > IEEE8021BRIDGECOMPONENT_CNT || *componentId < IEEE8021BRIDGECOMPONENTID ) {
        return VTSS_RC_ERROR;
    }

    if (*basePort == VTSS_VID_NULL) {
        return VTSS_RC_ERROR;
    }
    tmp_componentId = *componentId;
    tmp_basePort = *basePort - 1;


    if (VTSS_RC_OK != IEEE8021BRIDGE_BasePortTableEntryKey_getnext(&tmp_componentId, &tmp_basePort)) {
        return VTSS_RC_ERROR;
    }

    if (tmp_componentId != *componentId || tmp_basePort != *basePort) {
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}
static BOOL IEEE8021Preemption_ParameterTableEntry_get(ieee8021PreemptionParameterTable_entry_t *table_entry, int getnext)
{
    vtss_appl_tsn_fp_cfg_t     conf;
    vtss_ifindex_t             ifindex;
    vtss_ifindex_t             ifindex_next;
    mesa_prio_t                queue;
    mesa_rc                    rc;
    vtss_ifindex_elm_t         ife;

    if (table_entry->ieee8021BridgeBasePort == 1 && table_entry->ieee8021PreemptionPriority == 0) {
        vtss_appl_tsn_fp_cfg_itr(NULL, &ifindex);
    } else {
        if ((rc = vtss_ifindex_from_usid_uport(VTSS_USID_START, table_entry->ieee8021BridgeBasePort, &ifindex)) != VTSS_RC_OK) {
            return FALSE;
        }
    }
    queue = table_entry->ieee8021PreemptionPriority;
    if (getnext) {
        if ( queue < 7 ) {
            queue++;
        } else {
            queue = 0;
            if ((rc = vtss_appl_tsn_fp_cfg_itr(&ifindex, &ifindex_next)) != VTSS_RC_OK) {
                // reached end of port iterator
                return FALSE;
            }
            ifindex = ifindex_next;
        }
    }
    if ((rc = vtss_appl_tsn_fp_cfg_get(ifindex, &conf)) != VTSS_RC_OK) {
        T_W("Cannot get premption parameter data : %s  ifindex %u\n", error_txt(rc), ifindex );
        return FALSE;
    }
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    table_entry->ieee8021BridgeBasePort             = ife.ordinal + 1;
    table_entry->ieee8021PreemptionPriority         = queue;
    table_entry->ieee8021FramePreemptionAdminStatus = conf.admin_status[queue] ? IEEE8021_FP_ADMIN_PREEMTABLE : IEEE8021_FP_ADMIN_EXPRESS;
    return TRUE;
}

static BOOL IEEE8021Preemption_ConfigTableEntry_get(ieee8021PreemptionConfigTable_entry_t *table_entry, int getnext)
{

    vtss_ifindex_t                   ifindex;
    vtss_appl_tsn_fp_status_t        status;
    mesa_rc                          rc;

    if (table_entry->ieee8021BridgeBasePort == 1) {
        vtss_appl_tsn_fp_status_itr(NULL, &ifindex);
    } else {
        if ((rc = vtss_ifindex_from_usid_uport(VTSS_USID_START, table_entry->ieee8021BridgeBasePort, &ifindex)) != VTSS_RC_OK) {
            T_W("Cannot get the interface index : %s for port %lu\n", error_txt(rc), table_entry->ieee8021BridgeBasePort);
            return FALSE;
        }
    }
    if ((rc = vtss_appl_tsn_fp_status_get(ifindex, &status)) != VTSS_RC_OK) {
        T_W("Cannot get premption Config data : %s\n", error_txt(rc));
        return FALSE;
    }
    table_entry->ieee8021FramePreemptionHoldAdvance    = status.hold_advance;
    table_entry->ieee8021FramePreemptionReleaseAdvance = status.release_advance;
    table_entry->ieee8021FramePreemptionActive         = status.preemption_active ? IEEE8021_PREEMPTION_ACTIVE : IEEE8021_PREEMPTION_IDLE;
    table_entry->ieee8021FramePreemptionHoldRequest    = status.hold_request ? IEEE8021_PREEMPTION_HOLD : IEEE8021_PREEMPTION_RELEASE;
    return TRUE;
}


/******************************************************************************/
//
// Scalar access functions
//
/******************************************************************************/


/******************************************************************************/
//
// Table entry access functions
//
/******************************************************************************/
/**
  * \brief Get first table entry of ieee8021PreemptionParameterTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get the first table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int ieee8021PreemptionParameterTableEntry_getfirst(ieee8021PreemptionParameterTable_entry_t *table_entry)
{
    ieee8021PreemptionParameterTable_entry_t buff;

    T_D("enter");

    memset(&buff, 0, sizeof(buff));

    buff.ieee8021BridgeBaseComponentId  = 1;
    buff.ieee8021BridgeBasePort         = 1;
    buff.ieee8021PreemptionPriority     = 0;

    if (!IEEE8021Preemption_ParameterTableEntry_get(&buff, 0)) {
        return -1;
    }
    memcpy(table_entry, &buff, sizeof(ieee8021PreemptionParameterTable_entry_t));

    T_D("exit");
    return 0;
}

/**
  * \brief Get/Getnext table entry of ieee8021PreemptionParameterTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get/getnext the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int ieee8021PreemptionParameterTableEntry_get(ieee8021PreemptionParameterTable_entry_t *table_entry, int getnext)
{
    ieee8021PreemptionParameterTable_entry_t buff;

    T_D("enter");

    memset(&buff, 0, sizeof(buff));
    if ((table_entry->ieee8021BridgeBaseComponentId == 0 || table_entry->ieee8021BridgeBaseComponentId > IEEE8021BRIDGECOMPONENTID)) {
        return -1;
    }

    buff.ieee8021BridgeBaseComponentId     = table_entry->ieee8021BridgeBaseComponentId;
    buff.ieee8021BridgeBasePort            = table_entry->ieee8021BridgeBasePort;
    buff.ieee8021PreemptionPriority        = table_entry->ieee8021PreemptionPriority;
    if (!IEEE8021Preemption_ParameterTableEntry_get(&buff, getnext)) {
        return -1;
    }
    memcpy(table_entry, &buff, sizeof(ieee8021PreemptionParameterTable_entry_t));

    T_D("exit");
    return 0;
}

/**
  * \brief Set table entry of ieee8021PreemptionParameterTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to set the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int ieee8021PreemptionParameterTableEntry_set(ieee8021PreemptionParameterTable_entry_t *table_entry)
{
    vtss_appl_tsn_fp_cfg_t    conf;
    mesa_prio_t               queue;
    vtss_ifindex_t            ifindex;
    mesa_rc                   rc;
    T_D("enter");

    if ((rc = vtss_ifindex_from_usid_uport(VTSS_USID_START, table_entry->ieee8021BridgeBasePort, &ifindex)) != VTSS_RC_OK) {
        T_W("Cannot get the interface index : %s\n", error_txt(rc));
        return -1;
    }

    queue = table_entry->ieee8021PreemptionPriority;
    if (queue > 7) {
        T_W("ieee8021PreemptionPriority %u is out of range : [0 - 7]\n", queue);
        return -1;
    }

    if ((rc = vtss_appl_tsn_fp_cfg_get(ifindex, &conf)) != VTSS_RC_OK) {
        T_W("Cannot get preemption configuration parameter: %s\n", error_txt(rc));
        return -1;
    }

    conf.admin_status[queue] = (table_entry->ieee8021FramePreemptionAdminStatus == IEEE8021_FP_ADMIN_PREEMTABLE) ? TRUE : FALSE;

    if (vtss_appl_tsn_fp_cfg_set(ifindex, &conf) != VTSS_RC_OK) {
        return -1;
    }

    T_D("exit");
    return 0;
}

/**
  * \brief Get first table entry of ieee8021PreemptionConfigTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get the first table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int ieee8021PreemptionConfigTableEntry_getfirst(ieee8021PreemptionConfigTable_entry_t *table_entry)
{
    ieee8021PreemptionConfigTable_entry_t buff;
    u_long                                tmp_componentId = 0;
    u_long                                tmp_basePort = 0;

    T_D("enter");

    if (VTSS_RC_OK != IEEE8021BRIDGE_BasePortTableEntryKey_getnext(&tmp_componentId, &tmp_basePort)) {
        return -1;
    }
    buff.ieee8021BridgeBaseComponentId = tmp_componentId;
    buff.ieee8021BridgeBasePort        = tmp_basePort;

    if (!IEEE8021Preemption_ConfigTableEntry_get(&buff, 0)) {
        return -1;
    }
    memcpy(table_entry, &buff, sizeof(ieee8021PreemptionConfigTable_entry_t));

    T_D("exit");
    return 0;
}

/**
  * \brief Get/Getnext table entry of ieee8021PreemptionConfigTableEntry.
  *
  * \param table_entry [IN_OUT]: Pointer to structure that contains the table
  *                              entry to get/getnext the table entry.
  *
  * \return: 0 if the operation success, non-zero value otherwise.
  **/
int ieee8021PreemptionConfigTableEntry_get(ieee8021PreemptionConfigTable_entry_t *table_entry, int getnext)
{
    u_long                                tmp_componentId = table_entry->ieee8021BridgeBaseComponentId;
    u_long                                tmp_basePort = table_entry->ieee8021BridgeBasePort;
    ieee8021PreemptionConfigTable_entry_t buff;

    T_D("enter");
    if (getnext) {
        if (VTSS_RC_OK != IEEE8021BRIDGE_BasePortTableEntryKey_getnext(&tmp_componentId, &tmp_basePort)) {
            return -1;
        }
    } else {
        if (VTSS_RC_OK != IEEE8021BRIDGE_BasePortTableEntryKey_get(&tmp_componentId, &tmp_basePort)) {
            return -1;
        }
    }

    buff.ieee8021BridgeBaseComponentId = tmp_componentId;
    buff.ieee8021BridgeBasePort        = tmp_basePort;

    if (!IEEE8021Preemption_ConfigTableEntry_get(&buff, getnext)) {
        return -1;
    }
    memcpy(table_entry, &buff, sizeof(ieee8021PreemptionConfigTable_entry_t));

    T_D("exit");
    return 0;
}
