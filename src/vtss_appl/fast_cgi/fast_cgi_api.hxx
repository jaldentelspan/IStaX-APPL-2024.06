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

#ifndef __VTSS_FAST_CGI_API_HXX__
#define __VTSS_FAST_CGI_API_HXX__

#include "vtss_os_wrapper.h"
#include "fcgiapp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool cyg_bool;

#define CYG_HTTPD_MAXOUTBUFFER 8096
#define CYG_HTTPD_MAXINBUFFER 8096

// used by fast_cgi
#define VTSS_TRACE_FAST_CGI_GRP_DEFAULT     0
#define VTSS_TRACE_FAST_CGI_GRP_HIAWATHA    1
// used by https
#define VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT  2

// macro for https critd
#define HTTPS_CRIT_ENTER() critd_enter(&https_crit, __FUNCTION__, __LINE__);
#define HTTPS_CRIT_EXIT()  critd_exit( &https_crit, __FUNCTION__, __LINE__);

typedef struct {
    FCGX_Request *request;
    char outbuffer[CYG_HTTPD_MAXOUTBUFFER + 1];
    const char *args;       // Query arguments
    const char *boundary;   // formdata boundary
    char *post_data;        // Post data
    i32 content_len;        // Length of post_data
    char url[2048];         // For redirects
    int method;
    char content_type;
    char *http_referer;     // HTTP referer in URL
    char *remote_addr;      // Remote address of client

#ifdef VTSS_SW_OPTION_AUTH
    int  auth_lvl;          // Authenticated level from the auth module
    char auth_name[512];    // Authenticated username from the HTTP request
#endif
} CYG_HTTPD_STATE;

enum {
    CYG_HTTPD_METHOD_GET = 1,
    CYG_HTTPD_METHOD_HEAD = 2,
    CYG_HTTPD_METHOD_POST = 3,
    CYG_HTTPD_METHOD_UNKNOWN = 4
};

#define CYG_HTTPD_CONTENT_TYPE_UNKNOWN 0
#define CYG_HTTPD_CONTENT_TYPE_URLENCODED 1
#define CYG_HTTPD_CONTENT_TYPE_FORMDATA 2

#define CYG_HTTPD_STATUS_OK 200
#define CYG_HTTPD_STATUS_NO_CONTENT 204
#define CYG_HTTPD_STATUS_MOVED_PERMANENTLY 301
#define CYG_HTTPD_STATUS_MOVED_TEMPORARILY 302
#define CYG_HTTPD_STATUS_NOT_MODIFIED 304
#define CYG_HTTPD_STATUS_BAD_REQUEST 400
#define CYG_HTTPD_STATUS_NOT_AUTHORIZED 401
#define CYG_HTTPD_STATUS_FORBIDDEN 403
#define CYG_HTTPD_STATUS_NOT_FOUND 404
#define CYG_HTTPD_STATUS_TOO_LARGE 413
#define CYG_HTTPD_STATUS_METHOD_NOT_ALLOWED 405
#define CYG_HTTPD_STATUS_SYSTEM_ERROR 500
#define CYG_HTTPD_STATUS_NOT_IMPLEMENTED 501

#define CYG_HTTPD_HANDLER_TABLE_ENTRY(_name_, _path_, _handler_) \
    web_handler_t _name_ = {_path_, _handler_, false, false};    \
    void __attribute__((constructor)) _name_##_constructor() {   \
        _name_.next = web_root;                                  \
        web_root = &_name_;                                      \
    }

#define CYG_HTTPD_HANDLER_TABLE_ENTRY_JSON(_name_, _path_, _handler_) \
    web_handler_t _name_ = {_path_, _handler_, false, true};          \
    void __attribute__((constructor)) _name_##_constructor() {        \
        _name_.next = web_root;                                       \
        web_root = &_name_;                                           \
    }

#define CYG_HTTPD_HANDLER_TABLE_ENTRY_FS_LIKE_MATCH(_name_, _path_, _handler_) \
    web_handler_t _name_ = {_path_, _handler_, true, false};                   \
    void __attribute__((constructor)) _name_##_constructor() {                 \
        _name_.next = web_root;                                                \
        web_root = &_name_;                                                    \
    }

struct cyg_httpd_ires_table_entry {
    char *f_pname;
    unsigned char *f_ptr;
    int f_size;
};

typedef i32 (*handler)(CYG_HTTPD_STATE *);
typedef struct web_handler {
    const char         *path;
    handler            h;
    bool               fs_like_match;
    bool               json;
    struct web_handler *next;
} web_handler_t;

extern web_handler_t *web_root;
extern CYG_HTTPD_STATE httpstate;

int cyg_httpd_current_privilege_level();
void cyg_httpd_current_username(char *username, size_t username_max_len);
void cyg_httpd_send_error(i32 err_type);
void cyg_httpd_set_xreadonly_tag(int status);
void cyg_httpd_send_content_disposition(cyg_httpd_ires_table_entry *entry);
ssize_t cyg_httpd_write(char *buf, int buf_len);

ssize_t cyg_httpd_start_chunked(const char *extension);
ssize_t cyg_httpd_write_chunked(const char *buf, int len);
void cyg_httpd_end_chunked(void);

#if 0
{
#endif  // fixing indent

#ifdef __cplusplus
}
#endif
#endif /* __VTSS_FAST_CGI_API_HXX__ */
