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

#include "main.h"
#include "web_api.h"
#include "vtss_https_api.hxx"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_HTTPS


typedef enum {
    VTSS_HTTPS_CERT_COMMAND_NONE,
    VTSS_HTTPS_CERT_COMMAND_DELETE,
    VTSS_HTTPS_CERT_COMMAND_UPLOAD,
    VTSS_HTTPS_CERT_COMMAND_GENERATE,
    VTSS_HTTPS_CERT_COMMAND_END
} https_cert_command_t;

typedef enum {
    VTSS_HTTPS_CERT_DOWNLOAD_WEB,
    VTSS_HTTPS_CERT_DOWNLOAD_URL,
    VTSS_HTTPS_CERT_DOWNLOAD_END
} https_cert_upload_method_t;

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/
/* Redirect current connetion to HTTPS/ HTTP connection.
 *
 * Redirect the current page by changing the value of 'top.location.href'
 * For example: top.location.href = "https://10.9.52.130/"
 */
static BOOL redirect_current_connetion(CYG_HTTPD_STATE *p, BOOL redirect_to_https)
{
    const char  *buf_start = "<html><head><script type=\"text/javascript\">function redirect(){var starttime= new Date().getTime();do{}while((new Date().getTime()-starttime)<3000);top.location.href = \"";
    const char  *buf_1 = "\";}</script></head><body onload=\"redirect();\"><p> The ";
    const char  *buf_2 = " mode is disabled on this device. Redirect to ";
    const char  *buf_end = " connection...</p></body></html>";
    const char  *search_str = redirect_to_https ? "http://" : "https://";
    char        *host_start = NULL, *host_end = NULL;
    char        http_referer_host[40]; // 40 bytes for saving the ip v4/v6 address
    int         ct;

    if (!p->http_referer) {
        return FALSE;
    }

    // Grep the host string from the HTTP referer in URL (http_referer). Example - http://10.9.52.130/https_config.htm
    memset(http_referer_host, 0, sizeof(http_referer_host));
    if (p->http_referer && (host_start = strstr(p->http_referer, search_str)) != NULL) {
        host_start += strlen(search_str);
        if ((host_end = strchr(host_start, '/')) != NULL) {
            strncpy(http_referer_host, host_start, host_end - host_start);
        } else {
            strcpy(http_referer_host, host_start);
        }
    }

    if (!strlen(http_referer_host)) {
        return FALSE;
    }

    (void)cyg_httpd_start_chunked("html");
    ct = sprintf(httpstate.outbuffer, "%s%s%s%s%s%s%s%s",
                 buf_start,
                 redirect_to_https ? "https://" : "http://",
                 http_referer_host,
                 buf_1,
                 redirect_to_https ? "HTTP" : "HTTPS",
                 buf_2,
                 redirect_to_https ? "HTTPS" : "HTTP",
                 buf_end);
    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    cyg_httpd_end_chunked();

    return TRUE;
}

/*lint -esym(459, upload_result) */
static i32 handler_config_https(CYG_HTTPD_STATE *p)
{
    int                     ct;
    https_conf_t            *https_conf, *newconf;
    int                     cert_command = VTSS_HTTPS_CERT_COMMAND_NONE, cnt, upload_method = -1;
    char                    url[257] = {0}, passphrase[HTTPS_MGMT_MAX_PASS_PHRASE_LEN + 1];
    size_t                  buffer_len = 0;
    form_data_t             formdata[8];
    unsigned char           *buffer = NULL;
    static mesa_rc          upload_result = VTSS_RC_OK;
    BOOL                    is_https_conn = FALSE;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY)) {
        return -1;
    }
#endif

    if ((VTSS_MALLOC_CAST(https_conf, sizeof(https_conf_t))) == NULL) {
        return -1;
    }
    if ((VTSS_MALLOC_CAST(newconf, sizeof(https_conf_t))) == NULL) {
        VTSS_FREE(https_conf);
        return -1;
    }


    //Format: [https_mode]/[https_redirect]/[cert_maintain]/[cert_gen_algorithm]/[cert_passphrase]/
    //          [cert_upload_method]/[cert_url_upload]/[cert_status]/[cert_status_string]
    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* store form data */
        if (https_mgmt_cert_status() != HTTPS_CERT_IS_GENERATING && https_mgmt_conf_get(https_conf) == VTSS_RC_OK) {
            *newconf = *https_conf;
            sprintf(passphrase, "%s", "");
            if ((cnt = cyg_httpd_form_parse_formdata(p, formdata, ARRSZ(formdata))) > 0) {
                int i;
                for (i = 0; i < cnt; i++) {

                    if (!strcmp(formdata[i].name, "https_mode")) {
                        newconf->mode = atoi(formdata[i].value);
#if HTTPS_MGMT_SUPPORTED_REDIRECT
                    } else if (!strcmp(formdata[i].name, "https_redirect")) {
                        newconf->redirect = atoi(formdata[i].value);
#endif /* HTTPS_MGMT_SUPPORTED_REDIRECT */
                    } else if (!strcmp(formdata[i].name, "cert_maintain")) {
                        cert_command = atoi(formdata[i].value);
                    } else if (!strcmp(formdata[i].name, "cert_passphrase")) {
                        if (formdata[i].value_len > 0) {
                            (void) cgi_unescape(formdata[i].value, passphrase, formdata[i].value_len, sizeof(passphrase));
                        } else {
                            sprintf(passphrase, "%s", "");
                        }
                    } else if (!strcmp(formdata[i].name, "cert_upload_method")) {
                        upload_method = atoi(formdata[i].value);
                    } else if (!strcmp(formdata[i].name, "cert_url_upload")) {
                        if (formdata[i].value_len > 0) {
                            (void) cgi_unescape(formdata[i].value, url, formdata[i].value_len, sizeof(url));
                        }
                        strncpy(url, formdata[i].value, formdata[i].value_len);
                    } else if (!strcmp(formdata[i].name, "cert_data")) {
                        if (buffer) {
                            VTSS_FREE(buffer);
                            buffer = nullptr;
                        }

                        if ((VTSS_MALLOC_CAST(buffer, formdata[i].value_len))) {
                            memcpy(buffer, formdata[i].value, formdata[i].value_len);
                            buffer_len = formdata[i].value_len;
                        } else {
                            const char *err = "Allocation of firmware buffer failed";
                            T_E("%s (len %zu)", err, formdata[i].value_len);
                        }

                    }
                }

                T_D("cert_command = %d, passphrase = %s, upload_method = %d, url = %s", cert_command,
                    passphrase, upload_method, url);

                switch (cert_command ) {
                case VTSS_HTTPS_CERT_COMMAND_DELETE:
                    T_D("VTSS_HTTPS_CERT_COMMAND_DELETE");
                    if (HTTPS_MGMT_DISABLED == newconf->mode && newconf->mode != https_conf->mode) {
                        if (https_mgmt_conf_set(newconf) < 0) {
                            T_E("https_mgmt_conf_set(): failed");
                        }
                    }
                    (void) https_mgmt_cert_del();
                    break;
                case VTSS_HTTPS_CERT_COMMAND_UPLOAD:
                    strcpy(newconf->server_pass_phrase, passphrase);
                    if (HTTPS_MGMT_DISABLED == newconf->mode && newconf->mode != https_conf->mode) {
                        if (https_mgmt_conf_set(newconf) < 0) {
                            T_E("https_mgmt_conf_set(): failed");
                        }
                    }
                    if (upload_method < 0) {
                        T_W("BUG");
                    }
                    if (!upload_method) {
                        // WEB browser upload
                        T_D("WEB browser upload, buffer = %p, len = %zd", buffer, buffer_len);
                        if (buffer_len > sizeof(newconf->server_cert)) {
                            upload_result = HTTPS_ERROR_CERT_TOO_BIG;
                            T_D("HTTPS_ERROR_CERT_TOO_BIG");
                            goto redirect;
                        }
                        if (buffer) {
                            memset(newconf->server_cert, 0, sizeof(newconf->server_cert));
                            memcpy(newconf->server_cert, buffer, buffer_len);
                            memset(newconf->server_pkey, 0, sizeof(newconf->server_pkey));
                            newconf->self_signed_cert = FALSE;
                            if (newconf->server_cert[0] == '\0' ||
                                https_mgmt_cert_key_is_valid(newconf->server_cert, NULL, newconf->server_pass_phrase) == FALSE) {
                                upload_result = HTTPS_ERROR_INV_CERT;
                                T_D("invalid cetification");
                            } else {
                                upload_result = https_mgmt_cert_update(newconf);
                            }
                        }
                    } else {
                        // URL upload
                        T_D("URL upload");
                        upload_result =  https_mgmt_cert_upload(url, newconf->server_pass_phrase);
                    }
                    break;
                case VTSS_HTTPS_CERT_COMMAND_GENERATE:
                    if (HTTPS_MGMT_DISABLED == newconf->mode && newconf->mode != https_conf->mode) {
                        if (https_mgmt_conf_set(newconf) < 0) {
                            T_E("https_mgmt_conf_set(): failed");
                        }
                    }

                    T_D("VTSS_HTTPS_CERT_COMMAND_GENERATE");
                    if (https_mgmt_cert_gen() != VTSS_RC_OK) {
                        T_E("https_mgmt_cert_gen(): failed");
                    }
                    break;
                case VTSS_HTTPS_CERT_COMMAND_NONE:
                    T_D("VTSS_HTTPS_CERT_COMMAND_NONE");
                    if (memcmp(newconf, https_conf, sizeof(*newconf)) != 0) {
                        T_D("Calling https_mgmt_conf_set()");
                        if (https_mgmt_conf_set(newconf) < 0) {
                            T_E("https_mgmt_conf_set(): failed");
                        }
                    }

                    // Changing to new HTTP/HTTPS connection when
                    // 1. The current connection is HTTP but redirect mode is enabled
                    // 2. The current connection is HTTPS but HTTPS mode is disabled
                    is_https_conn = !(!p->http_referer) && ((strstr(p->http_referer, "https://")) != NULL);
                    if (((!is_https_conn && newconf->mode && newconf->redirect) || (is_https_conn && !newconf->mode)) &&
                        redirect_current_connetion(p, !is_https_conn)) {
                        T_D("redirect current connection fail is_https = %d", is_https_conn);
                        VTSS_FREE(newconf);
                        VTSS_FREE(https_conf);
                        if (buffer) {
                            VTSS_FREE(buffer);
                        }
                        return -1;
                    }
                    break;
                default:
                    T_D("VTSS_HTTPS_CERT_COMMAND_NONE");
                    break;
                }

            } else {
                cyg_httpd_send_error(CYG_HTTPD_STATUS_BAD_REQUEST);
            }

        }
redirect:
        T_D("redirect");
        redirect(p, "/https_config.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: [https_mode]/[https_redirect]/[cert_maintain]/
                   [cert_upload_method]/[cert_status]/[cert_status_string]/
                   [upload_result]/[upload_result_string]
        */
        if (https_mgmt_conf_get(https_conf) == VTSS_RC_OK) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%d/%d/%d/%d/%d/%s/%d/%s",
                          https_conf->mode,
                          https_conf->redirect, 0, 0, 0, https_mgmt_cert_status(),
                          https_cert_status_txt(https_mgmt_cert_status()),
                          upload_result, https_error_txt((https_error_t)upload_result));
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        (void)cyg_httpd_end_chunked();
        upload_result = VTSS_RC_OK;
    }

    VTSS_FREE(newconf);
    VTSS_FREE(https_conf);
    if (buffer) {
        VTSS_FREE(buffer);
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_https, "/config/https", handler_config_https);

