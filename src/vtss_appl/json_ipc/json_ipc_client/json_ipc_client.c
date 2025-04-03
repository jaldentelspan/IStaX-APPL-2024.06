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

#include <unistd.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>

static int trace_enabled, long_id;

static void trace_printf(const char *format, ...)
{
    va_list va;

    if (trace_enabled) {
        printf("json_ipc_client: ");
        va_start(va, format);
        vprintf(format, va);
        va_end(va);
        printf("\n");
    }
}

#define T_I(...) trace_printf(__VA_ARGS__)

/* Message format: 4 bytes length field followed by data */
#define JSON_IPC_HDR_LEN 4

#define JSON_ID_SIZE 4000000

static char msg[1024 + JSON_ID_SIZE];
static char line[1024];

static int read_cmd(int s, char *line)
{
    size_t     len = 0;
    int        done = 0, quote = 0, n;
    char       *p, *word = NULL, *cmd = NULL, *method = NULL, *param = NULL;

    for (p = line; !done; p++) {
        if (*p == '\n') {
            done = 1;
        } else if (*p == '"') {
            if (quote) {
                /* Exit quote mode */
                quote = 0;
                if (word == NULL) {
                    /* Start of word (handle "") */
                    word = p;
                }
            } else {
                /* Enter quote mode */
                quote = 1;
                continue;
            }
        } else if (*p != ' ') {
            if (word == NULL) {
                /* Start of word */
                word = p;
            }
            continue;
        }

        if (quote) {
            /* Inside quotes */
            continue;
        }
        *p = '\0';
        if (word == NULL) {
            continue;
        }

        /* First parameter: Command */
        if (cmd == NULL) {
            cmd = word;
            word = NULL;
            continue;
        }

        /* Second parameter: Method/event */
        if (method == NULL) {
            if (strcmp(cmd, "call") == 0) {
                /* Call JSON method */
                method = word;
            } else if (strcmp(cmd, "add") == 0) {
                /* Add JSON notification registration */
                method = "jsonIpc.config.notification.add";
                param = word;
            } else if (strcmp(cmd, "del") == 0) {
                /* Delete JSON notification registration */
                method = "jsonIpc.config.notification.del";
                param = word;
            } else {
                done = 1;
            }
        } else if (param == NULL) {
            param = word;
        }
        word = NULL;
    }

    if (method == NULL) {
        printf("Commands:\n");
        printf("call <method> [<param>]\n");
        printf("add <event>\n");
        printf("del <event>\n");
        return 0;
    }

    /* Build JSON message */
    p = &msg[JSON_IPC_HDR_LEN];
    p += sprintf(p, "{\"method\":\"%s\",\"params\":[", method);
    if (param != NULL) {
        p += sprintf(p, "\"%s\"", param);
    }
    p += sprintf(p, "],\"id\":\"");
    if (long_id) {
        memset(p, 'q', JSON_ID_SIZE);
        p += JSON_ID_SIZE;
    } else {
        p += sprintf(p, "%s", "json_ipc");
    }
    p += sprintf(p, "\"}");
    p = msg;
    n = strlen(p + JSON_IPC_HDR_LEN);
    *(uint32_t *)p = n;
    len = (n + JSON_IPC_HDR_LEN);
    while (len) {
        if ((n = write(s, p, len)) < 0) {
            perror("write");
            return 1;
        }
        p += len;
        len -= n;
    }
    return 0;
}

static int read_input(int s)
{
    char *p = line;
    int i, n;

    if ((n = read(STDIN_FILENO, line, sizeof(line))) <= 0) {
        perror("read");
        return 1;
    }

    for (i = 0; i < n; i++) {
        if (line[i] == '\n') {
            if (read_cmd(s, p)) {
                return 1;
            }
            p = &line[i + 1];
        }
    }
    return 0;
}

static int read_response(int s, int *cnt)
{
    int  n, c;
    char *p = msg;

    /* Show JSON response */
    if ((n = read(s, msg, sizeof(msg) - 1)) < 0) {
        perror("read");
        return 1;
    }

    p[n] = '\0';
    while (n) {
        if (*cnt == 0) {
            /* Parse header */
            if (n < JSON_IPC_HDR_LEN) {
                printf("tiny message\n");
                return 1;
            }
            *cnt = *(uint32_t *)p;
            n -= JSON_IPC_HDR_LEN;
            p += JSON_IPC_HDR_LEN;
            printf("Response[%u]: ", *cnt);
        }
        if (n > *cnt) {
            /* Buffer holds multiple responses */
            c = p[*cnt];
            p[*cnt] = '\0';
            printf("%s\n", p);
            p[*cnt] = c;
            n -= *cnt;
            p += *cnt;
            *cnt = 0;
        } else {
            *cnt -= n;
            printf("%s%s", p, *cnt == 0 ? "\n" : "");
            n = 0;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int                s, cnt = 0, opt;
    struct sockaddr_un remote;
    fd_set             fd_read, fd_active;
    
    while ((opt = getopt(argc, argv, "it")) != -1) {
        switch (opt) {
        case 'i':
            long_id = 1;
            break;
        case 't':
            trace_enabled = 1;
            break;
        default:
            printf("Usage: %s [-i] [-t]\n", argv[0]);
            return 1;
        }
    }

    T_I("creating socket");
    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    T_I("connecting");
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, "/var/run/json_ipc.socket");
    if (connect(s, (struct sockaddr *)&remote, sizeof(remote.sun_family) + strlen(remote.sun_path)) < 0) { 
        perror("connect");
        return 1;
    }
    T_I("connected");

    FD_ZERO(&fd_active);
    FD_SET(STDIN_FILENO, &fd_active);
    FD_SET(s, &fd_active);

    while (1) {
        fd_read = fd_active;
        if (select(s + 1, &fd_read, NULL, NULL, NULL) < 0) {
            perror("select");
            return 1;
        }

        /* Parse input */
        if (FD_ISSET(STDIN_FILENO, &fd_read) && read_input(s)) {
            return 1;
        }

        /* Parse responses */
        if (FD_ISSET(s, &fd_read) && read_response(s, &cnt)) {
            return 1;
        }
    } /* while(1) */

    T_I("exit");
    
    return 0;
}
