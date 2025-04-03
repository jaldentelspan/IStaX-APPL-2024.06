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

#include "vtss/appl/udld.h"
#include "udld_serializer.hxx"

const vtss_enum_descriptor_t vtss_appl_udld_mode_txt[] {
    {VTSS_APPL_UDLD_MODE_DISABLE, "disable"},
    {VTSS_APPL_UDLD_MODE_NORMAL, "normal"},
    {VTSS_APPL_UDLD_MODE_AGGRESSIVE, "aggressive"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_udld_detection_state_txt[] {
    {VTSS_UDLD_DETECTION_STATE_UNKNOWN, "inDeterminant"},
    {VTSS_UDLD_DETECTION_STATE_UNI_DIRECTIONAL, "uniDirectional"},
    {VTSS_UDLD_DETECTION_STATE_BI_DIRECTIONAL, "biDirectional"},
    {VTSS_UDLD_DETECTION_STATE_NEIGHBOR_MISMATCH, "neighborMismatch"},
    {VTSS_UDLD_DETECTION_STATE_LOOPBACK, "loopback"},
    {VTSS_UDLD_DETECTION_STATE_MULTIPLE_NEIGHBOR, "multipleNeighbor"},
    {0, 0},
};

mesa_rc udld_if2ife(vtss_ifindex_t ifindex, vtss_ifindex_elm_t *ife) {
    mesa_rc rc;
    if (!vtss_ifindex_is_port(ifindex)) {
        rc = VTSS_RC_ERROR;
    } else {
        rc = vtss_ifindex_decompose(ifindex, ife);
    }
    return rc;
}

mesa_rc vtss_appl_udld_interface_config_set(vtss_ifindex_t ifindex,
                                            const vtss_appl_udld_port_conf_struct_t *conf) {
    mesa_rc rc;
    vtss_ifindex_elm_t ife;
    if ((rc = udld_if2ife(ifindex, &ife)) == VTSS_RC_OK) {
        rc = vtss_appl_udld_port_conf_set(ife.isid, ife.ordinal, conf);
    }
    return rc;
}

mesa_rc vtss_appl_udld_interface_config_get(vtss_ifindex_t ifindex,
                                            vtss_appl_udld_port_conf_struct_t *conf) {
    mesa_rc rc;
    vtss_ifindex_elm_t ife;
    if ((rc = udld_if2ife(ifindex, &ife)) == VTSS_RC_OK) {
        rc = vtss_appl_udld_port_conf_get(ife.isid, ife.ordinal, conf);
    }
    return rc;
}

mesa_rc vtss_appl_udld_interface_status_get(vtss_ifindex_t ifindex,
                                            vtss_appl_udld_port_info_t *status) {
    mesa_rc rc;
    vtss_ifindex_elm_t ife;
    if ((rc = udld_if2ife(ifindex, &ife)) == VTSS_RC_OK) {
        rc = vtss_appl_udld_port_info_get(ife.isid, ife.ordinal, status);
    }
    return rc;

}

mesa_rc vtss_appl_udld_interface_neighbor_status_get(vtss_ifindex_t ifindex,
                                                     vtss_appl_udld_neighbor_info_t *status) {
    mesa_rc rc;
    vtss_ifindex_elm_t ife;
    if ((rc = udld_if2ife(ifindex, &ife)) == VTSS_RC_OK) {
        rc = vtss_appl_udld_neighbor_info_get_first(ife.isid, ife.ordinal, status);
    }
    return rc;

}
