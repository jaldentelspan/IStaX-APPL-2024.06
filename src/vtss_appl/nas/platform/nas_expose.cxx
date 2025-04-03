/*

 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "nas_serializer.hxx"
#include "vtss/appl/nas.h"
#include "vtss/appl/vlan.h" // For VTSS_APPL_VLAN_ID_DEFAULT
#include "vtss_common_iterator.hxx" /* For vtss_appl_iterator_ifindex_front_port*/
#include "vtss/basics/expose/snmp/iterator-compose-range.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-N.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-depend-N.hxx"
#include "dot1x_trace.h"
/****************************************************************************
 * Get/Set functions
 ****************************************************************************/
/* ----------------- dummy get methods, for WriteOnly tables ---------------*/
mesa_rc vtss_nas_ser_clr_statistics_dummy_get(vtss_ifindex_t ifindex, BOOL *const clear) {
    if (clear && *clear) {
        *clear = FALSE;
    }
    return VTSS_RC_OK;
}

/* ----------------- dummy get methods, for WriteOnly tables ---------------*/
mesa_rc vtss_nas_ser_reauth_dummy_get(vtss_ifindex_t ifindex, BOOL *const now) {
    if (now && *now) {
        *now = FALSE;
    }
    return VTSS_RC_OK;
}

/* ---------------- NAS set methods, for WriteOnly tables -----------------*/
mesa_rc vtss_nas_ser_clr_statistics_set(vtss_ifindex_t ifindex, BOOL const *clear) {

    if (clear && *clear) {
        return vtss_appl_nas_port_statistics_clear(ifindex);
    }

    return VTSS_RC_OK;
}

/* ---------------- NAS get status with valid vid (Work-around for Bugzilla#19279)-----------------*/
mesa_rc vtss_nas_ser_port_status_get(vtss_ifindex_t ifindex, vtss_appl_nas_interface_status_t *const status) {
    VTSS_RC(vtss_appl_nas_port_status_get(ifindex, status));

    if (status->vlan_type == VTSS_APPL_NAS_VLAN_TYPE_NONE) {
        // When the VLAN type is NONE, the NAS module sets vid = 0, which is invalid. Make vid valid. See Bugzilla#19279
        status->vid = VTSS_APPL_VLAN_ID_DEFAULT;
    }

    return VTSS_RC_OK;
}

/* ---------------- NAS get supplicant info with valid vid (Work-around for Bugzilla#19279)-----------------*/
mesa_rc vtss_nas_ser_last_supplicant_info_get(vtss_ifindex_t ifindex, vtss_appl_nas_client_info_t *last_supplicant_info) {
    VTSS_RC(vtss_appl_nas_last_supplicant_info_get(ifindex, last_supplicant_info));

    if (last_supplicant_info->vid_mac.vid == 0) {
        // When last_supplicant_info is "unknown", the NAS module sets vid = 0, which is invalid. Make vid valid.
        last_supplicant_info->vid_mac.vid = VTSS_APPL_VLAN_ID_DEFAULT;
    }

    return VTSS_RC_OK;
}

/****************************************************************************
 * Descriptions
 ****************************************************************************/
vtss_enum_descriptor_t vtss_nas_qos_class_prio_txt[] {
    {VTSS_APPL_NAS_QOS_CLASS_0,     "pcp0"},
    {VTSS_APPL_NAS_QOS_CLASS_1,     "pcp1"},
    {VTSS_APPL_NAS_QOS_CLASS_2,     "pcp2"},
    {VTSS_APPL_NAS_QOS_CLASS_3,     "pcp3"},
    {VTSS_APPL_NAS_QOS_CLASS_4,     "pcp4"},
    {VTSS_APPL_NAS_QOS_CLASS_5,     "pcp5"},
    {VTSS_APPL_NAS_QOS_CLASS_6,     "pcp6"},
    {VTSS_APPL_NAS_QOS_CLASS_7,     "pcp7"},
    {VTSS_APPL_NAS_QOS_CLASS_NONE,  "none"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_nas_port_status_txt[] {
    {VTSS_APPL_NAS_PORT_STATUS_LINK_DOWN,      "linkDown"},
    {VTSS_APPL_NAS_PORT_STATUS_AUTHORIZED,     "authorized"},
    {VTSS_APPL_NAS_PORT_STATUS_UNAUTHORIZED,   "unAuthorized"},
    {VTSS_APPL_NAS_PORT_STATUS_DISABLED,       "disabled"},
    {VTSS_APPL_NAS_PORT_STATUS_MULTI,          "multiMode"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_nas_vlan_type_txt[] {
    {VTSS_APPL_NAS_VLAN_TYPE_NONE,             "none"},
    {VTSS_APPL_NAS_VLAN_TYPE_BACKEND_ASSIGNED, "radiusAssigned"},
    {VTSS_APPL_NAS_VLAN_TYPE_GUEST_VLAN,       "guestVlan"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_nas_port_control_txt[] {
    {VTSS_APPL_NAS_VLAN_TYPE_NONE,                  "none"},
    {VTSS_APPL_NAS_PORT_CONTROL_FORCE_AUTHORIZED,   "forceAuthorized"},
    {VTSS_APPL_NAS_PORT_CONTROL_AUTO,               "auto"},
    {VTSS_APPL_NAS_PORT_CONTROL_FORCE_UNAUTHORIZED, "forceUnAuthorized"},
    {VTSS_APPL_NAS_PORT_CONTROL_MAC_BASED,          "macBased"},
    {VTSS_APPL_NAS_PORT_CONTROL_DOT1X_SINGLE,       "dot1xSingle"},
    {VTSS_APPL_NAS_PORT_CONTROL_DOT1X_MULTI,        "dot1xmulti"},
    {0, 0}
};
