/*

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

*/

#include "loop_protect_serializer.hxx"
#include "vtss/appl/loop_protect.h"

vtss_enum_descriptor_t vtss_loop_protection_action_txt[] {
    {VTSS_APPL_LOOP_PROTECT_ACTION_SHUTDOWN, "shutdown"},
    {VTSS_APPL_LOOP_PROTECT_ACTION_SHUT_LOG, "shutdownAndLogEvent"},
    {VTSS_APPL_LOOP_PROTECT_ACTION_LOG_ONLY, "logEvent"},
    {0, 0},
};

mesa_rc lprot_if2ife(vtss_ifindex_t ifindex, vtss_ifindex_elm_t *ife)
{
    mesa_rc rc;
    if (!vtss_ifindex_is_port(ifindex)) {
        rc = VTSS_RC_ERROR;
    } else {
        rc = vtss_ifindex_decompose(ifindex, ife);
    }
    return rc;
}

mesa_rc vtss_appl_lprot_interface_config_set(vtss_ifindex_t ifindex, const vtss_appl_loop_protect_port_conf_t *conf)
{
    mesa_rc rc;
    vtss_ifindex_elm_t ife;
    if ((rc = lprot_if2ife(ifindex, &ife)) == VTSS_RC_OK) {
        rc = vtss_appl_loop_protect_conf_port_set(ife.isid, ife.ordinal, conf);
    }
    return rc;
}

mesa_rc vtss_appl_lprot_interface_config_get(vtss_ifindex_t ifindex, vtss_appl_loop_protect_port_conf_t *conf)
{
    mesa_rc rc;
    vtss_ifindex_elm_t ife;
    if ((rc = lprot_if2ife(ifindex, &ife)) == VTSS_RC_OK) {
        rc = vtss_appl_loop_protect_conf_port_get(ife.isid, ife.ordinal, conf);
    }
    return rc;
}

mesa_rc vtss_appl_lprot_interface_status_get(vtss_ifindex_t ifindex, vtss_appl_loop_protect_port_info_t *status)
{
    mesa_rc rc;
    vtss_ifindex_elm_t ife;
    if ((rc = lprot_if2ife(ifindex, &ife)) == VTSS_RC_OK) {
        rc = vtss_appl_loop_protect_port_info_get(ife.isid, ife.ordinal, status);
    }
    return rc;
}
