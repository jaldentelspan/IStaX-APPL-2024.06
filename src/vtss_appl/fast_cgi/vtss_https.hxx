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

#ifndef _VTSS_HTTPS_HXX_
#define _VTSS_HTTPS_HXX_

#include "vtss/basics/stream.hxx"
#include <vtss/basics/trace.hxx>
#include <vtss/basics/notifications.hxx>
#include <vtss/basics/notifications/process-daemon.hxx>
#include "subject.hxx"
#include "vtss/appl/https.h"
#include "vtss_https_api.hxx"
#include "critd_api.h"

extern vtss::notifications::ProcessDaemon vtss_process_hiawatha;
/* global variable */
extern https_conf_t global_hiawatha_conf;
extern critd_t https_crit;

#define VTSS_HIAWATHA_WORKDIR   "/tmp/hiawatha"
#define VTSS_HIAWATHA_CONF      "/tmp/hiawatha/hiawatha.conf"
#define VTSS_HIAWATHA_CONF_TMP  "/tmp/hiawatha/tmp.conf"
#define VTSS_MIMETYPE_CONF      "/tmp/hiawatha/mimetype.conf"
#define VTSS_CGI_WRAPPER_CONF   "/tmp/hiawatha/cgi-wrapper.conf"
#define VTSS_SERVERKEY_PEM      "/tmp/hiawatha/serverkey.pem"

#define ORIG_MIMETYPE_CONF      "/etc/hiawatha/mimetype.conf"
#define ORIG_CGI_WRAPPER_CONF   "/etc/hiawatha/cgi-wrapper.conf"
#define ORIG_SERVERKEY_PEM      "/etc/hiawatha/serverkey.pem"

#define HTTPS_CA_DIR                        "/switch/.ca"
#define HTTPS_CA_PEM_FILE                   "/switch/.ca/serverkey.pem"
#define HTTPS_CA_PEM_UPLOAD_FILE            "/switch/.ca/serverkey-upload.pem"
#define HTTPS_RSA_PEM_FILE                  "/switch/.ca/rsa.key"
#define HTTPS_RSA_KEY_SIZE                  2048

/* Time in seconds for a CGI-process to finish its job
 * This value has to be a reasonable one.
 */
#define DEFAULT_TIME_FOR_CGI                60

template<typename T>
struct cfg {
    const char *name;
    T     value;
    cfg(const char *n, T v)
    {
        name  = n;
        value = v;
    }
};

typedef struct vtss_connections {
    cfg<int> total;
    cfg<int> per_ip;
    vtss_connections() :
        total               ("ConnectionsTotal = ", 1000),
        per_ip              ("ConnectionsPerIP = ", 100)
    {}
    /* Currently we only have these configs */
} vtss_connections_t;

typedef struct vtss_binding {
    cfg<int> port;
    cfg<const char *> intf;
    cfg<int> max_request_size;
    cfg<const char *> ssl_cert_file;
    vtss_binding() :
        port                ("        Port = ", 0),
        intf                ("        Interface = ", "0.0.0.0"),
        max_request_size    ("        MaxRequestSize = ", 20 * 1024), /* BZ#20869 - The maximum firmware image is set to 20MB by default. The maximum size in kilobytes of a request the webserver will accept as legitimate. */
        // Current Hiawatha version: 10.0
        // renamed to TLScertFile from version 9.13
        ssl_cert_file       ("        TLScertFile = ", NULL)
    {}
    /* Currently we only have these configs */
} vtss_binding_t;


typedef struct vtss_fcgiserver {
    cfg<const char *> connect_to;
    cfg<const char *> fast_fgi_id;
    cfg<const char *> extension;
    vtss_fcgiserver() :
        connect_to          ("        ConnectTo = ", "/tmp/json.socket"),
        fast_fgi_id         ("        FastCGIid = ", "JSON"),
        extension           ("        Extension = ", "cgi")
    {}
    /* Currently we only have these configs */
} vtss_fcgiserver_t;

typedef struct vtss_urltoolkit {
    BOOL force_tls;
    cfg<const char *> toolkit_id;
    cfg<const char *> use_tls;
    cfg<const char *> match_rewrite;
    cfg<const char *> request_uri;
    cfg<const char *> match;
    vtss_urltoolkit() :
        force_tls           (FALSE),
        toolkit_id          ("        ToolkitID = ", "webstax"),
        use_tls             ("        UseTLS    = ", "Skip 1"),
        match_rewrite       ("        ", "Match ^/(.*) Rewrite / Continue"),
        request_uri         ("        ", "RequestURI isfile Return"),
        match               ("        ", "Match / UseFastCGI JSON")
    {}
    /* Currently we only have these configs */
} vtss_urltoolkit_t;

/* Define hiawatha website
 * Reference: https://www.hiawatha-webserver.org/manpages/hiawatha
 */
typedef struct vtss_defwebsite {
    cfg<const char *> hostname;
    cfg<const char *> website_root;
    cfg<const char *> start_file;
    cfg<const char *> access_log;
    cfg<const char *> system_log;
    cfg<const char *> garbage_log;
    cfg<const char *> error_log;
    cfg<const char *> exploit_log;
    cfg<const char *> use_toolkit;
    cfg<const char *> ignore_dot_hiawatha;
    cfg<const char *> work_dir;
    cfg<const char *> http_auth_to_cgi;
    cfg<int>         time_for_cgi;
    cfg<const char *> custom_header1;
    cfg<const char *> prevent_csrf;
    cfg<const char *> prevent_sqli;
    cfg<const char *> prevent_xss;
    //    cfg<const char *> min_ssl_version;  // Option removed as of Hiawatha 11.0
    cfg<const char *> custom_header2;
    vtss_defwebsite() :
        hostname            ("Hostname          = ", "127.0.0.1"),
        website_root        ("WebsiteRoot       = ", "/var/www/webstax"),
        start_file          ("StartFile         = ", "login.htm"),
        access_log          ("AccessLogfile     = ", "none"),
        system_log          ("SystemLogfile     = ", "/dev/null"),
        garbage_log         ("GarbageLogfile    = ", "/dev/null"),
        error_log           ("ErrorLogfile      = ", "/dev/null"),
        exploit_log         ("ExploitLogfile    = ", "/dev/null"),
        use_toolkit         ("UseToolkit        = ", "webstax"),
        ignore_dot_hiawatha ("UseLocalConfig    = ", "no"),
        work_dir            ("WorkDirectory     = ", "/tmp/hiawatha"),
        http_auth_to_cgi    ("HTTPAuthToCGI     = ", "yes"),
        time_for_cgi        ("TimeForCGI        = ", DEFAULT_TIME_FOR_CGI),
        custom_header1      ("CustomHeaderClient= ", "X-Frame-Options: sameorigin"),
        // Current Hiawatha version 10.0
        // SECURITY RELATED
        // prevent Cross-site Request Forgery
        prevent_csrf        ("PreventCSRF       = ", "prevent"),
        // prevent SQL-injection
        // BZ#22547: Disable the options of 'PreventSQLi = yes' in Hiawatha v10.1 or later version.
        // It causes the HTTP connection failed if the HTTP request including the HTML attribute of enctype="multipart/form-data"
        // After the discussing with Lars, it should be safe to disable the SQL injection filter.
        prevent_sqli        ("PreventSQLi       = ", "no"),
        // prevent Cross-site scripting
        prevent_xss         ("PreventXSS        = ", "prevent"),
        // requires that Hiawatha was compiled with -DENABLE_TLS=on
        // renamed to MinTLSversion from version 9.13
        // min_ssl_version     ("MinTLSversion     = ", "1.1"), // Option removed as of Hiawatha 11.0
        // force users to visit websites via HTTPS
        // renamed RequireSSL to RequireTLS from version 9.13
        // NOTE:    RequireSSL is enabled in fast_cgi.cxx instead
        // Due to:  RequireSSL and RequiredBinding not allowed outside VirtualHost section
        // require_ssl         ("RequireSSL        = ", "yes")
        custom_header2      ("CustomHeaderClient= ", "Vary: Accept-Encoding")
        // add new cfg here
    {}
    /* Currently we only have these configs */
} vtss_defwebsite_t;


/* ================================================================= *
 *  HTTPS configuration blocks
 * ================================================================= */

/* Block versions */
#define HTTPS_CONF_BLK_VERSION      2

/* HTTPS configuration block */
typedef struct {
    u32             version;        /* Block version */
    https_conf_t    https_conf;     /* HTTPS configuration */
} https_conf_blk_t;


/* ================================================================= *
 *  HTTPS global structure
 * ================================================================= */

/* HTTPS global structure */
typedef struct {
    critd_t                 crit;
    BOOL                    apply_init_conf; /* A flag to specify the configuration should be applied at initial state */
    https_conf_t            https_conf;
    BOOL                    https_cert_gen_status;
} https_global_t;


vtss::ostream &operator<<(vtss::ostream &o, const vtss_binding_t    &b);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_fcgiserver_t &f);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_urltoolkit_t &u);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_defwebsite_t &d);

mesa_rc https_mgmt_conf_get(https_conf_t *conf);
mesa_rc https_mgmt_conf_set(const https_conf_t *conf);
mesa_rc https_mgmt_conf_reload(void);
https_cert_status_t https_mgmt_cert_status(void);

mesa_rc vtss_hiawatha_workdir_create(const char *workdir);
mesa_rc vtss_hiawatha_conf_update(const char *file_name, const https_conf_t *conf);
mesa_rc vtss_mimetype_conf_map(void);
mesa_rc vtss_cgi_wrapper_conf_map(void);


#endif  /*  _VTSS_HTTPS_HXX_  */
