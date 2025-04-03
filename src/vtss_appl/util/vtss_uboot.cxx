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

#include "vtss_uboot.hxx"
#include "vtss_cmdline.hxx"
#include <vtss/basics/notifications/process-cmd.hxx>
#include "vtss_trace.h"

static std::string boot_env_cache;
static bool boot_env_cache_valid = false;

static const char *fw_env_nor = "fw_env.config";
static const char *fw_env_mmc = "fw_env_mmc.config";
static const char *fw_env = nullptr;
static char fw_printenv[128] = "";
static char fw_setenv[128] = "";

static void vtss_uboot_env_init()
{

    // Determine whether uboot environment is located in nor or mmc
    const char *boot_source = vtss_cmdline_get("boot_source");
    if (boot_source && strncmp(boot_source, "mmc", strlen("mmc"))==0) {
        // uboot is running from mmc
        fw_env = fw_env_mmc;
    } else {
        fw_env = fw_env_nor;
        // Default, uboot is running from nor
    }
    sprintf(fw_printenv, "/usr/sbin/fw_printenv -l /tmp --config /etc/%s ", fw_env);
    sprintf(fw_setenv, "/usr/sbin/fw_setenv -l /tmp --config /etc/%s ", fw_env);
}

const char *vtss_uboot_env_get()
{
    if (!boot_env_cache_valid) {
        return nullptr;
    }
    return boot_env_cache.c_str();
}

#define UBOOT_MAX_ENV_SIZE 10000
vtss::Optional<std::string> vtss_uboot_get_env(const char *name)
{
    vtss_uboot_env_init();
    T_D("Search for '%s'", name);
    static vtss::Optional<std::string> value;
    value.clear();
    if (!boot_env_cache_valid) {
        vtss::notifications::process_cmd(fw_printenv, &boot_env_cache, nullptr, true, UBOOT_MAX_ENV_SIZE);
        boot_env_cache_valid = true;
        T_D("Bootenv:\n%s", boot_env_cache.c_str());
    }
    if (boot_env_cache.empty()) {
        return value;
    }
    char needle[128];
    sprintf(needle, "%s=", name);
    size_t pos = 0;
    while (pos < boot_env_cache.size()) {
        size_t delimiter = boot_env_cache.find('=', pos);
        size_t found = boot_env_cache.find(needle, pos);
        size_t end = boot_env_cache.find('\n', pos);
        if (end == std::string::npos) {
            end = boot_env_cache.size();
        }
        if (found != std::string::npos && found < delimiter) {
            value = boot_env_cache.substr(delimiter+1, end - delimiter -1);
            return value;
        }
        pos = end +1;
    }
    return value;
}

mesa_rc vtss_uboot_set_env(const char *name, const char *val)
{
    vtss_uboot_env_init();
    int ret = vtss::notifications::process_cmd(
                  std::string(fw_setenv + std::string(name) + " '"
                              + std::string(val) + "'").c_str(),
                  nullptr, nullptr);
    boot_env_cache_valid = false; // A value has been changed, cache no longer valid
    if (ret != 0) {
        T_E("Failed to update %s to %s", name, val);
        return VTSS_RC_ERROR;
    }
    T_D("Updated %s to %s", name, val);
    return VTSS_RC_OK;
}
