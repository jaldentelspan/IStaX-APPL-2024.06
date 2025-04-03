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

#include "icfg_api.h"
#include "misc_api.h"
#include "icli_api.h"
#include "icli_porting_util.h"
#include "topo_api.h"

#include "mac_api.h"
#include "vlan_api.h"
#include "mac_icfg.h"

#undef  VTSS_ALLOC_MODULE_ID
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_MAC

#define VTSS_TRACE_MODULE_ID        VTSS_MODULE_ID_MAC

//******************************************************************************
// ICFG callback functions
//******************************************************************************
static mesa_rc mac_icfg_global_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    mesa_rc               rc = VTSS_RC_OK;
    mac_age_conf_t        conf;
    switch_iter_t         sit;
    char                  buf[32];
    char                  dest_buf[600];
    mac_mgmt_addr_entry_t mac_entry;
    mesa_vid_mac_t        vid_mac;
    u32                   iport;
    vtss_appl_mac_vid_learn_mode_t mode;
    mesa_vid_t            vlan;
    u8                    vid_bitmask[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    char                  *vlan_buf = NULL;
    BOOL                  no_learn = 0;

    if (!req || !result) {
        return VTSS_RC_ERROR;
    }
    memset(vid_bitmask, 0, sizeof(vid_bitmask));

    /* Global level: mac address-table aging-time */
    (void)mac_mgmt_age_time_get(&conf);
    if (req->all_defaults || conf.mac_age_time != MAC_AGE_TIME_DEFAULT) {
        VTSS_RC(vtss_icfg_printf(result, "mac address-table aging-time %u\n", conf.mac_age_time));

    }

    /* Global level: mac address-table learning vlan <vlan_list>  */
    for (vlan = 1; vlan < VTSS_VIDS; vlan++) {
        (void)mac_mgmt_vlan_learn_mode_get(vlan, &mode);
        if (!mode.learning) {
            /* Create a bit mask */
            VTSS_BF_SET(vid_bitmask, vlan, 1);
            no_learn = 1;
        }
    }

    if (no_learn) {
        if ((vlan_buf = (char *)VTSS_MALLOC(VLAN_VID_LIST_AS_STRING_LEN_BYTES)) == NULL) {
            T_E("Out of memory");
            return VTSS_RC_ERROR;
        }
        /* Convert bitmask to txt string (through VLAN module) */
        (void)vlan_mgmt_vid_bitmask_to_txt(vid_bitmask, vlan_buf);

        if ((rc = vtss_icfg_printf(result, "no mac address-table learning vlan %s\n", vlan_buf)) != VTSS_RC_OK) {
            VTSS_FREE(vlan_buf);
            return VTSS_RC_ERROR;
        }
        if (vlan_buf) {
            VTSS_FREE(vlan_buf);
        }
    }
    /* only display learning enabled VLANs if all_defaults is requested */
    if (req->all_defaults) {
        memset(vid_bitmask, 0, sizeof(vid_bitmask));
        for (vlan = 1; vlan < VTSS_VIDS; vlan++) {
            (void)mac_mgmt_vlan_learn_mode_get(vlan, &mode);
            if (mode.learning) {
                VTSS_BF_SET(vid_bitmask, vlan, 1);
            }
        }
        if ((vlan_buf = (char *)VTSS_MALLOC(VLAN_VID_LIST_AS_STRING_LEN_BYTES)) == NULL) {
            T_E("Out of memory");
            return VTSS_RC_ERROR;
        }
        /* Convert bitmask to txt string (through VLAN module) */
        (void)vlan_mgmt_vid_bitmask_to_txt(vid_bitmask, vlan_buf);

        if ((rc = vtss_icfg_printf(result, "mac address-table learning vlan %s\n", vlan_buf)) != VTSS_RC_OK) {
            VTSS_FREE(vlan_buf);
            return VTSS_RC_ERROR;
        }
        if (vlan_buf) {
            VTSS_FREE(vlan_buf);
        }
    }

    /* Global level: mac address-table static, configurable switches */
    (void)icli_switch_iter_init(&sit);
    while (switch_iter_getnext(&sit)) {
        memset(&vid_mac, 0, sizeof(vid_mac));
        while (mac_mgmt_static_get_next(sit.isid, &vid_mac, &mac_entry, 1, 0) == VTSS_RC_OK) {
            BOOL first = TRUE;
            vid_mac = mac_entry.vid_mac;
            dest_buf[0] = '\0';
            for (iport = VTSS_PORT_NO_START; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
                if (mac_entry.destination[iport]) {
                    buf[0] = ' ';
                    (void)icli_port_info_txt(topo_isid2usid(sit.isid), iport2uport(iport), &buf[first ? 0 : 1]);
                    strcat(dest_buf, buf);
                    first = FALSE;
                }
            }
            VTSS_RC(vtss_icfg_printf(result, "mac address-table static %s vlan %u",
                                     icli_mac_txt(mac_entry.vid_mac.mac.addr, buf), mac_entry.vid_mac.vid));

            if (dest_buf[0]) {
                VTSS_RC(vtss_icfg_printf(result, " interface %s", dest_buf));
            }

            VTSS_RC(vtss_icfg_printf(result, "\n"));
        }
    }

    return rc;
}

static mesa_rc mac_icfg_intf_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    mesa_rc             rc = VTSS_RC_OK;
    vtss_isid_t         isid;
    mesa_port_no_t      iport;
    mesa_learn_mode_t   mode;
    BOOL                chg_allowed;

    if (!req || !result) {
        return VTSS_RC_ERROR;
    }

    /* Interface level: mac address secure */
    /* Interface level: mac address learning */

    isid = topo_usid2isid(req->instance_id.port.usid);
    iport = uport2iport(req->instance_id.port.begin_uport);
    VTSS_ASSERT(iport != VTSS_PORT_NO_NONE);

    (void)mac_mgmt_learn_mode_get(isid, iport, &mode, &chg_allowed);

    if (req->all_defaults || !mode.automatic) {
        if (mode.discard) {
            VTSS_RC(vtss_icfg_printf(result, " mac address-table learning secure\n"));
        } else {
            VTSS_RC(vtss_icfg_printf(result, " %smac address-table learning\n", mode.automatic ? "" : "no "));
        }
    }

    return rc;
}

//******************************************************************************
//   Public functions
//******************************************************************************

mesa_rc mac_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module. */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_MAC_INTERFACE_CONF, "mac", mac_icfg_intf_conf)) != VTSS_RC_OK) {
        return rc;
    }

    if ((rc = vtss_icfg_query_register(VTSS_ICFG_MAC_GLOBAL_CONF, "mac", mac_icfg_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    return VTSS_RC_OK;
}
