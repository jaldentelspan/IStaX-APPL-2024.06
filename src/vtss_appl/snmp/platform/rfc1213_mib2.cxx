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

#include <main.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif                          /* HAVE_STDLIB_H */
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif                          /* HAVE_STRING_H */

#include "vtss_os_wrapper_snmp.h"
#include "vtss_snmp_api.h"
#include "mibContextTable.h"  //mibContextTable_register
#include "rfc1213_mib2.h"
#include "ucd_snmp_rfc1213_mib2.h"

#if RFC1213_SUPPORTED_ORTABLE
#include "sysORTable.h"
#endif /* RFC1213_SUPPORTED_ORTABLE */

#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()
#include "msg_api.h"
#include "sysutil_api.h"
#include "port_iter.hxx"
#ifdef VTSS_SW_OPTION_LACP
#include "lacp_api.h"
#endif                          /* VTSS_SW_OPTION_LACP */
#include "mgmt_api.h"
#include "misc_api.h"
#include "vtss/appl/ip.h"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_SNMP
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_SNMP

#if RFC1213_SUPPORTED_SYSTEM
/* system --------------------------------------------------------------------*/
#include "snmp_custom_api.h"
#define SNMP_CONF_SYS_SERVICES      (2) /* 2 = datalink/subnetwork (e.g., bridges) */
#define SNMP_CONF_SYS_MAX_LEN       255
#define SNMP_SYS_DESCR_LEN          255

#define SYSDESCR        1
#define SYSOBJECTID     2
#define SYSUPTIME       3
#define SYSCONTACT      4
#define SYSNAME         5
#define SYSLOCATION     6
#define SYSSERVICES     7
#define SYSORLASTCHANGE 8
#define SYSORINDEX      9
#define SYSORID         10
#define SYSORDESCR      11
#define SYSORUPTIME     12

#define ETHER_ADDR_LEN 6
static system_conf_t old_system_conf, now_system_conf;

/*
 * Initializes the system module
 */
void
init_mib2_system(void)
{
    oid system_variables_oid[] = { 1, 3, 6, 1, 2, 1, 1 };

    //Register mibContextTable
    mibContextTable_register(system_variables_oid,
                             sizeof(system_variables_oid) / sizeof(oid),
                             "RFC1213-MIB : system");

    ucd_snmp_init_mib2_system();
}

/*
 * var_system():
 *   This function is called every time the agent gets a request for
 *   a scalar variable that might be found within your mib section
 *   registered above.  It is up to you to do the right thing and
 *   return the correct value.
 *     You should also correct the value of "var_len" if necessary.
 *
 *   Please see the documentation for more information about writing
 *   module extensions, and check out the examples in the examples
 *   and mibII directories.
 */
u_char         *
var_system(struct variable *vp,
           oid *name,
           size_t *length,
           int exact, size_t *var_len, WriteMethod **write_method)
{
    /*
     * variables we may use later
     * static long long_ret;
     * static u_long ulong_ret;
     * static u_char string[SPRINT_MAX_LEN];
     * static oid objid[MAX_OID_LEN];
     * static struct counter64 c64;
     */
    static u_long ulong_ret;

    if (header_generic(vp, name, length, exact, var_len, write_method)
        == MATCH_FAILED) {
        return NULL;
    }

    /*
     * this is where we do the value assignments for the mib results.
     */
    switch (vp->magic) {
#if RFC1213_SUPPORTED_ORTABLE
    case SYSORLASTCHANGE: {
        sysORTable_LastChange_get(&ulong_ret);
        return (u_char *) &ulong_ret;
    }
#endif /* RFC1213_SUPPORTED_ORTABLE */
    case SYSNAME: {
        *write_method = write_sysName;
        system_get_config(&old_system_conf);
        *var_len = strlen(old_system_conf.sys_name);
        return (u_char *)old_system_conf.sys_name;
    }
    case SYSSERVICES: {
        system_get_config(&old_system_conf);
        ulong_ret = old_system_conf.sys_services;
        return (u_char *) &ulong_ret;
    }
    case SYSLOCATION: {
        *write_method = write_sysLocation;
        system_get_config(&old_system_conf);
        *var_len = strlen(old_system_conf.sys_location);
        return (u_char *)old_system_conf.sys_location;
    }
    case SYSOBJECTID: {
        *var_len = snmp_private_mib_oid_len_get() * sizeof(oid);
        return (u_char *)snmp_private_mib_oid;
    }
    case SYSDESCR: {
        char *string_p;
        static char descr_version[SNMP_SYS_DESCR_LEN + 1];
        string_p = &descr_version[0];
        strncpy(descr_version, system_get_descr(), SNMP_SYS_DESCR_LEN);
        strncat(descr_version, ", firmware ", SNMP_SYS_DESCR_LEN - strlen(descr_version));
        strncat(descr_version, misc_software_version_txt(), SNMP_SYS_DESCR_LEN - strlen(descr_version));
        descr_version[SNMP_SYS_DESCR_LEN] = 0;
        *var_len = strlen((char *)string_p);
        return (u_char *)string_p;
    }
    case SYSUPTIME: {
        struct timeval now;
        struct timespec now_n;

        //James: gettimeofday(&now, NULL);
        clock_gettime(CLOCK_MONOTONIC, &now_n);
        now.tv_sec = now_n.tv_sec;
        now.tv_usec = now_n.tv_nsec / 1000;

        ulong_ret = (now.tv_sec * 100) + (now.tv_usec / 10000);
        return (u_char *) &ulong_ret;
    }
    case SYSCONTACT: {
        *write_method = write_sysContact;
        system_get_config(&old_system_conf);
        *var_len = strlen(old_system_conf.sys_contact);
        return (u_char *)old_system_conf.sys_contact;
    }
    default:
        DEBUGMSGTL(("snmpd", "unknown sub-id %d in var_system\n",
                    vp->magic));
    }
    return NULL;
}

int write_sysName(int action,
                  u_char *var_val,
                  u_char var_val_type,
                  size_t var_val_len,
                  u_char *statP, oid *name, size_t name_len)
{
    char           *buf, *old_buf;
    int             max_size;

    buf      = now_system_conf.sys_name;
    old_buf  = old_system_conf.sys_name;
    max_size = SNMP_CONF_SYS_MAX_LEN;

    switch (action) {
    case RESERVE1: {
        if (var_val_type != ASN_OCTET_STR) {
            snmp_log(LOG_ERR, "write to sysName: not ASN_OCTET_STR\n");
            return SNMP_ERR_WRONGTYPE;
        }
        if (var_val_len > max_size) {
            snmp_log(LOG_ERR, "write to sysName: bad length\n");
            return SNMP_ERR_WRONGLENGTH;
        }
        break;
    }
    case RESERVE2: {
        /*
         * Allocate memory and similar resources
         */
        break;
    }
    case FREE: {
        /*
         * Release any resources that have been allocated
         */
        break;
    }
    case ACTION: {
        /*
         * The variable has been stored in 'value' for you to use,
         * and you have just been asked to do something with it.
         * Note that anything done here must be reversable in the UNDO case
         */
        /*
         * Save to current configuration
         */
        system_conf_t system_conf;

        memcpy(buf, var_val, var_val_len);
        buf[var_val_len] = '\0';
        if (!RFC1213_MIB2C_is_nvt_string(buf)) {
            return SNMP_ERR_WRONGVALUE;
        }
        if (!system_name_is_administratively(buf)) {
            return SNMP_ERR_BADVALUE;
        }
        system_get_config(&system_conf);
        strcpy(system_conf.sys_name, buf);
        if (system_set_config(&system_conf) != VTSS_RC_OK) {
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        break;
    }
    case UNDO: {
        /*
         * Back out any changes made in the ACTION case
         */
        /*
         * Restore current configuration form old configuration
         */
        system_conf_t system_conf;

        strcpy(buf, old_buf);
        system_get_config(&system_conf);
        strcpy(system_conf.sys_name, buf);
        if (system_set_config(&system_conf) != VTSS_RC_OK) {
            return SNMP_ERR_UNDOFAILED;
        }
        break;
    }
    case COMMIT: {
        /*
         * Things are working well, so it's now safe to make the change
         * permanently.  Make sure that anything done here can't fail!
         */
        /*
         * Update old configuration
         */
        strcpy(old_buf, buf);
        break;
    }
    }
    return SNMP_ERR_NOERROR;
}

int write_sysLocation(int action,
                      u_char *var_val,
                      u_char var_val_type,
                      size_t var_val_len,
                      u_char *statP, oid *name, size_t name_len)
{
    char           *buf, *old_buf;
    int             max_size;

    buf      = now_system_conf.sys_location;
    old_buf  = old_system_conf.sys_location;
    max_size = SNMP_CONF_SYS_MAX_LEN;

    switch (action) {
    case RESERVE1: {
        if (var_val_type != ASN_OCTET_STR) {
            snmp_log(LOG_ERR,
                     "write to sysLocation: not ASN_OCTET_STR\n");
            return SNMP_ERR_WRONGTYPE;
        }
        if (var_val_len > max_size) {
            snmp_log(LOG_ERR, "write to sysLocation: bad length\n");
            return SNMP_ERR_WRONGLENGTH;
        }
        break;
    }
    case RESERVE2: {
        /*
         * Allocate memory and similar resources
         */
        break;
    }
    case FREE: {
        /*
         * Release any resources that have been allocated
         */
        break;
    }
    case ACTION: {
        /*
         * The variable has been stored in 'value' for you to use,
         * and you have just been asked to do something with it.
         * Note that anything done here must be reversable in the UNDO case
         */
        /*
         * Save to current configuration
         */
        system_conf_t system_conf;

        memcpy(buf, var_val, var_val_len);
        buf[var_val_len] = '\0';
        if (!RFC1213_MIB2C_is_nvt_string(buf)) {
            return SNMP_ERR_WRONGVALUE;
        }
        system_get_config(&system_conf);
        strcpy(system_conf.sys_location, buf);
        if (system_set_config(&system_conf) != VTSS_RC_OK) {
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        break;
    }
    case UNDO: {
        /*
         * Back out any changes made in the ACTION case
         */
        /*
         * Restore current configuration form old configuration
         */
        system_conf_t system_conf;

        strcpy(buf, old_buf);
        system_get_config(&system_conf);
        strcpy(system_conf.sys_location, buf);
        if (system_set_config(&system_conf) != VTSS_RC_OK) {
            return SNMP_ERR_UNDOFAILED;
        }
        break;
    }
    case COMMIT: {
        /*
         * Things are working well, so it's now safe to make the change
         * permanently.  Make sure that anything done here can't fail!
         */
        /*
         * Update old configuration
         */
        strcpy(old_buf, buf);
        break;
    }
    }
    return SNMP_ERR_NOERROR;
}

int write_sysContact(int action,
                     u_char *var_val,
                     u_char var_val_type,
                     size_t var_val_len,
                     u_char *statP, oid *name, size_t name_len)
{
    char           *buf, *old_buf;
    int             max_size;

    buf      = now_system_conf.sys_contact;
    old_buf  = old_system_conf.sys_contact;
    max_size = SNMP_CONF_SYS_MAX_LEN;

    switch (action) {
    case RESERVE1: {
        if (var_val_type != ASN_OCTET_STR) {
            snmp_log(LOG_ERR,
                     "write to sysContact: not ASN_OCTET_STR\n");
            return SNMP_ERR_WRONGTYPE;
        }
        if (var_val_len > max_size) {
            snmp_log(LOG_ERR, "write to sysContact: bad length\n");
            return SNMP_ERR_WRONGLENGTH;
        }
        break;
    }
    case RESERVE2: {
        /*
         * Allocate memory and similar resources
         */
        break;
    }
    case FREE: {
        /*
         * Release any resources that have been allocated
         */
        break;
    }
    case ACTION: {
        /*
         * The variable has been stored in 'value' for you to use,
         * and you have just been asked to do something with it.
         * Note that anything done here must be reversable in the UNDO case
         */
        /*
         * Save to current configuration
         */
        system_conf_t system_conf;

        memcpy(buf, var_val, var_val_len);
        buf[var_val_len] = '\0';
        if (!RFC1213_MIB2C_is_nvt_string(buf)) {
            return SNMP_ERR_WRONGVALUE;
        }
        system_get_config(&system_conf);
        strcpy(system_conf.sys_contact, buf);
        if (system_set_config(&system_conf) != VTSS_RC_OK) {
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }
        break;
    }
    case UNDO: {
        /*
         * Back out any changes made in the ACTION case
         */
        /*
         * Restore current configuration form old configuration
         */
        system_conf_t system_conf;

        strcpy(buf, old_buf);
        system_get_config(&system_conf);
        strcpy(system_conf.sys_contact, buf);
        if (system_set_config(&system_conf) != VTSS_RC_OK) {
            return SNMP_ERR_UNDOFAILED;
        }
        break;
    }
    case COMMIT: {
        /*
         * Things are working well, so it's now safe to make the change
         * permanently.  Make sure that anything done here can't fail!
         */
        /*
         * Update old configuration
         */
        strcpy(old_buf, buf);
        break;
    }
    }
    return SNMP_ERR_NOERROR;
}
#endif /* RFC1213_SUPPORTED_SYSTEM */

#if RFC1213_SUPPORTED_INTERFACES
/* interfaces --------------------------------------------------------------------*/
#include <sys/time.h>
#include "conf_api.h"

static CapArray<BOOL, VTSS_APPL_CAP_ISID_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> old_admin_status;
static CapArray<BOOL, VTSS_APPL_CAP_ISID_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> now_admin_status;
static CapArray<struct timeval, VTSS_APPL_CAP_ISID_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> if_LastChange;

#define IFNUMBER            1
#define IFINDEX             2
#define IFDESCR             3
#define IFTYPE              4
#define IFMTU               5
#define IFSPEED             6
#define IFPHYSADDRESS       7
#define IFADMINSTATUS       8
#define IFOPERSTATUS        9
#define IFLASTCHANGE        10
#define IFINOCTETS          11
#define IFINUCASTPKTS       12
#define IFINNUCASTPKTS      13
#define IFINDISCARDS        14
#define IFINERRORS          15
#define IFINUNKNOWNPROTOS   16
#define IFOUTOCTETS         17
#define IFOUTUCASTPKTS      18
#define IFOUTNUCASTPKTS     19
#define IFOUTDISCARDS       20
#define IFOUTERRORS         21
#define IFOUTQLEN           22
#define IFSPECIFIC          23
/*
 * Initializes the interfaces module
 */
void
init_mib2_interfaces(void)
{
    oid interfaces_variables_oid[] = { 1, 3, 6, 1, 2, 1, 2 };

    //Register mibContextTable
    mibContextTable_register(interfaces_variables_oid,
                             sizeof(interfaces_variables_oid) / sizeof(oid),
                             "RFC1213-MIB : interfaces");

    ucd_snmp_init_mib2_interfaces();
}

static ulong vlan_cnt_get(void)
{
    ulong                  vlan_cnt = 0;
    mesa_vid_t             vid      = VTSS_VID_NULL;
    vtss_appl_vlan_entry_t entry;

    while ((vtss_appl_vlan_get(VTSS_ISID_GLOBAL, vid, &entry, TRUE, VTSS_APPL_VLAN_USER_ALL)) == VTSS_RC_OK) {
        vid = entry.vid;
        vlan_cnt++;
    }

    return vlan_cnt;
}

ulong rfc1213_get_ifNumber(void)
{
    ulong          if_number = 0;
    iftable_info_t table_info;

    table_info.ifIndex = 0;
    while (TRUE == ifIndex_get_next(&table_info)) {
        if (IFTABLE_IFINDEX_TYPE_VLAN == table_info.type) {
            table_info.type = IFTABLE_IFINDEX_TYPE_VLAN;
            table_info.if_id = VTSS_APPL_VLAN_ID_MAX; // The highest VID the device supports
            (void) ifIndex_get_by_interface((&table_info));
            if_number += vlan_cnt_get();
        } else {
            if_number++;
        }
    }
    return if_number;
}

static BOOL update_ifTable_entry(vtss_isid_t isid, mesa_port_no_t port_idx, ifTable_entry_t *table_entry_p, char mac_address[ETHER_ADDR_LEN], BOOL is_vlan)
{
    vtss_appl_port_conf_t    port_conf;
    vtss_appl_port_status_t  port_status;
    mesa_port_counters_t     counters;
    int                      mac_idx, mac_add_value;

    T_D("isid = %d, port_idx = %d", (int)isid, (int)port_idx);
    if (port_idx >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        T_D("VTSS_PORT_IS_PORT fail");
        return FALSE;
    }

    vtss_ifindex_t ifindex;
    (void)vtss_ifindex_from_port(isid, port_idx, &ifindex);
    if (vtss_appl_port_conf_get(ifindex, &port_conf) != VTSS_RC_OK) {
        T_D("appl_port_conf_get fail");
        return FALSE;
    }

    if (msg_switch_exists(isid) && vtss_appl_port_status_get(ifindex, &port_status) != VTSS_RC_OK) {
        T_D("VTSS_PORT_IS_PORT fail");
        return FALSE;
    } else if (!msg_switch_exists(isid)) {
        memset(&port_status, 0, sizeof(port_status));
    }

    if (msg_switch_exists(isid) && vtss_appl_port_statistics_get(ifindex, &counters) != VTSS_RC_OK) {
        T_D("appl_port_counters_get fail");
        return FALSE;
    } else if (!msg_switch_exists(isid)) {
        memset(&counters, 0, sizeof(counters));
    }

    if (is_vlan) {
        memset(table_entry_p->ifPhysAddress, 0x0, sizeof(table_entry_p->ifPhysAddress));
        table_entry_p->ifAdminStatus = 1; /* ifAdminStatus is always enabled (read-only) for VLAN interfaces */
        table_entry_p->ifOperStatus  = port_status.link ? 1 : table_entry_p->ifOperStatus;
        return TRUE;
    }

    if (!memcmp(mac_address, table_entry_p->ifPhysAddress, ETHER_ADDR_LEN)) {
        for (mac_idx = 5, mac_add_value = port_idx + 1; mac_idx >= 0; mac_idx--) {
            if ((table_entry_p->ifPhysAddress[mac_idx] + mac_add_value) > 0xFF) {
                mac_add_value = 1;
                table_entry_p->ifPhysAddress[mac_idx] = (table_entry_p->ifPhysAddress[mac_idx] + mac_add_value) - 0xFF;
            } else {
                table_entry_p->ifPhysAddress[mac_idx] += mac_add_value;
                break;
            }
        }
    }

    old_admin_status[isid - VTSS_ISID_START][port_idx] = port_conf.admin.enable ? 1 : 2;

    table_entry_p->ifMtu         = port_conf.max_length;
    table_entry_p->ifAdminStatus = port_conf.admin.enable ? 1 : table_entry_p->ifAdminStatus;
    table_entry_p->ifOperStatus  = port_status.link ? 1 : table_entry_p->ifOperStatus;

    if (table_entry_p->ifOperStatus == 1) {
        table_entry_p->ifSpeed += (port_status.speed == MESA_SPEED_25G ? 25000 : (port_status.speed == MESA_SPEED_10G ? 10000 : (port_status.speed == MESA_SPEED_5G ? 5000 : (port_status.speed == MESA_SPEED_2500M ? 2500 : (port_status.speed == MESA_SPEED_1G ? 1000 : (port_status.speed == MESA_SPEED_100M ? 100 : 10))))));
    } else {
        table_entry_p->ifSpeed += (port_conf.speed == MESA_SPEED_25G ? 25000 : (port_conf.speed == MESA_SPEED_10G ? 10000 : (port_conf.speed == MESA_SPEED_5G ? 5000 : (port_conf.speed == MESA_SPEED_2500M ? 2500 : (port_conf.speed == MESA_SPEED_1G ? 1000 : (port_conf.speed == MESA_SPEED_100M ? 100 : 10))))));
    }

    if (table_entry_p->ifLastChange < (if_LastChange[isid - VTSS_ISID_START][port_idx].tv_sec * 100) + (if_LastChange[isid - VTSS_ISID_START][port_idx].tv_usec / 10000)) {
        table_entry_p->ifLastChange = (if_LastChange[isid - VTSS_ISID_START][port_idx].tv_sec * 100) + (if_LastChange[isid - VTSS_ISID_START][port_idx].tv_usec / 10000);
    }

    table_entry_p->ifInOctets         += counters.if_group.ifInOctets;
    table_entry_p->ifInUcastPkts      += counters.if_group.ifInUcastPkts;
    table_entry_p->ifInNUcastPkts     += counters.if_group.ifInNUcastPkts;
    table_entry_p->ifInDiscards       += counters.if_group.ifInDiscards;
    table_entry_p->ifInErrors         += counters.if_group.ifInErrors;
    //table_entry_p->ifInUnknownProtos  += 0;
    table_entry_p->ifOutOctets        += counters.if_group.ifOutOctets;
    table_entry_p->ifOutUcastPkts     += counters.if_group.ifOutUcastPkts;
    table_entry_p->ifOutNUcastPkts    += counters.rmon.tx_etherStatsBroadcastPkts + counters.rmon.tx_etherStatsMulticastPkts;
    table_entry_p->ifOutDiscards      += counters.if_group.ifOutDiscards;
    table_entry_p->ifOutErrors        += counters.if_group.ifOutErrors;
    //table_entry_p->ifOutQLen         += 0;
    //memset(table_entry_p->ifSpecific, 0x0, sizeof(table_entry_p->ifSpecific));
    table_entry_p->ifSpecific_len      = 2;

    T_D(" OK");
    return TRUE;
}

BOOL get_ifTable_entry(iftable_info_t *table_info, ifTable_entry_t *table_entry_p)
{
    u_char                   mac_address[ETHER_ADDR_LEN];
#ifdef VTSS_SW_OPTION_AGGR
    switch_iter_t            sit;
    port_iter_t              pit;
    aggr_mgmt_group_member_t aggr_members;
#endif /* VTSS_SW_OPTION_AGGR */

    memset(table_entry_p, 0x0, sizeof(ifTable_entry_t));
    memset(mac_address, 0x0, ETHER_ADDR_LEN);
    table_entry_p->ifIndex = table_info->ifIndex;
    table_entry_p->ifType = IANA_IFTYPE_ETHER;
    table_entry_p->ifAdminStatus = 2;
    table_entry_p->ifOperStatus  = 2;

    {
        conf_board_t board_conf;

        (void)conf_mgmt_board_get(&board_conf);
        bcopy(board_conf.mac_address.addr, mac_address, ETHER_ADDR_LEN);
    }

    bcopy(mac_address, table_entry_p->ifPhysAddress, ETHER_ADDR_LEN);

    switch (table_info->type) {
    case IFTABLE_IFINDEX_TYPE_PORT:
        snprintf((char *)table_entry_p->ifDescr, sizeof(table_entry_p->ifDescr), "Switch %2d - Port %2u", topo_isid2usid(table_info->isid), (u32)iport2uport(table_info->if_id));

        if (update_ifTable_entry(table_info->isid, table_info->if_id, table_entry_p, (char *)mac_address, FALSE) == FALSE) {
            return FALSE;
        }
        break;
#ifdef VTSS_SW_OPTION_AGGR
    case IFTABLE_IFINDEX_TYPE_LLAG:
        table_entry_p->ifType        = IANA_IFTYPE_LAG;

        if ((aggr_mgmt_port_members_get(table_info->isid, table_info->if_id, &aggr_members, FALSE) != VTSS_RC_OK)
#ifdef VTSS_SW_OPTION_LACP
            && (aggr_mgmt_lacp_members_get(table_info->isid, table_info->if_id, &aggr_members, FALSE) != VTSS_RC_OK)
#endif /* VTSS_SW_OPTION_LACP */
           ) {
            return FALSE;
        }

        snprintf((char *)table_entry_p->ifDescr, sizeof(table_entry_p->ifDescr), "Link Aggregations %2u", mgmt_aggr_no2id(table_info->if_id));

        (void)port_iter_init(&pit, NULL, table_info->isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (!aggr_members.entry.member[pit.iport]) {
                continue;
            }
            if (update_ifTable_entry(table_info->isid, pit.iport, table_entry_p, (char *)mac_address, FALSE) == FALSE) {
                return FALSE;
            }
        }
        break;
    case IFTABLE_IFINDEX_TYPE_GLAG:
        table_entry_p->ifType        = IANA_IFTYPE_LAG;
        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
            if (aggr_mgmt_port_members_get(sit.isid, mgmt_aggr_id2no(table_info->if_id), &aggr_members, FALSE) != VTSS_RC_OK) {
                continue;
            }

            snprintf((char *)table_entry_p->ifDescr, sizeof(table_entry_p->ifDescr), "Global Link Aggregations %u", mgmt_aggr_no2id(aggr_members.aggr_no) );

            (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (!aggr_members.entry.member[pit.iport]) {
                    continue;
                }
                if (update_ifTable_entry(sit.isid, pit.iport, table_entry_p, (char *)mac_address, FALSE) == FALSE) {
                    return FALSE;
                }
            }
        }
        break;
#endif /* VTSS_SW_OPTION_AGGR */
    case IFTABLE_IFINDEX_TYPE_VLAN:
        vtss_appl_ip_if_status_link_t       status;

        table_entry_p->ifType        = IANA_IFTYPE_L2VLAN;
        table_entry_p->ifSpeed           = 1000;

        snprintf((char *)table_entry_p->ifDescr, sizeof(table_entry_p->ifDescr), "VLAN %4d", (int)table_info->if_id);
        (void)vtss_appl_ip_if_status_link_get(vtss_ifindex_cast_from_u32_0(table_info->ifIndex), &status);

        table_entry_p->ifAdminStatus     = (status.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP) ? 1 : 2;
        table_entry_p->ifOperStatus      = (status.flags & VTSS_APPL_IP_IF_LINK_FLAG_RUNNING) ? 1 : 2;

        memset(table_entry_p->ifSpecific, 0x0, sizeof(table_entry_p->ifSpecific));
        table_entry_p->ifSpecific_len    = 2;
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

mesa_rc rfc1213_set_ifAdminStatus(vtss_isid_t isid, mesa_port_no_t port_no, BOOL status)
{
    mesa_rc     rc;
    vtss_appl_port_conf_t port_conf;

    vtss_ifindex_t ifindex;
    (void)vtss_ifindex_from_port(isid, port_no, &ifindex);

    if ((rc = vtss_appl_port_conf_get(ifindex, &port_conf)) != VTSS_RC_OK) {
        return rc;
    }
    port_conf.admin.enable = status;
    return (vtss_appl_port_conf_set(ifindex, &port_conf));
}

/*
 * Port state change indication
 */
void interfaces_port_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    struct timespec now_n;

    if (msg_switch_is_primary()) {
        //James: gettimeofday(&if_LastChange[0][port_no], NULL);
        clock_gettime(CLOCK_MONOTONIC, &now_n);
        if_LastChange[0][port_no].tv_sec = now_n.tv_sec;
        if_LastChange[0][port_no].tv_usec = now_n.tv_nsec / 1000;
    }
}

int write_ifAdminStatus(int action,
                        u_char *var_val,
                        u_char var_val_type,
                        size_t var_val_len,
                        u_char *statP, oid *name, size_t name_len)
{
    BOOL                     *buf, *old_buf;
    int                      max_size;
    long                     intval;
    int                      if_num;
    iftable_info_t           table_info;
#ifdef VTSS_SW_OPTION_AGGR
    vtss_isid_t              isid;
    aggr_mgmt_group_member_t aggr_members;
    port_iter_t              pit;
#endif /* VTSS_SW_OPTION_AGGR */

    buf      = NULL;
    old_buf  = NULL;
    max_size = sizeof(long);
    intval   = *((long *) var_val);

    if_num = name[name_len - 1];
    table_info.ifIndex = if_num;

    if (ifIndex_get(&table_info) == FALSE) {
        return SNMP_ERR_RESOURCEUNAVAILABLE;
    }

    switch (action) {
    case RESERVE1: {
        if (var_val_type != ASN_INTEGER) {
            snmp_log(LOG_ERR,
                     "write to ifAdminStatus: not ASN_INTEGER\n");
            return SNMP_ERR_WRONGTYPE;
        }
        if (var_val_len > max_size) {
            snmp_log(LOG_ERR, "write to ifAdminStatus: bad length\n");
            return SNMP_ERR_WRONGLENGTH;
        }
        if (intval != 1 && intval != 2 && intval != 3) {
            snmp_log(LOG_ERR, "write to ifAdminStatus: bad value\n");
            return SNMP_ERR_WRONGVALUE;
        }
        return SNMP_ERR_NOERROR;
    }
    case RESERVE2: {
        /*
         * Allocate memory and similar resources
         */
        return SNMP_ERR_NOERROR;
    }
    case FREE: {
        /*
         * Release any resources that have been allocated
         */
        return SNMP_ERR_NOERROR;
    }
    }

    switch (table_info.type) {
    case IFTABLE_IFINDEX_TYPE_PORT:
        buf = &now_admin_status[table_info.isid - VTSS_ISID_START][table_info.if_id];
        old_buf = &old_admin_status[table_info.isid - VTSS_ISID_START][table_info.if_id];
    case IFTABLE_IFINDEX_TYPE_LLAG:
    case IFTABLE_IFINDEX_TYPE_GLAG:
        break;
    case IFTABLE_IFINDEX_TYPE_VLAN:
        if (action == UNDO) {
            return SNMP_ERR_NOERROR;
        } else {
            return SNMP_ERR_NOTWRITABLE;
        }
    default:
        break;
    }

    switch (table_info.type) {
    case IFTABLE_IFINDEX_TYPE_PORT: {
        if (action == ACTION) {
            /*
             * The variable has been stored in 'value' for you to use,
             * * and you have just been asked to do something with it.
             * * Note that anything done here must be reversable in the UNDO case
             */
            /*
             * save to current configuration
             */
            *buf = *((BOOL *) var_val);
            /* AdminStatus - testing(3) does not supported in E-StaX34 project */
            if (rfc1213_set_ifAdminStatus(table_info.isid, table_info.if_id, (*buf == 2 ? 0 : 1)) != VTSS_RC_OK) {
                return SNMP_ERR_RESOURCEUNAVAILABLE;
            }
        } else if (action == UNDO) {
            /*
             * Back out any changes made in the ACTION case
             */
            /*
             * restore current configuration form old configuration
             */
            *buf = *old_buf;
            /* AdminStatus - testing(3) does not supported in E-StaX34 project */
            if (rfc1213_set_ifAdminStatus(table_info.isid, table_info.if_id, (*buf == 2 ? 0 : 1)) != VTSS_RC_OK) {
                return SNMP_ERR_UNDOFAILED;
            }
        } else if (action == COMMIT) {
            /*
             * update old configuration
             */
            *old_buf = *buf;
        }
        break;
    }

#ifdef VTSS_SW_OPTION_AGGR
    case IFTABLE_IFINDEX_TYPE_LLAG: {
        if ((aggr_mgmt_port_members_get(table_info.isid, table_info.if_id, &aggr_members, FALSE) != VTSS_RC_OK)
#ifdef VTSS_SW_OPTION_LACP
            && (aggr_mgmt_lacp_members_get(table_info.isid, table_info.if_id, &aggr_members, FALSE) != VTSS_RC_OK)
#endif /* VTSS_SW_OPTION_LACP */
           ) {
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        }

        (void)port_iter_init(&pit, NULL, table_info.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (!aggr_members.entry.member[pit.iport]) {
                continue;
            }

            buf = &now_admin_status[table_info.isid - VTSS_ISID_START][pit.iport];
            old_buf = &old_admin_status[table_info.isid - VTSS_ISID_START][pit.iport];

            if (action == ACTION) {
                /*
                 * save to current configuration
                 */
                *buf = *((BOOL *) var_val);
                /* AdminStatus - testing(3) does not supported in E-StaX34 project */
                if (rfc1213_set_ifAdminStatus(table_info.isid, pit.iport, (*buf == 2 ? 0 : 1)) != VTSS_RC_OK) {
                    return SNMP_ERR_RESOURCEUNAVAILABLE;
                }
            } else if (action == UNDO) {
                /*
                 * restore current configuration form old configuration
                 */
                *buf = *old_buf;
                /* AdminStatus - testing(3) does not supported in E-StaX34 project */
                if (rfc1213_set_ifAdminStatus(table_info.isid, pit.iport, (*buf == 2 ? 0 : 1)) != VTSS_RC_OK) {
                    return SNMP_ERR_UNDOFAILED;
                }
            } else if (action == COMMIT) {
                /*
                 * update old configuration
                 */
                *old_buf = *buf;
            }
        }
        break;
    }

    case IFTABLE_IFINDEX_TYPE_GLAG: {
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            if (!msg_switch_configurable(isid)) {
                continue;
            }

            if (aggr_mgmt_port_members_get(isid, table_info.if_id, &aggr_members, FALSE) != VTSS_RC_OK) {
                return SNMP_ERR_RESOURCEUNAVAILABLE;
            }

            (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (!aggr_members.entry.member[pit.iport]) {
                    continue;
                }

                buf = &now_admin_status[isid - VTSS_ISID_START][pit.iport];
                old_buf = &old_admin_status[isid - VTSS_ISID_START][pit.iport];

                if (action == ACTION) {
                    /*
                     * save to current configuration
                     */
                    *buf = *((BOOL *) var_val);
                    /* AdminStatus - testing(3) does not supported in E-StaX34 project */
                    if (rfc1213_set_ifAdminStatus(isid, pit.iport, (*buf == 2 ? 0 : 1)) != VTSS_RC_OK) {
                        return SNMP_ERR_RESOURCEUNAVAILABLE;
                    }
                } else if (action == UNDO) {
                    /*
                     * restore current configuration form old configuration
                     */
                    *buf = *old_buf;
                    /* AdminStatus - testing(3) does not supported in E-StaX34 project */
                    if (rfc1213_set_ifAdminStatus(isid, pit.iport, (*buf == 2 ? 0 : 1)) != VTSS_RC_OK) {
                        return SNMP_ERR_UNDOFAILED;
                    }
                } else if (action == COMMIT) {
                    /*
                     * update old configuration
                     */
                    *old_buf = *buf;
                }
            }
        }
        break;
    }
#endif /* VTSS_SW_OPTION_AGGR */
    default:
        break;
    }
    return SNMP_ERR_NOERROR;
}
#endif /* RFC1213_SUPPORTED_INTERFACES */

#if RFC1213_SUPPORTED_IP
/* ip --------------------------------------------------------------------*/
/*
 * Initializes the ip module
 */
void
init_mib2_ip(void)
{
    oid ip_variables_oid[] = { 1, 3, 6, 1, 2, 1, 4 };

    //Register mibContextTable
    mibContextTable_register(ip_variables_oid,
                             sizeof(ip_variables_oid) / sizeof(oid),
                             "RFC1213-MIB : ip");

    ucd_snmp_init_mib2_ip();
}
#endif /* RFC1213_SUPPORTED_IP */

#if RFC1213_SUPPORTED_ICMP
/* icmp --------------------------------------------------------------------*/
/*
 * Initializes the icmp module
 */
void
init_mib2_icmp(void)
{
    oid icmp_variables_oid[] = { 1, 3, 6, 1, 2, 1, 5 };

    //Register mibContextTable
    mibContextTable_register(icmp_variables_oid,
                             sizeof(icmp_variables_oid) / sizeof(oid),
                             "RFC1213-MIB : icmp");

    ucd_snmp_init_mib2_icmp();
}
#endif /* RFC1213_SUPPORTED_ICMP */

#if RFC1213_SUPPORTED_TCP
/* tcp --------------------------------------------------------------------*/
/*
 * Initializes the tcp module
 */
void
init_mib2_tcp(void)
{
    oid tcp_variables_oid[] = { 1, 3, 6, 1, 2, 1, 6 };

    //Register mibContextTable
    mibContextTable_register(tcp_variables_oid,
                             sizeof(tcp_variables_oid) / sizeof(oid),
                             "RFC1213-MIB : tcp");

    ucd_snmp_init_mib2_tcp();
}
#endif /* RFC1213_SUPPORTED_TCP */

#if RFC1213_SUPPORTED_UDP
/*
 * Initializes the snmp module
 */
void init_mib2_udp(void)
{
    oid udp_variables_oid[] = { 1, 3, 6, 1, 2, 1, 7 };

    //Register mibContextTable
    mibContextTable_register(udp_variables_oid,
                             sizeof(udp_variables_oid) / sizeof(oid),
                             "RFC1213-MIB : udp");

    ucd_snmp_init_mib2_udp();
}
#endif /* RFC1213_SUPPORTED_UDP */

#if RFC1213_SUPPORTED_SNMP
/*
 * Initializes the snmp module
 */
void init_mib2_snmp(void)
{
    oid snmp_variables_oid[] = { 1, 3, 6, 1, 2, 1, 5 };

    //Register mibContextTable
    mibContextTable_register(snmp_variables_oid,
                             sizeof(snmp_variables_oid) / sizeof(oid),
                             "RFC1213-MIB : snmp");

    ucd_snmp_init_mib2_snmp();
}

#endif /* RFC1213_SUPPORTED_SNMP */

int get_available_lport_ifTableIndex(int if_num)
{
    iftable_info_t info;
    u32            if_types = 0;

    if (if_num == 0xFFFFFFFF) {
        return IFTABLE_IFINDEX_END;
    }
    /* Since if_num must be a posotive number, so if_num must be convert to unsigne long */
    info.ifIndex = (ifIndex_id_t)if_num - 1;
    IFTABLE_IFINDEX_TYPE_FLAG_SET(if_types, IFTABLE_IFINDEX_TYPE_PORT);

    if (TRUE == ifIndex_get_next_by_type(&info, if_types)) {
        return info.ifIndex;
    }
    /* cann't find any valid interface, return the value of 'IFTABLE_IFINDEX_END' */
    return IFTABLE_IFINDEX_END;
}

int get_available_lag_ifTableIndex(int if_num)
{
    iftable_info_t info;
    u32            if_types = 0;

    info.ifIndex = if_num <= 0 ? 0 : (ifIndex_id_t)if_num - 1;
    IFTABLE_IFINDEX_TYPE_FLAG_SET(if_types, IFTABLE_IFINDEX_TYPE_LLAG);
    IFTABLE_IFINDEX_TYPE_FLAG_SET(if_types, IFTABLE_IFINDEX_TYPE_GLAG);

    if (TRUE == ifIndex_get_next_by_type(&info, if_types)) {
        return info.ifIndex;
    }

    /* cann't find any valid interface, return the value of 'IFTABLE_IFINDEX_END' */
    return IFTABLE_IFINDEX_END;

}

int get_available_vlan_ifTableIndex(int if_num)
{
    iftable_info_t info;

    info.type = IFTABLE_IFINDEX_TYPE_VLAN;
    if ( TRUE != ifIndex_get_first_by_type(&info)  ) {
        return IFTABLE_IFINDEX_END;
    }

    while ( (ifIndex_id_t)if_num <= info.ifIndex) {
        if ( TRUE != ifIndex_get_next(&info) || info.type != IFTABLE_IFINDEX_TYPE_VLAN ) {
            return IFTABLE_IFINDEX_END;
        }
    }

    return info.ifIndex;
}

int get_available_ifTableIndex(int if_num)
{
    iftable_info_t info;
    u32            if_types = 0;

    if (if_num == 0xFFFFFFFF) {
        return IFTABLE_IFINDEX_END;
    }
    /* Since if_num must be a posotive number, so if_num must be convert to unsigne long */
    info.ifIndex = (ifIndex_id_t)if_num - 1;
    IFTABLE_IFINDEX_TYPE_FLAG_SET(if_types, IFTABLE_IFINDEX_TYPE_PORT);
    IFTABLE_IFINDEX_TYPE_FLAG_SET(if_types, IFTABLE_IFINDEX_TYPE_LLAG);
    IFTABLE_IFINDEX_TYPE_FLAG_SET(if_types, IFTABLE_IFINDEX_TYPE_GLAG);

    if (TRUE == ifIndex_get_next_by_type(&info, if_types)) {
        return info.ifIndex;
    }

    /* cann't find any valid interface, return the value of 'IFTABLE_IFINDEX_END' */
    return IFTABLE_IFINDEX_END;
}

BOOL get_ifTableIndex_info(int if_index, iftable_info_t *table_info_p)
{
    if (NULL == table_info_p) {
        return FALSE;
    }

    table_info_p->ifIndex = if_index;
    if (FALSE == ifIndex_get_valid(table_info_p)) {
        return FALSE;
    }

    return TRUE;
}

BOOL RFC1213_MIB2C_is_nvt_string(char *str)
{
    /*
    The NVT ASCII character set, as defined in pages 4, 10-11 of RFC 854 defines the use of
    character codes 0-127 (decimal), the graphics characters (32-126) are
    interpreted as US ASCII, NUL(0x00), LF(0x0A), CR(0x0D), BEL(0x07), BS(0x08), HT(0x09), VT(0x0B) and FF(0x0C) have the special
    meanings specified in RFC 854, the other 25 codes have no standard
    interpretation, the sequence 'CR LF' means newline, the sequence 'CR NUL' means
    carriage-return, an 'LF' not preceded by a 'CR' means moving to the same column
    on the next line.

                  - the sequence 'CR x' for any x other than LF or NUL is
                    illegal.  (Note that this also means that a string may
                    end with either 'CR LF' or 'CR NUL', but not with CR.)

    The expected outcome is for the agent is to return wrongValue.
    */

    int i, len = strlen(str);

    for (i = 0; i < len; i++) {
        T_R("Char:%u", (u8) *str);
        if ((*str < 32 && *str != 0x0 && *str != 0x7 && *str != 0x8 && *str != 0x9 && *str != 0xA && *str != 0xB && *str != 0xC && *str != 0xD) ||
            *str > 126 ||
            (*str == 0xD /* CR */ && *(str + 1) != 0x0 && *(str + 1) != 0xA /* LF */)) {
            return FALSE;
        }
        str++;
    }
    T_D("Valid:%s", str);
    return TRUE;
}

