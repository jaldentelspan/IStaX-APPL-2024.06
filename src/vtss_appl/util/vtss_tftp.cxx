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

#include <stdio.h>
#include "main.h"
#include "vtss_os_wrapper.h"
#include "vtss_tftp_api.h"
#include "vtss/basics/notifications/process-cmd.hxx"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_SYSTEM /* Pick one... */

#include "vtss_trace_api.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

mesa_rc vtss_tftp_err(const char *str_errbuf)
{
    if (strlen(str_errbuf) > 0) {
        const char *p, *needle = "server error: (";
        if ((p = strstr(str_errbuf, needle)) != NULL) {
            int rc;
            if (sscanf(p + strlen(needle), "%d", &rc) == 1 &&
                rc >= 0 && rc <= 8) {
                if (rc == 1) {
                    /* It means VTSS_TFTP_ENOTFOUND, but Tracker-1740 want to
                     * tell 'Invalid Path' or 'Invalid Filename' specifically
                     */
                    if (strstr(str_errbuf, "Could not find a part of the path")) {
                        return VTSS_TFTP_INVALID_PATH;
                    } else if (strstr(str_errbuf, "Could not find file")) {
                        return VTSS_TFTP_INVALID_FILENAME;
                    }
                }
                return VTSS_TFTP_EPERM + rc;
            }
        } else if (strstr(str_errbuf, "timeout")) {
            return VTSS_TFTP_TIMEOUT;
        }
    }
    return VTSS_TFTP_IO_ERROR;
}

static int vtss_tftp_cmd(const char *cmd,
                         const char *filename,
                         const char *firmware,
                         const char *server,
                         int port,
                         int *err)
{
    int rc;
    char cmdbuf[512];
    std::string outbuf;
    std::string errbuf;

    snprintf(cmdbuf, sizeof(cmdbuf), "tftp %s -l %s -r %s %s", cmd, firmware,
             filename, server);
    T_D("TFTP: %s", cmdbuf);

    rc = vtss::notifications::process_cmd(cmdbuf, &outbuf, &errbuf);
    if (rc) {
        T_D("TFTP: command failed: %d\n", rc);
        *err = vtss_tftp_err(errbuf.c_str());
        return *err;
    }
    T_D("TFTP: returned OK: %d\n", rc);
    return VTSS_RC_OK;
}

int vtss_tftp_get(const char *filename,
                  const char *server,
                  int port,
                  char *buf,
                  size_t len,
                  vtss_bool_t binary,
                  int *err)
{
    char fsfile[128];
    int rc, fd, flen = -1;

    sprintf(fsfile, "/var/tmp/tftp.%d", getpid());
    if ((rc = vtss_tftp_cmd("-g", filename, fsfile, server, port, err)) == VTSS_RC_OK &&
        (fd = open(fsfile, O_RDONLY)) >= 0) {
        struct stat statb;
        T_D("TFTP: opened %s", fsfile);
        if (fstat(fd, &statb) == 0) {
            T_D("TFTP: file size = %zd", statb.st_size);
            if(statb.st_size > len) {
                T_D("TFTP: Too large (%ld, %zd)", statb.st_size, len);
                flen = -1;
                *err = VTSS_TFTP_TOOLARGE;
            } else {
                if (read(fd, buf, statb.st_size) == statb.st_size) {
                    T_D("TFTP: Read %zd bytes from %s", statb.st_size, fsfile);
                    flen = statb.st_size;
                    *err = VTSS_RC_OK;
                } else {
                    T_D("TFTP: read failed(%s): %s", fsfile, strerror(errno));
                    flen = -1;
                    *err = VTSS_TFTP_IO_ERROR;
                }
            }
        }
        close(fd);
    } else {
        flen = -1;
    }
    (void) unlink(fsfile);
    return flen;
}

int vtss_tftp_put(const char *filename,
                  const char *server,
                  int port,
                  const char *buf,
                  size_t len,
                  vtss_bool_t binary,
                  int *err)
{
    char fsfile[128];
    int rc, fd, flen = -1;

    sprintf(fsfile, "/var/tmp/tftp.%d", getpid());
    if ((fd = open(fsfile, O_WRONLY|O_CREAT|O_TRUNC)) >= 0) {
        T_D("TFTP: opened %s", fsfile);
        if (write(fd, buf, len) == len) {
            close(fd);
            if ((rc = vtss_tftp_cmd("-p", filename, fsfile, server, port, err)) == VTSS_RC_OK) {
                T_D("TFTP: OK writing %s", filename);
                flen = len;
            }
        } else {
            T_D("TFTP: write failed(%s): %s", fsfile, strerror(errno));
            *err = VTSS_TFTP_IO_ERROR;
            close(fd);
        }
        (void) unlink(fsfile);
    } else {
        *err = VTSS_TFTP_IO_ERROR;
        T_D("TFTP: open failed(%s): %s", fsfile, strerror(errno));
    }
    return flen;
}

const char *
vtss_tftp_err2str(int errcode)
{
    switch (errcode) {
    case VTSS_TFTP_EPERM:
        return "Bad file permissions.";
    case VTSS_TFTP_ENOTFOUND:
        return "File not found.";
    case VTSS_TFTP_EACCESS:
        return "Access violation.";
    case VTSS_TFTP_ENOSPACE:
        return "Disk full or allocation exceeded.";
    case VTSS_TFTP_EBADOP:
        return "Illegal TFTP operation.";
    case VTSS_TFTP_EBADID:
        return "Unknown transfer ID.";
    case VTSS_TFTP_EEXISTS:
        return "File already exists.";
    case VTSS_TFTP_ENOUSER:
        return "No such user.";
    case VTSS_TFTP_EBADOPTION:
        return "Bad option";
    case VTSS_TFTP_TIMEOUT:
        return "Operation timed out.";
    case VTSS_TFTP_NETERR:
        return "Network error";
    case VTSS_TFTP_INVALID:
        return "Invalid parameter";
    case VTSS_TFTP_PROTOCOL:
        return "Protocol violation";
    case VTSS_TFTP_TOOLARGE:
        return "File too large";
    case VTSS_TFTP_FORK_FAILED:
        return "Unable to start TFTP process";
    case VTSS_TFTP_IO_ERROR:
        return "Network Error";
    case VTSS_TFTP_INVALID_PATH:
        return "Invalid Path";
    case VTSS_TFTP_INVALID_FILENAME:
        return "Invalid Filename";
    }

    return NULL;
}
