/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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
//----------------------------------------------------------------------------
/**
 *  \file
 *      dhcp_server_platform.h
 *
 *  \brief
 *      Platform-dependent definitions
 *
 *  \author
 *      CP Wang
 *
 *  \date
 *      2015-01-22 14:49
 */
//----------------------------------------------------------------------------
#ifndef __DHCP_SERVER_PLATFORM_H__
#define __DHCP_SERVER_PLATFORM_H__
//----------------------------------------------------------------------------

/*
==============================================================================

    Include File

==============================================================================
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "vtss_os_wrapper_network.h"

#include "vtss_dhcp_server_type.h"
#include "vtss_dhcp_server_message.h"

#include "vtss_trace_api.h"
#include "vtss_module_id.h"
#include "vtss_trace_lvl_api.h"

/*lint -sem( dhcp_server_platform_sema_take, thread_lock )   */
/*lint -sem( dhcp_server_platform_sema_give, thread_unlock ) */

/*
==============================================================================

    Constant

==============================================================================
*/
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_DHCP_SERVER

/*
==============================================================================

    Macro

==============================================================================
*/
#define __SEMA_TAKE()     dhcp_server_platform_sema_take(__FILE__, __LINE__)
#define __SEMA_GIVE()     dhcp_server_platform_sema_give(__FILE__, __LINE__)

/*
==============================================================================

    Type Definition

==============================================================================
*/
/**
 *  \brief
 *      entry of the created thread.
 *
 *  \param
 *      thread_data [IN]: initialization data and callbacks.
 *
 *  \return
 *      TRUE  : successful.\n
 *      FALSE : failed.
 */
typedef BOOL dhcp_server_platform_thread_entry_cb_t(
    IN i32      thread_data
);

/*
==============================================================================

    Public Function

==============================================================================
*/
/**
 * \brief
 *      initialize platform
 *
 * \return
 *      TRUE  - successful
 *      FALSE - failed
 */
BOOL dhcp_server_platform_init(void);

/**
 * \brief
 *      Create thread.
 *
 * \param
 *      session_id [IN]: session ID
 *      name       [IN]: name of thread.
 *      priority   [IN]: thread priority.
 *      entry_cb   [IN]: thread running entry.
 *      entry_data [IN]: input parameter of thread running entry.
 *
 * \return
 *      TRUE : successful.
 *      FALSE: failed.
 */
BOOL dhcp_server_platform_thread_create(
    IN  i32                                     session_id,
    IN  const char                              *name,
    IN  u32                                     priority,
    IN  dhcp_server_platform_thread_entry_cb_t  *entry_cb,
    IN  i32                                     entry_data
);

/**
 * \brief
 *      get the time elapsed from system start in seconds.
 *      process wrap around.
 *
 * \param
 *      n/a.
 *
 * \return
 *      seconds from system start.
 */
u32 dhcp_server_platform_current_time_get(
    void
);

/**
 * \brief
 *      sleep for milli-seconds.
 *
 * \param
 *      t [IN]: milli-seconds for sleep.
 *
 * \return
 *      n/a.
 */
void dhcp_server_platform_sleep(
    IN u32  t
);

/**
 * \brief
 *      take semaphore
 *
 * \param
 *      file [IN]: file name
 *      line [IN]: line number
 *
 * \return
 *      n/a.
 */
void dhcp_server_platform_sema_take(
    const char *const   file,
    const int           line
);

/**
 * \brief
 *      give semaphore
 *
 * \param
 *      file [IN]: file name
 *      line [IN]: line number
 *
 * \return
 *      n/a.
 */
void dhcp_server_platform_sema_give(
    const char *const   file,
    const int           line
);

/**
 * \brief
 *      get the time elapsed from system start in seconds.
 *      process wrap around.
 *
 * \param
 *      n/a.
 *
 * \return
 *      seconds from system start.
 */
u32 dhcp_server_platform_current_time_get(
    void
);

/**
 *  \brief
 *      get IP interface of VLAN
 *
 *  \param
 *      vid     [IN] : VLAN ID
 *      ip      [OUT]: IP address of the VLAN
 *      netmask [OUT]: Netmask of the VLAN
 *
 *  \return
 *      TRUE  : successful
 *      FALSE : failed
 */
BOOL dhcp_server_platform_vid_info_get(
    IN  mesa_vid_t          vid,
    OUT mesa_ipv4_t         *ip,
    OUT mesa_ipv4_t         *netmask
);

/**
 * \brief
 *      register packet rx callback.
 *
 * \param
 *      n/a.
 *
 * \return
 *      n/a.
 */
void dhcp_server_platform_packet_rx_register(
    void
);

/**
 * \brief
 *      deregister packet rx callback.
 *
 * \param
 *      n/a.
 *
 * \return
 *      n/a.
 */
void dhcp_server_platform_packet_rx_deregister(
    void
);

/**
 * \brief
 *      send DHCP message.
 *
 * \param
 *      dhcp_message [IN]: DHCP message.
 *      option_len   [IN]: option field length.
 *      vid          [IN]: VLAN ID to send.
 *      sip          [IN]: source IP.
 *      dmac         [IN]: destination MAC.
 *      dip          [IN]: destination IP.
 *
 * \return
 *      TRUE  : successfully.
 *      FALSE : fail to send
 */
BOOL dhcp_server_platform_packet_tx(
    IN  dhcp_server_message_t   *dhcp_message,
    IN  u32                     option_len,
    IN  mesa_vid_t              vid,
    IN  mesa_ipv4_t             sip,
    IN  u8                      *dmac,
    IN  mesa_ipv4_t             dip
);

/**
 *  \brief
 *      syslog message.
 *
 *  \param
 *      format [IN] : message format.
 *      ...    [IN] : message parameters
 *
 *  \return
 *      n/a.
 */
void dhcp_server_platform_syslog(
    IN  const char  *format,
    IN  ...
);

//----------------------------------------------------------------------------
#endif //__DHCP_SERVER_PLATFORM_H__
