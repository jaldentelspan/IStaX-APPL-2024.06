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
#include "conf_api.h"
#include "packet_api.h"
#include "ip_utils.hxx" /* For vtss_ip_checksum() */
#include "main_types.h"
#include "misc_api.h"
#ifdef VTSS_SW_OPTION_ICFG
#include "udld_icfg.h"
#endif

#include "netdb.h"
#include "microchip/ethernet/switch/api.h"
#include "critd_api.h"
#include "sysutil_api.h"
#include "microchip/ethernet/switch/api.h"
#include "control_api.h"
#include "vlan_api.h"
#include "msg_api.h"
#include "port_api.h"
#include "port_iter.hxx"

#include "udld_api.h"
#include "udld.h"
#include "udld_trace.h"
#include "udld_tlv.h"
#include "vtss_safe_queue.hxx"

/* Critical region protection protecting the following block of variables */
static critd_t udld_crit_control;
static critd_t udld_crit_mgmt;
/* Thread variables */
static vtss_handle_t   udld_client_thread_handle;
static vtss_thread_t   udld_client_thread_block;
static vtss_handle_t   udld_control_timer_thread_handle;
static vtss_thread_t   udld_control_timer_thread_block;

/* Queue Variables */
static vtss::SafeQueue udld_queue;

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "UDLD", "Unidirectional link detection module."
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_UDLD] = {
        "udld",
        "Uni directional link detection ",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define UDLD_CRIT_CONTROL_ENTER() critd_enter(&udld_crit_control, __FILE__, __LINE__)
#define UDLD_CRIT_CONTROL_EXIT()  critd_exit( &udld_crit_control, __FILE__, __LINE__)
#define UDLD_CRIT_MGMT_ENTER()    critd_enter(&udld_crit_mgmt,    __FILE__, __LINE__)
#define UDLD_CRIT_MGMT_EXIT()     critd_exit( &udld_crit_mgmt,    __FILE__, __LINE__)

typedef struct {
    CapArray<vtss_appl_udld_port_conf_struct_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_conf; /**< Port conf */
} vtss_appl_udld_port_conf_t;

static udld_port_control_info_t      udld_port_control_info;
static vtss_appl_udld_port_conf_t    conf;   // Current configuration for this switch
static u8 udld_frame_buffer[MAX_UDLD_FRAME_SIZE];// Buffer for the frame currently being analysed.
static udld_packet_t                 udld_packet; // Stucture containing the UDLD information we supports
static bool                          system_conf_has_changed ; // Set by callback function if systemname changes.

#define  UDLD_PORT_LOCAL_INFO_PONTER_GET(port_no)        &udld_port_control_info.local_info.port[port_no]
#define  UDLD_PORT_REMOTE_CACHE_LIST_PONTER_GET(port_no) &udld_port_control_info.remote_info.port[port_no]
#define  UDLD_PORT_REMOTE_CACHE_LIST_GET(port_no)        udld_port_control_info.remote_info.port[port_no].list
#define  UDLD_TEST_RC(func)                              if(func != VTSS_RC_OK) { \
                                                            return(VTSS_RC_ERROR); \
                                                         }
#define  UDLD_TEST_PORT_ISID_RANGE(isid, port_no)        if(!(isid < VTSS_ISID_END && port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT))) { \
                                                            return(VTSS_RC_ERROR); \
                                                         }
#define  UDLD_TEST_SWITCH_PRIMARY()                      if (!msg_switch_is_primary()) { \
                                                            return(VTSS_RC_ERROR); \
                                                         }

static BOOL udld_is_admin_mode_enabled (const u32 port_no)
{
    return (conf.port_conf[port_no].admin);
}

static u32 udld_port_mode_get(u32 port_no, vtss_appl_udld_mode_t *mode)
{
    memset(mode, 0, sizeof(vtss_appl_udld_mode_t));
    *mode = conf.port_conf[port_no].udld_mode;
    return VTSS_UDLD_RC_OK;
}

static u32 udld_port_probe_msg_interval_get(u32 port_no, u32 *probe_msg_interval)
{
    UDLD_CRIT_MGMT_ENTER();
    *probe_msg_interval = conf.port_conf[port_no].probe_msg_interval;
    UDLD_CRIT_MGMT_EXIT();
    return VTSS_UDLD_RC_OK;
}

static void udld_port_conf_init(u32 port_no)
{
    conf.port_conf[port_no].admin              = VTSS_APPL_UDLD_ADMIN_DISABLE;
    conf.port_conf[port_no].udld_mode          = VTSS_APPL_UDLD_MODE_DISABLE;
    conf.port_conf[port_no].probe_msg_interval = UDLD_DEFAULT_MESSAGE_INTERVAL;
}

static mesa_rc udld_port_link_up_status_get(const u32 port_no, BOOL  *is_up)
{
    vtss_appl_port_status_t port_status;
    vtss_ifindex_t          ifindex;

    if (is_up == NULL) {
        return VTSS_UDLD_RC_INVALID_PARAMETER;
    }

    (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, port_no, &ifindex);
    if (vtss_appl_port_status_get(ifindex, &port_status) != VTSS_RC_OK) {
        return VTSS_UDLD_RC_INVALID_PARAMETER;
    }

    *is_up = port_status.link;
    return VTSS_UDLD_RC_OK;
}

static void udld_update_port_info_variable(udld_port_info_struct_t *port_info)
{
    if (port_info) {
        port_info->detection_state              = VTSS_UDLD_DETECTION_STATE_UNKNOWN;
        port_info->detection_start              = FALSE;
        port_info->detection_window_timer_start = 0;
        port_info->detection_window_timer_end   = UDLD_NORMAL_DETECTION_WINDOW;
        port_info->seq_num                      = UDLD_DEFAULT_SEQ_NUM;
        port_info->echo_tx                      = 0;
        port_info->probe_msg_interval           = UDLD_DEFAULT_MESSAGE_INTERVAL_LINKUP;
        port_info->flags                        = UDLD_FLAG_RT;
        port_info->proto_phase                  = VTSS_UDLD_PROTO_PHASE_LINK_UP;
    }
}
void udld_something_has_changed(void)
{
    system_conf_has_changed = 1;
}
static void udld_port_info_default_init(u32 port_no)
{
    udld_port_info_struct_t  *port_info = NULL;
    system_conf_t            sys_conf;
    char device_id[MAX_DEVICE_ID_LENGTH];
    char port_id[MAX_PORT_ID_LENGTH];
    char device_name[MAX_DEVICE_NAME_LENGTH];

    port_info = UDLD_PORT_LOCAL_INFO_PONTER_GET(port_no);
    memset(port_info, 0, sizeof(udld_port_info_struct_t));

    udld_get_chassis_id_str((char *)&device_id[0]);

    sprintf(port_id, "%u", (port_no + 1));

    if (system_get_config(&sys_conf) != VTSS_RC_OK) {
        strcpy(&device_name[0], "Unknown");
    } else {
        misc_strncpyz(&device_name[0], &sys_conf.sys_name[0], MAX_DEVICE_NAME_LENGTH);
    }
    strcpy((char *)&port_info->device_id[0], &device_id[0]);
    strcpy((char *)&port_info->port_id[0], &port_id[0]);
    strcpy((char *)&port_info->device_name[0], &device_name[0]);

    udld_update_port_info_variable(port_info);
    port_info->flags               = UDLD_FLAG_RT_RSY;
    port_info->admin_disable       = FALSE;
    port_info->probe_msg_rx        = 0; //using for aggressive mode port shutdown
}
static void udld_message_post(vtss_udld_message_t *message)
{
    T_D("Post UDLD event:%u on port(%u)", message->event_code,
        message->event_on_port);
    if (udld_queue.vtss_safe_queue_put(message) != TRUE) {
        VTSS_FREE(message);
    }
}
static void udld_aggressive_port_disable(mesa_port_no_t port_no, BOOL dis)
{
    port_vol_conf_t port_vol_conf;
    T_D("%sable port: %u", dis ? "dis" : "en", port_no);
    if (port_vol_conf_get(PORT_USER_UDLD, port_no, &port_vol_conf) == VTSS_RC_OK &&
        port_vol_conf.disable != dis) {
        port_vol_conf.disable = dis;
        if (port_vol_conf_set(PORT_USER_UDLD, port_no, &port_vol_conf) != VTSS_RC_OK) {
            T_E("port_vol_conf_set(%u) failed", port_no);
        }
    }
}
mesa_rc vtss_appl_udld_port_admin_set(vtss_isid_t isid,  mesa_port_no_t port_no, vtss_appl_udld_admin_t admin)
{
    T_D("port_no: %u, admin:%u", port_no, admin);
    vtss_udld_message_t            *message = NULL;
    UDLD_TEST_SWITCH_PRIMARY();
    UDLD_TEST_PORT_ISID_RANGE(isid, port_no);

    UDLD_CRIT_MGMT_ENTER();
    if (conf.port_conf[port_no].admin != admin) {
        conf.port_conf[port_no].admin = admin;
    } else {
        UDLD_CRIT_MGMT_EXIT();
        return VTSS_RC_OK;
    }
    UDLD_CRIT_MGMT_EXIT();
    if (admin) {
        udld_add_to_mac_table(VTSS_ISID_LOCAL, port_no, NULL);
    }
    message = (vtss_udld_message_t *)VTSS_MALLOC(sizeof(vtss_udld_message_t));
    if (message != NULL) {
        if (admin) {
            message->event_code = VTSS_UDLD_PORT_PROTO_ENABLE_EVENT;
        } else {
            message->event_code = VTSS_UDLD_PORT_PROTO_DISABLE_EVENT;
        }
        message->event_on_port = port_no;
        udld_message_post(message);
    } else {
        T_E("Unable to allocate the memory to handle the received event on port(%u)",
            port_no);
    }
    return VTSS_RC_OK;
}
mesa_rc vtss_appl_udld_port_admin_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_udld_admin_t *admin)
{
    if (!admin) {
        return VTSS_RC_ERROR;
    }
    UDLD_TEST_SWITCH_PRIMARY();
    UDLD_TEST_PORT_ISID_RANGE(isid, port_no);

    memset(admin, 0, sizeof(vtss_appl_udld_admin_t));
    UDLD_CRIT_MGMT_ENTER();
    *admin = conf.port_conf[port_no].admin;
    UDLD_CRIT_MGMT_EXIT();
    return VTSS_RC_OK;
}
mesa_rc vtss_appl_udld_port_mode_set(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_udld_mode_t mode)
{
    T_D("enter isid: %u, port_no: %u, mode: %u", isid, port_no, mode);
    UDLD_TEST_SWITCH_PRIMARY();
    UDLD_TEST_PORT_ISID_RANGE(isid, port_no);

    UDLD_CRIT_MGMT_ENTER();
    conf.port_conf[port_no].udld_mode = mode;
    UDLD_CRIT_MGMT_EXIT();
    return VTSS_RC_OK;
}
mesa_rc vtss_appl_udld_port_mode_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_udld_mode_t *mode)
{
    if (!mode) {
        return VTSS_RC_ERROR;
    }
    UDLD_TEST_SWITCH_PRIMARY();
    UDLD_TEST_PORT_ISID_RANGE(isid, port_no);

    memset(mode, 0, sizeof(vtss_appl_udld_mode_t));
    UDLD_CRIT_MGMT_ENTER();
    *mode = conf.port_conf[port_no].udld_mode;
    UDLD_CRIT_MGMT_EXIT();
    return VTSS_RC_OK;
}
mesa_rc vtss_appl_udld_port_probe_msg_interval_set(vtss_isid_t isid, mesa_port_no_t port_no, u32 probe_msg_interval)
{
    T_D("enter probe_msg_interval: %u", probe_msg_interval);
    UDLD_TEST_SWITCH_PRIMARY();
    UDLD_TEST_PORT_ISID_RANGE(isid, port_no);

    if (probe_msg_interval < 7 || probe_msg_interval > 90) {
        T_D("Invalid range probe_msg_interval: %u", probe_msg_interval);
        return VTSS_RC_ERROR;
    }
    UDLD_CRIT_MGMT_ENTER();
    conf.port_conf[port_no].probe_msg_interval = probe_msg_interval;
    UDLD_CRIT_MGMT_EXIT();
    return VTSS_RC_OK;
}
mesa_rc vtss_appl_udld_port_probe_msg_interval_get(vtss_isid_t isid, mesa_port_no_t port_no, u32 *probe_msg_interval)
{
    if (!probe_msg_interval) {
        return VTSS_RC_ERROR;
    }
    UDLD_TEST_SWITCH_PRIMARY();
    UDLD_TEST_PORT_ISID_RANGE(isid, port_no);

    UDLD_CRIT_MGMT_ENTER();
    *probe_msg_interval = conf.port_conf[port_no].probe_msg_interval;
    UDLD_CRIT_MGMT_EXIT();
    return VTSS_RC_OK;
}
mesa_rc vtss_appl_udld_port_conf_set(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_udld_port_conf_struct_t *port_conf)
{
    if (!port_conf) {
        return VTSS_RC_ERROR;
    }
    UDLD_TEST_SWITCH_PRIMARY();
    UDLD_TEST_PORT_ISID_RANGE(isid, port_no);

    T_D("udld_mode: %u, probe_msg_interva: %u", port_conf->udld_mode, port_conf->probe_msg_interval);
    if (port_conf->udld_mode) {
        UDLD_TEST_RC(vtss_appl_udld_port_admin_set(isid, port_no, VTSS_APPL_UDLD_ADMIN_ENABLE));
    } else {
        UDLD_TEST_RC(vtss_appl_udld_port_admin_set(isid, port_no, VTSS_APPL_UDLD_ADMIN_DISABLE));
    }
    UDLD_TEST_RC(vtss_appl_udld_port_mode_set(isid, port_no, port_conf->udld_mode));
    UDLD_TEST_RC(vtss_appl_udld_port_probe_msg_interval_set(isid, port_no, port_conf->probe_msg_interval));
    return VTSS_RC_OK;
}
mesa_rc vtss_appl_udld_port_conf_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_udld_port_conf_struct_t *port_conf)
{
    if (!port_conf) {
        return VTSS_RC_ERROR;
    }
    UDLD_TEST_SWITCH_PRIMARY();
    UDLD_TEST_PORT_ISID_RANGE(isid, port_no);

    UDLD_CRIT_MGMT_ENTER();
    port_conf->admin              = conf.port_conf[port_no].admin;
    port_conf->udld_mode          = conf.port_conf[port_no].udld_mode;
    port_conf->probe_msg_interval = conf.port_conf[port_no].probe_msg_interval;
    UDLD_CRIT_MGMT_EXIT();
    return VTSS_RC_OK;
}
mesa_rc vtss_appl_udld_neighbor_info_get_first(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_udld_neighbor_info_t *info)
{
    mesa_rc                  rc = VTSS_RC_OK;
    udld_remote_cache_list_t *tmp = NULL;
    udld_remote_cache_list_t *list = NULL;
    if (!info) {
        return VTSS_RC_ERROR;
    }
    UDLD_TEST_SWITCH_PRIMARY();
    UDLD_TEST_PORT_ISID_RANGE(isid, port_no);

    UDLD_CRIT_CONTROL_ENTER();
    list = UDLD_PORT_REMOTE_CACHE_LIST_GET(port_no);
    if (list != NULL) {
        tmp = list;
        if (tmp != NULL) {
            memcpy(&info->device_id[0], &tmp->info.device_id[0], MAX_DEVICE_ID_LENGTH);
            memcpy(&info->port_id[0], &tmp->info.port_id[0], MAX_PORT_ID_LENGTH);
            memcpy(&info->device_name[0], &tmp->info.device_name[0], MAX_DEVICE_NAME_LENGTH);
            info->detection_state = tmp->info.detection_state;
            info->next = tmp->next;
        }
    } else {
        rc = VTSS_RC_ERROR;
    }
    UDLD_CRIT_CONTROL_EXIT();
    return rc;
}
mesa_rc vtss_appl_udld_neighbor_info_get_next(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_udld_neighbor_info_t *info)
{
    udld_remote_cache_list_t       *tmp = NULL;
    udld_remote_cache_list_t       *list = NULL;
    BOOL                            node_found = FALSE;
    if (!info) {
        return VTSS_RC_ERROR;
    }
    UDLD_TEST_SWITCH_PRIMARY();
    UDLD_TEST_PORT_ISID_RANGE(isid, port_no);
    tmp = info->next;
    if (!tmp) {
        T_D("no more entries");
        return VTSS_RC_ERROR;
    }
    UDLD_CRIT_CONTROL_ENTER();
    list = UDLD_PORT_REMOTE_CACHE_LIST_GET(port_no);
    while (list) {
        if (list == tmp) {
            node_found = TRUE;
            break;
        }
        list = list->next;
    }
    if (!node_found) {
        T_D("topolgy changed, cache list modified");
        UDLD_CRIT_CONTROL_EXIT();
        return VTSS_RC_ERROR;
    }

    strcpy((char *)&info->device_id[0], (char *)&tmp->info.device_id[0]);
    strcpy((char *)&info->port_id[0], (char *)&tmp->info.port_id[0]);
    strcpy((char *)&info->device_name[0], (char *)&tmp->info.device_name[0]);
    info->detection_state = tmp->info.detection_state;
    info->next = tmp->next;
    UDLD_CRIT_CONTROL_EXIT();
    return VTSS_RC_OK;
}
mesa_rc vtss_appl_udld_port_info_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_udld_port_info_t *info)
{
    udld_port_info_struct_t  *port_info = NULL;
    if (!info) {
        return VTSS_RC_ERROR;
    }
    UDLD_TEST_SWITCH_PRIMARY();
    UDLD_TEST_PORT_ISID_RANGE(isid, port_no);

    UDLD_CRIT_CONTROL_ENTER();
    port_info = UDLD_PORT_LOCAL_INFO_PONTER_GET(port_no);
    if (port_info) {
        strcpy((char *)&info->device_id[0], (char *)&port_info->device_id[0]);
        strcpy((char *)&info->port_id[0], (char *)&port_info->port_id[0]);
        strcpy((char *)&info->device_name[0], (char *)&port_info->device_name[0]);
        info->detection_state = port_info->detection_state;
    }
    UDLD_CRIT_CONTROL_EXIT();
    return VTSS_RC_OK;
}

static BOOL udld_port_is_authorized(mesa_port_no_t port_idx)
{
    mesa_auth_state_t auth_state;
    (void)mesa_auth_port_state_get(NULL, port_idx, &auth_state);
    return (auth_state == MESA_AUTH_STATE_BOTH);
}
static void udld_clear_cache_entry(u32 port_no)
{
    udld_remote_cache_list_head_t  *head_cache = NULL;
    udld_remote_cache_list_t       *next_list = NULL;
    udld_remote_cache_list_t       *tmp_list = NULL;
    head_cache = UDLD_PORT_REMOTE_CACHE_LIST_PONTER_GET(port_no);
    if (head_cache && head_cache->list) {
        next_list = head_cache->list;
        while (next_list) {
            tmp_list = next_list->next;
            VTSS_FREE( next_list);
            next_list = tmp_list;
            head_cache->count--;
        }
        head_cache->list = NULL;
    }
}

static mesa_rc udld_rx_tlv_decode(const u8 *udld_data_ptr, u16 udld_data_len)
{
    i16           udld_byte_analysed = 0; // Contains the byte currently being analysed.
    u16           tlv_type; // The TLV type
    u16           tlv_len = 0; // The length of the TLV
    const uint8_t *address_ptr = udld_data_ptr;

    T_D("udld_data_len = %u", udld_data_len);
    memset(&udld_frame_buffer[0], '\0', sizeof(MAX_UDLD_FRAME_SIZE));
    memcpy(&udld_frame_buffer[0], udld_data_ptr, udld_data_len);
    udld_packet.id_pairs             = 0;
    while (udld_byte_analysed < udld_data_len) {
        tlv_type = UDLD_NTOHS(udld_data_ptr);       // Convert to 16 bits
        tlv_len  = UDLD_NTOHS((udld_data_ptr + 2)); // Convert to 16 bits
        T_D("tlv = 0x%X, tlv type = 0x%X", tlv_len, tlv_type);
        if (tlv_len == 0 || tlv_len > udld_data_len ) {
            T_D("UDLD frame contained invalid TLV length, tlv_len = %u, udld_data_len = %u", tlv_len, udld_data_len);
            return VTSS_RC_ERROR;
        }
        switch (tlv_type) {
        case UDLD_TLV_TYPE_DEVICE_ID:
            udld_packet.device_id = &udld_frame_buffer[4 + udld_byte_analysed] ;
            udld_packet.device_id_len = tlv_len - 4;
            T_D("DEVICE_ID, device_id = %s, device_id_len = %d", udld_packet.device_id, udld_packet.device_id_len);
            if (udld_packet.device_id_len >= MAX_DEVICE_ID_LENGTH) {
                udld_packet.device_id_len = (MAX_DEVICE_ID_LENGTH - 1);
                /* we support max length 254, last byte used for terminating \0 character */
            } else if (udld_packet.device_id_len == 0) {
                T_D("DEVICE_ID, device_id = %s, device_id_len = %d", udld_packet.device_id, udld_packet.device_id_len);
                /* device_id must be non zero */
                return VTSS_RC_ERROR;
            }
            break;
        case UDLD_TLV_TYPE_PORT_ID:
            udld_packet.port_id = &udld_frame_buffer[4 + udld_byte_analysed] ;
            udld_packet.port_id_len = tlv_len - 4;
            T_D("PORT_ID, ID = %s, len = %d", udld_packet.port_id, udld_packet.port_id_len);
            if (udld_packet.port_id_len >= MAX_PORT_ID_LENGTH) {
                udld_packet.port_id_len = (MAX_PORT_ID_LENGTH - 1);
                /* we support max length 254, last byte used for terminating \0 character. */
            } else if (udld_packet.port_id_len == 0) {
                T_D("Invalid PORT_ID, ID = %s, len = %d", udld_packet.port_id, udld_packet.port_id_len);
                /*port_id must be non zero */
                return VTSS_RC_ERROR;
            }
            break;
        case UDLD_TLV_TYPE_ECHO:
            address_ptr = udld_frame_buffer + 4 + udld_byte_analysed;
            udld_packet.echo_len = tlv_len - 4;
            udld_packet.id_pairs =  UDLD_NTOHL(address_ptr) ;
            udld_packet.echo     = (uint8_t *)address_ptr + 4;
            T_D("echo_len = %d pairs = %u", udld_packet.echo_len, udld_packet.id_pairs);
            break;
        case UDLD_TLV_TYPE_MESSAGE_INTERVAL:
            address_ptr = udld_frame_buffer + 4 + udld_byte_analysed ;
            udld_packet.msg_interval = *address_ptr ;
            udld_packet.msg_interval_len = tlv_len - 4; ;
            T_D("MESSAGE_INTERVAL, msg_interval = %u len: %u", udld_packet.msg_interval, udld_packet.msg_interval_len);
            break;
        case UDLD_TLV_TYPE_TIMEOUT_INTERVAL:
            address_ptr = udld_frame_buffer + 4 + udld_byte_analysed ;
            udld_packet.timeout_interval = *address_ptr ;
            udld_packet.timeout_interval_len = tlv_len - 4;
            T_D("TIMEOUT_INTERVAL, timeout_interval = %u len: %u", udld_packet.timeout_interval, udld_packet.timeout_interval_len);
            break;
        case UDLD_TLV_TYPE_DEVICE_NAME:
            udld_packet.device_name = &udld_frame_buffer[4 + udld_byte_analysed] ;
            udld_packet.device_name_len = tlv_len - 4;
            T_D("DEVICE_NAME = %s, len = %d", udld_packet.device_name, udld_packet.device_name_len);
            if (udld_packet.device_name_len >= MAX_DEVICE_NAME_LENGTH) {
                T_D("Invalid DEVICE_NAME = %s, len = %d", udld_packet.device_name, udld_packet.device_name_len);
                udld_packet.device_name_len = (MAX_DEVICE_NAME_LENGTH - 1);
                /* we support max length 254, last byte used for terminating \0 character */
            }
            break;
        case UDLD_TLV_TYPE_SEQUENCE_NUMBER:
            address_ptr = udld_frame_buffer + 4 + udld_byte_analysed ;
            udld_packet.seq_num = UDLD_NTOHL(address_ptr) ;
            udld_packet.seq_num_len = tlv_len - 4 ;
            T_D("SEQUENCE_NUMBER, seq_num = %u len: %u", udld_packet.seq_num, udld_packet.seq_num_len);
            break;
        default :
            T_D("UDLD frame contained a unknown TLV (0x%X) ", tlv_type);
        }
        udld_data_ptr += tlv_len; // Point to next TLV
        udld_byte_analysed += tlv_len;
    }
    return VTSS_RC_OK;
}
static BOOL udld_is_cache_has_device_port_id(udld_port_info_struct_t *info, u8 *device_id, u16 device_id_len, u8 *port_id, u16 port_id_len)
{
    BOOL match = 0;
    T_D(" device_id: %s, port_id: %s", device_id, port_id);
    if (info) {
        if ((memcmp(info->device_id, device_id, device_id_len) == 0) &&
            (memcmp(info->port_id, port_id, port_id_len) == 0)) {
            match = 1;
        }
    }
    return match;
}

static int16_t udld_tlv_msg_generate(u8 *udld_frame, u32 port_no, udld_port_info_struct_t *remote_port_info, udld_port_info_struct_t *port_info, u8 opcode, u8 flag, udld_remote_cache_list_head_t *head_cache)
{
    uint16_t frame_len = 0, udld_offset, cs_offset, cs;

    frame_len  = udld_update_pdu_header(        &udld_frame[frame_len]);
    udld_offset = frame_len;
    frame_len += udld_update_pdu_version_opcode(&udld_frame[frame_len], opcode);
    frame_len += udld_update_pdu_flags(         &udld_frame[frame_len], flag);

    // UDLD Checksum - Set to 0x0 for now - Will be updated later
    cs_offset = frame_len;
    udld_frame[frame_len++] = 0x00;
    udld_frame[frame_len++] = 0x00;

    // Append TLVs
    frame_len += udld_append_device_id_tlv(       &udld_frame[frame_len], port_no,          remote_port_info, port_info, opcode);
    frame_len += udld_append_port_id_tlv(         &udld_frame[frame_len], port_no,          remote_port_info, port_info, opcode);
    frame_len += udld_append_echo_tlv(            &udld_frame[frame_len], port_no,          remote_port_info, port_info, opcode, head_cache);
    frame_len += udld_append_msg_interval_tlv(    &udld_frame[frame_len], remote_port_info, port_info, opcode);
    frame_len += udld_append_timeout_interval_tlv(&udld_frame[frame_len], remote_port_info, port_info, opcode);
    frame_len += udld_append_device_name_tlv(     &udld_frame[frame_len], port_no,          remote_port_info, port_info, opcode);
    frame_len += udld_append_seq_number_tlv(      &udld_frame[frame_len], remote_port_info, port_info, opcode);

    // Update length field
    frame_len = frame_len - 14; // length field does not include smac/dmac/length field
    udld_frame[12] = (frame_len >> 8) & 0xFF;
    udld_frame[13] = frame_len & 0xFF;
    frame_len = frame_len + 14;

    // Update checksum. According to RFC5171, this is an IP-like checksum with
    // the exception that the last byte of an odd-sized message is used as the
    // low 8 bits of an extra word, rather than as the high 8 bits. This is
    // controlled with the last argument to vtss_ip_checksum().
    cs = vtss_ip_checksum(&udld_frame[udld_offset], &udld_frame[frame_len] - &udld_frame[udld_offset], VTSS_IP_CHECKSUM_TYPE_UDLD);
    udld_frame[cs_offset + 0] = cs >> 8;
    udld_frame[cs_offset + 1] = cs;

    return frame_len;
}

static mesa_rc udld_flush_msg_send(u32 port_no)
{
    mesa_rc                 rc = VTSS_RC_OK;
    u8                      *udld_frame = NULL;
    u16                     frame_len = 0;
    udld_port_info_struct_t *port_info = UDLD_PORT_LOCAL_INFO_PONTER_GET(port_no);

    udld_frame = (u8 *)vtss_os_alloc_xmit(port_no, MAX_UDLD_FRAME_SIZE);
    if (udld_frame == NULL) {
        T_W("failed to allocate UDLD frame for port(%u)", port_no);
        return rc ;
    }
    memset(udld_frame, '\0', MAX_UDLD_FRAME_SIZE);
    frame_len = udld_tlv_msg_generate(udld_frame, port_no, NULL, port_info, UDLD_OPCODE_FLUSH, UDLD_FLAG_VALUE_DEFAULT, NULL);
    if (frame_len) {
        rc = vtss_os_xmit(port_no, udld_frame, frame_len);
        if (rc != VTSS_COMMON_CC_OK) {
            T_E("Error %u occured while sending out UDLD frame on port(%u)", rc, port_no);
        }

        T_I("sent flush msg on port:%u, frame_len: %u", port_no, frame_len);
    }

    return rc;
}

static mesa_rc udld_detection_window_echo_msg_send(u32 port_no, u8 flags)
{
    mesa_rc                        rc = VTSS_RC_OK;
    udld_remote_cache_list_head_t  *head_cache = NULL;
    u8                             *udld_frame = NULL;
    u16                            frame_len = 0;
    udld_port_info_struct_t        *port_info = NULL;

    T_D(" port_no: %u", port_no);

    head_cache =  UDLD_PORT_REMOTE_CACHE_LIST_PONTER_GET(port_no);
    port_info =   UDLD_PORT_LOCAL_INFO_PONTER_GET(port_no);

    while (port_info && (port_info->echo_tx < UDLD_DETECTION_WINDOW_ECHO_TX)) {
        T_D("port_no: %u, echo_tx(%u), detection_start: %u,cache_count:%u", port_no, port_info->echo_tx, port_info->detection_start, head_cache->count);
        port_info->echo_tx++;
        udld_frame = (u8 *)vtss_os_alloc_xmit(port_no, MAX_UDLD_FRAME_SIZE);
        if (udld_frame == NULL) {
            T_W("failed to allocate UDLD frame for port(%u)", port_no);
            return VTSS_RC_ERROR;
        }
        memset(udld_frame, '\0', MAX_UDLD_FRAME_SIZE);
        frame_len = udld_tlv_msg_generate(udld_frame, port_no, NULL, port_info, UDLD_OPCODE_ECHO, flags, head_cache);
        if (frame_len) {
            rc = vtss_os_xmit(port_no, udld_frame, frame_len);
            if (rc != VTSS_COMMON_CC_OK) {
                T_E("Error %u occured while sending out UDLD frame on port(%u)", rc, port_no);
            }

            T_I("sent echo msg on port:%u, echo_tx:%u , frame_len: %u", port_no, port_info->echo_tx, frame_len);
            VTSS_OS_MSLEEP(100);//make sure that this frame sent out
        }
    }

    return rc;
}

static void udld_compare_echo_tlv_and_cached_entries(const u32 port_no, udld_port_info_struct_t *port_info, udld_remote_cache_list_t *tmp, u8 cache_count)
{
    i8                           id_pairs_count = 0;
    i16                          start = 0;
    udld_echo_tlv_struct_t       *echo_tlv = NULL;

    if ((udld_compare_tlv_string((char *)port_info->device_id, (char *)udld_packet.device_id, udld_packet.device_id_len)) && (udld_compare_tlv_string((char *)port_info->port_id, (char *)udld_packet.port_id, udld_packet.port_id_len))) {
        T_D("loopback, port :%u local_devid :(%s) local_portid:(%s) pkt_devid:(%s),pkt_portid:(%s)", port_no, port_info->device_id, port_info->port_id, udld_packet.device_id, udld_packet.port_id);
        //loopback port
        udld_update_port_info_variable(port_info);
        port_info->detection_state              = VTSS_UDLD_DETECTION_STATE_LOOPBACK;
        tmp->info.detection_state               = VTSS_UDLD_DETECTION_STATE_LOOPBACK;
    } else {
        echo_tlv = udld_parse_echo_tlv_pairs(&udld_packet);
        id_pairs_count = udld_packet.id_pairs;
        if (echo_tlv != NULL && id_pairs_count > 0) {
            while (start < id_pairs_count) {
                if ((udld_compare_tlv_string((char *)port_info->device_id, (char *)echo_tlv[start].device_id, echo_tlv[start].device_id_len)) && (udld_compare_tlv_string((char *)port_info->port_id, (char *)echo_tlv[start].port_id, echo_tlv[start].port_id_len))) {
                    T_D("port_no:%u, BiDirectional link detected,id_pairs_count:%u,cache_count:%u", port_no, id_pairs_count, cache_count);
                    tmp->info.detection_state  = VTSS_UDLD_DETECTION_STATE_BI_DIRECTIONAL;
                    break;
                }
                start++;
            }
            if (tmp->info.detection_state  == VTSS_UDLD_DETECTION_STATE_UNKNOWN) {
                T_D("port_no:%u, UniDirectional link detected", port_no);
                tmp->info.detection_state  = VTSS_UDLD_DETECTION_STATE_UNI_DIRECTIONAL;
            }
            VTSS_FREE(echo_tlv);
        }
    }
}
static void udld_update_status_based_on_rx_frame(const u32 port_no)
{
    udld_remote_cache_list_t       *tmp   = NULL;
    udld_remote_cache_list_t       *prev  = NULL;
    BOOL                           found  = FALSE;
    udld_remote_cache_list_head_t  *head_cache = NULL;
    udld_port_info_struct_t        *port_info  = NULL;

    head_cache = UDLD_PORT_REMOTE_CACHE_LIST_PONTER_GET(port_no);
    port_info  = UDLD_PORT_LOCAL_INFO_PONTER_GET(port_no);

    T_I("rx_frame on port_no: %u, opcode(0x%X):%s, flag: 0x%X", port_no,
        udld_packet.opcode, udld_packet.opcode == UDLD_OPCODE_PROBE ? "UDLD_OPCODE_PROBE" :
        udld_packet.opcode == UDLD_OPCODE_ECHO ? "UDLD_OPCODE_ECHO" :
        udld_packet.opcode == UDLD_OPCODE_FLUSH ? "UDLD_OPCODE_FLUSH" : "Unknown",
        udld_packet.flag);
    if (udld_packet.opcode == UDLD_OPCODE_PROBE || udld_packet.opcode == UDLD_OPCODE_ECHO) {
        head_cache = UDLD_PORT_REMOTE_CACHE_LIST_PONTER_GET(port_no);
        port_info  = UDLD_PORT_LOCAL_INFO_PONTER_GET(port_no);

        if (head_cache->list) {
            tmp = head_cache->list;
            while (tmp) {
                if (udld_is_cache_has_device_port_id(&(tmp->info), udld_packet.device_id, udld_packet.device_id_len, udld_packet.port_id, udld_packet.port_id_len)) {
                    T_D("Found in cache, port_no:%u, device_id:%s ,port_id:%s", port_no, udld_packet.device_id, udld_packet.port_id);
                    if ((udld_packet.opcode == UDLD_OPCODE_PROBE) && (udld_packet.flag & UDLD_FLAG_RSY)) {
                        if (port_info) {
                            udld_update_port_info_variable(port_info);
                            port_info->detection_start              = TRUE;
                            port_info->proto_phase                  = VTSS_UDLD_PROTO_PHASE_ADV;
                        }
                        tmp->info.detection_state  = VTSS_UDLD_DETECTION_STATE_UNI_DIRECTIONAL;
                        if (udld_flush_msg_send(port_no) != VTSS_RC_OK) {
                            T_W("failed to send flush msg ,port_no:%u", port_no);
                        }
                        //send echo message
                        if (udld_detection_window_echo_msg_send(port_no, UDLD_FLAG_RT) != VTSS_RC_OK) {
                            T_W("failed to send echo msg ,port_no:%u", port_no);
                        }
                    } else {
                        tmp->info.cache_hold_time = UDLD_DEFAULT_CACHE_ENTRY_HOLD_TIME;
                    }
                    if ((udld_packet.device_name_len != strlen((char *)&tmp->info.device_name[0])) || memcmp(&(tmp->info.device_name[0]), &udld_packet.device_name[0], udld_packet.device_name_len)) {
                        strncpy((char *) & (tmp->info.device_name[0]), (char *)&udld_packet.device_name[0], udld_packet.device_name_len);
                        tmp->info.device_name[udld_packet.device_name_len] = '\0';
                    }
                    if (port_info) {
                        if (head_cache->count > 1) {
                            port_info->detection_state = VTSS_UDLD_DETECTION_STATE_MULTIPLE_NEIGHBOR;
                        } else {
                            port_info->detection_state = tmp->info.detection_state;
                        }
                        if ((udld_packet.opcode == UDLD_OPCODE_PROBE)
                            && (udld_packet.flag & UDLD_FLAG_RT)
                            && (udld_packet.flag & UDLD_FLAG_RSY)) {
                            port_info->probe_msg_rx++; //using for aggressive mode port shutdown
                        }
                    }
                    found = TRUE;
                    break;
                }
                prev = tmp;
                tmp = tmp->next;
            }
        }
        if (!found ) { //cache is updating when recieved new msg (echo msg as well as probe)
            T_I("Not found in cache, port_no:%u, device_id:%s ,port_id:%s", port_no, udld_packet.device_id, udld_packet.port_id);
            tmp = (udld_remote_cache_list_t *)VTSS_MALLOC(sizeof(udld_remote_cache_list_t));
            if (tmp) {
                memset(tmp, 0, sizeof(udld_remote_cache_list_t));
                memcpy(tmp->info.device_id, udld_packet.device_id, udld_packet.device_id_len);
                memcpy(tmp->info.port_id, udld_packet.port_id, udld_packet.port_id_len);
                memcpy(tmp->info.device_name, udld_packet.device_name, udld_packet.device_name_len);
                tmp->info.cache_hold_time  = UDLD_DEFAULT_CACHE_ENTRY_HOLD_TIME;
                tmp->info.msg_interval     = udld_packet.msg_interval;
                tmp->info.timeout_interval = udld_packet.timeout_interval;
                tmp->info.seq_num          = udld_packet.seq_num;
                tmp->info.detection_state  = VTSS_UDLD_DETECTION_STATE_UNI_DIRECTIONAL;
                tmp->next = NULL;
                if (head_cache->list == NULL) {
                    head_cache->list  = tmp;
                    head_cache->count = 1;
                } else {
                    if (prev) {
                        prev->next = tmp;
                        head_cache->count++;
                    } else {
                        VTSS_FREE(tmp);
                    }
                }
                if (port_info) {
                    udld_update_port_info_variable(port_info);
                    port_info->detection_start              = TRUE;
                    port_info->proto_phase                  = VTSS_UDLD_PROTO_PHASE_ADV;
                    port_info->probe_msg_rx                 = 0; //using for aggressive mode port shutdown
                }
                if ((udld_packet.opcode == UDLD_OPCODE_PROBE) && (udld_packet.flag & UDLD_FLAG_RT)) {
                    if (udld_flush_msg_send(port_no) != VTSS_RC_OK) {
                        T_W("failed to send flush msg ,port_no:%u", port_no);
                    }
                }
                if (udld_detection_window_echo_msg_send(port_no, UDLD_FLAG_RT) != VTSS_RC_OK) {
                    T_W("@@ failed to send echo msg ,port_no:%u", port_no);
                }
            }
        }
    }
    switch (udld_packet.opcode) {
    case UDLD_OPCODE_PROBE:
        break;
    case UDLD_OPCODE_FLUSH:
        udld_clear_cache_entry(port_no);
        if (port_info) {
            udld_update_port_info_variable(port_info);
            port_info->flags  = UDLD_FLAG_RT_RSY;
        }
        break;
    case UDLD_OPCODE_ECHO:
        if (head_cache->list) {
            tmp = head_cache->list;
            while (tmp) {
                if (udld_is_cache_has_device_port_id(&(tmp->info), udld_packet.device_id, udld_packet.device_id_len, udld_packet.port_id, udld_packet.port_id_len)) {
                    if (port_info->detection_start && (port_info->detection_window_timer_start < port_info->detection_window_timer_end)) {
                        udld_compare_echo_tlv_and_cached_entries(port_no, port_info, tmp, head_cache->count);
                    }
                    break;
                }
                tmp = tmp->next;
            }
        }
        break;
    default :
        T_D("Unsupported opcode: 0x%X, flag: 0x%X", udld_packet.opcode, udld_packet.flag);
    } //end switch
}

static mesa_rc udld_rx_frame_decode(u8 port_no, const u8 *const frame, u16 frm_len)
{
    const uint8_t *udld_data_ptr = &frame[22]; // Ptr to UDLD data.
    uint16_t      cs;

    vtss_clear(udld_packet);
    udld_packet.len = &frame[frm_len] - udld_data_ptr; // Calulate the length of the UDLD packet frame
    if (frm_len > MAX_UDLD_FRAME_SIZE) {
        return VTSS_UNSPECIFIED_ERROR;
    } else {
        // For the last argument, see the other use of vtss_ip_checksum() in
        // this file.
        if ((cs = vtss_ip_checksum(udld_data_ptr, udld_packet.len, VTSS_IP_CHECKSUM_TYPE_UDLD)) == 0xFFFF) {
            T_D("UDLD checksum correct (0x%04x)", cs);
        } else {
            // Oddly enough, the old implementation of this didn't return if the
            // checksum was bad, so let's keep backwards compatibility!!!
            T_D("UDLD checksum incorrect (0x%04x) for UDLD datagram of %u bytes", cs, udld_packet.len);
        }

        udld_packet.version       = (*(udld_data_ptr) & UDLD_PDU_VERSION_BITS) >> 5; // Pick out UDLD version
        udld_packet.opcode        = *(udld_data_ptr) & UDLD_PDU_OPCODE_BITS;  // Pick out opcode version
        udld_packet.flag          = *(udld_data_ptr + 1); // pick out the flag value
        udld_packet.checksum      = UDLD_NTOHS(udld_data_ptr); // Convert to 16 bits
        T_D("port_no:%u version = %d, opcode = %d, flag = %d, checksum = 0x%X, len: %u", port_no, udld_packet.version, udld_packet.opcode, udld_packet.flag, udld_packet.checksum, udld_packet.len);
        udld_data_ptr += 4; // Point to first TLV
        VTSS_RC(udld_rx_tlv_decode(udld_data_ptr, udld_packet.len - 4)); // Call decoding of the TLVs
        udld_update_status_based_on_rx_frame(port_no);
        return VTSS_RC_OK;
    }
}

static BOOL udld_rx_pkt(void  *contxt, const u8 *const frm_p,
                        const mesa_packet_rx_info_t *const rx_info)
{
    BOOL                       eat_frm = FALSE; // Allow other subscribers to receive the packet
    vtss_udld_message_t        *message = NULL;
    UDLD_CRIT_MGMT_ENTER();
    if (!udld_port_is_authorized(rx_info->port_no)) {
        UDLD_CRIT_MGMT_EXIT();
        return eat_frm;
    }
    UDLD_CRIT_MGMT_EXIT();
    if (*(frm_p + 20) == 0x01 && *(frm_p + 21) == 0x11) {
        T_D("got udld frame, port_no: %u", (rx_info->port_no - VTSS_PORT_NO_START));
        UDLD_CRIT_MGMT_ENTER();
        if (!udld_is_admin_mode_enabled(rx_info->port_no - VTSS_PORT_NO_START)) {
            UDLD_CRIT_MGMT_EXIT();
            return eat_frm;
        }
        UDLD_CRIT_MGMT_EXIT();
        if (rx_info->tag_type != MESA_TAG_TYPE_UNTAGGED) {
            //Verify that PDU is not tagged
            T_D("Tagged UDLD PDU is received on port(%u)", rx_info->port_no);
            return eat_frm;
        }
        message = (vtss_udld_message_t *)VTSS_MALLOC(sizeof(vtss_udld_message_t));
        if (message != NULL) {
            message->event_code      = VTSS_UDLD_PDU_RX_EVENT;
            message->event_on_port   = rx_info->port_no;
            message->event_data_len  = rx_info->length;
            memcpy(message->event_data, frm_p, rx_info->length);
            udld_message_post(message);
        } else {
            T_D("Unable to allocate memory to handle the received frame on port(%u)", rx_info->port_no);
        }
    }
    return eat_frm;
}
// Because the cisco slow mac frame is a special multicast frame that the chip doesn't know
// how to handle, we need to add the MAC address to the MAC table manually.
// The hash entry to the MAC table consists of both the DMAC and VLAN VID.
// Since the port VLAN ID can be changed by the user this function MUST be called
// every time the port VLAN VID changes.
void udld_add_to_mac_table(vtss_isid_t isid, mesa_port_no_t port_no,
                           const vtss_appl_vlan_port_detailed_conf_t *vlan_conf_detailed)
{
    static CapArray<mesa_mac_table_entry_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> entry;
    const mesa_mac_t              vtss_udld_slowmac = {{0x01, 0x00, 0x0C, 0xCC, 0xCC, 0xCC}};
    vtss_appl_vlan_port_conf_t    vlan_conf;
    port_iter_t                   pit;
    mesa_port_no_t                i;
    mesa_port_no_t                port_index = port_no - VTSS_PORT_NO_START;
    T_D("enter ");
    UDLD_CRIT_MGMT_ENTER();
    if (port_index < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        mesa_vid_t  pvid;   // Port VLAN ID for the port that has changed configuration
        BOOL remove_entry_from_mac_table = TRUE;
        (void)vlan_mgmt_port_conf_get(VTSS_ISID_LOCAL, port_no, &vlan_conf, VTSS_APPL_VLAN_USER_ALL, TRUE);
        pvid = vlan_conf.hybrid.pvid;
        (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            (void)vlan_mgmt_port_conf_get(VTSS_ISID_LOCAL, pit.iport, &vlan_conf, VTSS_APPL_VLAN_USER_ALL, TRUE);
            if (vlan_conf.hybrid.pvid == entry[port_index].vid_mac.vid && udld_is_admin_mode_enabled(port_no)) {
                T_D("Another port uses the same PVID (%u) - Leaving entry in mac table", entry[port_index].vid_mac.vid);
                remove_entry_from_mac_table = FALSE; // There is another port that uses the same PVID, so we can not remove the entry.
            }
        }
        // Do the remove of the entry.
        if (remove_entry_from_mac_table) {
            T_D("removing mac entry");
            (void)mesa_mac_table_del(NULL, &entry[port_index].vid_mac);
        }
        if (udld_is_admin_mode_enabled(port_no)) {
            T_D("Adding UDLD to MAC table, pvid = %u, port_no: %u", pvid, port_no);
            vtss_clear(entry);
            entry[port_index].vid_mac.vid = pvid;
            entry[port_index].vid_mac.mac = vtss_udld_slowmac;
            entry[port_index].copy_to_cpu = 1;
            entry[port_index].cpu_queue = PACKET_XTR_QU_MAC;
            entry[port_index].locked = 1;
            entry[port_index].aged = 0;
            for (i = 1; i <= (fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) - 1); i++) {
                entry[port_index].destination[i] = 0;
            }
            (void)mesa_mac_table_add(NULL, &entry[port_index]);
        }
    }
    UDLD_CRIT_MGMT_EXIT();
}

static void udld_pkt_subscribe(void)
{
    packet_rx_filter_t rx_filter;
    static void *filter_id = NULL;
    const mesa_mac_t vtss_udld_slowmac = {{0x01, 0x00, 0x0C, 0xCC, 0xCC, 0xCC}};
    T_D("Registering UDLD frames");
    packet_rx_filter_init(&rx_filter);
    rx_filter.modid = VTSS_MODULE_ID_UDLD;
    rx_filter.match = PACKET_RX_FILTER_MATCH_DMAC;
    rx_filter.cb = udld_rx_pkt; // Setup callback function
    memcpy(rx_filter.dmac, vtss_udld_slowmac.addr, sizeof(rx_filter.dmac));
    rx_filter.prio   = PACKET_RX_FILTER_PRIO_NORMAL;
    if (packet_rx_filter_register(&rx_filter, &filter_id) != VTSS_RC_OK) {
        T_D("Not possible to register for UDLD packets. UDLD will not work !");
    }
    vlan_port_conf_change_register(VTSS_MODULE_ID_UDLD, udld_add_to_mac_table, FALSE);
}
// Callback function for when a port changes state.
static void udld_port_link(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    vtss_udld_message_t       *message = NULL;
    UDLD_CRIT_MGMT_ENTER();
    if (!udld_is_admin_mode_enabled(port_no)) {
        UDLD_CRIT_MGMT_EXIT();
        return;
    }
    UDLD_CRIT_MGMT_EXIT();
    T_D("Port %u Link %u\n", port_no, status->link);
    message = (vtss_udld_message_t *)VTSS_MALLOC(sizeof(vtss_udld_message_t));
    if (message != NULL) {
        if (status->link) {
            message->event_code = VTSS_UDLD_PORT_UP_EVENT;
        } else {
            message->event_code = VTSS_UDLD_PORT_DOWN_EVENT;
        }
        message->event_on_port = port_no;
        udld_message_post(message);
    } else {
        T_E("Unable to allocate the memory to handle the received frame on port(%u)",
            port_no);
    }
    return;
}

static mesa_rc udld_message_handler(vtss_udld_message_t *event_message)
{
    u32               port_no;
    mesa_rc           rc = VTSS_RC_OK;
    if (event_message == NULL) {
        return VTSS_RC_ERROR;
    }
    port_no = event_message->event_on_port - VTSS_PORT_NO_START;
    T_D("event_code : %s, port_no: %u",
        event_message->event_code == VTSS_UDLD_PDU_RX_EVENT ? "PDU_RX" :
        event_message->event_code == VTSS_UDLD_PORT_PROTO_ENABLE_EVENT ? "PROTO_ENABLE" :
        event_message->event_code == VTSS_UDLD_PORT_PROTO_DISABLE_EVENT ? "PROTO_DISABLE" :
        event_message->event_code == VTSS_UDLD_PORT_DOWN_EVENT ? "PORT_DOWN" :
        event_message->event_code == VTSS_UDLD_PORT_UP_EVENT ? "PORT_UP" : "UNKNOWN",
        port_no);

    switch (event_message->event_code) {
    case VTSS_UDLD_PDU_RX_EVENT:
        T_D("VTSS_UDLD_PDU_RX_EVENT recieved on port : %u", event_message->event_on_port);
        rc = udld_rx_frame_decode(port_no, event_message->event_data, event_message->event_data_len);
        break;
    case VTSS_UDLD_PORT_PROTO_ENABLE_EVENT:
        T_D("PROTO_ENABLE recieved on port : %u", port_no);
        udld_port_info_default_init(port_no);
        break;
    case VTSS_UDLD_PORT_PROTO_DISABLE_EVENT:
        if (udld_flush_msg_send(port_no) != VTSS_RC_OK) {
            T_W("failed to send flush msg ,port_no:%u", port_no);
        }
        udld_clear_cache_entry (port_no);
        {
            udld_port_info_struct_t  *port_info = NULL;
            port_info = UDLD_PORT_LOCAL_INFO_PONTER_GET(port_no);
            if (port_info && port_info->admin_disable) {
                T_D("udld re-enable port:%u, mode aggressive", port_no);
                port_info->admin_disable = FALSE;
                udld_aggressive_port_disable(port_no, FALSE);
            }
            udld_update_port_info_variable(port_info);
        }
        break;
    case VTSS_UDLD_PORT_DOWN_EVENT:
        T_D("PORT_DOWN recieved on port : %u", port_no);
        udld_update_port_info_variable(UDLD_PORT_LOCAL_INFO_PONTER_GET(port_no));
        udld_clear_cache_entry (port_no);
        break;
    case VTSS_UDLD_PORT_UP_EVENT:
        T_D("PORT_UP recieved on port : %u", port_no);
        if (udld_flush_msg_send(port_no) != VTSS_RC_OK) {
            T_W("failed to send flush msg ,port_no:%u", port_no);
        }
        udld_port_info_default_init(port_no);
        break;
    default:
        T_D("unsupported event(%u) on port: %u", event_message->event_code, port_no);
    }
    return rc;
}
static void udld_client_thread(vtss_addrword_t data)
{
    vtss_udld_message_t         *event_message;
    mesa_rc                     rc = VTSS_RC_OK;
    do {
        event_message = (vtss_udld_message_t *)udld_queue.vtss_safe_queue_get();
        T_D("Get UDLD event:%u on port(%u)", event_message->event_code,
            event_message->event_on_port);

        UDLD_CRIT_CONTROL_ENTER();
        rc = udld_message_handler(event_message);
        if (rc != VTSS_RC_OK) {
            T_D(":%u occured while processing the UDLD event:%u on port(%u)",
                rc, event_message->event_code, event_message->event_on_port);
        }
        VTSS_FREE(event_message);
        UDLD_CRIT_CONTROL_EXIT();
    } while (TRUE);
}

static void udld_update_cache_hold_time(u32 port_no)
{
    T_D("port_no: %u", port_no);
    udld_remote_cache_list_head_t *head_cache = NULL;
    udld_port_info_struct_t        *port_info = NULL;
    udld_remote_cache_list_t       *tmp = NULL;
    udld_remote_cache_list_t       *nxt = NULL;
    udld_remote_cache_list_t       *prev = NULL;
    udld_remote_cache_list_t       *list = NULL;

    head_cache = UDLD_PORT_REMOTE_CACHE_LIST_PONTER_GET(port_no);
    port_info = UDLD_PORT_LOCAL_INFO_PONTER_GET(port_no);
    list = head_cache->list;

    if (list != NULL && !port_info->detection_start) {
        tmp = list;
        while (tmp != NULL) {
            T_D("device_id: %s port_id: %s cache_hold_time =%d count : %u", tmp->info.device_id, tmp->info.port_id, tmp->info.cache_hold_time, udld_port_control_info.remote_info.port[port_no].count);
            if (tmp->info.cache_hold_time > 0) {
                tmp->info.cache_hold_time--;
                prev = tmp;
                tmp  = tmp->next;
            } else {
                //delete this entry from cache table
                if (prev == NULL) {
                    head_cache->list = tmp->next;
                }
                head_cache->count--;
                nxt = tmp->next;
                if (prev) {
                    prev->next = nxt;
                }
                VTSS_FREE(tmp);
                tmp = nxt;
                if (!head_cache->count) {
                    udld_update_port_info_variable(port_info);
                    port_info->flags   = UDLD_FLAG_RT_RSY;
                }
            }
        }
    }
}
static void udld_echo_msg_detection_window_timer_update(u32 port_no)
{
    T_D(" port_no: %u", port_no);
    udld_port_info_struct_t       *port_info  = NULL ;
    port_info = UDLD_PORT_LOCAL_INFO_PONTER_GET (port_no);
    if (port_info) {
        T_D("device_id: %s port_id: %s  detection window =%u detection_start: %u", port_info->device_id, port_info->port_id, port_info->detection_window_timer_start, port_info->detection_start);

        if (port_info->detection_start && port_info->detection_window_timer_start < port_info->detection_window_timer_end) {
            port_info->detection_window_timer_start++;
        } else {
            port_info->detection_window_timer_start = 0;
            port_info->detection_window_timer_end   = UDLD_NORMAL_DETECTION_WINDOW;
            port_info->detection_start              = FALSE;
        }
    }
}
static void udld_update_port_state_if_mode_aggressive(u32 port_no)
{
    T_D("port_no: %u", port_no);
    mesa_rc                        rc = VTSS_RC_OK;
    udld_port_info_struct_t        *port_info = NULL;
    vtss_appl_udld_mode_t          mode;

    UDLD_CRIT_MGMT_ENTER();
    rc = udld_port_mode_get(port_no, &mode);
    UDLD_CRIT_MGMT_EXIT();

    if ((rc == VTSS_UDLD_RC_OK) && (mode == VTSS_APPL_UDLD_MODE_AGGRESSIVE)) {
        UDLD_CRIT_CONTROL_ENTER();
        port_info = UDLD_PORT_LOCAL_INFO_PONTER_GET(port_no);
        if ((port_info->probe_msg_rx > UDLD_PROBE_MSG_RX_THRESHOLD) || !(port_info->detection_start)) {
            T_D("detection_state: %u, port_no: %u, probe_msg_rx: %u", port_info->detection_state, port_no, port_info->probe_msg_rx);
            if ((port_info->detection_state == VTSS_UDLD_DETECTION_STATE_UNI_DIRECTIONAL) ||
                (port_info->detection_state == VTSS_UDLD_DETECTION_STATE_LOOPBACK)) {
                T_W("UDLD enabled port:%u admin shut down, uni directionl link detected", port_no + 1);
                port_info->admin_disable = TRUE;
                udld_aggressive_port_disable(port_no, TRUE);
            }
        }
        UDLD_CRIT_CONTROL_EXIT();
    }
}

static mesa_rc udld_periodic_probe_msg_send(u32 port_no)
{
    mesa_rc                        rc = VTSS_RC_OK;
    udld_remote_cache_list_head_t  *head_cache = NULL;
    udld_port_info_struct_t        *port_info = NULL;
    u8                             *udld_frame = NULL;
    u16                            frame_len = 0;
    u32                            conf_timer;
    BOOL                           frame_transmit_needed = FALSE;

    T_D("port_no: %u", port_no);

    UDLD_CRIT_CONTROL_ENTER();
    head_cache =  UDLD_PORT_REMOTE_CACHE_LIST_PONTER_GET(port_no);
    port_info  =  UDLD_PORT_LOCAL_INFO_PONTER_GET(port_no);
    if (port_info != NULL && !(port_info->detection_start)) {
        if (port_info->probe_msg_interval == 0) {
            if (port_info->detection_state != VTSS_UDLD_DETECTION_STATE_UNKNOWN) {
                if (udld_port_probe_msg_interval_get(port_no, &conf_timer) == VTSS_UDLD_RC_OK) {
                    //currently we are not supporting message time interval tlv
                    //port_info->probe_msg_interval = conf_timer;
                    port_info->probe_msg_interval = UDLD_DEFAULT_MESSAGE_INTERVAL;
                } else {
                    port_info->probe_msg_interval = UDLD_DEFAULT_MESSAGE_INTERVAL_LINKUP;
                }
            } else {
                port_info->probe_msg_interval = UDLD_DEFAULT_MESSAGE_INTERVAL_LINKUP;
            }

            udld_frame = (u8 *)vtss_os_alloc_xmit(port_no, MAX_UDLD_FRAME_SIZE);
            if (udld_frame == NULL) {
                T_W("Error failed to allocate UDLD frame for port(%u)", port_no);
                UDLD_CRIT_CONTROL_EXIT();
                return VTSS_RC_ERROR; //It's should be fatal
            }
            memset(udld_frame, '\0', MAX_UDLD_FRAME_SIZE);
            frame_len = udld_tlv_msg_generate(udld_frame, port_no, NULL, port_info, UDLD_OPCODE_PROBE, port_info->flags, head_cache);
            if (frame_len) {
                frame_transmit_needed = TRUE;
            }
        }
        port_info->probe_msg_interval--;
    }

    UDLD_CRIT_CONTROL_EXIT();

    if (frame_transmit_needed) {
        rc = vtss_os_xmit(port_no, udld_frame, frame_len);
        if (rc != VTSS_COMMON_CC_OK) {
            T_E("Error %u occured while sending out UDLD frame on port(%u)", rc, port_no);
        }

        T_I("sent probe msg on port:%u, interval:%u, frame_len:%u", port_no, port_info->probe_msg_interval, frame_len);
    }

    return rc;
}

static void udld_control_timer_thread(vtss_addrword_t data)
{
    BOOL                        is_up = FALSE;
    u32                         port_no = 0;
    system_conf_t               sys_conf;
    udld_port_info_struct_t     *port_info = NULL;
    char                        device_name[MAX_DEVICE_NAME_LENGTH];

    while (TRUE) {
        for (port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            UDLD_CRIT_MGMT_ENTER();
            if ((udld_port_link_up_status_get(port_no, &is_up) == VTSS_UDLD_RC_OK)
                && !is_up) {
                UDLD_CRIT_MGMT_EXIT();
                continue;
            }
            UDLD_CRIT_MGMT_EXIT();

            UDLD_CRIT_MGMT_ENTER();
            if (!udld_is_admin_mode_enabled (port_no)) {
                UDLD_CRIT_MGMT_EXIT();
                continue;
            }
            UDLD_CRIT_MGMT_EXIT();
            udld_update_port_state_if_mode_aggressive(port_no);
            if (udld_periodic_probe_msg_send(port_no) != VTSS_RC_OK) {
                T_W("failed to send periodic probe msg on port: %u", port_no);
                continue;
            }
            UDLD_CRIT_CONTROL_ENTER();
            udld_update_cache_hold_time(port_no);
            UDLD_CRIT_CONTROL_EXIT();

            UDLD_CRIT_CONTROL_ENTER();
            udld_echo_msg_detection_window_timer_update(port_no);
            UDLD_CRIT_CONTROL_EXIT();
        }
        if (system_conf_has_changed && msg_switch_is_primary()) {
            if (system_get_config(&sys_conf) != VTSS_RC_OK) {
                strcpy(&device_name[0], "Unknown");
            } else {
                misc_strncpyz(&device_name[0], &sys_conf.sys_name[0], MAX_DEVICE_NAME_LENGTH);
            }
            UDLD_CRIT_CONTROL_ENTER();
            for (port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
                port_info = UDLD_PORT_LOCAL_INFO_PONTER_GET(port_no);
                strcpy((char *)&port_info->device_name[0], &device_name[0]);
            }
            system_conf_has_changed = 0;
            UDLD_CRIT_CONTROL_EXIT();
        }
        VTSS_OS_MSLEEP(1000);
    }
}

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void Udld_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_udld_json_init(void);
#endif
extern "C" int udld_icli_cmd_register();

/* Initialize module */
mesa_rc udld_init(vtss_init_data_t *data)
{
    mesa_rc                vtssrc = VTSS_RC_OK;
    u32                    port_no = 0;
    vtss_isid_t            isid = data->isid;
    switch (data->cmd) {
    case INIT_CMD_INIT:
        // Initialize critical regions
        critd_init(&udld_crit_control, "udld.control", VTSS_MODULE_ID_UDLD, CRITD_TYPE_MUTEX);

        UDLD_CRIT_CONTROL_ENTER();

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        Udld_mib_init(); /* Register our private mib */
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_udld_json_init();
#endif
        udld_icli_cmd_register();

        UDLD_CRIT_CONTROL_EXIT();

        critd_init(&udld_crit_mgmt, "udld.mgmt", VTSS_MODULE_ID_UDLD, CRITD_TYPE_MUTEX);
        T_D("enter, cmd=INIT");
        break;

    case INIT_CMD_START:
        T_D("enter, cmd=START");
        udld_pkt_subscribe(); // Subscribe for UDLD packets
        (void)port_change_register(VTSS_MODULE_ID_UDLD, udld_port_link);// Prepare callback function for link up/down for ports
#ifdef VTSS_SW_OPTION_ICFG
        vtssrc = udld_icfg_init();
        if (vtssrc != VTSS_RC_OK) {
            T_D("fail to init udld icfg registration, rc = %s", error_txt(vtssrc));
        }
#endif
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           udld_control_timer_thread,
                           0,
                           "UDLD TIMER",
                           nullptr,
                           0,
                           &udld_control_timer_thread_handle,
                           &udld_control_timer_thread_block);

        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           udld_client_thread,
                           0,
                           "UDLD CLIENT",
                           nullptr,
                           0,
                           &udld_client_thread_handle,
                           &udld_client_thread_block);

        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF");
        //Construct the UDLD configurations with factory defaults
        if (isid == VTSS_ISID_LOCAL) {
            for (port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
                udld_port_conf_init(port_no);
                udld_port_info_default_init(port_no);
                udld_clear_cache_entry(port_no);
            }
        }
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");
        for (port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            udld_port_conf_init(port_no);
            udld_port_info_default_init(port_no);
        }

        break;

    default:
        break;
    }

    return vtssrc;
}
