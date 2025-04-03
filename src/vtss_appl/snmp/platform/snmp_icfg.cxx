/*

 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/*
******************************************************************************

    Include files

******************************************************************************
*/
#include "icfg_api.h"
#include "vtss_snmp_api.h"
#include "snmp_icfg.h"
#include "misc_api.h"   /* For uport2iport(), iport2uport() */
#include "vtss_auth_api.h"
/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/

#undef  VTSS_ALLOC_MODULE_ID
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_ICFG

/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/

static void system_default_get(system_conf_t *conf)
{
    conf->sys_contact[0] = 0;
    conf->sys_name[0] = 0;
    conf->sys_location[0] = 0;
}

static BOOL snmpv3_mgmt_default_communities_check (snmpv3_communities_conf_t *conf, snmpv3_communities_conf_t *def_conf, u32 num )
{
    i32 i = 0;
    BOOL found = FALSE;
    snmpv3_communities_conf_t *ptr;

    for (i = 0, ptr = def_conf ; i < (i32) num; i++, ptr++) {
        if ( 0 == snmpv3_communities_conf_changed (ptr, conf) ) {
            found = TRUE;
            break;
        }
    }

    return found;
}

static mesa_rc SNMPV3_ICFG_community_conf(const vtss_icfg_query_request_t *req,
                                          vtss_icfg_query_result_t *result)
{
    mesa_rc             rc = VTSS_RC_OK;
    char                ip_buf[64], mask_buf[16];
    mesa_ipv4_t         sip_mask;
    snmpv3_communities_conf_t conf, *def_conf, *ptr;
    u32        num;
    i32        i = 0;

    snmpv3_default_communities_get( &num, NULL);
    if ((VTSS_MALLOC_CAST(def_conf, sizeof(conf) * num)) == NULL) {
        return VTSS_RC_ERROR;
    }
    snmpv3_default_communities_get ( NULL, def_conf);

    // Fail on IP overlap if create before delete; so delete communities first
    for (i = 0, ptr = def_conf; i < (i32)num && ptr != NULL; i++, ptr++) {
        /* COMMAND = no snmp-server community <word32> */
        if ( VTSS_RC_OK != snmpv3_mgmt_communities_conf_get(ptr, FALSE)) {
            rc = vtss_icfg_printf( result, "no snmp-server community %s\n", ptr->community);
        }
    }
    snmpv3_default_communities_get ( NULL, def_conf);

    strcpy(conf.community, "");
    while ( snmpv3_mgmt_communities_conf_get(&conf, TRUE) == VTSS_RC_OK ) {

        if ( TRUE == snmpv3_mgmt_default_communities_check(&conf, def_conf, num) && !req->all_defaults ) {
            continue;
        }

        /* COMMAND = snmp-server community <word32> [ { ip-range <ipv4_addr> <ipv4_netmask> | ipv6-range <ipv6_subnet> } ] encrypted <word96-160> */
        if (conf.sip.address.type == MESA_IP_TYPE_IPV4) {
            (void)misc_prefix2ipv4(conf.sip.prefix_size, &sip_mask);
            rc = vtss_icfg_printf(result, "%s%s ip-range %s %s encrypted %s\n",
                                  "snmp-server community ",
                                  conf.community, misc_ipv4_txt(conf.sip.address.addr.ipv4, ip_buf), misc_ipv4_txt(sip_mask, mask_buf), conf.encryptedSecret);
#ifdef VTSS_SW_OPTION_IPV6
        } else if (conf.sip.address.type == MESA_IP_TYPE_IPV6) {
            rc = vtss_icfg_printf(result, "%s%s ipv6-range %s/%u encrypted %s\n",
                                  "snmp-server community ",
                                  conf.community, misc_ipv6_txt(&conf.sip.address.addr.ipv6, ip_buf), conf.sip.prefix_size, conf.encryptedSecret);
#endif /* VTSS_SW_OPTION_IPV6 */
        }
    }

    VTSS_FREE(def_conf);

    return rc;
}

static mesa_rc SNMPV3_ICFG_user_conf(const vtss_icfg_query_request_t *req,
                                     vtss_icfg_query_result_t *result)
{
    u32        num;
    const      u32 cmd_size = 256;
    mesa_rc    rc = VTSS_RC_OK;
    char       cmd[cmd_size];
    snmpv3_users_conf_t conf, def_conf;
    char engineid_txt[SNMPV3_MAX_ENGINE_ID_LEN * 2 + 1];

    snmpv3_default_users_get(&num, NULL);
    if (num > 0) {
        snmpv3_default_users_get ( NULL, &def_conf);
    }

    /* only non-authitication users can be shown in running config */
    /* command: snmp-server
        snmp-server user <username:word32> engine-id <engineID:word10-32>
        no snmp-server user <username:word32> engine-id <engineID:word10-32>
       */

    strcpy(conf.user_name, "");
    while ( snmpv3_mgmt_users_conf_get(&conf, TRUE) == VTSS_RC_OK ) {

        if (num > 0) {
            if ((0 == snmpv3_users_conf_changed( &conf, &def_conf) && !req->all_defaults)) {
                continue;
            }
        }


        sprintf( cmd, "snmp-server user %s engine-id %s ", conf.user_name, misc_engineid2str(engineid_txt, conf.engineid, conf.engineid_len));

        if (conf.auth_protocol != SNMP_MGMT_AUTH_PROTO_NONE) {
            if (conf.auth_protocol == SNMP_MGMT_AUTH_PROTO_SHA) {
                strncat(cmd, "sha encrypted ", cmd_size - 1);
            } else {
                strncat(cmd, "md5 encrypted ", cmd_size - 1);
            }
            strncat(cmd, conf.auth_password, cmd_size - 1);
        }

        if (conf.priv_protocol != SNMP_MGMT_PRIV_PROTO_NONE) {
            if (conf.priv_protocol == SNMP_MGMT_PRIV_PROTO_DES) {
                strncat(cmd, " priv des encrypted ", cmd_size - 1);
            } else {
                strncat(cmd, " priv aes encrypted ", cmd_size - 1);
            }
            strncat(cmd, conf.priv_password, cmd_size - 1);
        }

        rc = vtss_icfg_printf(result, "%s\n", cmd);
    }

    if (num > 0) {
        if ( VTSS_RC_OK != snmpv3_mgmt_users_conf_get(&def_conf, FALSE)) {
            sprintf( cmd, "no snmp-server user %s engine-id %s ", def_conf.user_name, misc_engineid2str(engineid_txt, def_conf.engineid, def_conf.engineid_len));
            rc = vtss_icfg_printf(result, "%s\n", cmd);
        }
    }

    return rc;
}

static BOOL snmpv3_mgmt_default_groups_check (snmpv3_groups_conf_t *conf, snmpv3_groups_conf_t *def_conf, u32 num )
{
    i32 i = 0;
    BOOL found = FALSE;
    snmpv3_groups_conf_t *ptr;

    for (i = 0, ptr = def_conf ; i < (i32) num; i++, ptr++) {
        if ( 0 == snmpv3_groups_conf_changed (ptr, conf) ) {
            found = TRUE;
            break;
        }
    }

    return found;
}

static mesa_rc SNMPV3_ICFG_group_conf(const vtss_icfg_query_request_t *req,
                                      vtss_icfg_query_result_t *result)
{
    mesa_rc    rc = VTSS_RC_OK;
    char       cmd[256];
    snmpv3_groups_conf_t conf, *def_conf, *ptr;
    u32        num;
    i32        i = 0;

    snmpv3_default_groups_get( &num, NULL);
    if ((VTSS_MALLOC_CAST(def_conf, sizeof(conf) * num)) == NULL) {
        return VTSS_RC_ERROR;
    }
    snmpv3_default_groups_get ( NULL, def_conf);
    /* command: snmp-server
        snmp-server security-to-group model { v1 | v2c | v3 } name <word32> group <word32>
        no snmp-server security-to-group model { v1 | v2c | v3 } name <word32>
       */

    conf.security_model = SNMP_MGMT_SEC_MODEL_ANY;
    strcpy(conf.security_name, "");
    while ( snmpv3_mgmt_groups_conf_get(&conf, TRUE) == VTSS_RC_OK ) {
        sprintf( cmd, "snmp-server security-to-group model %s name %s group %s",
                 conf.security_model == SNMP_MGMT_SEC_MODEL_SNMPV1 ? "v1" : conf.security_model == SNMP_MGMT_SEC_MODEL_SNMPV2C ? "v2c" : "v3",
                 conf.security_name, conf.group_name);

        if ( TRUE == snmpv3_mgmt_default_groups_check(&conf, def_conf, num) && !req->all_defaults ) {
            continue;
        }

        rc = vtss_icfg_printf(result, "%s\n", cmd);
    }

    for (i = 0, ptr = def_conf; i < (i32)num && ptr != NULL; i++, ptr++) {
        if ( VTSS_RC_OK != snmpv3_mgmt_groups_conf_get(ptr, FALSE)) {
            sprintf( cmd, "no snmp-server security-to-group model %s name %s",
                     ptr->security_model == SNMP_MGMT_SEC_MODEL_SNMPV1 ? "v1" : ptr->security_model == SNMP_MGMT_SEC_MODEL_SNMPV2C ? "v2c" : "v3",
                     ptr->security_name);

            rc = vtss_icfg_printf(result, "%s\n", cmd);
        }
    }

    VTSS_FREE(def_conf);
    return rc;
}

static BOOL snmpv3_mgmt_default_accesses_check (snmpv3_accesses_conf_t *conf, snmpv3_accesses_conf_t *def_conf, u32 num )
{
    i32 i = 0;
    BOOL found = FALSE;
    snmpv3_accesses_conf_t *ptr;

    for (i = 0, ptr = def_conf ; i < (i32) num; i++, ptr++) {
        if ( 0 == snmpv3_accesses_conf_changed (ptr, conf) ) {
            found = TRUE;
            break;
        }
    }

    return found;
}

static BOOL snmpv3_mgmt_default_views_check (snmpv3_views_conf_t *conf, snmpv3_views_conf_t *def_conf, u32 num )
{
    i32 i = 0;
    BOOL found = FALSE;
    snmpv3_views_conf_t *ptr;

    for (i = 0, ptr = def_conf ; i < (i32) num; i++, ptr++) {
        if ( 0 == snmpv3_views_conf_changed (ptr, conf) ) {
            found = TRUE;
            break;
        }
    }

    return found;
}

static mesa_rc SNMPV3_ICFG_access_conf(const vtss_icfg_query_request_t *req,
                                       vtss_icfg_query_result_t *result)
{
    mesa_rc    rc = VTSS_RC_OK;
    char       cmd[256], read_view[64], write_view[64];
    snmpv3_accesses_conf_t conf, *def_conf, *ptr;
    u32        num;
    i32        i = 0;

    snmpv3_default_accesses_get( &num, NULL);
    if ((VTSS_MALLOC_CAST(def_conf, sizeof(conf) * num)) == NULL) {
        return VTSS_RC_ERROR;
    }
    snmpv3_default_accesses_get ( NULL, def_conf);
    /* command: snmp-server
        snmp-server access <word32> model { v1 | v2c | v3 | any } level { auth | noauth | priv } [ read <word32> ] [ write <word32> ]
        no snmp-server access <word32> model { v1 | v2c | v3 } level { auth | noauth | priv }
       */

    conf.security_model = 0;
    strcpy(conf.group_name, "");
    while ( snmpv3_mgmt_accesses_conf_get(&conf, TRUE) == VTSS_RC_OK ) {
        if ( strcmp(conf.read_view_name, SNMPV3_NONAME) ) {
            sprintf( read_view, "read %s", conf.read_view_name);
        } else {
            read_view[0] = 0;
        }

        if ( strcmp(conf.write_view_name, SNMPV3_NONAME) ) {
            sprintf( write_view, "write %s", conf.write_view_name);
        } else {
            write_view[0] = 0;
        }

        sprintf( cmd, "snmp-server access %s model %s level %s %s %s", conf.group_name,
                 conf.security_model == SNMP_MGMT_SEC_MODEL_ANY ? "any" : conf.security_model == SNMP_MGMT_SEC_MODEL_SNMPV1 ? "v1" : conf.security_model == SNMP_MGMT_SEC_MODEL_SNMPV2C ? "v2c" : "v3",
                 conf.security_level == SNMP_MGMT_SEC_LEVEL_AUTHNOPRIV ? "auth" : conf.security_level == SNMP_MGMT_SEC_LEVEL_NOAUTH ? "noauth" : "priv", read_view, write_view);


        if ( TRUE == snmpv3_mgmt_default_accesses_check(&conf, def_conf, num) && !req->all_defaults ) {
            continue;
        }

        rc = vtss_icfg_printf(result, "%s\n", cmd);
    }

    for (i = 0, ptr = def_conf; i < (i32)num && ptr != NULL; i++, ptr++) {
        if ( VTSS_RC_OK != snmpv3_mgmt_accesses_conf_get(ptr, FALSE)) {
            sprintf( cmd, "no snmp-server access %s model %s level %s ", ptr->group_name,
                     ptr->security_model == SNMP_MGMT_SEC_MODEL_ANY ? "any" : ptr->security_model == SNMP_MGMT_SEC_MODEL_SNMPV1 ? "v1" : ptr->security_model == SNMP_MGMT_SEC_MODEL_SNMPV2C ? "v2c" : "v3",
                     ptr->security_level == SNMP_MGMT_SEC_LEVEL_AUTHNOPRIV ? "auth" : ptr->security_level == SNMP_MGMT_SEC_LEVEL_NOAUTH ? "noauth" : "priv");

            rc = vtss_icfg_printf(result, "%s\n", cmd);
        }
    }

    VTSS_FREE(def_conf);
    return rc;
}

static mesa_rc SNMPV3_ICFG_view_conf (const vtss_icfg_query_request_t *req,
                                      vtss_icfg_query_result_t *result)
{
    mesa_rc    rc = VTSS_RC_OK;
    char       cmd[256];
    snmpv3_views_conf_t conf, *def_conf, *ptr;
    u32        num;
    i32        i = 0;

    snmpv3_default_views_get( &num, NULL);
    if ((VTSS_MALLOC_CAST(def_conf, sizeof(conf) * num)) == NULL) {
        return VTSS_RC_ERROR;
    }
    snmpv3_default_views_get ( NULL, def_conf);
    /* command: snmp-server
        snmp-server view <word32> <word255> { include | exclude }
        no snmp-server view <word32> <word255>
       */

    strcpy(conf.view_name, "");
    while ( snmpv3_mgmt_views_conf_get(&conf, TRUE) == VTSS_RC_OK ) {
        sprintf( cmd, "snmp-server view %s %s %s", conf.view_name,
                 conf.subtree,
                 conf.view_type == SNMPV3_MGMT_VIEW_INCLUDED ? "include" : "exclude");

        if ( TRUE == snmpv3_mgmt_default_views_check(&conf, def_conf, num) && !req->all_defaults ) {
            continue;
        }

        rc = vtss_icfg_printf(result, "%s\n", cmd);
    }

    for (i = 0, ptr = def_conf; i < (i32)num && ptr != NULL; i++, ptr++) {
        if ( VTSS_RC_OK != snmpv3_mgmt_views_conf_get(ptr, FALSE)) {
            sprintf( cmd, "no snmp-server view %s %s", ptr->view_name, ptr->subtree);
            rc = vtss_icfg_printf(result, "%s\n", cmd);
        }
    }

    VTSS_FREE(def_conf);
    return rc;

}

/* ICFG callback functions */
static mesa_rc SNMP_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                     vtss_icfg_query_result_t *result)
{
    mesa_rc             rc = VTSS_RC_OK;
    snmp_conf_t         conf, def_conf;
    system_conf_t       sysconf, def_sysconf;
    char engineid_txt[SNMPV3_MAX_ENGINE_ID_LEN * 2 + 1];

    (void) snmp_mgmt_snmp_conf_get(&conf);
    system_default_get(&def_sysconf);

    (void)system_get_config(&sysconf);
    snmp_default_get(&def_conf);
    /* command: snmp-server
                snmp-server engineID local <engineID:word10-32>
                snmp-server contact <line255>
                snmp-server location <line255>
       */
    if (req->all_defaults ||
        (conf.mode != def_conf.mode)) {
        rc = vtss_icfg_printf(result, "%s\n",
                              conf.mode == 1 ? "snmp-server" : "no snmp-server");
    }

    if (req->all_defaults ||
        (conf.engineid_len != def_conf.engineid_len) ||
        memcmp(conf.engineid, def_conf.engineid, def_conf.engineid_len) ) {
        rc = vtss_icfg_printf(result, "%s%s\n",
                              "snmp-server engine-id local ", misc_engineid2str(engineid_txt, conf.engineid, conf.engineid_len));
    }

    if (req->all_defaults ||
        (strcmp(sysconf.sys_contact, def_sysconf.sys_contact))) {
        rc = vtss_icfg_printf(result, "%s%s%s\n",
                              "snmp-server contact ", req->all_defaults ? "no" : "", sysconf.sys_contact);
    }

    if (req->all_defaults ||
        (strcmp(sysconf.sys_location, def_sysconf.sys_location))) {
        rc = vtss_icfg_printf(result, "%s%s%s\n",
                              "snmp-server location ", req->all_defaults ? "no" : "", sysconf.sys_location);
    }

    rc = SNMPV3_ICFG_community_conf(req, result);
    rc = SNMPV3_ICFG_user_conf(req, result);
    rc = SNMPV3_ICFG_group_conf(req, result);
    rc = SNMPV3_ICFG_view_conf(req, result);
    rc = SNMPV3_ICFG_access_conf(req, result);

    return rc;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
mesa_rc snmp_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_SNMP_GLOBAL_CONF, "snmp", SNMP_ICFG_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    return rc;
}
