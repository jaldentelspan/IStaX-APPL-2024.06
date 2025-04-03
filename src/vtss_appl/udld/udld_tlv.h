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

#ifndef _UDLD_TLV_H_
#define _UDLD_TLV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "udld.h"

typedef struct {
    u8    device_id[MAX_DEVICE_ID_LENGTH];
    u16   device_id_len;
    u8    port_id[MAX_PORT_ID_LENGTH];
    u16   port_id_len;
} udld_echo_tlv_struct_t;

typedef struct udld_echo_tlv_list_t {
    udld_echo_tlv_struct_t          echo;
    struct udld_echo_tlv_list_t     *next;
} udld_echo_tlv_list_t;

typedef struct  {
    uint     len;
    u8       version;
    u8       opcode;
    u8       flag;
    u16      checksum;
    u16      device_id_len;
    u8       *device_id;
    u16      port_id_len;
    u8       *port_id;
    u8       echo_len;
    u8       *echo;
    int8_t   id_pairs;
    u8       msg_interval;
    u8       msg_interval_len;
    u8       timeout_interval;
    u8       timeout_interval_len;
    u16      device_name_len;
    u8       *device_name;
    u32      seq_num;
    u8       seq_num_len;
} udld_packet_t;

udld_echo_tlv_struct_t *udld_parse_echo_tlv_pairs(udld_packet_t *udld_packet);

BOOL udld_compare_tlv_string(char *str1, char *str2, u16 len);

void udld_get_chassis_id_str (char *string_ptr);

u16 udld_update_pdu_flags (u8 *udld_frame, u8 flag);

u16 udld_update_pdu_version_opcode (u8 *udld_frame, u8 opcode);

u16 udld_update_pdu_header (u8 *udld_frame);

u16 udld_append_device_id_tlv(u8 *udld_frame, u32 port_no, udld_port_info_struct_t *info,  udld_port_info_struct_t  *port_info, u8 opcode);

u16 udld_append_port_id_tlv (u8 *udld_frame, u32 port_no, udld_port_info_struct_t *info, udld_port_info_struct_t  *port_info, u8 opcode);

u16 udld_append_echo_tlv (u8 *udld_frame, u32 port_no,  udld_port_info_struct_t *info, udld_port_info_struct_t  *port_info, u8 opcode, udld_remote_cache_list_head_t *head_cache);

u16 udld_append_msg_interval_tlv (u8 *udld_frame, udld_port_info_struct_t *info, udld_port_info_struct_t  *port_info, u8 opcode);

u16 udld_append_timeout_interval_tlv (u8 *udld_frame, udld_port_info_struct_t *info, udld_port_info_struct_t  *port_info, u8 opcode);

u16 udld_append_seq_number_tlv (u8 *udld_frame, udld_port_info_struct_t *info, udld_port_info_struct_t  *port_info, u8 opcode);

u16 udld_append_device_name_tlv (u8 *udld_frame, u32 port_no, udld_port_info_struct_t *info, udld_port_info_struct_t  *port_info, u8 opcode);

#ifdef __cplusplus
}
#endif

#endif /* _UDLD_TLV_H_ */

