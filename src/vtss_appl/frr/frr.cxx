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

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "frr_daemon.hxx"
#include "frr_api.hxx"
#include "frr_trace.hxx"

/******************************************************************************/
/** Namespaces using declaration                                              */
/******************************************************************************/
using namespace vtss;

/******************************************************************************/
/** Module memory allocate declaration                                        */
/******************************************************************************/
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_FRR

/******************************************************************************/
/** Module trace declaration                                                  */
/******************************************************************************/
static vtss_trace_reg_t FRR_trace_reg = {VTSS_TRACE_MODULE_ID, "frr", "FRR"};

static vtss_trace_grp_t FRR_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
    [FRR_TRACE_GRP_DAEMON] = {
        "daemon",
        "Access to the FRR daemon",
        VTSS_TRACE_LVL_ERROR
    },
    [FRR_TRACE_GRP_OSPF] = {
        "ospf",
        "OSPF",
        VTSS_TRACE_LVL_WARNING
    },
    [FRR_TRACE_GRP_OSPF6] = {
        "ospf6",
        "OSPF6",
        VTSS_TRACE_LVL_WARNING
    },
    [FRR_TRACE_GRP_RIP] = {
        "rip",
        "RIP",
        VTSS_TRACE_LVL_WARNING
    },
    [FRR_TRACE_GRP_ROUTER] = {
        "router",
        "Router",
        VTSS_TRACE_LVL_WARNING
    },
    [FRR_TRACE_GRP_IP_ROUTE] = {
        "ip_route",
        "IP Route",
        VTSS_TRACE_LVL_ERROR
    },
    [FRR_TRACE_GRP_ICLI] = {
        "cli",
        "CLI",
        VTSS_TRACE_LVL_WARNING
    },
    [FRR_TRACE_GRP_ICFG] = {
        "icfg",
        "ICFG",
        VTSS_TRACE_LVL_WARNING
    },
    [FRR_TRACE_GRP_SNMP] = {
        "snmp",
        "SNMP",
        VTSS_TRACE_LVL_WARNING
    },
};

VTSS_TRACE_REGISTER(&FRR_trace_reg, FRR_trace_grps);

/******************************************************************************/
/** Module error text (convert the return code to error text)                 */
/******************************************************************************/
const char *frr_error_txt(mesa_rc rc)
{
    switch (rc) {
    case FRR_RC_INVALID_ARGUMENT:
        return "Invalid argument";

    case FRR_RC_IFINDEX_MUST_BE_OF_TYPE_VLAN:
        return "Interface index does not represent a VLAN";

    case FRR_RC_INTERNAL_ERROR:
        return "Generic error code";

    case FRR_RC_DAEMON_TYPE_INVALID:
        return "Invalid daemon type";

    case FRR_RC_DAEMON_NOT_STARTED:
        return "Daemon not started";

    case FRR_RC_IP_ROUTE_PARSE_ERROR:
        return "Unable to parse output from route daemon";

    case FRR_RC_NOT_SUPPORTED:
        return "Feature not supported";

    case FRR_RC_INTERNAL_ACCESS:
        return "Internal framework access error";

    case FRR_RC_ENTRY_NOT_FOUND:
        return "Entry is not found";

    case FRR_RC_ENTRY_ALREADY_EXISTS:
        return "Entry already exists";

    case FRR_RC_LIMIT_REACHED:
        return "Reached the maximum table size";

    case FRR_RC_ADDR_RANGE_OVERLAP:
        return "The address range overlaps with another entry";

    case FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST:
        return "The VLAN interface does not exist";
    }

    return "FRR: Unknown error code";
}

/******************************************************************************/
// frr_init()
/******************************************************************************/
mesa_rc frr_init(vtss_init_data_t *data)
{
    // Propagate it to the daemon
    VTSS_RC(frr_daemon_init(data));

    return VTSS_RC_OK;
}

