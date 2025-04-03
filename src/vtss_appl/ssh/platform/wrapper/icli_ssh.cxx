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

#include "main.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/stat.h>

#include <termios.h>

#define ICLI_SSH_SESSION_CNT 4
static BOOL writestr(int conn, const char *buf, const char *def)
{
    size_t len;
    // fallback to default
    if (buf == NULL)
        buf = def;
    // Write string with "\n"
    len = strlen(buf);
    if (write(conn, buf, len) != len || write(conn, "\n", 1) != 1) {
        fprintf(stderr, "write(%s): %s\n", buf, strerror(errno));
        return FALSE;
    }
    return TRUE;
}

/* This function returns how many ICLI sessions are used by ssh. */
static int get_icli_ssh_session_count(void)
{
    char output_str[16];
    int num = -1;
    FILE *pipe = NULL;

    if ((pipe = popen("netstat -x | grep icli.socket | wc -l 2>&1", "r")) == NULL) {
        perror("open pipe fail:");
        return -1;
    }
    if (fgets(output_str, sizeof(output_str), pipe) == NULL) {
        perror("get pipe data fail:");
        pclose(pipe);
        return -2;
    }

    if(sscanf(output_str, "%d", &num) != 1) {
        perror("data format mismatch");
        pclose(pipe);
        return -3;
    }

    pclose(pipe);
    return num;
}

int main(int argc, char *argv[])
{
    const char *path = VTSS_FS_RUN_DIR "icli.socket";
    int srvr_sock;
    char buffer[1024];
    ssize_t i;
    struct sockaddr_un saddr;
    fd_set rfds;
    int ret, tty;
    struct termios raw_mode;

    if (get_icli_ssh_session_count() >= ICLI_SSH_SESSION_CNT) {
        fprintf(stderr, "Only %d connections allowed\n", ICLI_SSH_SESSION_CNT);
        return -1;
    }

    /* Create the socket: */
    if((srvr_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    /* Bind a name to the socket: */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sun_family = AF_UNIX;
    strncpy(saddr.sun_path, path, sizeof(saddr.sun_path));

    // Connect
    if (connect(srvr_sock, (struct sockaddr*)&saddr, sizeof(saddr)) < 0) {
        fprintf(stderr, "connect(%s): %s\n", path, strerror(errno));
        close(srvr_sock);
        return -1;
    }

    // Write current username / auth / connection
    if (!writestr(srvr_sock, getenv("LOGNAME"), "admin")) {
        close(srvr_sock);
        return -1;
    }
    if (!writestr(srvr_sock, getenv("AUTHRESP"), "")) {
        close(srvr_sock);
        return -1;
    }
    if (!writestr(srvr_sock, getenv("SSH_CONNECTION"), "unknown")) {
        close(srvr_sock);
        return -1;
    }

    // Non-block/raw stdin
    tty = fileno(stdin);
    (void) fcntl(tty, F_SETFL, fcntl(tty, F_GETFL) | O_NONBLOCK);
    memset(&raw_mode, 0, sizeof(raw_mode));
    tcgetattr(tty, &raw_mode);
    cfmakeraw(&raw_mode);
    raw_mode.c_oflag |= OPOST;
    tcsetattr(tty, TCSANOW, &raw_mode);

    while (true) {
        FD_ZERO(&rfds);
        FD_SET(tty, &rfds);
        FD_SET(srvr_sock, &rfds);
        if((ret = select(MAX(tty, srvr_sock) + 1, &rfds, NULL, NULL, NULL)) > 0) {

            // SSH => ICLI
            if (FD_ISSET(tty, &rfds)) {
                if ((i = read(tty, buffer, sizeof(buffer))) > 0) {
                    if(write(srvr_sock, buffer, i) != i) {
                        fprintf(stderr, "write(server): %s\n", strerror(errno));
                        close(srvr_sock);
                        return -1;
                    }
                } else {
                    close(srvr_sock);
                    // Closed ssh side
                    return 0;
                }
            }

            // ICLI => SSH
            if (FD_ISSET(srvr_sock, &rfds)) {
                if ((i = read(srvr_sock, buffer, sizeof(buffer))) > 0) {
                    if(write(1, buffer, i) != i) {
                        fprintf(stderr, "write(ssh): %s\n", strerror(errno));
                        close(srvr_sock);
                        return -1;
                    }
                } else {
                    close(srvr_sock);
                    // Closed icli side
                    return 0;
                }
            }
        } else {
            fprintf(stderr, "select(): Error - %s?\n", strerror(errno));
        }
    }

    close(srvr_sock);
    // Not reached
    return 0;
}
