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

#include "web_api.h"
#include "icfg.h"
#include "icfg_api.h"
#if defined(CYGPKG_FS_RAM)
#include "os_file_api.h"
#endif  /* CYGPKG_FS_RAM */
#include <dirent.h>
#include <unistd.h>

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_ICFG

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#include "control_api.h"
#undef VTSS_TRACE_MODULE_ID
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */


//*****************************************************************************
// Helper functions
//*****************************************************************************

static void icfg_write_http_h1(const char *msg)
{
    (void)cyg_httpd_write_chunked("<h1>", 4);
    (void)cyg_httpd_write_chunked(msg, strlen(msg));
    (void)cyg_httpd_write_chunked("</h1>\n<p></p>\n", 14);
}

static void icfg_load_defaults(void)
{
    T_D("Requesting reload of defaults without application of default-config");
    control_config_reset(VTSS_USID_ALL, INIT_CMD_PARM2_FLAGS_NO_DEFAULT_CONFIG);
}

static void icfg_send_error(CYG_HTTPD_STATE *httpd_state, const char *txt1, const char *txt2)
{
    char buf[1024];
    (void)snprintf(buf, sizeof(buf) - 1, "%s%s", txt1, txt2);
    buf[sizeof(buf) - 1] = 0;
    T_D("%s", buf);
    send_custom_error(httpd_state, buf, " ", 1);
}

//=============================================================================
// Download: Send file to browser.
//=============================================================================

static i32 icfg_handler_conf_download(CYG_HTTPD_STATE *httpd_state)
{
    vtss_icfg_query_result_buf_t *buf;
    vtss_icfg_query_result_t     res      = { NULL, NULL };
    char                         *tmp_buf = NULL;
    u32                          total    = 0;
    char                         filename[PATH_MAX];
    size_t                       var_len;
    const char                   *fn_p;

    T_D("entry");

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(httpd_state, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC)) {
        goto early_out;
    }
#endif
    if (httpd_state->method != CYG_HTTPD_METHOD_POST) {
        T_D("Not a POST");
        (void)cyg_httpd_start_chunked("html");
        (void)cyg_httpd_end_chunked();
        goto early_out;
    }

    fn_p = cyg_httpd_form_varable_string(httpd_state, "file_name", &var_len);
    if (var_len == 0) {
        icfg_send_error(httpd_state, "Missing file name parameter", "");
        goto early_out;
    }
    (void) cgi_unescape(fn_p, filename, var_len, sizeof(filename));

    if (!icfg_try_lock_io_mutex()) {
        icfg_send_error(httpd_state, "Another configuration I/O operation is in progress.\nPlease try again in a moment.", "");
        goto early_out;
    }

    if (!strcmp(filename, "running-config")) {
        T_D("Building configuration...");
        if (vtss_icfg_query_all(FALSE, &res) != VTSS_RC_OK) {
            icfg_send_error(httpd_state, "Failed to get switch configuration", "");
            goto out;
        }

        // The web save function expects a contiguous buffer, so we may have to
        // create one and copy all the blocks into it. We do try to avoid it,
        // though.

        buf = res.head;
        if (buf->next  &&  buf->next->used > 0) {
            char *p;
            while (buf != NULL  &&  buf->used > 0) {
                total += buf->used;
                buf = buf->next;
            }
            tmp_buf = (char *) VTSS_MALLOC(total);
            if (!tmp_buf) {
                icfg_send_error(httpd_state, "Failed to get switch configuration; insufficient memory", "");
                goto out;
            }
            buf = res.head;
            p = tmp_buf;
            while (buf != NULL  &&  buf->used > 0) {
                memcpy(p, buf->text, buf->used);
                p += buf->used;
                buf = buf->next;
            }
        } else {
            total = buf->used;
        }
    } else {
        // Normal file
        const char *msg = icfg_file_read(filename, &res);
        if (msg) {
            icfg_send_error(httpd_state, "Failed to read file: ", msg);
            goto out;
        }
        total = res.head->used;
    }

    buf = res.head;

    T_D("Setting download filename to '%s', size = %u bytes", filename, total);

    // Send configuration data to the browser as a file.
    cyg_httpd_ires_table_entry entry;
    entry.f_pname = filename;
    entry.f_ptr   = (unsigned char *)(tmp_buf ? tmp_buf : buf->text);
    entry.f_size  = total;
    cyg_httpd_send_content_disposition(&entry);

out:
    icfg_unlock_io_mutex();

early_out:
    if (tmp_buf) {
        VTSS_FREE(tmp_buf);
    }
    vtss_icfg_free_query_result(&res);

    return -1; // Do not further search the file system.
}



//=============================================================================
// Upload: Receive file from browser.
//=============================================================================

static i32 icfg_handler_conf_upload(CYG_HTTPD_STATE *httpd_state)
{
    form_data_t              formdata[3];
    vtss_icfg_query_result_t res               = { NULL, NULL };
    char                     filename[PATH_MAX];
    form_data_t              *http_file        = NULL;
    BOOL                     is_running_config = FALSE;
    BOOL                     merge             = FALSE;
    int                      i, cnt;
    size_t                   len;

    T_D("entry");

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(httpd_state, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC)) {
        goto early_out;
    }
#endif

    if (httpd_state->method != CYG_HTTPD_METHOD_POST) {
        T_D("Not a POST");
        (void)cyg_httpd_start_chunked("html");
        (void)cyg_httpd_end_chunked();
        goto early_out;
    }

    if ((cnt = cyg_httpd_form_parse_formdata(httpd_state, formdata, ARRSZ(formdata))) <= 0) {
        icfg_send_error(httpd_state, "Missing parameters", "");
        goto early_out;
    }

    filename[0] = 0;

    for (i = 0; i < cnt; i++) {
        if (!strcmp(formdata[i].name, "file_name")) {
            len = MIN(ARRSZ(filename) - 1, formdata[i].value_len);
            strncpy(filename, formdata[i].value, len);
            filename[len] = 0;
        } else if (!strcmp(formdata[i].name, "merge")) {
            merge = !strncmp(formdata[i].value, "true", formdata[i].value_len);
        } else if (!strcmp(formdata[i].name, "source_file")) {
            http_file = &formdata[i];
        } else {
            T_D("Unexpected parameter (%d received): %s", cnt, formdata[i].name);
        }
    }

    if (!filename[0]) {
        icfg_send_error(httpd_state, "Missing file name parameter", "");
        goto early_out;
    }

    if (!http_file) {
        icfg_send_error(httpd_state, "Missing source file parameter", "");
        goto early_out;
    }

    is_running_config = !strcmp(filename, "running-config");

    if (http_file->value_len <= 0) {
        // No configuration is received.
        redirect(httpd_state, "/icfg_invalid.htm");
        goto early_out;
    }

    // FIXME: I/O mutex

    T_D("Uploading " VPRIz" bytes to %s, mode %s", http_file->value_len, filename, merge ? "merge" : "replace");

    if (is_running_config) {
        // We need to alloc our own buffer that we can hand off to the config
        // commit thread since the http request storage is released when we exit
        // this function.
        if (vtss_icfg_init_query_result(http_file->value_len, &res) != VTSS_RC_OK) {
            icfg_send_error(httpd_state, "Allocation of configuration buffer failed", "");
            goto out;
        }
        memcpy(res.head->text, (char *)http_file->value, http_file->value_len);
        res.head->used = http_file->value_len;

        // Redirect away from this page and validate the file. Then, if we're
        // replacing the configuration, take a short break so we don't get
        // triggered again by a repeated POST -- loading defaults might cut
        // connectivity to the browser before an ACK for the POST made it back.

        redirect(httpd_state, "/icfg_conf_running.htm");

        if (!vtss_icfg_commit(ICLI_SESSION_ID_NONE, "Uploaded File", TRUE, TRUE, &res)) {
            goto out;
        }

        VTSS_OS_MSLEEP(1 * 1000);     // Give redirect a chance to get across.

        if (!merge) {
            icfg_load_defaults();
        }

        (void) icfg_commit_trigger("Uploaded File", &res);
    } else {
        if (vtss_icfg_overlay_query_result((char *)http_file->value, http_file->value_len, &res) != VTSS_RC_OK) {
            icfg_send_error(httpd_state, "Allocation of configuration buffer failed", "");
            goto out;
        }
        const char *msg = icfg_file_write(filename, &res);
        if (msg) {
            icfg_send_error(httpd_state, "Upload failed: ", msg);
            goto out;
        }

        const char *status = "Upload successfully completed.";
        (void)cyg_httpd_start_chunked("html");
        (void)cyg_httpd_write_chunked(status, strlen(status));
        (void)cyg_httpd_end_chunked();
    }

out:

early_out:
    vtss_icfg_free_query_result(&res);
    return -1; // Do not further search the file system.
}



//=============================================================================
// Activate: Read file from flash FS and apply it
//=============================================================================

static i32 icfg_handler_conf_activate(CYG_HTTPD_STATE *httpd_state)
{
    char                     filename[PATH_MAX];
    size_t                   var_len;
    const char               *fn_p;
    vtss_icfg_query_result_t res = { NULL, NULL };

    T_D("entry");

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(httpd_state, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC)) {
        goto out;
    }
#endif
    if (httpd_state->method == CYG_HTTPD_METHOD_POST) {
        fn_p = cyg_httpd_form_varable_string(httpd_state, "file_name", &var_len);
        if (var_len == 0) {
            icfg_send_error(httpd_state, "Missing file name parameter", "");
            goto out;
        }
        (void) cgi_unescape(fn_p, filename, var_len, sizeof(filename));

        T_D("Activation request: %s", filename);

        if (!strcmp(filename, "running-config")) {
            icfg_send_error(httpd_state, "Cannot activate: running-config is always active.", "");
            goto out;
        }

        // FIXME: I/O mutex

        const char *msg = icfg_file_read(filename, &res);
        if (msg) {
            icfg_send_error(httpd_state, "Cannot read file: ", msg);
            goto out;
        }

        redirect(httpd_state, "/icfg_conf_running.htm");

        if (!vtss_icfg_commit(ICLI_SESSION_ID_NONE, filename, TRUE, TRUE, &res)) {
            goto out;
        }

        VTSS_OS_MSLEEP(1 * 1000);     // Give redirect a chance to get across.
        icfg_load_defaults();

        (void) icfg_commit_trigger(filename, &res);
    } else {  // GET
        // Format: PROGRESS\nLine1\nLine2\n...
        // PROGRESS is one of: IDLE RUN DONE SYNERR ERR
        //      IDLE   = nothing happened yet
        //      RUN    = activation is running
        //      DONE   = activation successfully completed
        //      ERR    = activation completed with errors
        //      SYNERR = syntax errors during validation; activation not attempted
        const char               *status;
        const char               *output;
        u32                      output_length;
        icfg_commit_state_t      state;
        u32                      error_cnt;
        static const char *const state_to_str[] = { "IDLE\n", "RUN\n", "DONE\n", "SYNERR\n", "ERR\n" };

        icfg_commit_status_get(&state, &error_cnt);
        status = state_to_str[state];
        output = icfg_commit_output_buffer_get(&output_length);
        if (output_length == 0) {
            output        = "(No output was generated.)";
            output_length = strlen(output);
        }

        (void)cyg_httpd_start_chunked("html");
        (void)cyg_httpd_write_chunked(status, strlen(status));
        (void)cyg_httpd_write_chunked(output, output_length);
        (void)cyg_httpd_end_chunked();
    }

out:
    vtss_icfg_free_query_result(&res);
    return -1; // Do not further search the file system.
}



//=============================================================================
// Save: "copy running-config startup-config"
//=============================================================================

static i32 icfg_handler_conf_save(CYG_HTTPD_STATE *httpd_state)
{
    const char *res;
    const char *msg;

    T_D("entry");

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(httpd_state, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC)) {
        goto out;
    }
#endif
    if (httpd_state->method != CYG_HTTPD_METHOD_POST) {
        T_D("Not a POST");
        (void)cyg_httpd_start_chunked("html");
        (void)cyg_httpd_end_chunked();
        goto out;
    }

    res = vtss_icfg_running_config_save();

    if (res) {
        icfg_send_error(httpd_state, "Cannot save startup-config: ", res);
        goto out;
    }

    msg = "startup-config saved successfully.";
    (void)cyg_httpd_start_chunked("html");
    icfg_write_http_h1("Save Running Configuration to startup-config");
    (void)cyg_httpd_write_chunked(msg, strlen(msg));
    (void)cyg_httpd_end_chunked();

out:
    return -1; // Do not further search the file system.
}



//=============================================================================
// Delete file from flash FS
//=============================================================================

static i32 icfg_handler_conf_delete(CYG_HTTPD_STATE *httpd_state)
{
    char                         filename[PATH_MAX];
    size_t                       var_len;
    const char                   *fn_p;
    const char                   *msg;

    T_D("entry");

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(httpd_state, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC)) {
        T_D("bad privilege");
        goto early_out;
    }
#endif

    if (httpd_state->method != CYG_HTTPD_METHOD_POST) {
        T_D("Not a POST");
        (void)cyg_httpd_start_chunked("html");
        (void)cyg_httpd_end_chunked();
        goto early_out;
    }

    fn_p = cyg_httpd_form_varable_string(httpd_state, "file_name", &var_len);
    if (var_len == 0) {
        icfg_send_error(httpd_state, "Missing file name parameter", "");
        goto early_out;
    }
    (void) cgi_unescape(fn_p, filename, var_len, sizeof(filename));

    if (!icfg_try_lock_io_mutex()) {
        icfg_send_error(httpd_state, "Another configuration I/O operation is in progress.\nPlease try again in a moment.", "");
        goto early_out;
    }

    if ((msg = icfg_file_delete(filename)) != NULL) {
        icfg_send_error(httpd_state, "Cannot delete file: ", msg);
        goto out;
    }

    msg = " successfully deleted.";
    (void)cyg_httpd_start_chunked("html");
    icfg_write_http_h1("Delete Configuration File");
    (void)cyg_httpd_write_chunked(filename, strlen(filename));
    (void)cyg_httpd_write_chunked(msg, strlen(msg));
    (void)cyg_httpd_end_chunked();

out:
    icfg_unlock_io_mutex();
early_out:
    return -1; // Do not further search the file system.
}



//=============================================================================
// Get list of files to present to user
//=============================================================================

static i32 icfg_handler_conf_get_file_list(CYG_HTTPD_STATE *httpd_state)
{
    DIR        *dirp;
    int        err;
    int        cnt;
    char       *encoded_string = NULL;
    char       name[PATH_MAX + 1];     // + 1: Room for '*'
    const char *op;
    size_t     op_len;
    BOOL       inc_running_config;
    BOOL       inc_default_config;

    T_D("entry");
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(httpd_state, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC)) {
        T_D("bad privilege");
        goto early_out;
    }
#endif

    if (httpd_state->method != CYG_HTTPD_METHOD_GET) {
        T_D("Not a GET");
        goto early_out;
    }

    VTSS_MALLOC_CAST(encoded_string, 3 * PATH_MAX);
    if (!encoded_string) {
        T_D("out of mem");
        goto early_out;
    }

    if (!icfg_try_lock_io_mutex()) {
        T_D("I/O mutex is locked");
        cnt = cgi_escape("ERR_LOCK*", encoded_string);
        (void)cyg_httpd_start_chunked("html");
        (void)cyg_httpd_write_chunked(encoded_string, cnt);
        (void)cyg_httpd_end_chunked();
        goto early_out;
    }

    (void)cyg_httpd_start_chunked("html");

    /* op = "upload", "download", "delete", "activate".
     *
     * 'running-config' is included for upload and download; it cannot be
     * deleted nor activated.
     *
     * 'default-config' can be downloaded and activated, but since it's read-
     * only, it cannot be written nor deleted.
     *
     * Output format: One line of text containing filenames. Each filename is
     * terminated by one '*' character. Empty file list contains no '*'.
     */

    op = cyg_httpd_form_varable_string(httpd_state, "op", &op_len);
    if (op_len == 0) {
        T_D("Missing operation parameter");
        cnt = cgi_escape("ERR*", encoded_string);
        (void)cyg_httpd_write_chunked(encoded_string, cnt);
        goto out;
    }

    inc_running_config = (!strncmp(op, "upload",   op_len) && op_len == 6)  ||
                         (!strncmp(op, "download", op_len) && op_len == 8);

    inc_default_config = (!strncmp(op, "activate", op_len) && op_len == 8)  ||
                         (!strncmp(op, "download", op_len) && op_len == 8);

    cnt = cgi_escape("OK*", encoded_string);
    (void)cyg_httpd_write_chunked(encoded_string, cnt);

    if (inc_running_config) {
        cnt = cgi_escape("running-config*", encoded_string);
        (void)cyg_httpd_write_chunked(encoded_string, cnt);
    }

    // Add the other files from the FS.

    dirp = opendir(VTSS_ICFG_PATH);
    if (dirp == NULL) {
        T_E("Cannot list directory: %s\n", strerror(errno));
        goto out;
    }

    for (;;) {
        struct dirent *entry = readdir(dirp);

        if (entry == NULL) {
            break;
        }

        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") || !strcmp(entry->d_name, "crashfile")) {
            continue;
        }

        if (!inc_default_config  &&  !strcmp(entry->d_name, "default-config")) {
            continue;
        }

        name[0] = 0;
        strncat(name, entry->d_name, sizeof(name) - 2);
        strcat(name, "*");
        cnt = cgi_escape(name, encoded_string);
        (void)cyg_httpd_write_chunked(encoded_string, cnt);
    }

    err = closedir(dirp);
    if (err < 0) {
        T_D("closedir: %s\n", strerror(errno));
    }

out:
    (void)cyg_httpd_end_chunked();
    icfg_unlock_io_mutex();
early_out:
    if (encoded_string) {
        VTSS_FREE(encoded_string);
    }
    return -1; // Do not further search the file system.
}


/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t icfg_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[80];
    (void) snprintf(buff, sizeof(buff), "var configIcfgRwFilesMax = %d;\n", ICFG_MAX_WRITABLE_FILES_IN_FLASH_CNT);
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/

web_lib_config_js_tab_entry(icfg_lib_config_js);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_icfg_conf_download,      "/config/icfg_conf_download",      icfg_handler_conf_download);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_icfg_conf_upload,        "/config/icfg_conf_upload",        icfg_handler_conf_upload);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_icfg_conf_activate,      "/config/icfg_conf_activate",      icfg_handler_conf_activate);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_icfg_conf_save,          "/config/icfg_conf_save",          icfg_handler_conf_save);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_icfg_conf_delete,        "/config/icfg_conf_delete",        icfg_handler_conf_delete);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_icfg_conf_get_file_list, "/config/icfg_conf_get_file_list", icfg_handler_conf_get_file_list);

