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

#include "icli_api.h"
#include "conf_api.h"
#include "conf_icli_util.h"

icli_rc_t conf_icli_cmd(conf_icli_req_t *req)
{
    mesa_rc          rc = VTSS_RC_OK;
    u32              session_id = req->session_id;
    conf_sec_t       sec;
    conf_sec_info_t  info;
    conf_mgmt_blk_t  blk;
    conf_mgmt_conf_t conf;
    conf_blk_id_t    id;
    BOOL             first;
    ulong            size;

    switch (req->cmd) {
    case CONF_ICLI_CMD_BLOCKS:
        for (sec = CONF_SEC_LOCAL; sec < CONF_SEC_CNT; sec++) {
            if (req->clear) {
                conf_sec_renew(sec);
                continue;
            }

            ICLI_PRINTF("--- %s Section ---\n\n", sec == CONF_SEC_LOCAL ? "Local" : "Global");
            first = 1;
            size = 0;
            id = CONF_BLK_CONF_HDR;
            while (conf_mgmt_sec_blk_get(sec, id, &blk, 1) == VTSS_RC_OK) {
                if (first) {
                    ICLI_PRINTF("ID   Data        Size     CRC32       Changes  Name\n");
                    first = 0;
                }
                ICLI_PRINTF("%-3d  0x%08lx  %-7ld  0x%08lx  %-7ld  %s\n",
                            blk.id, (ulong)blk.data, blk.size, blk.crc, blk.change_count, blk.name);
                id = blk.id;
                size += blk.size;
            }
            ICLI_PRINTF("%sTotal size: %lu\n", first ? "" : "\n", size);
            conf_sec_get(sec, &info);
            ICLI_PRINTF("Save count: %lu\n", info.save_count);
            ICLI_PRINTF("Flash size: %lu\n\n", info.flash_size);
        }
        break;
    case CONF_ICLI_CMD_FLASH:
        if ((rc = conf_mgmt_conf_get(&conf)) == VTSS_RC_OK) {
            if (req->enable || req->disable) {
                conf.flash_save = req->enable;
                rc = conf_mgmt_conf_set(&conf);
            } else {
                ICLI_PRINTF("Flash Save: %s\n", conf.flash_save ? "Enabled" : "Disabled");
            }
        }
        break;
    case CONF_ICLI_CMD_STACK:
        if ((rc = conf_mgmt_conf_get(&conf)) == VTSS_RC_OK) {
            if (req->enable || req->disable) {
                conf.stack_copy = req->enable;
                rc = conf_mgmt_conf_set(&conf);
            } else {
                ICLI_PRINTF("Stack Copy: %s\n", conf.stack_copy ? "Enabled" : "Disabled");
            }
        }
        break;
    case CONF_ICLI_CMD_CHANGE:
        if ((rc = conf_mgmt_conf_get(&conf)) == VTSS_RC_OK) {
            if (req->enable || req->disable) {
                conf.change_detect = req->enable;
                rc = conf_mgmt_conf_set(&conf);
            } else {
                ICLI_PRINTF("Change Detect: %s\n", conf.change_detect ? "Enabled" : "Disabled");
            }
        }
        break;
    default:
        rc = VTSS_RC_ERROR;
        break;
    }
    return (rc == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR);
}
