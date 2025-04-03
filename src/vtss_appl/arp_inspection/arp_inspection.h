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

#ifndef _VTSS_ARP_INSPECTION_H_
#define _VTSS_ARP_INSPECTION_H_

#include "arp_inspection_api.h"
#include "vtss_module_id.h"
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_ARP_INSPECTION
#include <vtss_trace_api.h>

/* ARP Inspection ACE IDs */
#define ARP_INSPECTION_ACE_ID   1

/* ================================================================= *
 *  ARP header
 * ================================================================= */
typedef struct {
    vtss_mac_t dmac;        /* destination hardware address */
    vtss_mac_t smac;        /* source hardware address */
    uint16_t   type;        /* Ethertype */
} __attribute__((packed)) vtss_eth_header;

typedef struct {
    uint16_t hrd;    // Hardware type (Ethernet = 1)
    uint16_t pro;    // Protocol type (IPv4 = 0x0800)
    uint8_t  hln;    // Hardware size (sizeof(MAC address = 6)
    uint8_t  pln;    // Protocol size (sizeof(IPv4 address = 4)
    uint16_t op;     // Opcode (request = 1, reply = 2, etc.)
    uint8_t  sha[6]; // Sender MAC address
    uint32_t spa;    // Sender IPv4 address
    uint8_t  tha[6]; // Target MAC address
    uint32_t tpa;    // Target IPv4 address
} __attribute__((packed)) vtss_arp_header;

/* ================================================================= *
 *  ARP_INSPECTION stack messages
 * ================================================================= */

/* ARP_INSPECTION messages IDs */
typedef enum {
    ARP_INSPECTION_MSG_ID_ARP_INSPECTION_CONF_SET_REQ,          /* ARP_INSPECTION configuration set request (no reply) */
    ARP_INSPECTION_MSG_ID_FRAME_RX_IND,                         /* Frame receive indication */
    ARP_INSPECTION_MSG_ID_FRAME_TX_REQ                          /* Frame transmit request */
} arp_inspection_msg_id_t;

/* Request message */
typedef struct {
    /* Message ID */
    arp_inspection_msg_id_t    msg_id;

    /* Request data, depending on message ID */
    union {
        /* ARP_INSPECTION_MSG_ID_ARP_INSPECTION_CONF_SET_REQ */
        struct {
            //arp_inspection_conf_t  conf;
            arp_inspection_stacking_conf_t  conf;
        } conf_set;

        /* ARP_INSPECTION_MSG_ID_FRAME_RX_IND */
        struct {
            size_t              len;
            unsigned long       vid;
            unsigned long       port_no;
        } rx_ind;
        /* ARP_INSPECTION_MSG_ID_FRAME_TX_REQ */
        struct {
            size_t              len;
            unsigned long       vid;
            unsigned long       isid;
            BOOL                port_list[VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD]; // egress port list
        } tx_req;
    } req;
} arp_inspection_msg_req_t;

/* ARP_INSPECTION message buffer */
typedef struct {
    vtss_sem_t *sem;           /* Semaphore */
    void       *msg;           /* Message */
} arp_inspection_msg_buf_t;


/* ================================================================= *
 *  ARP_INSPECTION global structure
 * ================================================================= */

/* ARP_INSPECTION global structure */
typedef struct {
    critd_t                         crit;
    critd_t                         bip_crit;
    arp_inspection_conf_t           arp_inspection_conf;
    arp_inspection_entry_t          arp_inspection_dynamic_entry[ARP_INSPECTION_MAX_ENTRY_CNT];

    /* Request buffer and semaphore */
    struct {
        vtss_sem_t                  sem;
        arp_inspection_msg_req_t    msg;
    } request;

} arp_inspection_global_t;

#endif /* _VTSS_ARP_INSPECTION_H_ */

