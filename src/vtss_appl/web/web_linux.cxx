/*

 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "web.h"
#include "web_api.h"
#include "vtss_md5_api.h"


struct lib_config_js_t  *jsconfig_root;
struct lib_filter_css_t *css_config_root;


/*
 * POST formdata extraction
 */

char *
vtss_get_formdata_boundary(CYG_HTTPD_STATE* p, int *len)
{
    if (p->boundary) {
        int blen = strlen(p->boundary) + 2;
        char *buf = (char *) VTSS_MALLOC(blen+1);
        if(buf) {
            buf[0] = buf[1] = '-';
            strcpy(buf+2, p->boundary);
            *len = blen;
            return buf;
        }
    }
    return NULL;
}

size_t webCommonBufferHandler(char **base_ptr, char **cur_ptr, size_t *length, const char *buff)
{
    size_t used  = 0; // used buffer length.
    size_t avail = 0; // available buffer length.
    size_t ct    = 0;

    if ((*base_ptr == NULL) || (*cur_ptr == NULL) || (length == NULL) || (buff == NULL)) {
        return 0;               /* Argument error - nothing copied */
    }

    used  = *cur_ptr - *base_ptr; // calculating used buffer length
    avail = *length - used;       // calculating available buffer length
    ct = strlen(buff);

    if (ct >= avail) {
        return 0;
    }

    strcpy(*cur_ptr, buff);
    *cur_ptr += ct;
    return ct;
}

static i32 handler_chunklist(CYG_HTTPD_STATE* p, const struct web_chunk_entry *clist, const char *mime)
{
    const struct web_chunk_entry *chunk;
    char buffer[8092];
    char *fsname =  FCGX_GetParam("SCRIPT_FILENAME", p->request->envp);
    FILE *fp = NULL;

    if(fsname) {
        fp = fopen(fsname, "w");
    }

    cyg_httpd_start_chunked(mime);
    for (chunk = clist; chunk; chunk = chunk->next) {
        char *bufp = &buffer[0];
        size_t buf_size = sizeof(buffer), chunk_size;
        chunk_size = chunk->module_fun(&bufp, &bufp, &buf_size);
        cyg_httpd_write_chunked(buffer, chunk_size);
        if(fp && chunk_size) {
            fwrite(buffer, chunk_size, 1, fp);
            fwrite("\n", 1, 1, fp);
        }
    }

    if(fp) {
        fclose(fp);
    }

    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

static i32 handler_config_js(CYG_HTTPD_STATE* p)
{
    return handler_chunklist(p, jsconfig_root, "javascript");
}
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_handler_config_js, "/lib/config.js", handler_config_js);

static i32 handler_filter_css(CYG_HTTPD_STATE* p)
{
    return handler_chunklist(p, css_config_root, "css");
}

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_handler_filter_css, "/config/filter.css", handler_filter_css);

#define WEBDIR "/var/www/webstax"

mesa_rc web_init_os(vtss_init_data_t *data)
{
    if (data->cmd == INIT_CMD_INIT) {
        (void) unlink(WEBDIR "/lib/config.js");
        (void) unlink(WEBDIR "/config/filter.css");
    }
    return 0;
}
