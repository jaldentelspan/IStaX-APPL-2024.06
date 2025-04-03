/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_POE_H_
#define _VTSS_POE_H_

#include "poe_api.h"

/**********************************************************************
 * PoE Messages
 **********************************************************************/
typedef enum {
    POE_MSG_ID_GET_STAT_REQ,          /* Primary switch's request to get status from a secondary switch */
    POE_MSG_ID_GET_STAT_REP,          /* Secondary switch's reply upon a get status */
    POE_MSG_ID_GET_PD_CLASSES_REP,    /* Secondary switch's reply upon a get PD classes  */
    POE_MSG_ID_GET_PD_CLASSES_REQ,    /* Primary switch's request to get PD classes from a secondary switch */
} poe_msg_id_t;

#define POE_UNDETERMINED_CLASS 0xC

// Request message
typedef struct {
    // Message ID
    poe_msg_id_t msg_id;

    union {
        /* POE_MSG_ID_CONF_SET_REQ */

        /* POE_MSG_ID_GET_STAT_REQ: No data */

        /* POE_MSG_ID_GET_PD_CLASSES_REQ: No data */
    } req;
} poe_msg_req_t;

// Reply message
typedef struct {
    // Message ID
    poe_msg_id_t msg_id;

    union {
        /* POE_MSG_ID_CONF_STAT_REP */
        // poe_status_t status; // deleted as it was not used

        /* POE_MSG_ID_GET_PD_CLASSES_REP */
        char         classes[VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD];
    } rep;
} poe_msg_rep_t;

typedef struct {
    int dev;
    int base;
} poe_device_t;

// For containing software and hardware version
typedef struct {
    u16 sw_ver;
    u8  hw_ver;
    u8  param;
    u16 internal_sw_ver;
} poe_firmware_version_t;


#define MSCC_POE_FORCE_FIRMWARE_UPDATE 1

// bitset for PoE status led
#define POE_LED_POE_OFF     (1 << 0)
#define POE_LED_POE_ON      (1 << 1)
#define POE_LED_POE_DENIED  (1 << 2)
#define POE_LED_POE_ERROR   (1 << 3)


// ------  syslog  -----
enum poe_syslog_level_t
{
    eSYSLOG_LVL_ERROR,    // Highest priority
    eSYSLOG_LVL_WARNING,
    eSYSLOG_LVL_NOTICE,
    eSYSLOG_LVL_INFO,     // Lowest priority
};

enum poe_controller_type_t
{
    ePD69200,
    ePD69210,
    ePD69220
};

enum poe_syslog_message_type_t
{
    ePORT_STAT_CHANGE,
    ePOE_LLDP_PD_REQUEST,
    ePOE_CONTROLLER_FOUND,     // report controller index + controller type
    ePOE_CONTROLLER_NOT_FOUND,
    ePOE_FIRMWARE_UPDATE,
    ePOE_EXIT_POE_THREAD,
    ePOE_GLOBAL_MSG,
    ePOE_PORT_MSG
};


typedef union uPoE_syslog_info
{
    struct // PoE port status change
    {
        mesa_port_no_t  port_index;          // logical port index
        uint8_t         old_poe_port_stat;
        uint8_t         new_poe_port_stat;
        uint8_t         old_internal_poe_status;
        uint8_t         new_internal_poe_status;
        uint8_t         pd_type_sspd_dspd;
        char            *msg;
    } port_stat_change_t;

    struct // lldp pd request
    {
        mesa_port_no_t  port_index;          // logical port index
        bool                  is_bt_port;
        mesa_poe_milliwatt_t  alt_a_mw;
        mesa_poe_milliwatt_t  alt_b_mw;
        mesa_poe_milliwatt_t  single_mw;
        uint8_t               cable_length;
    } lldp_pd_power_request_t;

    struct  // PoE controller found
    {
        uint8_t               Index; // PCB111 and PCB135 use 2 PoE controllers (can be 1 or 2)
        poe_controller_type_t eType; // enum of 69200 / 69210 / 69220
    } controller_found_t;

    struct  // PoE firmware update
    {
        uint8_t               poe_mcu_index; // PCB111 and PCB135 use 2 PoE controllers (which one was updated)
        poe_controller_type_t eType;         // enum of 69200 / 69210 / 69220 for the controller type that was updated
        char                 *firmware_status;
    } poe_firmware_update_t;

    struct  // Exit PoE thread
    {
        const char *error_msg;  // error message why POE thread decided to terminate itself
        uint8_t     error_Code; // returned RC error code
    } exit_thread_t;

    struct  // Generic message
    {
        const char *msg;              // message
    } general_global_msg_t;

    struct  // Generic port message
    {
        mesa_port_no_t  port_index;    // logical port index
        const char      *msg;          // message
    } general_port_msg_t;

}uPOE_SYSLOG_INFO;

// Send PoE syslog message. pInfo pointer can be NULL for messages with no extra info
void poe_Send_SysLog_Msg(poe_syslog_level_t log_level, poe_syslog_message_type_t eMsgType, uPOE_SYSLOG_INFO *pInfo);


#endif //_VTSS_POE_H_

