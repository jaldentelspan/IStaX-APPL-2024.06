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

//----------------------------------------------------------------------------
/**
 *  \file
 *      dhcp_server.c
 *
 *  \brief
 *      DHCP server APIs
 *      1. platform APIs for base part
 *      2. public APIs
 *
 *  \author
 *      CP Wang
 *
 *  \date
 *      05/08/2013 17:51
 */
//----------------------------------------------------------------------------

/*
==============================================================================

    Include File

==============================================================================
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vtss_dhcp_server.h"
#include "vtss_dhcp_server_message.h"
#include "dhcp_server_platform.h"
#include "msg_api.h"
#include "dhcp_server_api.h"
#include "ip_utils.hxx"
#include "ip_api.h"
#include "port_api.h"
#include "vtss/appl/interface.h"

#ifdef VTSS_SW_OPTION_ICFG
#include "dhcp_server_icfg.h"
#endif

/*
==============================================================================

    Constant

==============================================================================
*/
#define _TIMER_IDLE_TIME        1000    /**< 1 second in milli-second */

/*
==============================================================================

    Macro

==============================================================================
*/

/*
==============================================================================

    Type Definition

==============================================================================
*/
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize our private mib */
VTSS_PRE_DECLS void dhcp_server_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_dhcp_server_json_init(void);
#endif

/*
==============================================================================

    Static Variable

==============================================================================
*/

/*
==============================================================================

    Static Function

==============================================================================
*/
/*
    because the lease time resolution is in minute,
    it does not need so exact so just sleep 1 second
*/
/**
 * \brief
 *      becauase the lease time resolution is in minute,
 *      it does not need so exact so just sleep 1 second.
 *
 * \param
 *      t [IN]: milli-seconds for sleep.
 *
 * \return
 *      n/a.
 */
static BOOL _timer_thread(IN i32 thread_data)
{
    while (1) {

        /* sleep 1 second */
        dhcp_server_platform_sleep(_TIMER_IDLE_TIME);

        if (msg_switch_is_primary()) {
            __SEMA_TAKE();
            vtss_dhcp_server_timer_process();
            __SEMA_GIVE();
        }
    }

#ifdef DHCP_SERVER_TARGET
    // Avoid "Unreachable code at token "return".
    // Statement must be here for the sake of GCC 4.7
    /*lint -e(527) */
    return TRUE;
#endif
}

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
u32 dhcp_server_current_time_get(
    void
)
{
    return dhcp_server_platform_current_time_get();
}

#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
/**
 * \brief
 *      Port callback. We look for link-down events and remove bindings (not only expire) for those reserved addresses
 *      that have matching ports.
 */
static void _dhcp_server_port_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    dhcp_server_pool_t    pool;
    vtss_ifindex_t        port_ifindex;
    dhcp_server_binding_t binding;

    // We're only interested in link-down events
    if (status->link) {
        return;
    }

    if (msg_switch_is_primary()) {
        if (vtss_ifindex_from_port(VTSS_ISID_START, port_no, &port_ifindex) != VTSS_RC_OK) {
            return;
        }

        memset(&pool,    0, sizeof(pool));
        memset(&binding, 0, sizeof(binding));

        __SEMA_TAKE();

        // Iterate all pools, find any reserved entries installed for port_no and clear them
        while (vtss_dhcp_server_pool_get_next(&pool) == VTSS_RC_OK) {
            if (pool.type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_NETWORK) {
                for (u32 i = 0; i < VTSS_APPL_DHCP_SERVER_RESERVED_CNT; i++) {
                    if (pool.reserved[i].ifindex == port_ifindex) {
                        T_D("Found reserved interface at %u: ifindex %u, IP 0x%08x. Removing binding", i, VTSS_IFINDEX_PRINTF_ARG(port_ifindex), pool.reserved[i].address);
                        binding.ip = pool.reserved[i].address;
                        // Delete binding, if any. First time moves it to expired; second time removes it completely
                        (void)vtss_dhcp_server_binding_delete(&binding);
                        (void)vtss_dhcp_server_binding_delete(&binding);
                        break;      // There can only be one matching reserved entry per pool, so move on to next pool
                    }
                }
            }
        }

        __SEMA_GIVE();
    }
}
#endif // VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES

extern "C" int dhcp_server_icli_cmd_register();

/*
==============================================================================

    Public API

==============================================================================
*/
/**
 * \brief
 *      Start DHCP server.
 *
 * \param
 *      data [IN]: init data.
 *
 * \return
 *      mesa_rc: VTSS_RC_OK on success.\n
 *               others are failed.
 */
mesa_rc dhcp_server_init(vtss_init_data_t *data)
{
    switch (data->cmd) {
    case INIT_CMD_INIT:
        /* initialize platform */
        if (dhcp_server_platform_init() == FALSE) {
            T_D("fail to initialize platform\n");
            return VTSS_RC_ERROR;
        }

        /* start dhcp server engine */
        if ( vtss_dhcp_server_init() == FALSE ) {
            T_D("fail to initialize DHCP server engine\n");
            return VTSS_RC_ERROR;
        }

        /* create timer thread */
        if ( dhcp_server_platform_thread_create(1, "DHCP_SERVER_TIMER", 0, _timer_thread, 0) == FALSE ) {
            T_D("Fail to create thread for timer\n");
            return FALSE;
        }

#ifdef VTSS_SW_OPTION_ICFG
        if ( dhcp_server_icfg_init() != VTSS_RC_OK ) {
            T_D("ICFG not initialized correctly");
        }
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register our private mib */
        dhcp_server_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_dhcp_server_json_init();
#endif
        dhcp_server_icli_cmd_register();

#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
        // Register for port changes so we'll learn of link-down events
        if (port_change_register(VTSS_MODULE_ID_DHCP_SERVER, _dhcp_server_port_callback) != VTSS_RC_OK) {
            T_E("Can't register for port changes");
        }
#endif
        break;

    case INIT_CMD_CONF_DEF:
        __SEMA_TAKE();
        vtss_dhcp_server_reset_to_default();
        __SEMA_GIVE();
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

/**
 * \brief
 *      error string for DHCP server.
 *
 * \param
 *      rc [IN]: error code.
 *
 * \return
 *      char *: error string.
 */
const char *dhcp_server_error_txt(
    IN mesa_rc  rc
)
{
    switch (rc) {
    case VTSS_APPL_DHCP_SERVER_RC_ERROR:
        return "General error";

    case VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER:
        return "Invalid parameter";

    case VTSS_APPL_DHCP_SERVER_RC_ERR_POOL_NOT_EXIST:
        return "No such pool";

    case VTSS_APPL_DHCP_SERVER_RC_ERR_NOT_EXIST:
        return "No such entry";

    case VTSS_APPL_DHCP_SERVER_RC_ERR_MEMORY:
        return "Insufficient memory for operation";

    case VTSS_APPL_DHCP_SERVER_RC_ERR_FULL:
        return "Database is full";

    case VTSS_APPL_DHCP_SERVER_RC_ERR_DUPLICATE:
        return "Duplicate configuration";

    case VTSS_APPL_DHCP_SERVER_RC_ERR_IP:
        return "Invalid IP address";

    case VTSS_APPL_DHCP_SERVER_RC_ERR_NOT_IN_SUBNET:
        return "Address is not within the pool subnet";

    case VTSS_APPL_DHCP_SERVER_RC_ERR_IFC_OCCUPIED:
        return "Interface occupied by another reserved address";

    case VTSS_APPL_DHCP_SERVER_RC_ERR_POOL_TYPE_CONFLICT:
        return "Configuration conflicts with pool type";

    default:
        return "Unknown error";
    }
}

/**
 *  \brief
 *      reset DHCP server engine to default configuration.
 *
 *  \param
 *      n/a.
 *
 *  \return
 *      n/a.
 */
void dhcp_server_reset_to_default(
    void
)
{
    __SEMA_TAKE();

    vtss_dhcp_server_reset_to_default();

    __SEMA_GIVE();
}

/**
 *  \brief
 *      Enable/Disable DHCP server.
 *
 *  \param
 *      b_enable [IN]: TRUE - enable, FALSE - disable.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_enable_set(
    IN  BOOL    b_enable
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_enable_set( b_enable );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Get if DHCP server is enabled or not.
 *
 *  \param
 *      b_enable [OUT]: TRUE - enable, FALSE - disable.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_enable_get(
    OUT BOOL    *b_enable
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_enable_get( b_enable );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Add excluded IP address range.
 *      index : low_ip, high_ip
 *
 *  \param
 *      excluded_ip [IN]: IP address range to be excluded.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_excluded_add(
    IN  dhcp_server_excluded_ip_t     *excluded_ip
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_excluded_add( excluded_ip );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Delete excluded IP address range.
 *      index : low_ip, high_ip
 *
 *  \param
 *      excluded_ip [IN]: Excluded IP address range to be deleted.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_excluded_delete(
    IN  dhcp_server_excluded_ip_t     *excluded_ip
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_excluded_delete( excluded_ip );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Get excluded IP address range.
 *      index : low_ip, high_ip
 *
 *  \param
 *      excluded_ip [IN] : index.
 *      excluded_ip [OUT]: Excluded IP address range data.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_excluded_get(
    INOUT  dhcp_server_excluded_ip_t     *excluded_ip
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_excluded_get( excluded_ip );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Get next of current excluded IP address range.
 *      index : low_ip, high_ip
 *
 *  \param
 *      excluded_ip [IN] : Currnet excluded IP address range index.
 *      excluded_ip [OUT]: Next excluded IP address range data to get.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_excluded_get_next(
    INOUT  dhcp_server_excluded_ip_t     *excluded_ip
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_excluded_get_next( excluded_ip );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Set DHCP pool.
 *      index: name
 *
 *  \param
 *      pool [IN]: new or modified DHCP pool.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_pool_set(
    IN  dhcp_server_pool_t     *pool
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_pool_set( pool );

    __SEMA_GIVE();
    return rc;
}

/**
 *  \brief
 *      Delete DHCP pool.
 *      index: name
 *
 *  \param
 *      pool [IN]: DHCP pool to be deleted.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_pool_delete(
    IN  dhcp_server_pool_t     *pool
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_pool_delete( pool );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Get DHCP pool.
 *      index: name
 *
 *  \param
 *      pool [IN] : index.
 *      pool [OUT]: DHCP pool data.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_pool_get(
    INOUT  dhcp_server_pool_t     *pool
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_pool_get( pool );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Get next of current DHCP pool.
 *      index: name
 *
 *  \param
 *      pool [IN] : Currnet DHCP pool index.
 *      pool [OUT]: Next DHCP pool data to get.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_pool_get_next(
    INOUT  dhcp_server_pool_t     *pool
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_pool_get_next( pool );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Set DHCP pool to be default value.
 *
 *  \param
 *      pool [OUT]: default DHCP pool.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_pool_default(
    IN  dhcp_server_pool_t     *pool
)
{
    return vtss_dhcp_server_pool_default( pool );
}

/**
 *  \brief
 *      Clear DHCP server statistics.
 *
 *  \param
 *      n/a.
 *
 *  \return
 *      n/a.
 */
void dhcp_server_statistics_clear(
    void
)
{
    __SEMA_TAKE();

    vtss_dhcp_server_statistics_clear();

    __SEMA_GIVE();
}

/**
 *  \brief
 *      Get DHCP server statistics.
 *
 *  \param
 *      statistics [OUT]: DHCP server statistics data to get.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_statistics_get(
    OUT dhcp_server_statistics_t  *statistics
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_statistics_get( statistics );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Delete DHCP binding.
 *      index: ip
 *
 *  \param
 *      binding [IN]: DHCP binding to be deleted.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_binding_delete(
    IN  dhcp_server_binding_t     *binding
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_binding_delete( binding );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Get DHCP binding.
 *      index: ip
 *
 *  \param
 *      binding [IN] : index.
 *      binding [OUT]: DHCP binding data.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_binding_get(
    INOUT  dhcp_server_binding_t     *binding
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_binding_get( binding );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Get next of current DHCP binding.
 *      index: ip
 *
 *  \param
 *      binding [IN] : Currnet DHCP binding index.
 *      binding [OUT]: Next DHCP binding data to get.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_binding_get_next(
    INOUT  dhcp_server_binding_t     *binding
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_binding_get_next( binding );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Clear DHCP bindings by binding type.
 *
 *  \param
 *      type - binding type
 *
 *  \return
 *      n/a.
 */
void dhcp_server_binding_clear_by_type(
    IN vtss_appl_dhcp_server_binding_type_t     type
)
{
    __SEMA_TAKE();

    vtss_dhcp_server_binding_clear_by_type( type );

    __SEMA_GIVE();
}

/**
 *  \brief
 *      Enable/Disable DHCP server per VLAN.
 *
 *  \param
 *      vid      [IN]: VLAN ID
 *      b_enable [IN]: TRUE - enable, FALSE - disable.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_vlan_enable_set(
    IN  mesa_vid_t      vid,
    IN  BOOL            b_enable
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_vlan_enable_set( vid, b_enable );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Get Enable/Disable DHCP server per VLAN.
 *
 *  \param
 *      vid      [IN] : VLAN ID
 *      b_enable [OUT]: TRUE - enable, FALSE - disable.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_vlan_enable_get(
    IN  mesa_vid_t      vid,
    OUT BOOL            *b_enable
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_vlan_enable_get( vid, b_enable );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      add a declined IP. This API should be used for debug only.
 *      index: declined_ip
 *
 *  \param
 *      declined_ip [IN] : index.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_declined_add(
    IN  mesa_ipv4_t     declined_ip
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_declined_add( declined_ip );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      delete a declined IP. This API should be used for debug only.
 *      index: declined_ip
 *
 *  \param
 *      declined_ip [IN] : index.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_declined_delete(
    IN  mesa_ipv4_t     declined_ip
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_declined_delete( declined_ip );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Get declined IP.
 *      index: declined_ip
 *
 *  \param
 *      declined_ip [IN] : index.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_declined_get(
    IN  mesa_ipv4_t     *declined_ip
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_declined_get( declined_ip );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Get next declined IP.
 *      index: declined_ip
 *
 *  \param
 *      declined_ip [IN] : Currnet declined IP.
 *      declined_ip [OUT]: Next declined IP to get.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc dhcp_server_declined_get_next(
    INOUT   mesa_ipv4_t     *declined_ip
)
{
    mesa_rc    rc;

    __SEMA_TAKE();

    rc = vtss_dhcp_server_declined_get_next( declined_ip );

    __SEMA_GIVE();

    return rc;
}

/**
 *  \brief
 *      Get date string from number of seconds
 *
 *  \param
 *      second [IN] : number of seconds.
 *      str    [OUT]: string of date.
 *
 *  \return
 *      string of date.
 */
char *dhcp_server_day_str_get(
    IN  u32     second,
    OUT char    *str
)
{
    u32     minute;
    u32     hour;
    u32     day;

    if ( str == NULL ) {
        return NULL;
    }

    day    = second / (24 * 60 * 60);
    hour   = ( second % (24 * 60 * 60) ) / (60 * 60);
    minute = ( second % (60 * 60) ) / 60;
    second = second % 60;

    if ( day ) {
        sprintf(str, "%u days %u hours %u minutes %u seconds", day, hour, minute, second);
    } else if ( hour ) {
        sprintf(str, "%u hours %u minutes %u seconds", hour, minute, second);
    } else if ( minute ) {
        sprintf(str, "%u minutes %u seconds", minute, second);
    } else {
        sprintf(str, "%u seconds", second);
    }
    return str;
}

BOOL dhcp_server_binding_x_get_next(
    IN      u32                     i,
    INOUT   dhcp_server_binding_t   *binding
)
{
    BOOL    b;

    __SEMA_TAKE();

    b = vtss_dhcp_server_binding_x_get_next(i, binding);

    __SEMA_GIVE();

    return b;
}

/*
==============================================================================

    Public APIs in vtss_appl\include\vtss\appl\dhcp_server.h

==============================================================================
*/
/**
 * \brief
 *      translate character to integer value.
 */
static i32 _hex_get_c(
    IN  char c
)
{
    if ( c >= '0' && c <= '9' ) {
        return ( c - '0' );
    } else if ( (c >= 'A') && (c <= 'F') ) {
        return ( c - 'A' + 10 );
    } else if ( (c >= 'a') && (c <= 'f') ) {
        return ( c - 'a' + 10 );
    }
    return -1;
}

/**
 * \brief
 *      translate string to integer value.
 */
static BOOL _hexval_get(
    IN  const char  *str,
    OUT u8          *hval,
    OUT u32         *len
)
{
    //common
    const char      *c;
    //by type
    i32             i;
    u32             k;
    BOOL            b_first;
    BOOL            b_begin;

    c = str;
    if ( (*c) != '0' ) {
        return FALSE;
    }

    c++;
    if ( *c == 0 ) {
        return FALSE;
    } else if ( (*c) != 'x' && (*c) != 'X' ) {
        return FALSE;
    }

    c++;
    if ( *c == 0 ) {
        return FALSE;
    }

    b_begin = TRUE;
    b_first = TRUE;
    k = 0;
    for ( ; *c; c++ ) {
        i = _hex_get_c(*c);
        if ( i == -1 ) {
            return FALSE;
        }

        // get begin position of 1 happen
        if ( b_begin ) {
            if ( strlen(str) % 2 ) {
                b_first = FALSE;
            } else {
                b_first = TRUE;
            }
            b_begin = FALSE;
        }

        if ( b_first ) {
            i <<= 4;
            hval[ k ] = (u8)i;
            b_first = FALSE;
        } else {
            hval[ k ] += (u8)i;
            ( k )++;
            b_first = TRUE;
        }
    }

    (*len) = k;
    return TRUE;
}

/**
 * \brief
 *      translate integer value to string.
 */
static char *_hexval_str_get(
    IN  u8      *val,
    IN  u32     len,
    OUT char    *str
)
{
    u32     i;
    char    *s;

    if ( len == 0 ) {
        *str = 0;
        return str;
    }

    s = str;
    sprintf(s, "0x");
    s += 2;
    for ( i = 0; i < len; i++ ) {
        sprintf(s, "%02x", val[i]);
        s += 2;
    }
    *s = 0;

    return str;
}

/**
 *  \brief
 *      check if the MAC address is not empty.
 */
static BOOL _not_empty_mac(
    IN  const mesa_mac_t  *mac
)
{
    u8 empty_mac[DHCP_SERVER_MAC_LEN] = {0, 0, 0, 0, 0, 0};

    if ( memcmp(mac, empty_mac, DHCP_SERVER_MAC_LEN) ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 * \brief Get DHCP server global configuration
 *
 * \param globals [OUT] The global configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_globals_get(
    vtss_appl_dhcp_server_config_globals_t          *const globals
)
{
    if ( globals == NULL ) {
        T_D("globals == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    return dhcp_server_enable_get(&globals->mode);
}

/**
 * \brief Set DHCP server global configuration
 *
 * \param globals [IN] The global configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_globals_set(
    const vtss_appl_dhcp_server_config_globals_t    *const globals
)
{
    if ( globals == NULL ) {
        T_D("globals == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    return dhcp_server_enable_set(globals->mode);
}

/**
 * \brief Iterate function of VLAN configuration table
 *
 * To get first and get next ifindex of VLAN.
 *
 * \param prev_ifindex [IN]  ifindex of previous VLAN.
 * \param next_ifindex [OUT] ifindex of next VLAN.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_vlan_entry_itr(const vtss_ifindex_t *const prev_ifindex, vtss_ifindex_t *const next_ifindex)
{
    return vtss_appl_ip_if_itr(prev_ifindex, next_ifindex, true /* Only return VLAN interfaces */);
}

/**
 * \brief Get VLAN configuration entry
 *
 * To read VLAN configuration of DHCP server.
 *
 * \param ifindex [IN]  (key) ifindex of VLAN
 * \param vlan    [OUT] VLAN configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_vlan_entry_get(
    vtss_ifindex_t                              ifindex,
    vtss_appl_dhcp_server_config_vlan_entry_t   *const vlan
)
{
    vtss_ifindex_elm_t ife;
    BOOL               mode;
    mesa_rc            rc;

    // check parameter
    if ( vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( ife.iftype != VTSS_IFINDEX_TYPE_VLAN ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( vlan == NULL ) {
        T_D("vlan == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    // get mode
    if ( (rc = dhcp_server_vlan_enable_get((mesa_vid_t)(ife.ordinal), &mode)) != VTSS_RC_OK ) {
        return rc;
    }

    vlan->mode = mode;

    return VTSS_RC_OK;
}

/**
 * \brief Set VLAN configuration entry
 *
 * To write VLAN configuration of DHCP server.
 *
 * \param ifindex [IN] (key) ifindex of VLAN
 * \param vlan    [IN] VLAN configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_vlan_entry_set(vtss_ifindex_t ifindex, const vtss_appl_dhcp_server_config_vlan_entry_t *const vlan)
{
    vtss_ifindex_elm_t ife;

    if (vlan == NULL) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    // check parameter
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_VLAN) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if (!vtss_appl_ip_if_exists(ifindex)) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    // set mode
    return dhcp_server_vlan_enable_set((mesa_vid_t)(ife.ordinal), vlan->mode);
}

/**
 * \brief Iterate function of excluded IP table
 *
 * To get first and get next indexes.
 *
 * \param prev_lowIp  [IN]  previous low IP address.
 * \param next_lowIp  [OUT] next low IP address.
 * \param prev_highIp [IN]  previous high IP address.
 * \param next_highIp [OUT] next high IP address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_excluded_ip_entry_itr(
    const mesa_ipv4_t       *const prev_lowIp,
    mesa_ipv4_t             *const next_lowIp,
    const mesa_ipv4_t       *const prev_highIp,
    mesa_ipv4_t             *const next_highIp
)
{
    dhcp_server_excluded_ip_t excluded;
    mesa_rc                   rc;

    // check parameter
    if ( next_lowIp == NULL ) {
        T_D("next_lowIp == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( next_highIp == NULL ) {
        T_D("next_highIp == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    excluded.low_ip  = prev_lowIp  ? *prev_lowIp  : 0;
    excluded.high_ip = prev_highIp ? *prev_highIp : 0;

    if ( (rc = dhcp_server_excluded_get_next(&excluded)) != VTSS_RC_OK ) {
        return rc;
    }

    *next_lowIp  = excluded.low_ip;
    *next_highIp = excluded.high_ip;

    return VTSS_RC_OK;
}

/**
 * \brief Get excluded IP configuration entry
 *
 * To read excluded IP configuration of DHCP server.
 *
 * \param lowIp  [IN] (key 1) low IP address.
 * \param highIp [IN] (key 2) high IP address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_excluded_ip_entry_get(
    mesa_ipv4_t     lowIp,
    mesa_ipv4_t     highIp
)
{
    dhcp_server_excluded_ip_t excluded;

    excluded.low_ip  = lowIp;
    excluded.high_ip = highIp;

    return dhcp_server_excluded_get(&excluded);
}

/**
 * \brief Set excluded IP configuration entry
 *
 * To write excluded IP configuration of DHCP server.
 *
 * \param lowIp  [IN] (key 1) low IP address.
 * \param highIp [IN] (key 2) high IP address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_excluded_ip_entry_set(
    mesa_ipv4_t     lowIp,
    mesa_ipv4_t     highIp
)
{
    dhcp_server_excluded_ip_t excluded;
    mesa_rc                   rc;

    excluded.low_ip  = lowIp;
    excluded.high_ip = highIp;

    rc = dhcp_server_excluded_add( &excluded );

    switch ( rc ) {
    case VTSS_RC_OK:
    case VTSS_APPL_DHCP_SERVER_RC_ERR_DUPLICATE:
        return VTSS_RC_OK;

    default:
        return VTSS_RC_ERROR;
    }
}

/**
 * \brief Delete excluded IP configuration entry
 *
 * To remove excluded IP configuration of DHCP server.
 *
 * \param lowIp  [IN] (key 1) low IP address.
 * \param highIp [IN] (key 2) high IP address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_excluded_ip_entry_del(
    mesa_ipv4_t     lowIp,
    mesa_ipv4_t     highIp
)
{
    dhcp_server_excluded_ip_t excluded;

    excluded.low_ip  = lowIp;
    excluded.high_ip = highIp;

    return dhcp_server_excluded_delete(&excluded);
}

/**
 * \brief Iterate function of pool table
 *
 * To get first and get next indexes.
 *
 * \param prev_poolname [IN]  previous pool name.
 * \param next_poolname [OUT] next pool name.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_pool_entry_itr(
    const vtss_appl_dhcp_server_pool_name_t    *const prev_poolname,
    vtss_appl_dhcp_server_pool_name_t          *const next_poolname
)
{
    dhcp_server_pool_t pool;
    mesa_rc            rc;

    // check parameter
    if ( next_poolname == NULL ) {
        T_D("next_poolname == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    memset(&pool, 0, sizeof(dhcp_server_pool_t));

    if ( prev_poolname ) {
        // get next
        strcpy(pool.pool_name, prev_poolname->pool_name);
    }

    if ( (rc = dhcp_server_pool_get_next(&pool)) != VTSS_RC_OK ) {
        return rc;
    }

    strcpy(next_poolname->pool_name, pool.pool_name);

    return VTSS_RC_OK;
}

/**
 * \brief Get DHCP Pool Configuration
 *
 * To read configuration of DHCP pool.
 *
 * \param pool_name [IN]  (key) Pool name.
 * \param pool     [OUT] The configuration of Pool
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_pool_entry_get(
    vtss_appl_dhcp_server_pool_name_t           pool_name,
    vtss_appl_dhcp_server_config_pool_entry_t   *const pool
)
{
    dhcp_server_pool_t  p;
    u32                 i;
    mesa_rc             rc;

    // check parameter
    if ( pool == NULL ) {
        T_D("pool == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    memset( &p, 0, sizeof(dhcp_server_pool_t) );
    strcpy( p.pool_name, pool_name.pool_name );

    if ( (rc = dhcp_server_pool_get(&p)) != VTSS_RC_OK ) {
        return rc;
    }

    memset( pool, 0, sizeof(vtss_appl_dhcp_server_config_pool_entry_t) );

    pool->type            = p.type;
    pool->ip              = p.ip;
    pool->subnet_mask      = p.subnet_mask;
    pool->subnet_broadcast = p.subnet_broadcast;
    pool->lease_day        = p.lease / (24 * 60 * 60);
    pool->lease_hour       = ( p.lease % (24 * 60 * 60) ) / (60 * 60);
    pool->lease_minute     = ( p.lease % (60 * 60) ) / 60;
    pool->netbios_node_type = p.netbios_node_type;

    strcpy( pool->domain_name,    p.domain_name );
    strcpy( pool->netbios_scope,  p.netbios_scope );
    strcpy( pool->nis_domain_name, p.nis_domain_name );
    strcpy( pool->client_name,    p.client_name );

    memcpy( &(pool->client_haddr), &(p.client_haddr), sizeof(mesa_mac_t) );

    for ( i = 0; i < VTSS_APPL_DHCP_SERVER_SERVER_MAX_CNT; ++i ) {
        pool->default_router[i]     = p.default_router[i];
        pool->dns_server[i]         = p.dns_server[i];
        pool->ntp_server[i]         = p.ntp_server[i];
        pool->netbios_name_server[i] = p.netbios_name_server[i];
        pool->nis_server[i]         = p.nis_server[i];
    }

    pool->client_identifier_type = p.client_identifier.type;
    switch ( p.client_identifier.type ) {
    case VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NAME:
        strcpy( pool->client_identifier_name, p.client_identifier.u.name );
        break;

    case VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_MAC:
        memcpy( &(pool->client_identifier_mac), &(p.client_identifier.u.mac), sizeof(mesa_mac_t) );
        break;

    default:
        break;
    }

    for ( i = 0; i < VTSS_APPL_DHCP_SERVER_VENDOR_CLASS_INFO_CNT; ++i ) {
        strcpy( pool->vendor_class_info[i].class_id, p.class_info[i].class_id );
        (void)_hexval_str_get( p.class_info[i].specific_info, p.class_info[i].specific_info_len, pool->vendor_class_info[i].specific_info );
    }

#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
    pool->reserved_only = p.reserved_only;
    for ( i = 0; i < VTSS_APPL_DHCP_SERVER_RESERVED_CNT; i++) {
        pool->reserved[i] = p.reserved[i];
    }
#endif

    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
static BOOL operator==(const vtss_appl_dhcp_server_reserved_entry_t &a, const vtss_appl_dhcp_server_reserved_entry_t &b)
{
    return a.ifindex == b.ifindex && a.address == b.address;
}
#endif

/**
 * \brief Set DHCP Pool Configuration
 *
 * To add or modify configuration of DHCP pool.
 *
 * \param pool_name [IN] (key) Pool name.
 * \param pool     [IN] The configuration of Pool
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_pool_entry_set(
    vtss_appl_dhcp_server_pool_name_t                   pool_name,
    const vtss_appl_dhcp_server_config_pool_entry_t     *const pool
)
{
    dhcp_server_pool_t      p;
    u32                     i;
    u32                     j;
    u8                      specific_info[VTSS_APPL_DHCP_SERVER_VENDOR_SPECIFIC_INFO_LEN];
    u32                     specific_info_len;

    // check parameter
    if ( pool == NULL ) {
        T_D("pool == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    /* check lease time */
    if ( pool->lease_day > 365 ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( pool->lease_hour > 23 ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( pool->lease_minute > 59 ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    memset( &p, 0, sizeof(dhcp_server_pool_t) );

    strcpy( p.pool_name, pool_name.pool_name );

    p.type              = pool->type;
    p.ip                = pool->ip;
    p.subnet_mask       = pool->subnet_mask;
    p.subnet_broadcast  = pool->subnet_broadcast;
    p.lease             = pool->lease_day * 24 * 60 * 60 + pool->lease_hour * 60 * 60 + pool->lease_minute * 60;
    p.netbios_node_type = pool->netbios_node_type;

    strcpy( p.domain_name,     pool->domain_name    );
    strcpy( p.netbios_scope,   pool->netbios_scope  );
    strcpy( p.nis_domain_name, pool->nis_domain_name );
    strcpy( p.client_name,     pool->client_name    );

    memcpy( &(p.client_haddr), &(pool->client_haddr), sizeof(mesa_mac_t) );

    for ( i = 0; i < VTSS_APPL_DHCP_SERVER_SERVER_MAX_CNT; ++i ) {
        p.default_router[i]      = pool->default_router[i];
        p.dns_server[i]          = pool->dns_server[i];
        p.ntp_server[i]          = pool->ntp_server[i];
        p.netbios_name_server[i] = pool->netbios_name_server[i];
        p.nis_server[i]          = pool->nis_server[i];
    }

    if ( strlen(pool->client_identifier_name) &&
         pool->client_identifier_type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NAME ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( _not_empty_mac(&(pool->client_identifier_mac)) &&
         pool->client_identifier_type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_MAC ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    p.client_identifier.type = pool->client_identifier_type;
    switch ( pool->client_identifier_type ) {
    case VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE:
        break;

    case VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NAME:
        strcpy( p.client_identifier.u.name, pool->client_identifier_name );
        break;

    case VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_MAC:
        memcpy( &(p.client_identifier.u.mac), &(pool->client_identifier_mac), sizeof(mesa_mac_t) );
        break;

    default:
        return VTSS_RC_ERROR;
    }

    /* check hardware address is unicast.*/
    if (pool->client_haddr.addr[0] & 0x01) {
        //T_D("Invalid MAC address (Multicast)!");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    for ( i = 0; i < VTSS_APPL_DHCP_SERVER_VENDOR_CLASS_INFO_CNT; ++i ) {
        if ( strlen(pool->vendor_class_info[i].class_id) ) {
            // check if duplicate
            for ( j = 0; j < VTSS_APPL_DHCP_SERVER_VENDOR_CLASS_INFO_CNT; ++j ) {
                if ( i == j ) {
                    continue;
                }

                if ( strcmp(pool->vendor_class_info[i].class_id, pool->vendor_class_info[j].class_id) == 0 ) {
                    // duplicate class identifier
                    return VTSS_APPL_DHCP_SERVER_RC_ERR_DUPLICATE;
                }
            }
        }

        memset( specific_info, 0, sizeof(specific_info) );
        specific_info_len = 0;

        if ( strlen(pool->vendor_class_info[i].specific_info) ) {
            if ( strlen(pool->vendor_class_info[i].class_id) == 0 ) {
                // Class Identifier must have value first
                return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
            }

            if ( _hexval_get(pool->vendor_class_info[i].specific_info, specific_info, &specific_info_len) == FALSE ) {
                return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
            }
        }

        strcpy( p.class_info[i].class_id, pool->vendor_class_info[i].class_id );
        memcpy( p.class_info[i].specific_info, specific_info, VTSS_APPL_DHCP_SERVER_VENDOR_SPECIFIC_INFO_LEN );
        p.class_info[i].specific_info_len = specific_info_len;
    }

#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
    // Reserved entry processing:

    // 1. Check if reserved entries are configured: If yes, they must be contained in the pool subnet,
    //    and they must be unicast host addresses.
    //    We don't check if the reserved addresses are excluded from the pool; although it's
    //    probably a misconfiguration it could also be intentional, e.g. as a short-term administrative
    //    work-around to a problem in the network.
    // 2. Check that the pool isn't of type HOST; we can't have reserved entries there
    // 2. Check that no two addresses use the same interface
    // 3. Remove duplicate entries

    CapArray<u32, MEBA_CAP_BOARD_PORT_MAP_COUNT> pmap; // Port => index into pool->reserved[] of first entry that uses that port
    vtss_ifindex_elm_t elm;

    for (i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
        pmap[i] = VTSS_APPL_DHCP_SERVER_RESERVED_CNT;
    }

    for (i = 0; i < VTSS_APPL_DHCP_SERVER_RESERVED_CNT; i++) {
        p.reserved[i] = pool->reserved[i];

        if (pool->reserved[i].ifindex != VTSS_IFINDEX_NONE) {
            if (pool->type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST) {
                return VTSS_APPL_DHCP_SERVER_RC_ERR_POOL_TYPE_CONFLICT;
            }
            if (! vtss_dhcp_server_is_host_addr(pool->reserved[i].address, pool->subnet_mask)) {
                return VTSS_APPL_DHCP_SERVER_RC_ERR_IP;
            }
            if ((pool->ip & pool->subnet_mask) != (pool->reserved[i].address & pool->subnet_mask)) {
                return VTSS_APPL_DHCP_SERVER_RC_ERR_NOT_IN_SUBNET;
            }

            // Test for entry conflicts:
            (void)vtss_ifindex_decompose(pool->reserved[i].ifindex, &elm);
            VTSS_ASSERT(elm.ordinal < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT));
            if (pmap[elm.ordinal] == VTSS_APPL_DHCP_SERVER_RESERVED_CNT) {
                pmap[elm.ordinal] = i;
            } else {
                if (pool->reserved[i] == pool->reserved[pmap[elm.ordinal]]) {
                    // Duplicate entry: Clear
                    p.reserved[i].ifindex = VTSS_IFINDEX_NONE;
                    p.reserved[i].address = 0;
                } else {
                    return VTSS_APPL_DHCP_SERVER_RC_ERR_IFC_OCCUPIED;
                }
            }
        }
    }

    p.reserved_only = pool->reserved_only;

#endif // VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES

    // Perform configuration

    return dhcp_server_pool_set(&p);
}

/**
 * \brief Delete DHCP Pool Configuration
 *
 * To delete configuration of DHCP pool.
 *
 * \param pool_name [IN] (key) Pool name.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_pool_entry_del(
    vtss_appl_dhcp_server_pool_name_t   pool_name
)
{
    dhcp_server_pool_t p;

    memset( &p, 0, sizeof(dhcp_server_pool_t) );
    strcpy( p.pool_name, pool_name.pool_name );

    return dhcp_server_pool_delete(&p);
}

/**
 * \brief Iterate function of declined IP table
 *
 * To get first and get next indexes.
 *
 * \param prev_entryNo  [IN]  previous entry number of declined IP address.
 * \param next_entryNo  [OUT] next entry number of declined IP address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_status_declined_ip_entry_itr(
    const u32       *const prev_entryNo,
    u32             *const next_entryNo
)
{
    mesa_ipv4_t     declined_ip;
    u32             cnt;

    // check parameter
    if ( next_entryNo == NULL ) {
        T_D("next_entryNo == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    cnt = 0;
    declined_ip = 0;
    while ( dhcp_server_declined_get_next(&declined_ip) == VTSS_RC_OK ) {
        ++cnt;
    }

    // no any declined IP
    if ( cnt == 0 ) {
        return VTSS_RC_ERROR;
    }

    if ( prev_entryNo ) {
        // get next
        if ( (*prev_entryNo + 1) > cnt ) {
            return VTSS_RC_ERROR;
        }
        *next_entryNo = *prev_entryNo + 1;
    } else {
        // get first
        *next_entryNo = 1;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get declined IP entry
 *
 * To read declined IP entry of DHCP server.
 *
 * \param entryNo    [IN]  (key) entry number
 * \param declinedIp [OUT] Declined IP address
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_status_declined_ip_entry_get(
    u32                                                 entryNo,
    vtss_appl_dhcp_server_status_declined_ip_entry_t    *const declinedIp
)
{
    mesa_ipv4_t ip;
    u32         cnt;

    // check parameter
    if ( entryNo == 0 ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( declinedIp == NULL ) {
        T_D("declinedIp == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    cnt = 0;
    ip  = 0;
    while ( dhcp_server_declined_get_next(&ip) == VTSS_RC_OK ) {
        ++cnt;
        if ( cnt == entryNo ) {
            break;
        }
    }

    if ( cnt != entryNo ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_NOT_EXIST;
    }

    declinedIp->ip = ip;

    return VTSS_RC_OK;
}

/**
 * \brief Get statistics of DHCP server
 *
 * \param statistics [OUT] The statistics.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_status_statistics_get(
    vtss_appl_dhcp_server_status_statistics_t       *const statistics
)
{
    dhcp_server_statistics_t s;
    mesa_rc                  rc;

    // check parameter
    if ( statistics == NULL ) {
        T_D("statistics == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( (rc = dhcp_server_statistics_get(&s)) != VTSS_RC_OK ) {
        return rc;
    }

    statistics->discover_cnt = s.discover_cnt;
    statistics->offer_cnt    = s.offer_cnt;
    statistics->request_cnt  = s.request_cnt;
    statistics->ack_cnt      = s.ack_cnt;
    statistics->nak_cnt      = s.nak_cnt;
    statistics->decline_cnt  = s.decline_cnt;
    statistics->release_cnt  = s.release_cnt;
    statistics->inform_cnt   = s.inform_cnt;

    return VTSS_RC_OK;
}

/**
 * \brief Iterate function of binding table
 *
 * To get first and get next indexes.
 *
 * \param prev_ip  [IN]  previous binding IP address.
 * \param next_ip  [OUT] next binding IP address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_status_binding_entry_itr(
    const mesa_ipv4_t       *const prev_ip,
    mesa_ipv4_t             *const next_ip
)
{
    dhcp_server_binding_t binding;
    mesa_rc               rc;

    // check parameter
    if ( next_ip == NULL ) {
        T_D("next_ip == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    memset(&binding, 0, sizeof(dhcp_server_binding_t));

    if ( prev_ip ) {
        binding.ip = *prev_ip;
    }

    if ( (rc = dhcp_server_binding_get_next(&binding)) != VTSS_RC_OK ) {
        return rc;
    }

    *next_ip = binding.ip;

    return VTSS_RC_OK;
}

/**
 * \brief Get binding entry
 *
 * To read binding entry in DHCP server.
 *
 * \param ip      [IN]  (key) IP address of the binding
 * \param binding [OUT] binding entry
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_status_binding_entry_get(
    mesa_ipv4_t                                     ip,
    vtss_appl_dhcp_server_status_binding_entry_t    *const binding
)
{
    dhcp_server_binding_t db;
    mesa_rc               rc;

    // check parameter
    if ( binding == NULL ) {
        T_D("binding == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    db.ip = ip;
    if ( (rc = dhcp_server_binding_get(&db)) != VTSS_RC_OK ) {
        return rc;
    }

    memset( binding, 0, sizeof(vtss_appl_dhcp_server_status_binding_entry_t) );

    binding->subnet_mask = db.subnet_mask;
    binding->state      = db.state;
    binding->type       = db.type;
    binding->server_id   = db.server_id;
    binding->vid        = db.vid;

    strncpy( binding->pool_name, db.pool->pool_name, VTSS_APPL_DHCP_SERVER_POOL_NAME_LEN );

    memcpy( &(binding->chaddr), &(db.chaddr), sizeof(mesa_mac_t) );

    binding->client_identifier_type = db.identifier.type;
    switch ( db.identifier.type ) {
    case VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NAME:
        strcpy( binding->client_identifier_name, db.identifier.u.name );
        break;

    case VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_MAC:
        memcpy( &(binding->client_identifier_mac), &(db.identifier.u.mac), sizeof(mesa_mac_t) );
        break;

    default:
        break;
    }

    switch ( db.state ) {
    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_COMMITTED:
    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_ALLOCATED:
        if ( db.lease ) {
            (void)dhcp_server_day_str_get( db.lease, binding->lease_str );
            (void)dhcp_server_day_str_get( db.expire_time - dhcp_server_platform_current_time_get(), binding->time_to_expire_str );
        } else {
            sprintf(binding->lease_str, "infinite");
            sprintf(binding->time_to_expire_str, "-");
        }
        break;

    default:
        sprintf(binding->lease_str, "-");
        sprintf(binding->time_to_expire_str, "-");
        break;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get action of statistics
 *
 * \param control [OUT] The action.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_control_statistics_get(
    vtss_appl_dhcp_server_control_statistics_t          *const control
)
{
    // check parameter
    if ( control == NULL ) {
        T_D("control == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    memset( control, 0, sizeof(vtss_appl_dhcp_server_control_statistics_t) );

    return VTSS_RC_OK;
}

/**
 * \brief Set action to statistics
 *
 * \param control [IN] The action.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_control_statistics_set(
    const vtss_appl_dhcp_server_control_statistics_t    *const control
)
{
    // check parameter
    if ( control == NULL ) {
        T_D("control == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( control->clear ) {
        dhcp_server_statistics_clear();
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get action of binding
 *
 * \param control [OUT] The action.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_control_binding_get(
    vtss_appl_dhcp_server_control_binding_t         *const control
)
{
    // check parameter
    if ( control == NULL ) {
        T_D("control == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    memset( control, 0, sizeof(vtss_appl_dhcp_server_control_binding_t) );

    return VTSS_RC_OK;
}

/**
 * \brief Set action to binding
 *
 * \param control [IN] The action.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_control_binding_set(
    const vtss_appl_dhcp_server_control_binding_t   *const control
)
{
    dhcp_server_binding_t   db;

    // check parameter
    if ( control == NULL ) {
        T_D("control == NULL");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( control->clear_by_ip ) {
        db.ip = control->clear_by_ip;
        (void)dhcp_server_binding_delete( &db );
    }

    switch ( control->clear_by_type ) {
    case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_NONE:
        break;
    case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_AUTOMATIC:
    case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_MANUAL:
    case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_EXPIRED:
        dhcp_server_binding_clear_by_type( control->clear_by_type );
        break;
    default:
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
/**
 * \brief Iterate function of reserved adresses
 *
 * To get first and get next indexes.
 *
 * \param prev_poolname [IN]  previous pool name.
 * \param prev_address  [IN]  previous ip address.
 * \param next_poolname [OUT] next pool name.
 * \param next_address  [OUT] next ip address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_reserved_entry_itr(
    const vtss_appl_dhcp_server_pool_name_t    *const prev_poolname,
    vtss_appl_dhcp_server_pool_name_t          *const next_poolname,
    const mesa_ipv4_t                          *const prev_ip_address,
    mesa_ipv4_t                                *const next_ip_address
)
{
    VTSS_ASSERT(next_poolname != nullptr);
    VTSS_ASSERT(next_ip_address != nullptr);

    dhcp_server_pool_t  pool;
    mesa_ipv4_t         ip_address;

    if (prev_poolname == nullptr) {
        memset(&pool, 0, sizeof(dhcp_server_pool_t));
    } else {
        strncpy(pool.pool_name, prev_poolname->pool_name, sizeof(pool.pool_name));
    }

    if (prev_ip_address == nullptr) {
        ip_address = 0;
    } else {
        ip_address = *prev_ip_address;
    }

    if ( dhcp_server_pool_get(&pool) == VTSS_RC_OK ) {
        mesa_ipv4_t tmp = 0xFFFFFFFF;
        bool found_one = false;
        for (int i = 0; i < VTSS_APPL_DHCP_SERVER_RESERVED_CNT; ++i) {
            if (pool.reserved[i].ifindex != VTSS_IFINDEX_NONE &&
                pool.reserved[i].address > ip_address &&
                pool.reserved[i].address <= tmp) {
                tmp = pool.reserved[i].address;
                found_one = true;
            }
        }
        if (found_one) {
            strncpy(next_poolname->pool_name, pool.pool_name, sizeof(next_poolname->pool_name));
            *next_ip_address = tmp;
            return VTSS_RC_OK;
        }
    }
    while ( dhcp_server_pool_get_next(&pool) == VTSS_RC_OK ) {
        ip_address = 0;
        mesa_ipv4_t tmp = 0xFFFFFFFF;
        bool found_one = false;
        for (int i = 0; i < VTSS_APPL_DHCP_SERVER_RESERVED_CNT; ++i) {
            if (pool.reserved[i].ifindex != VTSS_IFINDEX_NONE &&
                pool.reserved[i].address >= ip_address &&
                pool.reserved[i].address <= tmp) {
                tmp = pool.reserved[i].address;
                found_one = true;
            }
        }
        if (found_one) {
            strncpy(next_poolname->pool_name, pool.pool_name, sizeof(next_poolname->pool_name));
            *next_ip_address = tmp;
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

/**
 * \brief Get reserved address entry
 *
 * To read a reserved entry
 *
 * \param poolname  [IN]  Pool name.
 * \param address   [IN]  Ip address.
 * \param interface [IN]  Interface for which the address is reserved
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_reserved_entry_get(
    const vtss_appl_dhcp_server_pool_name_t    *const poolname,
    const mesa_ipv4_t                          *const ip_address,
    vtss_ifindex_t                             *const ifindex
)
{
    VTSS_ASSERT(poolname != nullptr);
    VTSS_ASSERT(ip_address != nullptr);

    dhcp_server_pool_t  pool;

    strncpy(pool.pool_name, poolname->pool_name, sizeof(pool.pool_name));

    if ( dhcp_server_pool_get(&pool) == VTSS_RC_OK ) {
        for (int i = 0; i < VTSS_APPL_DHCP_SERVER_RESERVED_CNT; ++i) {
            if (pool.reserved[i].ifindex != VTSS_IFINDEX_NONE &&
                pool.reserved[i].address == *ip_address) {
                *ifindex = pool.reserved[i].ifindex;
                return VTSS_RC_OK;
            }
        }
    }
    return VTSS_RC_ERROR;
}

/**
 * \brief Set reserved address entry
 *
 * To set a reserved entry
 *
 * \param poolname  [IN]  Pool name.
 * \param address   [IN]  Ip address.
 * \param interface [IN]  Interface for which the address is reserved
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_reserved_entry_set(
    const vtss_appl_dhcp_server_pool_name_t    *const poolname,
    const mesa_ipv4_t                          *const ip_address,
    const vtss_ifindex_t                       *const ifindex
)
{
    VTSS_ASSERT(poolname != nullptr);
    VTSS_ASSERT(ip_address != nullptr);
    VTSS_ASSERT(ifindex != nullptr);

    vtss_appl_dhcp_server_config_pool_entry_t pool;
    mesa_rc                                   rc;

    if (! vtss_ifindex_is_port(*ifindex)) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( (rc = vtss_appl_dhcp_server_config_pool_entry_get(*poolname, &pool)) != VTSS_RC_OK ) {
        return rc;
    }

    int i, free_idx = VTSS_APPL_DHCP_SERVER_RESERVED_CNT;
    for (i = 0; i < VTSS_APPL_DHCP_SERVER_RESERVED_CNT; ++i) {
        if (pool.reserved[i].ifindex == VTSS_IFINDEX_NONE && free_idx == VTSS_APPL_DHCP_SERVER_RESERVED_CNT) {
            free_idx = i;
        } else if (pool.reserved[i].ifindex != VTSS_IFINDEX_NONE &&
                   pool.reserved[i].address == *ip_address) {
            pool.reserved[i].ifindex = *ifindex;
            break;
        }
    }
    if (i == VTSS_APPL_DHCP_SERVER_RESERVED_CNT) {
        // No existing entry
        if (free_idx == VTSS_APPL_DHCP_SERVER_RESERVED_CNT) {
            return VTSS_APPL_DHCP_SERVER_RC_ERR_FULL;
        }
        pool.reserved[free_idx].ifindex = *ifindex;
        pool.reserved[free_idx].address = *ip_address;
    }

    return vtss_appl_dhcp_server_config_pool_entry_set(*poolname, &pool);
}

/**
 * \brief Delete reserved address entry
 *
 * To delete a reserved entry
 *
 * \param poolname  [IN]  Pool name.
 * \param address   [IN]  Ip address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_server_config_reserved_entry_del(
    const vtss_appl_dhcp_server_pool_name_t    *const poolname,
    const mesa_ipv4_t                          *const ip_address
)
{
    VTSS_ASSERT(poolname != nullptr);
    VTSS_ASSERT(ip_address != nullptr);

    vtss_appl_dhcp_server_config_pool_entry_t pool;
    mesa_rc                                   rc;

    if ( (rc = vtss_appl_dhcp_server_config_pool_entry_get(*poolname, &pool)) != VTSS_RC_OK ) {
        return rc;
    }

    for (int i = 0; i < VTSS_APPL_DHCP_SERVER_RESERVED_CNT; ++i) {
        if (pool.reserved[i].ifindex != VTSS_IFINDEX_NONE &&
            pool.reserved[i].address == *ip_address) {

            pool.reserved[i].ifindex = VTSS_IFINDEX_NONE;
            pool.reserved[i].address = 0;
            return vtss_appl_dhcp_server_config_pool_entry_set(*poolname, &pool);
        }
    }

    // Didn't find an entry: error
    return VTSS_RC_ERROR;
}
#endif // VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
