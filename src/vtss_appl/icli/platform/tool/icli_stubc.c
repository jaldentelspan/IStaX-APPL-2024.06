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

/*
==============================================================================

    Revision history
    > CP.Wang, 12/25/2013 16:31
        - create

==============================================================================
*/

/*
==============================================================================

    Include File

==============================================================================
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <termios.h>
#include <unistd.h>

#include "icli_def.h"       // for stub server port

/*
==============================================================================

    Constant and Macro

==============================================================================
*/
#define _close_socket           close
#define _DEBUG_MSG              0

/*
==============================================================================

    Type Definition

==============================================================================
*/
typedef i32 _thread_entry_cb_t(
    IN int      entry_arg
);

typedef void *(*start_routine)(void *);

/*
==============================================================================

    Static Variable

==============================================================================
*/
static int              g_socket_id;
static u32              g_console_id;
static struct termios   g_old_term;

/*
==============================================================================

    Static Function

==============================================================================
*/
static char _get_char(
    void
)
{
    struct termios  newt;
    int             ch;

    /* get original terminal */
    tcgetattr( STDIN_FILENO, &g_old_term );

    /* set new terminal */
    newt = g_old_term;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt );

    /* get char */
    ch = getchar();
    if ( ch == 0x7f ) {
        ch = 8;
    }

    /* restore terminal */
    tcsetattr( STDIN_FILENO, TCSANOW, &g_old_term );
    return (char)ch;
}

static BOOL _thread_create(
    IN  _thread_entry_cb_t  *entry_cb,
    IN  long                entry_arg
)
{
    pthread_t   t;

    if ( pthread_create(&t, NULL, (start_routine)entry_cb, (void *)entry_arg) != 0 ) {
        return FALSE;
    }
    return TRUE;
}

/*
    thread to process the input from console to stub server
*/
static i32 _client_to_server_thread(
    IN int      session_id
)
{
    BOOL    b_loop;
    int     r;
    char    ch;

#if _DEBUG_MSG
    printf("Start _client_to_server_thread\n");
#endif
    
    if ( session_id );

    b_loop = TRUE;
    while( b_loop ) {
        /* send input to stub server */
        ch = _get_char();
        r = send( g_socket_id, &ch, 1, 0 );
        if ( r != 1 ) {
            /* something wrong */
            b_loop = FALSE;
        }
    }

    _close_socket( g_socket_id );
    g_socket_id = -1;
    tcsetattr( STDIN_FILENO, TCSANOW, &g_old_term );
    exit( 0 );
}

/*
    thread to process the output from stub server to console
*/
static i32 _server_to_client_thread(
    IN int      session_id
)
{
    BOOL    b_loop;
    int     r;
    char    ch;

#if _DEBUG_MSG
    printf("Start _server_to_client_thread\n");
#endif

    if ( session_id );

    b_loop = TRUE;
    while( b_loop ) {
        r = recv( g_socket_id, &ch, 1, 0 );
        if ( r != 1 ) {
            b_loop = FALSE;
            break;
        }

        // put to console
        putchar( ch );
        fflush( stdout );
    }

    _close_socket( g_socket_id );
    g_socket_id = -1;
    tcsetattr( STDIN_FILENO, TCSANOW, &g_old_term );
    exit( 0 );
}

/*
==============================================================================

    Public Function

==============================================================================
*/
int main(i32 argc, char *argv[])
{
    struct sockaddr_in  server_addr;

    /* create socket */
    g_socket_id = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( g_socket_id == -1 ) {
        perror("socket()");
        return -1;
    }

    /* server address to connect */
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port        = htons(ICLI_STUB_SERVER_PORT);

    if ( connect(g_socket_id, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1 ) {
        perror("connect()");
        _close_socket( g_socket_id );
        return -1;
    }

    /* create thread */
    if ( _thread_create(_client_to_server_thread, 0) == FALSE ) {
        _close_socket( g_socket_id );
        return -1;
    }

    /* create thread */
    if ( _thread_create(_server_to_client_thread, 0) == FALSE ) {
        _close_socket( g_socket_id );
        return -1;
    }

    while( g_socket_id >= 0 ) {
        usleep( 100000 );
    }

    tcsetattr( STDIN_FILENO, TCSANOW, &g_old_term );
    return 0;
}
