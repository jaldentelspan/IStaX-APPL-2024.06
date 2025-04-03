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

#include "vcl_api.h"
#include "vcl.h"
#include <vlan_api.h>
#include <port_iter.hxx>
#include "msg_api.h"
#include "conf_api.h"               // For conf_mgmt_mac_addr_get()
#include "port_api.h"               // For port_count_max()
#include <vtss_trace_lvl_api.h>
#include <vcl_trace.h>
#include <misc_api.h>
#include <vtss_trace_api.h>
#include <vtss/basics/map.hxx>
#include <vtss/basics/set.hxx>
#include <vtss/basics/list.hxx>
#include <vtss/appl/vcl.h>

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void vcl_mib_init(void);
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vcl_json_init(void);
#endif

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "vcl", "VCL table"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING,
        VTSS_TRACE_FLAGS_USEC,
    },
    [TRACE_GRP_ICLI] = {
        "icli",
        "ICLI Interface",
        VTSS_TRACE_LVL_WARNING,
        VTSS_TRACE_FLAGS_USEC
    },
    [TRACE_GRP_WEB] = {
        "web",
        "WEB Interface",
        VTSS_TRACE_LVL_WARNING,
        VTSS_TRACE_FLAGS_USEC,
    },
    [TRACE_GRP_MIB] = {
        "mib",
        "MIB",
        VTSS_TRACE_LVL_WARNING,
        VTSS_TRACE_FLAGS_USEC,
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define VCL_CRIT_ENTER() critd_enter(&vcl_data.crit, __FUNCTION__, __LINE__)
#define VCL_CRIT_EXIT()  critd_exit( &vcl_data.crit, __FUNCTION__, __LINE__)

static vcl_data_t vcl_data;

/* Request structure required by the message module */
static void *vcl_request_pool;

/* Debug policy number */
static mesa_acl_policy_no_t vcl_debug_policy_no = MESA_ACL_POLICY_NO_NONE;

/******************************************************************************/
// VCL_mesa_vcap_key_type_to_str()
/******************************************************************************/
static const char *VCL_mesa_vcap_key_type_to_str(mesa_vcap_key_type_t key_type)
{
    switch (key_type) {
    case MESA_VCAP_KEY_TYPE_NORMAL:
        return "Normal: Half Key, SIP only";

    case MESA_VCAP_KEY_TYPE_DOUBLE_TAG:
        return "Double Tag: Quarter key, two tags";

    case MESA_VCAP_KEY_TYPE_IP_ADDR:
        return "IP Address: Half key, SIP and DIP";

    case MESA_VCAP_KEY_TYPE_MAC_IP_ADDR:
        return "MAC/IP address: Full key, MAC and IP addresses";

    default:
        T_E("Unknown key_type (%d)", key_type);
        return "Unknown";
    }
}

/******************************************************************************/
// mesa_vcl_port_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vcl_port_conf_t &conf)
{
    o << "{dmac_dip = "  << conf.dmac_dip
      << ", key_type = " << VCL_mesa_vcap_key_type_to_str(conf.key_type)
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vcl_port_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_vcl_port_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

static const char *vcl_msg_id_txt(vcl_msg_id_t msg_id)
{
    const char *txt;

    switch (msg_id) {
    case VCL_MSG_ID_MAC_VCE_SET:
        txt = "VCL_MAC_VCE_SET";
        break;
    case VCL_MSG_ID_MAC_VCE_ADD:
        txt = "VCL_MAC_VCE_ADD";
        break;
    case VCL_MSG_ID_MAC_VCE_DEL:
        txt = "VCL_MAC_VCE_DEL";
        break;
    case VCL_MSG_ID_PROTO_VCE_SET:
        txt = "VCL_PROTO_VCE_SET";
        break;
    case VCL_MSG_ID_PROTO_VCE_ADD:
        txt = "VCL_PROTO_VCE_ADD";
        break;
    case VCL_MSG_ID_PROTO_VCE_DEL:
        txt = "VCL_PROTO_VCE_DEL";
        break;
    case VCL_MSG_ID_IP_VCE_SET:
        txt = "VCL_IP_VCE_SET";
        break;
    case VCL_MSG_ID_IP_VCE_ADD:
        txt = "VCL_IP_VCE_ADD";
        break;
    case VCL_MSG_ID_IP_VCE_DEL:
        txt = "VCL_IP_VCE_DEL";
        break;
    default:
        txt = "?";
        break;
    }
    return txt;
}

typedef vtss::Map<uint32_t, vtss::List<mesa_vce_id_t> > vcl_extended_vce_map_t;
vcl_extended_vce_map_t vcl_extended_vce_map;

/******************************************************************************/
// VCL_mesa_vcl_port_conf_set()
// We always match on full rules, which means that users can configure e.g.
// streams to match on both DMAC and SMAC and SIP and DIP at the same time.
// The old "vcl {dmacdip | smacsip}" command is still implemented, but leads to
// doing nothing, because it's only needed when using MESA_VCAP_KEY_TYPE_NORMAL.
/******************************************************************************/
static void VCL_mesa_vcl_port_conf_set(void)
{
    mesa_vcl_port_conf_t             port_conf;
    uint32_t                         port_cnt = port_count_max();
    mesa_port_no_t                   port_no;
    mesa_rc                          rc;

    for (port_no = 0; port_no < port_cnt; port_no++) {
        if ((rc = mesa_vcl_port_conf_get(nullptr, port_no, &port_conf)) != VTSS_RC_OK) {
            T_E_PORT(port_no, "mesa_vcl_port_conf_get() failed: %s", error_txt(rc));
            vtss_clear(port_conf);
        }

        T_I_PORT(port_no, "mesa_vcl_port_conf_get() => %s", port_conf);

        // We always use full keys, because it's hard, if not impossible to
        // handle quarther or half keys in various modules that use VCEs.
        port_conf.key_type = MESA_VCAP_KEY_TYPE_MAC_IP_ADDR;

        T_I_PORT(port_no, "mesa_vcl_port_conf_set(%s)", port_conf);
        if ((rc = mesa_vcl_port_conf_set(nullptr, port_no, &port_conf)) != VTSS_RC_OK) {
            T_E_PORT(port_no, "mesa_vcl_port_conf_set(%s) failed: %s", port_conf, error_txt(rc));
        }
    }
}

mesa_rc vcl_register_vce(uint32_t user, mesa_vce_id_t id, mesa_vce_id_t position)
{
    auto itr = vcl_extended_vce_map.find(user);
    if (itr == vcl_extended_vce_map.end()) {
        // If no VCEs registered for user, then create a list and add the id.
        vcl_extended_vce_map[user].push_back(id);
        return VTSS_RC_OK;
    }
    auto itr_position = itr->second.end();
    for (auto i = itr->second.begin(); i != itr->second.end(); ++i) {
        if (*i == id) {
            // Id already found in list for user. Return error
            return VTSS_RC_ERROR;
        }
        if (*i == position) {
            // insertion point found, but confinue to check list for already created entry
            itr_position = i;
        }
    }

    if (position != 0 && itr_position == itr->second.end()) {
        // position != 0 means you expect to find a element to preceede but none was found
        return VTSS_RC_ERROR;
    }
    // Add new entry before position
    itr->second.insert(itr_position, id);

    return VTSS_RC_OK;
}

mesa_rc vcl_unregister_vce(uint32_t user, mesa_vce_id_t id)
{
    auto itr = vcl_extended_vce_map.find(user);
    if (itr != vcl_extended_vce_map.end()) {
        for (auto i = itr->second.begin(); i != itr->second.end(); ++i) {
            if (*i == id) {
                itr->second.erase(i);
                return VTSS_RC_OK;
            }
        }
    }
    return VTSS_RC_ERROR;
}

mesa_vce_id_t vcl_vce_get_first(uint32_t user)
{
    auto itr = vcl_extended_vce_map.greater_than_or_equal(user);
    if (itr == vcl_extended_vce_map.end()) {
        return MESA_VCE_ID_LAST;
    }

    if (itr->second.empty()) {
        return vcl_vce_get_first((itr->first) + 1);
    }

    return *(itr->second.begin());
}

mesa_vce_id_t vcl_vce_get_end(uint32_t user)
{
    return vcl_vce_get_first(++user);
}

void vcl_vce_debug_print(char *buf, int buf_size)
{
    char *p = buf;

    buf[0] = '\0';
    for (auto itr = vcl_extended_vce_map.begin(); buf_size && itr != vcl_extended_vce_map.end(); ++itr) {
        int n = snprintf(p, buf_size, "User %d: ", itr->first);
        p += n;
        buf_size -= n;
        for (auto i = itr->second.begin(); buf_size && i != itr->second.end(); ++i) {
            n = snprintf(p, buf_size, "%d ", *i);
            p += n;
            buf_size -= n;
        }
        n = snprintf(p, buf_size, "\n");
        p += n;
        buf_size -= n;
    }

}

#define VCL_TEST(P,R) if (R!=P) { printf("Test " #P "==" #R " Failed\n"); }
void vcl_test_vcl_extended_vce_map()
{
    char buf[1024];
    VCL_TEST(vcl_register_vce(2, 20, 0), VTSS_RC_OK);
    VCL_TEST(vcl_register_vce(2, 20, 0), VTSS_RC_ERROR);
    VCL_TEST(vcl_register_vce(2, 21, 20), VTSS_RC_OK);
    VCL_TEST(vcl_register_vce(3, 31, 0), VTSS_RC_OK);
    VCL_TEST(vcl_register_vce(3, 32, 20), VTSS_RC_ERROR);
    VCL_TEST(vcl_register_vce(3, 33, 0), VTSS_RC_OK);
    vcl_vce_debug_print(buf, sizeof(buf));
    sprintf("%s\n", buf);
    VCL_TEST(vcl_vce_get_first(1), 21);
    VCL_TEST(vcl_vce_get_first(2), 21);
    VCL_TEST(vcl_vce_get_first(3), 31);
    VCL_TEST(vcl_vce_get_first(4), 0);
    VCL_TEST(vcl_vce_get_end(1), 21);
    VCL_TEST(vcl_vce_get_end(2), 31);
    VCL_TEST(vcl_vce_get_end(3), 0);
    VCL_TEST(vcl_vce_get_end(4), 0);
    VCL_TEST(vcl_register_vce(2, 22, 20), VTSS_RC_OK);
    VCL_TEST(vcl_unregister_vce(2, 21), VTSS_RC_OK);
    VCL_TEST(vcl_register_vce(2, 21, 0), VTSS_RC_OK);
    VCL_TEST(vcl_vce_get_first(1), 22);
    VCL_TEST(vcl_register_vce(3, 30, 31), VTSS_RC_OK);
    vcl_vce_debug_print(buf, sizeof(buf));
    sprintf("%s\n", buf);
    VCL_TEST(vcl_unregister_vce(2, 21), VTSS_RC_OK);
    VCL_TEST(vcl_unregister_vce(2, 22), VTSS_RC_OK);
    VCL_TEST(vcl_unregister_vce(2, 20), VTSS_RC_OK);
    VCL_TEST(vcl_register_vce(2, 20, 0), VTSS_RC_OK);


}

/* Allocate request/reply buffer */
static void *vcl_msg_alloc(u32 ref_cnt)
{
    void *msg;

    if (ref_cnt == 0) {
        return NULL;
    }

    msg = msg_buf_pool_get(vcl_request_pool);
    VTSS_ASSERT(msg);
    if (ref_cnt > 1) {
        msg_buf_pool_ref_cnt_set(msg, ref_cnt);
    }
    return msg;
}

static void vcl_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
    vcl_msg_id_t msg_id = *(vcl_msg_id_t *)msg;

    T_D("VCL message with id %s was sent", vcl_msg_id_txt(msg_id));
    (void)msg_buf_pool_put(msg);
}

static BOOL vcl_msg_rx(void *contxt, const void *const rx_msg, const size_t len,
                       const vtss_module_id_t modid, const u32 isid);

/* Send the VCL Message */
static void vcl_msg_tx(void *msg, vtss_isid_t isid, size_t len)
{
    vcl_msg_id_t msg_id = *(vcl_msg_id_t *)msg;

    T_D("Switch with isid %d is sending VCL message with id %s and length %zd", isid, vcl_msg_id_txt(msg_id), len);
    //    msg_tx_adv(NULL, vcl_msg_tx_done, MSG_TX_OPT_DONT_FREE, VTSS_MODULE_ID_VCL, isid, msg, len);
    // No stacking, hence short circuit message module
    (void) vcl_msg_rx(NULL, msg, len, VTSS_MODULE_ID_VCL, isid);
    vcl_msg_tx_done(NULL, msg, MSG_TX_RC_OK);
}

static void vcl_default_set(void)
{
    vcl_mac_vce_global_t    *mac_p, **mac_lfree, **mac_lused;
    vcl_mac_vce_local_t     *mac_p_local, **mac_lfree_local, **mac_lused_local;
    vcl_ip_vce_global_t     *ip_p, **ip_lfree, **ip_lused;
    vcl_ip_vce_local_t      *ip_p_local, **ip_lfree_local, **ip_lused_local;
    vcl_proto_group_proto_t *proto_group_proto_p, **proto_group_proto_lfree, **proto_group_proto_lused;
    vcl_proto_group_entry_t *proto_group_entry_p, **proto_group_entry_lfree, **proto_group_entry_lused;
    vcl_proto_vce_global_t  *proto_p, **proto_lfree, **proto_lused;
    vcl_proto_vce_local_t   *proto_p_local, **proto_lfree_local, **proto_lused_local;
    u32                     i;

    VCL_mesa_vcl_port_conf_set();

    /* Initialize MAC-based VLAN Global list */
    mac_lfree = &vcl_data.mac_data.global_free;
    mac_lused = &vcl_data.mac_data.global_used;
    *mac_lfree = NULL;
    *mac_lused = NULL;
    for (i = 0; i < VCL_MAC_VCE_MAX; i++) {
        mac_p = &vcl_data.mac_data.global_table[i];
        mac_p->conf.id = VCL_MAC_VCE_MAX - i;
        mac_p->next = *mac_lfree;
        *mac_lfree = mac_p;
    }

    /* Initialize MAC-based VLAN Local list */
    mac_lfree_local = &vcl_data.mac_data.local_free;
    mac_lused_local = &vcl_data.mac_data.local_used;
    *mac_lfree_local = NULL;
    *mac_lused_local = NULL;
    for (i = 0; i < VCL_MAC_VCE_MAX; i++) {
        mac_p_local = &vcl_data.mac_data.local_table[i];
        mac_p_local->next = *mac_lfree_local;
        *mac_lfree_local = mac_p_local;
    }

    /* Initialize Subnet-based VLAN Global list */
    ip_lfree = &vcl_data.ip_data.global_free;
    ip_lused = &vcl_data.ip_data.global_used;
    *ip_lfree = NULL;
    *ip_lused = NULL;
    for (i = 0; i < VCL_IP_VCE_MAX; i++) {
        ip_p = &vcl_data.ip_data.global_table[i];
        ip_p->conf.id = VCL_IP_VCE_MAX - i;
        ip_p->next = *ip_lfree;
        *ip_lfree = ip_p;
    }

    /* Initialize Subnet-based VLAN Local list */
    ip_lfree_local = &vcl_data.ip_data.local_free;
    ip_lused_local = &vcl_data.ip_data.local_used;
    *ip_lfree_local = NULL;
    *ip_lused_local = NULL;
    for (i = 0; i < VCL_IP_VCE_MAX; i++) {
        ip_p_local = &vcl_data.ip_data.local_table[i];
        ip_p_local->next = *ip_lfree_local;
        *ip_lfree_local = ip_p_local;
    }

    /* Initialize Protocol-based VLAN Group(Protocol) list */
    proto_group_proto_lfree = &vcl_data.proto_data.group_proto_free;
    proto_group_proto_lused = &vcl_data.proto_data.group_proto_used;
    *proto_group_proto_lfree = NULL;
    *proto_group_proto_lused = NULL;
    for (i = 0; i < VCL_PROTO_PROTOCOL_MAX; i++) {
        proto_group_proto_p = &vcl_data.proto_data.group_proto_table[i];
        proto_group_proto_p->next = *proto_group_proto_lfree;
        *proto_group_proto_lfree = proto_group_proto_p;
    }

    /* Initialize Protocol-based VLAN Group(Entry) list */
    proto_group_entry_lfree = &vcl_data.proto_data.group_entry_free;
    proto_group_entry_lused = &vcl_data.proto_data.group_entry_used;
    *proto_group_entry_lfree = NULL;
    *proto_group_entry_lused = NULL;
    for (i = 0; i < VCL_PROTO_VCE_MAX; i++) {
        proto_group_entry_p = &vcl_data.proto_data.group_entry_table[i];
        proto_group_entry_p->next = *proto_group_entry_lfree;
        *proto_group_entry_lfree = proto_group_entry_p;
    }

    /* Initialize Protocol-based VLAN Global list */
    proto_lfree = &vcl_data.proto_data.global_free;
    proto_lused = &vcl_data.proto_data.global_used;
    *proto_lfree = NULL;
    *proto_lused = NULL;
    for (i = 0; i < VCL_PROTO_VCE_MAX; i++) {
        proto_p = &vcl_data.proto_data.global_table[i];
        proto_p->conf.id = VCL_PROTO_VCE_MAX - i;
        proto_p->next = *proto_lfree;
        *proto_lfree = proto_p;
    }

    /* Initialize Protocol-based VLAN Local list */
    proto_lfree_local = &vcl_data.proto_data.local_free;
    proto_lused_local = &vcl_data.proto_data.local_used;
    *proto_lfree_local = NULL;
    *proto_lused_local = NULL;
    for (i = 0; i < VCL_PROTO_VCE_MAX; i++) {
        proto_p_local = &vcl_data.proto_data.local_table[i];
        proto_p_local->next = *proto_lfree_local;
        *proto_lfree_local = proto_p_local;
    }
}

static void vcl_mac_global_default_set(void)
{
    vcl_mac_vce_global_t *mac_p, **lfree, **lused;
    u32                  i;

    VCL_CRIT_ENTER();
    /* Initialize MAC-based VLAN Global list */
    lfree = &vcl_data.mac_data.global_free;
    lused = &vcl_data.mac_data.global_used;
    *lfree = NULL;
    *lused = NULL;
    for (i = 0; i < VCL_MAC_VCE_MAX; i++) {
        mac_p = &vcl_data.mac_data.global_table[i];
        mac_p->conf.id = VCL_MAC_VCE_MAX - i;
        mac_p->next = *lfree;
        *lfree = mac_p;
    }
    VCL_CRIT_EXIT();
}

static void vcl_mac_local_default_set(void)
{
    vcl_mac_vce_local_t *mac_p, **lfree, **lused;
    u32                 i;

    VCL_CRIT_ENTER();
    /* Initialize MAC-based VLAN Local list */
    lfree = &vcl_data.mac_data.local_free;
    lused = &vcl_data.mac_data.local_used;
    *lfree = NULL;
    *lused = NULL;
    for (i = 0; i < VCL_MAC_VCE_MAX; i++) {
        mac_p = &vcl_data.mac_data.local_table[i];
        mac_p->next = *lfree;
        *lfree = mac_p;
    }
    VCL_CRIT_EXIT();
}

static u8 vcl_mac_cmp(mesa_mac_t *mac1, mesa_mac_t *mac2)
{
    u8 i = 0;

    while (i < 6) {
        if (mac1->addr[i] < mac2->addr[i]) {
            return 1;
        } else if (mac1->addr[i] == mac2->addr[i]) {
            if (i == 5) {
                return 0;
            } else {
                i++;
                continue;
            }
        } else {
            return 2;
        }
    }
    return 3;
}

static u8 vcl_sub_cmp(mesa_ipv4_network_t *sub1, mesa_ipv4_network_t *sub2)
{
    u8 ip1 = 0, ip2 = 0;
    i8 i = 3;

    if (sub1->prefix_size > sub2->prefix_size) {
        return 1;
    } else if (sub1->prefix_size < sub2->prefix_size) {
        return 2;
    } else {
        while (i >= 0) {
            ip1 = (sub1->address >> (i * 8)) & 0xff;
            ip2 = (sub2->address >> (i * 8)) & 0xff;
            if (ip1 < ip2) {
                return 1;
            } else if (ip1 == ip2) {
                if (i == 0) {
                    return 0;
                } else {
                    i--;
                    continue;
                }
            } else {
                return 2;
            }
        }
    }
    return 3;
}

static u8 vcl_sub_cmp_snmp(mesa_ipv4_network_t *sub1, mesa_ipv4_network_t *sub2)
{
    u8 ip1 = 0, ip2 = 0;
    i8 i = 3;

    while (i >= 0) {
        ip1 = (sub1->address >> (i * 8)) & 0xff;
        ip2 = (sub2->address >> (i * 8)) & 0xff;
        if (ip1 < ip2) {
            return 1;
        } else if (ip1 == ip2) {
            if (i == 0) {
                if (sub1->prefix_size < sub2->prefix_size) {
                    return 1;
                } else if (sub1->prefix_size > sub2->prefix_size) {
                    return 2;
                } else {
                    return 0;
                }
            } else {
                i--;
                continue;
            }
        } else {
            return 2;
        }
    }
    return 3;
}

static u8 vcl_proto_encap_cmp(vtss_appl_vcl_proto_t *enc1, vtss_appl_vcl_proto_t *enc2)
{
    u8 i = 0;

    if (enc1->proto_encap_type < enc2->proto_encap_type) {
        return 1;
    } else if (enc1->proto_encap_type > enc2->proto_encap_type) {
        return 2;
    } else {
        switch (enc1->proto_encap_type) {
        case VTSS_APPL_VCL_PROTO_ENCAP_ETH2:
            if (enc1->proto.eth2_proto.eth_type < enc2->proto.eth2_proto.eth_type) {
                return 1;
            } else if (enc1->proto.eth2_proto.eth_type == enc2->proto.eth2_proto.eth_type) {
                return 0;
            } else {
                return 2;
            }

        case VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP:
            i = 0;
            while (i < 3) {
                if (enc1->proto.llc_snap_proto.oui[i] < enc2->proto.llc_snap_proto.oui[i]) {
                    return 1;
                } else if (enc1->proto.llc_snap_proto.oui[i] == enc2->proto.llc_snap_proto.oui[i]) {
                    if (i == 2) {
                        if (enc1->proto.llc_snap_proto.pid < enc2->proto.llc_snap_proto.pid) {
                            return 1;
                        } else if (enc1->proto.llc_snap_proto.pid > enc2->proto.llc_snap_proto.pid) {
                            return 2;
                        } else {
                            return 0;
                        }
                    } else {
                        i++;
                        continue;
                    }
                } else {
                    return 2;
                }
            }
            break;

        case VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER:
            if (enc1->proto.llc_other_proto.dsap < enc2->proto.llc_other_proto.dsap) {
                return 1;
            } else if (enc1->proto.llc_other_proto.dsap > enc2->proto.llc_other_proto.dsap) {
                return 2;
            } else {
                if (enc1->proto.llc_other_proto.ssap < enc2->proto.llc_other_proto.ssap) {
                    return 1;
                } else if (enc1->proto.llc_other_proto.ssap > enc2->proto.llc_other_proto.ssap) {
                    return 2;
                } else {
                    return 0;
                }
            }

        default:
            return 3;
        }
    }
    return 3;
}

void vcl_ip_len2mask(u8 mask_len, mesa_ipv4_t *ip_mask)
{
    u8 i;
    *ip_mask = 0;
    for (i = 32; i > (32 - mask_len); i--) {
        *ip_mask |= (1 << (i - 1));
    }
}

void vcl_ip_addr2sub(mesa_ipv4_t *ip_addr, u8 mask_len)
{
    mesa_ipv4_t ip_mask;

    if (mask_len > 32) {
        T_E("Mask length cannot be greater than 32 - got: %u", mask_len);
        return;
    }
    vcl_ip_len2mask(mask_len, &ip_mask);
    *ip_addr = *ip_addr & ip_mask;
}

static void vcl_ip_vce_first_id_get(mesa_vce_id_t *id)
{
    vcl_ip_vce_local_t **lused, *tmp_vce = NULL;

    VCL_CRIT_ENTER();
    lused = &vcl_data.ip_data.local_used;

    if (*lused == NULL) {
        *id = MESA_VCE_ID_LAST;
    } else {
        tmp_vce = *lused;
        *id = tmp_vce->conf.id;
    }
    VCL_CRIT_EXIT();
}

static void vcl_proto_vce_first_id_get(mesa_vce_id_t *id)
{
    vcl_proto_vce_local_t **lused, *tmp_vce = NULL;

    VCL_CRIT_ENTER();
    lused = &vcl_data.proto_data.local_used;
    /* If no entry present return MESA_VCE_ID_LAST */
    if (*lused == NULL) {
        *id = MESA_VCE_ID_LAST;
    } else {
        tmp_vce = *lused;
        *id = tmp_vce->conf.id;
    }
    VCL_CRIT_EXIT();
    return;
}

static mesa_rc vcl_mac_vce_global_add(vcl_mac_vce_conf_global_t *mac_vce)
{
    vcl_mac_vce_global_t **lfree, **lused, *tmp_vce, *new_vce, *prev_vce, *ins = NULL, *ins_prev = NULL;
    BOOL                 update = FALSE;
    u32                  isid, i;
    u8                   res;

    if (mac_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    T_D("Adding VCE to global list: MAC %02x:%02x:%02x:%02x:%02x:%02x, VID %u",
        mac_vce->smac.addr[0], mac_vce->smac.addr[1],
        mac_vce->smac.addr[2], mac_vce->smac.addr[3], mac_vce->smac.addr[4],
        mac_vce->smac.addr[5], mac_vce->vid);

    VCL_CRIT_ENTER();
    lfree = &vcl_data.mac_data.global_free;
    lused = &vcl_data.mac_data.global_used;

    for (tmp_vce = *lused, prev_vce = NULL; tmp_vce != NULL; prev_vce = tmp_vce, tmp_vce = tmp_vce->next) {
        res = vcl_mac_cmp(&mac_vce->smac, &tmp_vce->conf.smac);
        if (res == 0) {
            if (tmp_vce->conf.vid == mac_vce->vid) {
                update = TRUE;
                ins = tmp_vce;
                break;
            } else {
                VCL_CRIT_EXIT();
                return VCL_ERROR_ENTRY_DIFF_VID;
            }
        } else if (res == 1) {
            ins = tmp_vce;
            ins_prev = prev_vce;
            break;
        } else if (res == 2) {
            continue;
        }
    }

    if (update == FALSE) {
        /* Get free node from free list */
        new_vce = *lfree;
        if (new_vce == NULL) {
            VCL_CRIT_EXIT();
            return VCL_ERROR_MAC_TABLE_FULL;
        }
        /* Update the free list */
        *lfree = new_vce->next;
        /* Copy the configuration */
        mac_vce->id = new_vce->conf.id;
        new_vce->conf = *mac_vce;
        /* Update the used list */
        new_vce->next = NULL;
        if (ins == NULL) { /* Add the entry to the end of list */
            if (*lused == NULL) {
                /* Adding first entry to the empty list */
                *lused = new_vce;
            } else {
                /* Adding the entry after last entry in the list */
                prev_vce->next = new_vce;
            }
        } else { /* Add the entry to either head or middle of the list */
            if (ins_prev != NULL) { /* Add the entry to the middle of the list */
                ins_prev->next = new_vce;
                new_vce->next = ins;
            } else { /* Add the entry before first entry */
                new_vce->next = *lused;
                *lused = new_vce;
            }
        }
        T_D("Added new VCE with id: %u", mac_vce->id);
    } else { /* Update the entry */
        if (ins != NULL) { /* This case always happens. But, this check is for satisfying LINT */
            for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
                for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
                    ins->conf.ports[isid - VTSS_ISID_START][i] = mac_vce->ports[isid - VTSS_ISID_START][i];
                }
            }
            mac_vce->id = ins->conf.id;
            T_D("Appended to existing VCE with id: %u", mac_vce->id);
        }
    }
    VCL_CRIT_EXIT();
    return VTSS_RC_OK;
}

static mesa_rc vcl_mac_vce_global_del(vcl_mac_vce_conf_global_t *mac_vce)
{
    vcl_mac_vce_global_t **lfree, **lused, *tmp_vce, *prev_vce, *tmp_vce_free, *prev_vce_free;
    BOOL                 found_entry = FALSE, ports_exist = FALSE;
    mesa_vce_id_t        id;
    u32                  isid, bf;

    if (mac_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    T_D("Deleting VCE from global list: MAC %02x:%02x:%02x:%02x:%02x:%02x",
        mac_vce->smac.addr[0], mac_vce->smac.addr[1],
        mac_vce->smac.addr[2], mac_vce->smac.addr[3], mac_vce->smac.addr[4],
        mac_vce->smac.addr[5]);

    VCL_CRIT_ENTER();
    lfree = &vcl_data.mac_data.global_free;
    lused = &vcl_data.mac_data.global_used;

    for (tmp_vce = *lused, prev_vce = NULL; tmp_vce != NULL; prev_vce = tmp_vce, tmp_vce = tmp_vce->next) {
        if (!memcmp(&mac_vce->smac, &tmp_vce->conf.smac, sizeof(mac_vce->smac))) {
            found_entry = TRUE;
            for (isid = 0; isid < VTSS_ISID_CNT; isid++) {
                for (bf = 0; bf < VTSS_PORT_BF_SIZE; bf++) {
                    tmp_vce->conf.ports[isid][bf] &= (~mac_vce->ports[isid][bf]);
                    if (tmp_vce->conf.ports[isid][bf]) {
                        ports_exist = TRUE;
                    }
                }
            }
            if (ports_exist == FALSE) {
                if (prev_vce == NULL) {
                    *lused = tmp_vce->next;
                } else {
                    prev_vce->next = tmp_vce->next;
                }
                mac_vce->id = tmp_vce->conf.id;
                // Erase previous configuration before returning it to the free list, but keep vce_id!!
                id = tmp_vce->conf.id;
                memset(&tmp_vce->conf, 0, sizeof(tmp_vce->conf));
                tmp_vce->conf.id = id;
                for (tmp_vce_free = *lfree, prev_vce_free = NULL; tmp_vce_free != NULL;
                     prev_vce_free = tmp_vce_free, tmp_vce_free = tmp_vce_free->next) {
                    if (tmp_vce->conf.id < tmp_vce_free->conf.id) {
                        break;
                    }
                }

                if (tmp_vce_free == NULL) { /* Add the entry to the end of list */
                    tmp_vce->next = NULL;
                    if (*lfree == NULL) {
                        /* Adding first entry to the empty list */
                        *lfree = tmp_vce;
                    } else {
                        /* Adding the entry after last entry in the list */
                        prev_vce_free->next = tmp_vce;
                    }
                } else { /* Add the entry to either head or middle of the list */
                    if (prev_vce_free != NULL) { /* Add the entry to the middle of the list */
                        prev_vce_free->next = tmp_vce;
                        tmp_vce->next = tmp_vce_free;
                    } else { /* Add the entry before first entry */
                        tmp_vce->next = *lfree;
                        *lfree = tmp_vce;
                    }
                }
                T_D("Deleted VCE with id: %u", tmp_vce->conf.id);
                break;
            }
            mac_vce->id = tmp_vce->conf.id;
            T_D("Appended to VCE with id: %u", tmp_vce->conf.id);
        }
    }
    VCL_CRIT_EXIT();
    return (found_entry == FALSE) ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK;
}

static mesa_rc vcl_mac_vce_global_get(vcl_mac_vce_conf_global_t *mac_vce, BOOL first, BOOL next)
{
    vcl_mac_vce_global_t **lused, *tmp_vce = NULL;
    BOOL                 use_next = FALSE;

    if (mac_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first) {
        T_D("Switch fetching the first VCE from global list");
    } else {
        T_D("Switch fetching VCE from global list with: MAC %02x:%02x:%02x:%02x:%02x:%02x, first = %d, next = %d",
            mac_vce->smac.addr[0], mac_vce->smac.addr[1],
            mac_vce->smac.addr[2], mac_vce->smac.addr[3], mac_vce->smac.addr[4],
            mac_vce->smac.addr[5], first, next);
    }

    VCL_CRIT_ENTER();
    lused = &vcl_data.mac_data.global_used;

    for (tmp_vce = *lused; tmp_vce != NULL; tmp_vce = tmp_vce->next) {
        if (first == TRUE) {
            break;
        } else {
            if (use_next) {
                break;
            }
            if (!memcmp(&mac_vce->smac, &tmp_vce->conf.smac, sizeof(mac_vce->smac))) {
                if (next) {
                    use_next = TRUE;
                } else {
                    break;
                }
            }
        }
    }

    if (tmp_vce != NULL) {
        T_D("Found VCE with id: %u", tmp_vce->conf.id);
        *mac_vce = tmp_vce->conf;
    }
    VCL_CRIT_EXIT();
    return (tmp_vce == NULL ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK);
}

static mesa_rc vcl_mac_vce_local_add(vcl_mac_vce_conf_local_t *mac_vce)
{
    vcl_mac_vce_local_t **lfree, **lused, *tmp_vce, *new_vce, *prev_vce, *ins = NULL, *ins_prev = NULL;
    BOOL                update = FALSE;
    u32                 i;
    u8                  res;

    /* Check for NULL pointer */
    if (mac_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    T_D("Adding VCE to local list: MAC %02x:%02x:%02x:%02x:%02x:%02x, VID %u, ID %u",
        mac_vce->smac.addr[0], mac_vce->smac.addr[1],
        mac_vce->smac.addr[2], mac_vce->smac.addr[3], mac_vce->smac.addr[4],
        mac_vce->smac.addr[5], mac_vce->vid, mac_vce->id);

    VCL_CRIT_ENTER();
    lfree = &vcl_data.mac_data.local_free;
    lused = &vcl_data.mac_data.local_used;
    for (tmp_vce = *lused, prev_vce = NULL; tmp_vce != NULL; prev_vce = tmp_vce, tmp_vce = tmp_vce->next) {
        res = vcl_mac_cmp(&mac_vce->smac, &tmp_vce->conf.smac);
        if (res == 0) {
            if (tmp_vce->conf.vid == mac_vce->vid) {
                update = TRUE;
                ins = tmp_vce;
                break;
            } else {
                VCL_CRIT_EXIT();
                return VCL_ERROR_ENTRY_DIFF_VID;
            }
        } else if (res == 1) {
            ins = tmp_vce;
            ins_prev = prev_vce;
            break;
        } else if (res == 2) {
            continue;
        }
    }

    if (update == FALSE) {
        /* Get free node from free list */
        new_vce = *lfree;
        if (new_vce == NULL) {
            VCL_CRIT_EXIT();
            return VCL_ERROR_MAC_TABLE_FULL;
        }
        /* Update the free list */
        *lfree = new_vce->next;
        /* Copy the configuration */
        new_vce->conf = *mac_vce;
        /* Update the used list */
        new_vce->next = NULL;
        if (ins == NULL) { /* Add the entry to the end of list */
            if (*lused == NULL) {
                /* Adding first entry to the empty list */
                *lused = new_vce;
            } else {
                /* Adding the entry after last entry in the list */
                prev_vce->next = new_vce;
            }
        } else { /* Add the entry to either head or middle of the list */
            if (ins_prev != NULL) { /* Add the entry to the middle of the list */
                ins_prev->next = new_vce;
                new_vce->next = ins;
            } else { /* Add the entry before first entry */
                new_vce->next = *lused;
                *lused = new_vce;
            }
        }
        T_D("Added new VCE with id: %u", new_vce->conf.id);
    } else { /* Update the entry */
        if (ins != NULL) { /* This case always happens. But, this check is for satisfying LINT */
            for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
                ins->conf.ports[i] = mac_vce->ports[i];
            }
            T_D("Appended to existing VCE with id: %u", ins->conf.id);
        }
    }
    VCL_CRIT_EXIT();
    return VTSS_RC_OK;
}

static mesa_rc vcl_mac_vce_local_del(vcl_mac_vce_conf_local_t *mac_vce)
{
    vcl_mac_vce_local_t **lfree, **lused, *tmp_vce = NULL, *prev_vce;
    BOOL                found_entry = FALSE;
    mesa_vce_id_t       id;

    /* Check for NULL pointer */
    if (mac_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    T_D("Switch deleting VCE from local list: MAC %02x:%02x:%02x:%02x:%02x:%02x, ID %u",
        mac_vce->smac.addr[0], mac_vce->smac.addr[1],
        mac_vce->smac.addr[2], mac_vce->smac.addr[3], mac_vce->smac.addr[4],
        mac_vce->smac.addr[5], mac_vce->id);

    VCL_CRIT_ENTER();
    lfree = &vcl_data.mac_data.local_free;
    lused = &vcl_data.mac_data.local_used;

    for (tmp_vce = *lused, prev_vce = NULL; tmp_vce != NULL; prev_vce = tmp_vce, tmp_vce = tmp_vce->next) {
        if (!memcmp(&mac_vce->smac, &tmp_vce->conf.smac, sizeof(mac_vce->smac))) {
            found_entry = TRUE;
            if (prev_vce == NULL) {
                *lused = tmp_vce->next;
            } else {
                prev_vce->next = tmp_vce->next;
            }
            id = tmp_vce->conf.id;
            memset(&tmp_vce->conf, 0, sizeof(tmp_vce->conf));
            tmp_vce->next = *lfree;
            *lfree = tmp_vce;
            T_D("Deleted VCE with id: %u", id);
            break;
        }
    }
    VCL_CRIT_EXIT();
    return (found_entry == FALSE) ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK;
}

static mesa_rc vcl_mac_vce_local_get(vcl_mac_vce_conf_local_t *mac_vce, BOOL first, BOOL next)
{
    vcl_mac_vce_local_t **lused, *tmp_vce = NULL;
    BOOL                use_next = FALSE;

    /**
     * Check for NULL pointer
     **/
    if (mac_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first) {
        T_D("Switch fetching the first VCE from local list");
    } else {
        T_D("Switch fetching VCE from local list with: MAC %02x:%02x:%02x:%02x:%02x:%02x, first = %d, next = %d",
            mac_vce->smac.addr[0], mac_vce->smac.addr[1],
            mac_vce->smac.addr[2], mac_vce->smac.addr[3], mac_vce->smac.addr[4],
            mac_vce->smac.addr[5], first, next);
    }

    VCL_CRIT_ENTER();
    lused = &vcl_data.mac_data.local_used;

    for (tmp_vce = *lused; tmp_vce != NULL; tmp_vce = tmp_vce->next) {
        if (first == TRUE) {
            break;
        } else {
            if (use_next) {
                break;
            }
            if (!memcmp(&mac_vce->smac, &tmp_vce->conf.smac, sizeof(mac_vce->smac))) {
                if (next) {
                    use_next = TRUE;
                } else {
                    break;
                }
            }
        }
    }
    if (tmp_vce != NULL) {
        T_D("Found VCE with id: %u", tmp_vce->conf.id);
        *mac_vce = tmp_vce->conf;
    }
    VCL_CRIT_EXIT();
    return (tmp_vce == NULL ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK);
}

static mesa_rc vcl_mac_vce_switchapi_add(vcl_mac_vce_conf_local_t *mac_vce)
{
    mesa_vce_t           vce;
    u32                  i;
    mesa_rc              rc = VTSS_RC_OK;
    mesa_vce_id_t        vce_id, vce_id_next = MESA_VCE_ID_LAST;
    port_iter_t          pit;

    /* Check for NULL pointer */
    if (mac_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    vce_id = mac_vce->id;
    /* Check for valid VCE ID */
    if (vce_id >= MESA_VCL_ID_END) {
        return VCL_ERROR_VCE_ID_EXCEEDED;
    }

    if ((rc = mesa_vce_init(NULL, MESA_VCE_TYPE_ANY,  &vce)) != VTSS_RC_OK) {
        return rc;
    }

    T_N("VCE is valid, generating switch API VCE");
    /* Prepare key */
    (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        vce.key.port_list[pit.iport] = VTSS_PORT_BF_GET(mac_vce->ports, pit.iport);
    }
    for (i = 0; i < sizeof(mesa_mac_t); i++) {
        vce.key.mac.smac.value[i] = mac_vce->smac.addr[i];
        vce.key.mac.smac.mask[i] = 0xFF;
    }
    vce.key.mac.dmac_mc = MESA_VCAP_BIT_ANY;
    vce.key.mac.dmac_bc = MESA_VCAP_BIT_ANY;

    /* Prepare action - Only action is to classify to VLAN specified */
    vce.action.vid = mac_vce->vid;
    VCL_CRIT_ENTER();
    vce.action.policy_no = vcl_debug_policy_no;
    VCL_CRIT_EXIT();

    /* Populate VCE ID: First 16 bits (15:0) -> real VCE_ID; next 4-bits (19:16) -> VCL Type (MAC or Protocol)  */
    vce.id = ((vce_id & 0xFFFF) | ((VCL_TYPE_MAC & 0xF) << 16));
    vcl_ip_vce_first_id_get(&vce_id_next);
    if (vce_id_next == MESA_VCE_ID_LAST) {
        vcl_proto_vce_first_id_get(&vce_id_next);
        if (vce_id_next == MESA_VCE_ID_LAST) {
            vce_id_next = MESA_VCE_ID_LAST;
        } else {
            vce_id_next = ((vce_id_next & 0xFFFF) | ((VCL_TYPE_PROTO & 0xF) << 16));
        }
    } else {
        vce_id_next = ((vce_id_next & 0xFFFF) | ((VCL_TYPE_IP & 0xF) << 16));
    }
    T_N("Generated VCE with key: %u", vce.id);
    /* Call the switch API for setting the configuration in ASIC. MAC-based VLAN has more priority than
       Protocol-based VLAN. Hence add any MAC-based VLAN entries before first protocol-based VLAN entry and IP Subnet VLAN*/
    //dump_vce(__FILE__, __LINE__, vce);
    if ((rc = mesa_vce_add(NULL, vce_id_next,  &vce)) != VTSS_RC_OK) {
        return rc;
    } else {
        if (VTSS_RC_OK != vcl_register_vce(VCL_USR_DEFAULT, vce.id, vce_id_next)) {
            T_D("Could not register vce %d before %d", vce.id, vce_id_next);
        }
        T_D("VCE with key: %u was added to the switch API right before VCE with key: %u", vce.id, vce_id_next);
    }
    return VTSS_RC_OK;
}

static mesa_rc vcl_msg_mac_vce_set(vtss_isid_t isid)
{
    vcl_msg_mac_vce_set_t     *msg;
    vcl_mac_vce_conf_global_t entry;
    u32                       cnt = 0;
    BOOL                      found_sid, first = TRUE, next = FALSE;
    mesa_port_no_t            port;
    switch_iter_t             sit;

    T_D("Creating VLC msg SET for switch with isid: %d", isid);

    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        /* Initiate counter to count number of messages to be sent to sit.isid */
        cnt = 0;
        msg = (vcl_msg_mac_vce_set_t *)vcl_msg_alloc(1);
        msg->msg_id = VCL_MSG_ID_MAC_VCE_SET;
        /* Loop through all the entries in the db */
        while (vcl_mac_vce_global_get(&entry, first, next) == VTSS_RC_OK) {
            found_sid = FALSE;
            for (port = 0; port < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port++) {
                VTSS_PORT_BF_SET(msg->conf[cnt].ports, port, VTSS_PORT_BF_GET(entry.ports[sit.isid - VTSS_ISID_START], port));
                if (VTSS_PORT_BF_GET(entry.ports[sit.isid - VTSS_ISID_START], port)) {
                    found_sid = TRUE;
                }
            }
            if (found_sid == FALSE) {
                first = FALSE;
                next = TRUE;
                continue;
            }
            msg->conf[cnt].id = entry.id;
            msg->conf[cnt].smac = entry.smac;
            msg->conf[cnt].vid = entry.vid;
            cnt++;
            first = FALSE;
            next = TRUE;
        }
        msg->count = cnt;
        /* The below function also frees the msg after tx */
        vcl_msg_tx(msg, isid, sizeof(*msg));
        T_D("Created VCL msg SET for switch with isid: %d, number of configurations found: %d", isid, cnt);
    }
    return VTSS_RC_OK;
}

static mesa_rc vcl_msg_mac_vce_add_del(vtss_isid_t isid, vcl_mac_vce_conf_local_t *mac_vce, BOOL add)
{
    vcl_msg_mac_vce_t *msg;
    switch_iter_t     sit;

    T_D("Creating VLC msg for switch with isid: %d", isid);

    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID);
    /* Allocate a message with a ref-count corresponding to the number of times switch_iter_getnext() will return TRUE. */
    if ((msg = (vcl_msg_mac_vce_t *)vcl_msg_alloc(sit.remaining)) != NULL) {
        if (add) {
            msg->msg_id = VCL_MSG_ID_MAC_VCE_ADD;
        } else {
            msg->msg_id = VCL_MSG_ID_MAC_VCE_DEL;
        }
        VCL_CRIT_ENTER();
        memcpy(&msg->conf, mac_vce, sizeof(msg->conf));
        VCL_CRIT_EXIT();
        while (switch_iter_getnext(&sit)) {
            vcl_msg_tx(msg, sit.isid, sizeof(*msg));
        }
    } else {
        return VCL_ERROR_MSG_CREATION_FAIL;
    }
    T_D("Created VCL msg for switch with isid: %d", isid);
    return VTSS_RC_OK;
}

static void vcl_mac_mgmtl2vceg_conf(vtss_isid_t isid, vcl_mac_mgmt_vce_conf_local_t *mac_vce, vcl_mac_vce_conf_global_t *conf)
{
    switch_iter_t sit;
    port_iter_t   pit;

    memset(conf, 0, sizeof(*conf));

    conf->smac = mac_vce->smac;
    conf->vid = mac_vce->vid;

    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            conf->ports[sit.isid - VTSS_ISID_START][pit.iport / 8] |= mac_vce->ports[pit.iport] << (pit.iport % 8);
        }
    }

}

static void vcl_mac_vceg2mgmtg_conf(vtss_isid_t isid, vcl_mac_vce_conf_global_t *mac_vce, vcl_mac_mgmt_vce_conf_global_t *conf)
{
    switch_iter_t sit;
    port_iter_t   pit;

    vtss_clear(*conf);

    conf->smac = mac_vce->smac;
    conf->vid = mac_vce->vid;

    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            conf->ports[sit.isid - VTSS_ISID_START][pit.iport] = VTSS_PORT_BF_GET(mac_vce->ports[sit.isid - VTSS_ISID_START], pit.iport);
        }
    }
}

static void vcl_mac_vcel2mgmtl_conf(vtss_isid_t isid, vcl_mac_vce_conf_local_t *mac_vce, vcl_mac_mgmt_vce_conf_local_t *conf)
{
    port_iter_t   pit;

    vtss_clear(*conf);

    conf->smac = mac_vce->smac;
    conf->vid = mac_vce->vid;

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        conf->ports[pit.iport] = VTSS_PORT_BF_GET(mac_vce->ports, pit.iport);
    }
}

static void vcl_ports_global2local(u8 gports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE], u8 lports[VTSS_PORT_BF_SIZE], vtss_isid_t isid)
{
    int i;

    for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
        lports[i] = gports[isid - VTSS_ISID_START][i];
    }
}

mesa_rc vcl_mac_mgmt_conf_add(vtss_isid_t isid_add, vcl_mac_mgmt_vce_conf_local_t *mac_vce)
{
    vcl_mac_vce_conf_global_t entry;
    vcl_mac_vce_conf_local_t  conf;
    mesa_rc                   rc = VTSS_RC_OK;
    u8                        system_mac[6];
    u32                       i;
    switch_iter_t             sit;

    if (!msg_switch_is_primary()) {
        T_W("Switch is not the primary - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }
    /**
    * Check for ISID validity
    **/
    if (!(VTSS_ISID_LEGAL(isid_add) || (isid_add == VTSS_ISID_GLOBAL))) {
        T_E("Invalid ISID: %u - LEGAL one expected", isid_add);
        return VCL_ERROR_INVALID_ISID;
    }
    /**
    * Check the pointer for NULL
    **/
    if (mac_vce == NULL) {
        T_E("Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    /* System address and broadcast/multicast address cannot be used */
    if (conf_mgmt_mac_addr_get(system_mac, 0) == 0) {
        for (i = 0; i < 6; i++) {
            if (system_mac[i] != mac_vce->smac.addr[i]) {
                break;
            }
        }
        /* i will be 6 only when system MAC matches entered MAC address */
        if (i == 6) {
            T_D("System MAC address cannot be used for MAC-based VLAN");
            return VCL_ERROR_SYSTEM_MAC;
        }
    }

    /* Multicast or Broadcast MAC is identified by Least Significant Bit set in MSB of MAC */
    if ((mac_vce->smac.addr[0] & 0x1) == 1) {
        T_D("Multicast or Broadcast MAC address cannot be used for MAC-based VLAN");
        return VCL_ERROR_MULTIBROAD_MAC;
    }

    if (mac_vce->vid < VTSS_APPL_VLAN_ID_MIN || mac_vce->vid > VTSS_APPL_VLAN_ID_MAX) {
        T_W("VLAN ID must be between 1 and 4095");
        return VCL_ERROR_INVALID_VLAN_ID;
    }

    T_D("MGMT API adding VCE with: MAC %02x:%02x:%02x:%02x:%02x:%02x, VID %u, on switch %u",
        mac_vce->smac.addr[0], mac_vce->smac.addr[1],
        mac_vce->smac.addr[2], mac_vce->smac.addr[3], mac_vce->smac.addr[4],
        mac_vce->smac.addr[5], mac_vce->vid, isid_add);

    /**
    * Convert the vcl_mac_vlan_mgmt_entry_t to a vcl_mac_vlan_conf_entry_t.
    **/
    vcl_mac_mgmtl2vceg_conf(isid_add, mac_vce, &entry);

    if ((rc = vcl_mac_vce_global_add(&entry)) != VTSS_RC_OK) {
        return rc;
    } else {
        conf.id = entry.id;
        conf.smac = entry.smac;
        conf.vid = entry.vid;
        (void)switch_iter_init(&sit, isid_add, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
            vcl_ports_global2local(entry.ports, conf.ports, sit.isid);
            if ((rc = vcl_msg_mac_vce_add_del(sit.isid, &conf, TRUE)) != VTSS_RC_OK) {
                return rc;
            }
        }
    }
    T_D("MGMT API added the above VCE");
    return VTSS_RC_OK;
}

mesa_rc vcl_mac_mgmt_conf_del(vtss_isid_t isid_del, mesa_mac_t *smac)
{
    vcl_mac_vce_conf_global_t entry;
    vcl_mac_vce_conf_local_t  conf;
    mesa_rc                   rc = VTSS_RC_OK;
    u32                       port_num;

    if (!msg_switch_is_primary()) {
        T_W("Switch is not the primary switch - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (!(VTSS_ISID_LEGAL(isid_del))) {
        T_E("Invalid ISID: %u - LEGAL one expected", isid_del);
        return VCL_ERROR_INVALID_ISID;
    }

    if (smac == NULL) {
        T_E("Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    T_D("MGMT API deleting/editing VCE with: MAC %02x:%02x:%02x:%02x:%02x:%02x, on switch %u",
        smac->addr[0], smac->addr[1], smac->addr[2], smac->addr[3],
        smac->addr[4], smac->addr[5], isid_del);


    entry.smac = *smac;
    memset(entry.ports, 0, sizeof(entry.ports));
    for (port_num = 0; port_num < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_num++) {
        VTSS_PORT_BF_SET(entry.ports[isid_del - VTSS_ISID_START], port_num, 1);
    }
    if ((rc = vcl_mac_vce_global_del(&entry)) != VTSS_RC_OK) {
        return rc;
    } else {
        conf.id = entry.id;
        conf.smac = entry.smac;
        if ((rc = vcl_msg_mac_vce_add_del(isid_del, &conf, FALSE)) != VTSS_RC_OK) {
            return rc;
        }
    }
    T_D("MGMT API deleted/edited the above VCE");
    return VTSS_RC_OK;
}

mesa_rc vcl_mac_mgmt_conf_get(vtss_isid_t isid_get, vcl_mac_mgmt_vce_conf_global_t *mac_vce, BOOL first, BOOL next)
{
    vcl_mac_vce_conf_global_t entry;
    mesa_rc                   rc = VTSS_RC_OK;
    BOOL                      found_sid = FALSE, first_l, next_l;
    port_iter_t               pit;

    if (!msg_switch_is_primary()) {
        T_W("Switch is not the primary switch - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (!VTSS_ISID_LEGAL(isid_get) && (isid_get != VTSS_ISID_GLOBAL)) {
        T_E("Invalid ISID: %u - LEGAL one expected", isid_get);
        return VCL_ERROR_INVALID_ISID;
    }

    if (mac_vce == NULL) {
        T_E("Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first) {
        T_D("MGMT API fetching the first VCE from global list");
    } else {
        T_D("MGMT API fetching VCE from global list with: MAC %02x:%02x:%02x:%02x:%02x:%02x, first = %d, next = %d",
            mac_vce->smac.addr[0], mac_vce->smac.addr[1],
            mac_vce->smac.addr[2], mac_vce->smac.addr[3], mac_vce->smac.addr[4],
            mac_vce->smac.addr[5], first, next);
    }

    memset(&mac_vce->vid, 0, sizeof(mac_vce->vid));
    vtss_clear(mac_vce->ports);
    memset(&entry, 0, sizeof(entry));

    entry.smac = mac_vce->smac;
    first_l = first;
    next_l = next;
    while ((rc = vcl_mac_vce_global_get(&entry, first_l, next_l)) == VTSS_RC_OK) {
        T_D("mac is %02x:%02x:%02x:%02x:%02x:%02x, vid = %d", entry.smac.addr[0], entry.smac.addr[1], entry.smac.addr[2],
            entry.smac.addr[3], entry.smac.addr[4], entry.smac.addr[5], entry.vid);
        vcl_mac_vceg2mgmtg_conf(isid_get, &entry, mac_vce);
        if (isid_get == VTSS_ISID_GLOBAL) {
            found_sid = TRUE;
        } else {
            (void)port_iter_init(&pit, NULL, isid_get, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (mac_vce->ports[isid_get - VTSS_ISID_START][pit.iport] == 1) {
                    found_sid = TRUE;
                    break;
                }
            }
        }
        if (found_sid == FALSE) {
            next_l = TRUE;
            first_l = FALSE;
            T_N("MGMT API found the above VCE in the global list, but rejected it since it is not present in the requested switch #%u",
                isid_get);
            continue;
        }
        T_D("MGMT API found the above VCE");
        break;
    }

    T_D("MGMT API Exit");
    return rc;
}

mesa_rc vcl_mac_mgmt_conf_local_get(vcl_mac_mgmt_vce_conf_local_t *mac_vce, BOOL first, BOOL next)
{
    mesa_rc                  rc;
    vcl_mac_vce_conf_local_t entry;

    if (mac_vce == NULL) {
        T_E("Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }
    if (first) {
        T_D("MGMT API fetching the first VCE from local list");
    } else {
        T_D("MGMT API fetching VCE from local list with: MAC %02x:%02x:%02x:%02x:%02x:%02x, first = %d, next = %d",
            mac_vce->smac.addr[0], mac_vce->smac.addr[1],
            mac_vce->smac.addr[2], mac_vce->smac.addr[3], mac_vce->smac.addr[4],
            mac_vce->smac.addr[5], first, next);
    }

    memset(&entry, 0, sizeof(entry));
    entry.smac = mac_vce->smac;
    if ((rc = vcl_mac_vce_local_get(&entry, first, next)) != VTSS_RC_OK) {
        return rc;
    } else {
        vcl_mac_vcel2mgmtl_conf(VTSS_ISID_LOCAL, &entry, mac_vce);
    }
    T_D("MGMT API found the above VCE");
    return VTSS_RC_OK;
}

mesa_rc vcl_mac_mgmt_conf_itr(mesa_mac_t *mac, BOOL first)
{
    vcl_mac_vce_global_t **lused, *tmp_vce = NULL;
    u8                   res;

    if (!msg_switch_is_primary()) {
        T_WG(TRACE_GRP_MIB, "Switch is not the primary switch - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (first) {
        T_DG(TRACE_GRP_MIB, "Iterator fetching the first MAC VCE from the list");
    } else {
        T_DG(TRACE_GRP_MIB, "Iterator fetching the MAC VCE after the one with: MAC %02x:%02x:%02x:%02x:%02x:%02x",
             mac->addr[0], mac->addr[1], mac->addr[2],
             mac->addr[3], mac->addr[4], mac->addr[5]);
    }

    VCL_CRIT_ENTER();
    lused = &vcl_data.mac_data.global_used;

    for (tmp_vce = *lused; tmp_vce != NULL; tmp_vce = tmp_vce->next) {
        if (first == TRUE) {
            break;
        } else {
            res = vcl_mac_cmp(mac, &tmp_vce->conf.smac);
            if (res == 0) {
                tmp_vce = tmp_vce->next;
                break;
            } else if (res == 1) {
                break;
            } else if (res == 2) {
                continue;
            }
        }
    }

    if (tmp_vce != NULL) {
        T_DG(TRACE_GRP_MIB, "Found VCE with id: %u", tmp_vce->conf.id);
        *mac = tmp_vce->conf.smac;
    } else {
        T_DG(TRACE_GRP_MIB, "There is no MAC VCE after the provided one");
    }
    VCL_CRIT_EXIT();
    return (tmp_vce == NULL ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK);
}

static void vcl_mac_default_set(void)
{
    BOOL                     first = TRUE, next = FALSE;
    vcl_mac_vce_conf_local_t entry;
    mesa_vce_id_t            vce_id;
    mesa_rc                  rc;

    /* Delete all the existing entries */
    while ((vcl_mac_vce_local_get(&entry, first, next)) == VTSS_RC_OK) {
        if (vcl_mac_vce_local_del(&entry) == VTSS_RC_OK) {
            vce_id = entry.id;
            vce_id = ((vce_id & 0xFFFF) | ((VCL_TYPE_MAC & 0xF) << 16));
            if (VTSS_RC_OK != vcl_unregister_vce(VCL_USR_DEFAULT, vce_id)) {
                T_D("Failure while unregistering vce %d", vce_id);
            }
            /* Call the switch API */
            if ((rc = mesa_vce_del(NULL, vce_id)) != VTSS_RC_OK) {
                T_D("Failure while deleting old MAC entries (rc = %s)", error_txt(rc));
            }
        }
    }

    memset(&vcl_data.mac_data, 0, sizeof(vcl_mac_data_t));
    vcl_mac_global_default_set();
    vcl_mac_local_default_set();
}

static void vcl_ip_global_default_set(void)
{
    vcl_ip_vce_global_t *ip_p, **lfree, **lused;
    u32                 i;

    VCL_CRIT_ENTER();
    /* Initialize MAC-based VLAN Global list */
    lfree = &vcl_data.ip_data.global_free;
    lused = &vcl_data.ip_data.global_used;
    *lfree = NULL;
    *lused = NULL;
    for (i = 0; i < VCL_IP_VCE_MAX; i++) {
        ip_p = &vcl_data.ip_data.global_table[i];
        ip_p->conf.id = VCL_IP_VCE_MAX - i;
        ip_p->next = *lfree;
        *lfree = ip_p;
    }
    VCL_CRIT_EXIT();
}

static void vcl_ip_local_default_set(void)
{
    vcl_ip_vce_local_t *ip_p, **lfree, **lused;
    u32                i;

    VCL_CRIT_ENTER();
    /* Initialize MAC-based VLAN Local list */
    lfree = &vcl_data.ip_data.local_free;
    lused = &vcl_data.ip_data.local_used;
    *lfree = NULL;
    *lused = NULL;
    for (i = 0; i < VCL_IP_VCE_MAX; i++) {
        ip_p = &vcl_data.ip_data.local_table[i];
        ip_p->next = *lfree;
        *lfree = ip_p;
    }
    VCL_CRIT_EXIT();
}

static mesa_rc vcl_ip_vce_global_add(vcl_ip_vce_conf_global_t *ip_vce)
{
    vcl_ip_vce_global_t **lfree, **lused, *tmp_vce, *new_vce, *prev_vce, *ins = NULL, *ins_prev = NULL;
    mesa_ipv4_network_t sub1, sub2;
    BOOL                update = FALSE;
    u32                 isid, i;
    char                ip_str[100];
    u8                  res;

    if (ip_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    T_D("Adding VCE to global list: Subnet %s/%u, VID %u", misc_ipv4_txt(ip_vce->ip_addr, ip_str), ip_vce->mask_len, ip_vce->vid);

    VCL_CRIT_ENTER();
    lfree = &vcl_data.ip_data.global_free;
    lused = &vcl_data.ip_data.global_used;

    sub1.address = ip_vce->ip_addr;
    sub1.prefix_size = ip_vce->mask_len;
    for (tmp_vce = *lused, prev_vce = NULL; tmp_vce != NULL; prev_vce = tmp_vce, tmp_vce = tmp_vce->next) {
        sub2.address = tmp_vce->conf.ip_addr;
        sub2.prefix_size = tmp_vce->conf.mask_len;
        res = vcl_sub_cmp(&sub1, &sub2);
        if (res == 0) {
            if (tmp_vce->conf.vid == ip_vce->vid) {
                update = TRUE;
                ins = tmp_vce;
                break;
            } else {
                VCL_CRIT_EXIT();
                return VCL_ERROR_ENTRY_DIFF_VID;
            }
        } else if (res == 1) {
            ins = tmp_vce;
            ins_prev = prev_vce;
            break;
        } else if (res == 2) {
            continue;
        }
    }

    if (update == FALSE) {
        /* Get free node from free list */
        new_vce = *lfree;
        if (new_vce == NULL) {
            VCL_CRIT_EXIT();
            return VCL_ERROR_IP_TABLE_FULL;
        }
        /* Update the free list */
        *lfree = new_vce->next;
        /* Copy the configuration */
        ip_vce->id = new_vce->conf.id;
        new_vce->conf = *ip_vce;
        /* Update the used list */
        new_vce->next = NULL;
        if (ins == NULL) { /* Add the entry to the end of list */
            if (*lused == NULL) {
                /* Adding first entry to the empty list */
                *lused = new_vce;
            } else {
                /* Adding the entry after last entry in the list */
                prev_vce->next = new_vce;
            }
        } else { /* Add the entry to either head or middle of the list */
            if (ins_prev != NULL) { /* Add the entry to the middle of the list */
                ins_prev->next = new_vce;
                new_vce->next = ins;
            } else { /* Add the entry before first entry */
                new_vce->next = *lused;
                *lused = new_vce;
            }
        }
        T_D("Added new VCE with id: %u", ip_vce->id);
    } else { /* Update the entry */
        if (ins != NULL) { /* This case always happens. But, this check is for satisfying LINT */
            for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
                for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
                    ins->conf.ports[isid - VTSS_ISID_START][i] = ip_vce->ports[isid - VTSS_ISID_START][i];
                }
            }
            ip_vce->id = ins->conf.id;
            T_D("Appended to existing VCE with id: %u", ip_vce->id);
        }
    }
    VCL_CRIT_EXIT();
    return VTSS_RC_OK;
}

static mesa_rc vcl_ip_vce_global_del(vcl_ip_vce_conf_global_t *ip_vce)
{
    vcl_ip_vce_global_t **lfree, **lused, *tmp_vce, *prev_vce, *tmp_vce_free, *prev_vce_free;
    BOOL                found_entry = FALSE, ports_exist = FALSE;
    mesa_vce_id_t       id;
    u32                 isid, i;
    char                ip_str[100];

    if (ip_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    T_D("Deleting VCE from global list: Subnet %s/%u", misc_ipv4_txt(ip_vce->ip_addr, ip_str), ip_vce->mask_len);

    VCL_CRIT_ENTER();
    lfree = &vcl_data.ip_data.global_free;
    lused = &vcl_data.ip_data.global_used;

    for (tmp_vce = *lused, prev_vce = NULL; tmp_vce != NULL; prev_vce = tmp_vce, tmp_vce = tmp_vce->next) {
        if ((tmp_vce->conf.mask_len == ip_vce->mask_len) && (tmp_vce->conf.ip_addr == ip_vce->ip_addr)) {
            found_entry = TRUE;
            for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
                for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
                    tmp_vce->conf.ports[isid - VTSS_ISID_START][i] &= (~ip_vce->ports[isid - VTSS_ISID_START][i]);
                    if (tmp_vce->conf.ports[isid - VTSS_ISID_START][i]) {
                        ports_exist = TRUE;
                    }
                }
            }
            if (ports_exist == FALSE) {
                if (prev_vce == NULL) {
                    *lused = tmp_vce->next;
                } else {
                    prev_vce->next = tmp_vce->next;
                }
                ip_vce->id = tmp_vce->conf.id;
                // Erase previous configuration before returning it to the free list, but keep vce_id!!
                id = tmp_vce->conf.id;
                memset(&tmp_vce->conf, 0, sizeof(tmp_vce->conf));
                tmp_vce->conf.id = id;
                for (tmp_vce_free = *lfree, prev_vce_free = NULL; tmp_vce_free != NULL;
                     prev_vce_free = tmp_vce_free, tmp_vce_free = tmp_vce_free->next) {
                    if (tmp_vce->conf.id < tmp_vce_free->conf.id) {
                        break;
                    }
                }

                if (tmp_vce_free == NULL) { /* Add the entry to the end of list */
                    tmp_vce->next = NULL;
                    if (*lfree == NULL) {
                        /* Adding first entry to the empty list */
                        *lfree = tmp_vce;
                    } else {
                        /* Adding the entry after last entry in the list */
                        prev_vce_free->next = tmp_vce;
                    }
                } else { /* Add the entry to either head or middle of the list */
                    if (prev_vce_free != NULL) { /* Add the entry to the middle of the list */
                        prev_vce_free->next = tmp_vce;
                        tmp_vce->next = tmp_vce_free;
                    } else { /* Add the entry before first entry */
                        tmp_vce->next = *lfree;
                        *lfree = tmp_vce;
                    }
                }
                T_D("Deleted VCE with id: %u", tmp_vce->conf.id);
                break;
            }
            ip_vce->id = tmp_vce->conf.id;
            T_D("Appended to VCE with id: %u", tmp_vce->conf.id);
            break;
        }
    }
    VCL_CRIT_EXIT();
    return (found_entry == FALSE) ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK;
}

static mesa_rc vcl_ip_vce_global_get(vcl_ip_vce_conf_global_t *ip_vce, BOOL first, BOOL next)
{
    vcl_ip_vce_global_t **lused, *tmp_vce = NULL;
    BOOL                use_next = FALSE;
    char                ip_str[100];

    if (ip_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first) {
        T_D("Switch fetching the first VCE from global list");
    } else {
        T_D("Switch fetching VCE from global list with: Subnet %s/%u, first = %d, next = %d",
            misc_ipv4_txt(ip_vce->ip_addr, ip_str), ip_vce->mask_len, first, next);
    }

    VCL_CRIT_ENTER();
    lused = &vcl_data.ip_data.global_used;

    for (tmp_vce = *lused; tmp_vce != NULL; tmp_vce = tmp_vce->next) {
        if (first == TRUE) {
            break;
        } else {
            if (use_next) {
                break;
            }
            if ((tmp_vce->conf.mask_len == ip_vce->mask_len) && (tmp_vce->conf.ip_addr == ip_vce->ip_addr)) {
                if (next) {
                    use_next = TRUE;
                } else {
                    break;
                }
            }
        }
    }

    if (tmp_vce != NULL) {
        T_D("Found VCE with id: %u", tmp_vce->conf.id);
        *ip_vce = tmp_vce->conf;
    }
    VCL_CRIT_EXIT();
    return (tmp_vce == NULL ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK);
}

static mesa_rc vcl_ip_vce_local_add(vcl_ip_vce_conf_local_t *ip_vce, mesa_vce_id_t *id_next)
{
    vcl_ip_vce_local_t  **lfree, **lused, *tmp_vce, *new_vce, *prev_vce, *ins = NULL, *ins_prev = NULL;
    mesa_ipv4_network_t sub1, sub2;
    BOOL                update = FALSE;
    u32                 i;
    char                ip_str[100];
    u8                  res;

    if (ip_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    T_D("Adding VCE to local list: Subnet %s/%u, VID %u", misc_ipv4_txt(ip_vce->ip_addr, ip_str), ip_vce->mask_len, ip_vce->vid);

    VCL_CRIT_ENTER();
    lfree = &vcl_data.ip_data.local_free;
    lused = &vcl_data.ip_data.local_used;
    *id_next = MESA_VCE_ID_LAST;


    sub1.address = ip_vce->ip_addr;
    sub1.prefix_size = ip_vce->mask_len;
    for (tmp_vce = *lused, prev_vce = NULL; tmp_vce != NULL; prev_vce = tmp_vce, tmp_vce = tmp_vce->next) {
        sub2.address = tmp_vce->conf.ip_addr;
        sub2.prefix_size = tmp_vce->conf.mask_len;
        res = vcl_sub_cmp(&sub1, &sub2);
        if (res == 0) {
            if (tmp_vce->conf.vid == ip_vce->vid) {
                update = TRUE;
                ins = tmp_vce;
                break;
            } else {
                VCL_CRIT_EXIT();
                return VCL_ERROR_ENTRY_DIFF_VID;
            }
        } else if (res == 1) {
            ins = tmp_vce;
            ins_prev = prev_vce;
            break;
        } else if (res == 2) {
            continue;
        }
    }

    if (update == FALSE) {
        /* Get free node from free list */
        new_vce = *lfree;
        if (new_vce == NULL) {
            VCL_CRIT_EXIT();
            return VCL_ERROR_IP_TABLE_FULL;
        }
        /* Update the free list */
        *lfree = new_vce->next;
        /* Copy the configuration */
        new_vce->conf = *ip_vce;
        /* Update the used list */
        new_vce->next = NULL;
        if (ins == NULL) { /* Add the entry to the end of list */
            if (*lused == NULL) {
                /* Adding first entry to the empty list */
                *lused = new_vce;
            } else {
                /* Adding the entry after last entry in the list */
                prev_vce->next = new_vce;
            }
        } else { /* Add the entry to either head or middle of the list */
            if (ins_prev != NULL) { /* Add the entry to the middle of the list */
                ins_prev->next = new_vce;
                new_vce->next = ins;
                *id_next = ins->conf.id;
            } else { /* Add the entry before first entry */
                new_vce->next = *lused;
                *lused = new_vce;
                *id_next = ins->conf.id;
            }
        }
        T_D("Added new VCE with id: %u", ip_vce->id);
    } else { /* Update the entry */
        if (ins != NULL) { /* This case always happens. But, this check is for satisfying LINT */
            for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
                ins->conf.ports[i] = ip_vce->ports[i];
            }
            T_D("Appended to existing VCE with id: %u", ip_vce->id);
        }
    }
    VCL_CRIT_EXIT();
    return VTSS_RC_OK;
}

static mesa_rc vcl_ip_vce_local_del(vcl_ip_vce_conf_local_t *ip_vce)
{
    vcl_ip_vce_local_t **lfree, **lused, *tmp_vce, *prev_vce;
    BOOL                found_entry = FALSE;
    mesa_vce_id_t       id;
    char                ip_str[100];

    if (ip_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    T_D("Switch deleting VCE from local list: Subnet %s/%u", misc_ipv4_txt(ip_vce->ip_addr, ip_str), ip_vce->mask_len);

    VCL_CRIT_ENTER();
    lfree = &vcl_data.ip_data.local_free;
    lused = &vcl_data.ip_data.local_used;

    for (tmp_vce = *lused, prev_vce = NULL; tmp_vce != NULL; prev_vce = tmp_vce, tmp_vce = tmp_vce->next) {
        if ((tmp_vce->conf.mask_len == ip_vce->mask_len) && (tmp_vce->conf.ip_addr == ip_vce->ip_addr)) {
            found_entry = TRUE;
            if (prev_vce == NULL) {
                *lused = tmp_vce->next;
            } else {
                prev_vce->next = tmp_vce->next;
            }
            id = tmp_vce->conf.id;
            memset(&tmp_vce->conf, 0, sizeof(tmp_vce->conf));
            tmp_vce->next = *lfree;
            *lfree = tmp_vce;
            T_D("Deleted VCE with id: %u", id);
            break;
        }
    }
    VCL_CRIT_EXIT();
    return (found_entry == FALSE) ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK;
}

static mesa_rc vcl_ip_vce_local_get(vcl_ip_vce_conf_local_t *ip_vce, BOOL first, BOOL next)
{
    vcl_ip_vce_local_t **lused, *tmp_vce = NULL;
    BOOL               use_next = FALSE;
    char               ip_str[100];

    if (ip_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first) {
        T_D("Switch fetching the first VCE from local list");
    } else {
        T_D("Switch fetching VCE from local list with: Subnet %s/%u, first = %d, next = %d",
            misc_ipv4_txt(ip_vce->ip_addr, ip_str), ip_vce->mask_len, first, next);
    }

    VCL_CRIT_ENTER();
    lused = &vcl_data.ip_data.local_used;

    for (tmp_vce = *lused; tmp_vce != NULL; tmp_vce = tmp_vce->next) {
        if (first == TRUE) {
            break;
        } else {
            if (use_next) {
                break;
            }
            if ((tmp_vce->conf.mask_len == ip_vce->mask_len) && (tmp_vce->conf.ip_addr == ip_vce->ip_addr)) {
                if (next) {
                    use_next = TRUE;
                } else {
                    break;
                }
            }
        }
    }
    if (tmp_vce != NULL) {
        T_D("Found VCE with id: %u", tmp_vce->conf.id);
        *ip_vce = tmp_vce->conf;
    }
    VCL_CRIT_EXIT();
    return ((tmp_vce != NULL) ? (mesa_rc)VTSS_RC_OK : VCL_ERROR_ENTRY_NOT_FOUND);
}

static mesa_rc vcl_ip_vce_switchapi_add(vcl_ip_vce_conf_local_t *ip_vce, mesa_vce_id_t id_next)
{
    mesa_vce_t           vce;
    mesa_rc              rc = VTSS_RC_OK;
    mesa_vce_id_t        vce_id, vce_id_next = MESA_VCE_ID_LAST;
    port_iter_t          pit;

    /* Check for NULL pointer */
    if (ip_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    vce_id = ip_vce->id;
    /* Check for valid VCE ID */
    if (vce_id >= MESA_VCL_ID_END) {
        return VCL_ERROR_VCE_ID_EXCEEDED;
    }

    if ((rc = mesa_vce_init(NULL, MESA_VCE_TYPE_IPV4,  &vce)) != VTSS_RC_OK) {
        return rc;
    }

    T_N("VCE is valid, generating switch API VCE");
    /* Prepare key */
    (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        vce.key.port_list[pit.iport] = VTSS_PORT_BF_GET(ip_vce->ports, pit.iport);
    }
    vce.key.frame.ipv4.sip.value = ip_vce->ip_addr;
    vcl_ip_len2mask(ip_vce->mask_len, &vce.key.frame.ipv4.sip.mask);

    /* Allow only for untagged Frames */
    vce.key.tag.tagged = MESA_VCAP_BIT_0;

    /* Prepare action - Only action is to classify to VLAN specified */
    vce.action.vid = ip_vce->vid;
    VCL_CRIT_ENTER();
    vce.action.policy_no = vcl_debug_policy_no;
    VCL_CRIT_EXIT();

    /* Populate VCE ID: First 16 bits (15:0) -> real VCE_ID; next 4-bits (19:16) -> VCL Type (MAC or Protocol)  */
    vce.id = ((vce_id & 0xFFFF) | ((VCL_TYPE_IP & 0xF) << 16));
    if (id_next == MESA_VCE_ID_LAST) {
        vcl_proto_vce_first_id_get(&vce_id_next);
        if (vce_id_next == MESA_VCE_ID_LAST) {
            vce_id_next = MESA_VCE_ID_LAST;
        } else {
            vce_id_next = ((vce_id_next & 0xFFFF) | ((VCL_TYPE_PROTO & 0xF) << 16));
        }
    } else {
        vce_id_next = ((id_next & 0xFFFF) | ((VCL_TYPE_IP & 0xF) << 16));
    }
    T_N("Generated VCE with key: %u", vce.id);
    /* Call the switch API for setting the configuration in ASIC. MAC-based VLAN has more priority than
       Protocol-based VLAN. Hence add any MAC-based VLAN entries before first protocol-based VLAN entry and IP Subnet VLAN*/
    //dump_vce(__FILE__, __LINE__, vce);
    if ((rc = mesa_vce_add(NULL, vce_id_next,  &vce)) != VTSS_RC_OK) {
        return rc;
    } else {
        if (VTSS_RC_OK != vcl_register_vce(VCL_USR_DEFAULT, vce.id, vce_id_next)) {
            T_D("Could not register vce %d before %d", vce.id, vce_id_next);
        }
        T_D("VCE with key: %u was added to the switch API right before VCE with key: %u", vce.id, vce_id_next);
    }

    /* Add VCE rule for priority-tagged frames - All fields remain same except tagged and vid */
    vce.key.tag.tagged = MESA_VCAP_BIT_1;
    vce.key.tag.vid.value = 0x0;
    vce.key.tag.vid.mask = 0xFFFF;
    /* Priority-tagged VCE entry is identified by setting 20th bit of vce_id */
    vce.id = ((vce_id & 0xFFFF) | ((VCL_TYPE_IP & 0xF) << 16) | (0x1 << 20));
    //dump_vce(__FILE__, __LINE__, vce);
    if ((rc = mesa_vce_add(NULL, vce_id_next,  &vce)) != VTSS_RC_OK) {
        return rc;
    } else {
        if (VTSS_RC_OK != vcl_register_vce(VCL_USR_DEFAULT, vce.id, vce_id_next)) {
            T_D("Could not register vce %d before %d", vce.id, vce_id_next);
        }
        T_D("VCE with key: %u was added to the switch API right before VCE with key: %u (priority-tagged)", vce.id, vce_id_next);
    }
    return VTSS_RC_OK;
}

static mesa_rc vcl_msg_ip_vce_set(vtss_isid_t isid)
{
    vcl_msg_ip_vce_set_t     *msg;
    vcl_ip_vce_conf_global_t entry;
    u32                      cnt;
    BOOL                     found_sid, first = TRUE, next = FALSE;
    mesa_port_no_t           port;
    switch_iter_t            sit;

    T_D("Creating VLC msg for switch with isid: %d", isid);

    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        /* Initiate counter to count number of messages to be sent to sit.isid */
        cnt = 0;
        msg = (vcl_msg_ip_vce_set_t *)vcl_msg_alloc(1);
        msg->msg_id = VCL_MSG_ID_IP_VCE_SET;
        /* Loop through all the entries in the db */
        while (vcl_ip_vce_global_get(&entry, first, next) == VTSS_RC_OK) {
            found_sid = FALSE;
            for (port = 0; port < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port++) {
                VTSS_PORT_BF_SET(msg->conf[cnt].ports, port, VTSS_PORT_BF_GET(entry.ports[sit.isid - VTSS_ISID_START], port));
                if (VTSS_PORT_BF_GET(entry.ports[sit.isid - VTSS_ISID_START], port)) {
                    found_sid = TRUE;
                }
            }
            if (found_sid == FALSE) {
                first = FALSE;
                next = TRUE;
                continue;
            }
            msg->conf[cnt].id = entry.id;
            msg->conf[cnt].ip_addr = entry.ip_addr;
            msg->conf[cnt].mask_len = entry.mask_len;
            msg->conf[cnt].vid = entry.vid;
            cnt++;
            first = FALSE;
            next = TRUE;
        }
        msg->count = cnt;
        /* The below function also frees the msg after tx */
        vcl_msg_tx(msg, isid, sizeof(*msg));
    }
    T_D("Created VCL msg for switch with isid: %d", isid);
    return VTSS_RC_OK;
}

static mesa_rc vcl_msg_ip_vce_add_del(vtss_isid_t isid, vcl_ip_vce_conf_local_t *ip_vce, BOOL add)
{
    vcl_msg_ip_vce_t *msg;
    switch_iter_t    sit;

    T_D("Creating VLC msg for switch with isid: %d", isid);

    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID);
    /* Allocate a message with a ref-count corresponding to the number of times switch_iter_getnext() will return TRUE. */
    if ((msg = (vcl_msg_ip_vce_t *)vcl_msg_alloc(sit.remaining)) != NULL) {
        if (add) {
            msg->msg_id = VCL_MSG_ID_IP_VCE_ADD;
        } else {
            msg->msg_id = VCL_MSG_ID_IP_VCE_DEL;
        }
        VCL_CRIT_ENTER();
        memcpy(&msg->conf, ip_vce, sizeof(msg->conf));
        VCL_CRIT_EXIT();
        while (switch_iter_getnext(&sit)) {
            vcl_msg_tx(msg, sit.isid, sizeof(*msg));
        }
    } else {
        return VCL_ERROR_MSG_CREATION_FAIL;
    }
    T_D("Created VCL msg for switch with isid: %d", isid);
    return VTSS_RC_OK;
}

static void vcl_ip_mgmtl2vceg_conf(vtss_isid_t isid, vcl_ip_mgmt_vce_conf_local_t *ip_vce, vcl_ip_vce_conf_global_t *conf)
{
    switch_iter_t sit;
    port_iter_t   pit;

    memset(conf, 0, sizeof(*conf));

    conf->ip_addr = ip_vce->ip_addr;
    conf->mask_len = ip_vce->mask_len;
    conf->vid = ip_vce->vid;

    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            conf->ports[sit.isid - VTSS_ISID_START][pit.iport / 8] |= ip_vce->ports[pit.iport] << (pit.iport % 8);
        }
    }

}

static void vcl_ip_vceg2mgmtg_conf(vtss_isid_t isid, vcl_ip_vce_conf_global_t *ip_vce, vcl_ip_mgmt_vce_conf_global_t *conf)
{
    switch_iter_t sit;
    port_iter_t   pit;

    vtss_clear(*conf);

    conf->ip_addr = ip_vce->ip_addr;
    conf->mask_len = ip_vce->mask_len;
    conf->vid = ip_vce->vid;

    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            conf->ports[sit.isid - VTSS_ISID_START][pit.iport] = VTSS_PORT_BF_GET(ip_vce->ports[sit.isid - VTSS_ISID_START], pit.iport);
        }
    }
}

static void vcl_ip_vcel2mgmtl_conf(vtss_isid_t isid, vcl_ip_vce_conf_local_t *ip_vce, vcl_ip_mgmt_vce_conf_local_t *conf)
{
    port_iter_t pit;

    vtss_clear(*conf);

    conf->ip_addr = ip_vce->ip_addr;
    conf->mask_len = ip_vce->mask_len;
    conf->vid = ip_vce->vid;

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        conf->ports[pit.iport] = VTSS_PORT_BF_GET(ip_vce->ports, pit.iport);
    }
}

mesa_rc vcl_ip_mgmt_conf_add(vtss_isid_t isid_add, vcl_ip_mgmt_vce_conf_local_t *ip_vce)
{
    vcl_ip_vce_conf_global_t entry;
    vcl_ip_vce_conf_local_t  conf;
    mesa_rc                  rc = VTSS_RC_OK;
    switch_iter_t            sit;
    char                     ip_str[100];

    if (!msg_switch_is_primary()) {
        T_W("Switch is not the primary switch - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (!(VTSS_ISID_LEGAL(isid_add) || (isid_add == VTSS_ISID_GLOBAL))) {
        T_E("Invalid ISID: %u - LEGAL one expected", isid_add);
        return VCL_ERROR_INVALID_ISID;
    }

    if (ip_vce == NULL) {
        T_E("Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (ip_vce->mask_len > 32) {
        T_W("Mask length must be between 0 and 32");
        return VCL_ERROR_INVALID_MASK_LENGTH;
    }
    if (ip_vce->vid < VTSS_APPL_VLAN_ID_MIN || ip_vce->vid > VTSS_APPL_VLAN_ID_MAX) {
        T_W("VLAN ID must be between 1 and 4095");
        return VCL_ERROR_INVALID_VLAN_ID;
    }

    vcl_ip_addr2sub(&ip_vce->ip_addr, ip_vce->mask_len);
    T_D("MGMT API adding VCE with: Subnet %s/%u, VID %u, on switch %u",
        misc_ipv4_txt(ip_vce->ip_addr, ip_str), ip_vce->mask_len, ip_vce->vid, isid_add);

    /**
    * Convert the vcl_mac_vlan_mgmt_entry_t to a vcl_mac_vlan_conf_entry_t.
    **/
    vcl_ip_mgmtl2vceg_conf(isid_add, ip_vce, &entry);

    if ((rc = vcl_ip_vce_global_add(&entry)) != VTSS_RC_OK) {
        return rc;
    } else {
        conf.id = entry.id;
        conf.ip_addr = entry.ip_addr;
        conf.mask_len = entry.mask_len;
        conf.vid = entry.vid;
        (void)switch_iter_init(&sit, isid_add, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
            vcl_ports_global2local(entry.ports, conf.ports, sit.isid);
            if ((rc = vcl_msg_ip_vce_add_del(sit.isid, &conf, TRUE)) != VTSS_RC_OK) {
                return rc;
            }
        }
    }
    T_D("MGMT API added the above VCE");
    return VTSS_RC_OK;
}

mesa_rc vcl_ip_mgmt_conf_del(vtss_isid_t isid_del, vcl_ip_mgmt_vce_conf_local_t *ip_vce)
{
    vcl_ip_vce_conf_global_t entry;
    vcl_ip_vce_conf_local_t  conf;
    mesa_rc                  rc = VTSS_RC_OK;
    u32                      port_num;
    char                     ip_str[100];

    if (!msg_switch_is_primary()) {
        T_W("Switch is not the primary switch - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (!(VTSS_ISID_LEGAL(isid_del))) {
        T_E("Invalid ISID: %u - LEGAL one expected", isid_del);
        return VCL_ERROR_INVALID_ISID;
    }

    if (ip_vce == NULL) {
        T_E("Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (ip_vce->mask_len > 32) {
        T_E("Mask length must be between 0 and 32 - Received mask length of %u", ip_vce->mask_len);
        return VCL_ERROR_INVALID_MASK_LENGTH;
    }

    vcl_ip_addr2sub(&ip_vce->ip_addr, ip_vce->mask_len);

    T_D("MGMT API deleting/editing VCE with: Subnet %s/%u, on switch %u",
        misc_ipv4_txt(ip_vce->ip_addr, ip_str), ip_vce->mask_len, isid_del);

    entry.ip_addr = ip_vce->ip_addr;
    entry.mask_len = ip_vce->mask_len;
    memset(entry.ports, 0, sizeof(entry.ports));
    for (port_num = 0; port_num < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_num++) {
        VTSS_PORT_BF_SET(entry.ports[isid_del - VTSS_ISID_START], port_num, 1);
    }
    if ((rc = vcl_ip_vce_global_del(&entry)) != VTSS_RC_OK) {
        return rc;
    } else {
        conf.id = entry.id;
        conf.ip_addr = entry.ip_addr;
        conf.mask_len = entry.mask_len;
        if ((rc = vcl_msg_ip_vce_add_del(isid_del, &conf, FALSE)) != VTSS_RC_OK) {
            return rc;
        }
    }
    T_D("MGMT API deleted/edited the above VCE");
    return VTSS_RC_OK;
}

mesa_rc vcl_ip_mgmt_conf_get(vtss_isid_t isid_get, vcl_ip_mgmt_vce_conf_global_t *ip_vce, BOOL first, BOOL next)
{
    vcl_ip_vce_conf_global_t entry;
    mesa_rc                  rc = VTSS_RC_OK;
    BOOL                     found_sid = FALSE, first_l, next_l;
    port_iter_t              pit;
    char                     ip_str[100];

    if (!msg_switch_is_primary()) {
        T_W("Switch is not the primary switch - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (!VTSS_ISID_LEGAL(isid_get) && (isid_get != VTSS_ISID_GLOBAL)) {
        T_E("Invalid ISID: %u - LEGAL one expected", isid_get);
        return VCL_ERROR_INVALID_ISID;
    }

    if (ip_vce == NULL) {
        T_E("Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first == FALSE) {
        if (ip_vce->mask_len > 32) {
            T_E("Mask length must be between 0 and 32");
            return VCL_ERROR_INVALID_MASK_LENGTH;
        }
    }

    if (first) {
        T_D("MGMT API fetching the first VCE from global list");
    } else {
        vcl_ip_addr2sub(&ip_vce->ip_addr, ip_vce->mask_len);
        T_D("MGMT API fetching VCE from global list with: Subnet %s/%u, first = %d, next = %d",
            misc_ipv4_txt(ip_vce->ip_addr, ip_str), ip_vce->mask_len, first, next);
    }

    memset(&ip_vce->vid, 0, sizeof(ip_vce->vid));
    vtss_clear(ip_vce->ports);
    memset(&entry, 0, sizeof(entry));

    entry.ip_addr = ip_vce->ip_addr;
    entry.mask_len = ip_vce->mask_len;
    first_l = first;
    next_l = next;
    while ((rc = vcl_ip_vce_global_get(&entry, first_l, next_l)) == VTSS_RC_OK) {
        T_D("Subnet is %s/%u, VID = %d", misc_ipv4_txt(entry.ip_addr, ip_str), entry.mask_len, entry.vid);
        vcl_ip_vceg2mgmtg_conf(isid_get, &entry, ip_vce);
        if (isid_get == VTSS_ISID_GLOBAL) {
            found_sid = TRUE;
        } else {
            (void)port_iter_init(&pit, NULL, isid_get, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (ip_vce->ports[isid_get - VTSS_ISID_START][pit.iport] == 1) {
                    found_sid = TRUE;
                    break;
                }
            }
        }
        if (found_sid == FALSE) {
            next_l = TRUE;
            first_l = FALSE;
            T_N("MGMT API found the above VCE in the global list, but rejected it since it is not present in the requested switch #%u",
                isid_get);
            continue;
        }
        T_D("MGMT API found the above VCE");
        break;
    }

    T_D("MGMT API Exit");
    return rc;
}

mesa_rc vcl_ip_mgmt_conf_local_get(vcl_ip_mgmt_vce_conf_local_t *ip_vce, BOOL first, BOOL next)
{
    mesa_rc                 rc;
    vcl_ip_vce_conf_local_t entry;
    char                    ip_str[100];

    if (ip_vce == NULL) {
        T_E("Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first == FALSE) {
        if (ip_vce->mask_len > 32) {
            T_E("Mask length must be between 0 and 32");
            return VCL_ERROR_INVALID_MASK_LENGTH;
        }
    }

    if (first) {
        T_D("MGMT API fetching the first VCE from local list");
    } else {
        vcl_ip_addr2sub(&ip_vce->ip_addr, ip_vce->mask_len);
        T_D("MGMT API fetching VCE from local list with: Subnet %s/%u, first = %d, next = %d",
            misc_ipv4_txt(ip_vce->ip_addr, ip_str), ip_vce->mask_len, first, next);
    }

    memset(&entry, 0, sizeof(entry));
    entry.ip_addr = ip_vce->ip_addr;
    entry.mask_len = ip_vce->mask_len;
    if ((rc = vcl_ip_vce_local_get(&entry, first, next)) != VTSS_RC_OK) {
        return rc;
    } else {
        vcl_ip_vcel2mgmtl_conf(VTSS_ISID_LOCAL, &entry, ip_vce);
    }
    T_D("MGMT API found the above VCE");
    return VTSS_RC_OK;
}

mesa_rc vcl_ip_mgmt_conf_itr(mesa_ipv4_network_t *sub, BOOL first)
{
    vcl_ip_vce_global_t **lused, *tmp_vce = NULL;
    mesa_ipv4_network_t sub2, stored = {};
    char                ip_str[100];
    u8                  res;
    BOOL                has_stored = FALSE;

    if (!msg_switch_is_primary()) {
        T_WG(TRACE_GRP_MIB, "Switch is not the primary switch - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (sub == NULL) {
        T_EG(TRACE_GRP_MIB, "Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first) {
        T_DG(TRACE_GRP_MIB, "Iterator fetching the first Subnet VCE from the list");
    } else {
        if (sub->prefix_size > 32) {
            sub->prefix_size = 32;
        }
        //vcl_ip_addr2sub(&sub->address, (u8)sub->prefix_size);
        T_DG(TRACE_GRP_MIB, "Iterator fetching the Subnet VCE after the one with: Subnet %s/%u",
             misc_ipv4_txt(sub->address, ip_str), sub->prefix_size);
    }

    VCL_CRIT_ENTER();
    lused = &vcl_data.ip_data.global_used;

    for (tmp_vce = *lused; tmp_vce != NULL; tmp_vce = tmp_vce->next) {
        sub2.address = tmp_vce->conf.ip_addr;
        sub2.prefix_size = tmp_vce->conf.mask_len;
        res = vcl_sub_cmp_snmp(sub, &sub2);
        if (res == 0) {
            continue;
        } else if (res == 1) {
            if (has_stored == TRUE) {
                res = vcl_sub_cmp_snmp(&stored, &sub2);
                if (res == 2) {
                    stored.address = tmp_vce->conf.ip_addr;
                    stored.prefix_size = tmp_vce->conf.mask_len;
                }
            } else {
                stored.address = tmp_vce->conf.ip_addr;
                stored.prefix_size = tmp_vce->conf.mask_len;
                has_stored = TRUE;
            }
            continue;
        } else if (res == 2) {
            continue;
        }
    }

    if (has_stored == TRUE) {
        *sub = stored;
        T_DG(TRACE_GRP_MIB, "Found the next Subnet VCE with: Subnet %s/%u", misc_ipv4_txt(sub->address, ip_str), sub->prefix_size);
    } else {
        T_DG(TRACE_GRP_MIB, "There is no Subnet VCE after the provided one");
    }
    VCL_CRIT_EXIT();
    return (has_stored == FALSE ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK);
}

static void vcl_ip_default_set(void)
{
    vcl_ip_vce_conf_local_t entry;
    mesa_vce_id_t           vce_id;
    mesa_rc                 rc;

    /* Delete all the existing entries */
    while ((vcl_ip_vce_local_get(&entry, TRUE, FALSE)) == VTSS_RC_OK) {
        if (vcl_ip_vce_local_del(&entry) == VTSS_RC_OK) {
            vce_id = entry.id;
            vce_id = ((vce_id & 0xFFFF) | ((VCL_TYPE_IP & 0xF) << 16));
            /* Call the switch API */
            if (VTSS_RC_OK != vcl_unregister_vce(VCL_USR_DEFAULT, vce_id)) {
                T_D("Failure while unregistering vce %d", vce_id);
            }
            if ((rc = mesa_vce_del(NULL, vce_id)) != VTSS_RC_OK) {
                T_D("Failure while deleting Subnet entries (rc = %s)", error_txt(rc));
            } else {
                vce_id = (vce_id | (0x1 << 20));
                if (VTSS_RC_OK != vcl_unregister_vce(VCL_USR_DEFAULT, vce_id)) {
                    T_D("Failure while unregistering vce %d", vce_id);
                }
                if ((rc = mesa_vce_del(NULL, vce_id)) != VTSS_RC_OK) {
                    T_D("Failure while deleting Subnet entries (priority tagged) (rc = %s)", error_txt(rc));
                }
            }
        }
    }

    memset(&vcl_data.ip_data, 0, sizeof(vcl_ip_data_t));
    vcl_ip_global_default_set();
    vcl_ip_local_default_set();
}

static void vcl_proto_group_proto_default_set(void)
{
    vcl_proto_group_proto_t *proto_p, **lfree, **lused;
    u32                     i;

    VCL_CRIT_ENTER();
    /* Initialize MAC-based VLAN Local list */
    lfree = &vcl_data.proto_data.group_proto_free;
    lused = &vcl_data.proto_data.group_proto_used;
    *lfree = NULL;
    *lused = NULL;
    for (i = 0; i < VCL_PROTO_PROTOCOL_MAX; i++) {
        proto_p = &vcl_data.proto_data.group_proto_table[i];
        proto_p->next = *lfree;
        *lfree = proto_p;
    }
    VCL_CRIT_EXIT();
}

static void vcl_proto_group_entry_default_set(void)
{
    vcl_proto_group_entry_t *proto_p, **lfree, **lused;
    u32                     i;

    VCL_CRIT_ENTER();
    /* Initialize MAC-based VLAN Local list */
    lfree = &vcl_data.proto_data.group_entry_free;
    lused = &vcl_data.proto_data.group_entry_used;
    *lfree = NULL;
    *lused = NULL;
    for (i = 0; i < VCL_PROTO_PROTOCOL_MAX; i++) {
        proto_p = &vcl_data.proto_data.group_entry_table[i];
        proto_p->next = *lfree;
        *lfree = proto_p;
    }
    VCL_CRIT_EXIT();
}

static void vcl_proto_global_default_set(void)
{
    vcl_proto_vce_global_t *proto_p, **lfree, **lused;
    u32                    i;

    VCL_CRIT_ENTER();
    /* Initialize MAC-based VLAN Global list */
    lfree = &vcl_data.proto_data.global_free;
    lused = &vcl_data.proto_data.global_used;
    *lfree = NULL;
    *lused = NULL;
    for (i = 0; i < VCL_PROTO_VCE_MAX; i++) {
        proto_p = &vcl_data.proto_data.global_table[i];
        proto_p->conf.id = VCL_PROTO_VCE_MAX - i;
        proto_p->next = *lfree;
        *lfree = proto_p;
    }
    VCL_CRIT_EXIT();
}

static void vcl_proto_local_default_set(void)
{
    vcl_proto_vce_local_t *proto_p, **lfree, **lused;
    u32                   i;

    VCL_CRIT_ENTER();
    /* Initialize MAC-based VLAN Local list */
    lfree = &vcl_data.proto_data.local_free;
    lused = &vcl_data.proto_data.local_used;
    *lfree = NULL;
    *lused = NULL;
    for (i = 0; i < VCL_PROTO_VCE_MAX; i++) {
        proto_p = &vcl_data.proto_data.local_table[i];
        proto_p->next = *lfree;
        *lfree = proto_p;
    }
    VCL_CRIT_EXIT();
}

static mesa_rc vcl_proto_group_name_check(u8 *grp_name)
{
    uint    idx;
    BOOL    error = FALSE;

    for (idx = 0; idx < strlen((char *)grp_name); idx++) {
        if ((grp_name[idx] < 48) || (grp_name[idx] > 122)) {
            error = TRUE;
        } else {
            if ((grp_name[idx] > 57) && (grp_name[idx] < 65)) {
                error = TRUE;
            } else if ((grp_name[idx] > 90) && (grp_name[idx] < 97)) {
                error = TRUE;
            }
        }
    }
    if (error == TRUE) {
        return VCL_ERROR_INVALID_GROUP_NAME;
    }
    return VTSS_RC_OK;
}

const char *vcl_proto_mgmt_encaptype2string(vtss_appl_vcl_proto_encap_type_t encap)
{
    switch (encap) {
    case VTSS_APPL_VCL_PROTO_ENCAP_ETH2:
        return "EthernetII";
    case VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP:
        return "LLC_SNAP";
    case VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER:
        return "LLC_Other";
    }
    return "";
}

char *vcl_proto_mgmt_protocol2string(vtss_appl_vcl_proto_encap_type_t proto_encap_type, vtss_appl_vcl_proto_encap_t proto)
{
    /*lint -esym(459, s)      */
    static char s[80];

    if (proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_ETH2) {
        sprintf(s, "%s ETYPE:0x%x", vcl_proto_mgmt_encaptype2string(proto_encap_type), proto.eth2_proto.eth_type);
    } else if (proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP) {
        sprintf(s, "%s OUI-%02x:%02x:%02x PID:0x%x", vcl_proto_mgmt_encaptype2string(proto_encap_type),
                proto.llc_snap_proto.oui[0], proto.llc_snap_proto.oui[1], proto.llc_snap_proto.oui[2],
                proto.llc_snap_proto.pid);
    } else {
        sprintf(s, "%s DSAP:0x%x SSAP:0x%x", vcl_proto_mgmt_encaptype2string(proto_encap_type), proto.llc_other_proto.dsap,
                proto.llc_other_proto.ssap);
    }
    return s;
}

static mesa_rc vcl_proto_group_proto_add(vcl_proto_group_conf_proto_t *conf)
{
    vcl_proto_group_proto_t **lfree, **lused, *tmp_group, *prev_group, *new_group, *ins = NULL, *ins_prev = NULL;
    vtss_appl_vcl_proto_t   enc1, enc2;
    mesa_rc                 rc = VTSS_RC_OK;
    u8                      res;

    if (conf == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    /* Check group name - it should only contain alphabets or digits */
    if ((rc = vcl_proto_group_name_check(conf->name)) != VTSS_RC_OK) {
        return rc;
    }

    T_D("Adding group to Group(Protocol) list: Group %s Protocol %s",
        conf->name, vcl_proto_mgmt_protocol2string(conf->proto_encap_type, conf->proto));

    VCL_CRIT_ENTER();
    lfree = &vcl_data.proto_data.group_proto_free;
    lused = &vcl_data.proto_data.group_proto_used;

    enc1.proto_encap_type = conf->proto_encap_type;
    enc1.proto = conf->proto;
    for (tmp_group = *lused, prev_group = NULL; tmp_group != NULL; prev_group = tmp_group, tmp_group = tmp_group->next) {
        enc2.proto_encap_type = tmp_group->conf.proto_encap_type;
        enc2.proto = tmp_group->conf.proto;
        res = vcl_proto_encap_cmp(&enc1, &enc2);
        if (res == 0) {
            VCL_CRIT_EXIT();
            return VCL_ERROR_PROTOCOL_ALREADY_CONF;
        } else if (res == 1) {
            ins = tmp_group;
            ins_prev = prev_group;
            break;
        } else if (res == 2) {
            continue;
        }
    }

    /* Get free node from free list */
    new_group = *lfree;
    if (new_group == NULL) {
        VCL_CRIT_EXIT();
        return VCL_ERROR_GROUP_PROTO_TABLE_FULL;
    }
    /* Update the free list */
    *lfree = new_group->next;
    /* Copy the configuration */
    new_group->conf = *conf;
    /* Update the used list */
    new_group->next = NULL;
    if (ins == NULL) { /* Add the entry to the end of list */
        if (*lused == NULL) {
            /* Adding first entry to the empty list */
            *lused = new_group;
        } else {
            /* Adding the entry after last entry in the list */
            prev_group->next = new_group;
        }
    } else { /* Add the entry to either head or middle of the list */
        if (ins_prev != NULL) { /* Add the entry to the middle of the list */
            ins_prev->next = new_group;
            new_group->next = ins;
        } else { /* Add the entry before first entry */
            new_group->next = *lused;
            *lused = new_group;
        }
    }
    T_D("Added the above protocol to group: %s", conf->name);
    VCL_CRIT_EXIT();
    return VTSS_RC_OK;
}

static mesa_rc vcl_proto_group_proto_del(vcl_proto_group_conf_proto_t *conf)
{
    vcl_proto_group_proto_t **lfree, **lused, *tmp_group, *prev_group;
    BOOL                    found_entry = FALSE;

    /* Check for NULL pointer */
    if (conf == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    /* Check for valid encap type */
    if (!((conf->proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_ETH2) || (conf->proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP) ||
          (conf->proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER))) {
        return VCL_ERROR_INVALID_ENCAP_TYPE;
    }

    T_D("Deleting group/protocol from Group(Protocol) list: Protocol %s",
        vcl_proto_mgmt_protocol2string(conf->proto_encap_type, conf->proto));

    VCL_CRIT_ENTER();
    lfree = &vcl_data.proto_data.group_proto_free;
    lused = &vcl_data.proto_data.group_proto_used;
    /* Search for existing entry */
    for (tmp_group = *lused, prev_group = NULL; tmp_group != NULL; prev_group = tmp_group, tmp_group = tmp_group->next) {
        if (tmp_group->conf.proto_encap_type == conf->proto_encap_type) {
            if ((tmp_group->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_ETH2) &&
                (conf->proto.eth2_proto.eth_type == tmp_group->conf.proto.eth2_proto.eth_type)) {
                found_entry = TRUE;
                break;
            } else if (tmp_group->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP) {
                if (!memcmp(conf->proto.llc_snap_proto.oui, tmp_group->conf.proto.llc_snap_proto.oui, OUI_SIZE)
                    && (conf->proto.llc_snap_proto.pid == tmp_group->conf.proto.llc_snap_proto.pid)) {
                    found_entry = TRUE;
                    break;
                }
            } else if (tmp_group->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER) {
                if ((conf->proto.llc_other_proto.dsap == tmp_group->conf.proto.llc_other_proto.dsap)
                    && (conf->proto.llc_other_proto.ssap == tmp_group->conf.proto.llc_other_proto.ssap)) {
                    found_entry = TRUE;
                    break;
                }
            }
        }
    }

    if (tmp_group != NULL) {
        /* Move entry from used list to free list */
        if (prev_group == NULL) {
            *lused = tmp_group->next;
        } else {
            prev_group->next = tmp_group->next;
        }
        memcpy(conf->name, tmp_group->conf.name, MAX_GROUP_NAME_LEN);
        /* Move entry from used list to free list */
        memset(&tmp_group->conf, 0, sizeof(tmp_group->conf));
        tmp_group->next = *lfree;
        *lfree = tmp_group;
        T_D("Deleted the above protocol from group: %s", conf->name);
    }
    VCL_CRIT_EXIT();
    return (found_entry == FALSE) ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK;
}

static mesa_rc vcl_proto_group_proto_get(vcl_proto_group_conf_proto_t *conf, BOOL first, BOOL next)
{
    vcl_proto_group_proto_t **lused, *tmp_group = NULL;
    BOOL                    use_next = FALSE;

    if (conf == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first) {
        T_D("Switch fetching the first entry from the Group(Protocol) list");
    } else {
        T_D("Switch fetching group from the Group(Protocol) list with: Protocol %s, first = %d, next = %d",
            vcl_proto_mgmt_protocol2string(conf->proto_encap_type, conf->proto), first, next);
    }

    VCL_CRIT_ENTER();
    lused = &vcl_data.proto_data.group_proto_used;

    for (tmp_group = *lused; tmp_group != NULL; tmp_group = tmp_group->next) {
        if (first == TRUE) {
            break;
        } else {
            if (use_next) {
                break;
            }
            if (tmp_group->conf.proto_encap_type == conf->proto_encap_type) {
                if ((tmp_group->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_ETH2) &&
                    (conf->proto.eth2_proto.eth_type == tmp_group->conf.proto.eth2_proto.eth_type)) {
                    if (next) {
                        use_next = TRUE;
                    } else {
                        break;
                    }
                } else if (tmp_group->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP) {
                    if (!memcmp(conf->proto.llc_snap_proto.oui, tmp_group->conf.proto.llc_snap_proto.oui, OUI_SIZE)
                        && (conf->proto.llc_snap_proto.pid == tmp_group->conf.proto.llc_snap_proto.pid)) {
                        if (next) {
                            use_next = TRUE;
                        } else {
                            break;
                        }
                    }
                } else if (tmp_group->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER) {
                    if ((conf->proto.llc_other_proto.dsap == tmp_group->conf.proto.llc_other_proto.dsap)
                        && (conf->proto.llc_other_proto.ssap == tmp_group->conf.proto.llc_other_proto.ssap)) {
                        if (next) {
                            use_next = TRUE;
                        } else {
                            break;
                        }
                    }
                }
            }
        }
    }

    if (tmp_group != NULL) {
        T_D("Found group named: %s", tmp_group->conf.name);
        *conf = tmp_group->conf;
    }
    VCL_CRIT_EXIT();
    return (tmp_group == NULL ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK);
}

static mesa_rc vcl_proto_group_proto_name_get(vcl_proto_group_conf_proto_t *conf, BOOL first, BOOL next)
{
    vcl_proto_group_proto_t **lused, *tmp_group = NULL;
    BOOL                    use_next = FALSE;

    if (conf == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first) {
        T_D("Switch fetching the first entry from the Group(Protocol) list named %s", conf->name);
    } else {
        T_D("Switch fetching group from the Group(Protocol) list placed after the one named: %s with first = %d, next = %d",
            conf->name, first, next);
    }

    VCL_CRIT_ENTER();
    lused = &vcl_data.proto_data.group_proto_used;

    for (tmp_group = *lused; tmp_group != NULL; tmp_group = tmp_group->next) {
        if (first == TRUE) {
            if (!memcmp(tmp_group->conf.name, conf->name, MAX_GROUP_NAME_LEN)) {
                break;
            } else {
                continue;
            }
        } else {
            if (!memcmp(tmp_group->conf.name, conf->name, MAX_GROUP_NAME_LEN)) {
                if (use_next) {
                    break;
                }
                if (tmp_group->conf.proto_encap_type == conf->proto_encap_type) {
                    if ((tmp_group->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_ETH2) &&
                        (conf->proto.eth2_proto.eth_type == tmp_group->conf.proto.eth2_proto.eth_type)) {
                        use_next = TRUE;
                        continue;
                    }
                } else if (tmp_group->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP) {
                    if (!memcmp(conf->proto.llc_snap_proto.oui, tmp_group->conf.proto.llc_snap_proto.oui, OUI_SIZE)
                        && (conf->proto.llc_snap_proto.pid == tmp_group->conf.proto.llc_snap_proto.pid)) {
                        use_next = TRUE;
                        continue;
                    }
                } else if (tmp_group->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER) {
                    if ((conf->proto.llc_other_proto.dsap == tmp_group->conf.proto.llc_other_proto.dsap)
                        && (conf->proto.llc_other_proto.ssap == tmp_group->conf.proto.llc_other_proto.ssap)) {
                        use_next = TRUE;
                        continue;
                    }
                }
            }
        }
    }

    if (tmp_group != NULL) {
        T_D("Found group named: %s", tmp_group->conf.name);
        *conf = tmp_group->conf;
    }
    VCL_CRIT_EXIT();
    return (tmp_group == NULL ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK);
}

static mesa_rc vcl_proto_group_entry_add(vcl_proto_group_conf_entry_t *conf)
{
    vcl_proto_group_entry_t **lfree, **lused, *tmp_group, *prev_group, *new_group, *ins = NULL, *ins_prev = NULL;
    mesa_rc                 rc = VTSS_RC_OK;
    u32                     isid, i;
    int                     res;
    BOOL                    update = FALSE;

    if (conf == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    /* Check group name - it should only contain alphabets or digits */
    if ((rc = vcl_proto_group_name_check(conf->name)) != VTSS_RC_OK) {
        return rc;
    }

    T_D("Adding group to Group(Entry) list: Group %s, VID %u", conf->name, conf->vid);

    VCL_CRIT_ENTER();
    lfree = &vcl_data.proto_data.group_entry_free;
    lused = &vcl_data.proto_data.group_entry_used;

    for (tmp_group = *lused, prev_group = NULL; tmp_group != NULL; prev_group = tmp_group, tmp_group = tmp_group->next) {
        res = strcmp((char *)conf->name, (char *)tmp_group->conf.name);
        if (res == 0) {
            if (tmp_group->conf.vid == conf->vid) {
                update = TRUE;
                ins = tmp_group;
                break;
            } else {
                VCL_CRIT_EXIT();
                return VCL_ERROR_ENTRY_DIFF_VID;
            }
        } else if (res < 0) {
            ins = tmp_group;
            ins_prev = prev_group;
            break;
        } else if (res > 0) {
            continue;
        }
    }

    if (update == FALSE) {
        /* Get free node from free list */
        new_group = *lfree;
        if (new_group == NULL) {
            VCL_CRIT_EXIT();
            return VCL_ERROR_GROUP_ENTRY_TABLE_FULL;
        }
        /* Update the free list */
        *lfree = new_group->next;
        /* Copy the configuration */
        new_group->conf = *conf;
        /* Update the used list */
        new_group->next = NULL;
        if (ins == NULL) { /* Add the entry to the end of list */
            if (*lused == NULL) {
                /* Adding first entry to the empty list */
                *lused = new_group;
            } else {
                /* Adding the entry after last entry in the list */
                prev_group->next = new_group;
            }
        } else { /* Add the entry to either head or middle of the list */
            if (ins_prev != NULL) { /* Add the entry to the middle of the list */
                ins_prev->next = new_group;
                new_group->next = ins;
            } else { /* Add the entry before first entry */
                new_group->next = *lused;
                *lused = new_group;
            }
        }
        T_D("Added the above group entry");
    } else {
        if (ins != NULL) { /* This case always happens. But, this check is for satisfying LINT */
            for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
                for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
                    ins->conf.ports[isid - VTSS_ISID_START][i] = conf->ports[isid - VTSS_ISID_START][i];
                }
            }
            T_D("Appended to existing group entry");
        }
    }
    VCL_CRIT_EXIT();
    return VTSS_RC_OK;
}

static mesa_rc vcl_proto_group_entry_del(vcl_proto_group_conf_entry_t *conf)
{
    vcl_proto_group_entry_t **lfree, **lused, *tmp_group, *prev_group;
    BOOL                    found_entry = FALSE, ports_exist = FALSE;
    u32                     isid, i;
    mesa_rc                 rc = VTSS_RC_OK;

    /* Check for NULL pointer */
    if (conf == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if ((rc = vcl_proto_group_name_check(conf->name)) != VTSS_RC_OK) {
        return rc;
    }

    T_D("Deleting group from Group(Entry) list: Group %s, VID %u", conf->name, conf->vid);

    VCL_CRIT_ENTER();
    lfree = &vcl_data.proto_data.group_entry_free;
    lused = &vcl_data.proto_data.group_entry_used;
    /* Search for existing entry */
    for (tmp_group = *lused, prev_group = NULL; tmp_group != NULL; prev_group = tmp_group, tmp_group = tmp_group->next) {
        if (!strcmp((char *)tmp_group->conf.name, (char *)conf->name)) {
            found_entry = TRUE;
            for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
                for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
                    tmp_group->conf.ports[isid - VTSS_ISID_START][i] &= (~conf->ports[isid - VTSS_ISID_START][i]);
                    if (tmp_group->conf.ports[isid - VTSS_ISID_START][i]) {
                        ports_exist = TRUE;
                    }
                }
            }
            if (ports_exist == FALSE) {
                if (prev_group == NULL) {
                    *lused = tmp_group->next;
                } else {
                    prev_group->next = tmp_group->next;
                }
                // Erase previous configuration before returning it to the free list
                memset(&tmp_group->conf, 0, sizeof(tmp_group->conf));
                tmp_group->next = *lfree;
                *lfree = tmp_group;
                T_D("Deleted the above group entry");
                break;
            }
            T_D("Edited the existing group entry");
            break;
        }
    }
    VCL_CRIT_EXIT();
    return (found_entry == FALSE) ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK;
}

static mesa_rc vcl_proto_group_entry_get(vcl_proto_group_conf_entry_t *conf, BOOL first, BOOL next)
{
    vcl_proto_group_entry_t **lused, *tmp_group = NULL;
    BOOL                    use_next = FALSE;
    mesa_rc                 rc = VTSS_RC_OK;

    if (conf == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first == FALSE) {
        if ((rc = vcl_proto_group_name_check(conf->name)) != VTSS_RC_OK) {
            return rc;
        }
    }

    if (first) {
        T_D("Switch fetching the first entry from the Group(Entry) list");
    } else {
        T_D("Switch fetching an entry from the Group(Entry) list with: Group %s, VID %u, first = %d, next = %d",
            conf->name, conf->vid, first, next);
    }

    VCL_CRIT_ENTER();
    lused = &vcl_data.proto_data.group_entry_used;

    for (tmp_group = *lused; tmp_group != NULL; tmp_group = tmp_group->next) {
        if (first == TRUE) {
            break;
        } else {
            if (use_next) {
                break;
            }
            if (!strcmp((char *)tmp_group->conf.name, (char *)conf->name)) {
                if (next) {
                    use_next = TRUE;
                } else {
                    break;
                }
            }
        }
    }

    if (tmp_group != NULL) {
        T_D("Found entry with Group: %s, VID %u", tmp_group->conf.name, tmp_group->conf.vid);
        *conf = tmp_group->conf;
    }
    VCL_CRIT_EXIT();
    return (tmp_group == NULL ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK);
}

static mesa_rc vcl_proto_vce_global_add(vcl_proto_vce_conf_global_t *proto_vce)
{
    vcl_proto_vce_global_t **lfree, **lused, *tmp_vce, *prev_vce, *new_vce, *ins = NULL;
    BOOL                   update = FALSE, proto_exists = FALSE;
    u32                    isid, i;

    if (proto_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    T_D("Adding VCE to global list: Protocol %s, VID %u",
        vcl_proto_mgmt_protocol2string(proto_vce->proto_encap_type, proto_vce->proto), proto_vce->vid);

    VCL_CRIT_ENTER();
    lfree = &vcl_data.proto_data.global_free;
    lused = &vcl_data.proto_data.global_used;

    for (tmp_vce = *lused, prev_vce = NULL; tmp_vce != NULL; prev_vce = tmp_vce, tmp_vce = tmp_vce->next) {
        proto_exists = FALSE;
        if (tmp_vce->conf.proto_encap_type == proto_vce->proto_encap_type) {
            if ((tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_ETH2) &&
                (proto_vce->proto.eth2_proto.eth_type == tmp_vce->conf.proto.eth2_proto.eth_type)) {
                proto_exists = TRUE;
            } else if (tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP) {
                if (!memcmp(proto_vce->proto.llc_snap_proto.oui, tmp_vce->conf.proto.llc_snap_proto.oui, OUI_SIZE)
                    && (proto_vce->proto.llc_snap_proto.pid == tmp_vce->conf.proto.llc_snap_proto.pid)) {
                    proto_exists = TRUE;
                }
            } else if (tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER) {
                if ((proto_vce->proto.llc_other_proto.dsap == tmp_vce->conf.proto.llc_other_proto.dsap)
                    && (proto_vce->proto.llc_other_proto.ssap == tmp_vce->conf.proto.llc_other_proto.ssap)) {
                    proto_exists = TRUE;
                }
            }
        }
        if (proto_exists == TRUE) {
            update = TRUE;
            ins = tmp_vce;
            break;
        }
    }
    if (update == FALSE) {
        /* Get free node from free list */
        new_vce = *lfree;
        if (new_vce == NULL) {
            VCL_CRIT_EXIT();
            return VCL_ERROR_PROTO_TABLE_FULL;
        }
        /* Update the free list */
        *lfree = new_vce->next;
        /* Copy the configuration */
        proto_vce->id = new_vce->conf.id;
        new_vce->conf = *proto_vce;
        /* Update the used list */
        new_vce->next = NULL;
        if (*lused == NULL) {
            /* Adding first entry to the empty list */
            *lused = new_vce;
        } else {
            /* Adding the entry after last entry in the list */
            prev_vce->next = new_vce;
        }
        T_D("Added new VCE with id: %u", proto_vce->id);
    } else { /* Update the entry */
        if (ins != NULL) { /* This case always happens. But, this check is for satisfying LINT */
            for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
                for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
                    ins->conf.ports[isid - VTSS_ISID_START][i] = proto_vce->ports[isid - VTSS_ISID_START][i];
                }
            }
            proto_vce->id = ins->conf.id;
            T_D("Appended to existing VCE with id: %u", proto_vce->id);
        }
    }
    VCL_CRIT_EXIT();
    return VTSS_RC_OK;
}

static mesa_rc vcl_proto_vce_global_del(vcl_proto_vce_conf_global_t *proto_vce)
{
    vcl_proto_vce_global_t **lfree, **lused, *tmp_vce, *prev_vce, *tmp_vce_free, *prev_vce_free;
    BOOL                   found_entry = FALSE, found_proto = FALSE, ports_exist = FALSE;
    mesa_vce_id_t          id;
    u32                    isid, i;

    if (proto_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    T_D("Deleting VCE from global list: Protocol %s, VID %u",
        vcl_proto_mgmt_protocol2string(proto_vce->proto_encap_type, proto_vce->proto), proto_vce->vid);

    VCL_CRIT_ENTER();
    lfree = &vcl_data.proto_data.global_free;
    lused = &vcl_data.proto_data.global_used;

    for (tmp_vce = *lused, prev_vce = NULL; tmp_vce != NULL; prev_vce = tmp_vce, tmp_vce = tmp_vce->next) {
        found_proto = FALSE;
        if (tmp_vce->conf.proto_encap_type == proto_vce->proto_encap_type) {
            if ((tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_ETH2) &&
                (proto_vce->proto.eth2_proto.eth_type == tmp_vce->conf.proto.eth2_proto.eth_type)) {
                found_proto = TRUE;
            } else if (tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP) {
                if (!memcmp(proto_vce->proto.llc_snap_proto.oui, tmp_vce->conf.proto.llc_snap_proto.oui, OUI_SIZE)
                    && (proto_vce->proto.llc_snap_proto.pid == tmp_vce->conf.proto.llc_snap_proto.pid)) {
                    found_proto = TRUE;
                }
            } else if (tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER) {
                if ((proto_vce->proto.llc_other_proto.dsap == tmp_vce->conf.proto.llc_other_proto.dsap)
                    && (proto_vce->proto.llc_other_proto.ssap == tmp_vce->conf.proto.llc_other_proto.ssap)) {
                    found_proto = TRUE;
                }
            }
        }
        if (found_proto) {
            found_entry = TRUE;
            for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
                for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
                    tmp_vce->conf.ports[isid - VTSS_ISID_START][i] &= (~proto_vce->ports[isid - VTSS_ISID_START][i]);
                    if (tmp_vce->conf.ports[isid - VTSS_ISID_START][i]) {
                        ports_exist = TRUE;
                    }
                }
            }
            if (ports_exist == FALSE) {
                if (prev_vce == NULL) {
                    *lused = tmp_vce->next;
                } else {
                    prev_vce->next = tmp_vce->next;
                }
                proto_vce->id = tmp_vce->conf.id;
                // Erase previous configuration before returning it to the free list, but keep vce_id!!
                id = tmp_vce->conf.id;
                memset(&tmp_vce->conf, 0, sizeof(tmp_vce->conf));
                tmp_vce->conf.id = id;
                for (tmp_vce_free = *lfree, prev_vce_free = NULL; tmp_vce_free != NULL;
                     prev_vce_free = tmp_vce_free, tmp_vce_free = tmp_vce_free->next) {
                    if (tmp_vce->conf.id < tmp_vce_free->conf.id) {
                        break;
                    }
                }

                if (tmp_vce_free == NULL) { /* Add the entry to the end of list */
                    tmp_vce->next = NULL;
                    if (*lfree == NULL) {
                        /* Adding first entry to the empty list */
                        *lfree = tmp_vce;
                    } else {
                        /* Adding the entry after last entry in the list */
                        prev_vce_free->next = tmp_vce;
                    }
                } else { /* Add the entry to either head or middle of the list */
                    if (prev_vce_free != NULL) { /* Add the entry to the middle of the list */
                        prev_vce_free->next = tmp_vce;
                        tmp_vce->next = tmp_vce_free;
                    } else { /* Add the entry before first entry */
                        tmp_vce->next = *lfree;
                        *lfree = tmp_vce;
                    }
                }
                T_D("Deleted VCE with id: %u", tmp_vce->conf.id);
                break;
            }
            proto_vce->id = tmp_vce->conf.id;
            T_D("Appended to VCE with id: %u", tmp_vce->conf.id);
            break;
        }
    }
    VCL_CRIT_EXIT();
    return (found_entry == FALSE) ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK;
}

static mesa_rc vcl_proto_vce_global_get(vcl_proto_vce_conf_global_t *proto_vce, BOOL first, BOOL next)
{
    vcl_proto_vce_global_t **lused, *tmp_vce = NULL;
    BOOL                   use_next = FALSE, found_proto = FALSE;

    if (proto_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first) {
        T_D("Switch fetching the first VCE from global list");
    } else {
        T_D("Switch fetching VCE from global list with: Protocol %s, VID %u, first = %d, next = %d",
            vcl_proto_mgmt_protocol2string(proto_vce->proto_encap_type, proto_vce->proto), proto_vce->vid, first, next);
    }

    VCL_CRIT_ENTER();
    lused = &vcl_data.proto_data.global_used;

    for (tmp_vce = *lused; tmp_vce != NULL; tmp_vce = tmp_vce->next) {
        if (first == TRUE) {
            break;
        } else {
            if (use_next) {
                break;
            }
            found_proto = FALSE;
            if (tmp_vce->conf.proto_encap_type == proto_vce->proto_encap_type) {
                if ((tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_ETH2) &&
                    (proto_vce->proto.eth2_proto.eth_type == tmp_vce->conf.proto.eth2_proto.eth_type)) {
                    found_proto = TRUE;
                } else if (tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP) {
                    if (!memcmp(proto_vce->proto.llc_snap_proto.oui, tmp_vce->conf.proto.llc_snap_proto.oui, OUI_SIZE)
                        && (proto_vce->proto.llc_snap_proto.pid == tmp_vce->conf.proto.llc_snap_proto.pid)) {
                        found_proto = TRUE;
                    }
                } else if (tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER) {
                    if ((proto_vce->proto.llc_other_proto.dsap == tmp_vce->conf.proto.llc_other_proto.dsap)
                        && (proto_vce->proto.llc_other_proto.ssap == tmp_vce->conf.proto.llc_other_proto.ssap)) {
                        found_proto = TRUE;
                    }
                }
            }
            if (found_proto) {
                if (next) {
                    use_next = TRUE;
                } else {
                    break;
                }
            }
        }
    }

    if (tmp_vce != NULL) {
        T_D("Found VCE with id: %u", tmp_vce->conf.id);
        *proto_vce = tmp_vce->conf;
    }
    VCL_CRIT_EXIT();
    return (tmp_vce == NULL ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK);
}

static mesa_rc vcl_proto_vce_local_add(vcl_proto_vce_conf_local_t *proto_vce)
{
    vcl_proto_vce_local_t **lfree, **lused, *tmp_vce, *prev_vce, *new_vce, *ins = NULL;
    BOOL                  update = FALSE, found_proto = FALSE;
    u32                   i;

    if (proto_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    T_D("Adding VCE to local list: Protocol %s, VID %u",
        vcl_proto_mgmt_protocol2string(proto_vce->proto_encap_type, proto_vce->proto), proto_vce->vid);

    VCL_CRIT_ENTER();
    lfree = &vcl_data.proto_data.local_free;
    lused = &vcl_data.proto_data.local_used;

    for (tmp_vce = *lused, prev_vce = NULL; tmp_vce != NULL; prev_vce = tmp_vce, tmp_vce = tmp_vce->next) {
        found_proto = FALSE;
        if (tmp_vce->conf.proto_encap_type == proto_vce->proto_encap_type) {
            if ((tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_ETH2) &&
                (proto_vce->proto.eth2_proto.eth_type == tmp_vce->conf.proto.eth2_proto.eth_type)) {
                found_proto = TRUE;
            } else if (tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP) {
                if (!memcmp(proto_vce->proto.llc_snap_proto.oui, tmp_vce->conf.proto.llc_snap_proto.oui, OUI_SIZE)
                    && (proto_vce->proto.llc_snap_proto.pid == tmp_vce->conf.proto.llc_snap_proto.pid)) {
                    found_proto = TRUE;
                }
            } else if (tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER) {
                if ((proto_vce->proto.llc_other_proto.dsap == tmp_vce->conf.proto.llc_other_proto.dsap)
                    && (proto_vce->proto.llc_other_proto.ssap == tmp_vce->conf.proto.llc_other_proto.ssap)) {
                    found_proto = TRUE;
                }
            }
        }
        if (found_proto == TRUE) {
            update = TRUE;
            ins = tmp_vce;
            break;
        }
    }
    if (update == FALSE) {
        /* Get free node from free list */
        new_vce = *lfree;
        if (new_vce == NULL) {
            VCL_CRIT_EXIT();
            return VCL_ERROR_PROTO_TABLE_FULL;
        }
        /* Update the free list */
        *lfree = new_vce->next;
        /* Copy the configuration */
        new_vce->conf = *proto_vce;
        /* Update the used list */
        new_vce->next = NULL;
        if (*lused == NULL) {
            /* Adding first entry to the empty list */
            *lused = new_vce;
        } else {
            /* Adding the entry after last entry in the list */
            prev_vce->next = new_vce;
        }
        T_D("Added new VCE with id: %u", proto_vce->id);
    } else { /* Update the entry */
        if (ins != NULL) { /* This case always happens. But, this check is for satisfying LINT */
            for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
                ins->conf.ports[i] = proto_vce->ports[i];
            }
            T_D("Appended to existing VCE with id: %u", proto_vce->id);
        }
    }
    VCL_CRIT_EXIT();
    return VTSS_RC_OK;
}

static mesa_rc vcl_proto_vce_local_del(vcl_proto_vce_conf_local_t *proto_vce)
{
    vcl_proto_vce_local_t **lfree, **lused, *tmp_vce, *prev_vce;
    BOOL                   found_entry = FALSE, found_proto = FALSE;
    mesa_vce_id_t          id;

    if (proto_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    T_D("Switch deleting VCE from local list: Protocol %s, VID %u",
        vcl_proto_mgmt_protocol2string(proto_vce->proto_encap_type, proto_vce->proto), proto_vce->vid);

    VCL_CRIT_ENTER();
    lfree = &vcl_data.proto_data.local_free;
    lused = &vcl_data.proto_data.local_used;

    for (tmp_vce = *lused, prev_vce = NULL; tmp_vce != NULL; prev_vce = tmp_vce, tmp_vce = tmp_vce->next) {
        found_proto = FALSE;
        if (tmp_vce->conf.proto_encap_type == proto_vce->proto_encap_type) {
            if ((tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_ETH2) &&
                (proto_vce->proto.eth2_proto.eth_type == tmp_vce->conf.proto.eth2_proto.eth_type)) {
                found_proto = TRUE;
            } else if (tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP) {
                if (!memcmp(proto_vce->proto.llc_snap_proto.oui, tmp_vce->conf.proto.llc_snap_proto.oui, OUI_SIZE)
                    && (proto_vce->proto.llc_snap_proto.pid == tmp_vce->conf.proto.llc_snap_proto.pid)) {
                    found_proto = TRUE;
                }
            } else if (tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER) {
                if ((proto_vce->proto.llc_other_proto.dsap == tmp_vce->conf.proto.llc_other_proto.dsap)
                    && (proto_vce->proto.llc_other_proto.ssap == tmp_vce->conf.proto.llc_other_proto.ssap)) {
                    found_proto = TRUE;
                }
            }
        }
        if (found_proto) {
            found_entry = TRUE;
            if (prev_vce == NULL) {
                *lused = tmp_vce->next;
            } else {
                prev_vce->next = tmp_vce->next;
            }
            id = tmp_vce->conf.id;
            memset(&tmp_vce->conf, 0, sizeof(tmp_vce->conf));
            tmp_vce->conf.id = id;
            tmp_vce->next = *lfree;
            *lfree = tmp_vce;
            T_D("Deleted VCE with id: %u", tmp_vce->conf.id);
            break;
        }
    }
    VCL_CRIT_EXIT();
    return (found_entry == FALSE) ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK;
}

static mesa_rc vcl_proto_vce_local_get(vcl_proto_vce_conf_local_t *proto_vce, BOOL first, BOOL next)
{
    vcl_proto_vce_local_t **lused, *tmp_vce = NULL;
    BOOL                  use_next = FALSE, found_proto = FALSE;

    if (proto_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first) {
        T_D("Switch fetching the first VCE from local list");
    } else {
        T_D("Switch fetching VCE from local list with: Protocol %s, VID %u, first = %d, next = %d",
            vcl_proto_mgmt_protocol2string(proto_vce->proto_encap_type, proto_vce->proto), proto_vce->vid, first, next);
    }

    VCL_CRIT_ENTER();
    lused = &vcl_data.proto_data.local_used;

    for (tmp_vce = *lused; tmp_vce != NULL; tmp_vce = tmp_vce->next) {
        if (first == TRUE) {
            break;
        } else {
            if (use_next) {
                break;
            }
            found_proto = FALSE;
            if (tmp_vce->conf.proto_encap_type == proto_vce->proto_encap_type) {
                if ((tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_ETH2) &&
                    (proto_vce->proto.eth2_proto.eth_type == tmp_vce->conf.proto.eth2_proto.eth_type)) {
                    found_proto = TRUE;
                } else if (tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP) {
                    if (!memcmp(proto_vce->proto.llc_snap_proto.oui, tmp_vce->conf.proto.llc_snap_proto.oui, OUI_SIZE)
                        && (proto_vce->proto.llc_snap_proto.pid == tmp_vce->conf.proto.llc_snap_proto.pid)) {
                        found_proto = TRUE;
                    }
                } else if (tmp_vce->conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER) {
                    if ((proto_vce->proto.llc_other_proto.dsap == tmp_vce->conf.proto.llc_other_proto.dsap)
                        && (proto_vce->proto.llc_other_proto.ssap == tmp_vce->conf.proto.llc_other_proto.ssap)) {
                        found_proto = TRUE;
                    }
                }
            }
            if (found_proto) {
                if (next) {
                    use_next = TRUE;
                } else {
                    break;
                }
            }
        }
    }

    if (tmp_vce != NULL) {
        T_D("Found VCE with id: %u", tmp_vce->conf.id);
        *proto_vce = tmp_vce->conf;
    }
    VCL_CRIT_EXIT();
    return ((tmp_vce != NULL) ? (mesa_rc)VTSS_RC_OK : VCL_ERROR_ENTRY_NOT_FOUND);
}

static void vcl_snap_appl2mesa(mesa_vce_frame_snap_t &v_mesa, const vtss_appl_vcl_proto_llc_snap_t &v_appl)
{
    /* Copy OUI field */
    v_mesa.data.value[0] = v_appl.oui[0];
    v_mesa.data.value[1] = v_appl.oui[1];
    v_mesa.data.value[2] = v_appl.oui[2];
    v_mesa.data.mask[0] = 0xFF;
    v_mesa.data.mask[1] = 0xFF;
    v_mesa.data.mask[2] = 0xFF;
    /* Copy PID field */
    v_mesa.data.value[3] = (v_appl.pid & 0xFF00) >> 8;
    v_mesa.data.value[4] = (v_appl.pid & 0xFF);
    v_mesa.data.mask[3] = 0xFF;
    v_mesa.data.mask[4] = 0xFF;
    v_mesa.data.mask[5] = 0x00;
}

static void vcl_etype_appl2mesa(mesa_vce_frame_etype_t &v_mesa, const vtss_appl_vcl_proto_eth2_t &v_appl)
{
    v_mesa.etype.value[0] = (v_appl.eth_type & 0xFF00) >> 8;
    v_mesa.etype.value[1] = v_appl.eth_type & 0xFF;
    v_mesa.etype.mask[0]  = 0xFF;
    v_mesa.etype.mask[1]  = 0xFF;
    memset(v_mesa.data.value, 0, sizeof(v_mesa.data.value));
    memset(v_mesa.data.mask, 0, sizeof(v_mesa.data.value));
    v_mesa.mel.value  = 0;
    v_mesa.mel.mask  = 0;
}

static void vcl_llc_appl2mesa(mesa_vce_frame_llc_t &v_mesa, const vtss_appl_vcl_proto_llc_other_t &v_appl)
{
    /* Copy DSAP and SSAP fields */
    v_mesa.data.value[0] = v_appl.dsap;
    v_mesa.data.value[1] = v_appl.ssap;
    v_mesa.data.mask[0] = 0xFF;
    v_mesa.data.mask[1] = 0xFF;
}

void dump_vce(const char *file, int line, const mesa_vce_t &vce)
{
    printf("%s called from %s/%d\n", __FUNCTION__, file, line);
    printf("vce.id: %d\n", vce.id);
    printf("vce.key.port_list:");
    for (auto i = 0; i < vce.key.port_list.array_size(); ++i) {
        printf(" %d", vce.key.port_list._private[i]);
    }
    printf("\n");
    printf("vce.key.mac.dmac_mc: %d \n", vce.key.mac.dmac_mc);
    printf("vce.key.mac.dmac_bc: %d \n", vce.key.mac.dmac_bc);
    printf("vce.key.mac.dmac: %02x:%02x:%02x:%02x:%02x:%02x/%02x:%02x:%02x:%02x:%02x:%02x \n",
           vce.key.mac.dmac.value[0], vce.key.mac.dmac.value[1], vce.key.mac.dmac.value[2],
           vce.key.mac.dmac.value[3], vce.key.mac.dmac.value[4], vce.key.mac.dmac.value[5],
           vce.key.mac.dmac.mask[0], vce.key.mac.dmac.mask[1], vce.key.mac.dmac.mask[2],
           vce.key.mac.dmac.mask[3], vce.key.mac.dmac.mask[4], vce.key.mac.dmac.mask[5] );
    printf("vce.key.mac.smac: %02x:%02x:%02x:%02x:%02x:%02x/%02x:%02x:%02x:%02x:%02x:%02x \n",
           vce.key.mac.smac.value[0], vce.key.mac.smac.value[1], vce.key.mac.smac.value[2],
           vce.key.mac.smac.value[3], vce.key.mac.smac.value[4], vce.key.mac.smac.value[5],
           vce.key.mac.smac.mask[0], vce.key.mac.smac.mask[1], vce.key.mac.smac.mask[2],
           vce.key.mac.smac.mask[3], vce.key.mac.smac.mask[4], vce.key.mac.smac.mask[5] );
    printf("vce.key.tag.vid %04x/%04x\n", vce.key.tag.vid.value, vce.key.tag.vid.mask);
    printf("vce.key.tag.pcp %02x/%02x\n", vce.key.tag.pcp.value, vce.key.tag.pcp.mask);
    printf("vce.key.tag.dei %d\n", vce.key.tag.dei);
    printf("vce.key.tag.tagged %d\n", vce.key.tag.tagged);
    printf("vce.key.tag.s_tag %d\n", vce.key.tag.s_tag);
    printf("vce.key.inner_tag.vid %04x/%04x\n", vce.key.inner_tag.vid.value, vce.key.inner_tag.vid.mask);
    printf("vce.key.inner_tag.pcp %02x/%02x\n", vce.key.inner_tag.pcp.value, vce.key.inner_tag.pcp.mask);
    printf("vce.key.inner_tag.dei %d\n", vce.key.inner_tag.dei);
    printf("vce.key.inner_tag.tagged %d\n", vce.key.inner_tag.tagged);
    printf("vce.key.inner_tag.s_tag %d\n", vce.key.inner_tag.s_tag);
    printf("vce.key.type %d\n", vce.key.type);
    switch (vce.key.type) {
    case MESA_VCE_TYPE_ANY:
        printf("vce.key.frame\n");
        break;
    case MESA_VCE_TYPE_ETYPE:   // Ethernet Type
        printf("vce.key.frame.etype\n");
        break;
    case MESA_VCE_TYPE_LLC:     // LLC
        printf("vce.key.frame.llc\n");
        break;
    case MESA_VCE_TYPE_SNAP:    // SNAP
        printf("vce.key.frame.snap\n");
        break;
    case MESA_VCE_TYPE_IPV4:    // IPv4
        printf("vce.key.frame.ipv4\n");
        break;
    case MESA_VCE_TYPE_IPV6:     // IPv6
        printf("vce.key.frame.ipv6\n");
        break;
    default:
        printf("vce.key.type invalid\n");
    }
    printf("vce.action.vid: %d\n", vce.action.vid);
    printf("vce.action.policy_no: %d\n", vce.action.policy_no);
    printf("vce.action.pop_enable: %d\n", vce.action.pop_enable);
    printf("vce.action.pop_cnt: %d\n", vce.action.pop_cnt);
    printf("vce.action.map_sel: %d\n", vce.action.map_sel);
    printf("vce.action.map_id: %d\n", vce.action.map_id);
    printf("vce.action.flow_id: %d\n", vce.action.flow_id);
    printf("vce.action.oam_detect: %d\n", vce.action.oam_detect);
}

static mesa_rc vcl_proto_vce_switchapi_add(vcl_proto_vce_conf_local_t *proto_vce)
{
    mesa_vce_t      vce;
    mesa_rc         rc = VTSS_RC_OK;
    mesa_vce_id_t   vce_id;
    port_iter_t     pit;
    mesa_vce_type_t proto_type = MESA_VCE_TYPE_ANY;

    /* Check for NULL pointer */
    if (proto_vce == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    vce_id = proto_vce->id;
    /* Check for valid VCE ID */
    if (vce_id >= MESA_VCL_ID_END) {
        return VCL_ERROR_VCE_ID_EXCEEDED;
    }

    /* Get protocol type for initialisation */
    if (proto_vce->proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_ETH2) {
        if (proto_vce->proto.eth2_proto.eth_type == ETHERTYPE_IP) {
            proto_type = MESA_VCE_TYPE_IPV4;
        } else if (proto_vce->proto.eth2_proto.eth_type == ETHERTYPE_IP6) {
            proto_type = MESA_VCE_TYPE_IPV6;
        } else {
            proto_type = MESA_VCE_TYPE_ETYPE;
        }
    } else if (proto_vce->proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP) {
        proto_type = MESA_VCE_TYPE_SNAP;
    } else if (proto_vce->proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER) {
        proto_type = MESA_VCE_TYPE_LLC;
    }

    if ((rc = mesa_vce_init(NULL, proto_type,  &vce)) != VTSS_RC_OK) {
        return rc;
    }

    T_N("VCE is valid, generating switch API VCE");
    /* Prepare key */
    (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        vce.key.port_list[pit.iport] = VTSS_PORT_BF_GET(proto_vce->ports, pit.iport);
    }
    if (proto_vce->proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_ETH2) {
        if (vce.key.type == MESA_VCE_TYPE_ETYPE) {
            vcl_etype_appl2mesa(vce.key.frame.etype, proto_vce->proto.eth2_proto);
        }
    } else if (proto_vce->proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP) {
        vcl_snap_appl2mesa(vce.key.frame.snap, proto_vce->proto.llc_snap_proto);
    } else if (proto_vce->proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER) {
        /* Copy DSAP and SSAP fields */
        vcl_llc_appl2mesa(vce.key.frame.llc, proto_vce->proto.llc_other_proto);
    }

    /* Allow only for untagged Frames */
    vce.key.tag.tagged = MESA_VCAP_BIT_0;

    /* Prepare action - Only action is to classify to VLAN specified */
    vce.action.vid = proto_vce->vid;
    VCL_CRIT_ENTER();
    vce.action.policy_no = vcl_debug_policy_no;
    VCL_CRIT_EXIT();

    /* Populate VCE ID: First 16 bits (15:0) -> real VCE_ID; next 4-bits (19:16) -> VCL Type (MAC, Subnet or Protocol) */
    vce.id = ((vce_id & 0xFFFF) | ((VCL_TYPE_PROTO & 0xF) << 16));
    T_N("Generated VCE with key: %u", vce.id);
    /* Call the switch API for setting the configuration in ASIC. Protocol-based VLAN has the least priority.
       Hence add any Protocol-based VLAN entries at the end of the VCE list */
    //dump_vce(__FILE__, __LINE__, vce);
    if ((rc = mesa_vce_add(NULL, MESA_VCE_ID_LAST, &vce)) != VTSS_RC_OK) {
        return rc;
    } else {
        if (VTSS_RC_OK != vcl_register_vce(VCL_USR_DEFAULT, vce.id, MESA_VCE_ID_LAST)) {
            T_D("Could not register vce %d before %d", vce.id, MESA_VCE_ID_LAST);
        }
        T_D("VCE with key: %u was added to the switch API, to the end of the VCE list", vce.id);
    }

    /* Add VCE rule for priority-tagged frames - All fields remain the same except for tagged and vid */
    vce.key.tag.tagged = MESA_VCAP_BIT_1;
    vce.key.tag.vid.value = 0x0;
    vce.key.tag.vid.mask = 0xFFFF;
    /* Priority-tagged VCE entry is identified by setting the 20th bit of vce_id to 1*/
    vce.id = ((vce_id & 0xFFFF) | ((VCL_TYPE_PROTO & 0xF) << 16) | (0x1 << 20));
    //dump_vce(__FILE__, __LINE__, vce);
    if ((rc = mesa_vce_add(NULL, MESA_VCE_ID_LAST, &vce)) != VTSS_RC_OK) {
        return rc;
    } else {
        if (VTSS_RC_OK != vcl_register_vce(VCL_USR_DEFAULT, vce.id, MESA_VCE_ID_LAST)) {
            T_D("Could not register vce %d before %d", vce.id, MESA_VCE_ID_LAST);
        }
        T_D("VCE with key: %u was added to the switch API, to the end of the VCE list (priority-tagged)", vce.id);
    }
    return VTSS_RC_OK;
}

static mesa_rc vcl_msg_proto_vce_set(vtss_isid_t isid)
{
    vcl_msg_proto_vce_set_t     *msg;
    vcl_proto_vce_conf_global_t entry;
    u32                         cnt;
    BOOL                        found_sid, first = TRUE, next = FALSE;
    mesa_port_no_t              port;
    switch_iter_t               sit;

    T_D("Creating VLC msg for switch with isid: %d", isid);

    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        /* Initiate counter to count number of messages to be sent to sit.isid */
        cnt = 0;
        msg = (vcl_msg_proto_vce_set_t *)vcl_msg_alloc(1);
        msg->msg_id = VCL_MSG_ID_PROTO_VCE_SET;
        /* Loop through all the entries in the db */
        while (vcl_proto_vce_global_get(&entry, first, next) == VTSS_RC_OK) {
            found_sid = FALSE;
            for (port = 0; port < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port++) {
                VTSS_PORT_BF_SET(msg->conf[cnt].ports, port, VTSS_PORT_BF_GET(entry.ports[sit.isid - VTSS_ISID_START], port));
                if (VTSS_PORT_BF_GET(entry.ports[sit.isid - VTSS_ISID_START], port)) {
                    found_sid = TRUE;
                }
            }
            if (found_sid == FALSE) {
                first = FALSE;
                next = TRUE;
                continue;
            }
            msg->conf[cnt].id = entry.id;
            msg->conf[cnt].proto_encap_type = entry.proto_encap_type;
            msg->conf[cnt].proto = entry.proto;
            msg->conf[cnt].vid = entry.vid;
            cnt++;
            first = FALSE;
            next = TRUE;
        }
        msg->count = cnt;
        /* The below function also frees the msg after tx */
        vcl_msg_tx(msg, isid, sizeof(*msg));
    }
    T_D("Created VCL msg for switch with isid: %d", isid);
    return VTSS_RC_OK;
}

static mesa_rc vcl_msg_proto_vce_add_del(vtss_isid_t isid, vcl_proto_vce_conf_local_t *proto_vce, BOOL add)
{
    vcl_msg_proto_vce_t *msg;
    switch_iter_t       sit;

    T_D("Creating VLC msg for switch with isid: %d", isid);

    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID);
    /* Allocate a message with a ref-count corresponding to the number of times switch_iter_getnext() will return TRUE. */
    if ((msg = (vcl_msg_proto_vce_t *)vcl_msg_alloc(sit.remaining)) != NULL) {
        if (add) {
            msg->msg_id = VCL_MSG_ID_PROTO_VCE_ADD;
        } else {
            msg->msg_id = VCL_MSG_ID_PROTO_VCE_DEL;
        }
        VCL_CRIT_ENTER();
        memcpy(&msg->conf, proto_vce, sizeof(msg->conf));
        VCL_CRIT_EXIT();
        while (switch_iter_getnext(&sit)) {
            vcl_msg_tx(msg, sit.isid, sizeof(*msg));
        }
    } else {
        return VCL_ERROR_MSG_CREATION_FAIL;
    }
    T_D("Created VCL msg for switch with isid: %d", isid);
    return VTSS_RC_OK;
}

static void vcl_proto_mgmtl2vceg_conf(vtss_isid_t isid, vcl_proto_mgmt_group_conf_entry_local_t *proto_vce, vcl_proto_group_conf_entry_t *conf)
{
    switch_iter_t sit;
    port_iter_t   pit;

    memset(conf, 0, sizeof(*conf));

    memcpy(conf->name, proto_vce->name, MAX_GROUP_NAME_LEN);
    conf->vid = proto_vce->vid;

    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            conf->ports[sit.isid - VTSS_ISID_START][pit.iport / 8] |= proto_vce->ports[pit.iport] << (pit.iport % 8);
        }
    }

}

static void vcl_proto_vceg2mgmtg_conf(vtss_isid_t isid, vcl_proto_group_conf_entry_t *proto_vce, vcl_proto_mgmt_group_conf_entry_global_t *conf)
{
    switch_iter_t sit;
    port_iter_t   pit;

    vtss_clear(*conf);

    memcpy(conf->name, proto_vce->name, MAX_GROUP_NAME_LEN);
    conf->vid = proto_vce->vid;

    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            conf->ports[sit.isid - VTSS_ISID_START][pit.iport] = VTSS_PORT_BF_GET(proto_vce->ports[sit.isid - VTSS_ISID_START], pit.iport);
        }
    }
}

static void vcl_proto_vcel2mgmtprotol_conf(vtss_isid_t isid, vcl_proto_vce_conf_local_t *proto_vce, vcl_proto_mgmt_proto_conf_local_t *conf)
{
    port_iter_t pit;

    vtss_clear(*conf);

    conf->proto_encap_type = proto_vce->proto_encap_type;
    conf->proto = proto_vce->proto;
    conf->vid = proto_vce->vid;

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        conf->ports[pit.iport] = VTSS_PORT_BF_GET(proto_vce->ports, pit.iport);
    }
}

mesa_rc vcl_proto_mgmt_proto_add(vcl_proto_mgmt_group_conf_proto_t *group_conf)
{
    vcl_proto_group_conf_proto_t group_proto;
    vcl_proto_group_conf_entry_t group_entry = {0};
    vcl_proto_vce_conf_global_t  entry = {0};
    vcl_proto_vce_conf_local_t   conf;
    mesa_rc                      rc = VTSS_RC_OK;
    switch_iter_t                sit;
    BOOL                         first = FALSE, next = FALSE;
    u32                          isid, i;

    if (!msg_switch_is_primary()) {
        T_W("Switch is not the primary switch - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (group_conf == NULL) {
        T_E("Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    //T_D("MGMT API adding Protocol %s to Group %s",
    //    vcl_proto_mgmt_protocol2string(group_conf->proto_encap_type, group_conf->proto), group_conf->name);

    memcpy(group_proto.name, group_conf->name, MAX_GROUP_NAME_LEN);
    group_proto.proto_encap_type = group_conf->proto_encap_type;
    group_proto.proto = group_conf->proto;

    memcpy(group_entry.name, group_conf->name, MAX_GROUP_NAME_LEN);

    if ((rc = vcl_proto_group_proto_add(&group_proto)) != VTSS_RC_OK) {
        return rc;
    } else {
        T_D("MGMT API added the above Protocol to Group mapping");
        if ((rc = vcl_proto_group_entry_get(&group_entry, first, next)) == VTSS_RC_OK) {
            T_D("MGMT API adding VCE with the above protocol and VID %u", group_entry.vid);
            entry.proto_encap_type = group_conf->proto_encap_type;
            entry.proto = group_conf->proto;
            entry.vid = group_entry.vid;
            for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
                for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
                    entry.ports[isid - VTSS_ISID_START][i] = group_entry.ports[isid - VTSS_ISID_START][i];
                }
            }
            if ((rc = vcl_proto_vce_global_add(&entry)) != VTSS_RC_OK) {
                return rc;
            } else {
                conf.id = entry.id;
                conf.proto_encap_type = entry.proto_encap_type;
                conf.proto = entry.proto;
                conf.vid = entry.vid;
                (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
                while (switch_iter_getnext(&sit)) {
                    vcl_ports_global2local(entry.ports, conf.ports, sit.isid);
                    if ((rc = vcl_msg_proto_vce_add_del(sit.isid, &conf, TRUE)) != VTSS_RC_OK) {
                        return rc;
                    }
                }
            }
            T_D("MGMT API added the above VCE");
        }
    }
    T_D("MGMT API added the above Protocol (and all respective VCEs)");
    return VTSS_RC_OK;
}

mesa_rc vcl_proto_mgmt_proto_del(vcl_proto_mgmt_group_conf_proto_t *group_conf)
{
    vcl_proto_group_conf_proto_t group_proto;
    vcl_proto_group_conf_entry_t group_entry = {0};
    vcl_proto_vce_conf_global_t  entry;
    vcl_proto_vce_conf_local_t   conf;
    mesa_rc                      rc = VTSS_RC_OK;
    switch_iter_t                sit;
    BOOL                         first = FALSE, next = FALSE;
    u32                          isid, i;

    if (!msg_switch_is_primary()) {
        T_W("Switch is not the primary switch - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (group_conf == NULL) {
        T_E("Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    T_D("MGMT API deleting Protocol %s from Protocol to Group list",
        vcl_proto_mgmt_protocol2string(group_conf->proto_encap_type, group_conf->proto));

    group_proto.proto_encap_type = group_conf->proto_encap_type;
    group_proto.proto = group_conf->proto;

    if ((rc = vcl_proto_group_proto_del(&group_proto)) != VTSS_RC_OK) {
        return rc;
    } else {
        memcpy(group_entry.name, group_proto.name, MAX_GROUP_NAME_LEN);
        T_D("MGMT API deleted the above Protocol to Group mapping");
        if ((rc = vcl_proto_group_entry_get(&group_entry, first, next)) == VTSS_RC_OK) {
            T_D("MGMT API deleting VCE with the above protocol and VID %u", group_entry.vid);
            entry.proto_encap_type = group_conf->proto_encap_type;
            entry.proto = group_conf->proto;
            entry.vid = group_entry.vid;
            memset(entry.ports, 0, sizeof(entry.ports));
            for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
                for (i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
                    VTSS_PORT_BF_SET(entry.ports[isid - VTSS_ISID_START], i, 1);
                }
            }
            if ((rc = vcl_proto_vce_global_del(&entry)) != VTSS_RC_OK) {
                return rc;
            } else {
                conf.id = entry.id;
                conf.proto_encap_type = entry.proto_encap_type;
                conf.proto = entry.proto;
                conf.vid = entry.vid;
                (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
                while (switch_iter_getnext(&sit)) {
                    if ((rc = vcl_msg_proto_vce_add_del(sit.isid, &conf, FALSE)) != VTSS_RC_OK) {
                        return rc;
                    }
                }
            }
            T_D("MGMT API deleted the above VCE");
        }
    }
    T_D("MGMT API deleted the above Protocol (and all respective VCEs)");
    return VTSS_RC_OK;
}

mesa_rc vcl_proto_mgmt_proto_get(vcl_proto_mgmt_group_conf_proto_t *group_conf, BOOL first, BOOL next)
{
    vcl_proto_group_conf_proto_t entry;
    mesa_rc                      rc = VTSS_RC_OK;

    if (!msg_switch_is_primary()) {
        T_W("Switch is not the primary switch - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (group_conf == NULL) {
        T_E("Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first) {
        T_D("MGMT API fetching the first protocol from the Group(Protocol) list");
    } else {
        T_D("MGMT API fetching group from the Group(Protocol) list with: Protocol %s, first = %d, next = %d",
            vcl_proto_mgmt_protocol2string(group_conf->proto_encap_type, group_conf->proto), first, next);
    }

    memset(&entry, 0, sizeof(entry));
    memcpy(entry.name, group_conf->name, MAX_GROUP_NAME_LEN);
    entry.proto_encap_type = group_conf->proto_encap_type;
    entry.proto = group_conf->proto;
    if ((rc = vcl_proto_group_proto_get(&entry, first, next)) != VTSS_RC_OK) {
        return rc;
    } else {
        memcpy(group_conf->name, entry.name, MAX_GROUP_NAME_LEN);
        group_conf->proto_encap_type = entry.proto_encap_type;
        group_conf->proto = entry.proto;
    }
    T_D("MGMT API found the above protocol");
    return rc;
}

mesa_rc vcl_proto_mgmt_proto_itr(vtss_appl_vcl_proto_t *enc, BOOL first)
{
    vcl_proto_group_proto_t **lused, *tmp_group = NULL;
    vtss_appl_vcl_proto_t   enc2;
    u8                      res;

    if (!msg_switch_is_primary()) {
        T_WG(TRACE_GRP_MIB, "Switch is not the primary switch - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (enc == NULL) {
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first) {
        T_DG(TRACE_GRP_MIB, "Iterator fetching the first Protocol to Group entry from the list");
    } else {
        T_DG(TRACE_GRP_MIB, "Iterator fetching the Protocol to Group entry after the one with: Protocol %s",
             vcl_proto_mgmt_protocol2string(enc->proto_encap_type, enc->proto));
    }

    VCL_CRIT_ENTER();
    lused = &vcl_data.proto_data.group_proto_used;
    for (tmp_group = *lused; tmp_group != NULL; tmp_group = tmp_group->next) {
        if (first == TRUE) {
            break;
        } else {
            enc2.proto_encap_type = tmp_group->conf.proto_encap_type;
            enc2.proto = tmp_group->conf.proto;
            res = vcl_proto_encap_cmp(enc, &enc2);
            if (res == 0) {
                tmp_group = tmp_group->next;
                break;
            } else if (res == 1) {
                break;
            } else if (res == 2) {
                continue;
            }
        }
    }

    if (tmp_group != NULL) {
        T_DG(TRACE_GRP_MIB, "Found group named: %s", tmp_group->conf.name);
        enc->proto_encap_type = tmp_group->conf.proto_encap_type;
        enc->proto = tmp_group->conf.proto;
    } else {
        T_DG(TRACE_GRP_MIB, "There is no Protocol to Group entry after the provided one");
    }
    VCL_CRIT_EXIT();
    return (tmp_group == NULL ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK);
}

mesa_rc vcl_proto_mgmt_conf_add(vtss_isid_t isid_add, vcl_proto_mgmt_group_conf_entry_local_t *proto_vce)
{
    vcl_proto_group_conf_proto_t group_proto;
    vcl_proto_group_conf_entry_t group_entry;
    vcl_proto_vce_conf_global_t  entry = {0};
    vcl_proto_vce_conf_local_t   conf;
    mesa_rc                      rc = VTSS_RC_OK;
    switch_iter_t                sit;
    BOOL                         first = TRUE, next = FALSE, found_group = FALSE;
    u32                          isid, i;

    if (!msg_switch_is_primary()) {
        T_W("Switch is not the primary switch - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (!(VTSS_ISID_LEGAL(isid_add) || (isid_add == VTSS_ISID_GLOBAL))) {
        T_E("Invalid ISID: %u - LEGAL one expected", isid_add);
        return VCL_ERROR_INVALID_ISID;
    }

    if (proto_vce == NULL) {
        T_E("Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if ((rc = vcl_proto_group_name_check(proto_vce->name)) != VTSS_RC_OK) {
        T_E("Invalid Group name: %s - LEGAL one expected", proto_vce->name);
        return rc;
    }

    if (proto_vce->vid < VTSS_APPL_VLAN_ID_MIN || proto_vce->vid > VTSS_APPL_VLAN_ID_MAX) {
        T_W("VLAN ID must be between 1 and 4095");
        return VCL_ERROR_INVALID_VLAN_ID;
    }

    T_D("MGMT API adding VCE with: Group name %s, VID %u, on switch %u",
        proto_vce->name, proto_vce->vid, isid_add);

    /**
    * Convert the vcl_mac_vlan_mgmt_entry_t to a vcl_mac_vlan_conf_entry_t.
    **/
    vcl_proto_mgmtl2vceg_conf(isid_add, proto_vce, &group_entry);
    memset(&group_proto, 0, sizeof(group_proto));
    memcpy(group_proto.name, group_entry.name, MAX_GROUP_NAME_LEN);

    if ((rc = vcl_proto_group_entry_add(&group_entry)) != VTSS_RC_OK) {
        return rc;
    } else {
        T_D("MGMT API added the above Group to VID mapping");
        while ((rc = vcl_proto_group_proto_name_get(&group_proto, first, next)) == VTSS_RC_OK) {
            T_D("MGMT API adding VCE with: Group name %s, Protocol %s and VID %u",
                group_entry.name, vcl_proto_mgmt_protocol2string(group_proto.proto_encap_type, group_proto.proto), group_entry.vid);
            if (found_group == FALSE) {
                found_group = TRUE;
                first = FALSE;
                next = TRUE;
            }
            entry.proto_encap_type = group_proto.proto_encap_type;
            entry.proto = group_proto.proto;
            entry.vid = group_entry.vid;
            for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
                for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
                    entry.ports[isid - VTSS_ISID_START][i] = group_entry.ports[isid - VTSS_ISID_START][i];
                }
            }
            if ((rc = vcl_proto_vce_global_add(&entry)) != VTSS_RC_OK) {
                return rc;
            } else {
                conf.id = entry.id;
                conf.proto_encap_type = entry.proto_encap_type;
                conf.proto = entry.proto;
                conf.vid = entry.vid;
                (void)switch_iter_init(&sit, isid_add, SWITCH_ITER_SORT_ORDER_ISID);
                while (switch_iter_getnext(&sit)) {
                    vcl_ports_global2local(entry.ports, conf.ports, sit.isid);
                    if ((rc = vcl_msg_proto_vce_add_del(sit.isid, &conf, TRUE)) != VTSS_RC_OK) {
                        return rc;
                    }
                }
            }
            T_D("MGMT API added the above VCE");
        }
    }
    T_D("MGMT API added the above Group (and all respective VCEs)");
    return VTSS_RC_OK;
}

mesa_rc vcl_proto_mgmt_conf_del(vtss_isid_t isid_del, vcl_proto_mgmt_group_conf_entry_local_t *proto_vce)
{
    vcl_proto_group_conf_proto_t group_proto;
    vcl_proto_group_conf_entry_t group_entry;
    vcl_proto_vce_conf_global_t  entry;
    vcl_proto_vce_conf_local_t   conf;
    mesa_rc                      rc = VTSS_RC_OK;
    switch_iter_t                sit;
    BOOL                         first = TRUE, next = FALSE, found_group = FALSE;
    u32                          i;

    if (!msg_switch_is_primary()) {
        T_W("Switch is not the primary switch - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (!(VTSS_ISID_LEGAL(isid_del))) {
        T_E("Invalid ISID: %u - LEGAL one expected", isid_del);
        return VCL_ERROR_INVALID_ISID;
    }

    if (proto_vce == NULL) {
        T_E("Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if ((rc = vcl_proto_group_name_check(proto_vce->name)) != VTSS_RC_OK) {
        T_E("Invalid Group name: %s - LEGAL one expected", proto_vce->name);
        return rc;
    }

    T_D("MGMT API deleting VCE with: Group name %s, VID %u, on switch %u",
        proto_vce->name, proto_vce->vid, isid_del);

    /**
    * Convert the vcl_mac_vlan_mgmt_entry_t to a vcl_mac_vlan_conf_entry_t.
    **/
    vcl_proto_mgmtl2vceg_conf(isid_del, proto_vce, &group_entry);

    memcpy(group_proto.name, proto_vce->name, MAX_GROUP_NAME_LEN);
    memset(group_entry.ports, 0, sizeof(group_entry.ports));
    for (i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
        VTSS_PORT_BF_SET(group_entry.ports[isid_del - VTSS_ISID_START], i, 1);
    }

    if ((rc = vcl_proto_group_entry_del(&group_entry)) != VTSS_RC_OK) {
        return rc;
    } else {
        T_D("MGMT API deleted the above Group to VID mapping");
        while ((rc = vcl_proto_group_proto_name_get(&group_proto, first, next)) == VTSS_RC_OK) {
            T_D("MGMT API deleting VCE with: Group name %s, Protocol %s and VID %u",
                group_entry.name, vcl_proto_mgmt_protocol2string(group_proto.proto_encap_type, group_proto.proto), group_entry.vid);
            if (found_group == FALSE) {
                found_group = TRUE;
                first = FALSE;
                next = TRUE;
            }
            entry.proto_encap_type = group_proto.proto_encap_type;
            entry.proto = group_proto.proto;
            entry.vid = group_entry.vid;
            memset(entry.ports, 0, sizeof(entry.ports));
            for (i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
                VTSS_PORT_BF_SET(entry.ports[isid_del - VTSS_ISID_START], i, 1);
            }
            if ((rc = vcl_proto_vce_global_del(&entry)) != VTSS_RC_OK) {
                return rc;
            } else {
                conf.id = entry.id;
                conf.proto_encap_type = entry.proto_encap_type;
                conf.proto = entry.proto;
                conf.vid = entry.vid;
                (void)switch_iter_init(&sit, isid_del, SWITCH_ITER_SORT_ORDER_ISID);
                while (switch_iter_getnext(&sit)) {
                    vcl_ports_global2local(entry.ports, conf.ports, sit.isid);
                    if ((rc = vcl_msg_proto_vce_add_del(sit.isid, &conf, FALSE)) != VTSS_RC_OK) {
                        return rc;
                    }
                }
            }
            T_D("MGMT API deleted the above VCE");
        }
    }
    T_D("MGMT API deleted the above Group (and all respective VCEs)");
    return VTSS_RC_OK;
}

mesa_rc vcl_proto_mgmt_conf_get(vtss_isid_t isid_get, vcl_proto_mgmt_group_conf_entry_global_t *proto_vce, BOOL first, BOOL next)
{
    vcl_proto_group_conf_entry_t group_entry;
    mesa_rc                      rc = VTSS_RC_OK;
    port_iter_t                  pit;
    BOOL                         first_l, next_l, found_sid = FALSE;

    if (!msg_switch_is_primary()) {
        T_W("Switch is not the primary switch - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (!VTSS_ISID_LEGAL(isid_get) && (isid_get != VTSS_ISID_GLOBAL)) {
        T_E("Invalid ISID: %u - LEGAL one expected", isid_get);
        return VCL_ERROR_INVALID_ISID;
    }

    if (proto_vce == NULL) {
        T_E("Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first == FALSE) {
        if ((rc = vcl_proto_group_name_check(proto_vce->name)) != VTSS_RC_OK) {
            T_E("Invalid Group name: %s - LEGAL one expected", proto_vce->name);
            return rc;
        }
    }

    if (first) {
        T_D("MGMT API fetching the first VCE from global list");
    } else {
        T_D("MGMT API fetching VCE from global list with: Group name %s, VID %d, first = %d, next = %d",
            proto_vce->name, proto_vce->vid, first, next);
    }

    vtss_clear(proto_vce->ports);
    memset(&group_entry, 0, sizeof(group_entry));

    memcpy(group_entry.name, proto_vce->name, MAX_GROUP_NAME_LEN);
    group_entry.vid = proto_vce->vid;
    first_l = first;
    next_l = next;
    while ((rc = vcl_proto_group_entry_get(&group_entry, first_l, next_l)) == VTSS_RC_OK) {
        T_D("Group name is %s, VID = %d", group_entry.name, group_entry.vid);
        vcl_proto_vceg2mgmtg_conf(isid_get, &group_entry, proto_vce);
        if (isid_get == VTSS_ISID_GLOBAL) {
            found_sid = TRUE;
        } else {
            (void)port_iter_init(&pit, NULL, isid_get, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (proto_vce->ports[isid_get - VTSS_ISID_START][pit.iport] == 1) {
                    found_sid = TRUE;
                    break;
                }
            }
        }
        if (found_sid == FALSE) {
            next_l = TRUE;
            first_l = FALSE;
            T_N("MGMT API found the above VCE in the global list, but rejected it since it is not present in the requested switch #%u",
                isid_get);
            continue;
        }
        T_D("MGMT API found the above VCE");
        break;
    }

    T_D("MGMT API Exit");
    return rc;
}

mesa_rc vcl_proto_mgmt_conf_local_proto_get(vcl_proto_mgmt_proto_conf_local_t *proto_vce, BOOL first, BOOL next)
{
    mesa_rc                    rc;
    vcl_proto_vce_conf_local_t entry;

    if (proto_vce == NULL) {
        T_E("Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first) {
        T_D("MGMT API fetching the first VCE from local list");
    } else {
        T_D("MGMT API fetching VCE from local list with: Protocol %s, VID %d, first = %d, next = %d",
            vcl_proto_mgmt_protocol2string(proto_vce->proto_encap_type, proto_vce->proto), proto_vce->vid, first, next);
    }

    memset(&entry, 0, sizeof(entry));
    entry.proto_encap_type = proto_vce->proto_encap_type;
    entry.proto = proto_vce->proto;
    entry.vid = proto_vce->vid;
    if ((rc = vcl_proto_vce_local_get(&entry, first, next)) != VTSS_RC_OK) {
        return rc;
    } else {
        vcl_proto_vcel2mgmtprotol_conf(VTSS_ISID_LOCAL, &entry, proto_vce);
    }
    T_D("MGMT API found the above VCE");
    return VTSS_RC_OK;
}

mesa_rc vcl_proto_mgmt_conf_itr(vtss_appl_vcl_proto_group_conf_proto_t *group, BOOL first)
{
    vcl_proto_group_entry_t **lused, *tmp_group = NULL;
    int                     res;

    if (!msg_switch_is_primary()) {
        T_WG(TRACE_GRP_MIB, "Switch is not the primary switch - therefore cannot process the request");
        return VCL_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (group == NULL) {
        T_EG(TRACE_GRP_MIB, "Request provided an empty entry - NULL pointer");
        return VCL_ERROR_EMPTY_ENTRY;
    }

    if (first) {
        T_DG(TRACE_GRP_MIB, "Iterator fetching the first Group to VID entry from the list");
    } else {
        T_DG(TRACE_GRP_MIB, "Iterator fetching the Group to VID entry after the one with: Group %s", group->name);
    }

    VCL_CRIT_ENTER();
    lused = &vcl_data.proto_data.group_entry_used;
    for (tmp_group = *lused; tmp_group != NULL; tmp_group = tmp_group->next) {
        if (first == TRUE) {
            break;
        } else {
            res = strcmp((char *)group->name, (char *)tmp_group->conf.name);
            if (res == 0) {
                tmp_group = tmp_group->next;
                break;
            } else if (res < 0) {
                break;
            } else if (res > 0) {
                continue;
            }
        }
    }

    if (tmp_group != NULL) {
        T_DG(TRACE_GRP_MIB, "Found entry with Group: %s, VID %u", tmp_group->conf.name, tmp_group->conf.vid);
        memcpy(group->name, tmp_group->conf.name, sizeof(group->name));
    } else {
        T_DG(TRACE_GRP_MIB, "There is no Group to VID entry after the provided one");
    }
    VCL_CRIT_EXIT();
    return (tmp_group == NULL ? (mesa_rc)VCL_ERROR_ENTRY_NOT_FOUND : VTSS_RC_OK);
}

static void vcl_proto_default_set(void)
{
    vcl_proto_vce_conf_local_t entry;
    mesa_vce_id_t              vce_id;
    mesa_rc                    rc;

    /* Delete all the existing entries */
    while ((vcl_proto_vce_local_get(&entry, TRUE, FALSE)) == VTSS_RC_OK) {
        if (vcl_proto_vce_local_del(&entry) == VTSS_RC_OK) {
            vce_id = entry.id;
            vce_id = ((vce_id & 0xFFFF) | ((VCL_TYPE_PROTO & 0xF) << 16));
            /* Call the switch API */
            if (VTSS_RC_OK != vcl_unregister_vce(VCL_USR_DEFAULT, vce_id)) {
                T_D("Failure while unregistering vce %d", vce_id);
            }
            if ((rc = mesa_vce_del(NULL, vce_id)) != VTSS_RC_OK) {
                T_D("Failure while deleting old Protocol entries (rc = %s)", error_txt(rc));
            } else {
                vce_id = (vce_id | (0x1 << 20));
                if (VTSS_RC_OK != vcl_unregister_vce(VCL_USR_DEFAULT, vce_id)) {
                    T_D("Failure while unregistering vce %d", vce_id);
                }
                if ((rc = mesa_vce_del(NULL, vce_id)) != VTSS_RC_OK) {
                    T_D("Failure while deleting old Protocol entries (priority tagged) (rc = %s)", error_txt(rc));
                }
            }
        }
    }

    memset(&vcl_data.proto_data, 0, sizeof(vcl_proto_data_t));
    vcl_proto_group_proto_default_set();
    vcl_proto_group_entry_default_set();
    vcl_proto_global_default_set();
    vcl_proto_local_default_set();
}

/* VCL Message Receive Handler */
static BOOL vcl_msg_rx(void *contxt, const void *const rx_msg, const size_t len,
                       const vtss_module_id_t modid, const u32 isid)
{
    vcl_msg_id_t msg_id = *(vcl_msg_id_t *)rx_msg;
    mesa_rc rc = VTSS_RC_OK;

    T_D("VCL module received a message with id %s and length %zd", vcl_msg_id_txt(msg_id), len);

    switch (msg_id) {
    case VCL_MSG_ID_MAC_VCE_SET: {
        u32                      cnt;
        BOOL                     first = TRUE, next = FALSE;
        vcl_msg_mac_vce_set_t    *msg;
        vcl_mac_vce_conf_local_t entry;
        mesa_vce_id_t            vce_id;

        /* Delete all the existing entries */
        while ((vcl_mac_vce_local_get(&entry, first, next)) == VTSS_RC_OK) {
            if (vcl_mac_vce_local_del(&entry) == VTSS_RC_OK) {
                vce_id = entry.id;
                vce_id = ((vce_id & 0xFFFF) | ((VCL_TYPE_MAC & 0xF) << 16));
                /* Call the switch API */
                if (VTSS_RC_OK != vcl_unregister_vce(VCL_USR_DEFAULT, vce_id)) {
                    T_D("Failure while unregistering vce %d", vce_id);
                }
                if ((rc = mesa_vce_del(NULL, vce_id)) != VTSS_RC_OK) {
                    T_D("Failure while deleting old MAC entries (rc = %s)", error_txt(rc));
                    return FALSE;
                }
            }
        }

        msg = (vcl_msg_mac_vce_set_t *)rx_msg;
        /* Add all the new entries */
        for (cnt = 0; cnt < msg->count; cnt++) {
            entry.id = msg->conf[cnt].id;
            entry.smac = msg->conf[cnt].smac;
            entry.vid = msg->conf[cnt].vid;
            memcpy(entry.ports, msg->conf[cnt].ports, VTSS_PORT_BF_SIZE);
            if ((rc = vcl_mac_vce_local_add(&entry)) != VTSS_RC_OK) {
                T_D("Failure while adding MAC entries to the local list (rc = %s)", error_txt(rc));
                return FALSE;
            } else {
                /* Call the switch API */
                if ((rc = vcl_mac_vce_switchapi_add(&entry)) != VTSS_RC_OK) {
                    T_D("Failure while adding MAC entries to the switch API (rc = %s)", error_txt(rc));
                    (void)vcl_mac_vce_local_del(&entry);
                    T_I("VCE could not be added on the switch API - therefore entry was deleted from the local list as well");
                    return FALSE;
                } else {
                    T_D("Added the above VCE");
                }
            }
        }
        break;
    }
    case VCL_MSG_ID_MAC_VCE_ADD: {
        vcl_mac_vce_conf_local_t entry;
        vcl_msg_mac_vce_t        *msg;

        msg = (vcl_msg_mac_vce_t *)rx_msg;
        entry.id = msg->conf.id;
        entry.smac = msg->conf.smac;
        entry.vid = msg->conf.vid;
        memcpy(entry.ports, msg->conf.ports, VTSS_PORT_BF_SIZE);
        if ((rc = vcl_mac_vce_local_add(&entry)) != VTSS_RC_OK) {
            T_D("Failure while adding new MAC entries to the local list (rc = %s)", error_txt(rc));
            return FALSE;
        } else {
            /* Call the switch API */
            if ((rc = vcl_mac_vce_switchapi_add(&entry)) != VTSS_RC_OK) {
                T_D("Failure while adding new MAC entries to the switch API (rc = %s)", error_txt(rc));
                (void)vcl_mac_vce_local_del(&entry);
                T_I("VCE could not be added on the switch API - therefore entry was deleted from the local list as well");
                return FALSE;
            } else {
                T_D("Added the above VCE");
            }
        }
        break;
    }
    case VCL_MSG_ID_MAC_VCE_DEL: {
        vcl_mac_vce_conf_local_t entry;
        vcl_msg_mac_vce_t        *msg;
        mesa_vce_id_t            vce_id;

        msg = (vcl_msg_mac_vce_t *)rx_msg;
        entry.id = msg->conf.id;
        entry.smac = msg->conf.smac;
        if ((rc = vcl_mac_vce_local_del(&entry)) != VTSS_RC_OK) {
            T_D("Failure while deleting MAC entries from the local list (rc = %s)", error_txt(rc));
            return FALSE;
        } else {
            vce_id = entry.id;
            vce_id = ((vce_id & 0xFFFF) | ((VCL_TYPE_MAC & 0xF) << 16));
            /* Call the switch API */
            if (VTSS_RC_OK != vcl_unregister_vce(VCL_USR_DEFAULT, vce_id)) {
                T_D("Failure while unregistering vce %d", vce_id);
            }
            if ((rc = mesa_vce_del(NULL, vce_id)) != VTSS_RC_OK) {
                T_D("Failure while deleting MAC entries from the switch API (rc = %s)", error_txt(rc));
                return FALSE;
            } else {
                T_D("Switch deleted the above VCE (key #%u)", vce_id);
            }
        }
        break;
    }
    case VCL_MSG_ID_PROTO_VCE_SET: {
        u32                        cnt;
        vcl_msg_proto_vce_set_t    *msg;
        vcl_proto_vce_conf_local_t entry;
        mesa_vce_id_t              vce_id;

        /* Delete all the existing entries */
        while ((vcl_proto_vce_local_get(&entry, TRUE, FALSE)) == VTSS_RC_OK) {
            if (vcl_proto_vce_local_del(&entry) == VTSS_RC_OK) {
                vce_id = entry.id;
                vce_id = ((vce_id & 0xFFFF) | ((VCL_TYPE_PROTO & 0xF) << 16));
                /* Call the switch API */
                if (VTSS_RC_OK != vcl_unregister_vce(VCL_USR_DEFAULT, vce_id)) {
                    T_D("Failure while unregistering vce %d", vce_id);
                }
                if ((rc = mesa_vce_del(NULL, vce_id)) != VTSS_RC_OK) {
                    T_D("Failure while deleting old Protocol entries (rc = %s)", error_txt(rc));
                    return FALSE;
                } else {
                    vce_id = (vce_id | (0x1 << 20));
                    if (VTSS_RC_OK != vcl_unregister_vce(VCL_USR_DEFAULT, vce_id)) {
                        T_D("Failure while unregistering vce %d", vce_id);
                    }
                    if ((rc = mesa_vce_del(NULL, vce_id)) != VTSS_RC_OK) {
                        T_D("Failure while deleting old Protocol entries (priority tagged) (rc = %s)", error_txt(rc));
                        return FALSE;
                    }
                }
            }
        }

        msg = (vcl_msg_proto_vce_set_t *)rx_msg;
        /* Add all the new entries */
        for (cnt = 0; cnt < msg->count; cnt++) {
            if ((rc = vcl_proto_vce_local_add(&msg->conf[cnt])) != VTSS_RC_OK) {
                T_D("Failure while adding Protocol entries to the local list (rc = %s)", error_txt(rc));
                return FALSE;
            } else {
                /* Call the switch API */
                if ((rc = vcl_proto_vce_switchapi_add(&msg->conf[cnt])) != VTSS_RC_OK) {
                    T_D("Failure while adding Protocol entries to the switch API (rc = %s)", error_txt(rc));
                    (void)vcl_proto_vce_local_del(&msg->conf[cnt]);
                    T_I("VCE could not be added on the switch API - therefore entry was deleted from the local list as well");
                    return FALSE;
                } else {
                    T_D("Added the above VCE");
                }
            }
        }
        break;
    }
    case VCL_MSG_ID_PROTO_VCE_ADD: {
        vcl_msg_proto_vce_t *msg;

        msg = (vcl_msg_proto_vce_t *)rx_msg;
        if ((rc = vcl_proto_vce_local_add(&msg->conf)) != VTSS_RC_OK) {
            T_D("Failure while adding new Protocol entries to the local list (rc = %s)", error_txt(rc));
            return FALSE;
        } else {
            /* Call the switch API */
            if ((rc = vcl_proto_vce_switchapi_add(&msg->conf)) != VTSS_RC_OK) {
                T_D("Failure while adding new Protocol entries to the switch API (rc = %s)", error_txt(rc));
                (void)vcl_proto_vce_local_del(&msg->conf);
                T_I("VCE could not be added on the switch API - therefore entry was deleted from the local list as well");
                return FALSE;
            } else {
                T_D("Added the above VCE");
            }
        }
        break;
    }
    case VCL_MSG_ID_PROTO_VCE_DEL: {
        vcl_msg_proto_vce_t *msg;
        mesa_vce_id_t       vce_id;

        msg = (vcl_msg_proto_vce_t *)rx_msg;
        if ((rc = vcl_proto_vce_local_del(&msg->conf)) != VTSS_RC_OK) {
            T_D("Failure while deleting Protocol entries from the local list (rc = %s)", error_txt(rc));
            return FALSE;
        } else {
            vce_id = msg->conf.id;
            vce_id = ((vce_id & 0xFFFF) | ((VCL_TYPE_PROTO & 0xF) << 16));
            /* Call the switch API */
            if (VTSS_RC_OK != vcl_unregister_vce(VCL_USR_DEFAULT, vce_id)) {
                T_D("Failure while unregistering vce %d", vce_id);
            }
            if ((rc = mesa_vce_del(NULL, vce_id)) != VTSS_RC_OK) {
                T_D("Failure while deleting Protocol entries from the switch API (rc = %s)", error_txt(rc));
                return FALSE;
            } else {
                T_N("Switch deleted the above VCE (key #%u) - plain version", vce_id);
                vce_id = (vce_id | (0x1 << 20));
                if (VTSS_RC_OK != vcl_unregister_vce(VCL_USR_DEFAULT, vce_id)) {
                    T_D("Failure while unregistering vce %d", vce_id);
                }
                if ((rc = mesa_vce_del(NULL, vce_id)) != VTSS_RC_OK) {
                    T_D("Failure while deleting Protocol entries from the switch API (priority tagged) (rc = %s)", error_txt(rc));
                    return FALSE;
                } else {
                    T_D("Switch deleted the above VCE (key #%u) - both versions", vce_id);
                }
            }
        }
        break;
    }
    case VCL_MSG_ID_IP_VCE_SET: {
        u32                     cnt;
        vcl_msg_ip_vce_set_t    *msg;
        vcl_ip_vce_conf_local_t entry;
        mesa_vce_id_t           vce_id, id_next = MESA_VCE_ID_LAST;

        /* Delete all the existing entries */
        while ((vcl_ip_vce_local_get(&entry, TRUE, FALSE)) == VTSS_RC_OK) {
            if (vcl_ip_vce_local_del(&entry) == VTSS_RC_OK) {
                vce_id = entry.id;
                vce_id = ((vce_id & 0xFFFF) | ((VCL_TYPE_IP & 0xF) << 16));
                /* Call the switch API */
                if (VTSS_RC_OK != vcl_unregister_vce(VCL_USR_DEFAULT, vce_id)) {
                    T_D("Failure while unregistering vce %d", vce_id);
                }
                if ((rc = mesa_vce_del(NULL, vce_id)) != VTSS_RC_OK) {
                    T_D("Failure while deleting Subnet entries (rc = %s)", error_txt(rc));
                    return FALSE;
                } else {
                    vce_id = (vce_id | (0x1 << 20));
                    if (VTSS_RC_OK != vcl_unregister_vce(VCL_USR_DEFAULT, vce_id)) {
                        T_D("Failure while unregistering vce %d", vce_id);
                    }
                    if ((rc = mesa_vce_del(NULL, vce_id)) != VTSS_RC_OK) {
                        T_D("Failure while deleting Subnet entries (priority tagged) (rc = %s)", error_txt(rc));
                        return FALSE;
                    }
                }
            }
        }

        msg = (vcl_msg_ip_vce_set_t *)rx_msg;
        /* Add all the new entries */
        for (cnt = 0; cnt < msg->count; cnt++) {
            if ((rc = vcl_ip_vce_local_add(&msg->conf[cnt], &id_next)) != VTSS_RC_OK) {
                T_D("Failure while adding Subnet entries to the local list (rc = %s)", error_txt(rc));
                return FALSE;
            } else {
                /* Call the switch API */
                if ((rc = vcl_ip_vce_switchapi_add(&msg->conf[cnt], id_next)) != VTSS_RC_OK) {
                    T_D("Failure while adding Subnet entries to the switch API (rc = %s)", error_txt(rc));
                    (void)vcl_ip_vce_local_del(&msg->conf[cnt]);
                    T_I("VCE could not be added on the switch API - therefore entry was deleted from the local list as well");
                    return FALSE;
                } else {
                    T_D("Added the above VCE");
                }
            }
        }
        break;
    }
    case VCL_MSG_ID_IP_VCE_ADD: {
        vcl_msg_ip_vce_t *msg;
        mesa_vce_id_t     id_next = MESA_VCE_ID_LAST;

        msg = (vcl_msg_ip_vce_t *)rx_msg;
        if ((rc = vcl_ip_vce_local_add(&msg->conf, &id_next)) != VTSS_RC_OK) {
            T_D("Failure while adding new Subnet entries to the local list (rc = %s)", error_txt(rc));
            return FALSE;
        } else {
            /* Call the switch API */
            if ((rc = vcl_ip_vce_switchapi_add(&msg->conf, id_next)) != VTSS_RC_OK) {
                T_D("Failure while adding new Subnet entries to the switch API (rc = %s)", error_txt(rc));
                (void)vcl_ip_vce_local_del(&msg->conf);
                T_I("VCE could not be added on the switch API - therefore entry was deleted from the local list as well");
                return FALSE;
            } else {
                T_D("Added the above VCE");
            }
        }
        break;
    }
    case VCL_MSG_ID_IP_VCE_DEL: {
        vcl_msg_ip_vce_t *msg;
        mesa_vce_id_t    vce_id;

        msg = (vcl_msg_ip_vce_t *)rx_msg;
        if ((rc = vcl_ip_vce_local_del(&msg->conf)) != VTSS_RC_OK) {
            T_D("Failure while deleting Subnet entries from the local list (rc = %s)", error_txt(rc));
            return FALSE;
        } else {
            vce_id = msg->conf.id;
            vce_id = ((vce_id & 0xFFFF) | ((VCL_TYPE_IP & 0xF) << 16));
            /* Call the switch API */
            if (VTSS_RC_OK != vcl_unregister_vce(VCL_USR_DEFAULT, vce_id)) {
                T_D("Failure while unregistering vce %d", vce_id);
            }
            if ((rc = mesa_vce_del(NULL, vce_id)) != VTSS_RC_OK) {
                T_D("Failure while deleting Subnet entries from the switch API (rc = %s)", error_txt(rc));
                return FALSE;
            } else {
                T_N("Switch deleted the above VCE (key #%u) - plain version", vce_id);
                vce_id = (vce_id | (0x1 << 20));
                if (VTSS_RC_OK != vcl_unregister_vce(VCL_USR_DEFAULT, vce_id)) {
                    T_D("Failure while unregistering vce %d", vce_id);
                }
                if ((rc = mesa_vce_del(NULL, vce_id)) != VTSS_RC_OK) {
                    T_D("Failure while deleting Subnet entries from the switch API (priority tagged) (rc = %s)", error_txt(rc));
                    return FALSE;
                } else {
                    T_D("Switch deleted the above VCE (key #%u) - both versions", vce_id);
                }
            }
        }
        break;
    }
    default: {
        T_W("Received message with unknown ID: %d", msg_id);
        break;
    }
    }
    return TRUE;
}

/* Register to the stack call back */
mesa_rc vcl_stack_register(void)
{
    msg_rx_filter_t filter;

    memset(&filter, 0, sizeof(filter));
    filter.cb = vcl_msg_rx;
    filter.modid = VTSS_MODULE_ID_VCL;
    return msg_rx_filter_register(&filter);
}

const char *vcl_error_txt(mesa_rc rc)
{
    const char *txt;

    switch (rc) {
    case VCL_ERROR_ENTRY_NOT_FOUND:
        txt = "VCL Error - The requested entry was not found in the switch";
        break;
    case VCL_ERROR_EMPTY_ENTRY:
        txt = "VCL Error - The entry provided to a VCL function was empty";
        break;
    case VCL_ERROR_ENTRY_DIFF_VID:
        txt = "VCL Error - entry exists with a different VLAN ID. Not possible to update the VLAN ID of the entry";
        break;
    case VCL_ERROR_MAC_TABLE_FULL:
        txt = "VCL Error - MAC VCE database is full";
        break;
    case VCL_ERROR_VCE_ID_EXCEEDED:
        txt = "VCL Error - VCE id exceeded the maximum limit";
        break;
    case VCL_ERROR_NOT_PRIMARY_SWITCH:
        txt = "VCL Error - Request can only be processed on the primary switch";
        break;
    case VCL_ERROR_INVALID_ISID:
        txt = "VCL Error - The provided isid was invalid";
        break;
    case VCL_ERROR_SYSTEM_MAC:
        txt = "VCL Error - Cannot use system mac address";
        break;
    case VCL_ERROR_MULTIBROAD_MAC:
        txt = "VCL Error - Cannot use multicast/broadcast mac address";
        break;
    case VCL_ERROR_MSG_CREATION_FAIL:
        txt = "VCL Error - Failed while creating vcl message";
        break;
    case VCL_ERROR_IP_TABLE_FULL:
        txt = "VCL Error - IP Subnet VCE database is full";
        break;
    case VCL_ERROR_INVALID_MASK_LENGTH:
        txt = "VCL Error - The provided subnet mask length was invalid (valid: 0 - 32)";
        break;
    case VCL_ERROR_INVALID_VLAN_ID:
        txt = "VCL Error - The provided VLAN ID was invalid (valid: 1 - 4095)";
        break;
    case VCL_ERROR_INVALID_GROUP_NAME:
        txt = "VCL Error - The provided Group name was invalid. Only characters and digits are allowed";
        break;
    case VCL_ERROR_PROTOCOL_ALREADY_CONF:
        txt = "VCL Error - The provided protocol is already mapped to a Group name";
        break;
    case VCL_ERROR_GROUP_PROTO_TABLE_FULL:
        txt = "VCL Error - Group database if full";
        break;
    case VCL_ERROR_INVALID_ENCAP_TYPE:
        txt = "VCL Error - The provided protocol encapsulation type was invalid";
        break;
    case VCL_ERROR_ENTRY_OVERLAPPING_PORTS:
        txt = "VCL Error - The provided port list has overlapping ports with a different Group to VID mapping";
        break;
    case VCL_ERROR_GROUP_ENTRY_TABLE_FULL:
        txt = "VCL Error - Protocol VCE database is full";
        break;
    case VCL_ERROR_PROTO_TABLE_FULL:
        txt = "VCL Error - Protocol VCE database is full";
        break;
    case VCL_ERROR_INVALID_PROTO_CNT:
        txt = "VCL Error - Select only one protocol encapsulation type (eth2, snap or llc) to delete a protocol to group mapping";
        break;
    case VCL_ERROR_NULL_GROUP_NAME:
        txt = "VCL Error - Failed to specify a Group name. Please provide one";
        break;
    case VCL_ERROR_INVALID_PID:
        txt = "VCL Error - Invalid PID. IF OUI is zero, PID is in the range of Etype(0x600-0xFFFF)";
        break;
    case VCL_ERROR_NO_PROTO_SELECTED:
        txt = "VCL Error - Select one of the protocol encapsulation types to create a group";
        break;
    case VCL_ERROR_EMPTY_PORT_LIST:
        txt = "VCL Error - The provided port list was empty.";
        break;
    case VCL_ERROR_INVALID_SUBNET:
        txt = "VCL Error - Subnet 0.0.0.0/x is not valid.";
        break;
    case VCL_ERROR_INVALID_ENCAP:
        txt = "VCL Error - Invalid Protocol Encapsulation was provided.";
        break;
    case VCL_ERROR_INVALID_IF_INDEX:
        txt = "VCL Error - Invalid interface index, index must be a port index";
        break;
    default:
        T_E("VCL: Unknown error code (%d = 0x%08x)", rc, rc);
        txt = "VCL: unknown error";
        break;
    }

    return txt;
}

mesa_rc vcl_debug_policy_no_set(mesa_acl_policy_no_t policy_no)
{
    if ((policy_no < fast_cap(MESA_CAP_ACL_POLICY_CNT)) || (policy_no == MESA_ACL_POLICY_NO_NONE)) {
        VCL_CRIT_ENTER();
        vcl_debug_policy_no = policy_no;
        VCL_CRIT_EXIT();
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc vcl_debug_policy_no_get(mesa_acl_policy_no_t *policy_no)
{
    if (policy_no) {
        VCL_CRIT_ENTER();
        *policy_no = vcl_debug_policy_no;
        VCL_CRIT_EXIT();
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}
extern "C" int vcl_icli_cmd_register();

/* Initialize module */
mesa_rc vcl_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
    mesa_rc  rc = VTSS_RC_OK;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        /* PPP1004: Initialize local data structures; Create and initialize
           OS objects(threads, mutexes, event flags etc). Resume threads if they should
           be running. This command is executed before scheduler is started, so don't
           perform any blocking operation such as critd_enter() */
        /* Initialize and register trace ressources */
        T_D("INIT_CMD_INIT");

        /* Initializing the local data structures */
        vcl_default_set();

        /* Create message pool. In standalone, we need three buffers because of
         * three requests in the INIT_CMD_ICFG_LOADING_POST event, and in
         * stacking, we need one per switch. */
        // Avoid "Warning -- Constant value Boolean" Lint warning, due to the use of VTSS_ISID_CNT below
        /*lint -e{506} */
        vcl_request_pool = msg_buf_pool_create(VTSS_MODULE_ID_VCL, "Request", VTSS_ISID_CNT > 3 ? VTSS_ISID_CNT : 3, sizeof(vcl_msg_req_t));

        /* Create semaphore for critical regions */
        critd_init(&vcl_data.crit, "VCL", VTSS_MODULE_ID_VCL, CRITD_TYPE_MUTEX_RECURSIVE);

#ifdef VTSS_SW_OPTION_ICFG
        mesa_rc vcl_icfg_init(void);
        if ((rc = vcl_icfg_init()) != VTSS_RC_OK) {
            T_E("ICFG Initialization failed (rc = %s)", error_txt(rc));
        }
#endif

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
        vcl_mib_init();
#endif

#if defined(VTSS_SW_OPTION_JSON_RPC)
        vcl_json_init();
#endif
        vcl_icli_cmd_register();
        break;

    case INIT_CMD_START:
        T_D("INIT_CMD_START");
        /* PPP1004 : Initialize the things that might perform blocking opearations as
           scheduler has been started. Also, register callbacks from other modules */
        rc = vcl_stack_register();
        break;

    case INIT_CMD_CONF_DEF:
        T_D("INIT_CMD_CONF_DEF at isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            vcl_mac_default_set();
            (void)vcl_msg_mac_vce_set(isid);
            vcl_proto_default_set();
            (void)vcl_msg_proto_vce_set(isid);
            vcl_ip_default_set();
            (void)vcl_msg_ip_vce_set(isid);
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("INIT_CMD_ICFG_LOADING_PRE");
        /* Read stack and switch configuration */
        vcl_mac_default_set();
        vcl_proto_default_set();
        vcl_ip_default_set();
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("INIT_CMD_ICFG_LOADING_POST");
        /* Apply all configuration to switch */
        rc = vcl_msg_proto_vce_set(isid);
        if (rc != VTSS_RC_OK) {
            T_D("Error while setting Protocol configuration to switch %d (rc = %s)", isid, error_txt(rc));
            return rc;
        }
        rc = vcl_msg_ip_vce_set(isid);
        if (rc != VTSS_RC_OK) {
            T_D("Error while setting Subnet configuration to switch %d (rc = %s)", isid, error_txt(rc));
            return rc;
        }
        rc = vcl_msg_mac_vce_set(isid);
        if (rc != VTSS_RC_OK) {
            T_D("Error while setting MAC configuration to switch %d (rc = %s)", isid, error_txt(rc));
            return rc;
        }
        break;

    default:
        break;
    }
    return rc;
}
