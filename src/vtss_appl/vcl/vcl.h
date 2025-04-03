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

#ifndef _VTSS_VCL_H_
#define _VTSS_VCL_H_

/* MAC part only-------------------------------------------*/
typedef struct {
    mesa_vce_id_t id;
    mesa_mac_t    smac;                                    /**< Source MAC Address */
    mesa_vid_t    vid;                                     /**< VLAN ID            */
    u8            ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE]; /**< Global port mask   */
} vcl_mac_vce_conf_global_t;

typedef struct {
    mesa_vce_id_t id;
    mesa_mac_t    smac;                                    /**< Source MAC Address */
    mesa_vid_t    vid;                                     /**< VLAN ID            */
    u8            ports[VTSS_PORT_BF_SIZE]; /**< Local port mask   */
} vcl_mac_vce_conf_local_t;

typedef struct vcl_mac_vce_global_t {
    struct vcl_mac_vce_global_t *next;
    vcl_mac_vce_conf_global_t   conf;
} vcl_mac_vce_global_t;

typedef struct vcl_mac_vce_local_t {
    struct vcl_mac_vce_local_t *next;
    vcl_mac_vce_conf_local_t   conf;
} vcl_mac_vce_local_t;

typedef struct {
    vcl_mac_vce_global_t global_table[VCL_MAC_VCE_MAX];
    vcl_mac_vce_local_t  local_table[VCL_MAC_VCE_MAX];
    vcl_mac_vce_global_t *global_used;
    vcl_mac_vce_global_t *global_free;
    vcl_mac_vce_local_t  *local_used;
    vcl_mac_vce_local_t  *local_free;
} vcl_mac_data_t;
/* MAC part only-------------------------------------------*/

/* IP Subnet part only-------------------------------------*/
typedef struct {
    mesa_vce_id_t id;                                         /**< VCE ID                                     */
    ulong         ip_addr;                                    /**< Source IP Address                          */
    u8            mask_len;                                   /**< Mask length                                */
    mesa_vid_t    vid;                                        /**< VLAN ID                                    */
    u8            ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE];    /**< Ports bit map                              */
} vcl_ip_vce_conf_global_t;

typedef struct {
    mesa_vce_id_t id;                               /**< VCE ID                                             */
    ulong         ip_addr;                          /**< IP Address                                  */
    u8            mask_len;                         /**< Mask length                                 */
    mesa_vid_t    vid;                              /**< VLAN ID                                     */
    u8            ports[VTSS_PORT_BF_SIZE];         /**< Ports on which IP-based VLAN is enabled     */
} vcl_ip_vce_conf_local_t;

typedef struct vcl_ip_vce_global_t {
    struct vcl_ip_vce_global_t *next;
    vcl_ip_vce_conf_global_t   conf;
} vcl_ip_vce_global_t;

typedef struct vcl_ip_vce_local_t {
    struct vcl_ip_vce_local_t *next;
    vcl_ip_vce_conf_local_t   conf;
} vcl_ip_vce_local_t;

typedef struct {
    vcl_ip_vce_global_t global_table[VCL_IP_VCE_MAX];
    vcl_ip_vce_local_t  local_table[VCL_IP_VCE_MAX];
    vcl_ip_vce_global_t *global_used;
    vcl_ip_vce_global_t *global_free;
    vcl_ip_vce_local_t  *local_used;
    vcl_ip_vce_local_t  *local_free;
} vcl_ip_data_t;
/* IP Subnet part only-------------------------------------*/

/* Protocol part only--------------------------------------*/
typedef struct {
    u8                               name[MAX_GROUP_NAME_LEN];
    vtss_appl_vcl_proto_encap_type_t proto_encap_type;
    vtss_appl_vcl_proto_encap_t      proto;
} vcl_proto_group_conf_proto_t;

typedef struct {
    u8         name[MAX_GROUP_NAME_LEN];
    mesa_vid_t vid;
    u8         ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE];
} vcl_proto_group_conf_entry_t;

typedef struct {
    mesa_vce_id_t                    id;
    vtss_appl_vcl_proto_encap_type_t proto_encap_type;
    vtss_appl_vcl_proto_encap_t      proto;
    mesa_vid_t                       vid;
    u8                               ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE];
} vcl_proto_vce_conf_global_t;

typedef struct {
    mesa_vce_id_t                    id;
    vtss_appl_vcl_proto_encap_type_t proto_encap_type;
    vtss_appl_vcl_proto_encap_t      proto;
    mesa_vid_t                       vid;
    u8                               ports[VTSS_PORT_BF_SIZE];
} vcl_proto_vce_conf_local_t;

typedef struct vcl_proto_group_proto_t {
    struct vcl_proto_group_proto_t *next;
    vcl_proto_group_conf_proto_t   conf;
} vcl_proto_group_proto_t;

typedef struct vcl_proto_group_entry_t {
    struct vcl_proto_group_entry_t *next;
    vcl_proto_group_conf_entry_t   conf;
} vcl_proto_group_entry_t;

typedef struct vcl_proto_vce_global_t {
    struct vcl_proto_vce_global_t *next;
    vcl_proto_vce_conf_global_t   conf;
} vcl_proto_vce_global_t;

typedef struct vcl_proto_vce_local_t {
    struct vcl_proto_vce_local_t *next;
    vcl_proto_vce_conf_local_t   conf;
} vcl_proto_vce_local_t;

typedef struct {
    vcl_proto_group_proto_t group_proto_table[VCL_PROTO_PROTOCOL_MAX];
    vcl_proto_group_entry_t group_entry_table[VCL_PROTO_VCE_MAX];
    vcl_proto_vce_global_t  global_table[VCL_PROTO_VCE_MAX];
    vcl_proto_vce_local_t   local_table[VCL_PROTO_VCE_MAX];
    vcl_proto_group_proto_t *group_proto_used;
    vcl_proto_group_proto_t *group_proto_free;
    vcl_proto_group_entry_t *group_entry_used;
    vcl_proto_group_entry_t *group_entry_free;
    vcl_proto_vce_global_t  *global_used;
    vcl_proto_vce_global_t  *global_free;
    vcl_proto_vce_local_t   *local_used;
    vcl_proto_vce_local_t   *local_free;
} vcl_proto_data_t;
/* Protocol part only--------------------------------------*/

typedef struct {
    critd_t          crit;
    vcl_mac_data_t   mac_data;
    vcl_ip_data_t    ip_data;
    vcl_proto_data_t proto_data;
} vcl_data_t;

/* ================================================================= *
 *  VCL stack messages
 * ================================================================= */
/**
 * \brief VCL messages IDs
 */
typedef enum {
    VCL_MSG_ID_MAC_VCE_SET,                           /**< VCL MAC-based VLAN configuration set request          */
    VCL_MSG_ID_MAC_VCE_ADD,                           /**< VCL MAC-based VLAN configuration add request          */
    VCL_MSG_ID_MAC_VCE_DEL,                           /**< VCL MAC-based VLAN configuration delete request       */
    VCL_MSG_ID_IP_VCE_SET,
    VCL_MSG_ID_IP_VCE_ADD,
    VCL_MSG_ID_IP_VCE_DEL,
    VCL_MSG_ID_PROTO_VCE_SET,                         /**< VCL protocol-based VLAN configuration set request     */
    VCL_MSG_ID_PROTO_VCE_ADD,                         /**< VCL protocol-based VLAN configuration add request     */
    VCL_MSG_ID_PROTO_VCE_DEL,                         /**< VCL protocol-based VLAN configuration delete request  */
} vcl_msg_id_t;

typedef struct {
    vcl_msg_id_t             msg_id;
    u32                      count;
    vcl_mac_vce_conf_local_t conf[VCL_MAC_VCE_MAX];
} vcl_msg_mac_vce_set_t;

typedef struct {
    vcl_msg_id_t             msg_id;
    vcl_mac_vce_conf_local_t conf;
} vcl_msg_mac_vce_t;

typedef struct {
    vcl_msg_id_t             msg_id;
    u32                      count;
    vcl_ip_vce_conf_local_t conf[VCL_IP_VCE_MAX];
} vcl_msg_ip_vce_set_t;

typedef struct {
    vcl_msg_id_t             msg_id;
    vcl_ip_vce_conf_local_t conf;
} vcl_msg_ip_vce_t;

typedef struct {
    vcl_msg_id_t               msg_id;                               /**< Message ID                             */
    u32                        count;                                /**< Number of entries                      */
    vcl_proto_vce_conf_local_t conf[VCL_PROTO_VCE_MAX];   /**< Configuration                          */
} vcl_msg_proto_vce_set_t;

typedef struct {
    vcl_msg_id_t               msg_id;                           /**< Message ID                                 */
    vcl_proto_vce_conf_local_t conf;                             /**< Configuration                              */
} vcl_msg_proto_vce_t;

typedef struct {
    union {
        vcl_msg_mac_vce_set_t   a;             /**< MAC VCL SET request message                       */
        vcl_msg_mac_vce_t       b;             /**< MAC VCL ADD/DELETE request message                */
        vcl_msg_proto_vce_set_t c;             /**< PROTO VCL SET request message                     */
        vcl_msg_proto_vce_t     d;             /**< PROTO VCL ADD/DELETE request message              */
        vcl_msg_ip_vce_set_t    e;             /**< IP VCL SET request message                        */
        vcl_msg_ip_vce_t        f;             /**< IP VCL ADD/DELETE request message                 */
    } data;
} vcl_msg_req_t;
/* ================================================================= */

#endif /* _VTSS_VCL_H_ */
