/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _PTP_PIM_API_H_
#define _PTP_PIM_API_H_

typedef struct {
    u32 request;        // Number of received PIM request messages.
    u32 reply;          // Number of received PIM reply messages.
    u32 event;          // Number of received PIM event messages.
    u32 dropped;        // Number of dropped received PIM messages.
    u32 tx_dropped;     // Number of dropped transmitted PIM messages.
    u32 errors;         // Number of received PIM messages containing errors.
} ptp_pim_frame_statistics_t;

typedef struct {
    // Callout function called when a 1pps message is received.
    // Arguments:
    //   #port_no : Rx port.
    //   #ts      : Points to the received timestamp.
    void (*co_1pps)(mesa_port_no_t port_no, const mesa_timestamp_t *ts);
    // Callout function called when a delay message is received.
    // Arguments:
    //   #port_no : Rx port.
    //   #delay   : Points to the received delay value (the value is a scaled ns value).
    void (*co_delay)(mesa_port_no_t port_no, const mesa_timeinterval_t *ts);
    // Callout function called when a pre delay message is received.
    // Arguments:
    //   #port_no : Rx port.
    void (*co_pre_delay)(mesa_port_no_t port_no);
    // Callout function called when alarm is received.
    void (*co_alarm)(mesa_port_no_t port_no, mesa_bool_t alarm);
    // trace group
    u32 tg;
} ptp_pim_init_t;

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
// ptp_pim_init()
/******************************************************************************/
void ptp_pim_init(const ptp_pim_init_t *ini, bool pim_active);

/******************************************************************************/
// ptp_pim_1pps_msg_send()
// Transmit the time of next 1pps pulse.
/******************************************************************************/
void ptp_pim_1pps_msg_send(mesa_port_no_t port_no, const mesa_timestamp_t *ts);

/******************************************************************************/
// ptp_pim_modem_delay_msg_send()
// Send a request to a specific port.
/******************************************************************************/
void ptp_pim_modem_delay_msg_send(mesa_port_no_t port_no, const mesa_timeinterval_t *delay);

/******************************************************************************/
// ptp_pim_modem_pre_delay_msg_send()
// Send a request to a specific port.
/******************************************************************************/
void ptp_pim_modem_pre_delay_msg_send(mesa_port_no_t port_no);

/******************************************************************************/
// ptp_pim_statistics_get()
// Read the protocol statisticcs, optionally clear the statistics.
/******************************************************************************/
void ptp_pim_statistics_get(mesa_port_no_t port_no, ptp_pim_frame_statistics_t *stati, bool clear);

// Transmit alarm through pim interface.
void ptp_pim_alarm_msg_send(mesa_port_no_t port_no, mesa_bool_t alarm);

#ifdef __cplusplus
}
#endif
#endif /* _PTP_PIM_API_H_ */

