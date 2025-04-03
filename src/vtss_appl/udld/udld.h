/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _UDLD_H_
#define _UDLD_H_

#include "vtss_common_os.h"
#include "udld_api.h"
#include "vlan_api.h"

#define MAX_UDLD_FRAME_SIZE               1518

//version
#define UDLD_PDU_VERSION_BITS             0xE0  //3 bit
#define UDLD_PDU_VERSION_DEFAULT          0x20  //default version 1
//Opcode
#define UDLD_PDU_OPCODE_BITS              0x1F  //5 bit
#define UDLD_OPCODE_PROBE                 0x01
#define UDLD_OPCODE_ECHO                  0x02
#define UDLD_OPCODE_FLUSH                 0x03
#define UDLD_OPCODE_NONE                  0x00


//flags
#define UDLD_FLAG_RT                      0x01 //0th bit recommended timeout flag
#define UDLD_FLAG_RSY                     0x02 //1st bit re-synch flag
#define UDLD_FLAG_RT_RSY                  0x03
#define UDLD_FLAG_VALUE_DEFAULT           0x00

// UDLD TLV types.
#define UDLD_TLV_TYPE_DEVICE_ID           0x0001
#define UDLD_TLV_TYPE_PORT_ID             0x0002
#define UDLD_TLV_TYPE_ECHO                0x0003
#define UDLD_TLV_TYPE_MESSAGE_INTERVAL    0x0004
#define UDLD_TLV_TYPE_TIMEOUT_INTERVAL    0x0005
#define UDLD_TLV_TYPE_DEVICE_NAME         0x0006
#define UDLD_TLV_TYPE_SEQUENCE_NUMBER     0x0007
#define UDLD_TLV_TYPE_RESERVED            0x0008 //unknown tlv,skipped parsing


#define UDLD_DEFAULT_TIMEOUT_INTERVAL                       5  //in sec
#define UDLD_DEFAULT_MESSAGE_INTERVAL_LINKUP                1  //in sec
#define UDLD_MESSAGE_INTERVAL_LINK_STATE_DETECTED           14 //in sec

#define UDLD_TLV_SIZE_MESSAGE_INTERVAL     1 //1 byte
#define UDLD_TLV_SIZE_TIMEOUT_INTERVAL     1 //1 byte
#define UDLD_TLV_SIZE_SEQUENCE_NUMBER      4 //4 byte unsigned
#define UDLD_TLV_SIZE_TYPE_PLUS_LENGTH     4 // 4 byte
#define UDLD_DEFAULT_CACHE_ENTRY_HOLD_TIME 15 //sec

#define UDLD_DETECTION_WINDOW_ECHO_TX      3 //sec

#define UDLD_NORMAL_DETECTION_WINDOW       10//sec
#define UDLD_DEFAULT_SEQ_NUM               1
#define UDLD_PROBE_MSG_RX_THRESHOLD        3

// Conversion from network formart to out host format.
#define UDLD_NTOHS(data_ptr)  ((*data_ptr << 8) + (*(data_ptr+1)))
#define UDLD_NTOHL(data_ptr)  ((*data_ptr << 24) + (*(data_ptr+1) << 16) + (*(data_ptr+2) << 8) + (*(data_ptr+3)))

/*
    ************************
    protocol data structure
    ************************
    *****************
*/

typedef struct udld_port_info_struct_t {
    uint8_t                      device_id[MAX_DEVICE_ID_LENGTH];
    uint8_t                      port_id[MAX_PORT_ID_LENGTH];
    uint8_t                      device_name[MAX_DEVICE_NAME_LENGTH];
    uint8_t                      msg_interval;
    uint8_t                      timeout_interval;
    uint32_t                     seq_num;
    uint8_t                      probe_msg_interval;
    uint8_t                      flags;
    uint8_t                      cache_hold_time;
    uint8_t                      detection_window_timer_start;
    uint8_t                      detection_window_timer_end;
    uint8_t                      detection_start;
    uint8_t                      echo_tx;
    uint8_t                      admin_disable;
    vtss_udld_detection_state_t  detection_state;
    vtss_udld_proto_phase_t      proto_phase;
    uint8_t                      probe_msg_rx;//using for aggressive mode port shutdown
} udld_port_info_struct_t;

typedef struct udld_remote_cache_list_t {
    udld_port_info_struct_t          info;
    struct udld_remote_cache_list_t  *next;
} udld_remote_cache_list_t;

typedef struct udld_remote_cache_list_head_t {
    struct udld_remote_cache_list_t     *list;
    uint8_t                              count;
} udld_remote_cache_list_head_t;

typedef struct {
    CapArray<udld_remote_cache_list_head_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port;
} udld_remote_cache_info_t;

typedef struct {
    CapArray<udld_port_info_struct_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port;
} udld_local_port_info_t;

typedef struct {
    udld_remote_cache_info_t remote_info;
    udld_local_port_info_t   local_info;
} udld_port_control_info_t;

/* UDLD Module Events data structures*/
typedef enum {
    VTSS_UDLD_NEW_NEIGHBOR_DISC_EVENT,
    VTSS_UDLD_NEIGHBOR_STATE_CHANGE_EVENT,
    VTSS_UDLD_NEIGHBOR_RESYNC_EVENT,
    VTSS_UDLD_PORT_PROTO_ENABLE_EVENT,
    VTSS_UDLD_PORT_PROTO_DISABLE_EVENT,
    VTSS_UDLD_PORT_UP_EVENT,
    VTSS_UDLD_PORT_DOWN_EVENT,
    VTSS_UDLD_PDU_RX_EVENT,
    VTSS_UDLD_PDU_TX_EVENT
} vtss_udld_events_t;

typedef struct udld_message {
    vtss_udld_events_t         event_code;
    u32                        event_on_port;
    u16                        event_data_len;
    u8                         event_data[MAX_UDLD_FRAME_SIZE];
} vtss_udld_message_t;

void udld_add_to_mac_table(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_vlan_port_detailed_conf_t *vlan_conf);

#endif
