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

#ifndef _VTSS_WEB_API_LINUX_H_
#define _VTSS_WEB_API_LINUX_H_

#ifdef VTSS_SW_OPTION_FAST_CGI
#include "fast_cgi_api.hxx"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct web_chunk_entry {
    size_t (*module_fun)(char **, char **, size_t *);
    struct web_chunk_entry *next;
};

#define lib_config_js_t web_chunk_entry
extern struct lib_config_js_t *jsconfig_root;

#define web_lib_config_js_tab_entry(_mf_)                               \
    struct lib_config_js_t _jsentry_##_mf_ = { _mf_ };                  \
    void  __attribute__ ((constructor)) _mf_ ## _jstab() {              \
        _jsentry_##_mf_.next = jsconfig_root;                           \
        jsconfig_root = &_jsentry_##_mf_;                               \
    }

#define lib_filter_css_t web_chunk_entry
extern struct lib_filter_css_t *css_config_root;

#define web_lib_filter_css_tab_entry(_mf_)                              \
    struct lib_filter_css_t _cssentry_##_mf_ = { _mf_ };                \
    void  __attribute__ ((constructor)) _mf_ ## _csstab() {             \
        _cssentry_##_mf_.next = css_config_root;                        \
        css_config_root = &_cssentry_##_mf_;                            \
    }

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_WEB_API_LINUX_H_ */

