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

#include <unistd.h>
#include <vtss/basics/notifications/process-cmd.hxx>
#include <vtss/basics/optional.hxx>
#include "vtss_trace.h"
#include "vtss_uboot.hxx"

// For otp debugging:
//const char *otp_file = "/switch/otp";
// For normal usage:
const char *otp_file = "/sys/bus/nvmem/devices/lan9662-otp0/nvmem";
const char *otp_cmd = "/usr/bin/otp";

bool has_otp()
{
    int res = access(otp_cmd, F_OK);
    return res == 0;
}
bool otp_get_byte(unsigned char *c, const char *otp)
{
    int v;
    int res=sscanf(otp, "%02x", &v);
    *c = v;
    return res == 1;
}

mesa_rc otp_get_field(const char *field_name, int size, unsigned char *field_value)
{
    char cmd[256];
    std::string otp_value;
    int result_size;
    int i;
    if (!has_otp()) return VTSS_RC_ERROR;
    sprintf(cmd, "%s --no-confirm -d %s field get %s", otp_cmd, otp_file, field_name);
    vtss::notifications::process_cmd(cmd, &otp_value, nullptr, true);
    const char *p_otp_value = otp_value.c_str();
    if (otp_value.size() == 0 || sscanf(p_otp_value, "%d ", &result_size) != 1 || result_size >= size) {
        return VTSS_RC_ERROR;
    }
    p_otp_value = strchr(p_otp_value, ' ')+1;
    for (i=0; i<result_size && otp_get_byte(&field_value[i], &p_otp_value[i*2]); ++i) {}
    return VTSS_RC_OK;
}

mesa_rc otp_get_tag(const char *tag_name, int size, char *tag_value)
{
    char cmd[256];
    std::string otp_value;
    int result_size;
    int i;
    if (!has_otp()) return VTSS_RC_ERROR;
    sprintf(cmd, "%s --no-confirm -d %s tag get %s", otp_cmd, otp_file, tag_name);
    vtss::notifications::process_cmd(cmd, &otp_value, nullptr, true);
    const char *p_otp_value = otp_value.c_str();
    if (otp_value.size() ==0 || sscanf(p_otp_value, "%d ", &result_size) != 1 || result_size >= size) {
        return VTSS_RC_ERROR;
    }
    p_otp_value = strchr(p_otp_value, ' ')+1;
    for (i=0; i<result_size && otp_get_byte((unsigned char*)&tag_value[i],
                                            &p_otp_value[i*2]); ++i) {}
    return VTSS_RC_OK;
}

mesa_rc otp_get_mac_address(mesa_mac_t *mac_address)
{
    char tag_value[16];
    if (VTSS_RC_OK == otp_get_tag("ethaddr", sizeof(tag_value), tag_value)) {
        mac_address->addr[0] = tag_value[5];
        mac_address->addr[1] = tag_value[4];
        mac_address->addr[2] = tag_value[3];
        mac_address->addr[3] = tag_value[2];
        mac_address->addr[4] = tag_value[1];
        mac_address->addr[5] = tag_value[0];
        return VTSS_RC_OK;
    }

    return VTSS_RC_ERROR;
}

mesa_rc otp_get_password(size_t password_size, char *password)
{
    return otp_get_tag("password", password_size, password);
}
