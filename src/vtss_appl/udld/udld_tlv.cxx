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

#include "conf_api.h"
#include "packet_api.h"
#include "main_types.h"
#include "misc_api.h"
#include "netdb.h"
#include "microchip/ethernet/switch/api.h"
#include "sysutil_api.h"
#include "microchip/ethernet/switch/api.h"
#include "control_api.h"
#include "udld_api.h"
#include "udld.h"
#include "udld_trace.h"
#include "udld_tlv.h"

u8 const VTSS_UDLD_MACADDR[] = { 0x01, 0x00, 0x0C, 0xCC, 0xCC, 0xCC };

udld_echo_tlv_struct_t *udld_parse_echo_tlv_pairs(udld_packet_t *udld_packet)
{
    udld_echo_tlv_struct_t    *echo_tlv = NULL;
    u8                        *address_ptr = NULL;
    u16                       id_len = 0;
    i32                       start = 0;
    i8                        pairs = 0;

    if (udld_packet->id_pairs > 0) {
        pairs = udld_packet->id_pairs;
        echo_tlv = (udld_echo_tlv_struct_t *)VTSS_MALLOC(sizeof(udld_echo_tlv_struct_t) * pairs);
        if (echo_tlv) {
            memset(echo_tlv, '\0', sizeof(udld_echo_tlv_struct_t) * (udld_packet->id_pairs));
            address_ptr =  udld_packet->echo;
            while (pairs > 0) {
                id_len  = UDLD_NTOHS(address_ptr) ;
                if (id_len > 0) {
                    *(&echo_tlv[start].device_id_len) = id_len;
                    address_ptr = address_ptr + 2;
                    memcpy(&echo_tlv[start].device_id[0], address_ptr, id_len);
                    address_ptr = address_ptr + id_len;
                    T_D("id_len:%u device_id_len: %u", id_len, echo_tlv[start].device_id_len);

                }
                id_len  = UDLD_NTOHS(address_ptr) ;
                if (id_len > 0) {
                    *(&echo_tlv[start].port_id_len) = id_len;
                    address_ptr = address_ptr + 2;
                    memcpy(&echo_tlv[start].port_id[0], address_ptr, id_len);
                    address_ptr = address_ptr + id_len;
                    T_D("id_len:%u port_id_len: %u", id_len, echo_tlv[start].port_id_len);
                }
                start++;
                pairs--;
            }
        }
    }
    return echo_tlv;
}

BOOL udld_compare_tlv_string(char *str1, char *str2, u16 len)
{
    BOOL match = 0;
    T_D("str1: %s, str2: %s, len: %u", str1, str2, len);
    if (memcmp(str1, str2, len) == 0) {
        match = 1;
    }
    return match;
}

void udld_mac_addr2str(const char *mac_addr, char *str)
{
    sprintf(str, "%02X-%02X-%02X-%02X-%02X-%02X",
            (u8)mac_addr[0], (u8)mac_addr[1], (u8)mac_addr[2], (u8)mac_addr[3], (u8)mac_addr[4], (u8)mac_addr[5]);
}

// The Chassis ID we are using for our BIST test.
void udld_get_chassis_id_str(char *string_ptr)
{
    // We uses our MAC address as device/chassis ID
    vtss_common_macaddr_t mac_addr;
    vtss_os_get_systemmac(&mac_addr);
    udld_mac_addr2str((char *)&mac_addr.macaddr[0], string_ptr);
}

u16 udld_update_pdu_header(u8 *udld_frame)
{
    vtss_common_macaddr_t mac_addr;
    vtss_os_get_systemmac(&mac_addr);
    u16 frame_len = 0;

    /* fill in DA, SA */
    udld_frame[frame_len++] = 0x01;
    udld_frame[frame_len++] = 0x00;
    udld_frame[frame_len++] = 0x0C;
    udld_frame[frame_len++] = 0xCC;
    udld_frame[frame_len++] = 0xCC;
    udld_frame[frame_len++] = 0xCC;

    memcpy(&udld_frame[6], mac_addr.macaddr, VTSS_COMMON_MACADDR_SIZE);
    frame_len += 6;

    // Frame length set to 0 for now. Will be updated later on.
    udld_frame[frame_len++] = 0x00;
    udld_frame[frame_len++] = 0x00;

    udld_frame[frame_len++] = 0xAA; // DSAP
    udld_frame[frame_len++] = 0xAA; // SSAP
    udld_frame[frame_len++] = 0x03; // Control Field

    udld_frame[frame_len++] = 0x00; // Organization code
    udld_frame[frame_len++] = 0x00; // Organization code
    udld_frame[frame_len++] = 0x0C; // Organization code

    udld_frame[frame_len++] = 0x01; // PID
    udld_frame[frame_len++] = 0x11; // PID

    return frame_len;
}

u16 udld_update_pdu_version_opcode(u8 *udld_frame, u8 opcode)
{
    udld_frame[0] = UDLD_PDU_VERSION_DEFAULT | opcode;
    return 1;
}

u16 udld_update_pdu_flags(u8 *udld_frame, u8 flag)
{
    udld_frame[0] = flag;
    return 1;
}

u16 udld_append_device_id_tlv(u8 *udld_frame, u32 port_no, udld_port_info_struct_t *info, udld_port_info_struct_t *port_info, u8 opcode)
{
    u16 frame_len = 0;
    u16 device_id_len = 0;
    u16 total_len = 0;

    udld_frame[frame_len++] = 0x00;
    udld_frame[frame_len++] = UDLD_TLV_TYPE_DEVICE_ID;

    device_id_len = strlen((char *)port_info->device_id);
    total_len = device_id_len + UDLD_TLV_SIZE_TYPE_PLUS_LENGTH;
    udld_frame[frame_len++] = (total_len >> 8) & 0x00FF;
    udld_frame[frame_len++] = total_len & 0x00FF;
    memcpy(&udld_frame[frame_len], port_info->device_id, device_id_len);
    frame_len += device_id_len;
    T_D("device_id: %s device_id_len: %u", port_info->device_id, device_id_len);
    return frame_len;
}

u16 udld_append_port_id_tlv(u8 *udld_frame, u32 port_no, udld_port_info_struct_t *remote_info, udld_port_info_struct_t *port_info, u8 opcode)
{
    u16 frame_len = 0;
    u8 lport_id_len = 0;
    u16 total_len = 0;

    udld_frame[frame_len++] = 0x00;
    udld_frame[frame_len++] = UDLD_TLV_TYPE_PORT_ID;

    lport_id_len = strlen((char *)&port_info->port_id[0]);
    total_len = lport_id_len + UDLD_TLV_SIZE_TYPE_PLUS_LENGTH;

    udld_frame[frame_len++] = (total_len >> 8) & 0x00FF;
    udld_frame[frame_len++] = total_len & 0x00FF;
    memcpy(&udld_frame[frame_len], &port_info->port_id[0], lport_id_len);
    frame_len += lport_id_len;
    T_D("port_id: %s lport_id_len: %u", port_info->port_id, lport_id_len);
    return frame_len;
}

u16 udld_append_echo_tlv(u8 *udld_frame, u32 port_no,  udld_port_info_struct_t *remote_info, udld_port_info_struct_t *port_info, u8 opcode, udld_remote_cache_list_head_t *head_cache)
{
    u16 frame_len      = 0;
    u8 rdevice_id_len  = 0;
    u8 rport_id_len    = 0;
    u16 total_len      = 0;
    u8   count         = 0;
    udld_remote_cache_list_t     *list_tmp = NULL;
    udld_port_info_struct_t      *info_tmp = NULL;

    if (head_cache != NULL) {
        list_tmp = head_cache->list;
        count = head_cache->count;
    }
    udld_frame[frame_len++] = 0x00; /*[frame_len]= 0 */
    udld_frame[frame_len++] = UDLD_TLV_TYPE_ECHO; /*[frame_len]= 1*/

    if ((opcode == UDLD_OPCODE_FLUSH) || count == 0) {
        T_D("@@@ count :%u", count);
        udld_frame[frame_len++] = 0x00; /*[frame_len]= 2*/
        udld_frame[frame_len++] = UDLD_TLV_SIZE_TYPE_PLUS_LENGTH + UDLD_TLV_SIZE_TYPE_PLUS_LENGTH; /*[frame_len]= 3*/
        udld_frame[frame_len++] = 0x00;
        udld_frame[frame_len++] = 0x00;
        udld_frame[frame_len++] = 0x00;
        udld_frame[frame_len++] = 0x00; //number of pairs device id/port id
    } else {
        T_D("#### count :%u", count);
        udld_frame[frame_len++] = 0x00; /*[frame_len]= 2*/
        udld_frame[frame_len++] = UDLD_TLV_SIZE_TYPE_PLUS_LENGTH + UDLD_TLV_SIZE_TYPE_PLUS_LENGTH; /*[frame_len]= 3*/
        udld_frame[frame_len++] = 0x00;
        udld_frame[frame_len++] = 0x00;
        udld_frame[frame_len++] = 0x00;
        udld_frame[frame_len++] = count;

        while (list_tmp != NULL && count > 0) {
            T_D("list_tmp != NULL count :%u", count);
            info_tmp = &list_tmp->info;
            udld_frame[frame_len++] = 0x00;

            rdevice_id_len = strlen((char *)&info_tmp->device_id[0]);
            udld_frame[frame_len++] = rdevice_id_len;

            memcpy(&udld_frame[frame_len], info_tmp->device_id, rdevice_id_len);
            frame_len += rdevice_id_len;

            udld_frame[frame_len++]  = 0x00;

            rport_id_len = strlen((char *)&info_tmp->port_id[0]);
            udld_frame[frame_len++]  = rport_id_len;

            memcpy(&udld_frame[frame_len], info_tmp->port_id, rport_id_len);
            frame_len += rport_id_len;

            total_len = total_len + rport_id_len + rdevice_id_len + 2 + 2;

            list_tmp = list_tmp->next;
            count--;
        }
        total_len = total_len + UDLD_TLV_SIZE_TYPE_PLUS_LENGTH + 4 ;
        udld_frame[2] = (total_len >> 8) & 0x00FF;
        udld_frame[3] = total_len & 0x00FF;
    }
    T_D("ldevice_id:(%s), lport_id:(%s)", port_info->device_id, port_info->port_id);
    return frame_len;
}

u16 udld_append_msg_interval_tlv (u8 *udld_frame, udld_port_info_struct_t *remote_info, udld_port_info_struct_t *port_info, u8 opcode)
{
    u16 frame_len = 0;
    udld_frame[frame_len++] = 0x00;
    udld_frame[frame_len++] = UDLD_TLV_TYPE_MESSAGE_INTERVAL;

    udld_frame[frame_len++] = 0x00;
    udld_frame[frame_len++] = 1 + UDLD_TLV_SIZE_TYPE_PLUS_LENGTH; // TLV Length.
    udld_frame[frame_len++] = port_info->msg_interval;
    return frame_len;
}

u16 udld_append_timeout_interval_tlv(u8 *udld_frame, udld_port_info_struct_t *remote_info, udld_port_info_struct_t *port_info, u8 opcode)
{
    u16 frame_len = 0;
    udld_frame[frame_len++] = 0x00;
    udld_frame[frame_len++] = UDLD_TLV_TYPE_TIMEOUT_INTERVAL;

    udld_frame[frame_len++] = 0x00;
    udld_frame[frame_len++] = 1 + UDLD_TLV_SIZE_TYPE_PLUS_LENGTH; // TLV Length.

    udld_frame[frame_len++] = port_info->timeout_interval;

    return frame_len;
}

u16 udld_append_seq_number_tlv(u8 *udld_frame, udld_port_info_struct_t *remote_info, udld_port_info_struct_t *port_info, u8 opcode)
{
    u16 frame_len = 0;
    u32 seq_num_h = 1;
    udld_frame[frame_len++] = 0x00;
    udld_frame[frame_len++] = UDLD_TLV_TYPE_SEQUENCE_NUMBER;

    udld_frame[frame_len++] = 0x00;
    udld_frame[frame_len++] = UDLD_TLV_SIZE_SEQUENCE_NUMBER + UDLD_TLV_SIZE_TYPE_PLUS_LENGTH; // TLV Length.

    seq_num_h = HOST2NETL(seq_num_h);
    memcpy((udld_frame + frame_len), &seq_num_h, 4);

    if ((opcode == UDLD_OPCODE_PROBE) && (port_info != NULL)) {
        seq_num_h = HOST2NETL(port_info->seq_num);
        memcpy((udld_frame + frame_len), &seq_num_h, 4);
        T_D("seq_num: %u seq_num_h: %u", port_info->seq_num, seq_num_h);
    }

    if ((opcode == UDLD_OPCODE_ECHO) && (remote_info != NULL)) {
        seq_num_h = HOST2NETL(remote_info->seq_num);
        memcpy((udld_frame + frame_len), &seq_num_h, 4);
    }
    frame_len = frame_len + 4;
    return frame_len;
}

u16 udld_append_device_name_tlv(u8 *udld_frame, u32 port_no, udld_port_info_struct_t *remote_info, udld_port_info_struct_t *port_info, u8 opcode)
{
    u16 frame_len = 0;
    u16 device_name_len = 0;
    u16 total_len = 0;
    udld_frame[frame_len++] = 0x00;
    udld_frame[frame_len++] = UDLD_TLV_TYPE_DEVICE_NAME;
    device_name_len = strlen((char *)&port_info->device_name[0]);
    total_len = device_name_len + UDLD_TLV_SIZE_TYPE_PLUS_LENGTH;
    udld_frame[frame_len++] = (total_len >> 8) & 0x00FF;
    udld_frame[frame_len++] = total_len & 0x00FF;
    memcpy(&udld_frame[frame_len], &port_info->device_name[0], device_name_len);
    frame_len += device_name_len;
    T_D("device_name: %s device_name_len: %u, total_len: %u, frame_len:%u", port_info->device_name, device_name_len, total_len, frame_len);
    return frame_len;
}

