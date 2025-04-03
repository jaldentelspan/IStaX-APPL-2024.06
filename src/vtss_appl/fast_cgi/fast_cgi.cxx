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

#include <sys/stat.h>
#include <fcgiapp.h>
#include <fcgi_config.h>

#include "main_conf.hxx"
#include "vtss_trace_api.h"
#include <vtss/basics/trace.hxx>
#include <vtss/basics/notifications.hxx>
#include <vtss/basics/notifications/process-daemon.hxx>
#include "subject.hxx"

#include "fast_cgi_api.hxx"
#include "vtss_https.hxx"
#include "vtss_https_api.hxx"
#include "vtss_https_icfg.hxx"

#ifdef VTSS_SW_OPTION_AUTH
#include "vtss_auth_api.h"
#endif

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#else
/* Define dummy syslog macros */
#define S_I(fmt, ...)
#define S_W(fmt, ...)
#define S_E(fmt, ...)
#endif

#define HTTP_HEADER_LINE_TRAILER    "\r\n"
#define HTTP_HEADER_END_MARKER      "\r\n\r\n"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_FAST_CGI
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_FAST_CGI

web_handler_t *web_root;
CYG_HTTPD_STATE httpstate;
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "fast_cgi", "fast_cgi"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_FAST_CGI_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
    [VTSS_TRACE_FAST_CGI_GRP_HIAWATHA] = {
        "hiawatha",
        "Hiawatha",
        VTSS_TRACE_LVL_ERROR
    },
    [VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT] = {
        "https.mgmt",
        "https management",
        VTSS_TRACE_LVL_WARNING
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

static vtss_handle_t fast_cgi_thread_handle;
static vtss_thread_t fast_cgi_thread_block;
static bool web_handlers_enabled;

vtss::notifications::ProcessDaemon vtss_process_hiawatha(&vtss::notifications::subject_main_thread, "hiawatha");

static web_handler_t *FAST_CGI_find_handler(const char *path)
{
    web_handler_t *h, *best_handler;
    size_t        best_len;
    bool          fs_like_match_found = false;

    T_D("Searching for handler for %s", path);

    for (h = web_root; h; h = h->next) {
        if (!h->json && !web_handlers_enabled) {
            // Skip web handlers
        } else if (h->fs_like_match) {
            // If we don't find an exact match, do the
            // file-system-like matching below.
            fs_like_match_found = true;
        } else if (strcmp(h->path, path) == 0) {
            T_D("Exact match for %s found", path);
            return h;
        }
    }

    if (!fs_like_match_found) {
        // No need to go through the below stuff, since there
        // are no handlers that are file-system like.
        T_D("No exact match for %s found, and there's no file system like handlers", path);
        return NULL;
    }

    // Search through all handlers that have the fs_like_match flag set and
    // find the one with the longest match (don't just stop at the first handler
    // that matches). In this way, we can have a handler for e.g. "/a/<anything>",
    // but a more specific handler for "/a/b/<anything>".
    best_len = 0;
    best_handler = NULL;
    for (h = web_root; h; h = h->next) {
        size_t len;

        if (!h->fs_like_match) {
            continue;
        }

        len = strlen(h->path);

        if (strncmp(h->path, path, len) != 0) {
            continue;
        }

        // Candidate found. If the handler hasn't specified a slash
        // at the end of its path, we must check that as well.
        if (h->path[len - 1] != '/') {
            if (strlen(path) != len && path[len] != '/') {
                // Not an exact match and the URL didn't end with a slash as well.
                continue;
            }

            len++;
        }


        // See if this candidate is better than any previously found.
        if (len > best_len) {
            best_len = len;
            best_handler = h;
        }
    }

    if (best_handler) {
        T_D("File system like handler found for %s", path);
    } else {
        T_D("No exact match and no file system like handlers found for %s", path);
        S_I("Attempt to read page %s, but page not found", path);
    }

    return best_handler;
}

static void FAST_CGI_setup_content(FCGX_Request *request, CYG_HTTPD_STATE *s)
{
    const char *contentLength = FCGX_GetParam("CONTENT_LENGTH", request->envp);

    if (s->method != CYG_HTTPD_METHOD_POST) {
        goto NO_CONTENT;
    }

    if (!contentLength) {
        goto NO_CONTENT;
    }

    s->content_len = strtol(contentLength, NULL, 10);
    if (s->content_len <= 0) {
        goto NO_CONTENT;
    }

    s->post_data = (char *)VTSS_MALLOC(s->content_len + 1);
    if (!s->post_data) {
        goto NO_CONTENT;
    }

    int len;
    for (len = 0; len < s->content_len; len++) {
        int ch = FCGX_GetChar(request->in);

        if (ch < 0) {
            VTSS_TRACE(ERROR) << "Not enough bytes received on standard input, "
                              "got " << len << ", wanted " << s->content_len;
            s->content_len = len;
            return;

        } else {
            s->post_data[len] = (char)ch;
        }
    }

    s->post_data[len] = (char)0;
    return;

NO_CONTENT:
    s->post_data = NULL;
    s->content_len = 0;
}

static void FAST_CGI_setup_request(FCGX_Request *request, CYG_HTTPD_STATE *s,
                                   const char *path)
{
    const char *method = FCGX_GetParam("REQUEST_METHOD", request->envp);
    const char *content = FCGX_GetParam("CONTENT_TYPE", request->envp);

    s->request = request;
    s->http_referer = FCGX_GetParam("HTTP_REFERER", request->envp);
    s->remote_addr = FCGX_GetParam("REMOTE_ADDR", request->envp);

    // When simulating a file system, the application needs
    // the URL to be filled in as well.
    strncpy(s->url, path, sizeof(s->url) - 1);
    s->url[sizeof(s->url) - 1] = '\0';

    if (method && (strcmp(method, "POST") == 0)) {
        s->method = CYG_HTTPD_METHOD_POST;
    } else {
        s->method = CYG_HTTPD_METHOD_GET;
    }

    if (!(s->args = FCGX_GetParam("QUERY_STRING", request->envp))) {
        s->args = "";    // Ensure valid ptr even if no args
    }

    s->content_type = CYG_HTTPD_CONTENT_TYPE_UNKNOWN;
    if (content) {
        const char *urlencoded = "application/x-www-form-urlencoded";
        const char *formdata = "multipart/form-data";
        if (strncasecmp(content, urlencoded, strlen(urlencoded)) == 0) {
            s->content_type = CYG_HTTPD_CONTENT_TYPE_URLENCODED;
        } else if (strncasecmp(content, formdata, strlen(formdata)) == 0) {
            const char *key = "boundary=";
            s->content_type = CYG_HTTPD_CONTENT_TYPE_FORMDATA;
            if ((s->boundary = strcasestr(content, key))) {
                s->boundary += strlen(key);
            }
        }
    }

    FAST_CGI_setup_content(request, s);
}

static bool FAST_CGI_web_request(FCGX_Request *request, CYG_HTTPD_STATE *state,
                                 const char *path)
{
    web_handler_t *h = FAST_CGI_find_handler(path);
    if (!h) {
        return false;
    }

    FAST_CGI_setup_request(request, state, path);

    h->h(state);  // Call handler

    if (state->post_data) {
        VTSS_FREE(state->post_data);
    }

    return true;
}

ssize_t cyg_httpd_write(char *buf, int buf_len)
{
    return FCGX_PutStr(buf, buf_len, httpstate.request->out);
}

ssize_t cyg_httpd_start_chunked(const char *extension)
{
    return FCGX_FPrintF(httpstate.request->out,
                        "Content-type: text/%s"
                        HTTP_HEADER_END_MARKER,
                        extension);
}

void cyg_httpd_end_chunked(void)
{
    FCGX_Finish_r(httpstate.request);
}

ssize_t cyg_httpd_write_chunked(const char *buf, int len)
{
    return FCGX_PutStr(buf, len, httpstate.request->out);
}

void cyg_httpd_send_error(i32 err_type)
{
    switch (err_type) {
    case CYG_HTTPD_STATUS_MOVED_TEMPORARILY:
        FCGX_FPrintF(httpstate.request->out,
                     "Location: %s"
                     HTTP_HEADER_END_MARKER,
                     httpstate.url);
        break;

    case CYG_HTTPD_STATUS_BAD_REQUEST:
        FCGX_FPrintF(httpstate.request->out,
                     "Status: 400 Bad Request"
                     HTTP_HEADER_END_MARKER);
        break;

    case CYG_HTTPD_STATUS_NOT_FOUND:
        FCGX_FPrintF(httpstate.request->out,
                     "Status: 404 Not Found"
                     HTTP_HEADER_END_MARKER);
        break;

    case CYG_HTTPD_STATUS_SYSTEM_ERROR:
        FCGX_FPrintF(httpstate.request->out,
                     "Status: 500 Internal Server Error"
                     HTTP_HEADER_END_MARKER);
        break;

    default:
        FCGX_FPrintF(httpstate.request->out,
                     "Status: %d"
                     HTTP_HEADER_END_MARKER,
                     err_type);
        break;
    }
    FCGX_Finish_r(httpstate.request);
}

void cyg_httpd_send_content_disposition(cyg_httpd_ires_table_entry *entry)
{
    FCGX_FPrintF(httpstate.request->out,
                 "Content-Type: application/x-download"
                 HTTP_HEADER_LINE_TRAILER
                 "Content-disposition: attachment; filename=\"%s\"; size=%u"
                 HTTP_HEADER_END_MARKER,
                 entry->f_pname,
                 entry->f_size);
    FCGX_PutStr((char *)entry->f_ptr, entry->f_size, httpstate.request->out);
}

void cyg_httpd_set_xreadonly_tag(int status)
{
    if (status) {
        FCGX_FPrintF(httpstate.request->out,
                     "X-ReadOnly: true"
                     HTTP_HEADER_LINE_TRAILER);
    } else {
        FCGX_FPrintF(httpstate.request->out,
                     "X-ReadOnly: null"
                     HTTP_HEADER_LINE_TRAILER);
    }
}

int cyg_httpd_current_privilege_level()
{
    return httpstate.auth_lvl;
}

void cyg_httpd_current_username(char *username, size_t username_max_len)
{
    memset(username, 0, username_max_len);
    strncpy(username, httpstate.auth_name, username_max_len);
}

#ifdef VTSS_SW_OPTION_AUTH
static bool FAST_CGI_authenticate(FCGX_Request *req, CYG_HTTPD_STATE *state)
{
    vtss::SBuf128 buf;

    T_I("Enter");

    // Auth is a string of the form: "Basic YWRtaW46"
    const char *auth = FCGX_GetParam("HTTP_AUTHORIZATION", req->envp);
    if (!auth) {
        VTSS_TRACE(INFO) << "No HTTP_AUTHORIZATION header";
        return false;
    }

    // Check that first word is "Basic " (case-insensitively, according to RFC2617).
    if (strncasecmp(auth, "Basic ", strlen("Basic ")) != 0) {
        T_I("Unsupported authentication method");
        return false;
    }

    const char *base64_str = strchr(auth, ' ');
    if (!base64_str) {
        VTSS_TRACE(INFO) << "No user/password";
        return false;
    }

    base64_str++;
    if (vtss_httpd_base64_decode(buf.begin(), buf.size(), base64_str,
                                 strlen(base64_str)) != VTSS_RC_OK) {
        VTSS_TRACE(INFO) << "Failed to decode";
        return false;
    }

    char *user = buf.begin();
    char *pass = strchr(buf.begin(), ':');
    if (!pass) {
        VTSS_TRACE(INFO) << "Invalid format";
        return false;
    }

    // Split the string into username and password
    *pass++ = 0;

    memset(state->auth_name, 0, sizeof(state->auth_name));
    strncpy(state->auth_name, user, sizeof(state->auth_name) - 1);

    if (!vtss_auth_mgmt_httpd_authenticate(user, pass, &state->auth_lvl)) {
        VTSS_TRACE(INFO) << "Authentication failed";
        return false;
    }

    T_I("Exit");
    return true;
}
#endif

static void FAST_CGI_accept_loop(vtss_addrword_t data)
{
    int sock;
    FCGX_Request request;
    const char *sockname = "/tmp/json.socket";
    BOOL b_redirect;

    if (FCGX_Init()) {
        T_W("init failed");
        return;
    }

    if ((sock = FCGX_OpenSocket(sockname, 10)) < 0) {
        T_W("socket open");
        return;
    }

    (void)chmod(sockname, 0777);

    FCGX_InitRequest(&request, sock, 0);

    while (FCGX_Accept_r(&request) >= 0) {
        VTSS_TRACE(NOISE) << "Fast-CGI Request:";
        char **e = request.envp;
        while (*e) {
            VTSS_TRACE(NOISE) << "    " << *e;
            e++;
        }

        const char *script      = FCGX_GetParam("SCRIPT_NAME", request.envp);
        const char *http_scheme = FCGX_GetParam("HTTP_SCHEME", request.envp);
        const char *http_host   = FCGX_GetParam("HTTP_HOST",   request.envp);
        const char *request_uri = FCGX_GetParam("REQUEST_URI", request.envp);

        // Bugzilla#19826
        if (!script) {
            script = "";
        }
        if (!http_scheme) {
            http_scheme = "";
        }
        if (!http_host) {
            http_host = "";
        }
        if (!request_uri) {
            request_uri = "";
        }

        // Redirect http to https if it is enabled, equivalent as
        // RequireSSL=yes in hiawatha.conf
        // redirect can NOT be enabled if https server is not enabled
        // NOTE: redirect has to be done before the authentication!!!
        HTTPS_CRIT_ENTER();
        b_redirect = global_hiawatha_conf.redirect;
        HTTPS_CRIT_EXIT();

        if ( b_redirect &&
             http_scheme && (strcmp("https", http_scheme) != 0)) {
            if (http_host[0] == '\0') {
                FCGX_FPrintF(request.out,
                             "Status: 404 Not found"
                             HTTP_HEADER_LINE_TRAILER
                             "Content-type: text/html"
                             HTTP_HEADER_END_MARKER);
            } else {
                VTSS_TRACE(NOISE) << "    " << "REDIRECT HTTP TO HTTPS";
                FCGX_FPrintF(request.out,
                             "Location: https://%s%s"
                             HTTP_HEADER_END_MARKER,
                             http_host,
                             request_uri);
            }
            FCGX_Finish_r(&request);
            continue;
        }
        // RequireSSL=yes security check ends //

        // Landing page after logout - does not require authentication
        if (script && strcmp("/logout.htm", script) == 0) {
            FCGX_FPrintF(request.out,
                         "Content-type: text/html"
                         HTTP_HEADER_END_MARKER
                         "<title>Logout</title>"
                         "<h1><a href=\"/login.htm\">Go to login!</a></h1>\n");
            FCGX_Finish_r(&request);
            continue;
        }

#ifdef VTSS_SW_OPTION_AUTH
        // Check authentication or return 401.
        if (!FAST_CGI_authenticate(&request, &httpstate)) {
            FCGX_FPrintF(request.out,
                         "Status: 401 Unauthorized"
                         HTTP_HEADER_LINE_TRAILER
                         "WWW-Authenticate: Basic realm=\"WebStaX\""
                         HTTP_HEADER_LINE_TRAILER
                         "Content-type: text/html"
                         HTTP_HEADER_END_MARKER);
            FCGX_Finish_r(&request);
            continue;
        }
#endif
        VTSS_TRACE(INFO) << "Fast-CGI Authenticated";

        if (script && strcmp("/login.htm", script) == 0) {
            // Redirect to the page with the "real" content
            FCGX_FPrintF(request.out,
                         "Location: index.htm"
                         HTTP_HEADER_END_MARKER);
        } else if (script && strcmp("/logout", script) == 0) {
            FCGX_FPrintF(request.out,
                         "Content-type: text/html"
                         HTTP_HEADER_END_MARKER);
        } else if (FAST_CGI_web_request(&request, &httpstate, script)) {
            // Handled request
        } else {
            FCGX_FPrintF(request.out,
                         "Status: 404 Not found"
                         HTTP_HEADER_LINE_TRAILER
                         "Content-type: text/html"
                         HTTP_HEADER_END_MARKER);
        }
        FCGX_Finish_r(&request);
    }
}

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize our private mib */
VTSS_PRE_DECLS void https_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_hiawatha_json_init(void);
#endif
extern "C" int vtss_https_icli_cmd_register();

static bool web_enabled;

bool web_module_enabled(void)
{
    return web_enabled;
}

extern "C" mesa_rc vtss_fast_cgi_init(vtss_init_data_t *data)
{
    if (data->cmd == INIT_CMD_INIT) {
        /* Init critd for https, register under FAST_CGI(Linux) */
        critd_init(&https_crit, "https", VTSS_MODULE_ID_FAST_CGI, CRITD_TYPE_MUTEX);
        web_enabled = vtss::appl::main::module_enabled("web");
    }

    if (!web_enabled) {
        T_D("module disabled");
        return VTSS_RC_OK;
    }

    // Initialize the extern feature first.
    (void)https_init(data);

    VTSS_TRACE(DEBUG) << "enter, cmd: " << data->cmd << ", isid: " << data->isid
                      << ", flags: " << data->flags;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        vtss_process_hiawatha.executable = "hiawatha";

        // Do not fork - we want to keep contact such that we can monitor it
        vtss_process_hiawatha.arguments.push_back("-d");

        // Setup trace group for stdout and stderr from hiawatha
        vtss_process_hiawatha.trace_stdout_conf(VTSS_TRACE_MODULE_ID, VTSS_TRACE_FAST_CGI_GRP_HIAWATHA, VTSS_TRACE_LVL_INFO);
        vtss_process_hiawatha.trace_stderr_conf(VTSS_TRACE_MODULE_ID, VTSS_TRACE_FAST_CGI_GRP_HIAWATHA, VTSS_TRACE_LVL_ERROR);

        // Use our own configuration file
        // /tmp/hiawatha/.
        vtss_hiawatha_workdir_create(VTSS_HIAWATHA_WORKDIR);
        // write default conf into /tmp/hiawatha/hiawatha.conf
        vtss_hiawatha_conf_update(VTSS_HIAWATHA_CONF, &global_hiawatha_conf);
        // copy /etc/hiawatha/mimetype.conf to /tmp/hiawatha/.
        vtss_mimetype_conf_map();
        // copy /etc/hiawatha/cgi_wrapper.conf to /tmp/hiawatha/.
        vtss_cgi_wrapper_conf_map();
        // enable prevent csrf
        global_hiawatha_conf.prevent_csrf = TRUE;

        // use SEGKILL instead of SEGTERM
        vtss_process_hiawatha.kill_policy(true);
        // define hiawatha conf path/directory
        vtss_process_hiawatha.arguments.push_back("-c");
        // /tmp/hiawatha as conf path/directory
        vtss_process_hiawatha.arguments.push_back(VTSS_HIAWATHA_WORKDIR);
        // Set nice to 10
        vtss_process_hiawatha.be_nice(10);

#ifdef VTSS_SW_OPTION_ICFG
        if (vtss_https_icfg_init() != VTSS_RC_OK) {
            T_W("https_icfg init failed!");
        }
#endif  /* VTSS_SW_OPTION_ICFG */

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        // Register our private mib
        https_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        // register json interface
        vtss_appl_hiawatha_json_init();
#endif
        vtss_https_icli_cmd_register();
        break;

    case INIT_CMD_START:
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           FAST_CGI_accept_loop,
                           0,
                           "Fast CGI",
                           nullptr,
                           0,
                           &fast_cgi_thread_handle,
                           &fast_cgi_thread_block);

        {
            // Check if web handlers are enabled
            auto &c = vtss::appl::main::module_conf_get("web");
            web_handlers_enabled = c.bool_get("handlers", true);
        }

        break;

    case INIT_CMD_CONF_DEF: {
        // reload default config
        https_conf_t def_conf;
        https_conf_mgmt_get_default(&def_conf);
        if (https_mgmt_conf_set(&def_conf) != VTSS_RC_OK) {
            T_W("reload https default config failed!");
        }
        break;
    }
    default:
        ;
    }

    return VTSS_RC_OK;
}
