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

#ifndef _VTSS_MAC_API_H_
#define _VTSS_MAC_API_H_

#include "microchip/ethernet/switch/api.h"
#include <vtss/appl/mac.h>
#include "vlan_api.h"

/* MAC module defines */
#define MAC_AGE_TIME_DISABLE       0
#define MAC_AGE_TIME_MIN           10
#define MAC_AGE_TIME_MAX           1000000
#define MAC_AGE_TIME_DEFAULT       300
#define MAC_LEARN_MAX              VTSS_MAC_ADDRS
#define MAC_ADDR_NON_VOLATILE_MAX  64
#define MAC_ALL_VLANS              4095

// Perhaps we could go down to 0 here, if no other module needs it.
#define MAC_ADDR_VOLATILE_MAX   64

#ifdef __cplusplus
extern "C" {
#endif

/* Mac API error codes (mesa_rc) */
enum {
    MAC_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_MAC),  /* Generic error code */
    MAC_ERROR_MAC_RESERVED,      /* MAC address is reserved */
    MAC_ERROR_REG_TABLE_FULL,    /* Registration table full */
    MAC_ERROR_REQ_TIMEOUT,       /* Timeout on message request */
    MAC_ERROR_STACK_STATE,       /* Illegal primary/secopndary switch state */
    MAC_ERROR_MAC_EXIST,         /* MAC address already exists */
    MAC_ERROR_MAC_SYSTEM_EXIST,  /* MAC address exists and is a system address */
    MAC_ERROR_MAC_VOL_EXIST,     /* Volatile MAC address exists  */
    MAC_ERROR_MAC_NOT_EXIST,     /* MAC address does not exist */
    MAC_ERROR_LEARN_FORCE_SECURE,/* Learn force secure is 'on' */
    MAC_ERROR_NOT_FOUND,          /* Not found */
    MAC_ERROR_MAC_ONE_DESTINATION_ALLOWED, /* Only one destination is allowed */
    MAC_ERROR_VLAN_INVALID        /* Invalid VLAN*/
}; // Let it be anonymous (tagless) for the sake of Lint.

/* MAC address table statistics */
typedef struct {
    ulong learned[VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD]; /* Number of learned entries per port */
    ulong learned_total;                 /* Total number of learned entries */
    ulong static_total;                  /* Total number of static entries */
} mac_table_stats_t;

typedef struct {
    BOOL only_this_vlan; /* Only look in this VLAN                          */
    BOOL only_in_conf;   /* Only look for entries added through this module */
    BOOL not_dynamic;    /* Not dynamic learn addresses                     */
    BOOL not_static;     /* Not static addresses                            */
    BOOL not_cpu;        /* Not addresses destined to cpu                   */
    BOOL not_mc;         /* Not MC addresses                                */
    BOOL not_uc;         /* Not UC addresses                                */
} mac_mgmt_addr_type_t;

typedef struct {
    mesa_vid_mac_t        vid_mac;                                          /* VLAN ID and MAC addr */
    mesa_port_list_t      destination[VTSS_ISID_END]; /* Dest. ports per ISID */
    BOOL                  copy_to_cpu;                                      /* CPU copy flag */
    BOOL                  locked;                                           /* Locked/static flag */
} mac_mgmt_table_stack_t;

typedef struct {
    mesa_vid_mac_t        vid_mac;     /* VLAN ID and MAC address                        */
    mesa_port_list_t      destination; /* Dest. ports  for this mac address              */
    BOOL                  dynamic;     /* Dynamic (1) or Static (locked) (0)             */
    BOOL                  volatil;     /* Volatile (0) (saved to flash) or not (1)       */
    BOOL                  copy_to_cpu; /* Make a CPU copy of frames to/from this MAC (1) */
} mac_mgmt_addr_entry_t;

typedef vtss_appl_mac_age_conf_t   mac_age_conf_t;

/* Get the age time. Unit sec. This is the local age time but is should be the same for the whole stack.*/
mesa_rc mac_mgmt_age_time_get(mac_age_conf_t *const conf);


/* Set the age time. Unit sec. All switches is the stack is given the same age-value */
mesa_rc mac_mgmt_age_time_set(const mac_age_conf_t *const conf);

/* Get the next Mac-address, dynamic or static per switch
 * 'next=1' means get-next while 'next=0' means a lookup
 * Address 00-00-00-00-00-00 will get the first entry. */
mesa_rc mac_mgmt_table_get_next(vtss_isid_t isid, mesa_vid_mac_t *vid_mac,
                                mesa_mac_table_entry_t *const entry, BOOL next);

/* Another version of 'get next mac-address'
   Mac address is search across the stack and all destination front ports are returned (per ISID).
   Addresses only learned on stack ports are not returned. The function will search for  specific addresses
   defined in *type. */
mesa_rc mac_mgmt_stack_get_next(mesa_vid_mac_t *vid_mac, mac_mgmt_table_stack_t *entry,
                                mac_mgmt_addr_type_t *type, BOOL next);

/* Get MAC address statistics per switch*/
mesa_rc mac_mgmt_table_stats_get(vtss_isid_t isid, mac_table_stats_t *stats);

/* Get MAC address statistics per VLAN */
mesa_rc mac_mgmt_table_vlan_stats_get(vtss_isid_t isid, mesa_vid_t vlan, mac_table_stats_t *stats);

/* Flush dynamic MAC address table, static entry are not touched. All switches in the stack are flushed. */
mesa_rc mac_mgmt_table_flush(void);

/* Use another age time for specified time.
 * Units: seconds. Zero will disable aging. No primary/secondary switch support */
mesa_rc mac_age_time_set(ulong mac_age_time, ulong time);

/* Add a static mac address to the mac table. Per switch
 * The address is automatically added to all stack ports in the stack, making it known everywhere  */
mesa_rc mac_mgmt_table_add(vtss_isid_t isid, mac_mgmt_addr_entry_t *entry);

/* Delete a volatile or non-volatile static mac address from the mac table. Per switch.
 * vol = 0 : non-volatile group - saved in flash
 * vol = 1 : volatile group - not saved in flash
 * The address is deleted from the ports given and the function will by it self find out if it should
 * delete the address from the Stack ports (if the address no longer owned by any front ports)
 * Use <MAC_ALL_VLANS> as an vlan id to make the function ignore vlans       */
mesa_rc mac_mgmt_table_del(vtss_isid_t isid, mesa_vid_mac_t *entry, BOOL vol);

/* Delete all volatile or non-volatile mac addresses from the port regardless of VLAN id. Per switch.
 * vol = 0 : non-volatile group - saved in flash
 * vol = 1 : volatile group - not saved in flash
 * This function is basically using <mac_mgmt_table_del> to delete the addresses */
mesa_rc mac_mgmt_table_port_del(vtss_isid_t isid, mesa_port_no_t port_no, BOOL vol);

/* Get the next  Mac-address which have been added through this API. Could be static or dynamic.
 * Address 00-00-00-00-00-00 will get the first entry.
 * 'next=1' means get-next while 'next=0' means a lookup
 * vol = 0 : search non-volatile group (saved in flash)
 * vol = 1 : search volatile group (not saved in flash)                     */
mesa_rc mac_mgmt_static_get_next(vtss_isid_t isid, mesa_vid_mac_t *search_mac,
                                 mac_mgmt_addr_entry_t *return_mac, BOOL next, BOOL vol);


/* Set the learning mode for each port in the switch.
 *    Learn modes: automatic (normal learning, default)
 *               : cpu       (learning disabled)
 *               : discard   (learning frames dropped)
 * Dynamic learned addresses are flushed in the process            */
mesa_rc mac_mgmt_learn_mode_set(vtss_isid_t isid, mesa_port_no_t port_no, const mesa_learn_mode_t *const learn_mode);


/* Enable / Disable learning per VLAN */
mesa_rc mac_mgmt_vlan_learn_mode_set(mesa_vid_t     vid,
                                     const vtss_appl_mac_vid_learn_mode_t *const mode);

/* Get learning per VLAN */
mesa_rc mac_mgmt_vlan_learn_mode_get(mesa_vid_t     vid,
                                     vtss_appl_mac_vid_learn_mode_t *const mode);

/* Get learning-disabled VLANs */
mesa_rc mac_mgmt_learning_disabled_vids_get(u8 vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES]);

/* Set learning-disabled VLANs */
mesa_rc mac_mgmt_learning_disabled_vids_set(u8 vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES]);

/* Get the learning mode for each port in the switch. */
void mac_mgmt_learn_mode_get(vtss_isid_t isid, mesa_port_no_t port_no, mesa_learn_mode_t *const learn_mode, BOOL *chg_allowed);

/* Force the learning mode for the port to secure (discard).
 * Mode is not saved to flash and can not be changed by <mac_mgmt_learn_mode_set>
 * Dynamic learned addresses are flushed in the process            */
mesa_rc mac_mgmt_learn_mode_force_secure(vtss_isid_t isid, mesa_port_no_t port_no, BOOL cpu_copy);

/* Revert the learning mode to saved value */
mesa_rc mac_mgmt_learn_mode_revert(vtss_isid_t isid, mesa_port_no_t port_no);

/* A port mac-address table is automaticlly flushed if the port link goes down.        */
/* This process can be bypassed with the mac_mgmt_port_mac_table_auto_flush() function.*/
/* enable = 1 : port is flushed. enable = 0 : port is not flushed.                     */
/* The state is volatile.                                                              */
mesa_rc mac_mgmt_port_mac_table_auto_flush(vtss_isid_t isid, mesa_port_no_t port_no, BOOL enable);

/* Initialize module */
mesa_rc mac_init(vtss_init_data_t *data);

/* Return an error text string based on a return code. */
const char *mac_error_txt(mesa_rc rc);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_MAC_API_H_ */

