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
/*
******************************************************************************

    Revision history
    > CP.Wang, 2012/09/24 11:22
        - create

******************************************************************************
*/

/*
******************************************************************************

    Include files

******************************************************************************
*/
#include <stdlib.h>
#include "icfg_api.h"
#include "rmon_api.h"
#include "misc_api.h"   //uport2iport(), iport2uport()
#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()
#include "rmon_trace.h"

/*
******************************************************************************

    Static Function

******************************************************************************
*/
static const char *_oid_2_name(IN oid *name, IN int name_len)
{

    switch (name[9]) {
    case 10:
        return "ifInOctets";
    case 11:
        return "ifInUcastPkts";
    case 12:
        return "ifInNUcastPkts";
    case 13:
        return "ifInDiscards";
    case 14:
        return "ifInErrors";
    case 15:
        return "ifInUnknownProtos";
    case 16:
        return "ifOutOctets";
    case 17:
        return "ifOutUcastPkts";
    case 18:
        return "ifOutNUcastPkts";
    case 19:
        return "ifOutDiscards";
    case 20:
        return "ifOutErrors";
    case 21:
        return "ifOutQLen";
    default:
        return "unKnown";

    }
}

/* ICFG callback functions */
static mesa_rc _rmon_icfg(
    IN const vtss_icfg_query_request_t  *req,
    IN vtss_icfg_query_result_t         *result
)
{
    mesa_rc                     rc;
    vtss_alarm_ctrl_entry_t     alarm_entry;
    vtss_event_ctrl_entry_t     event_entry;
    iftable_info_t              iftable_info;
    vtss_stat_ctrl_entry_t      stat_entry;
    vtss_isid_t                 isid;
    mesa_port_no_t              iport;
    vtss_history_ctrl_entry_t   history_entry;

    if ( req == NULL ) {
        T_E("req == NULL\n");
        return VTSS_RC_ERROR;
    }

    if ( result == NULL ) {
        T_E("result == NULL\n");
        return VTSS_RC_ERROR;
    }

    switch ( req->cmd_mode ) {
    case ICLI_CMD_MODE_GLOBAL_CONFIG:
        /* rmon alarm */
        memset(&alarm_entry, 0, sizeof(alarm_entry));
        while ( rmon_mgmt_alarm_entry_get(&alarm_entry, TRUE) == VTSS_RC_OK ) {
            rc = vtss_icfg_printf(result, "rmon alarm %u %s %lu %u %s rising-threshold %d %u falling-threshold %d %u %s\n",
                                  alarm_entry.id,
                                  _oid_2_name(alarm_entry.var_name.objid, alarm_entry.var_name.length),
                                  alarm_entry.var_name.objid[10],
                                  alarm_entry.interval,
                                  (alarm_entry.sample_type == VTSS_APPL_RMON_SAMPLE_TYPE_ABSOLUTE) ? "absolute" : "delta",
                                  (i32)(alarm_entry.rising_threshold),
                                  alarm_entry.rising_event_index,
                                  (i32)(alarm_entry.falling_threshold),
                                  alarm_entry.falling_event_index,
                                  (alarm_entry.startup_type == VTSS_APPL_RMON_ALARM_RISING) ? "rising" : (alarm_entry.startup_type == VTSS_APPL_RMON_ALARM_FALLING) ? "falling" : "both"
                                 );
            if ( rc != VTSS_RC_OK ) {
                T_E("fail to print to icfg\n");
                return VTSS_RC_ERROR;
            }
        }

        /* rmon event */
        memset(&event_entry, 0, sizeof(event_entry));
        while ( rmon_mgmt_event_entry_get(&event_entry, TRUE) == VTSS_RC_OK ) {
            switch ( event_entry.event_type ) {
            case VTSS_APPL_RMON_EVENT_NONE:
                if ( event_entry.event_description ) {
                    rc = vtss_icfg_printf(result, "rmon event %u description %s\n",
                                          event_entry.id,
                                          event_entry.event_description
                                         );
                } else {
                    rc = vtss_icfg_printf(result, "rmon event %u\n",
                                          event_entry.id
                                         );
                }
                break;

            case VTSS_APPL_RMON_EVENT_LOG:
                if ( event_entry.event_description ) {
                    rc = vtss_icfg_printf(result, "rmon event %u log description %s\n",
                                          event_entry.id,
                                          event_entry.event_description
                                         );
                } else {
                    rc = vtss_icfg_printf(result, "rmon event %u log\n",
                                          event_entry.id
                                         );
                }
                break;

            case VTSS_APPL_RMON_EVENT_TRAP:
                if ( event_entry.event_description ) {
                    rc = vtss_icfg_printf(result, "rmon event %u trap description %s\n",
                                          event_entry.id,
                                          event_entry.event_description
                                         );
                } else {
                    rc = vtss_icfg_printf(result, "rmon event %u trap\n",
                                          event_entry.id
                                         );
                }
                break;

            case VTSS_APPL_RMON_EVENT_LOG_AND_TRAP:
                if ( event_entry.event_description ) {
                    rc = vtss_icfg_printf(result, "rmon event %u log trap description %s\n",
                                          event_entry.id,
                                          event_entry.event_description
                                         );
                } else {
                    rc = vtss_icfg_printf(result, "rmon event %u log trap\n",
                                          event_entry.id
                                         );
                }
                break;

            default:
                T_E("invalid event type %u\n", event_entry.event_type);
                return VTSS_RC_ERROR;
            }

            if ( rc != VTSS_RC_OK ) {
                T_E("fail to print to icfg\n");
                return VTSS_RC_ERROR;
            }
        }
        break;

    case ICLI_CMD_MODE_INTERFACE_PORT_LIST:
        /* get isid and iport */
        isid = topo_usid2isid(req->instance_id.port.usid);
        iport = uport2iport(req->instance_id.port.begin_uport);

        /* rmon collection stats */
        memset(&stat_entry, 0, sizeof(stat_entry));
        while ( rmon_mgmt_statistics_entry_get(&stat_entry, TRUE) == VTSS_RC_OK ) {
            iftable_info.ifIndex = (ifIndex_id_t)(stat_entry.data_source.objid[10]);
            if ( ifIndex_get(&iftable_info) == FALSE ) {
                T_E("fail to get from ifIndex\n");
                return VTSS_RC_ERROR;
            }
            if ( iftable_info.type != IFTABLE_IFINDEX_TYPE_PORT ) {
                T_E("invalid ifindex type\n");
                return VTSS_RC_ERROR;
            }
            if ( iftable_info.isid == isid && iftable_info.if_id == iport ) {
                rc = vtss_icfg_printf(result, " rmon collection stats %u\n",
                                      stat_entry.id
                                     );
                if ( rc != VTSS_RC_OK ) {
                    T_E("fail to print to icfg\n");
                    return VTSS_RC_ERROR;
                }
            }
        }

        /* rmon collection history */
        memset(&history_entry, 0, sizeof(history_entry));
        while ( rmon_mgmt_history_entry_get(&history_entry, TRUE) == VTSS_RC_OK ) {
            iftable_info.ifIndex = (ifIndex_id_t)(history_entry.data_source.objid[10]);
            if ( ifIndex_get(&iftable_info) == FALSE ) {
                T_E("fail to get from ifIndex\n");
                return VTSS_RC_ERROR;
            }
            if ( iftable_info.type != IFTABLE_IFINDEX_TYPE_PORT ) {
                T_E("invalid ifindex type\n");
                return VTSS_RC_ERROR;
            }
            if ( iftable_info.isid == isid && iftable_info.if_id == iport ) {
                rc = vtss_icfg_printf(result, " rmon collection history %u buckets %lu interval %lu\n",
                                      history_entry.id,
                                      history_entry.scrlr.data_requested,
                                      history_entry.interval
                                     );
                if ( rc != VTSS_RC_OK ) {
                    T_E("fail to print to icfg\n");
                    return VTSS_RC_ERROR;
                }
            }
        }
        break;

    default:
        /* no config in other modes */
        break;
    }
    return VTSS_RC_OK;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
mesa_rc rmon_icfg_init(void)
{
    mesa_rc rc;

    /*
        Register Global config callback function to ICFG module.
    */
    rc = vtss_icfg_query_register(VTSS_ICFG_GLOBAL_RMON, "rmon", _rmon_icfg);
    if ( rc != VTSS_RC_OK ) {
        return rc;
    }

    /*
        Register Interface port list callback function to ICFG module.
    */
    rc = vtss_icfg_query_register(VTSS_ICFG_INTERFACE_ETHERNET_RMON, "rmon", _rmon_icfg);
    return rc;
}
