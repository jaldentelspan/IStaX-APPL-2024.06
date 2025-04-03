/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "main.h"
#include "critd_api.h"
#include "port_iter.hxx"
#include "vcl_api.h"
#include "vlan_api.h"
#include "misc_api.h"
#include "vcl_trace.h"
#include "vtss/appl/types.hxx"

/* Pricate helper functions */
static BOOL check_vid(mesa_vid_t vid)
{
    if (vid < VTSS_APPL_VLAN_ID_MIN || vid > VTSS_APPL_VLAN_ID_MAX) {
        return FALSE;
    }
    return TRUE;
}

static BOOL check_mask_len(u8 len)
{
    if (len > 32 || len < 1) {
        return FALSE;
    }
    return TRUE;
}

static BOOL check_proto_group_name(u8 *name)
{
    if (name[0] == '\0') {
        return FALSE;
    }
    for (uint idx = 0; idx < strlen((char *)name); idx++) {
        if ((name[idx] < 48) || (name[idx] > 122)) {
            return FALSE;
        } else {
            if ((name[idx] > 57) && (name[idx] < 65)) {
                return FALSE;
            } else if ((name[idx] > 90) && (name[idx] < 97)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

/****************************************************************************
 * MAC-based VLAN configuration
 ****************************************************************************/

mesa_rc vtss_appl_vcl_mac_table_conf_set(mesa_mac_t mac, const vtss_appl_vcl_generic_conf_global_t *const entry)
{
    vcl_mac_mgmt_vce_conf_local_t conf;
    port_iter_t                   pit;
    switch_iter_t                 sit;
    BOOL                          ports_exist = FALSE, plist_empty = TRUE;
    mesa_rc                       rc;
    vtss::PortListStackable       &pls = (vtss::PortListStackable &)entry->ports;

    T_DG(TRACE_GRP_MIB, "Enter Add MAC to VID mapping");
    if (entry == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }
    if (!check_vid(entry->vid)) {
        return VCL_ERROR_INVALID_VLAN_ID;
    }
    vtss_clear(conf);
    memcpy(&conf.smac, &mac, sizeof(mac));
    conf.vid = entry->vid;
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        ports_exist = FALSE;
        while (port_iter_getnext(&pit)) {
            conf.ports[pit.iport] = pls.get(sit.isid, pit.iport);
            if (conf.ports[pit.iport]) {
                ports_exist = TRUE;
                plist_empty = FALSE;
            }
        }
        if (ports_exist) {
            if ((rc = vcl_mac_mgmt_conf_add(sit.isid, &conf)) != VTSS_RC_OK) {
                return rc;
            }
        }
    }
    if (plist_empty) {
        return VCL_ERROR_EMPTY_PORT_LIST;
    }
    T_DG(TRACE_GRP_MIB, "Exit Add MAC to VID mapping");
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_mac_table_conf_del(mesa_mac_t mac)
{
    vcl_mac_mgmt_vce_conf_global_t conf;
    switch_iter_t                  sit;
    port_iter_t                    pit;
    BOOL                           found_sid;
    mesa_rc                        rc = VTSS_RC_OK;

    T_DG(TRACE_GRP_MIB, "Enter Delete MAC to VID mapping");
    vtss_clear(conf);
    memcpy(&conf.smac, &mac, sizeof(mac));
    if ((rc = vcl_mac_mgmt_conf_get(VTSS_ISID_GLOBAL, &conf, FALSE, FALSE)) != VTSS_RC_OK) {
        return rc;
    }
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        found_sid = FALSE;
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (conf.ports[sit.isid - VTSS_ISID_START][pit.iport] == 1) {
                found_sid = TRUE;
                break;
            }
        }
        if (found_sid) {
            if ((rc = vcl_mac_mgmt_conf_del(sit.isid, &mac)) != VTSS_RC_OK) {
                return rc;
            }
        }
    }
    T_DG(TRACE_GRP_MIB, "Exit Delete MAC to VID mapping");
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_mac_table_conf_get(mesa_mac_t mac, vtss_appl_vcl_generic_conf_global_t *entry)
{
    vcl_mac_mgmt_vce_conf_global_t conf;
    switch_iter_t                  sit;
    port_iter_t                    pit;
    mesa_rc                        rc;
    vtss::PortListStackable        &pls = (vtss::PortListStackable &)entry->ports;

    T_DG(TRACE_GRP_MIB, "Enter Get MAC to VID mapping");
    vtss_clear(conf);
    memset(entry, 0, sizeof(*entry));
    memcpy(&conf.smac, &mac, sizeof(mac));
    if ((rc = vcl_mac_mgmt_conf_get(VTSS_ISID_GLOBAL, &conf, FALSE, FALSE)) != VTSS_RC_OK) {
        return rc;
    } else {
        entry->vid = conf.vid;
        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
            (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                T_DG(TRACE_GRP_MIB, "setting stackable port list for isid %u and iport %u", sit.isid, pit.iport);
                if (conf.ports[sit.isid - VTSS_ISID_START][pit.iport]) {
                    pls.set(sit.isid, pit.iport);
                } else {
                    pls.clear(sit.isid, pit.iport);
                }
            }
        }
    }
    T_DG(TRACE_GRP_MIB, "Exit Get MAC to VID mapping");
    return VTSS_RC_OK;
}


mesa_rc vtss_appl_vcl_mac_table_conf_itr(const mesa_mac_t *const prev_mac, mesa_mac_t *const next_mac)
{
    mesa_mac_t mac;
    mesa_rc    rc;
    BOOL       first;

    T_DG(TRACE_GRP_MIB, "Enter MAC Table iterator");
    memset(&mac, 0, sizeof(mac));
    if (prev_mac == NULL) {
        // Get first address
        first = TRUE;
    } else {
        // Have previous MAC, get next MAC
        first = FALSE;
        memcpy(&mac, prev_mac, sizeof(mac));
    }
    if ((rc = vcl_mac_mgmt_conf_itr(&mac, first)) != VTSS_RC_OK) {
        return rc;
    } else {
        memcpy(next_mac, &mac, sizeof(*next_mac));
    }
    T_DG(TRACE_GRP_MIB, "Exit MAC Table iterator");
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_mac_table_conf_def(mesa_mac_t *mac, vtss_appl_vcl_generic_conf_global_t *entry)
{
    vtss::PortListStackable &pls = (vtss::PortListStackable &)entry->ports;

    for (uint i = 0; i < 6; i++) {
        mac->addr[i] = 0;
    }
    entry->vid = 1;
    pls.clear_all();
    return VTSS_RC_OK;
}
/****************************************************************************/

/****************************************************************************
 * Subnet-based VLAN configuration
 ****************************************************************************/

mesa_rc vtss_appl_vcl_ip_table_conf_set(mesa_ipv4_network_t subnet, const vtss_appl_vcl_generic_conf_global_t *const entry)
{
    vcl_ip_mgmt_vce_conf_local_t conf;
    port_iter_t                  pit;
    switch_iter_t                sit;
    BOOL                         ports_exist = FALSE, plist_empty = TRUE;
    mesa_rc                      rc;
    vtss::PortListStackable      &pls = (vtss::PortListStackable &)entry->ports;

    T_DG(TRACE_GRP_MIB, "Enter Add Subnet to VID mapping");
    if (entry == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }
    if (!check_vid(entry->vid)) {
        return VCL_ERROR_INVALID_VLAN_ID;
    }
    if (!check_mask_len((u8)subnet.prefix_size)) {
        return VCL_ERROR_INVALID_MASK_LENGTH;
    }
    if (!subnet.address) {
        return VCL_ERROR_INVALID_SUBNET;
    }
    vtss_clear(conf);
    conf.ip_addr = subnet.address;
    conf.mask_len = (u8)subnet.prefix_size;
    conf.vid = entry->vid;
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        ports_exist = FALSE;
        while (port_iter_getnext(&pit)) {
            conf.ports[pit.iport] = pls.get(sit.isid, pit.iport);
            if (conf.ports[pit.iport]) {
                ports_exist = TRUE;
                plist_empty = FALSE;
            }
        }
        if (ports_exist) {
            if ((rc = vcl_ip_mgmt_conf_add(sit.isid, &conf)) != VTSS_RC_OK) {
                return rc;
            }
        }
    }
    if (plist_empty) {
        return VCL_ERROR_EMPTY_PORT_LIST;
    }
    T_DG(TRACE_GRP_MIB, "Exit Add Subnet to VID mapping");
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_ip_table_conf_del(mesa_ipv4_network_t subnet)
{
    vcl_ip_mgmt_vce_conf_global_t gconf;
    vcl_ip_mgmt_vce_conf_local_t  conf;
    switch_iter_t                 sit;
    port_iter_t                   pit;
    BOOL                          found_sid;
    char                          ip_buf[100];
    mesa_rc                       rc = VTSS_RC_OK;

    T_DG(TRACE_GRP_MIB, "Enter Delete Subnet to VID mapping");
    T_DG(TRACE_GRP_MIB, "Got Subnet: %s/%u", misc_ipv4_txt(subnet.address, ip_buf), subnet.prefix_size);
    vtss_clear(gconf);
    gconf.ip_addr = subnet.address;
    gconf.mask_len = (u8)subnet.prefix_size;
    if ((rc = vcl_ip_mgmt_conf_get(VTSS_ISID_GLOBAL, &gconf, FALSE, FALSE)) != VTSS_RC_OK) {
        return rc;
    }
    vtss_clear(conf);
    conf.ip_addr = subnet.address;
    conf.mask_len = (u8)subnet.prefix_size;
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        found_sid = FALSE;
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (gconf.ports[sit.isid - VTSS_ISID_START][pit.iport] == 1) {
                found_sid = TRUE;
                break;
            }
        }
        if (found_sid) {
            if ((rc = vcl_ip_mgmt_conf_del(sit.isid, &conf)) != VTSS_RC_OK) {
                return rc;
            }
        }
    }
    T_DG(TRACE_GRP_MIB, "Exit Delete Subnet to VID mapping");
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_ip_table_conf_get(mesa_ipv4_network_t subnet, vtss_appl_vcl_generic_conf_global_t *entry)
{
    vcl_ip_mgmt_vce_conf_global_t conf;
    switch_iter_t                 sit;
    port_iter_t                   pit;
    mesa_rc                       rc;
    vtss::PortListStackable       &pls = (vtss::PortListStackable &)entry->ports;

    T_DG(TRACE_GRP_MIB, "Enter Get Subnet to VID mapping");
    vtss_clear(conf);
    memset(entry, 0, sizeof(*entry));
    conf.ip_addr = subnet.address;
    conf.mask_len = (u8)subnet.prefix_size;
    if ((rc = vcl_ip_mgmt_conf_get(VTSS_ISID_GLOBAL, &conf, FALSE, FALSE)) != VTSS_RC_OK) {
        return rc;
    } else {
        entry->vid = conf.vid;
        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
            (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (conf.ports[sit.isid - VTSS_ISID_START][pit.iport]) {
                    pls.set(sit.isid, pit.iport);
                } else {
                    pls.clear(sit.isid, pit.iport);
                }
            }
        }
    }
    T_DG(TRACE_GRP_MIB, "Exit Get Subnet to VID mapping");
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_ip_table_conf_itr(const mesa_ipv4_network_t *const prev_subnet, mesa_ipv4_network_t *const next_subnet)
{
    mesa_ipv4_network_t sub;
    mesa_rc             rc;
    BOOL                first;

    T_DG(TRACE_GRP_MIB, "Enter Subnet Table iterator");
    memset(&sub, 0, sizeof(sub));
    if (prev_subnet == NULL) {
        // Get first address
        first = TRUE;
    } else {
        // Have previous, get next
        first = FALSE;
        sub = *prev_subnet;
    }
    if ((rc = vcl_ip_mgmt_conf_itr(&sub, first)) != VTSS_RC_OK) {
        return rc;
    } else {
        *next_subnet = sub;
    }
    T_DG(TRACE_GRP_MIB, "Exit Subnet Table iterator");
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_ip_table_conf_def(mesa_ipv4_network_t *subnet, vtss_appl_vcl_generic_conf_global_t *entry)
{
    vtss::PortListStackable &pls = (vtss::PortListStackable &)entry->ports;

    subnet->address = 0;
    subnet->prefix_size = 24;
    entry->vid = 1;
    pls.clear_all();
    return VTSS_RC_OK;
}
/****************************************************************************/

/****************************************************************************
 * Protocol-based VLAN configuration
 ****************************************************************************/
static mesa_rc vtss_appl_vcl_proto_encapsulation_check(vtss_appl_vcl_proto_t protocol)
{
    switch (protocol.proto_encap_type) {
    case VTSS_APPL_VCL_PROTO_ENCAP_ETH2:
        if (protocol.proto.eth2_proto.eth_type < 1536) {
            return VCL_ERROR_INVALID_ENCAP;
        }
        return VTSS_RC_OK;

    case VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP:
        if (protocol.proto.llc_snap_proto.oui[0] == 0 &&
            protocol.proto.llc_snap_proto.oui[1] == 0 &&
            protocol.proto.llc_snap_proto.oui[2] == 0) {
            if (protocol.proto.llc_snap_proto.pid < 1536) {
                return VCL_ERROR_INVALID_ENCAP;
            }
        }
        return VTSS_RC_OK;

    case VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER:
        return VTSS_RC_OK;

    default:
        return VCL_ERROR_INVALID_ENCAP;
    }
}

mesa_rc vtss_appl_vcl_proto_table_proto_set(vtss_appl_vcl_proto_t protocol, const vtss_appl_vcl_proto_group_conf_proto_t *const entry)
{
    vcl_proto_mgmt_group_conf_proto_t conf;
    mesa_rc                           rc;

    T_DG(TRACE_GRP_MIB, "Enter Add Protocol to Group mapping");
    if (entry == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }
    if ((rc = vtss_appl_vcl_proto_encapsulation_check(protocol)) != VTSS_RC_OK) {
        T_DG(TRACE_GRP_MIB, "Invalid Protocol encapsulation type!");
        return rc;
    }
    if (!check_proto_group_name((u8 *)entry->name)) {
        return VCL_ERROR_INVALID_GROUP_NAME;
    }
    memset(&conf, 0, sizeof(conf));
    memcpy(conf.name, entry->name, VTSS_APPL_VCL_MAX_GROUP_NAME_LEN);
    conf.name[strlen((char *)entry->name)] = '\0';
    conf.proto_encap_type = protocol.proto_encap_type;
    conf.proto = protocol.proto;
    if ((rc = vcl_proto_mgmt_proto_add(&conf)) != VTSS_RC_OK) {
        if (rc == VCL_ERROR_PROTOCOL_ALREADY_CONF) {
            if ((rc = vcl_proto_mgmt_proto_del(&conf)) != VTSS_RC_OK) {
                return rc;
            } else {
                if ((rc = vcl_proto_mgmt_proto_add(&conf)) != VTSS_RC_OK) {
                    return rc;
                }
            }
        } else {
            return rc;
        }
    }
    T_DG(TRACE_GRP_MIB, "Exit Add Protocol to Group mapping");
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_proto_table_proto_del(vtss_appl_vcl_proto_t protocol)
{
    vcl_proto_mgmt_group_conf_proto_t conf;
    mesa_rc                           rc;

    T_DG(TRACE_GRP_MIB, "Enter Delete Protocol from Group mapping");
    if ((rc = vtss_appl_vcl_proto_encapsulation_check(protocol)) != VTSS_RC_OK) {
        T_DG(TRACE_GRP_MIB, "Invalid Protocol encapsulation type!");
        return rc;
    }
    memset(&conf, 0, sizeof(conf));
    conf.proto_encap_type = protocol.proto_encap_type;
    conf.proto = protocol.proto;
    if ((rc = vcl_proto_mgmt_proto_del(&conf)) != VTSS_RC_OK) {
        return rc;
    }
    T_DG(TRACE_GRP_MIB, "Exit Delete Protocol from Group mapping");
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_proto_table_proto_get(vtss_appl_vcl_proto_t protocol, vtss_appl_vcl_proto_group_conf_proto_t *entry)
{
    vcl_proto_mgmt_group_conf_proto_t conf;
    mesa_rc                           rc;

    T_DG(TRACE_GRP_MIB, "Enter Get Protocol to Group mapping");
    memset(&conf, 0, sizeof(conf));
    memset(entry, 0, sizeof(*entry));
    conf.proto_encap_type = protocol.proto_encap_type;
    conf.proto = protocol.proto;
    if ((rc = vcl_proto_mgmt_proto_get(&conf, FALSE, FALSE)) != VTSS_RC_OK) {
        return rc;
    } else {
        memcpy(entry->name, conf.name, VTSS_APPL_VCL_MAX_GROUP_NAME_LEN);
    }
    T_DG(TRACE_GRP_MIB, "Exit Get Protocol to Group mapping");
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_proto_table_proto_itr(const vtss_appl_vcl_proto_t *const prev_protocol, vtss_appl_vcl_proto_t *const next_protocol)
{
    vtss_appl_vcl_proto_t enc;
    mesa_rc               rc;
    BOOL                  first;

    T_DG(TRACE_GRP_MIB, "Enter Protocol Table iterator");
    memset(&enc, 0, sizeof(enc));
    if (prev_protocol == NULL) {
        // Get first address
        first = TRUE;
    } else {
        // Have previous, get next
        first = FALSE;
        enc = *prev_protocol;
    }
    if ((rc = vcl_proto_mgmt_proto_itr(&enc, first)) != VTSS_RC_OK) {
        return rc;
    } else {
        *next_protocol = enc;
    }
    T_DG(TRACE_GRP_MIB, "Exit Protocol Table iterator");
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_proto_table_proto_def(vtss_appl_vcl_proto_t *protocol,
                                            vtss_appl_vcl_proto_group_conf_proto_t *entry)
{
    protocol->proto_encap_type = VTSS_APPL_VCL_PROTO_ENCAP_ETH2;
    protocol->proto.eth2_proto.eth_type = 1536;
    strncpy((char *)entry->name, "default", VTSS_APPL_VCL_MAX_GROUP_NAME_LEN);
    entry->name[VTSS_APPL_VCL_MAX_GROUP_NAME_LEN -  1] = '\0';
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_proto_table_conf_set(vtss_appl_vcl_proto_group_conf_proto_t group,
                                           const vtss_appl_vcl_generic_conf_global_t *const entry)
{
    vcl_proto_mgmt_group_conf_entry_local_t conf;
    port_iter_t                             pit;
    switch_iter_t                           sit;
    BOOL                                    ports_exist = FALSE, plist_empty = TRUE;
    mesa_rc                                 rc;
    vtss::PortListStackable                 &pls = (vtss::PortListStackable &)entry->ports;

    T_DG(TRACE_GRP_MIB, "Enter Add Group to VID mapping");
    if (entry == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }
    if (!check_proto_group_name(group.name)) {
        return VCL_ERROR_INVALID_GROUP_NAME;
    }
    if (!check_vid(entry->vid)) {
        return VCL_ERROR_INVALID_VLAN_ID;
    }
    vtss_clear(conf);
    memcpy(conf.name, group.name, VTSS_APPL_VCL_MAX_GROUP_NAME_LEN);
    conf.vid = entry->vid;
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        ports_exist = FALSE;
        while (port_iter_getnext(&pit)) {
            conf.ports[pit.iport] = pls.get(sit.isid, pit.iport);
            if (conf.ports[pit.iport]) {
                ports_exist = TRUE;
                plist_empty = FALSE;
            }
        }
        if (ports_exist) {
            if ((rc = vcl_proto_mgmt_conf_add(sit.isid, &conf)) != VTSS_RC_OK) {
                return rc;
            }
        }
    }
    if (plist_empty) {
        return VCL_ERROR_EMPTY_PORT_LIST;
    }
    T_DG(TRACE_GRP_MIB, "Exit Add Group to VID mapping");
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_proto_table_conf_del(vtss_appl_vcl_proto_group_conf_proto_t group)
{
    vcl_proto_mgmt_group_conf_entry_global_t gconf;
    vcl_proto_mgmt_group_conf_entry_local_t  conf;
    switch_iter_t                            sit;
    port_iter_t                              pit;
    BOOL                                     found_sid;
    mesa_rc                                  rc = VTSS_RC_OK;

    T_DG(TRACE_GRP_MIB, "Enter Delete Group to VID mapping");
    vtss_clear(gconf);
    memcpy(gconf.name, group.name, VTSS_APPL_VCL_MAX_GROUP_NAME_LEN);
    if ((rc = vcl_proto_mgmt_conf_get(VTSS_ISID_GLOBAL, &gconf, FALSE, FALSE)) != VTSS_RC_OK) {
        return rc;
    }
    vtss_clear(conf);
    memcpy(conf.name, group.name, VTSS_APPL_VCL_MAX_GROUP_NAME_LEN);
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        found_sid = FALSE;
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (gconf.ports[sit.isid - VTSS_ISID_START][pit.iport] == 1) {
                found_sid = TRUE;
                break;
            }
        }
        if (found_sid) {
            if ((rc = vcl_proto_mgmt_conf_del(sit.isid, &conf)) != VTSS_RC_OK) {
                return rc;
            }
        }
    }
    T_DG(TRACE_GRP_MIB, "Exit Delete Group to VID mapping");
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_proto_table_conf_get(vtss_appl_vcl_proto_group_conf_proto_t group,
                                           vtss_appl_vcl_generic_conf_global_t *entry)
{
    vcl_proto_mgmt_group_conf_entry_global_t conf;
    switch_iter_t                            sit;
    port_iter_t                              pit;
    mesa_rc                                  rc;
    vtss::PortListStackable                  &pls = (vtss::PortListStackable &)entry->ports;

    T_DG(TRACE_GRP_MIB, "Enter Get Group to VID mapping");
    vtss_clear(conf);
    memset(entry, 0, sizeof(*entry));
    memcpy(conf.name, group.name, VTSS_APPL_VCL_MAX_GROUP_NAME_LEN);
    if ((rc = vcl_proto_mgmt_conf_get(VTSS_ISID_GLOBAL, &conf, FALSE, FALSE)) != VTSS_RC_OK) {
        return rc;
    } else {
        entry->vid = conf.vid;
        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
            (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (conf.ports[sit.isid - VTSS_ISID_START][pit.iport]) {
                    pls.set(sit.isid, pit.iport);
                } else {
                    pls.clear(sit.isid, pit.iport);
                }
            }
        }
    }
    T_DG(TRACE_GRP_MIB, "Exit Get Group to VID mapping");
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_proto_table_conf_itr(const vtss_appl_vcl_proto_group_conf_proto_t *const prev_group,
                                           vtss_appl_vcl_proto_group_conf_proto_t       *const next_group)
{
    vtss_appl_vcl_proto_group_conf_proto_t group;
    mesa_rc                                rc;
    BOOL                                   first;

    T_DG(TRACE_GRP_MIB, "Enter Protocol Group Table iterator");
    memset(&group, 0, sizeof(group));
    if (prev_group == NULL) {
        // Get first address
        first = TRUE;
    } else {
        // Have previous, get next
        first = FALSE;
        memcpy(group.name, prev_group->name, VTSS_APPL_VCL_MAX_GROUP_NAME_LEN);
    }
    if ((rc = vcl_proto_mgmt_conf_itr(&group, first)) != VTSS_RC_OK) {
        return rc;
    } else {
        memcpy(next_group->name, group.name, VTSS_APPL_VCL_MAX_GROUP_NAME_LEN);
    }
    T_DG(TRACE_GRP_MIB, "Exit Protocol Group Table iterator");
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vcl_proto_table_conf_def(vtss_appl_vcl_proto_group_conf_proto_t *group,
                                           vtss_appl_vcl_generic_conf_global_t    *entry)
{
    vtss::PortListStackable &pls = (vtss::PortListStackable &)entry->ports;

    strncpy((char *)group->name, "default", VTSS_APPL_VCL_MAX_GROUP_NAME_LEN);
    group->name[VTSS_APPL_VCL_MAX_GROUP_NAME_LEN -  1] = '\0';
    entry->vid = 1;
    pls.clear_all();
    return VTSS_RC_OK;
}
