/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_WEB_API_H_
#define _VTSS_WEB_API_H_

#include "web_api_linux.h"
#include "main.h" /* For vtss_usid_t */

#ifdef VTSS_SW_OPTION_CLI
#include "cli_io_api.h"
#endif /* VTSS_SW_OPTION_CLI */
#include "misc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_ERR_URL "/stack_error.htm"

#define MAX_FORM_NAME_LEN       32

typedef struct {
    char        name[MAX_FORM_NAME_LEN];
    const char  *content_header;
    const char  *value;
    size_t      value_len;
} form_data_t;

#define HTML_VAR_LEN   16

/* Web dummy CLI IO types */
enum {
    WEB_CLI_IO_TYPE_BOOT_LOADER,
    WEB_CLI_IO_TYPE_FIRMWARE,
    WEB_CLI_IO_TYPE_PING,
    WEB_CLI_IO_MAX = 8
};

vtss_usid_t web_retreive_request_sid(CYG_HTTPD_STATE* p);

/* Initialize module */
mesa_rc web_init(vtss_init_data_t *data);

#ifdef VTSS_SW_OPTION_CLI
/* Pseudo HTTP CLI iolayer */
cli_iolayer_t *web_get_iolayer(int web_io_type);
cli_iolayer_t *web_set_cli(cli_iolayer_t *pIO); // Set pIO->fd to point to an open fifo for writing

/* When web_io_type is equal to WEB_CLI_IO_TYPE_BOOT_LOADER or WEB_CLI_IO_TYPE_FIRMWARE,
   the parameter of "io" should be NULL.
   When web_io_type is equal to WEB_CLI_IO_TYPE_PING,
   the parameter of "io" should be specific memory address.
  */
void web_send_iolayer_output(int web_io_type, cli_iolayer_t *io, const char *mimetype);
#endif /* VTSS_SW_OPTION_CLI */

void stat_portstate_switch_lu26(CYG_HTTPD_STATE* p, vtss_usid_t usid, vtss_isid_t isid);
void stat_portstate_switch_serval(CYG_HTTPD_STATE* p, vtss_usid_t usid, vtss_isid_t isid);
void stat_portstate_switch_jr2(CYG_HTTPD_STATE* p, vtss_usid_t usid, vtss_isid_t isid);
void stat_portstate_switch_sparx5(CYG_HTTPD_STATE* p);
void stat_portstate_switch_lan966x(CYG_HTTPD_STATE* p);

BOOL
redirectUnmanagedOrInvalid(CYG_HTTPD_STATE* p, vtss_isid_t isid);
void
redirect(CYG_HTTPD_STATE* p, const char *to);

/* Append a magic keyword "ResponseErrMsg=" in the HTML URL.
 * It is used to notify the Web page alert the error message.
 *
 * Note1: The total length (*to + *errmsg) should not over 497 bytes.
 * The maximum URL length is refer to CYG_HTTPD_MAXURL (512).
 * It is defined in \eCos\packages\net\athttpd\current\include\http.h
 *
 * Note2: Must adding processResponseErrMsg() in the Web page onload event.
 * For example:
 * <body class="content" onload="processResponseErrMsg(); requestUpdate();">
 */
void
redirect_errmsg(CYG_HTTPD_STATE* p, const char *to, const char *errmsg);

void
send_custom_error(CYG_HTTPD_STATE *p,
                  const char      *title,
                  const char      *errtxt,
                  size_t           errtxt_len);

#define CHECK_FORMAT(__format__, __args__) __attribute__((format (printf, __format__, __args__)))

/* The parameter of 'idx' start from 1, it means the first matched.
   When the value is 2, it means the seconds matched and so on. */
cyg_bool    cyg_httpd_form_multi_varable_int(CYG_HTTPD_STATE* p, const char *name, int *pVal, int idx);
const char *cyg_httpd_form_varable_string(   CYG_HTTPD_STATE* p, const char *name, size_t *pLen);
vtss_isid_t web_retrieve_request_sid(        CYG_HTTPD_STATE* p);
cyg_bool    cyg_httpd_form_varable_int(      CYG_HTTPD_STATE* p, const char *name, int *pVal);
cyg_bool    cyg_httpd_form_variable_u32(     CYG_HTTPD_STATE* p, const char *name, u32 *pVal);
cyg_bool    cyg_httpd_form_varable_uint64(   CYG_HTTPD_STATE* p, const char *name, u64 *pVal);
cyg_bool    cyg_httpd_form_varable_ipv4(     CYG_HTTPD_STATE* p, const char *name, mesa_ipv4_t *pVal);
cyg_bool    cyg_httpd_form_variable_ipv4_fmt(CYG_HTTPD_STATE *p, mesa_ipv4_t *pip, const char *fmt, ...);
cyg_bool    cyg_httpd_form_varable_ipv6(     CYG_HTTPD_STATE* p, const char *name, mesa_ipv6_t *pVal);
cyg_bool    cyg_httpd_form_variable_ipv6_fmt(CYG_HTTPD_STATE *p, mesa_ipv6_t *pip, const char *fmt, ...);
cyg_bool    cyg_httpd_form_varable_ip(       CYG_HTTPD_STATE* p, const char *name, mesa_ip_addr_t *pVal);
cyg_bool    cyg_httpd_form_variable_ip_fmt  (CYG_HTTPD_STATE *p, mesa_ip_addr_t *pip, const char *fmt, ...);

int  cgi_escape(               const char *from, char *to);
int  cgi_escape_n(             const char *from, char *to, size_t maxlen);
BOOL cgi_unescape(             const char *from, char *to, uint from_len, uint to_len);
int  cgi_text_str_to_ascii_str(const char *from, char *to, uint from_len, uint to_len);
BOOL cgi_ascii_str_to_text_str(const char *from, char *to, uint from_len, uint to_len);

/* HACK! (type converting macro - pval pointer) */
#define cyg_httpd_form_varable_long_int(p, name, pval) ({               \
            long _arg;                                                  \
            cyg_bool ret = _cyg_httpd_form_varable_long_int(p, name, &_arg); \
            if(ret) { *(pval) = (u32) _arg; }                           \
            ret;                                                        \
        })

/* Inner function */
cyg_bool   _cyg_httpd_form_varable_long_int(       CYG_HTTPD_STATE* p, const char *name, long *pVal);
const char *cyg_httpd_form_varable_find(           CYG_HTTPD_STATE* p, const char *name);
char *      cyg_httpd_form_varable_strdup(         CYG_HTTPD_STATE* p, const char *fmt, ...) CHECK_FORMAT(2, 3);
cyg_bool    cyg_httpd_form_variable_int_fmt(       CYG_HTTPD_STATE* p, int *pVal, const char *fmt, ...) CHECK_FORMAT(3, 4);
cyg_bool    cyg_httpd_form_variable_u32_fmt(       CYG_HTTPD_STATE* p, u32 *pVal, const char *fmt, ...) CHECK_FORMAT(3, 4);
cyg_bool    cyg_httpd_form_variable_check_fmt(     CYG_HTTPD_STATE* p, const char *fmt, ...) CHECK_FORMAT(2, 3);
const char *cyg_httpd_form_variable_str_fmt(       CYG_HTTPD_STATE* p, size_t *pLen, const char *fmt, ...) CHECK_FORMAT(3, 4);
cyg_bool    cyg_httpd_form_varable_hex(            CYG_HTTPD_STATE* p, const char *name, ulong *pVal);
int         httpd_form_get_value_int(              CYG_HTTPD_STATE* p, const char form_name[255],int min_value,int max_value);
cyg_bool    cyg_httpd_form_varable_mac(            CYG_HTTPD_STATE* p, const char *name, uchar pVal[6]);
int         cyg_httpd_form_parse_formdata(         CYG_HTTPD_STATE* p, form_data_t *formdata, int maxdata);
int         cyg_httpd_form_parse_formdata_filename(CYG_HTTPD_STATE* p, const form_data_t *formdata, char *fn, size_t maxfn);
cyg_bool    cyg_httpd_form_variable_long_int_fmt(  CYG_HTTPD_STATE* p, ulong *pVal, const char *fmt, ...) CHECK_FORMAT(3, 4);
cyg_bool    cyg_httpd_form_variable_mac(           CYG_HTTPD_STATE* p, const char *name, mesa_mac_t *mac);
cyg_bool    cyg_httpd_form_varable_oui(            CYG_HTTPD_STATE* p, const char *name, uchar pVal[3]);
cyg_bool    cyg_httpd_str_to_hex(const char *str_p, ulong *hex_value_p);

char *vtss_get_formdata_boundary(CYG_HTTPD_STATE* p, int *len);

size_t webCommonBufferHandler(char **base, char **offset, size_t *length, const char *buff);
int  web_get_method(CYG_HTTPD_STATE *p, int module_id);
void web_page_disable(const char *const name);
void web_css_filter_add(const char *const name);

void WEB_navbar_menu_item(CYG_HTTPD_STATE* p, int &size, const char *s, int &prev_level);
int  WEB_navbar(CYG_HTTPD_STATE* p, int &size);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_WEB_API_H_ */

