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

    Include files

******************************************************************************
*/
#include "icfg_api.h"
#include "mirror_api.h"
#include "mirror.h"
#include "mirror_icfg.h"
#include "icli_porting_util.h"

#include "vlan_api.h"
#include "msg_api.h"

#include "misc_api.h"   //uport2iport(), iport2uport()
#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
#else
#include "standalone_api.h"
#endif

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/

/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/

static char *_rmirror_list2txt(BOOL *list, int min, int max, char *buf)
{
    int  i, first = 1, count = 0;
    BOOL member;
    char *p;

    p = buf;
    *p = '\0';
    for (i = min; i <= max; i++) {
        member = list[i];
        if ((member && (count == 0 || i == max)) || (!member && count > 1)) {
            p += sprintf(p, "%s%d",
                         first ? "" : count > (member ? 1 : 2) ? "-" : ",",
                         member ? (i) : i - 1);
            first = 0;
        }
        if (member) {
            count++;
        } else {
            count = 0;
        }
    }
    return buf;
}

/* ICFG callback functions */

// ICFG for the global configuration
static mesa_rc RMIRROR_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                        vtss_icfg_query_result_t *result)
{
    mesa_rc                 rc = VTSS_RC_OK;
    rmirror_switch_conf_t   conf, *conf_ptr = &conf;
    rmirror_switch_conf_t   def_conf, *def_conf_ptr = &def_conf;
    rmirror_switch_conf_t   switch_conf, *switch_conf_ptr = &switch_conf;
    mesa_port_no_t          iport;
    vtss_isid_t             isid;
    char                    str_buf[512];
    i32                     idx;
    u16                     next_session_id, *prev_session_id_ptr = NULL;
    BOOL                    found_vlan = FALSE;

    while (VTSS_RC_OK == vtss_appl_mirror_session_entry_itr(prev_session_id_ptr, &next_session_id)) {

        rmirror_mgmt_default_set(conf_ptr);
        rmirror_mgmt_default_set(def_conf_ptr);
        rmirror_mgmt_default_set(switch_conf_ptr);
        conf_ptr->session_num = next_session_id;
        if ((rc = rmirror_mgmt_conf_get(conf_ptr)) != VTSS_RC_OK) {
            T_D("exit, get mgmt conf error.");
            return rc;
        }

        /* source */
        // example: monitor session <uint> [ source { vlan <vlan_list> } ]
        for (idx = VTSS_APPL_VLAN_ID_MIN; idx <= VTSS_APPL_VLAN_ID_MAX; idx++) {
            if (conf_ptr->source_vid[idx] != def_conf_ptr->source_vid[idx]) {
                found_vlan = TRUE;
                break;
            }
        }
        if (found_vlan) {
            (void)_rmirror_list2txt(conf_ptr->source_vid.data(), VTSS_APPL_VLAN_ID_MIN, VTSS_APPL_VLAN_ID_MAX, str_buf);
            rc = vtss_icfg_printf(result, "monitor session " VPRIlu" source vlan %s\n",
                                  conf_ptr->session_num,
                                  str_buf);
            found_vlan = FALSE;
        }

        /* type */
        switch (conf_ptr->type) {
        case VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE:
#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
            // example: destination remote vlan <vlan_id>
            rc = (vtss_icfg_printf(result, "monitor session %d destination remote vlan %d\n",
                                   conf_ptr->session_num,
                                   conf_ptr->vid));


            // example: inner egress-tag [ none | all ]
            if (conf_ptr->reflector_port) {
                rc = (vtss_icfg_printf(result, "monitor session %d inner egress-tag none\n",
                                       conf_ptr->session_num));
            }
            if (req->all_defaults && conf_ptr->reflector_port == 0) {
                rc = (vtss_icfg_printf(result, "monitor session %d inner egress-tag all\n",
                                       conf_ptr->session_num));
            }

#else
            // example: destination remote vlan <vlan_id> reflector-port <port_type> <port_id>
            if (conf_ptr->vid != def_conf_ptr->vid ||
                conf_ptr->rmirror_switch_id != def_conf_ptr->rmirror_switch_id ||
                conf_ptr->reflector_port != def_conf_ptr->reflector_port) {

                T_D("conf_ptr->vid: %d, conf_ptr->rmirror_switch_id: %d, conf_ptr->reflector_port: %d",
                    conf_ptr->vid, conf_ptr->rmirror_switch_id, conf_ptr->reflector_port);
                T_D("def_conf_ptr->vid: %d, def_conf_ptr->rmirror_switch_id: %d, def_conf_ptr->reflector_port: %d",
                    def_conf_ptr->vid, def_conf_ptr->rmirror_switch_id, def_conf_ptr->reflector_port);

                rc = (vtss_icfg_printf(result, "monitor session " VPRIlu" destination remote vlan %d reflector-port %s\n",
                                       conf_ptr->session_num,
                                       conf_ptr->vid,
                                       icli_port_info_txt(topo_isid2usid(conf_ptr->rmirror_switch_id), iport2uport(conf_ptr->reflector_port), &str_buf[0])));
                T_D("conf_ptr->rmirror_switch_id: %d, conf_ptr->reflector_port: %d ", conf_ptr->rmirror_switch_id, conf_ptr->reflector_port);
            }
#endif
            break;
        case VTSS_APPL_RMIRROR_SWITCH_TYPE_DESTINATION:
            // example: source remote vlan <vlan_id>
            rc = (vtss_icfg_printf(result, "monitor session " VPRIlu" source remote vlan %d\n",
                                   conf_ptr->session_num,
                                   conf_ptr->vid));
            break;
        case VTSS_APPL_RMIRROR_SWITCH_TYPE_MIRROR:
        default:
            break;
        }

        /* source */
        // example: monitor session <uint> [ source { interface <port_type> <port_list> [ both | rx | tx ] | remote vlan <vlan_id> } ]
        // Loop through all switches in a stack
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            if (msg_switch_exists(isid)) {
                rmirror_mgmt_default_set(switch_conf_ptr);
                switch_conf_ptr->session_num = next_session_id;
                if (rmirror_mgmt_switch_conf_get(isid, switch_conf_ptr) != VTSS_RC_OK) {
                    //T_E("Could not get rmirror conf, switch id %d", isid);
                }

                //
                // Source ports
                //

                // We make a list of ports for each possible configuration, in order to be able to concatenate the interface list.
                mesa_port_list_t default_list;
                mesa_port_list_t rx_list;
                mesa_port_list_t tx_list;
                mesa_port_list_t both_list;

                // The found BOOLs indicates that at least one interface has the corresponding configuration.
                BOOL rx_found = FALSE;
                BOOL tx_found = FALSE;
                BOOL both_found = FALSE;
                BOOL default_found = FALSE;

                // Loop through all ports and set the BOOL list accordingly
                for (iport = 0; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
                    default_list[iport] = FALSE;
                    rx_list[iport] = FALSE;
                    tx_list[iport] = FALSE;
                    both_list[iport] = FALSE;


                    if ((switch_conf_ptr->source_port[iport].rx_enable == FALSE) &&
                        (switch_conf_ptr->source_port[iport].tx_enable == FALSE) ) {
                        default_list[iport] = TRUE;
                        default_found = TRUE;
                    }

                    if (switch_conf_ptr->source_port[iport].rx_enable && switch_conf_ptr->source_port[iport].tx_enable) {
                        both_list[iport] = TRUE;
                        both_found = TRUE;
                    } else if (switch_conf_ptr->source_port[iport].rx_enable) {
                        rx_list[iport] = TRUE;
                        rx_found = TRUE;

                    } else if (switch_conf_ptr->source_port[iport].tx_enable) {
                        tx_list[iport] = TRUE;
                        tx_found = TRUE;
                    }
                }

                // Print out configuration if at least one interface has that configuration
                if (default_found && req->all_defaults) {
                    rc = (vtss_icfg_printf(result, "no monitor session " VPRIlu" source interface %s\n",
                                           switch_conf_ptr->session_num,
                                           icli_port_list_info_txt(isid, default_list, &str_buf[0], FALSE)));
                }

                if (both_found) {
                    rc = (vtss_icfg_printf(result, "monitor session " VPRIlu" source interface %s both\n",
                                           switch_conf_ptr->session_num,
                                           icli_port_list_info_txt(isid, both_list, &str_buf[0], FALSE)));
                }

                if (rx_found) {
                    rc = (vtss_icfg_printf(result, "monitor session " VPRIlu" source interface %s rx\n",
                                           switch_conf_ptr->session_num,
                                           icli_port_list_info_txt(isid, rx_list, &str_buf[0], FALSE)));
                }

                if (tx_found) {
                    rc = (vtss_icfg_printf(result, "monitor session " VPRIlu" source interface %s tx\n",
                                           switch_conf_ptr->session_num,
                                           icli_port_list_info_txt(isid, tx_list, &str_buf[0], FALSE)));
                }
            }
        }

        /* source cpu */
        // example: monitor session <uint> source cpu [ both | rx | tx ]
        // Loop through all switches in a stack
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {

            BOOL rx_found = FALSE;
            BOOL tx_found = FALSE;
            BOOL both_found = FALSE;
            BOOL default_found = FALSE;

            if (msg_switch_exists(isid)) {
                rmirror_mgmt_default_set(switch_conf_ptr);
                switch_conf_ptr->session_num = next_session_id;
                if (rmirror_mgmt_switch_conf_get(isid, switch_conf_ptr) != VTSS_RC_OK) {
                    //T_E("Could not get rmirror conf, switch id %d", isid);
                }

                if (switch_conf_ptr->cpu_src_enable && switch_conf_ptr->cpu_dst_enable) {
                    both_found = TRUE;
                } else if (switch_conf_ptr->cpu_src_enable) {
                    rx_found = TRUE;
                } else if (switch_conf_ptr->cpu_dst_enable) {
                    tx_found = TRUE;
                } else {
                    default_found = TRUE;
                }

                // Print out configuration if at least one interface has that configuration
                if (default_found && req->all_defaults) {
                    rc = (vtss_icfg_printf(result, "no monitor session " VPRIlu" source cpu\n",
                                           switch_conf_ptr->session_num));
                }

                if (both_found) {
                    rc = (vtss_icfg_printf(result, "monitor session " VPRIlu" source cpu both\n",
                                           switch_conf_ptr->session_num));
                }

                if (rx_found) {
                    rc = (vtss_icfg_printf(result, "monitor session " VPRIlu" source cpu rx\n",
                                           switch_conf_ptr->session_num));
                }

                if (tx_found) {
                    rc = (vtss_icfg_printf(result, "monitor session " VPRIlu" source cpu tx\n",
                                           switch_conf_ptr->session_num));
                }
            }
        }

        /* destination */
        // example: monitor session <uint> [ destination { interface <port_type> <port_id> | remote vlan <vlan_id> reflector-port interface <port_type> <port_id> } ]
        // Loop through all switches in a stack
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            if (msg_switch_exists(isid)) {
                rmirror_mgmt_default_set(switch_conf_ptr);
                switch_conf_ptr->session_num = next_session_id;
                if (rmirror_mgmt_switch_conf_get(isid, switch_conf_ptr) != VTSS_RC_OK) {
                    //T_E("Could not get rmirror conf, switch id %d", isid);
                }

                //
                // Destination ports
                //

                // We make a list of ports for each possible configuration, in order to be able to concatenate the interface list.
                mesa_port_list_t default_list;
                mesa_port_list_t dest_list;

                // The found BOOLs indicates that at least one interface has the corresponding configuration.
                BOOL default_found = FALSE;
                BOOL found = FALSE;

                // Loop through all ports and set the BOOL list accordingly
                for (iport = 0; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
                    default_list[iport] = FALSE;
                    dest_list[iport] = FALSE;

                    if (switch_conf_ptr->destination_port[iport] == FALSE) {
                        default_list[iport] = TRUE;
                        default_found = TRUE;
                    } else {
                        dest_list[iport] = TRUE;
                        found = TRUE;
                    }
                }

                // Print out configuration if at least one interface has that configuration
                if (default_found && req->all_defaults) {
                    rc = (vtss_icfg_printf(result, "no monitor session " VPRIlu" destination interface %s\n",
                                           switch_conf_ptr->session_num,
                                           icli_port_list_info_txt(isid, default_list, &str_buf[0], FALSE)));
                }

                if (found) {
                    rc = (vtss_icfg_printf(result, "monitor session " VPRIlu" destination interface %s\n",
                                           switch_conf_ptr->session_num,
                                           icli_port_list_info_txt(isid, dest_list, &str_buf[0], FALSE)));
                }
            }
        }

        /* global mode */
        // example: monitor session <uint>
        if (req->all_defaults || conf_ptr->enabled != def_conf_ptr->enabled) {
            rc = vtss_icfg_printf(result, "%s%s " VPRIlu"\n",
                                  conf_ptr->enabled == RMIRROR_MGMT_ENABLED ? "" : RMIRROR_NO_FORM_TEXT,
                                  RMIRROR_GLOBAL_MODE_ENABLE_TEXT,
                                  conf_ptr->session_num);
        }

        prev_session_id_ptr = &next_session_id;
    }

    return rc;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
mesa_rc vtss_rmirror_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_MIRROR_GLOBAL_CONF, "vtss-rmirror", RMIRROR_ICFG_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    return rc;
}
