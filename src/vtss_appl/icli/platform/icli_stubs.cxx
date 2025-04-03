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
/*
******************************************************************************

    Include File

******************************************************************************
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "icli_api.h"
#include "icli_os.h"
#include "icli_porting_trace.h"
#include "mgmt_api.h"

#include "vtss/appl/users.h"

#include <sys/un.h>
#include <sys/stat.h>

/*
******************************************************************************

    Constant and Macro

******************************************************************************
*/
#define _WAITING_SLICE          100     // milli-sec
#define _DEBUG_MSG              0

/*
******************************************************************************

    Type Definition

******************************************************************************
*/

/*
******************************************************************************

    Static Variable

******************************************************************************
*/
static int  g_client_id[ ICLI_SESSION_CNT ];
static u32  g_session_id[ ICLI_SESSION_CNT ];

static vtss_handle_t ssh_handle;

/*
******************************************************************************

    Static Function

******************************************************************************
*/
static void _exit_thread(
    IN  u32     app_id
)
{
#if _DEBUG_MSG
    printf("exit thread %u\n", app_id);
#endif

    T_I("Close session %u", app_id);

    /* close client socket */
    icli_close_socket( g_client_id[app_id] );

    /* clear database */
    g_client_id[app_id] = -1;

    /* close session */
    (void)icli_session_close( g_session_id[app_id] );
}

/*
    get session input by char

    INPUT
        app_id    : application ID
        wait_time : in millisecond
                    = 0 - no wait
                    < 0 - forever

    OUTPUT
        c : char inputted

    RETURN
        TRUE  : successful
        FALSE : failed due to timeout
*/
static BOOL _stubs_char_get(
    IN  icli_addrword_t     app_id,
    IN  i32                 wait_time,
    OUT i32                 *c
)
{
    struct timeval      tv;
    fd_set              fs;
    i32                 n;
    u32                 t;
    int                 r;
    char                ch;

    if ( app_id >= ICLI_SESSION_CNT ) {
        return FALSE;
    }

    if ( g_client_id[app_id] == -1 ) {
        return FALSE;
    }

    // get milli-seconds for waiting
    t = _WAITING_SLICE;
    if ( wait_time == 0 ) {
        // no wait
        n = 1;
        t = 0;
    } else if ( wait_time < 0 ) {
        n = -1;
    } else {
        n = wait_time / _WAITING_SLICE;
        if ( wait_time % _WAITING_SLICE ) {
            n++;
        }
    }

    while ( n ) {
        /* socket set */
        FD_ZERO( &fs );
        FD_SET( (unsigned int)(g_client_id[app_id]), &fs );

        /* timer */
        tv.tv_sec  = t / 1000;
        tv.tv_usec = (t % 1000) * 1000;

        /* select and wait */
        r = select(g_client_id[app_id] + 1, &fs, NULL, NULL, &tv);

        switch ( r ) {
        case 1: // There is something to read
            r = recv( g_client_id[app_id], &ch, 1, 0 );
            if ( r != 1 ) {
                /* exit the thread */
                _exit_thread( app_id );
                return FALSE;
            }
            *c = (i32)ch;
#if _DEBUG_MSG
            printf("get: %x\n", ch);
#endif
            return TRUE;

        case 0: // Timeout
            if ( n > 0 ) {
                --n;
            }
            break;

        default: // socket error
            /* exit the thread */
            _exit_thread( app_id );
            return FALSE;
        }
    }
    return FALSE;
}

/*
    output/display one char on session

    INPUT
        app_id : application ID
        c      : the char for output/display

    OUTPUT
        n/a

    RETURN
        TRUE  : successful
        FALSE : failed
*/
static BOOL _stubs_char_put(
    IN  icli_addrword_t     app_id,
    IN  char                ch
)
{
    if ( app_id >= ICLI_SESSION_CNT ) {
        return FALSE;
    }

    if ( g_client_id[app_id] == -1 ) {
        return FALSE;
    }

    if ( send(g_client_id[app_id], &ch, 1, 0) != 1 ) {
        return FALSE;
    }

#if _DEBUG_MSG
    printf("put: %x\n", ch);
#endif

    return TRUE;
}

/*
    close APP session

    INPUT
        app_id : application ID
        reason : why the session is closed

    OUTPUT
        n/a

    RETURN
        n/a
*/
static void _stubs_app_close(
    IN  icli_addrword_t                 app_id,
    IN  icli_session_close_reason_t     reason
)
{
#if _DEBUG_MSG
    printf("_stubs_app_close: %u\n", app_id);
#endif

    T_I("_stubs_app_close: %d", (int) app_id);

    if ( app_id >= ICLI_SESSION_CNT ) {
        return;
    }

    if ( g_client_id[app_id] == -1 ) {
        return;
    }

    /* close client socket */
    icli_close_socket( g_client_id[app_id] );

    /* clear database */
    g_client_id[app_id] = -1;
}

static BOOL ssh_gets(int conn, char *buf, size_t maxbuf)
{
    char c;
    int  i = 0;
    while (true) {
        if (read(conn, &c, 1) == 1) {
            if (i < maxbuf) {
                buf[i++] = c;
            }
            // On newline - chomp, return TRUE
            if (c == '\n') {
                buf[i - 1] = '\0';
                return TRUE;
            }
        } else {
            T_E("ssh connection error");
            return FALSE;
        }
    }
    // Not reached
    return FALSE;
}

/*
    open an ICLI session

    INPUT
        client_id : client socket ID

    OUTPUT
        n/a

    RETURN
        TRUE  : successful
        FALSE : failed
*/
static BOOL _icli_session_open(
    IN int  client_id
)
{
    i32                         i;
    i32                         rc;
    u32                         session_id;
    icli_session_open_data_t    open_data;
    char                        user[VTSS_APPL_USERS_NAME_LEN + 1], authstring[256], sshconn[256], *p;
    int                         port, priv_lvl, agent_id;

    // Read username
    if (!ssh_gets(client_id, user, sizeof(user)) ||
        !ssh_gets(client_id, authstring, sizeof(authstring)) ||
        !ssh_gets(client_id, sshconn, sizeof(sshconn))) {
        return FALSE;
    }

    if (sscanf(authstring, "ACCEPTED LEVEL=%d ID=%d", &priv_lvl, &agent_id) != 2) {
        T_W("garbled autstring: %s", authstring);
        priv_lvl = ICLI_PRIVILEGE_1;
    }

    T_I("SSH connection from %s credentials %s through %s", user, authstring, sshconn);

    /* find empty connection */
    for ( i = 0; i < ICLI_SESSION_CNT; ++i ) {
        if ( g_client_id[i] == -1 ) {
            break;
        }
    }

    /* connection full */
    if ( i == ICLI_SESSION_CNT ) {
        T_W("SSH connection table full, dropping client");
        return FALSE;
    }

    /* Open ICLI session */
    memset(&open_data, 0, sizeof(open_data));

    open_data.name      = "ICLI Stub";
    open_data.way       = ICLI_SESSION_WAY_SSH;
    open_data.app_id    = (u32)i;
    // split at space
    if ((p = strchr(sshconn, ' '))) {
        *p = '\0';
        if (sscanf(p + 1, "%d", &port)) {
            open_data.client_port = port;
        }
    }
    if (mgmt_txt2ipv4(sshconn, &open_data.client_ip.addr.ipv4, NULL, FALSE) == VTSS_RC_OK) {
        T_I("ssh IPv4: %s", sshconn);
        open_data.client_ip.type = MESA_IP_TYPE_IPV4;
    }
#ifdef VTSS_SW_OPTION_IPV6
    else if (mgmt_txt2ipv6(sshconn, &open_data.client_ip.addr.ipv6) == VTSS_RC_OK) {
        T_I("ssh IPv6: %s", sshconn);
        open_data.client_ip.type = MESA_IP_TYPE_IPV6;
    }
#endif
    else {
        T_E("ssh address error: %s", sshconn);
        open_data.client_ip.type = MESA_IP_TYPE_NONE;
    }

    /* I/O callback */
    open_data.char_get  = _stubs_char_get;
    open_data.char_put  = _stubs_char_put;
    open_data.client_fd = client_id;
    open_data.client_need_cli_iolayer_in_thread_data =
        CLIENT_NEED_CLI_IOLAYER_IN_THREAD_DATA_MAGIC;

    /* APP session callback */
    open_data.app_close = _stubs_app_close;

    rc = icli_session_open(&open_data, &session_id);
    if ( rc != ICLI_RC_OK ) {
        T_E("fail to open a session for console\n");
        return FALSE;
    }

    /* update database */
    g_client_id[i] = client_id;
    g_session_id[i] = session_id;

    // Establish credentials
    if (icli_session_privilege_set(session_id, (icli_privilege_t) priv_lvl) != ICLI_RC_OK) {
        T_E("Fail to set priv_lvl %u to ICLI session %d\n", priv_lvl, session_id);
    }
    if (icli_session_agent_id_set(session_id, (u16) agent_id) != ICLI_RC_OK) {
        T_E("Fail to set agent_id %u to ICLI session %d\n", i, session_id);
    }
    if (icli_session_user_name_set(session_id, user) != ICLI_RC_OK) {
        T_E("Fail to set user name %s to ICLI session %d", user, session_id);
    }


    //NOTE: This code was part of icli_session_open() above but was
    //moved here to avoid a race condition
    rc = icli_session_begin(&open_data, session_id);
    if ( rc != ICLI_RC_OK ) {
        T_E("fail to begin a session for console\n");
        return FALSE;
    }

    return TRUE;
}

/*
    Stub server thread to listen stub client connection request

    INPUT
        session_id : no used actually

    OUTPUT
        n/a

    RETURN
        Should never return, otherwise, failed
*/
static void _stubs_thread(
    vtss_addrword_t data
)
{
    int                 server_id;
    int                 addr_size;
    struct sockaddr_in  client_addr;
    int                 client_id;
    BOOL                b_loop;
    i32                 i;
    const char *path = VTSS_FS_RUN_DIR "icli.socket";
    struct sockaddr_un saddr;

    /* With eMMC files created in /run are created in the filesystem and will be there
       after reboot. Sockets must be deleted before they can be created again (JIRA-3500) */
    (void)remove(path);

    /* create socket */
    server_id = socket(AF_UNIX, SOCK_STREAM, 0);
    if ( server_id == -1 ) {
        return;
    }

    /* Bind a name to the socket: */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sun_family = AF_UNIX;
    strncpy(saddr.sun_path, path, sizeof(saddr.sun_path));

    /* bind */
    if ( bind(server_id, (struct sockaddr *)&saddr, sizeof(saddr)) == -1 ) {
        perror("bind()");
        icli_close_socket( server_id );
        return;
    }

    (void) chmod(path, 0776);

    /* listen */
    listen( server_id, ICLI_SESSION_CNT );

    /*
        In a continous loop, wait for incoming clients. Once one is detected,
        create an ICLI session to handle it.
    */
    b_loop = TRUE;
    while ( b_loop ) {
        /* wait for new connection */
        addr_size = sizeof(client_addr);
        client_id = accept(server_id, (struct sockaddr *)&client_addr, (socklen_t *)&addr_size);
        if ( client_id == -1 ) {
            perror("accept()");
            b_loop = FALSE;
            continue;
        }

        /* create ICLI session */
        if ( _icli_session_open(client_id) == FALSE ) {
            // connection full
            icli_close_socket( client_id );
        }
    }

    /* close server socket */
    icli_close_socket( server_id );

    /* close client socket */
    for ( i = 0; i < ICLI_SESSION_CNT; ++i ) {
        if ( g_client_id[i] >= 0 ) {
            icli_close_socket( g_client_id[i] );
            g_client_id[i] = -1;
        }
    }
}

/*
******************************************************************************

    Public Function

******************************************************************************
*/
/*
    start ICLI Stub Server
*/
BOOL icli_stubs_start(
    void
)
{
    i32     i;

    for ( i = 0; i < ICLI_SESSION_CNT; ++i ) {
        g_client_id[i] = -1;
    }

    vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                       _stubs_thread,
                       0,
                       "ICLI SSH Server",
                       nullptr,
                       0,
                       &ssh_handle,
                       0);

    return TRUE;
}
