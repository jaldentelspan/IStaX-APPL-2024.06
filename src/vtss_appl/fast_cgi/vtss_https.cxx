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
#include <sys/types.h>  /* create dir */
#include <sys/socket.h>
#include <netinet/in.h>  /* inet_aton() */
#include <arpa/inet.h>

#define HTTPS_USE_MBEDTLS_LIB  // we use PolarSSL library on Linux platform

#if defined(HTTPS_USE_MBEDTLS_LIB)
#include "mbedtls/config.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/x509.h"
#include "mbedtls/rsa.h"
#endif /* HTTPS_USE_MBEDTLS_LIB */

#include "vtss_trace_api.h"
#include <vtss/basics/trace.hxx>
#include "fast_cgi_api.hxx"
#include "vtss_https.hxx"
#include "vtss_https_api.hxx"
#include "conf_api.h"
#include "vtss_remote_file_transfer.hxx"

#include "ip_api.h"
#include "ip_utils.hxx"
#include "vtss/basics/memory.hxx"
#include "msg_api.h"
#include "misc_api.h" /* For misc_url_decompose() */
#include "vtss_tftp_api.h" /* For vtss_tftp_get() */
#include "mgmt_api.h"   // For mgmt_txt2ipv6_type()

#ifdef VTSS_SW_OPTION_AUTH
#include "vtss_auth_api.h"
#endif /* VTSS_SW_OPTION_AUTH */

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_FAST_CGI
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_FAST_CGI

#define HTTPS_CA_PEM_CA_HEADER_START                "-----BEGIN CERTIFICATE-----"
#define HTTPS_CA_PEM_CA_HEADER_END                  "-----END CERTIFICATE-----"

/* Thread variables */
static vtss_handle_t HTTPS_cert_thread_handle;
static vtss_thread_t HTTPS_cert_thread_block;

static vtss_handle_t HTTPS_gen_thread_handle;
static vtss_thread_t HTTPS_gen_thread_block;

#define HTTPS_CERT_THREAD_EVENT_CHECK_CA            0x00000001 /* Generate cerificate if it is not present */
#define HTTPS_CERT_THREAD_EVENT_REGEN_CA            0x00000002 /* Re-generate cerificate */
#define HTTPS_CERT_THREAD_EVENT_CONF_APPLY          0x00000004 /* Apply configuration */
#define HTTPS_CERT_THREAD_EVENT_CONF_APPLY_LATER    0x00000008 /* Apply configuration later */
#define HTTPS_CERT_THREAD_EVENT_APPLY_NOW           0x00000010 /* Apply configuration now (internal command) */
#define HTTPS_CERT_THREAD_EVENT_ANY         (HTTPS_CERT_THREAD_EVENT_CHECK_CA | HTTPS_CERT_THREAD_EVENT_REGEN_CA | HTTPS_CERT_THREAD_EVENT_CONF_APPLY | HTTPS_CERT_THREAD_EVENT_CONF_APPLY_LATER | HTTPS_CERT_THREAD_EVENT_APPLY_NOW) /* Any possible bit */
#define HTTPS_CERT_THREAD_EVENT_ANY_CA      (HTTPS_CERT_THREAD_EVENT_CHECK_CA | HTTPS_CERT_THREAD_EVENT_REGEN_CA | HTTPS_CERT_THREAD_EVENT_CONF_APPLY | HTTPS_CERT_THREAD_EVENT_CONF_APPLY_LATER)
static vtss_flag_t                          HTTPS_cert_thread_event;

#define HTTPS_GEN_THREAD_EVENT_GEN_CA       0x00000001 /* Generate cerificate */
static vtss_flag_t                          HTTPS_gen_thread_event;

/* global structure */
https_conf_t global_hiawatha_conf;
critd_t https_crit;
static BOOL global_cert_gen_status = FALSE;

/* helper macro - write config */
/* always check if the element contain a valid value */
#define CONF_WR(o, t, x)                        \
    do {                                        \
        if (t.x.value) {                        \
            o << "\n" << t.x.name << t.x.value; \
        }                                       \
    } while (0)

/* helper macro - write section, comments */
#define SECTION_WR(o, x)                        \
    do {                                        \
        o << "\n" << x;                         \
    } while (0)

/* overloading operator "<<" */
vtss::ostream &operator<<(vtss::ostream &o, const vtss_connections_t &c)
{
    CONF_WR(o, c, total);
    CONF_WR(o, c, per_ip);
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const vtss_binding_t &b)
{
    SECTION_WR(o, "# BINDING SETTINGS");
    SECTION_WR(o, "Binding {");
    {
        CONF_WR(o, b, port);
        CONF_WR(o, b, intf);
        CONF_WR(o, b, max_request_size);
        CONF_WR(o, b, ssl_cert_file);
    }
    SECTION_WR(o, "}");
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const vtss_fcgiserver_t &f)
{
    SECTION_WR(o, "# This defines the FastCGI interface to the WebStaX JSON/CGI server");
    SECTION_WR(o, "FastCGIserver {");
    {
        CONF_WR(o, f, connect_to);
        CONF_WR(o, f, fast_fgi_id);
        CONF_WR(o, f, extension);
    }
    SECTION_WR(o, "}");
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const vtss_urltoolkit_t &u)
{
    SECTION_WR(o, "UrlToolkit {");
    {
        CONF_WR(o, u, toolkit_id);
        if (u.force_tls) {
            CONF_WR(o, u, use_tls);
            CONF_WR(o, u, match_rewrite);
        }
        CONF_WR(o, u, request_uri);
        CONF_WR(o, u, match);

    }
    SECTION_WR(o, "}");
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const vtss_defwebsite_t &d)
{
    SECTION_WR(o, "# DEFAULT WEBSITE - WebStaX GUI");
    {
        CONF_WR(o, d, hostname);
        CONF_WR(o, d, website_root);
        CONF_WR(o, d, start_file);
        CONF_WR(o, d, access_log);
        CONF_WR(o, d, system_log);
        CONF_WR(o, d, garbage_log);
        CONF_WR(o, d, error_log);
        CONF_WR(o, d, exploit_log);
        CONF_WR(o, d, use_toolkit);
        CONF_WR(o, d, ignore_dot_hiawatha);
        CONF_WR(o, d, work_dir);
        CONF_WR(o, d, http_auth_to_cgi);
        CONF_WR(o, d, time_for_cgi);
        CONF_WR(o, d, custom_header1);
        CONF_WR(o, d, prevent_sqli);
        CONF_WR(o, d, prevent_xss);
        //        CONF_WR(o, d, min_ssl_version);  // Option removed as of Hiawatha 11.0
        CONF_WR(o, d, custom_header2);
        // RequireSSL is not allowed outside VirtualHost section
        // It is handled by fast_cgi.cxx
        // reference -> https://www.hiawatha-webserver.org/forum/topic/631
    }
    return o;
}

mesa_rc vtss_hiawatha_workdir_create(const char *workdir)
{
    int res;
    res = mkdir(VTSS_HIAWATHA_WORKDIR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (res == -1) {
        T_D("Create %s failed [%s]", VTSS_HIAWATHA_WORKDIR, strerror(errno));
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

mesa_rc vtss_hiawatha_conf_update(const char *file_name, const https_conf_t *conf)
{
    FILE *fp;
    vtss_connections_t connections;
    vtss_binding_t     binding_http, binding_https;
    vtss_fcgiserver_t  fcgiserver;
    vtss_urltoolkit_t  urltoolkit;
    vtss_defwebsite_t  defwebsite;

    /* make sure buf length is enough for hiawatha.conf */
    vtss::Buffer buf(4096);
    vtss::BufPtrStream ss(&buf);

    T_D("enter, https mode: %s, redirect: %s",
        conf->mode ? "ON" : "OFF",
        conf->redirect ? "ON" : "OFF");

    fp = fopen(VTSS_HIAWATHA_CONF_TMP, "w+");
    if (fp == NULL) {
        T_D("Open %s failed [%s]!", VTSS_HIAWATHA_CONF_TMP, strerror(errno));
        return VTSS_RC_ERROR;
    }

    if (chmod(VTSS_HIAWATHA_CONF_TMP, 0664) == -1) {
        T_D("Chmod %s failed [%s]!", VTSS_HIAWATHA_CONF_TMP, strerror(errno));
        fclose(fp);
        return VTSS_RC_ERROR;
    }

    binding_http.port.value = 80;
    binding_http.ssl_cert_file.value = NULL;

    binding_https.port.value = 443;
    binding_https.ssl_cert_file.value = HTTPS_CA_PEM_FILE;

    ss << connections << binding_http;
    binding_http.intf.value = "::"; //bind ipv6 address
    ss << binding_http << "\n";

    if (conf->mode) {
        ss << binding_https;
        binding_https.intf.value = "::"; //bind ipv6 address
        ss << binding_https << "\n";
        T_D("added binding for https(443)");
        if (conf->redirect) {
            urltoolkit.force_tls = TRUE;
        }
    }
    ss << fcgiserver << urltoolkit << defwebsite << "\n";

    fwrite(ss.begin(), 1, ss.size(), fp);
    fclose(fp);

    // rename does an atomic replacement of the old configuration file with the new.
    auto rc = rename(VTSS_HIAWATHA_CONF_TMP, file_name);
    if (rc != 0) {
        T_E("Could not rename %s to %s. Error: %s\n",
            VTSS_HIAWATHA_CONF_TMP, file_name, strerror(errno));
    }

    return VTSS_RC_OK;
}

/* simple copy function */
static mesa_rc conf_copy(const char *ori_file, const char *new_file)
{
    FILE *fp1, *fp2;
    int tmp;

    fp1 = fopen(ori_file, "r");
    if (fp1 == NULL) {
        printf("Open %s failed [%s]\n", ori_file, strerror(errno));
        return VTSS_RC_ERROR;
    }

    fp2 = fopen(new_file, "w+");
    if (fp2 == NULL) {
        printf("Open %s failed [%s]", new_file, strerror(errno));
        fclose(fp1);
        return VTSS_RC_ERROR;
    }

    while (1) {
        tmp = fgetc(fp1);
        if (!feof(fp1)) {
            fputc(tmp, fp2);
        } else {
            break;
        }
    }

    fclose(fp1);
    fclose(fp2);

    return VTSS_RC_OK;
}

mesa_rc vtss_mimetype_conf_map(void)
{
    return conf_copy(ORIG_MIMETYPE_CONF, VTSS_MIMETYPE_CONF);
}

mesa_rc vtss_cgi_wrapper_conf_map(void)
{
    return conf_copy(ORIG_CGI_WRAPPER_CONF, VTSS_CGI_WRAPPER_CONF);
}

/* adapt from vtss_appl/https/ */
/* HTTPS error text */
const char *https_error_txt(https_error_t rc)
{
    switch (rc) {
    case HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH:
        return "Operation only valid on the primary switch";

    case HTTPS_ERROR_INV_PARAM:
        return "Invalid parameter supplied to function";

    case HTTPS_ERROR_GET_CERT_INFO:
        return "Get certificate information fail";

    case HTTPS_ERROR_MUST_BE_DISABLED_MODE:
        return "Operation only valid under HTTPS mode disalbed";

    case HTTPS_ERROR_HTTPS_CORE_START:
        return "HTTPS core initial failed";

    case HTTPS_ERROR_INV_CERT:
        return "HTTPS invalid Certificate";

    case HTTPS_ERROR_INV_DH_PARAM:
        return "HTTPS invalid DH parameter";

    case HTTPS_ERROR_INTERNAL_RESOURCE:
        return "HTTPS out of internal resource";

    case HTTPS_ERROR_INV_URL:
        return "HTTPS invalid URL parameter";

    case HTTPS_ERROR_UPLOAD_CERT:
        return "Upload certificate failure";

    case HTTPS_ERROR_CERT_TOO_BIG:
        return "Certificate PEM file size too big";

    case HTTPS_ERROR_CERT_NOT_EXISTING:
        return "Certificate is not existing";

    case HTTPS_ERROR_GEN_HOSTKEY:
        return "Generate host key failed";

    case HTTPS_ERROR_GEN_CA:
        return "Generate certificate failed";

    default:
        return "HTTPS: Unknown error code";
    }
}

/* HTTPS Certificate status text */
const char *https_cert_status_txt(https_cert_status_t status)
{
    switch (status) {
    case HTTPS_CERT_PRESENT:
        return "Switch secure HTTP certificate is presented";
    case HTTPS_CERT_NOT_PRESENT:
        return "Switch secure HTTP certificate is not presented";
    case HTTPS_CERT_IS_GENERATING:
        return "Switch secure HTTP certificate is generating ...";
    case HTTPS_CERT_ERROR:
        return "Switch secure HTTP certificate is invalid";
    default:
        return "HTTPS: Unknown error code";
    }
}

/* Get HTTPS defaults */
void https_conf_mgmt_get_default(https_conf_t *conf)
{
    HTTPS_CRIT_ENTER();
    vtss_clear(*conf);
    conf->mode = HTTPS_MGMT_DEF_MODE;
    conf->redirect = HTTPS_MGMT_DEF_REDIRECT_MODE;
    if (conf != &global_hiawatha_conf) {
        conf->self_signed_cert = global_hiawatha_conf.self_signed_cert ? TRUE : FALSE;
        if (global_hiawatha_conf.server_cert[0] != '\0') {
            strcpy(conf->server_cert, global_hiawatha_conf.server_cert);
            strcpy(conf->server_pkey, global_hiawatha_conf.server_pkey);
            strcpy(conf->server_dh_parameters, global_hiawatha_conf.server_dh_parameters);
            strcpy(conf->server_pass_phrase, global_hiawatha_conf.server_pass_phrase);
        }
    }
    HTTPS_CRIT_EXIT();
}

int https_mgmt_conf_changed(const https_conf_t *const old, const https_conf_t *const new_)
{
    return (new_->mode != old->mode
            || new_->redirect != old->redirect
           );
}

mesa_rc https_mgmt_conf_get(https_conf_t *conf)
{
    if (conf == NULL) {
        return VTSS_RC_ERROR;
    }

    HTTPS_CRIT_ENTER();
    *conf = global_hiawatha_conf;
    HTTPS_CRIT_EXIT();
    return VTSS_RC_OK;
}

mesa_rc HTTPS_conf_apply(const https_conf_t *conf)
{
    https_conf_t        https_cfg;
    https_cert_status_t cert_status = https_mgmt_cert_status();

    https_cfg = *conf;
    if ((cert_status != HTTPS_CERT_PRESENT) && https_cfg.mode) {
        if (https_cfg.redirect) {
            // No certificate ready and HTTPS is forced...
            return VTSS_RC_ERROR;
        } else {
            // No certificate ready; disable HTTPS for now, will be enabled when cert is generated
            https_cfg.mode = FALSE;
            https_cfg.redirect = FALSE;
        }
    }

    if (vtss_hiawatha_conf_update(VTSS_HIAWATHA_CONF, &https_cfg) != VTSS_RC_OK) {
        T_W("hiawatha.conf update failed!");
    }

    vtss_process_hiawatha.adminMode(vtss::notifications::ProcessDaemon::DISABLE);
    vtss_process_hiawatha.stop_and_wait(true);

#ifdef VTSS_SW_OPTION_AUTH
    vtss_appl_auth_authen_agent_conf_t c;
    if (vtss_appl_auth_authen_agent_conf_get(VTSS_APPL_AUTH_AGENT_HTTP, &c) != VTSS_RC_OK) {
        T_W("vtss_appl_auth_authen_agent_conf_get(http) failed");
    } else {
        if (c.method[0] != VTSS_APPL_AUTH_AUTHEN_METHOD_NONE) {
            vtss_process_hiawatha.adminMode(vtss::notifications::ProcessDaemon::ENABLE);
        } else {
            T_D("hiawatha disabled!");
        }
    }
#else
    vtss_process_hiawatha.adminMode(vtss::notifications::ProcessDaemon::ENABLE);
#endif /* VTSS_SW_OPTION_AUTH */
    return VTSS_RC_OK;
}

mesa_rc https_mgmt_conf_set(const https_conf_t *conf)
{
    BOOL current_mode, mode_changed = FALSE;

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Enter");

    // Switch role check: must be primary switch
    if (!msg_switch_is_primary()) {
        T_WG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: not primary switch");
        return HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    // Parameter check
    if (conf == NULL) {
        T_EG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Null point");
        return HTTPS_ERROR_INV_PARAM;
    }
    if (conf->mode != HTTPS_MGMT_ENABLED && conf->mode != HTTPS_MGMT_DISABLED) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: Global mode is neither enabled nor disabld");
        return HTTPS_ERROR_INV_PARAM;
    }
    if (conf->redirect != HTTPS_MGMT_ENABLED && conf->redirect != HTTPS_MGMT_DISABLED) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: Redirect mode is neither enabled nor disabld");
        return HTTPS_ERROR_INV_PARAM;
    }

    HTTPS_CRIT_ENTER();
    current_mode = global_hiawatha_conf.mode;
    if (current_mode != conf->mode) {
        /* BZ#20784: After applying change HTTP mode, HTTP configured web page is
         * broken via Chrome browser (but it works on IE/FF).
         *
         * Applying mode change will restart the HTTP server and the current HTTP
         * connected sessions will be disconnected, so we need to apply the new
         * configuration later. Because if the SET operation comes from HTTP session,
         * we have to reply its HTTP request before restart the HTTP server.
         * Especially for Chrome browser, it take the different behavior (compare to
         * IE/FF) when receiving the HTTP code 304 (Not Modified).
         */
        mode_changed = TRUE;
    }
    HTTPS_CRIT_EXIT();

    // Saving the new configuration
    HTTPS_CRIT_ENTER();
    global_hiawatha_conf.mode = conf->mode;
    global_hiawatha_conf.redirect = conf->mode ? conf->redirect : HTTPS_MGMT_DISABLED;
    HTTPS_CRIT_EXIT();

    /* Activate changed configuration */
    HTTPS_CRIT_ENTER();
    vtss_flag_setbits(&HTTPS_cert_thread_event,
                      mode_changed ? HTTPS_CERT_THREAD_EVENT_CONF_APPLY_LATER : HTTPS_CERT_THREAD_EVENT_CONF_APPLY);
    HTTPS_CRIT_EXIT();

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: OK");
    return VTSS_RC_OK;
}

mesa_rc https_mgmt_conf_reload(void)
{
    /* Activate configuration */
    HTTPS_CRIT_ENTER();
    vtss_flag_setbits(&HTTPS_cert_thread_event, HTTPS_CERT_THREAD_EVENT_CONF_APPLY);
    HTTPS_CRIT_EXIT();
    return VTSS_RC_OK;
}

void https_mgmt_cert_save(void)
{
    https_conf_blk_t *https_conf_blk_p;
    conf_blk_id_t    blk_id = CONF_BLK_HTTPS_CONF;

    if ((https_conf_blk_p = (https_conf_blk_t *)conf_sec_open(CONF_SEC_GLOBAL, blk_id, NULL)) == NULL) {
        T_W("Failed to open HTTPS table");
    } else {
        HTTPS_CRIT_ENTER();
        https_conf_blk_p->https_conf = global_hiawatha_conf;
        HTTPS_CRIT_EXIT();
        https_conf_blk_p->https_conf.mode = HTTPS_MGMT_DEF_MODE;
        https_conf_blk_p->https_conf.redirect = HTTPS_MGMT_DEF_REDIRECT_MODE;
        https_conf_blk_p->https_conf.active_sess_timeout = HTTPS_MGMT_DEF_ACTIVE_SESS_TIMEOUT;
        https_conf_blk_p->https_conf.absolute_sess_timeout = HTTPS_MGMT_DEF_ABSOLUTE_SESS_TIMEOUT;
        conf_sec_close(CONF_SEC_GLOBAL, blk_id);
    }
}

/**
 * Get HTTPS system parameters
 *
 * To read current system parameters in HTTPS
 *
 * \param param [OUT] the HTTPS system configuration data
 *
 * \return VTSS_RC_OK if the operation successed.
 */
mesa_rc vtss_appl_https_system_config_get(
    vtss_appl_https_param_t *const param)
{
    // Parameter check
    if (param == NULL) {
        T_EG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Null point");
        return HTTPS_ERROR_INV_PARAM;
    }

    // Switch role check: must be primary switch
    if (!msg_switch_is_primary()) {
        T_WG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: not primary switch");
        return HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    HTTPS_CRIT_ENTER();
    param->mode = global_hiawatha_conf.mode;
    param->redirect_to_https = global_hiawatha_conf.redirect;
    HTTPS_CRIT_EXIT();
    return VTSS_RC_OK;
}

/**
 * Set HTTPS system parameters
 *
 * To modify current system parameters in HTTPS.
 *
 * \param param [IN] The HTTPS system configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_https_system_config_set(
    const vtss_appl_https_param_t *const param)
{
    mesa_rc         rc;
    https_conf_t    conf;

    // Parameter check
    if (param == NULL) {
        T_EG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Null point");
        return HTTPS_ERROR_INV_PARAM;
    }

    // Switch role check: must be primary switch
    if (!msg_switch_is_primary()) {
        T_WG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: not primary switch");
        return HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    // Saving the new configuration
    vtss_clear(conf);
    conf.mode = param->mode;
    conf.redirect = param->redirect_to_https;
    if ((rc = https_mgmt_conf_set(&conf)) != VTSS_RC_OK) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Calling https_mgmt_conf_set() failed");
    }

    return rc;
}

/*==============================================================================

    Certificate management APIs

==============================================================================*/

/*
 * Callback data structure for HTTPS certificate data download
 */
struct HTTPS_cert_netload_data_t {
    u8          *buffer;
    size_t      max_length;
    size_t      curr_size;

    size_t get_remain()
    {
        return max_length - curr_size;
    }
};

/*
 * Callback for HTTPS certificate data download
 */
static size_t HTTPS_cert_write_callback_chunk(char *ptr, size_t size, size_t nmemb, void *callback_context)
{
    auto cbdata = (HTTPS_cert_netload_data_t *)callback_context;
    size_t total_size = size * nmemb;

    if (cbdata->get_remain() < total_size) {
        T_D("Not enough space to save data: Needed %u, have: %u", total_size, cbdata->get_remain());
        return 0;
    }

    memcpy(&(cbdata->buffer[cbdata->curr_size]), ptr, total_size);
    cbdata->curr_size += total_size;

    return total_size;
}

/* Download certificate */
static u8 *HTTPS_cert_netload(const char *target, size_t max_size, size_t *actual_size)
{
    u8          *buffer = NULL;
    misc_url_parts_t url_parts;
    HTTPS_cert_netload_data_t cbdata;
    vtss::remote_file_options_t options;
    int err;

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Enter");

    misc_url_parts_init(&url_parts,
                        MISC_URL_PROTOCOL_TFTP |
                        MISC_URL_PROTOCOL_FTP |
                        MISC_URL_PROTOCOL_HTTP |
                        MISC_URL_PROTOCOL_SFTP |
                        MISC_URL_PROTOCOL_SCP);

    if (!misc_url_decompose(target, &url_parts)) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "%s is an invalid URL - Expecting something like [tftp|http|ftp]://<username>:<password>@<host>/<path>\n", target);
        return nullptr;
    }

    if ((VTSS_MALLOC_CAST(cbdata.buffer, max_size)) == NULL) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Failed to alloc dynamic memory for loading HTTPS certificate.\n");
        return nullptr;
    }

    cbdata.max_length = max_size;
    cbdata.curr_size = 0;

    vtss_clear(options);

    if (!vtss::remote_file_get_chunked(&url_parts, HTTPS_cert_write_callback_chunk, &cbdata, options, &err)) {
        T_D("Error fetching HTTPS certificate: %s", vtss::remote_file_errstring_get(err));
        VTSS_FREE(cbdata.buffer);
        return nullptr;
    }

    *actual_size = cbdata.curr_size;
    return cbdata.buffer;

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit");
    return buffer;
}

#if defined(HTTPS_USE_MBEDTLS_LIB)
/* Generate RSA key */
static mesa_rc HTTP_rsa_key_gen(int keysize)
{
    mesa_rc             rc = HTTPS_ERROR_GEN_HOSTKEY;
    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char               *personal_data = "ISTAX mbedTLS RSA generator";
    mbedtls_pk_context       key;
    u8                       output_buf[HTTPS_MGMT_MAX_PKEY_LEN * 2];
    FILE                     *key_file = NULL;
    size_t                   len = 0;

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Enter");

    // Seeding the random number generator
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Seeding the random number generator ... Start");
    if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                              (unsigned char *)personal_data, strlen(personal_data)) != 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Calling mbedtls_ctr_drbg_seed() failed");
        goto key_gen_end;
    }
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Seeding the random number generator ... Done");

    // 1 - Generate RSA private key, at least 1024 bits
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Generate RSA private key ... Start");
    mbedtls_pk_init(&key);
    if (mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)) != 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Calling mbedtls_pk_setup() failed");
        goto key_gen_end;
    }
    if (mbedtls_rsa_gen_key(mbedtls_pk_rsa(key), mbedtls_ctr_drbg_random, &ctr_drbg, keysize, 65537) != 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Calling rsa_gen_key() failed");
        goto key_gen_end;
    }
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Generate RSA private key ... Done");

    // 2 - Output private key file
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Write private key file ... Start");
    memset(output_buf, 0, sizeof(output_buf));
    if (mbedtls_pk_write_key_pem(&key, output_buf, sizeof(output_buf)) != 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Calling mbedtls_pk_write_key_pem() failed");
        goto key_gen_end;
    }
    len = strlen((char *)output_buf);
    if ((key_file = fopen(HTTPS_RSA_PEM_FILE, "w+")) == NULL) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Open hostkey file failed");
        goto key_gen_end;
    }
    if (fwrite(output_buf, 1, len, key_file) != len) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Wirte hostkey file failed");
        goto key_gen_end;
    }
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Write private key file ... Done");
    rc = VTSS_RC_OK;

key_gen_end:
    if (key_file) {
        fclose(key_file);
        if (rc != VTSS_RC_OK) {
            (void)remove(HTTPS_RSA_PEM_FILE);
        }
    }
    mbedtls_pk_free(&key);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit");
    return rc;
}
#endif /* HTTPS_USE_MBEDTLS_LIB */

/* Generate selfsigned certificate */
static mesa_rc HTTPS_cert_gen(void)
{
#if defined(HTTPS_USE_MBEDTLS_LIB)
    mesa_rc                  rc = HTTPS_ERROR_GEN_CA;
    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char               *personal_data = "ISTAX mbedTLS Self Signed Certificate";
    mbedtls_mpi              serial;
    mbedtls_pk_context       private_key;
    mbedtls_x509write_cert   crt;
    FILE                     *crt_pem_file = NULL, *rsa_key_file = NULL;
    u8                       output_buf[HTTPS_MGMT_MAX_CERT_LEN + HTTPS_MGMT_MAX_PKEY_LEN];
    long                     len = 0;
    char                     tmp_buf[512];

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Enter");

    // Switch role check: must be primary switch
    if (!msg_switch_is_primary()) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: not primary switch");
        return HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    // Initialize
    mbedtls_entropy_init(&entropy);
    mbedtls_pk_init(&private_key);
    mbedtls_x509write_crt_init(&crt);
    mbedtls_mpi_init(&serial);

    // Seeding the random number generator
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Seeding the random number generator ... Start");
    mbedtls_ctr_drbg_init(&ctr_drbg);
    if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                              (unsigned char *)personal_data, strlen(personal_data)) != 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Calling mbedtls_ctr_drbg_seed() failed");
        goto cert_gen_end;
    }
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Seeding the random number generator ... Done");

    // Generate RSA key
    if ((rc = HTTP_rsa_key_gen(HTTPS_RSA_KEY_SIZE)) != VTSS_RC_OK) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: HTTP_rsa_key_gen() failed");
        goto cert_gen_end;
    }

    // Delete the original one
    (void)remove(HTTPS_CA_PEM_FILE);

    // 1 - Load certificate key
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Load private key ... Start");
    if (mbedtls_pk_parse_keyfile(&private_key, HTTPS_RSA_PEM_FILE, "") != 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Load private key failed");
        goto cert_gen_end;
    }
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Load private key ... Done");

    // 2.1 - Set certificate version - 0: X509_CRT_VERSION_1    1: X509_CRT_VERSION_2    2: X509_CRT_VERSION_3
    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA512);
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate version X509_CRT_VERSION_3");
    mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);

    // 2.2 - Set certificate serial number (need > 1, browser don't accept serial number 1)
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate serial number ... Start");
    sprintf(tmp_buf, "%u", (rand() & 0x7FFFFFF0) + 1);
    if (mbedtls_mpi_read_string(&serial, 10, tmp_buf) != 0 ||
        mbedtls_x509write_crt_set_serial(&crt, &serial) != 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate serial number failed");
        goto cert_gen_end;
    }
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate serial number ... Start");

    // 2.3 - Set certificate valid time (20 years)
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate valid time ... Start");
    if (mbedtls_x509write_crt_set_validity(&crt, "20201031000000", "20401031235959") != 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate valid time failed");
        goto cert_gen_end;
    }
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate valid time ... Done");

    // 2.4 - Set certificate subject name
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate subject name ... Start");
#if defined(VTSS_PRODUCT_NAME)
    // Common Name (CN), Organization (O), Organizational Unit (OU), Country (C), State (S), Locality (L)
    sprintf(tmp_buf, "CN=%s,O=%s-Selfsigned", VTSS_PRODUCT_NAME, VTSS_PRODUCT_NAME);
#else
    sprintf(tmp_buf, "CN=ISTAX,O=ISTAX-Selfsigned",);
#endif /* VTSS_PRODUCT_NAME */
    if (mbedtls_x509write_crt_set_subject_name(&crt, tmp_buf) != 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate subject name failed");
        goto cert_gen_end;
    }
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate subject name ... Done");

    // 2.5 - Set certificate subject key
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate subject key ... Start");
    mbedtls_x509write_crt_set_subject_key(&crt, &private_key);
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate subject key ... Done");

    // 2.6 - Set certificate issuer name
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate issuer name ... Start");
    if (mbedtls_x509write_crt_set_issuer_name(&crt, tmp_buf) != 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate issuer name failed");
        goto cert_gen_end;
    }
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate issuer name ... Done");

    // 2.7 - Set certificate sign
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate sign ... Start");
    mbedtls_x509write_crt_set_issuer_key(&crt, &private_key);
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set certificate sign ... Done");

    // 2.8 - Set certificate sign - Not CA
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set basic constraints ... Start");
    if (mbedtls_x509write_crt_set_basic_constraints(&crt, 0, -1) != 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set basic constraints failed");
        goto cert_gen_end;
    }
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Set basic constraints ... Done");

    // 3 - Output pem file
    // 3.1 - Write certificate to CA PEM file
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Write certificate to CA PEM file ... Start");
    memset(output_buf, 0, sizeof(output_buf));
    if (mbedtls_x509write_crt_pem(&crt, output_buf, sizeof(output_buf), mbedtls_ctr_drbg_random, &ctr_drbg) < 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Calling mbedtls_x509write_crt_pem() failed");
        goto cert_gen_end;
    }
    if ((crt_pem_file = fopen(HTTPS_CA_PEM_FILE, "w+")) == NULL) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Open CA PEM file failed");
        goto cert_gen_end;
    }
    len = strlen((char *)output_buf);
    if (fwrite(output_buf, 1, len, crt_pem_file) != len) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Write certificate to CA PEM file failed");
        fclose(crt_pem_file);
        crt_pem_file = NULL;
        (void)remove(HTTPS_CA_PEM_FILE);
        goto cert_gen_end;
    } else {
        fclose(crt_pem_file);
    }

    // 3.2 - Write rsa key to CA PEM file
    if ((crt_pem_file = fopen(HTTPS_CA_PEM_FILE, "a")) == NULL ||
        (rsa_key_file = fopen(HTTPS_RSA_PEM_FILE, "r")) == NULL) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Open CA PEM file failed");
        goto cert_gen_end;
    }
    fseek(rsa_key_file, 0, SEEK_END);
    if ((len = ftell(rsa_key_file)) <= 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Calling ftell() failed");
        goto cert_gen_end;
    }
    rewind(rsa_key_file);
    memset(output_buf, 0, sizeof(output_buf));
    if (fread(output_buf, 1, len, rsa_key_file) != len) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Read RSA key file failed");
        goto cert_gen_end;
    }
    if (fwrite(output_buf, 1, len, crt_pem_file) != len) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Write RSA key to CA PEM file failed");
        goto cert_gen_end;
    }
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Write certificate to pem file ... Done");
    rc = VTSS_RC_OK;

cert_gen_end:
    if (rsa_key_file) {
        fclose(rsa_key_file);
    }
    if (crt_pem_file) {
        fclose(crt_pem_file);
    }
    mbedtls_x509write_crt_free(&crt);
    mbedtls_pk_free(&private_key);
    mbedtls_mpi_free(&serial);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    HTTPS_CRIT_ENTER();
    global_cert_gen_status = FALSE;
    HTTPS_CRIT_EXIT();

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit");
    return rc;
#else
    return VTSS_RC_ERROR;
#endif /* HTTPS_USE_MBEDTLS_LIB */
}

/* Remove pass phrase from private key */
static mesa_rc HTTPS_cert_pass_phrase_remove(const char *pass_phrase)
{
    mesa_rc     rc = VTSS_RC_OK;
    u8          output_buf[HTTPS_MGMT_MAX_CERT_LEN + HTTPS_MGMT_MAX_PKEY_LEN];

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Enter");

    if (strlen(pass_phrase) == 0) {
        // Copy upload file
        memset((output_buf), 0x0, sizeof(output_buf));
        sprintf((char *)output_buf, "cp -f %s %s", HTTPS_CA_PEM_UPLOAD_FILE, HTTPS_CA_PEM_FILE);
        (void)system((char *)output_buf);
#if defined(HTTPS_USE_MBEDTLS_LIB)
    } else {
        mbedtls_x509_crt    crt;
        mbedtls_pk_context  key;
        FILE                *crt_pem_file = NULL;
        long                len = 0;
        char                *crt_str_start, *crt_str_end;

        mbedtls_x509_crt_init(&crt);
        mbedtls_pk_init(&key);

        // Check the CA pem file
        if (mbedtls_x509_crt_parse_file(&crt, HTTPS_CA_PEM_UPLOAD_FILE) != 0 ||
            mbedtls_pk_parse_keyfile(&key, HTTPS_CA_PEM_UPLOAD_FILE, pass_phrase)) {
            T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "CA/KEY is invalid");
            rc = HTTPS_ERROR_INV_CERT;
            goto remove_end;
        }

        // Output CA pem file
        rc = HTTPS_ERROR_INTERNAL_RESOURCE;

        // Read certificate from the upload pem file
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Read certificate");
        if ((crt_pem_file = fopen(HTTPS_CA_PEM_UPLOAD_FILE, "r")) == NULL) {
            T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Open CA PEM upload file failed");
            goto remove_end;
        }
        fseek(crt_pem_file, 0, SEEK_END);
        if ((len = ftell(crt_pem_file)) <= 0) {
            T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Calling ftell() failed");
            goto remove_end;
        }
        rewind(crt_pem_file);
        memset(output_buf, 0, sizeof(output_buf));
        if (fread(output_buf, 1, len, crt_pem_file) != len) {
            T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Read CA PEM upload file failed");
            goto remove_end;
        }
        if ((crt_str_start = strstr((char *)output_buf, HTTPS_CA_PEM_CA_HEADER_START)) == 0 ||
            (crt_str_end = strstr((char *)output_buf, HTTPS_CA_PEM_CA_HEADER_END)) == 0) {
            T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Get CA info. failed");
            goto remove_end;
        }
        crt_str_end += strlen(HTTPS_CA_PEM_CA_HEADER_END);
        fclose(crt_pem_file);
        crt_pem_file = NULL;

        // Write certificate
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Write certificate");
        if ((crt_pem_file = fopen(HTTPS_CA_PEM_FILE, "w+")) == NULL) {
            T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Open CA PEM file failed");
            goto remove_end;
        }

        len = crt_str_end - crt_str_start;
        if (fwrite(crt_str_start, 1, len, crt_pem_file) != len) {
            T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Write certificate to CA PEM file failed");
            goto remove_end;
        } else {
            (void) fwrite("\n", 1, sizeof("\n"), crt_pem_file);

            // Write new private key
            T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Write private key");
            memset(output_buf, 0, sizeof(output_buf));
            if (mbedtls_pk_write_key_pem(&key, output_buf, sizeof(output_buf)) != 0) {
                T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Calling mbedtls_pk_write_key_pem() failed");
                goto remove_end;
            }
            len = strlen((char *)output_buf);
            if (fwrite(output_buf, 1, len, crt_pem_file) != len) {
                T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Write hostkey file failed");
                goto remove_end;
            }
            (void) fwrite("\n", 1, sizeof("\n"), crt_pem_file);
            rc = VTSS_RC_OK;
        }

remove_end:
        if (crt_pem_file) {
            fclose(crt_pem_file);
        }
        mbedtls_pk_free(&key);
        mbedtls_x509_crt_free(&crt);
#endif /* HTTPS_USE_MBEDTLS_LIB */
    }

    if (rc == VTSS_RC_OK) {
        HTTPS_CRIT_ENTER();
        global_hiawatha_conf.self_signed_cert = FALSE;
        HTTPS_CRIT_EXIT();
    }

    (void)remove(HTTPS_CA_PEM_UPLOAD_FILE);
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit");
    return rc;
}

static void HTTPS_cert_thread(vtss_addrword_t data)
{
    vtss_flag_value_t   received_events;
    https_conf_t        https_cfg;

    while (1) {
        received_events = vtss_flag_wait(&HTTPS_cert_thread_event, HTTPS_CERT_THREAD_EVENT_ANY, VTSS_FLAG_WAITMODE_OR_CLR);
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Receiving HTTPS thread event");

        if (received_events & HTTPS_CERT_THREAD_EVENT_ANY_CA) {
            T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Received EVENT_ANY_CA");
            // Create a selfsigned CA if it is not existing or invalid.
            if (https_mgmt_cert_status() != HTTPS_CERT_PRESENT ||
                (received_events & HTTPS_CERT_THREAD_EVENT_REGEN_CA)) {
                HTTPS_CRIT_ENTER();
                if (!global_cert_gen_status) {
                    global_cert_gen_status = TRUE;
                    vtss_flag_setbits(&HTTPS_gen_thread_event, HTTPS_GEN_THREAD_EVENT_GEN_CA);
                }
                HTTPS_CRIT_EXIT();

                /* BZ#22782 - The HTTPS configuration web page is broken on Chrome browser (Version 57.0.2987.133).
                 * Unlike IE or Firfox, Chorme won't handle the process of HTTP connection reset.
                 * So we stop the CA generate process here and apply the new CA after it is done.
                 */
                continue;
            }
            if (received_events & HTTPS_CERT_THREAD_EVENT_CONF_APPLY_LATER) {
                VTSS_OS_MSLEEP(2000);
            }
            HTTPS_CRIT_ENTER();
            vtss_flag_setbits(&HTTPS_cert_thread_event, HTTPS_CERT_THREAD_EVENT_APPLY_NOW);
            HTTPS_CRIT_EXIT();
        }

        if (received_events & HTTPS_CERT_THREAD_EVENT_APPLY_NOW) {
            T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Received EVENT_APPLY_NOW");
            HTTPS_CRIT_ENTER();
            https_cfg = global_hiawatha_conf;
            HTTPS_CRIT_EXIT();
            (void)HTTPS_conf_apply(&https_cfg);
        }

    }
}

/* Generate RSA key will take a long time and the process occupy a hugh stack size.
   We create a new thread here for the RSA key generating instead of in 'Init Modules' thread.
   That we won't wait a long time and occur stack size overflow in 'Init Modules' thread. */
static void HTTPS_gen_thread(vtss_addrword_t data)
{
    char         cmdbuf[512];
    https_conf_t https_cfg;

    while (1) {
        (void)vtss_flag_wait(&HTTPS_gen_thread_event, HTTPS_GEN_THREAD_EVENT_GEN_CA, VTSS_FLAG_WAITMODE_OR_CLR);
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Receiving HTTPS gen thread event");

        // Create CA directory if it is not existing
        memset(cmdbuf, 0x0, sizeof(cmdbuf));
        sprintf(cmdbuf, "mkdir -p %s", HTTPS_CA_DIR);
        (void)system(cmdbuf);

        // Generate CA and apply it when global mode is enabeld
        if (HTTPS_cert_gen() == VTSS_RC_OK) {
            HTTPS_CRIT_ENTER();
            T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Cert generated, https enable = %d", global_hiawatha_conf.mode);
            vtss_flag_setbits(&HTTPS_cert_thread_event, HTTPS_CERT_THREAD_EVENT_APPLY_NOW);
            HTTPS_CRIT_EXIT();
        } else {
            T_EG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Cert generate error");
        }
    }
}

/**
  * \brief Check the HTTPS certificate/key is valid or not.
  *
  * \param server_cert [IN]: Pointer to the sting of certificate.
  * \param server_key  [IN]: Pointer to the sting of privary key. Use NULL input for this parameter when the input 'server_cert' already includes the private key.
  * \param pass_phrase [IN]: Pointer to the sting of privary key pass phrase.
  *
  * \return
  * TRUE for valid certificate, otherwise FLASE.
  */
BOOL https_mgmt_cert_key_is_valid(const char *server_cert, const char *server_key, const char *pass_phrase)
{
#if defined(HTTPS_USE_MBEDTLS_LIB)
    BOOL               is_valid = TRUE;
    mbedtls_x509_crt   crt;
    mbedtls_pk_context key;
    int                ret = 0;
    FILE                *crt_pem_file;
    size_t              server_cert_len, server_key_len;

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Enter");

    // Check general parameter
    if (server_cert == NULL) {
        T_EG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: server_cert is NULL point");
        return FALSE;
    }

    mbedtls_x509_crt_init(&crt);
    mbedtls_pk_init(&key);

    // Wirte CA to PEM file
    if ((crt_pem_file = fopen(HTTPS_CA_PEM_UPLOAD_FILE, "w+")) == NULL) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "fopen() failed");
        is_valid = FALSE;
        goto cert_check_end;
    }
    if (crt_pem_file) {
        server_cert_len = strlen(server_cert);
        if (fwrite(server_cert, 1, server_cert_len, crt_pem_file) != server_cert_len) {
            T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "fwrite(server_cert) failed");
            is_valid = FALSE;
            fclose(crt_pem_file);
            goto cert_check_end;
        }

        if (server_key && strlen(server_key)) {
            server_key_len = strlen(server_key);
            if (fwrite(server_key, 1, server_key_len, crt_pem_file) != server_key_len) {
                T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "fwrite(server_key) failed");
                is_valid = FALSE;
                fclose(crt_pem_file);
                goto cert_check_end;
            }
        }

        fclose(crt_pem_file);
    }

    // Check certificate
    if (!crt_pem_file || mbedtls_x509_crt_parse_file(&crt, HTTPS_CA_PEM_UPLOAD_FILE) != 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Calling mbedtls_x509_crt_parse_file() failed");
        is_valid = FALSE;
        goto cert_check_end;
    }

    if ((ret = mbedtls_pk_parse_keyfile(&key, HTTPS_CA_PEM_UPLOAD_FILE, (pass_phrase == NULL || strlen(pass_phrase) == 0) ? "" : pass_phrase)) != 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Calling mbedtls_pk_parse_keyfile() failed, ret=%d", ret);
        is_valid = FALSE;
    }

cert_check_end:
    if (crt_pem_file) {
        (void)remove(HTTPS_CA_PEM_UPLOAD_FILE);
    }
    mbedtls_pk_free(&key);
    mbedtls_x509_crt_free(&crt);

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: is_valid = %s", is_valid ? "T" : "F");
    return is_valid;
#else
    return TRUE;
#endif /* HTTPS_USE_MBEDTLS_LIB */
}

/**
  * \brief Get the HTTPS certificate generation status.
  *
  * \return
  *  HTTPS_CERT_PRESENT         -     Certifiaction is presented.
  *  HTTPS_CERT_NOT_PRESENT     -     Certifiaction is not presented.
  *  HTTPS_CERT_IS_GENERATING   -     Certifiation is being generated.
  *  HTTPS_CERT_ERROR           -     Internal Error.
  */
https_cert_status_t https_mgmt_cert_status(void)
{
    https_cert_status_t cert_status = HTTPS_CERT_ERROR;
    BOOL                gen_status = FALSE;
    FILE                *file;
#if defined(HTTPS_USE_MBEDTLS_LIB)
    mbedtls_x509_crt    crt;
    mbedtls_pk_context  key;
#endif /* HTTPS_USE_MBEDTLS_LIB */

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Enter");

    HTTPS_CRIT_ENTER();
    gen_status = global_cert_gen_status;
    HTTPS_CRIT_EXIT();

    if (gen_status) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "HTTPS_CERT_IS_GENERATING");
        cert_status = HTTPS_CERT_IS_GENERATING;
    } else {
        file = fopen(HTTPS_CA_PEM_FILE, "r");
        if (file == NULL) {
            T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "HTTPS_CERT_NOT_PRESENT %s", HTTPS_CA_PEM_FILE);
            cert_status = HTTPS_CERT_NOT_PRESENT;
        } else {
            T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "HTTPS_CERT_PRESENT");

#if defined(HTTPS_USE_MBEDTLS_LIB)
            // Check CA/KEY
            mbedtls_x509_crt_init(&crt);
            mbedtls_pk_init(&key);

            if (mbedtls_x509_crt_parse_file(&crt, HTTPS_CA_PEM_FILE) != 0) {
                T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Calling mbedtls_x509_crt_parse_file() failed");
            } else if (mbedtls_pk_parse_keyfile(&key, HTTPS_CA_PEM_FILE, "")) {
                T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Calling mbedtls_pk_parse_keyfile() failed");
            } else {
                cert_status = HTTPS_CERT_PRESENT;
            }

            mbedtls_pk_free(&key);
            mbedtls_x509_crt_free(&crt);
#else
            cert_status = HTTPS_CERT_PRESENT;
#endif /* HTTPS_USE_MBEDTLS_LIB */

            fclose(file);
        }
    }

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: cert_status=%s", cert_status == HTTPS_CERT_PRESENT ? "Present" : "Not Present");
    return cert_status;
}

/* Delete the HTTPS certificate */
mesa_rc https_mgmt_cert_del(void)
{
    mesa_rc         rc;
    https_conf_t    conf;

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Enter");

    // Switch role check: must be primary switch
    if (!msg_switch_is_primary()) {
        T_WG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: not primary switch");
        return HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    // Pre-condition check: mode must be disabled
    if ((rc = https_mgmt_conf_get(&conf)) != VTSS_RC_OK) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: https_mgmt_conf_get() failed");
        return rc;
    }
    if (conf.mode == HTTPS_MGMT_ENABLED) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: HTTPS mode must be disabled");
        return HTTPS_ERROR_MUST_BE_DISABLED_MODE;
    }

    // Delete certificate
    (void)remove(HTTPS_CA_PEM_FILE);

    HTTPS_CRIT_ENTER();
    global_hiawatha_conf.self_signed_cert = TRUE;
    HTTPS_CRIT_EXIT();

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: OK");
    return VTSS_RC_OK;
}

/**
  * \brief Get the HTTPS certificate generation status.
  *
  * \return
  *    TRUE  - Certificate generation in progress.\n
  *    FALSE - No certificate generation in progress.
  */
mesa_rc https_mgmt_cert_gen(void)
{
    mesa_rc         rc;
    https_conf_t    conf;

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Enter");

    // Switch role check: must be primary switch
    if (!msg_switch_is_primary()) {
        T_WG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: not primary switch");
        return HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    // Pre-condition check: mode must be disabled
    if ((rc = https_mgmt_conf_get(&conf)) != VTSS_RC_OK) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: https_mgmt_conf_get() failed");
        return rc;
    }
    if (conf.mode == HTTPS_MGMT_ENABLED) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: HTTPS mode must be disabled");
        return HTTPS_ERROR_MUST_BE_DISABLED_MODE;
    }

    HTTPS_CRIT_ENTER();
    if (global_cert_gen_status == FALSE) { // Prevent repeated configuration
        vtss_flag_setbits(&HTTPS_cert_thread_event, HTTPS_CERT_THREAD_EVENT_REGEN_CA);
    }
    HTTPS_CRIT_EXIT();

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit");
    return VTSS_RC_OK;
}

/**
  * \brief Update the HTTPS certificate.
  *        Before calling this API, https_mgmt_cert_del() must be called first
  *        Or disable HTTPS mode first.
  *
  * \param cfg [IN]: Pointer to structure that contains the
  *                  global configuration to apply to the
  *                  voice VLAN module.
  * \return
  *    VTSS_RC_OK on success.\n
  *    HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    HTTPS_ERROR_INV_CERT if Certificate is invaled.\n
  *    HTTPS_ERROR_INV_DH_PARAM if DH parameters is invaled.\n
  */
mesa_rc https_mgmt_cert_update(https_conf_t *cfg)
{
    mesa_rc         rc;
    https_conf_t    conf;
    FILE            *crt_pem_file;

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Enter");

    // Switch role check: must be primary switch
    if (!msg_switch_is_primary()) {
        T_WG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: not primary switch");
        return HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    // Pre-condition check: mode must be disabled
    if ((rc = https_mgmt_conf_get(&conf)) != VTSS_RC_OK) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: https_mgmt_conf_get() failed");
        return rc;
    }
    if (conf.mode == HTTPS_MGMT_ENABLED) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: HTTPS mode must be disabled");
        return HTTPS_ERROR_MUST_BE_DISABLED_MODE;
    }

    // Check pem format (pass_phrase, rsa)
    if (https_mgmt_cert_key_is_valid(cfg->server_cert, cfg->server_pkey, cfg->server_pass_phrase) == FALSE) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: Invalid certificate");
        return HTTPS_ERROR_MUST_BE_DISABLED_MODE;
    }

    // Update CA PEM file
    if ((crt_pem_file = fopen(HTTPS_CA_PEM_UPLOAD_FILE, "w+")) == NULL) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "fopen() failed");
        return HTTPS_ERROR_INTERNAL_RESOURCE;
    } else {
        size_t len = sizeof(cfg->server_cert);
        if (fwrite(cfg->server_cert, 1, len, crt_pem_file) != len) {
            T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "fwrite() failed");
            rc = HTTPS_ERROR_INTERNAL_RESOURCE;
        }
        fclose(crt_pem_file);
        if (rc == VTSS_RC_OK) {
            rc = HTTPS_cert_pass_phrase_remove(cfg->server_pass_phrase);
        }
    }

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: %s", rc == VTSS_RC_OK ? "OK" : "Not OK");
    return rc;
}

/**
  * \brief Upload HTTPS certificate from URL
  *
  * \param url         [IN]: Pointer to the URL where stored the certificate PEM file.
  * \param pass_phrase [IN]: Pointer to the string of privary key pass phrase.
  *
  * \return
  *    HTTPS_ERROR_MUST_BE_DISABLED_MODE - Current HTTPS mode is enabled, this operation can not be processed.\n
  *    HTTPS_ERROR_INTERNAL_RESOURCE - Dynamic memory allocated failure.\n
  *    HTTPS_ERROR_INV_URL - Invalid URL.\n
  *    HTTPS_ERROR_UPLOAD_CERT - Download process failure.\n
  *    HTTPS_ERROR_INV_CERT - Invalid certificate format.\n
  *    HTTPS_ERROR_CERT_TOO_BIG - Certificate PEM file size too big.
  */
mesa_rc https_mgmt_cert_upload(const char *url, const char *pass_phrase)
{
    mesa_rc             rc = VTSS_RC_OK;
    u8                  *buffer = NULL;
    size_t              recv_len = 0;
    https_conf_t        conf;
    misc_url_parts_t    url_parts;
    FILE                *crt_pem_file;

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Enter: url=%s, pass_phrase:%s", url, pass_phrase ? pass_phrase : "");

    // Switch role check: must be primary switch
    if (!msg_switch_is_primary()) {
        T_WG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: not primary switch");
        return HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    // Pre-condition check: mode must be disabled
    if ((rc = https_mgmt_conf_get(&conf)) != VTSS_RC_OK) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: https_mgmt_conf_get() failed");
        return rc;
    }
    if (conf.mode == HTTPS_MGMT_ENABLED) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: HTTPS mode must be disabled");
        return HTTPS_ERROR_MUST_BE_DISABLED_MODE;
    }

    // Download protocol check
    if (strncmp(url, "https://", 8) &&
#if defined(CYGPKG_DEVS_DISK_MMC) && defined(CYGPKG_FS_FAT)
        strncmp(url, "file:///", 8) &&
#endif
        strncmp(url, "http://", 7) &&
        strncmp(url, "tftp://", 7) &&
        strncmp(url, "ftp://", 6)) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: Download protocol is not supported");
        return HTTPS_ERROR_INV_URL;
    }

    strcpy(conf.server_pass_phrase, pass_phrase ? pass_phrase : "");

    do {
        misc_url_parts_init(&url_parts, MISC_URL_PROTOCOL_TFTP  |
                            MISC_URL_PROTOCOL_FTP   |
                            MISC_URL_PROTOCOL_HTTP  |
                            MISC_URL_PROTOCOL_HTTPS |
                            MISC_URL_PROTOCOL_FILE);

        if (!misc_url_decompose(url, &url_parts)) {
            rc = HTTPS_ERROR_INV_URL;
            break;
        }

        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Downloading file ... Start");
        if (!strcmp(url_parts.protocol, "tftp") &&
            (!url_parts.port || url_parts.port == 69 /* Not supported port assign yet */)) {
            int                 tftp_rc;
            struct hostent      *host = NULL;
            struct in_addr      address;
            struct sockaddr_in  server_addr;

            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;

            /* Look up the DNS host name or IPv4 address */
            if ((server_addr.sin_addr.s_addr = inet_addr(url_parts.host)) == 0xFFFFFFFF) {
                if ((host = gethostbyname(url_parts.host)) != NULL &&
                    (host->h_length == sizeof(struct in_addr))) {
                    address = *((struct in_addr **)host->h_addr_list)[0];
                    server_addr.sin_addr.s_addr = address.s_addr;
                } else {
                    rc = HTTPS_ERROR_INV_URL;
                    break;
                }
            }

            if ((VTSS_MALLOC_CAST(buffer, HTTPS_MGMT_MAX_CERT_LEN + 1)) == NULL) {
                rc = HTTPS_ERROR_INTERNAL_RESOURCE;
                break;
            }

            // Get tftp data
            if ((recv_len = vtss_tftp_get(url_parts.path, url_parts.host, url_parts.port, (char *) buffer, HTTPS_MGMT_MAX_CERT_LEN, true, &tftp_rc)) <= 0) {
                rc = HTTPS_ERROR_UPLOAD_CERT;
                break;
            }

            /* Check certificate size */
            if (recv_len > HTTPS_MGMT_MAX_CERT_LEN) {
                rc = HTTPS_ERROR_CERT_TOO_BIG;
                break;
            }
        } else if (!strcmp(url_parts.protocol, "ftp") ||
#if defined(CYGPKG_DEVS_DISK_MMC) && defined(CYGPKG_FS_FAT)
                   !strcmp(url_parts.protocol, "file") ||
#endif
                   !strcmp(url_parts.protocol, "http") ||
                   !strcmp(url_parts.protocol, "https")) {
            if ((buffer = HTTPS_cert_netload(url, HTTPS_MGMT_MAX_CERT_LEN + 1, &recv_len)) == NULL) {
                rc = HTTPS_ERROR_UPLOAD_CERT;
                break;
            }
        } else {
            rc = HTTPS_ERROR_INV_URL;
            break;
        }
    } while (0);
    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Downloading file ... Done");

    /* Check certificate is valid */
    if (rc == VTSS_RC_OK && buffer) {
        if (https_mgmt_cert_key_is_valid((char *)buffer, NULL, pass_phrase) == FALSE) {
            rc = HTTPS_ERROR_INV_CERT;
        } else {
            // Update CA PEM file
            if ((crt_pem_file = fopen(HTTPS_CA_PEM_UPLOAD_FILE, "w+")) == NULL) {
                T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "fopen() failed");
                rc = HTTPS_ERROR_INTERNAL_RESOURCE;
            } else {
                if (fwrite(buffer, 1, recv_len, crt_pem_file) != recv_len) {
                    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "fwrite() failed");
                    rc = HTTPS_ERROR_INTERNAL_RESOURCE;
                }
                fclose(crt_pem_file);
                if (rc == VTSS_RC_OK) {
                    rc = HTTPS_cert_pass_phrase_remove(pass_phrase);
                }
            }
        }
    }

    // Free resource
    if (buffer) {
        VTSS_FREE(buffer);
    }

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit: %s", rc == VTSS_RC_OK ? "OK" : "Not OK");
    return rc;
}

/**
  * \brief Get the HTTPS certificate information.
  *
  * \param cert_info [OUT]: The certificate information.
  * \param buf_len   [IN]:  The output buffer length.
   * \return
  *    VTSS_RC_OK on success.\n
  *    HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    HTTPS_ERROR_GET_CERT_INFO if get fail.\n
  */
mesa_rc https_mgmt_cert_info_get(char *cert_info, size_t buf_len)
{
#if defined(HTTPS_USE_MBEDTLS_LIB)
    mesa_rc          rc = VTSS_RC_OK;
    mbedtls_x509_crt crt;

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Enter");

    mbedtls_x509_crt_init(&crt);
    if (mbedtls_x509_crt_parse_file(&crt, HTTPS_CA_PEM_FILE) != 0) {
        T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Calling mbedtls_x509_crt_parse_file() failed");
        rc = HTTPS_ERROR_INV_CERT;
    } else {
        mbedtls_x509_crt_info(cert_info, buf_len, "", &crt);
    }

    mbedtls_x509_crt_free(&crt);

    T_DG(VTSS_TRACE_FAST_CGI_GRP_HTTPS_MGMT, "Exit");
    return rc;
#else
    return VTSS_RC_ERROR;
#endif /* HTTPS_USE_MBEDTLS_LIB */
}

/*==============================================================================

    Module initialize API

==============================================================================*/
/* Initialize module */
mesa_rc https_init(vtss_init_data_t *data)
{
    switch (data->cmd) {
    case INIT_CMD_START:
        // Initialize thread event flags
        vtss_flag_init(&HTTPS_cert_thread_event);
        vtss_flag_init(&HTTPS_gen_thread_event);

        /* Create HTTPS certificate thread */
        vtss_thread_create(VTSS_THREAD_PRIO_BELOW_NORMAL,
                           HTTPS_cert_thread,
                           0,
                           "HTTPS certificate config",
                           nullptr,
                           0,
                           &HTTPS_cert_thread_handle,
                           &HTTPS_cert_thread_block);

        /* Create HTTPS generation thread */
        vtss_thread_create(VTSS_THREAD_PRIO_BELOW_NORMAL,
                           HTTPS_gen_thread,
                           0,
                           "HTTPS certificate generation",
                           nullptr,
                           0,
                           &HTTPS_gen_thread_handle,
                           &HTTPS_gen_thread_block);

        break;

    case INIT_CMD_CONF_DEF:
        vtss_flag_setbits(&HTTPS_cert_thread_event, HTTPS_CERT_THREAD_EVENT_CHECK_CA);
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        vtss_flag_setbits(&HTTPS_cert_thread_event, HTTPS_CERT_THREAD_EVENT_CHECK_CA);
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Upload HTTPS certificate from URL
 *
 * This is a wrapper function that is essentially calling
 * https_mgmt_cert_upload()
 *
 * \param cert_param [IN]: HTTPS certificate parameter
 *
 * \return VTSS_RC_OK if the operation succeed
 */
mesa_rc vtss_appl_https_cert_upload(
    const vtss_appl_https_cert_t *const cert)
{
    return https_mgmt_cert_upload(cert->url, cert->pass_phrase);
}

/**
 * \brief Delete HTTPS certificat
 *
 * This is a wrapper function that is essentially calling
 * https_mgmt_cert_del();
 *
 * \return VTSS_RC_OK if the operation succeed
 */
mesa_rc vtss_appl_https_cert_delete(
    const vtss_appl_https_cert_del_t *const del)
{
    if (del->del) {
        return https_mgmt_cert_del();
    }
    return VTSS_RC_OK;
}

/**
 * \brief Dummy function to delete HTTPS certificate
 * \return VTSS_RC_OK always return ok
 */
mesa_rc vtss_appl_https_cert_delete_dummy(
    vtss_appl_https_cert_del_t *const del)
{
    del->del = 0;
    return VTSS_RC_OK;
}

/**
 * \brief Generate HTTPS certificate
 *
 * \return VTSS_RC_OK if the operation succeed
 */
mesa_rc vtss_appl_https_cert_generate(
    const vtss_appl_https_cert_gen_t *const gen)
{
    if (gen->gen) {
        return https_mgmt_cert_gen();
    }
    return VTSS_RC_OK;
}
/**
 * \brief Dummy function that generate HTTPS certificate
 *
 * \return VTSS_RC_OK always return VTSS_OL
 */
mesa_rc vtss_appl_https_cert_generate_dummy(
    vtss_appl_https_cert_gen_t *const gen)
{
    gen->gen = 0;
    return VTSS_RC_OK;
}
