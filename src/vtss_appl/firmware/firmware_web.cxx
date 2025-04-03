/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <vtss/basics/string-utils.hxx>

#include "main.h"
#include "web_api.h"
#include "firmware_api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif
#include "flash_mgmt_api.h"
#include "firmware.h"
/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_FIRMWARE
#include <vtss_trace_api.h>
/* ============== */

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_FIRMWARE

/*
 * Decoded firmware chunk information from HTTP request URL arguments.
 *
 * The arguments look like this:
 * resumableChunkNumber=1&resumableChunkSize=1048576&resumableCurrentChunkSize=343712&resumableTotalSize=343712&resumableType=application%2Fx-object&resumableIdentifier=343712-access_mgmto&resumableFilename=access_mgmt.o&resumableRelativePath=access_mgmt.o&resumableTotalChunks=1
 */
typedef struct {
    uint32_t chunk_number;
    uint32_t chunk_size;
    uint32_t curr_chunk_size;
    uint32_t total_size;
    uint32_t total_chunks;
    uint32_t session_id;

} firmware_chunk_info_t;

/*
 * Keeps track of received firmware chunk numbers
 */
static vtss::Vector<bool> chunk_status;

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

/*
 * Send a HTTP reply with the current firmware update status
 */
static i32 handler_firmware_status(CYG_HTTPD_STATE* p)
{
    const char *firmware_status = firmware_status_get();
    (void)cyg_httpd_start_chunked("html");
    (void)cyg_httpd_write_chunked(firmware_status, strlen(firmware_status));
    (void)cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

/*
 * Send a HTTP reply with the given HTTP status.
 * We ensure that the 'Content-type' is set to 'html' to avoid Firefox
 * throwing an 'XML Parsing Error' in the browser debug console.
 */
static void firmware_send_http_result(int http_code)
{
    char status[1024];
    (void)cyg_httpd_start_chunked("html");
    sprintf(status, "Status: %d%s", http_code, "\r\n\r\n");
    (void)cyg_httpd_write(status, strlen(status));
    (void)cyg_httpd_end_chunked();
}

/*
 * Map the internal error code to a suitable HTTP error code and send a reply.
 */
static void firmware_send_http_error_for_rc(mesa_rc rc)
{
    switch (rc) {
    case VTSS_RC_OK:
        break;
    case FIRMWARE_ERROR_BUSY:
        firmware_send_http_result(CYG_HTTPD_STATUS_NOT_AUTHORIZED);
        break;
    default:
        firmware_send_http_result(CYG_HTTPD_STATUS_BAD_REQUEST);
        break;
    }
}

/*
 * Parse the arguments for a chunked firmware upload request
 */
static bool parse_firmware_chunk_args(CYG_HTTPD_STATE* p, firmware_chunk_info_t & chunk_info)
{
    vtss_clear(chunk_info);

    auto arglist = vtss::split(vtss::str(p->args, strlen(p->args)), '&');
    for (auto & arg : arglist) {
        auto argparts = vtss::split(arg, '=');
        if (argparts.size() != 2) {
            // ignore non-conforming arguments
            continue;
        }

        auto argname = argparts[0];
        auto argvalue = argparts[1];

        T_R("Got arg: %s = %s", argname, argvalue);

        if (argname == vtss::str("resumableChunkNumber")) {
            chunk_info.chunk_number = atoi(argvalue.begin());
        } else if (argname == vtss::str("resumableChunkSize")) {
            chunk_info.chunk_size = atoi(argvalue.begin());
        } else if (argname == vtss::str("resumableCurrentChunkSize")) {
            chunk_info.curr_chunk_size = atoi(argvalue.begin());
        } else if (argname == vtss::str("resumableTotalSize")) {
            chunk_info.total_size = atoi(argvalue.begin());
        } else if (argname == vtss::str("resumableTotalChunks")) {
            chunk_info.total_chunks = atoi(argvalue.begin());
        } else if (argname == vtss::str("resumableIdentifier")) {
            chunk_info.session_id = atoi(argvalue.begin());
        }
    }

    return true;
}

static i32 handler_firmware_chunk(CYG_HTTPD_STATE* p)
{
    mesa_rc result = VTSS_RC_OK;
    form_data_t formdata[20];
    int cnt;
    char filename[FIRMWARE_IMAGE_NAME_MAX];
    const char *mtd_name = firmware_fis_to_update();
    firmware_chunk_info_t chunk_info;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FIRMWARE))
        return -1;
#endif

    parse_firmware_chunk_args(p, chunk_info);

    T_D("Chunk upload request: chunk_number:%u, chunk_size:%u, curr_chunk_size:%u, total_size:%u, session_id:%u",
        chunk_info.chunk_number, chunk_info.chunk_size, chunk_info.curr_chunk_size, chunk_info.total_size, chunk_info.session_id);

    if (p->method == CYG_HTTPD_METHOD_POST) {
        T_N("Got POST chunk: %s", p->args);
        
        if((cnt = cyg_httpd_form_parse_formdata(p, formdata, ARRSZ(formdata))) > 0) {
            int i;
            form_data_t *firmware = NULL;
            mesa_restart_t restart_type = MESA_RESTART_WARM; // Default
            
            T_N("Parsed POST data, cnt = %d", cnt);

            for (i = 0; i < cnt; i++) {
                if (!strcmp(formdata[i].name, "firmware")) {
                    firmware = &formdata[i];
                    if (cyg_httpd_form_parse_formdata_filename(p, firmware, filename, sizeof(filename)) == 0) {
                        strncpy(filename, "web-unknown", sizeof(filename));
                    }
                }
            }

            if (firmware) {
                T_D("Found firmware part, filename = %s", filename);

                if (firmware_is_nor_only()) {
                    if (firmware->value_len > firmware_section_size(mtd_name)) {
                        const char *err = "The size of the firmware image is too big to fit into the flash";
                        send_custom_error(p, "Firmware Upload Error", err, strlen(err));
                        return -1;
                    }
                }

                if (chunk_info.chunk_number == 1) {
                    result = firmware_update_async_start(chunk_info.session_id,
                                                         chunk_info.total_chunks,
                                                         web_set_cli(web_get_iolayer(WEB_CLI_IO_TYPE_FIRMWARE)),
                                                         filename);
                    if (result != VTSS_RC_OK) {
                        firmware_send_http_error_for_rc(result);
                        return -1;
                    }
                }

                result = firmware_update_async_write(chunk_info.session_id,
                                                     chunk_info.chunk_number,
                                                     (const unsigned char *)firmware->value,
                                                     firmware->value_len);
                if (result != VTSS_RC_OK) {
                    firmware_update_async_abort(chunk_info.session_id);
                    firmware_send_http_error_for_rc(result);
                    return -1;
                } else {
                    firmware_send_http_result(CYG_HTTPD_STATUS_OK);
                }

                if (chunk_info.chunk_number == chunk_info.total_chunks) {
                    result = firmware_update_async_commit(chunk_info.session_id, restart_type);

                    // Update firmware status for web client polls
                    if (result == VTSS_RC_OK || result == FIRMWARE_ERROR_IN_PROGRESS) {
                        firmware_status_set("Flashing, please wait...");
                    } else {
                        firmware_status_str_set(std::string("Error: ") + firmware_error_txt(result));
                    }
                }
            } else {
                const char *err = "Firmware not found in data";
                T_E("%s", err);
                send_custom_error(p, err, err, strlen(err));
            }
        }

    } else if (p->method == CYG_HTTPD_METHOD_GET) {
        T_N("Got GET: %s", p->args);

        // check if we have gotten the indicated chunk and reply OK if so
        uint32_t last_chunk_number = 0;
        result =  firmware_update_async_get_last_chunk(chunk_info.session_id, last_chunk_number);
        if (result != VTSS_RC_OK) {
            T_D("Firmware session %u failure: FW is busy", chunk_info.session_id);
            cyg_httpd_send_error(CYG_HTTPD_STATUS_NOT_AUTHORIZED);
        } else {
            if (chunk_info.chunk_number > last_chunk_number) {
                // Indicate that we do not have the indicated chunk by sending 204 No Content
                // (don't use firmware_send_http_result() as this will actually return content)
                T_D("Firmware chunk %u not received, got %u", chunk_info.chunk_number, last_chunk_number);
                cyg_httpd_send_error(CYG_HTTPD_STATUS_NO_CONTENT);
            } else {
                // Indicate that we do have the indicated chunk by sending 200 OK
                T_D("Firmware chunk %u already received, got %u", chunk_info.chunk_number, last_chunk_number);
                firmware_send_http_result(CYG_HTTPD_STATUS_OK);
            }
        }

    } else {
        T_D("Got other request, %u", p->method);
        firmware_send_http_result(CYG_HTTPD_STATUS_BAD_REQUEST);
    }

    return -1; // Do not further search the file system.
}

static i32 handler_firmware(CYG_HTTPD_STATE* p)
{
    form_data_t formdata[2];
    int cnt;
    char filename[FIRMWARE_IMAGE_NAME_MAX];
    const char *mtd_name = firmware_fis_to_update();

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FIRMWARE))
        return -1;
#endif

    if(p->method == CYG_HTTPD_METHOD_POST) {
        if((cnt = cyg_httpd_form_parse_formdata(p, formdata, ARRSZ(formdata))) > 0) {
            int i;
            form_data_t *firmware = NULL;
            mesa_restart_t restart_type = MESA_RESTART_WARM; // Default
            // Figure out which of the entries found in the POST request contains the firmware
            // and whether the coolstart entry exists (meaning that the checkbox is checked on the page).
            for (i = 0; i < cnt; i++) {
                if (!strcmp(formdata[i].name, "firmware")) {
                    firmware = &formdata[i];
                    if (cyg_httpd_form_parse_formdata_filename(p, firmware, filename, sizeof(filename)) == 0) {
                        strncpy(filename, "web-unknown", sizeof(filename));
                    }
                } else if (!strcmp(formdata[i].name, "coolstart")) {
                    restart_type = MESA_RESTART_COOL;
                }
            }

            if (firmware) {
                if (firmware_is_nor_only()) {
                    if (firmware->value_len > firmware_section_size(mtd_name)) {
                        const char *err = "The size of the firmware image is too big to fit into the flash";
                        send_custom_error(p, "Firmware Upload Error", err, strlen(err));
                        return -1;
                    }
                }
                mesa_rc result = firmware_update_async(web_set_cli(web_get_iolayer(WEB_CLI_IO_TYPE_FIRMWARE)),
                                                       (const unsigned char *)firmware->value,
                                                       // Tell the firmware module to free the POST data buffer
                                                       p->post_data,
                                                       firmware->value_len,
                                                       filename,
                                                       restart_type);
                if (result == VTSS_RC_OK || result == FIRMWARE_ERROR_IN_PROGRESS) {
                    firmware_status_set("Flashing, please wait...");
                    redirect(p, "/upload_flashing.htm");
                } else {
                    const char *err_buf_ptr = error_txt(result);
                    send_custom_error(p, "Firmware Upload Error", err_buf_ptr, strlen(err_buf_ptr));
                }

                 // Inform fast-cgi that p->post_data has been free'd by firmware module.
                p->post_data = nullptr;
            } else {
                const char *err = "Firmware not found in data";
                T_E("%s", err);
                send_custom_error(p, err, err, strlen(err));
            }
        } else {
            cyg_httpd_send_error(CYG_HTTPD_STATUS_BAD_REQUEST);
        }
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        web_send_iolayer_output(WEB_CLI_IO_TYPE_FIRMWARE, web_get_iolayer(WEB_CLI_IO_TYPE_FIRMWARE), "html");
    }

    return -1; // Do not further search the file system.
}

static i32 handler_sw_select(CYG_HTTPD_STATE* p)
{
    int ct;
    const char *pri_name = firmware_fis_prim(), *alt_name = firmware_fis_bak();
    const char *fis_act = NULL, *fis_alt = NULL;
    BOOL have_alt;
    char filename[FIRMWARE_IMAGE_NAME_MAX];
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FIRMWARE))
        return -1;
#endif
    (void)cyg_httpd_start_chunked("html");

    fis_act = firmware_fis_act();
    fis_alt = firmware_fis_to_update();
    have_alt = (pri_name != alt_name);

    if (firmware_image_name_get(fis_act, filename, sizeof(filename)) != VTSS_RC_OK) {
        if (fis_act) {
            strncpy(filename, fis_act, sizeof(filename));
        }
    }
    const char *version_txt = misc_software_version_txt();
    const char *date_txt = misc_software_date_txt();
    /* Current image info */
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                  "%s|%s|%s",
                  filename,
                  version_txt?version_txt:"",
                  date_txt?date_txt:"");
    (void)cyg_httpd_write_chunked(p->outbuffer, ct);

    if (have_alt) {
        vtss_appl_firmware_status_image_t image_entry;

        if (firmware_image_name_get(fis_alt, filename, sizeof(filename)) != VTSS_RC_OK) {
            if (fis_alt) {
                strncpy(filename, fis_alt, sizeof(filename));
            }
        }

        if ( (vtss_appl_firmware_status_image_entry_get(VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE, &image_entry)) != VTSS_RC_OK) {
            T_W("Image entry get FAILED! [%s]", fis_alt ? fis_alt : "(nil)");
        }
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      "^%s|%s|%s",
                      filename,
                      image_entry.version,
                      image_entry.built_date);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    } else {
        T_I("Expect to have alternate fis!");
    }
    (void)cyg_httpd_end_chunked();
    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_firmware_status, "/config/firmware_status", handler_firmware_status);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_firmware, "/config/firmware", handler_firmware);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_firmware_chunk, "/config/firmware/chunkupload", handler_firmware_chunk);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_sw_select, "/config/sw_select", handler_sw_select);
