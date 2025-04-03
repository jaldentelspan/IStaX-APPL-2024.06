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

#ifndef _VCL_API_H_
#define _VCL_API_H_

/**
 * \file vcl_api.h
 * \brief VCL platform header file
 *
 * This file contains the definitions of management API functions and associated types.
 * This file is exposed to other modules.
 */

#include <vtss/appl/vcl.h>
#include "main_types.h" /**< For uXXX, iXXX, and BOOL          */

using namespace vtss;

#define VCL_TYPE_MAC    1
#define VCL_TYPE_IP     2
#define VCL_TYPE_PROTO  3
#define VCL_TYPE_STREAM 4

/**
 * Maximum number of MAC to VLAN ID mappings that can be stored in the stack (these mappings are unique).
 */
#define VCL_MAC_VCE_MAX        256

/**
 * Maximum number of IP Subnet to VLAN ID mappings that can be stored in the stack (these mappings are unique).
 */
#define VCL_IP_VCE_MAX         128

/**
 * Maximum number of Protocol to Protocol Group mappings that can be stored in the stack (these mappings are unique).
 */
#define VCL_PROTO_PROTOCOL_MAX 128

/**
 * Maximum number of Protocol to VLAN ID mappings that can be stored in the stack.
 *
 * These mappings are not unique, so a certain Protocol can be mapped to various VIDs (always through a Protocol Group)
 * as long as these mappings do NOT share overlapping port sets.
 *
 * NOTE : This is the number of Protocol to VID mappings and not Protocol Group to VID mappings. The latter are not limited
 * directly, but indirectly through the combination of VCL_PROTO_PROTOCOL_MAX and VCL_PROTO_VCE_MAX.
 */
#define VCL_PROTO_VCE_MAX      256

#define OUI_SIZE VTSS_APPL_VCL_OUI_SIZE
#define MAX_GROUP_NAME_LEN VTSS_APPL_VCL_MAX_GROUP_NAME_LEN

#define ETHERTYPE_ARP 0x806
#define ETHERTYPE_IP  0x800
#define ETHERTYPE_IP6 0x86DD
#define ETHERTYPE_IPX 0x8137
#define ETHERTYPE_AT  0x809B

/**
 * \brief VCL error codes.
 * This enum identifies different error types used in the VCL module.
 */
enum {
    VCL_ERROR_ENTRY_NOT_FOUND = MODULE_ERROR_START(VTSS_MODULE_ID_VCL),
    VCL_ERROR_EMPTY_ENTRY,
    VCL_ERROR_ENTRY_DIFF_VID,
    VCL_ERROR_MAC_TABLE_FULL,
    VCL_ERROR_VCE_ID_EXCEEDED,
    VCL_ERROR_NOT_PRIMARY_SWITCH,
    VCL_ERROR_INVALID_ISID,
    VCL_ERROR_SYSTEM_MAC,
    VCL_ERROR_MULTIBROAD_MAC,
    VCL_ERROR_MSG_CREATION_FAIL,
    VCL_ERROR_IP_TABLE_FULL,
    VCL_ERROR_INVALID_MASK_LENGTH,
    VCL_ERROR_INVALID_VLAN_ID,
    VCL_ERROR_INVALID_GROUP_NAME,
    VCL_ERROR_PROTOCOL_ALREADY_CONF,
    VCL_ERROR_GROUP_PROTO_TABLE_FULL,
    VCL_ERROR_INVALID_ENCAP_TYPE,
    VCL_ERROR_ENTRY_OVERLAPPING_PORTS,
    VCL_ERROR_GROUP_ENTRY_TABLE_FULL,
    VCL_ERROR_PROTO_TABLE_FULL,
    VCL_ERROR_INVALID_PROTO_CNT,
    VCL_ERROR_NULL_GROUP_NAME,
    VCL_ERROR_INVALID_PID,
    VCL_ERROR_NO_PROTO_SELECTED,
    VCL_ERROR_EMPTY_PORT_LIST,
    VCL_ERROR_INVALID_SUBNET,
    VCL_ERROR_INVALID_ENCAP,
    VCL_ERROR_INVALID_IF_INDEX,
};

typedef enum {
    VCL_USR_CFM,
    VCL_USR_STREAM,
    VCL_USR_DEFAULT
} vcl_user_t;

typedef struct {
    mesa_mac_t       smac;                                      /**< Source MAC Address                         */
    mesa_vid_t       vid;                                       /**< VLAN ID                                    */
    mesa_port_list_t ports;                                     /**< Ports on which SMAC based VLAN is enabled  */
} vcl_mac_mgmt_vce_conf_local_t;

typedef struct {
    mesa_mac_t       smac;                                      /**< Source MAC Address                         */
    mesa_vid_t       vid;                                       /**< VLAN ID                                    */
    mesa_port_list_t ports[VTSS_ISID_CNT];                      /**< Ports on which SMAC based VLAN is enabled  */
} vcl_mac_mgmt_vce_conf_global_t;

typedef struct {
    mesa_ipv4_t      ip_addr;                                   /**< IP Address                                 */
    u8               mask_len;                                  /**< Mask length                                */
    mesa_vid_t       vid;                                       /**< VLAN ID                                    */
    mesa_port_list_t ports;                                     /**< Ports on which IP-based VLAN is enabled    */
} vcl_ip_mgmt_vce_conf_local_t;

typedef struct {
    mesa_ipv4_t      ip_addr;                                   /**< IP Address                                 */
    u8               mask_len;                                  /**< Mask length                                */
    mesa_vid_t       vid;                                       /**< VLAN ID                                    */
    mesa_port_list_t ports[VTSS_ISID_CNT];                      /**< Ports on which IP-based VLAN is enabled    */
} vcl_ip_mgmt_vce_conf_global_t;

typedef struct {
    u8                               name[MAX_GROUP_NAME_LEN];
    vtss_appl_vcl_proto_encap_type_t proto_encap_type;
    vtss_appl_vcl_proto_encap_t      proto;
} vcl_proto_mgmt_group_conf_proto_t;

typedef struct {
    u8               name[MAX_GROUP_NAME_LEN]; /**< Group ID                                   */
    mesa_vid_t       vid;                      /**< VLAN ID                                    */
    mesa_port_list_t ports;                    /**< Port list on which this mapping is valid   */
} vcl_proto_mgmt_group_conf_entry_local_t;

typedef struct {
    u8               name[MAX_GROUP_NAME_LEN]; /**< Group ID                                   */
    mesa_vid_t       vid;                      /**< VLAN ID                                    */
    mesa_port_list_t ports[VTSS_ISID_CNT];     /**< Port list on which this mapping is valid   */
} vcl_proto_mgmt_group_conf_entry_global_t;

typedef struct {
    vtss_appl_vcl_proto_encap_type_t proto_encap_type;
    vtss_appl_vcl_proto_encap_t      proto;
    mesa_vid_t                       vid;
    mesa_port_list_t                 ports;
} vcl_proto_mgmt_proto_conf_local_t;

/**
 *  \brief  VCL module init.
 *  This function initializes the VCL module during startup.
 *
 *  \param data [IN]:      Pointer to vtss_init_data_t structure that contains command, isid
 *                         and other information.
 *
 *  \return
 *  VTSS_RC_OK on success.
 */
mesa_rc vcl_init(vtss_init_data_t *data);

const char *vcl_error_txt(mesa_rc rc);

const char *vcl_proto_mgmt_encaptype2string(vtss_appl_vcl_proto_encap_type_t encap);

mesa_rc vcl_mac_mgmt_conf_add(vtss_isid_t isid_add, vcl_mac_mgmt_vce_conf_local_t *mac_vce);

mesa_rc vcl_mac_mgmt_conf_del(vtss_isid_t isid_del, mesa_mac_t *smac);

mesa_rc vcl_mac_mgmt_conf_get(vtss_isid_t isid_get, vcl_mac_mgmt_vce_conf_global_t *mac_vce, BOOL first, BOOL next);

mesa_rc vcl_mac_mgmt_conf_local_get(vcl_mac_mgmt_vce_conf_local_t *mac_vce, BOOL first, BOOL next);

mesa_rc vcl_mac_mgmt_conf_itr(mesa_mac_t *mac, BOOL first);

mesa_rc vcl_ip_mgmt_conf_add(vtss_isid_t isid_add, vcl_ip_mgmt_vce_conf_local_t *ip_vce);

mesa_rc vcl_ip_mgmt_conf_del(vtss_isid_t isid_del, vcl_ip_mgmt_vce_conf_local_t *ip_vce);

mesa_rc vcl_ip_mgmt_conf_get(vtss_isid_t isid_get, vcl_ip_mgmt_vce_conf_global_t *ip_vce, BOOL first, BOOL next);

mesa_rc vcl_ip_mgmt_conf_local_get(vcl_ip_mgmt_vce_conf_local_t *ip_vce, BOOL first, BOOL next);

mesa_rc vcl_ip_mgmt_conf_itr(mesa_ipv4_network_t *sub, BOOL first);

mesa_rc vcl_proto_mgmt_proto_add(vcl_proto_mgmt_group_conf_proto_t *group_conf);

mesa_rc vcl_proto_mgmt_proto_del(vcl_proto_mgmt_group_conf_proto_t *group_conf);

mesa_rc vcl_proto_mgmt_proto_get(vcl_proto_mgmt_group_conf_proto_t *group_conf, BOOL first, BOOL next);

mesa_rc vcl_proto_mgmt_proto_itr(vtss_appl_vcl_proto_t *enc, BOOL first);

mesa_rc vcl_proto_mgmt_conf_add(vtss_isid_t isid_add, vcl_proto_mgmt_group_conf_entry_local_t *proto_vce);

mesa_rc vcl_proto_mgmt_conf_del(vtss_isid_t isid_del, vcl_proto_mgmt_group_conf_entry_local_t *proto_vce);

mesa_rc vcl_proto_mgmt_conf_get(vtss_isid_t isid_get, vcl_proto_mgmt_group_conf_entry_global_t *proto_vce, BOOL first, BOOL next);

mesa_rc vcl_proto_mgmt_conf_local_proto_get(vcl_proto_mgmt_proto_conf_local_t *proto_vce, BOOL first, BOOL next);

mesa_rc vcl_proto_mgmt_conf_itr(vtss_appl_vcl_proto_group_conf_proto_t *group, BOOL first);

/**
 * \brief VCL debug command for setting the policy number.
 * This function sets the policy number that is used in subsequent VCL add commands.
 * Works on local switch only (no stack support).
 *
 * \param policy_no      [IN]:     Policy number. 0..( VTSS_ACL_POLICIES - 1) or VTSS_ACL_POLICY_NO_NONE
 *
 * \return
 * VTSS_RC_OK on success.
 */
mesa_rc vcl_debug_policy_no_set(mesa_acl_policy_no_t policy_no);

/**
 * \brief VCL debug command for getting the policy number.
 * This function gets the currently configured policy number that is used in subsequent VCL add commands.
 * Works on local switch only (no stack support).
 *
 * \param policy_no      [OUT]:    Policy number. 0..( VTSS_ACL_POLICIES - 1) or VTSS_ACL_POLICY_NO_NONE
 *
 * \return
 * VTSS_RC_OK on success.
 */
mesa_rc vcl_debug_policy_no_get(mesa_acl_policy_no_t *policy_no);

mesa_rc vcl_register_vce(uint32_t user, mesa_vce_id_t id, mesa_vce_id_t position);
mesa_rc vcl_unregister_vce(uint32_t user, mesa_vce_id_t id);
mesa_rc vcl_vce_get_first(vcl_user_t user, mesa_vce_id_t &id);
mesa_rc vcl_vce_get_last(vcl_user_t user, mesa_vce_id_t &id);
void vcl_vce_debug_print(char *buf, int buf_size);
void dump_vce(const char *file, int line, const mesa_vce_t &vce);
#endif /* _VCL_API_H_ */

