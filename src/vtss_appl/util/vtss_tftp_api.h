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

#ifndef _VTSS_TFTP_API_H_
#define _VTSS_TFTP_API_H_

#include "vtss_os_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    VTSS_TFTP_EPERM = 0,     /* bad file permissions */
    VTSS_TFTP_ENOTFOUND,     /* file not found */
    VTSS_TFTP_EACCESS,       /* access violation */
    VTSS_TFTP_ENOSPACE,      /* disk full or allocation exceeded */
    VTSS_TFTP_EBADOP,        /* illegal TFTP operation */
    VTSS_TFTP_EBADID,        /* unknown transfer ID */
    VTSS_TFTP_EEXISTS,       /* file already exists */
    VTSS_TFTP_ENOUSER,       /* no such user */
    VTSS_TFTP_EBADOPTION,    /* Bad option */
    // These codes are not protocol-defined
    VTSS_TFTP_TIMEOUT,       /* operation timed out */
    VTSS_TFTP_NETERR,        /* some sort of network error */
    VTSS_TFTP_INVALID,       /* invalid parameter */
    VTSS_TFTP_PROTOCOL,      /* protocol violation */
    VTSS_TFTP_TOOLARGE,      /* file is larger than buffer */
    VTSS_TFTP_FORK_FAILED,   /* Unable to start TFTP process */
    VTSS_TFTP_IO_ERROR,      /* IO error */
    VTSS_TFTP_INVALID_PATH,  /* Invalid path */
    VTSS_TFTP_INVALID_FILENAME, /* Invalid filename */
};

mesa_rc vtss_tftp_err(const char *str_errbuf);

int vtss_tftp_get(const char *filename,
                  const char *server,
                  int port,
                  char *buf,
                  size_t len,
                  vtss_bool_t binary,
                  int *err);

int vtss_tftp_put(const char *filename,
                  const char *server,
                  int port,
                  const char *buf,
                  size_t len,
                  vtss_bool_t binary,
                  int *err);

const char *vtss_tftp_err2str(int errcode);

#ifdef __cplusplus
}
#endif

#endif // _VTSS_TFTP_API_H_

