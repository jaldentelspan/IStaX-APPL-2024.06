/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _DHCP6C_PORTING_HXX_
#define _DHCP6C_PORTING_HXX_

#include "dhcp6c_priv.hxx"

#define VTSS_DHCP6C_PKT_HELPER      (defined(VTSS_SW_OPTION_DHCP6_HELPER))

#if VTSS_DHCP6C_PKT_HELPER
#define VTSS_DHCP6C_MY_PKT_HANDLER  0
#else
#define VTSS_DHCP6C_USE_PKT_FILTER  1               // Porting option
#define VTSS_DHCP6C_MY_PKT_HANDLER  (defined(VTSS_SW_OPTION_PACKET) && VTSS_DHCP6C_USE_PKT_FILTER)
#endif /* VTSS_DHCP6C_PKT_HELPER */

namespace vtss
{
namespace dhcp6c
{

namespace porting
{

namespace pkt
{
#if VTSS_DHCP6C_MY_PKT_HANDLER
#include "packet_api.h"
#endif /* VTSS_DHCP6C_MY_PKT_HANDLER */

/* Set/Unset DHCPv6 frame trapper */
mesa_rc rx_trapper_set(BOOL state);

/* Start the DHCP6C RX Process */
mesa_rc rx_process_start(dhcp6c_ifidx_t ifx);

/* Stop the DHCP6C RX Process */
mesa_rc rx_process_stop(dhcp6c_ifidx_t ifx);

/* TX misc. */
mesa_rc tx_misc_config(dhcp6c_ifidx_t ifidx, BOOL state);

/* TX process */
mesa_rc tx_process(const pkt_info_t *const info, const u8 *const frm);
}; /* pkt */

namespace os
{
struct my_thread_entry_data_t {
    i64     msg;
};

/* Create thread */
void thread_create(
    vtss_thread_entry_f *entry,
    const char          *name,
    void                *stack_base,
    u32                 stack_size,
    vtss_handle_t       *handle,
    vtss_thread_t       *thrd
);

/* Thread Exit */
void thread_exit(vtss_addrword_t data);

/* Event Create */
void event_create(vtss_flag_t *const flag);

/* Event Set */
void event_set(vtss_flag_t *const flag, vtss_flag_value_t value);

/* Event Mask */
void event_mask(vtss_flag_t *const flag, vtss_flag_value_t value);

/* Event Delete */
void event_delete(vtss_flag_t *const flag);

/* Event Wait */
vtss_flag_value_t event_wait(
    vtss_flag_t         *const flag,
    vtss_flag_value_t   pattern,
    vtss_flag_mode_t    mode
);
}; /* os */

} /* porting */

} /* dhcp6c */
} /* vtss */

#endif /* _DHCP6C_PORTING_HXX_ */
