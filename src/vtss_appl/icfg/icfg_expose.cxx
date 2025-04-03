/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss/basics/snmp.hxx"
#include "vtss/appl/icfg.h"

vtss_enum_descriptor_t icfg_reload_default_txt[] {
    {VTSS_APPL_ICFG_RELOAD_DEFAULT_NONE,    "none"},
    {VTSS_APPL_ICFG_RELOAD_DEFAULT,         "default"},
    {VTSS_APPL_ICFG_RELOAD_DEFAULT_KEEP_IP, "defaultKeepIp"},
    {0, 0},
};

vtss_enum_descriptor_t icfg_config_type_txt[] {
    {VTSS_APPL_ICFG_CONFIG_TYPE_NONE,       "none"},
    {VTSS_APPL_ICFG_CONFIG_TYPE_RUNNING,    "runningConfig"},
    {VTSS_APPL_ICFG_CONFIG_TYPE_STARTUP,    "startupConfig"},
    {VTSS_APPL_ICFG_CONFIG_TYPE_FILE,       "configFile"},
    {0, 0},
};

vtss_enum_descriptor_t icfg_config_status_txt[] {
    {VTSS_APPL_ICFG_COPY_STATUS_NONE,                       "none"},
    {VTSS_APPL_ICFG_COPY_STATUS_SUCCESS,                    "success"},
    {VTSS_APPL_ICFG_COPY_STATUS_IN_PROGRESS,                "inProgress"},
    {VTSS_APPL_ICFG_COPY_STATUS_FAILED_OTHER_IN_PROCESSING, "errOtherInProcessing"},
    {VTSS_APPL_ICFG_COPY_STATUS_FAILED_NO_SUCH_FILE,        "errNoSuchFile"},
    {VTSS_APPL_ICFG_COPY_STATUS_FAILED_SAME_SRC_DST,        "errSameSrcDst"},
    {VTSS_APPL_ICFG_COPY_STATUS_FAILED_PERMISSION_DENIED,   "errPermissionDenied"},
    {VTSS_APPL_ICFG_COPY_STATUS_FAILED_LOAD_SRC,            "errLoadSrc"},
    {VTSS_APPL_ICFG_COPY_STATUS_FAILED_SAVE_DST,            "errSaveDst"},
    {0, 0},
};

