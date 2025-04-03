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

#include "main.h"
#include "msg_api.h"
#include "critd_api.h"
#include "misc_api.h"
#include "ip_source_guard_api.h"
#include "ip_source_guard.h"

#include "vtss_avl_tree_api.h"
#include "port_iter.hxx"
#include "acl_api.h"
#include "dhcp_snooping_api.h"
#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif
#include "topo_api.h"   // For topo_usid2isid(), topo_isid2usid()
//#include <network.h>

#ifdef VTSS_SW_OPTION_ICFG
#include "ip_source_guard_icfg.h"
#endif

// For public header
#include "vtss_common_iterator.hxx"
//#include "ifIndex_api.h"

#include "ip_utils.hxx"

#include "ip_source_guard_serializer.hxx"
#include "vtss/appl/dhcp_snooping.h"
/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "ip_guard", "IP_SOURCE_GUARD"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define IP_SOURCE_GUARD_CRIT_ENTER() critd_enter(&ip_source_guard_global.crit, __FILE__, __LINE__)
#define IP_SOURCE_GUARD_CRIT_EXIT()  critd_exit( &ip_source_guard_global.crit, __FILE__, __LINE__)

#define IP_SOURCE_GUARD_MAC_LENGTH  6
#define IP_SOURCE_GUARD_BUF_LENGTH  40

/* Global structure */
static ip_source_guard_global_t ip_source_guard_global;

static i32 _ip_source_guard_entry_compare_func(void *elm1, void *elm2);
static i32 _ip_source_guard_dynamic_entry_compare_func(void *elm1, void *elm2);
static mesa_rc _ip_source_guard_mgmt_conf_get_static_entry(ip_source_guard_entry_t *entry);
static mesa_rc _ip_source_guard_mgmt_conf_update_static_entry(ip_source_guard_entry_t *entry);

VTSS_AVL_TREE(static_ip_source_entry_list_avlt, "IP_Source_Guard_static_avlt", VTSS_MODULE_ID_IP_SOURCE_GUARD, _ip_source_guard_entry_compare_func, IP_SOURCE_GUARD_MAX_ENTRY_CNT)
static  int     static_ip_source_entry_list_created_done = FALSE;

VTSS_AVL_TREE(dynamic_ip_source_entry_list_avlt, "IP_Source_Guard_dynamic_avlt", VTSS_MODULE_ID_IP_SOURCE_GUARD, _ip_source_guard_dynamic_entry_compare_func, IP_SOURCE_GUARD_MAX_ENTRY_CNT)
static  int     dynamic_ip_source_entry_list_created_done = FALSE;

/****************************************************************************/
/*  Various local functions with no semaphore protection                    */
/****************************************************************************/

static i32 _ip_source_guard_entry_compare_func(void *elm1, void *elm2)
{
    ip_source_guard_entry_t *element1, *element2;

    /* BODY
     */
    element1 = (ip_source_guard_entry_t *)elm1;
    element2 = (ip_source_guard_entry_t *)elm2;
    if (element1->isid > element2->isid) {
        return 1;
    } else if (element1->isid < element2->isid) {
        return -1;
    } else if (element1->port_no > element2->port_no) {
        return 1;
    } else if (element1->port_no < element2->port_no) {
        return -1;
    } else if (element1->vid > element2->vid) {
        return 1;
    } else if (element1->vid < element2->vid) {
        return -1;
    } else if (element1->assigned_ip > element2->assigned_ip) {
        return 1;
    } else if (element1->assigned_ip < element2->assigned_ip) {
        return -1;
    } else {
        return 0;
    }
}

static i32 _ip_source_guard_dynamic_entry_compare_func(void *elm1, void *elm2)
{
    ip_source_guard_entry_t *element1, *element2;

    /* BODY
     */
    element1 = (ip_source_guard_entry_t *)elm1;
    element2 = (ip_source_guard_entry_t *)elm2;
    if (element1->isid > element2->isid) {
        return 1;
    } else if (element1->isid < element2->isid) {
        return -1;
    } else if (element1->port_no > element2->port_no) {
        return 1;
    } else if (element1->port_no < element2->port_no) {
        return -1;
    } else if (element1->vid > element2->vid) {
        return 1;
    } else if (element1->vid < element2->vid) {
        return -1;
    } else if (element1->assigned_ip > element2->assigned_ip) {
        return 1;
    } else if (element1->assigned_ip < element2->assigned_ip) {
        return -1;
    } else {
        return 0;
    }
}

/* Add IP_SOURCE_GUARD static entry */
static mesa_rc _ip_source_guard_mgmt_conf_add_static_entry(ip_source_guard_entry_t *entry, BOOL allocated)
{
    ip_source_guard_entry_t *entry_p;
    mesa_rc                 rc = VTSS_RC_OK;
    int                     i;

    /* allocated memory */
    if (allocated) {
        if (_ip_source_guard_mgmt_conf_get_static_entry(entry) == VTSS_RC_OK) {
            if (_ip_source_guard_mgmt_conf_update_static_entry(entry) != VTSS_RC_OK) {
                T_W("_ip_source_guard_mgmt_conf_update_static_entry() failed");
            }
        } else {
            /* Find an unused entry */
            for (i = 0; i < IP_SOURCE_GUARD_MAX_ENTRY_CNT; i++) {
                if (ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry[i].valid) {
                    continue;
                }
                /* insert the entry on global memory for saving configuration */
                memcpy(&ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry[i], entry, sizeof(ip_source_guard_entry_t));
                entry_p = &(ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry[i]);

                /* add the entry into static DB */
                if (vtss_avl_tree_add(&static_ip_source_entry_list_avlt, entry_p) != TRUE) {
                    memset(&ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry[i], 0x0, sizeof(ip_source_guard_entry_t));
                    T_D("add the entry into static DB failed");
                    rc = IP_SOURCE_GUARD_ERROR_DATABASE_ADD;
                }

                break;
            }
        }
    } else {
        /* only insert the entry into link */
        if (vtss_avl_tree_add(&static_ip_source_entry_list_avlt, entry) != TRUE) {
            rc = IP_SOURCE_GUARD_ERROR_DATABASE_ADD;
        }
    }

    return rc;
}

/* Update IP_SOURCE_GUARD static entry */
static mesa_rc _ip_source_guard_mgmt_conf_update_static_entry(ip_source_guard_entry_t *entry)
{
    mesa_rc                 rc = VTSS_RC_OK;
    ip_source_guard_entry_t *entry_p;
    int                     i;

    /* Find the exist entry */
    for (i = 0; i < IP_SOURCE_GUARD_MAX_ENTRY_CNT; i++) {
        if (!ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry[i].valid) {
            continue;
        }

        entry_p = &(ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry[i]);
        T_N("i=%d, isid=%u , port_no=%u, vid=%u, assigned_ip=%u, ip_mask=%u", i, entry_p->isid, entry_p->port_no, entry_p->vid, entry_p->assigned_ip, entry_p->ip_mask);
        T_N("isid=%u , port_no=%u, vid=%u, assigned_ip=%u, ip_mask=%u", entry->isid, entry->port_no, entry->vid, entry->assigned_ip, entry->ip_mask);

        if (_ip_source_guard_entry_compare_func(&ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry[i], entry) == 0) {
            /* update the entry on global memory */
            memcpy(&ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry[i], entry, sizeof(ip_source_guard_entry_t));
            T_N("_ip_source_guard_mgmt_conf_update_static_entry success");
            break;
        }
    }

    return rc;
}

/* Delete IP_SOURCE_GUARD static entry */
static mesa_rc _ip_source_guard_mgmt_conf_del_static_entry(ip_source_guard_entry_t *entry, BOOL free_node)
{
    ip_source_guard_entry_t *entry_p;
    mesa_rc                 rc = VTSS_RC_OK;
    int                     i;

    if (free_node) {
        /* Find the exist entry */
        for (i = 0; i < IP_SOURCE_GUARD_MAX_ENTRY_CNT; i++) {
            if (!ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry[i].valid) {
                continue;
            }

            if (_ip_source_guard_entry_compare_func(&ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry[i], entry) == 0) {
                entry_p = &(ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry[i]);

                /* delete the entry on static DB */
                if (vtss_avl_tree_delete(&static_ip_source_entry_list_avlt, (void **) &entry_p) != TRUE) {
                    rc = IP_SOURCE_GUARD_ERROR_DATABASE_DEL;
                } else {
                    /* clear global cache memory */
                    memset(&ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry[i], 0x0, sizeof(ip_source_guard_entry_t));
                }

                break;
            }
        }
    } else {
        /* only delete the entry on link */
        if (vtss_avl_tree_delete(&static_ip_source_entry_list_avlt, (void **) &entry_p) != TRUE) {
            rc = IP_SOURCE_GUARD_ERROR_DATABASE_DEL;
        }
    }

    return rc;
}

/* Delete All IP_SOURCE_GUARD static entry */
static mesa_rc _ip_source_guard_mgmt_conf_del_all_static_entry(BOOL free_node)
{
    mesa_rc rc = VTSS_RC_OK;

    if (free_node) {
        vtss_avl_tree_destroy(&static_ip_source_entry_list_avlt);
        if (vtss_avl_tree_init(&static_ip_source_entry_list_avlt) != TRUE) {
            T_W("vtss_avl_tree_init() failed");
        }
        /* clear global cache memory */
        memset(ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry, 0x0, IP_SOURCE_GUARD_MAX_ENTRY_CNT * sizeof(ip_source_guard_entry_t));
    } else {
        vtss_avl_tree_destroy(&static_ip_source_entry_list_avlt);
        if (vtss_avl_tree_init(&static_ip_source_entry_list_avlt) != TRUE) {
            T_W("vtss_avl_tree_init() failed");
        }
    }

    return rc;
}

/* Get IP_SOURCE_GUARD static entry */
static mesa_rc _ip_source_guard_mgmt_conf_get_static_entry(ip_source_guard_entry_t *entry)
{
    ip_source_guard_entry_t *entry_p;
    mesa_rc                 rc = VTSS_RC_OK;

    entry_p = entry;
    if (vtss_avl_tree_get(&static_ip_source_entry_list_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET) != TRUE) {
        rc = IP_SOURCE_GUARD_ERROR_DATABASE_ACCESS;
    } else {
        memcpy(entry, entry_p, sizeof(ip_source_guard_entry_t));
    }

    return rc;
}

/* Get First IP_SOURCE_GUARD static entry */
static mesa_rc _ip_source_guard_mgmt_conf_get_first_static_entry(ip_source_guard_entry_t *entry)
{
    ip_source_guard_entry_t *entry_p;
    mesa_rc                 rc = VTSS_RC_OK;

    entry_p = entry;
    if (vtss_avl_tree_get(&static_ip_source_entry_list_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET_FIRST) != TRUE) {
        rc = IP_SOURCE_GUARD_ERROR_DATABASE_ACCESS;
    } else {
        memcpy(entry, entry_p, sizeof(ip_source_guard_entry_t));
    }

    return rc;
}

/* Get Next IP_SOURCE_GUARD static entry */
static mesa_rc _ip_source_guard_mgmt_conf_get_next_static_entry(ip_source_guard_entry_t *entry)
{
    ip_source_guard_entry_t *entry_p;
    mesa_rc                 rc = VTSS_RC_OK;

    entry_p = entry;
    if (vtss_avl_tree_get(&static_ip_source_entry_list_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET_NEXT) != TRUE) {
        rc = IP_SOURCE_GUARD_ERROR_DATABASE_ACCESS;
    } else {
        memcpy(entry, entry_p, sizeof(ip_source_guard_entry_t));
    }

    return rc;
}

/* Create IP_SOURCE_GUARD static data base */
static mesa_rc _ip_source_guard_mgmt_conf_create_static_db(void)
{
    mesa_rc rc = VTSS_RC_OK;

    if (!static_ip_source_entry_list_created_done) {
        /* Create data base for storing static entries */
        if (vtss_avl_tree_init(&static_ip_source_entry_list_avlt)) {
            static_ip_source_entry_list_created_done = TRUE;
        } else {
            T_W("vtss_avl_tree_init() failed");
            rc = IP_SOURCE_GUARD_ERROR_DATABASE_CREATE;
        }
    }

    return rc;
}

/* Add IP_SOURCE_GUARD dynamic entry */
static mesa_rc _ip_source_guard_mgmt_conf_add_dynamic_entry(ip_source_guard_entry_t *entry, BOOL allocated)
{
    ip_source_guard_entry_t *entry_p;
    mesa_rc                 rc = VTSS_RC_OK;
    int                     i;

    /* allocated memory */
    if (allocated) {
        /* Find an unused entry */
        for (i = 0; i < IP_SOURCE_GUARD_MAX_ENTRY_CNT; i++) {
            if (ip_source_guard_global.ip_source_guard_dynamic_entry[i].valid) {
                continue;
            }
            /* insert the entry on global memory */
            memcpy(&ip_source_guard_global.ip_source_guard_dynamic_entry[i], entry, sizeof(ip_source_guard_entry_t));
            entry_p = &(ip_source_guard_global.ip_source_guard_dynamic_entry[i]);

            /* add the entry into dynamic DB */
            if (vtss_avl_tree_add(&dynamic_ip_source_entry_list_avlt, entry_p) != TRUE) {
                memset(&ip_source_guard_global.ip_source_guard_dynamic_entry[i], 0x0, sizeof(ip_source_guard_entry_t));
                T_D("add the entry into dynamic DB failed");
                rc = IP_SOURCE_GUARD_ERROR_DATABASE_ADD;
            }

            break;
        }
    } else {
        /* only insert the entry into link */
        if (vtss_avl_tree_add(&dynamic_ip_source_entry_list_avlt, entry) != TRUE) {
            rc = IP_SOURCE_GUARD_ERROR_DATABASE_ADD;
        }
    }

    return rc;
}

/* Delete IP_SOURCE_GUARD dynamic entry */
static mesa_rc _ip_source_guard_mgmt_conf_del_dynamic_entry(ip_source_guard_entry_t *entry, BOOL free_node)
{
    ip_source_guard_entry_t *entry_p;
    mesa_rc                 rc = VTSS_RC_OK;
    int                     i;

    if (free_node) {
        /* Find the exist entry */
        for (i = 0; i < IP_SOURCE_GUARD_MAX_ENTRY_CNT; i++) {
            if (!ip_source_guard_global.ip_source_guard_dynamic_entry[i].valid) {
                continue;
            }

            if (_ip_source_guard_dynamic_entry_compare_func(&ip_source_guard_global.ip_source_guard_dynamic_entry[i], entry) == 0) {
                entry_p = &(ip_source_guard_global.ip_source_guard_dynamic_entry[i]);

                /* delete the entry on dynamic DB */
                if (vtss_avl_tree_delete(&dynamic_ip_source_entry_list_avlt, (void **) &entry_p) != TRUE) {
                    rc = IP_SOURCE_GUARD_ERROR_DATABASE_DEL;
                } else {
                    /* clear global cache memory */
                    memset(&ip_source_guard_global.ip_source_guard_dynamic_entry[i], 0x0, sizeof(ip_source_guard_entry_t));
                }

                break;
            }
        }
    } else {
        /* only delete the entry on link */
        if (vtss_avl_tree_delete(&dynamic_ip_source_entry_list_avlt, (void **) &entry_p) != TRUE) {
            rc = IP_SOURCE_GUARD_ERROR_DATABASE_DEL;
        }
    }

    return rc;
}

/* Delete All IP_SOURCE_GUARD dynamic entry */
static mesa_rc _ip_source_guard_mgmt_conf_del_all_dynamic_entry(BOOL free_node)
{
    mesa_rc rc = VTSS_RC_OK;

    if (free_node) {
        vtss_avl_tree_destroy(&dynamic_ip_source_entry_list_avlt);
        if (vtss_avl_tree_init(&dynamic_ip_source_entry_list_avlt) != TRUE) {
            T_W("vtss_avl_tree_init() failed");
        }
        /* clear global cache memory */
        memset(ip_source_guard_global.ip_source_guard_dynamic_entry, 0x0, IP_SOURCE_GUARD_MAX_ENTRY_CNT * sizeof(ip_source_guard_entry_t));
    } else {
        vtss_avl_tree_destroy(&dynamic_ip_source_entry_list_avlt);
        if (vtss_avl_tree_init(&dynamic_ip_source_entry_list_avlt) != TRUE) {
            T_W("vtss_avl_tree_init() failed");
        }
    }

    return rc;
}

/* Get IP_SOURCE_GUARD dynamic entry */
static mesa_rc _ip_source_guard_mgmt_conf_get_dynamic_entry(ip_source_guard_entry_t *entry)
{
    ip_source_guard_entry_t *entry_p;
    mesa_rc                 rc = VTSS_RC_OK;

    entry_p = entry;
    if (vtss_avl_tree_get(&dynamic_ip_source_entry_list_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET) != TRUE) {
        rc = IP_SOURCE_GUARD_ERROR_DATABASE_ACCESS;
    } else {
        memcpy(entry, entry_p, sizeof(ip_source_guard_entry_t));
    }

    return rc;
}

/* Get First IP_SOURCE_GUARD dynamic entry */
static mesa_rc _ip_source_guard_mgmt_conf_get_first_dynamic_entry(ip_source_guard_entry_t *entry)
{
    ip_source_guard_entry_t *entry_p;
    mesa_rc                 rc = VTSS_RC_OK;

    entry_p = entry;
    if (vtss_avl_tree_get(&dynamic_ip_source_entry_list_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET_FIRST) != TRUE) {
        rc = IP_SOURCE_GUARD_ERROR_DATABASE_ACCESS;
    } else {
        memcpy(entry, entry_p, sizeof(ip_source_guard_entry_t));
    }

    return rc;
}

/* Get Next IP_SOURCE_GUARD dynamic entry */
static mesa_rc _ip_source_guard_mgmt_conf_get_next_dynamic_entry(ip_source_guard_entry_t *entry)
{
    ip_source_guard_entry_t *entry_p;
    mesa_rc                 rc = VTSS_RC_OK;

    entry_p = entry;
    if (vtss_avl_tree_get(&dynamic_ip_source_entry_list_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET_NEXT) != TRUE) {
        rc = IP_SOURCE_GUARD_ERROR_DATABASE_ACCESS;
    } else {
        memcpy(entry, entry_p, sizeof(ip_source_guard_entry_t));
    }

    return rc;
}

/* Create IP_SOURCE_GUARD dynamic data base */
static mesa_rc _ip_source_guard_mgmt_conf_create_dynamic_db(void)
{
    mesa_rc rc = VTSS_RC_OK;

    if (!dynamic_ip_source_entry_list_created_done) {
        /* Create data base for storing static entries */
        if (vtss_avl_tree_init(&dynamic_ip_source_entry_list_avlt)) {
            dynamic_ip_source_entry_list_created_done = TRUE;
        } else {
            T_W("vtss_avl_tree_init() failed");
            rc = IP_SOURCE_GUARD_ERROR_DATABASE_CREATE;
        }
    }

    return rc;
}

/* Translate IP_SOURCE_GUARD dynamic entries into static entries */
static mesa_rc _ip_source_guard_mgmt_conf_translate_dynamic_entry_into_static_entry(ip_source_guard_entry_t *entry)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter");

    /* avoid inserting the null ace id on static db */
    if (entry->ace_id == ACL_MGMT_ACE_ID_NONE) {
        T_W("ace id is empty");
        return IP_SOURCE_GUARD_ERROR_INV_PARAM;
    }

    /* add the entry into static DB */
    if ((rc = _ip_source_guard_mgmt_conf_add_static_entry(entry, TRUE)) != VTSS_RC_OK) {
        T_D("_ip_source_guard_mgmt_conf_add_static_entry() failed");
        T_D("exit");
        return rc;
    } else {
        /* delete the entry on dynamic DB */
        if ((rc = _ip_source_guard_mgmt_conf_del_dynamic_entry(entry, TRUE)) != VTSS_RC_OK) {
            T_D("_ip_source_guard_mgmt_conf_del_dynamic_entry() failed");
        }
    }

    T_D("exit");
    return rc;
}

/* Save IP_SOURCE_GUARD configuration */
static mesa_rc _ip_source_guard_mgmt_conf_save_static_configuration(void)
{
    mesa_rc                     rc = VTSS_RC_OK;

    T_D("exit");
    return rc;
}

/* IP_SOURCE_GUARD entry count */
static mesa_rc _ip_source_guard_entry_count(void)
{
    mesa_rc rc = VTSS_RC_OK;
    int     i;

    /* Find the exist entry on static DB */
    for (i = 0; i < IP_SOURCE_GUARD_MAX_ENTRY_CNT; i++) {
        if (!ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry[i].valid) {
            continue;
        }
        rc++;
    }

    /* Find the exist entry on dynamic DB */
    for (i = 0; i < IP_SOURCE_GUARD_MAX_ENTRY_CNT; i++) {
        if (!ip_source_guard_global.ip_source_guard_dynamic_entry[i].valid) {
            continue;
        }
        rc++;
    }

    return rc;
}

/* Get configuration from per port dynamic entry count */
static mesa_rc _ip_source_guard_mgmt_conf_get_port_dynamic_entry_cnt(vtss_isid_t isid, ip_source_guard_port_dynamic_entry_conf_t *port_dynamic_entry_conf)
{
    mesa_rc         rc = VTSS_RC_OK;
    port_iter_t     pit;

    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        port_dynamic_entry_conf->entry_cnt[pit.iport] = ip_source_guard_global.ip_source_guard_conf.port_dynamic_entry_conf[isid - VTSS_ISID_START].entry_cnt[pit.iport];
    }

    return rc;
}

/* Get current port dynamic entry count */
static mesa_rc _ip_source_guard_mgmt_conf_get_current_port_dynamic_entry_cnt(vtss_isid_t isid, mesa_port_no_t port_no, ulong *current_port_dynamic_entry_cnt_p)
{
    ip_source_guard_entry_t entry;
    ulong                   current_port_entry_cnt = 0;

    if (_ip_source_guard_mgmt_conf_get_first_dynamic_entry(&entry) == VTSS_RC_OK) {
        if ((entry.isid == isid) && (entry.port_no == port_no)) {
            current_port_entry_cnt++;
        }
        while (_ip_source_guard_mgmt_conf_get_next_dynamic_entry(&entry) == VTSS_RC_OK) {
            if ((entry.isid == isid) && (entry.port_no == port_no)) {
                current_port_entry_cnt++;
            }
        }
    }
    *current_port_dynamic_entry_cnt_p = current_port_entry_cnt;

    return VTSS_RC_OK;
}

/* check is reach IP_SOURCE_GUARD max port dynamic entry count */
static BOOL _ip_source_guard_is_reach_max_port_dynamic_entry_cnt(ip_source_guard_entry_t *entry)
{
    ip_source_guard_port_dynamic_entry_conf_t   port_dynamic_entry_conf;
    ulong                                       max_dynamic_entry_cnt, current_port_dynamic_entry_cnt = 0;
#ifdef VTSS_SW_OPTION_SYSLOG
    char ip_txt[IP_SOURCE_GUARD_BUF_LENGTH], mac_txt[IP_SOURCE_GUARD_BUF_LENGTH];
    char syslog_txt[512], *syslog_txt_p;
#endif /* VTSS_SW_OPTION_SYSLOG */

    max_dynamic_entry_cnt = _ip_source_guard_entry_count();

    if (max_dynamic_entry_cnt >= IP_SOURCE_GUARD_MAX_ENTRY_CNT) {
#ifdef VTSS_SW_OPTION_SYSLOG
        syslog_txt_p = &syslog_txt[0];
        syslog_txt_p += sprintf(syslog_txt_p, "IP_SOURCE_GUARD-TABLE_FULL: Dynamic IP Source Guard Table full. Could not add entry <VLAN ID = %d, ", entry->vid);
        syslog_txt_p += sprintf(syslog_txt_p, " Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
        syslog_txt_p += sprintf(syslog_txt_p, ", IP Address = %s, MAC Address = %s> to table",
                                misc_ipv4_txt(entry->assigned_ip, ip_txt),
                                misc_mac_txt(entry->assigned_mac, mac_txt));
        S_PORT_N(entry->isid, entry->port_no, "%s", syslog_txt);
#endif /* VTSS_SW_OPTION_SYSLOG */
        return TRUE;
    }

    if (_ip_source_guard_mgmt_conf_get_port_dynamic_entry_cnt(entry->isid, &port_dynamic_entry_conf) != VTSS_RC_OK ||
        _ip_source_guard_mgmt_conf_get_current_port_dynamic_entry_cnt(entry->isid, entry->port_no, &current_port_dynamic_entry_cnt) != VTSS_RC_OK) {
        return FALSE;
    }
    if (current_port_dynamic_entry_cnt == port_dynamic_entry_conf.entry_cnt[entry->port_no]) {
#ifdef VTSS_SW_OPTION_SYSLOG
        syslog_txt_p = &syslog_txt[0];
        syslog_txt_p += sprintf(syslog_txt_p, "IP_SOURCE_GUARD-TABLE_FULL: Dynamic IP Source Guard Table reach port limitation(" VPRIlu"). Could not add entry <VLAN ID = %d, ", current_port_dynamic_entry_cnt, entry->vid);
        syslog_txt_p += sprintf(syslog_txt_p, " Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
        syslog_txt_p += sprintf(syslog_txt_p, ", IP Address = %s, MAC Address = %s> to table",
                                misc_ipv4_txt(entry->assigned_ip, ip_txt),
                                misc_mac_txt(entry->assigned_mac, mac_txt));
        S_PORT_N(entry->isid, entry->port_no, "%s", syslog_txt);
#endif /* VTSS_SW_OPTION_SYSLOG */
        return TRUE;
    }

    return FALSE;
}

/*
 * Get port number from interface index.
 */
static mesa_port_no_t get_port_from_ifindex(vtss_ifindex_t ifindex, vtss_isid_t *isid = nullptr)
{
    vtss_ifindex_elm_t ife;

    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return MESA_PORT_NO_NONE;
    }

    if (isid) {
        *isid = ife.isid;
    }
    return ife.ordinal;
}

/*
 * Gets entry info from snooping module and adds to source guard if applicable.
 */
static mesa_rc ip_source_guard_get_snooping_entries(const ip_source_guard_entry_t *entry,
                                                    mesa_bool_t check_for_entry)
{
    mesa_port_no_t curr_port_no;
    vtss_isid_t isid = 0;
    mesa_mac_t *prev_mac = nullptr;
    mesa_mac_t next_mac;
    mesa_vid_t *prev_vid = nullptr;
    mesa_vid_t next_vid;
    vtss_appl_dhcp_snooping_assigned_ip_t   assigned_ip;

    T_D("Enter");

    while (vtss_appl_dhcp_snooping_assigned_ip_itr(
               prev_mac, &next_mac, prev_vid, &next_vid) == VTSS_RC_OK) {

        if (vtss_appl_dhcp_snooping_assigned_ip_get(next_mac, next_vid, &assigned_ip) != VTSS_RC_OK) {
            T_D("Could not get client info from snooping module, exit");
            return IP_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }

        /* Check if the current entry should be added. */
        curr_port_no = get_port_from_ifindex(assigned_ip.ifIndex, &isid);
        if (curr_port_no != entry->port_no) {
            prev_mac = &next_mac;
            prev_vid = &next_vid;
            T_D("Not the correct port.");
            continue;
        }

        /* A check in case we are searching for one, particular entry.*/
        if (check_for_entry) {
            vtss_ifindex_t port_ifindex;
            vtss_ifindex_from_port(entry->isid, entry->port_no, &port_ifindex);

            if (assigned_ip.ifIndex != port_ifindex || assigned_ip.ipAddr != entry->assigned_ip || next_vid != entry->vid) {
                prev_mac = &next_mac;
                prev_vid = &next_vid;
                T_D("Not the correct entry.");
                continue;
            }
        }

        ip_source_guard_entry_t new_entry;
        memset(&new_entry, 0x0, sizeof(new_entry));
        new_entry.vid = next_vid;
        memcpy(new_entry.assigned_mac, next_mac.addr, IP_SOURCE_GUARD_MAC_LENGTH);
        new_entry.assigned_ip = assigned_ip.ipAddr;
        new_entry.ip_mask = 0xFFFFFFFF;
        new_entry.isid = isid;
        new_entry.port_no = entry->port_no;
        new_entry.type = IP_SOURCE_GUARD_DYNAMIC_TYPE;

        new_entry.valid = TRUE;
        if (ip_source_guard_mgmt_conf_set_dynamic_entry(&new_entry)) {
            T_D("ip_source_guard_mgmt_conf_set_dynamic_entry() failed");
        }

        /* If only searching for a specific entry, now it has been added and we can return. */
        if (check_for_entry) {
            break;
        }

        prev_mac = &next_mac;
        prev_vid = &next_vid;
    }

    T_D("exit");
    return VTSS_RC_OK;
}
/*
 * Helper function for checking if a certain entry needs to be added to dynamic table.
 */
static mesa_rc ip_source_guard_check_snooping_entries(const ip_source_guard_entry_t *entry)
{
    T_D("Enter");
    return ip_source_guard_get_snooping_entries(entry, true);
}


/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/

/* Set IP_SOURCE_GUARD defaults */
static void _ip_source_guard_default_set(ip_source_guard_conf_t *conf)
{
    int isid, j;

    vtss_clear(*conf);
    conf->mode = IP_SOURCE_GUARD_DEFAULT_MODE;

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        for (j = VTSS_PORT_NO_START; j < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); j++) {
            conf->port_mode_conf[isid - VTSS_ISID_START].mode[j] = IP_SOURCE_GUARD_DEFAULT_PORT_MODE;
            conf->port_dynamic_entry_conf[isid - VTSS_ISID_START].entry_cnt[j] = IP_SOURCE_GUARD_DEFAULT_DYNAMIC_ENTRY_CNT;
        }
    }

    /* clear global cache memory */
    memset(conf->ip_source_guard_static_entry, 0x0, IP_SOURCE_GUARD_MAX_ENTRY_CNT * sizeof(ip_source_guard_entry_t));
    T_D("clear static entries on global cache memory");

    return;
}

/* Set IP_SOURCE_GUARD defaults for dynamic entries */
static void _ip_source_guard_default_set_dynamic_entries(void)
{
    /* clear global cache memory */
    memset(ip_source_guard_global.ip_source_guard_dynamic_entry, 0x0, IP_SOURCE_GUARD_MAX_ENTRY_CNT * sizeof(ip_source_guard_entry_t));
    T_D("clear dynamic entries on global cache memory");

    return;
}

/****************************************************************************/
/*  Reserved ACEs functions                                                 */
/****************************************************************************/
/* Add port default ACE for IP source guard.
   Add a port rule for deny all frames to the end of ACL */
static mesa_rc _ip_source_guard_default_ace_add(vtss_isid_t isid, mesa_port_no_t port_no)
{
    mesa_rc             rc;
    acl_entry_conf_t    conf;

    /* We use the new parameter of portlist on ACL V2 */
    if (acl_mgmt_ace_get(ACL_USER_IP_SOURCE_GUARD, IP_SOURCE_GUARD_DEFAULT_ACE_ID(isid, VTSS_PORT_NO_START), &conf, NULL, FALSE) == VTSS_RC_OK) {
        conf.port_list[port_no] = TRUE;
        return acl_mgmt_ace_add(ACL_USER_IP_SOURCE_GUARD, ACL_MGMT_ACE_ID_NONE, &conf);
    }

    if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_IPV4, &conf)) != VTSS_RC_OK) {
        return rc;
    }
    conf.isid                       = VTSS_ISID_LOCAL;
    conf.id                         = IP_SOURCE_GUARD_DEFAULT_ACE_ID(isid, VTSS_PORT_NO_START);
    conf.action.port_action         = MESA_ACL_PORT_ACTION_FILTER;
    memset(conf.action.port_list, 0, sizeof(conf.action.port_list));
    memset(conf.port_list, 0, sizeof(conf.port_list));
    conf.port_list[port_no]         = TRUE;
    return acl_mgmt_ace_add(ACL_USER_IP_SOURCE_GUARD, ACL_MGMT_ACE_ID_NONE, &conf);
}

static mesa_rc _ip_source_guard_default_ace_del(vtss_isid_t isid, mesa_port_no_t port_no)
{
    acl_entry_conf_t    conf;
    port_iter_t         pit;

    if (acl_mgmt_ace_get(ACL_USER_IP_SOURCE_GUARD, IP_SOURCE_GUARD_DEFAULT_ACE_ID(isid, VTSS_PORT_NO_START), &conf, NULL, FALSE) == VTSS_RC_OK) {
        int            port_cnt = 0;

        conf.port_list[port_no] = FALSE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (conf.port_list[pit.iport] == TRUE) {
                port_cnt++;
            }
        }
        if (port_cnt) {
            return acl_mgmt_ace_add(ACL_USER_IP_SOURCE_GUARD, ACL_MGMT_ACE_ID_NONE, &conf);
        } else {
            return (acl_mgmt_ace_del(ACL_USER_IP_SOURCE_GUARD, IP_SOURCE_GUARD_DEFAULT_ACE_ID(isid, VTSS_PORT_NO_START)));
        }
    }
    return VTSS_RC_OK;
}

/* Alloc IP source guard ACE ID */
static BOOL _ip_source_guard_ace_alloc(mesa_ace_id_t *id)
{
    int i, found = FALSE;

    /* Get next available ID */
    for (i = VTSS_ISID_CNT * fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) + 1; i <= ACL_MGMT_ACE_ID_END; i++) {
        if (!ip_source_guard_global.id_used[i - ACL_MGMT_ACE_ID_START]) {
            found = TRUE;
            *id = (mesa_ace_id_t) i;
            ip_source_guard_global.id_used[i - ACL_MGMT_ACE_ID_START] = TRUE;
            break;
        }
    }

    if (found) {
        return TRUE;
    } else {
        T_W("ACE Auto-assigned fail");
        return FALSE;
    }
}

/* Free IP source guard ACE ID
   Free all if id = ACL_MGMT_ACE_ID_NONE */
static void _ip_source_guard_ace_free(mesa_ace_id_t id)
{
    int i;

    if (id == ACL_MGMT_ACE_ID_NONE) {
        for (i = 0; i < ACL_MGMT_ACE_ID_END; i++) {
            ip_source_guard_global.id_used[i] = FALSE;
        }
    } else {
        ip_source_guard_global.id_used[id - ACL_MGMT_ACE_ID_START] = FALSE;
    }
}

/* Add reserved ACE */
static mesa_rc _ip_source_guard_ace_add(ip_source_guard_entry_t *entry)
{
    acl_entry_conf_t    conf;
    mesa_rc             rc;
    int                 alloc_flag = FALSE;

    /* Check switch ID */
    if (!msg_switch_configurable(entry->isid)) {
        T_D("isid: %d not exist", entry->isid);
        T_D("exit");
        return IP_SOURCE_GUARD_ERROR_ISID;
    }

    if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_IPV4, &conf)) != VTSS_RC_OK) {
        return rc;
    }
    if (entry->ace_id == ACL_MGMT_ACE_ID_NONE) {
        if (!_ip_source_guard_ace_alloc(&conf.id)) {
            return IP_SOURCE_GUARD_ERROR_ACE_AUTO_ASSIGNED_FAIL;
        }
        alloc_flag = TRUE;
    } else {
        conf.id = entry->ace_id;
    }
    conf.isid                       = VTSS_ISID_LOCAL;
    memset(conf.port_list, 0, sizeof(conf.port_list));
    conf.port_list[entry->port_no]  = TRUE;
    conf.vid.mask                   = 0xFFF;
    conf.vid.value                  = entry->vid;
    conf.frame.ipv4.sip_smac.enable = TRUE;
    conf.frame.ipv4.sip_smac.sip = entry->assigned_ip;
    memcpy(conf.frame.ipv4.sip_smac.smac.addr, entry->assigned_mac, IP_SOURCE_GUARD_MAC_LENGTH);
    /* We always use parameter of 'VTSS_PORT_NO_START' to get the port's default ACE */
    if ((rc = acl_mgmt_ace_add(ACL_USER_IP_SOURCE_GUARD, IP_SOURCE_GUARD_DEFAULT_ACE_ID(entry->isid, VTSS_PORT_NO_START), &conf)) == VTSS_RC_OK) {
        entry->ace_id = conf.id;
    } else {
        entry->ace_id = ACL_MGMT_ACE_ID_NONE;
        if (alloc_flag) {
            _ip_source_guard_ace_free(conf.id);
        }
    }
    return rc;
}

/* Delete reserved ACE */
static mesa_rc _ip_source_guard_ace_del(ip_source_guard_entry_t *entry)
{
    mesa_rc rc = acl_mgmt_ace_del(ACL_USER_IP_SOURCE_GUARD, entry->ace_id);

    T_N("ace_id = %u", entry->ace_id);

    if (rc == VTSS_RC_OK) {
        _ip_source_guard_ace_free(entry->ace_id);
        entry->ace_id = ACL_MGMT_ACE_ID_NONE;
        T_N("ACL_MGMT_ACE_ID_NONE");
    }

    return rc;
}

/* Clear all ACEs of IP source guard */
static void _ip_source_guard_ace_clear(void)
{
    acl_entry_conf_t ace_conf;

    _ip_source_guard_ace_free(ACL_MGMT_ACE_ID_NONE);

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return;
    }

    while (acl_mgmt_ace_get(ACL_USER_IP_SOURCE_GUARD, ACL_MGMT_ACE_ID_NONE, &ace_conf, NULL, 1) == VTSS_RC_OK) {
        if (acl_mgmt_ace_del(ACL_USER_IP_SOURCE_GUARD, ace_conf.id)) {
            T_W("acl_mgmt_ace_del(ACL_USER_IP_SOURCE_GUARD, %d) failed", ace_conf.id);
        }
    }
}

/****************************************************************************/
/*  Dynamic entry functions                                                 */
/****************************************************************************/
/* Get first IP_SOURCE_GUARD dynamic entry */
mesa_rc ip_source_guard_mgmt_conf_get_first_dynamic_entry(ip_source_guard_entry_t *entry)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();
    rc = _ip_source_guard_mgmt_conf_get_first_dynamic_entry(entry);
    IP_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Get IP_SOURCE_GUARD dynamic entry */
mesa_rc ip_source_guard_mgmt_conf_get_dynamic_entry(ip_source_guard_entry_t *entry)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();
    rc = _ip_source_guard_mgmt_conf_get_dynamic_entry(entry);
    IP_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Get Next IP_SOURCE_GUARD dynamic entry */
mesa_rc ip_source_guard_mgmt_conf_get_next_dynamic_entry(ip_source_guard_entry_t *entry)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();
    rc = _ip_source_guard_mgmt_conf_get_next_dynamic_entry(entry);
    IP_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Set IP_SOURCE_GUARD dynamic entry */
mesa_rc ip_source_guard_mgmt_conf_set_dynamic_entry(ip_source_guard_entry_t *entry)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check switch ID */
    if (!msg_switch_exists(entry->isid)) {
        T_D("isid: %d not exist", entry->isid);
        T_D("exit");
        return IP_SOURCE_GUARD_ERROR_ISID;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();

    /* Check system mode is enabled */
    if (ip_source_guard_global.ip_source_guard_conf.mode == IP_SOURCE_GUARD_MGMT_DISABLED) {
        IP_SOURCE_GUARD_CRIT_EXIT();
        T_D("exit");
        return rc;
    }

    /* Check port mode is enabled */
    if (ip_source_guard_global.ip_source_guard_conf.port_mode_conf[entry->isid - VTSS_ISID_START].mode[entry->port_no] == IP_SOURCE_GUARD_MGMT_DISABLED) {
        IP_SOURCE_GUARD_CRIT_EXIT();
        T_D("exit");
        return rc;
    }

    /* Check the entry exist or not ? */
    if ((rc = _ip_source_guard_mgmt_conf_get_dynamic_entry(entry)) != IP_SOURCE_GUARD_ERROR_DATABASE_ACCESS) {
        //if existing, return the event
        T_D("the entry existing on dynamic db, exit, rc=%d", rc);
        IP_SOURCE_GUARD_CRIT_EXIT();
        T_D("exit");
        return IP_SOURCE_GUARD_ERROR_ENTRY_EXIST_ON_DB;
    }
    if ((rc = _ip_source_guard_mgmt_conf_get_static_entry(entry)) != IP_SOURCE_GUARD_ERROR_DATABASE_ACCESS) {
        //if existing, return the event
        T_D("the entry existing on static db, exit, rc=%d", rc);
        IP_SOURCE_GUARD_CRIT_EXIT();
        T_D("exit");
        return IP_SOURCE_GUARD_ERROR_ENTRY_EXIST_ON_DB;
    }

    /* Check total count reach the max value or not? */
    if (_ip_source_guard_is_reach_max_port_dynamic_entry_cnt(entry)) {
        IP_SOURCE_GUARD_CRIT_EXIT();
        T_D("exit");
        T_W("ip source guard: port entry full or table full.");
        return IP_SOURCE_GUARD_ERROR_DYNAMIC_TABLE_FULL;
    }

    entry->valid = TRUE;
    entry->ace_id = ACL_MGMT_ACE_ID_NONE;
    entry->type = IP_SOURCE_GUARD_DYNAMIC_TYPE;

    /* add the entry on ACL */
    if ((rc = _ip_source_guard_ace_add(entry)) != VTSS_RC_OK) {
        IP_SOURCE_GUARD_CRIT_EXIT();
        T_W("_ip_source_guard_ace_add() failed");
        T_D("exit");
        return rc;
    } else {
        /* add the entry into dynamic DB */
        if ((rc = _ip_source_guard_mgmt_conf_add_dynamic_entry(entry, TRUE)) != VTSS_RC_OK) {

            /* check ACL inserted or not  */
            if (entry->ace_id != ACL_MGMT_ACE_ID_NONE) {
                /* delete the ace entry */
                if (_ip_source_guard_ace_del(entry) != VTSS_RC_OK) {
                    T_D("_ip_source_guard_ace_del() failed");
                }
            }
        }
    }

    IP_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Delete IP_SOURCE_GUARD dynamic entry */
mesa_rc ip_source_guard_mgmt_conf_del_dynamic_entry(ip_source_guard_entry_t *entry)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check switch ID */
    if (!msg_switch_exists(entry->isid)) {
        T_D("isid: %d not exist", entry->isid);
        T_D("exit");
        return IP_SOURCE_GUARD_ERROR_ISID;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();

    /* Check system mode is enabled */
    if (ip_source_guard_global.ip_source_guard_conf.mode == IP_SOURCE_GUARD_MGMT_DISABLED) {
        IP_SOURCE_GUARD_CRIT_EXIT();
        return rc;
    }

    /* Check port mode is enabled */
    if (ip_source_guard_global.ip_source_guard_conf.port_mode_conf[entry->isid - VTSS_ISID_START].mode[entry->port_no] == IP_SOURCE_GUARD_MGMT_DISABLED) {
        IP_SOURCE_GUARD_CRIT_EXIT();
        return rc;
    }

    /* Check the entry exist or not ? */
    if ((rc = _ip_source_guard_mgmt_conf_get_dynamic_entry(entry)) == IP_SOURCE_GUARD_ERROR_DATABASE_ACCESS) {
        //if not existing, return
        IP_SOURCE_GUARD_CRIT_EXIT();
        T_D("exit");
        return rc;
    }

    /* check ACL inserted or not */
    if (entry->valid && entry->ace_id != ACL_MGMT_ACE_ID_NONE) {
        /* delete the entry on ACL */
        if (_ip_source_guard_ace_del(entry)) {
            T_D("_ip_source_guard_ace_del() failed");
        }
    }

    /* delete the entry on dynamic DB */
    if ((rc = _ip_source_guard_mgmt_conf_del_dynamic_entry(entry, TRUE)) != VTSS_RC_OK) {
        T_D("_ip_source_guard_mgmt_conf_del_dynamic_entry() failed");
    }

    IP_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* flush IP_SOURCE_GUARD dynamic entry by port */
static mesa_rc ip_source_guard_mgmt_conf_flush_dynamic_entry_by_port(vtss_isid_t isid, mesa_port_no_t port_no)
{
    mesa_rc                 rc = VTSS_RC_OK;
    ip_source_guard_entry_t entry;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();
    if (_ip_source_guard_mgmt_conf_get_first_dynamic_entry(&entry) == VTSS_RC_OK) {
        if (entry.isid == isid && entry.port_no == port_no) {
            if (entry.valid && entry.ace_id != ACL_MGMT_ACE_ID_NONE) {
                if (_ip_source_guard_ace_del(&entry)) {
                    T_D("_ip_source_guard_ace_del() failed");
                }
            }

            /* delete the entry on dynamic DB */
            if ((rc = _ip_source_guard_mgmt_conf_del_dynamic_entry(&entry, TRUE)) != VTSS_RC_OK) {
                T_D("_ip_source_guard_mgmt_conf_del_dynamic_entry() failed");
            }
        }
        while (_ip_source_guard_mgmt_conf_get_next_dynamic_entry(&entry) == VTSS_RC_OK) {
            if (entry.isid == isid && entry.port_no == port_no) {
                if (entry.valid && entry.ace_id != ACL_MGMT_ACE_ID_NONE) {
                    if (_ip_source_guard_ace_del(&entry)) {
                        T_D("_ip_source_guard_ace_del() failed");
                    }
                }

                /* delete the entry on dynamic DB */
                if ((rc = _ip_source_guard_mgmt_conf_del_dynamic_entry(&entry, TRUE)) != VTSS_RC_OK) {
                    T_D("_ip_source_guard_mgmt_conf_del_dynamic_entry() failed");
                }
            }
        }
    }
    IP_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* del first dynamic entry with port id */
static mesa_rc ip_source_guard_del_first_dynamic_entry_with_specific_port(vtss_isid_t isid, mesa_port_no_t port_no)
{
    ip_source_guard_entry_t     entry;
    int                         del_done = FALSE;

#ifdef VTSS_SW_OPTION_SYSLOG
    char ip_txt[IP_SOURCE_GUARD_BUF_LENGTH], mac_txt[IP_SOURCE_GUARD_BUF_LENGTH];
    char syslog_txt[512], *syslog_txt_p;
#endif

    if (ip_source_guard_mgmt_conf_get_first_dynamic_entry(&entry) == VTSS_RC_OK) {
        if ((entry.isid == isid) && (entry.port_no == port_no)) {
            if (ip_source_guard_mgmt_conf_del_dynamic_entry(&entry)) {
                T_W("ip_source_guard_mgmt_conf_del_dynamic_entry() failed");
            } else {
                /* Fill system log which entry had been eliminated */
#ifdef VTSS_SW_OPTION_SYSLOG
                syslog_txt_p = &syslog_txt[0];
                syslog_txt_p += sprintf(syslog_txt_p, "IP_SOURCE_GUARD-TABLE_FULL: Dynamic IP Source Guard Table port limitation changed. should delete entry <VLAN ID = %d,", entry.vid);
                syslog_txt_p += sprintf(syslog_txt_p, " Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
                syslog_txt_p += sprintf(syslog_txt_p, ", IP Address = %s, MAC Address = %s> to table",
                                        misc_ipv4_txt(entry.assigned_ip, ip_txt),
                                        misc_mac_txt(entry.assigned_mac, mac_txt));
                S_PORT_N(entry.isid, entry.port_no, "%s", syslog_txt);
#endif /* VTSS_SW_OPTION_SYSLOG */
                del_done = TRUE;
            }
        }

        if (del_done) {
            return VTSS_RC_OK;
        }

        while (ip_source_guard_mgmt_conf_get_next_dynamic_entry(&entry) == VTSS_RC_OK) {
            if ((entry.isid == isid) && (entry.port_no == port_no)) {
                if (ip_source_guard_mgmt_conf_del_dynamic_entry(&entry)) {
                    T_W("ip_source_guard_mgmt_conf_del_dynamic_entry() failed");
                } else {
                    /* Fill system log which entry had been eliminated */
#ifdef VTSS_SW_OPTION_SYSLOG
                    syslog_txt_p = &syslog_txt[0];
                    syslog_txt_p += sprintf(syslog_txt_p, "IP_SOURCE_GUARD-TABLE_FULL: Dynamic IP Source Guard Table port limitation changed. should delete entry <VLAN ID = %d,", entry.vid);
                    syslog_txt_p += sprintf(syslog_txt_p, " Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
                    syslog_txt_p += sprintf(syslog_txt_p, ", IP Address = %s, MAC Address = %s> to table",
                                            misc_ipv4_txt(entry.assigned_ip, ip_txt),
                                            misc_mac_txt(entry.assigned_mac, mac_txt));
                    S_PORT_N(entry.isid, entry.port_no, "%s", syslog_txt);
#endif /* VTSS_SW_OPTION_SYSLOG */
                    del_done = TRUE;
                    break;
                }
            }
        }
    }
    if (del_done) {
        return VTSS_RC_OK;
    } else {
        return IP_SOURCE_GUARD_ERROR_DATABASE_ACCESS;
    }
}

/* Get current port dynamic entry count */
static mesa_rc ip_source_guard_mgmt_conf_get_current_port_dynamic_entry_cnt(vtss_isid_t isid, mesa_port_no_t port_no, ulong *current_port_dynamic_entry_cnt_p)
{
    ip_source_guard_entry_t entry;
    ulong                   current_port_entry_cnt = 0;

    if (ip_source_guard_mgmt_conf_get_first_dynamic_entry(&entry) == VTSS_RC_OK) {
        if ((entry.isid == isid) && (entry.port_no == port_no)) {
            current_port_entry_cnt++;
        }
        while (ip_source_guard_mgmt_conf_get_next_dynamic_entry(&entry) == VTSS_RC_OK) {
            if ((entry.isid == isid) && (entry.port_no == port_no)) {
                current_port_entry_cnt++;
            }
        }
    }
    *current_port_dynamic_entry_cnt_p = current_port_entry_cnt;

    return VTSS_RC_OK;
}

/* Eliminated dynamic entry */
static void ip_source_guard_eliminated_dynamic_entry(vtss_isid_t isid, mesa_port_no_t port_no, ulong entry_cnt)
{
    //1. get current entry cnt
    //2. entry_cnt is user config value
    //3. we should del (current entry cnt - user config value) entriese from list

    ulong   current_port_entry_cnt;
    int     extra_entry_cnt;

    if (ip_source_guard_mgmt_conf_get_current_port_dynamic_entry_cnt(isid, port_no, &current_port_entry_cnt)) {
        T_W("ip_source_guard_mgmt_conf_get_current_port_dynamic_entry_cnt() failed");
    }

    extra_entry_cnt = current_port_entry_cnt - entry_cnt;

    T_D("extra_entry_cnt = %d", extra_entry_cnt);
    T_D("current_port_entry_cnt = " VPRIlu, current_port_entry_cnt);
    T_D("entry_cnt = " VPRIlu, entry_cnt);

    if (extra_entry_cnt > 0) {
        while (extra_entry_cnt) {
            if (ip_source_guard_del_first_dynamic_entry_with_specific_port(isid, port_no)) {
                T_W("ip_source_guard_del_first_dynamic_entry_with_specific_port() failed, %d", extra_entry_cnt);
            }
            extra_entry_cnt--;
        }
    }

    return;
}

/* set port dynamic entry cnt */
mesa_rc ip_source_guard_mgmt_conf_set_port_dynamic_entry_cnt(vtss_isid_t isid, ip_source_guard_port_dynamic_entry_conf_t *port_dynamic_entry_conf)
{
    mesa_rc     rc = VTSS_RC_OK;
    CapArray<ulong, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_conf_change;
    ulong       global_mode, port_mode;
    int         changed = FALSE;
    port_iter_t pit;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }

    /* check the max value on all ports */
    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        if (port_dynamic_entry_conf->entry_cnt[pit.iport] != IP_SOURCE_GUARD_DYNAMIC_UNLIMITED && port_dynamic_entry_conf->entry_cnt[pit.iport] > IP_SOURCE_GUARD_MAX_DYNAMIC_LIMITED_CNT) {
            return IP_SOURCE_GUARD_ERROR_INV_PARAM;
        }
    }

    IP_SOURCE_GUARD_CRIT_ENTER();
    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        if (ip_source_guard_global.ip_source_guard_conf.port_dynamic_entry_conf[isid - VTSS_ISID_START].entry_cnt[pit.iport] != port_dynamic_entry_conf->entry_cnt[pit.iport] &&
            ip_source_guard_global.ip_source_guard_conf.port_dynamic_entry_conf[isid - VTSS_ISID_START].entry_cnt[pit.iport] > port_dynamic_entry_conf->entry_cnt[pit.iport]) {
            port_conf_change[pit.iport] = TRUE;
        }
        ip_source_guard_global.ip_source_guard_conf.port_dynamic_entry_conf[isid - VTSS_ISID_START].entry_cnt[pit.iport] = port_dynamic_entry_conf->entry_cnt[pit.iport];
    }
    changed = TRUE;
    IP_SOURCE_GUARD_CRIT_EXIT();

    if (changed) {
        IP_SOURCE_GUARD_CRIT_ENTER();
        global_mode = ip_source_guard_global.ip_source_guard_conf.mode;
        IP_SOURCE_GUARD_CRIT_EXIT();

        if (global_mode == IP_SOURCE_GUARD_MGMT_ENABLED) {
            (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (port_conf_change[pit.iport] == FALSE) {
                    continue;
                }
                IP_SOURCE_GUARD_CRIT_ENTER();
                port_mode = ip_source_guard_global.ip_source_guard_conf.port_mode_conf[isid - VTSS_ISID_START].mode[pit.iport];
                IP_SOURCE_GUARD_CRIT_EXIT();
                if (port_mode == IP_SOURCE_GUARD_MGMT_ENABLED) {
                    ip_source_guard_eliminated_dynamic_entry(isid, pit.iport, port_dynamic_entry_conf->entry_cnt[pit.iport]);
                }
            }
        }
    }

    T_D("exit");
    return rc;
}

mesa_rc ip_source_guard_mgmt_conf_get_port_dynamic_entry_cnt(vtss_isid_t isid, ip_source_guard_port_dynamic_entry_conf_t *port_dynamic_entry_conf)
{
    mesa_rc         rc = VTSS_RC_OK;
    port_iter_t     pit;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();
    //(void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_ALL);
    while (port_iter_getnext(&pit)) {
        port_dynamic_entry_conf->entry_cnt[pit.iport] = ip_source_guard_global.ip_source_guard_conf.port_dynamic_entry_conf[isid - VTSS_ISID_START].entry_cnt[pit.iport];
    }
    IP_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/****************************************************************************/
/*  Register functions                                                      */
/****************************************************************************/

/* Register DHCP receive */
static void ip_source_guard_dhcp_pkt_receive(dhcp_snooping_ip_assigned_info_t *info, dhcp_snooping_info_reason_t reason)
{
    ip_source_guard_entry_t entry;

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return;
    }

    memset(&entry, 0x0, sizeof(entry));
    entry.vid = info->vid;
    memcpy(entry.assigned_mac, info->mac, IP_SOURCE_GUARD_MAC_LENGTH);
    entry.assigned_ip = info->assigned_ip;
    entry.ip_mask = 0xFFFFFFFF;
    entry.isid = info->isid;
    entry.port_no = info->port_no;
    entry.type = IP_SOURCE_GUARD_DYNAMIC_TYPE;
    if (reason == DHCP_SNOOPING_INFO_REASON_ASSIGN_COMPLETED) {
        entry.valid = TRUE;
        if (ip_source_guard_mgmt_conf_set_dynamic_entry(&entry)) {
            T_D("ip_source_guard_mgmt_conf_set_dynamic_entry() failed");
        }
    } else {
        entry.valid = FALSE;
        if (ip_source_guard_mgmt_conf_del_dynamic_entry(&entry)) {
            T_D("ip_source_guard_mgmt_conf_del_dynamic_entry() failed");
        }
    }

    return;
}

/****************************************************************************/
/*  IP source guard configuration                                           */
/****************************************************************************/

/* Determine if IP_SOURCE_GUARD configuration has changed */
static int ip_source_guard_conf_changed(ip_source_guard_conf_t *old, ip_source_guard_conf_t *new_)
{
    return vtss_memcmp(*old, *new_);
}

/* Apply IP source guard configuration */
static void ip_source_guard_conf_apply(void)
{
    ip_source_guard_entry_t             entry;
    uchar                               mac[IP_SOURCE_GUARD_MAC_LENGTH], null_mac[IP_SOURCE_GUARD_MAC_LENGTH] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    mesa_vid_t                          vid;
    dhcp_snooping_ip_assigned_info_t    info;
    ip_source_guard_port_mode_conf_t    *port_mode_conf;
    switch_iter_t                       sit;
    port_iter_t                         pit;

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_D("not primary switch");
        return;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();
    if (ip_source_guard_global.ip_source_guard_conf.mode == IP_SOURCE_GUARD_MGMT_ENABLED) {
        port_mode_conf = ip_source_guard_global.ip_source_guard_conf.port_mode_conf;

        /* Set default ACE */
        (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
            if (!msg_switch_exists(sit.isid)) {
                continue;
            }

            (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (port_mode_conf[sit.isid - VTSS_ISID_START].mode[pit.iport] == IP_SOURCE_GUARD_MGMT_DISABLED) {
                    continue;
                }
                if (_ip_source_guard_default_ace_add(sit.isid, pit.iport)) {
                    T_W("_ip_source_guard_default_ace_add() failed");
                }
            }
        }

        /* Get information from DHCP snooping */
        memset(&entry, 0x0, sizeof(entry));
        memcpy(mac, null_mac, sizeof(mac));
        vid = 0;
        while (dhcp_snooping_ip_assigned_info_getnext(mac, vid, &info)) {
            memcpy(mac, info.mac, IP_SOURCE_GUARD_MAC_LENGTH);
            vid = info.vid;
            if (!msg_switch_exists(info.isid) ||
                port_mode_conf[info.isid - VTSS_ISID_START].mode[info.port_no] == IP_SOURCE_GUARD_MGMT_DISABLED) {
                continue;
            }
            entry.vid = info.vid;
            memcpy(entry.assigned_mac, info.mac, IP_SOURCE_GUARD_MAC_LENGTH);
            entry.assigned_ip = info.assigned_ip;
            entry.ip_mask = 0xFFFFFFFF;
            entry.isid = info.isid;
            entry.port_no = info.port_no;
            entry.type = IP_SOURCE_GUARD_DYNAMIC_TYPE;
            entry.ace_id = ACL_MGMT_ACE_ID_NONE;
            entry.valid = TRUE;

            /* Check if reach max count */
            if (_ip_source_guard_is_reach_max_port_dynamic_entry_cnt(&entry)) {
                break;
            }

            /* Add ACE for dynamic entry */
            if (_ip_source_guard_mgmt_conf_get_static_entry(&entry) != VTSS_RC_OK) {
                if (_ip_source_guard_ace_add(&entry)) {
                    T_W("_ip_source_guard_ace_add() failed");
                } else {
                    if (_ip_source_guard_mgmt_conf_add_dynamic_entry(&entry, TRUE) != VTSS_RC_OK) {
                        T_W("_ip_source_guard_mgmt_conf_add_dynamic_entry() failed");
                    }
                }
            }
        };

        /* Add ACEs for static entries */
        if (_ip_source_guard_mgmt_conf_get_first_static_entry(&entry) == VTSS_RC_OK) {
            if (port_mode_conf[entry.isid - VTSS_ISID_START].mode[entry.port_no] == IP_SOURCE_GUARD_MGMT_ENABLED) {
                if (entry.valid /* && entry.ace_id == VTSS_APPL_ACL_ACE_ID_NONE */ && msg_switch_exists(entry.isid)) {
                    if (_ip_source_guard_ace_add(&entry)) {
                        T_W("_ip_source_guard_ace_add() failed");
                    } else {
                        T_N("ace_id = %u", entry.ace_id);
                        if (_ip_source_guard_mgmt_conf_update_static_entry(&entry) != VTSS_RC_OK) {
                            T_W("_ip_source_guard_mgmt_conf_update_static_entry() failed");
                        }
                    }
                }
            }
            while (_ip_source_guard_mgmt_conf_get_next_static_entry(&entry) == VTSS_RC_OK) {
                if (port_mode_conf[entry.isid - VTSS_ISID_START].mode[entry.port_no] == IP_SOURCE_GUARD_MGMT_DISABLED) {
                    continue;
                }
                if (entry.valid /* && entry.ace_id == VTSS_APPL_ACL_ACE_ID_NONE */ && msg_switch_exists(entry.isid)) {
                    if (_ip_source_guard_ace_add(&entry)) {
                        T_W("_ip_source_guard_ace_add() failed");
                    } else {
                        T_N("ace_id = %u", entry.ace_id);
                        if (_ip_source_guard_mgmt_conf_update_static_entry(&entry) != VTSS_RC_OK) {
                            T_W("_ip_source_guard_mgmt_conf_update_static_entry() failed");
                        }
                    }
                }
            }
        }

        /* Register DHCP snooping */
        dhcp_snooping_ip_assigned_info_register(ip_source_guard_dhcp_pkt_receive);
    } else {
        /* Unregister DHCP snooping */
        dhcp_snooping_ip_assigned_info_unregister(ip_source_guard_dhcp_pkt_receive);

        /* Delete ACEs from static entries */
        if (_ip_source_guard_mgmt_conf_get_first_static_entry(&entry) == VTSS_RC_OK) {
            if (entry.valid && entry.ace_id != ACL_MGMT_ACE_ID_NONE) {
                entry.ace_id = ACL_MGMT_ACE_ID_NONE;
                if (_ip_source_guard_mgmt_conf_update_static_entry(&entry) != VTSS_RC_OK) {
                    T_W("_ip_source_guard_mgmt_conf_update_static_entry() failed");
                }
            }
            while (_ip_source_guard_mgmt_conf_get_next_static_entry(&entry) == VTSS_RC_OK) {
                if (entry.valid && entry.ace_id != ACL_MGMT_ACE_ID_NONE) {
                    entry.ace_id = ACL_MGMT_ACE_ID_NONE;
                    if (_ip_source_guard_mgmt_conf_update_static_entry(&entry) != VTSS_RC_OK) {
                        T_W("_ip_source_guard_mgmt_conf_update_static_entry() failed");
                    }
                }
            }
        }

        /* Clear all dynamic entries */
        if (_ip_source_guard_mgmt_conf_del_all_dynamic_entry(TRUE) != VTSS_RC_OK) {
            T_W("_ip_source_guard_mgmt_conf_del_all_dynamic_entry() failed");
        }

        /* Clear all IP source guard ACEs */
        _ip_source_guard_ace_clear();
    }
    IP_SOURCE_GUARD_CRIT_EXIT();

    return;
}

/* Apply IP source guard port configuration change */
static void ip_source_guard_port_conf_changed_apply(vtss_isid_t isid, mesa_port_no_t port_no, ulong mode)
{
    ip_source_guard_entry_t             entry;
    dhcp_snooping_ip_assigned_info_t    info;
    uchar                               mac[IP_SOURCE_GUARD_MAC_LENGTH], null_mac[IP_SOURCE_GUARD_MAC_LENGTH] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    mesa_vid_t                          vid;

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();
    if (ip_source_guard_global.ip_source_guard_conf.mode == IP_SOURCE_GUARD_MGMT_DISABLED) {
        IP_SOURCE_GUARD_CRIT_EXIT();
        return;
    }
    IP_SOURCE_GUARD_CRIT_EXIT();

    if (mode == IP_SOURCE_GUARD_MGMT_ENABLED) {
        IP_SOURCE_GUARD_CRIT_ENTER();

        /* Set default ACE */
        if (_ip_source_guard_default_ace_add(isid, port_no)) {
            T_W("_ip_source_guard_default_ace_add() failed");
        }

        /* Get information from DHCP snooping */
        memset(&entry, 0x0, sizeof(entry));
        memcpy(mac, null_mac, sizeof(mac));
        vid = 0;
        while (dhcp_snooping_ip_assigned_info_getnext(mac, vid, &info)) {
            memcpy(mac, info.mac, IP_SOURCE_GUARD_MAC_LENGTH);
            vid = info.vid;
            if (info.isid == isid && info.port_no == port_no) {
                entry.vid = info.vid;
                memcpy(entry.assigned_mac, info.mac, IP_SOURCE_GUARD_MAC_LENGTH);
                entry.assigned_ip = info.assigned_ip;
                entry.ip_mask = 0xFFFFFFFF;
                entry.isid = info.isid;
                entry.port_no = info.port_no;
                entry.type = IP_SOURCE_GUARD_DYNAMIC_TYPE;
                entry.ace_id = ACL_MGMT_ACE_ID_NONE;
                entry.valid = TRUE;

                /* Check if reach max count */
                if (_ip_source_guard_is_reach_max_port_dynamic_entry_cnt(&entry)) {
                    break;
                }

                /* Add ACE for dynamic entry */
                if (_ip_source_guard_mgmt_conf_get_static_entry(&entry) != VTSS_RC_OK) {
                    if (_ip_source_guard_ace_add(&entry)) {
                        T_W("_ip_source_guard_ace_add() failed");
                    } else {
                        if (_ip_source_guard_mgmt_conf_add_dynamic_entry(&entry, TRUE) != VTSS_RC_OK) {
                            T_W("_ip_source_guard_mgmt_conf_add_dynamic_entry() failed");
                        }
                    }
                }
            }
        };

        /* Add ACEs for static entries */
        if (_ip_source_guard_mgmt_conf_get_first_static_entry(&entry) == VTSS_RC_OK) {
            if (entry.isid == isid && entry.port_no == port_no && entry.valid) {
                entry.ace_id = ACL_MGMT_ACE_ID_NONE;
                if (_ip_source_guard_ace_add(&entry)) {
                    T_W("_ip_source_guard_ace_add() failed");
                } else {
                    T_N("ace_id = %u", entry.ace_id);
                    if (_ip_source_guard_mgmt_conf_update_static_entry(&entry) != VTSS_RC_OK) {
                        T_W("_ip_source_guard_mgmt_conf_update_static_entry() failed");
                    }
                }
            }
            while (_ip_source_guard_mgmt_conf_get_next_static_entry(&entry) == VTSS_RC_OK) {
                if (entry.isid == isid && entry.port_no == port_no && entry.valid) {
                    entry.ace_id = ACL_MGMT_ACE_ID_NONE;
                    if (_ip_source_guard_ace_add(&entry)) {
                        T_W("_ip_source_guard_ace_add() failed");
                    } else {
                        T_N("ace_id = %u", entry.ace_id);
                        if (_ip_source_guard_mgmt_conf_update_static_entry(&entry) != VTSS_RC_OK) {
                            T_W("_ip_source_guard_mgmt_conf_update_static_entry() failed");
                        }
                    }
                }
            }
        }
        IP_SOURCE_GUARD_CRIT_EXIT();
    } else {
        /* Delete ACEs from static entries */
        IP_SOURCE_GUARD_CRIT_ENTER();
        if (_ip_source_guard_mgmt_conf_get_first_static_entry(&entry) == VTSS_RC_OK) {
            if (entry.isid == isid && entry.port_no == port_no && entry.valid) {
                if (_ip_source_guard_ace_del(&entry)) {
                    T_W("_ip_source_guard_ace_del() failed");
                } else {
                    T_N("ace_id = %u", entry.ace_id);
                    if (_ip_source_guard_mgmt_conf_update_static_entry(&entry) != VTSS_RC_OK) {
                        T_W("_ip_source_guard_mgmt_conf_update_static_entry() failed");
                    }
                }
            }
            while (_ip_source_guard_mgmt_conf_get_next_static_entry(&entry) == VTSS_RC_OK) {
                if (entry.isid == isid && entry.port_no == port_no && entry.valid) {
                    if (_ip_source_guard_ace_del(&entry)) {
                        T_W("_ip_source_guard_ace_del() failed");
                    } else {
                        T_N("ace_id = %u", entry.ace_id);
                        if (_ip_source_guard_mgmt_conf_update_static_entry(&entry) != VTSS_RC_OK) {
                            T_W("_ip_source_guard_mgmt_conf_update_static_entry() failed");
                        }
                    }
                }
            }
        }
        IP_SOURCE_GUARD_CRIT_EXIT();

        /* Clear all dynamic entry by port */
        (void) ip_source_guard_mgmt_conf_flush_dynamic_entry_by_port(isid, port_no);

        /* Delete default ACE */
        IP_SOURCE_GUARD_CRIT_ENTER();
        if (_ip_source_guard_default_ace_del(isid, port_no)) {
            T_D("_ip_source_guard_default_ace_del() failed");
        }
        IP_SOURCE_GUARD_CRIT_EXIT();
    }

    return;
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/* Set IP_SOURCE_GUARD defaults */
void ip_source_guard_default_set(ip_source_guard_conf_t *conf)
{
    _ip_source_guard_default_set(conf);
    return;
}

/* IP_SOURCE_GUARD error text */
const char *ip_source_guard_error_txt(mesa_rc rc)
{
    switch (rc) {
    case IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH:
        return "IP source guard: operation only valid on primary switch.";

    case IP_SOURCE_GUARD_ERROR_ISID:
        return "IP source guard: invalid Switch ID.";

    case IP_SOURCE_GUARD_ERROR_ISID_NON_EXISTING:
        return "IP source guard: switch ID is non-existing";

    case IP_SOURCE_GUARD_ERROR_INV_PARAM:
        return "IP source guard: invalid parameter supplied to function.";

    case IP_SOURCE_GUARD_ERROR_STATIC_TABLE_FULL:
        return "IP source guard: static table is full.";

    case IP_SOURCE_GUARD_ERROR_DYNAMIC_TABLE_FULL:
        return "IP source guard: dynamic table is full.";

    case IP_SOURCE_GUARD_ERROR_ACE_AUTO_ASSIGNED_FAIL:
        return "IP source guard: ACE auto-assigned fail.";

    case IP_SOURCE_GUARD_ERROR_DATABASE_ACCESS:
        return "IP source guard: ACE databse access error.";

    case IP_SOURCE_GUARD_ERROR_ENTRY_EXIST_ON_DB:
        return "IP source guard: the entry exists in the database.";

    case IP_SOURCE_GUARD_ERROR_DATABASE_ADD:
        return "IP source guard: add the entry in the database fail.";

    case IP_SOURCE_GUARD_ERROR_DATABASE_DEL:
        return "IP source guard: delete the entry in the database fail.";

    case IP_SOURCE_GUARD_ERROR_LOAD_CONF:
        return "IP source guard: open configuration fail.";

    case IP_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL:
        return "IP source guard: Operation unsuccessful.";

    default:
        return "IP source guard: unknown error code.";
    }
}

/* Get IP_SOURCE_GUARD configuration */
mesa_rc ip_source_guard_mgmt_conf_get_mode(ulong *mode)
{
    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();
    *mode = ip_source_guard_global.ip_source_guard_conf.mode;
    IP_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/* Set IP_SOURCE_GUARD configuration */
mesa_rc ip_source_guard_mgmt_conf_set_mode(ulong mode)
{
    mesa_rc rc = VTSS_RC_OK;
    int     changed = FALSE;

    T_D("enter, mode: " VPRIlu, mode);

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check illegal parameter */
    if (mode != IP_SOURCE_GUARD_MGMT_ENABLED &&
        mode != IP_SOURCE_GUARD_MGMT_DISABLED) {
        return IP_SOURCE_GUARD_ERROR_INV_PARAM;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();
    if (ip_source_guard_global.ip_source_guard_conf.mode != mode) {
        ip_source_guard_global.ip_source_guard_conf.mode = mode;
        changed = TRUE;
    }
    IP_SOURCE_GUARD_CRIT_EXIT();

    if (changed) {
        /* Activate changed configuration */
        ip_source_guard_conf_apply();
    }

    T_D("exit");
    return rc;
}

/* Get first IP_SOURCE_GUARD static entry */
mesa_rc ip_source_guard_mgmt_conf_get_first_static_entry(ip_source_guard_entry_t *entry)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();
    rc = _ip_source_guard_mgmt_conf_get_first_static_entry(entry);
    IP_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Get IP_SOURCE_GUARD static entry */
mesa_rc ip_source_guard_mgmt_conf_get_static_entry(ip_source_guard_entry_t *entry)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();
    rc = _ip_source_guard_mgmt_conf_get_static_entry(entry);
    IP_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Get Next IP_SOURCE_GUARD static entry */
mesa_rc ip_source_guard_mgmt_conf_get_next_static_entry(ip_source_guard_entry_t *entry)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();
    rc = _ip_source_guard_mgmt_conf_get_next_static_entry(entry);
    IP_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Set IP_SOURCE_GUARD static entry */
mesa_rc ip_source_guard_mgmt_conf_set_static_entry(ip_source_guard_entry_t *entry)
{
    mesa_rc     rc = VTSS_RC_OK;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check switch ID */
    if (!msg_switch_configurable(entry->isid)) {
        T_W("isid: %d isn't configurable switch", entry->isid);
        T_D("exit");
        return IP_SOURCE_GUARD_ERROR_ISID_NON_EXISTING;
    }
    /* Check illegal parameter */
    if (entry->assigned_ip == 0) {
        T_D("exit");
        return IP_SOURCE_GUARD_ERROR_INV_PARAM;
    }
    /* Check illegal parameter */
    if (entry->vid >= VTSS_VIDS) {
        T_D("illegal vid: %u", entry->vid);
        T_D("exit");
        return IP_SOURCE_GUARD_ERROR_INV_PARAM;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();

    /* Check the entry exist or not ? */
    if ((rc = _ip_source_guard_mgmt_conf_get_dynamic_entry(entry)) != IP_SOURCE_GUARD_ERROR_DATABASE_ACCESS) {
        //if existing, transfer the dynamic entry into static entry
        T_D("the entry existing on dynamic db, transfer, rc=%d", rc);
        if ((rc = _ip_source_guard_mgmt_conf_translate_dynamic_entry_into_static_entry(entry)) == VTSS_RC_OK) {
            /* save configuration */
            rc = _ip_source_guard_mgmt_conf_save_static_configuration();
        }
        IP_SOURCE_GUARD_CRIT_EXIT();
        T_D("exit");
        return rc;
    }
    if ((rc = _ip_source_guard_mgmt_conf_get_static_entry(entry)) != IP_SOURCE_GUARD_ERROR_DATABASE_ACCESS) {
        //if existing, return the event
        T_D("the entry existing on static db, exit, rc=%d", rc);
        IP_SOURCE_GUARD_CRIT_EXIT();
        T_D("exit");
        return IP_SOURCE_GUARD_ERROR_ENTRY_EXIST_ON_DB;
    }

    /* Check total count reach the max value or not? */
    if (_ip_source_guard_entry_count() >= IP_SOURCE_GUARD_MAX_ENTRY_CNT) {
        T_D("total count, rc=%d", _ip_source_guard_entry_count());
        IP_SOURCE_GUARD_CRIT_EXIT();
        T_D("exit");
        T_W("ip source guard: table full.");
        return IP_SOURCE_GUARD_ERROR_STATIC_TABLE_FULL;
    }

    entry->valid = TRUE;
    entry->ace_id = ACL_MGMT_ACE_ID_NONE;
    entry->type = IP_SOURCE_GUARD_STATIC_TYPE;

    /* check the system mode and port mode */
    if (ip_source_guard_global.ip_source_guard_conf.mode == IP_SOURCE_GUARD_MGMT_ENABLED &&
        ip_source_guard_global.ip_source_guard_conf.port_mode_conf[entry->isid - VTSS_ISID_START].mode[entry->port_no] == IP_SOURCE_GUARD_MGMT_ENABLED) {
        /* add the entry into ACL */
        if ((rc = _ip_source_guard_ace_add(entry)) != VTSS_RC_OK) {
            T_W("_ip_source_guard_ace_add() failed");
        }
    }

    /* add the entry into static DB */
    if ((rc = _ip_source_guard_mgmt_conf_add_static_entry(entry, TRUE)) == VTSS_RC_OK) {
        /* save configuration */
        rc = _ip_source_guard_mgmt_conf_save_static_configuration();
    } else {
        /* check ACL inserted or not  */
        if (entry->ace_id != ACL_MGMT_ACE_ID_NONE) {
            /* delete the ace entry */
            if (_ip_source_guard_ace_del(entry) != VTSS_RC_OK) {
                T_D("_ip_source_guard_ace_del() failed");
            }
        }
    }

    IP_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Set IP_SOURCE_GUARD static entry */
mesa_rc ip_source_guard_mgmt_conf_del_static_entry(ip_source_guard_entry_t *entry)
{
    mesa_rc     rc = VTSS_RC_OK;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check switch ID */
    if (!msg_switch_configurable(entry->isid)) {
        T_W("isid: %d isn't configurable switch", entry->isid);
        T_D("exit");
        return IP_SOURCE_GUARD_ERROR_ISID_NON_EXISTING;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();

    /* Check if entry exist? */
    if ((rc = _ip_source_guard_mgmt_conf_get_static_entry(entry)) == IP_SOURCE_GUARD_ERROR_DATABASE_ACCESS) {
        //if not existing, return
        IP_SOURCE_GUARD_CRIT_EXIT();
        T_D("exit");
        return rc;
    } else {
        /* check ACL inserted or not */
        if (entry->valid && entry->ace_id != ACL_MGMT_ACE_ID_NONE) {
            /* delete the entry on ACL */
            if (_ip_source_guard_ace_del(entry) != VTSS_RC_OK) {
                T_D("_ip_source_guard_ace_del() failed");
            }
        }
    }

    /* delete the entry on static DB */
    if ((rc = _ip_source_guard_mgmt_conf_del_static_entry(entry, TRUE)) != VTSS_RC_OK) {
        T_W("_ip_source_guard_mgmt_conf_del_static_entry failed");
    }

    /* save configuration */
    rc = _ip_source_guard_mgmt_conf_save_static_configuration();

    IP_SOURCE_GUARD_CRIT_EXIT();

    /*
     * If source guard is enabled globally and on port we need to check if the
     * entry that was deleted exists in dhcp6 snooping table, and if so add it
     * as a dynamic entry if dynamic port limit has not been reached.
     */

    ulong mode;
    if (ip_source_guard_mgmt_conf_get_mode(&mode) != VTSS_RC_OK) {
        T_E("ip_source_guard_mgmt_conf_get_mode()\n");
        return VTSS_RC_ERROR;
    }
    if (mode != IP_SOURCE_GUARD_MGMT_ENABLED) {
        T_D("Module not enabled, exit");
        return VTSS_RC_OK;
    }

    ip_source_guard_port_mode_conf_t            mode_conf;
    if (ip_source_guard_mgmt_conf_get_port_mode(entry->isid, &mode_conf) != VTSS_RC_OK) {
        T_E("ip_source_guard_mgmt_conf_get_port_mode()\n");
        return VTSS_RC_ERROR;
    }
    if (mode_conf.mode[entry->port_no] == IP_SOURCE_GUARD_MGMT_DISABLED) {
        T_D("port not enabled, exit");
        return VTSS_RC_OK;
    }

    // refresh snooping entries for port
    ip_source_guard_check_snooping_entries(entry);

    T_D("exit");
    return rc;
}

/* del all IP_SOURCE_GUARD static entry */
mesa_rc ip_source_guard_mgmt_conf_del_all_static_entry(void)
{
    mesa_rc                         rc = VTSS_RC_OK;
    ip_source_guard_entry_t         entry;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();
    /* Delete ACEs from static entries */
    while (_ip_source_guard_mgmt_conf_get_first_static_entry(&entry) == VTSS_RC_OK) {
        if (entry.valid && entry.ace_id != ACL_MGMT_ACE_ID_NONE) {
            if (_ip_source_guard_ace_del(&entry)) {
                T_D("_ip_source_guard_ace_del() failed");
            }
        }

        /* delete the entry on static DB */
        if ((rc = _ip_source_guard_mgmt_conf_del_static_entry(&entry, TRUE)) != VTSS_RC_OK) {
            T_W("_ip_source_guard_mgmt_conf_del_static_entry failed");
        }
    }

    /* save configuration */
    rc = _ip_source_guard_mgmt_conf_save_static_configuration();

    IP_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    return rc;
}

mesa_rc ip_source_guard_mgmt_conf_set_port_mode(vtss_isid_t isid, ip_source_guard_port_mode_conf_t *port_mode_conf)
{
    int                                 changed = FALSE;
    CapArray<ulong, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_conf_change;
    port_iter_t                         pit;
    ip_source_guard_port_mode_conf_t    *global_port_mode_ptr;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check switch ID */
    if (!msg_switch_configurable(isid)) {
        T_W("isid: %d isn't configurable switch", isid);
        T_D("exit");
        return IP_SOURCE_GUARD_ERROR_ISID_NON_EXISTING;
    }
    /* Check illegal parameter */
    if (port_mode_conf == NULL) {
        T_W("illegal parameter");
        T_D("exit");
        return IP_SOURCE_GUARD_ERROR_INV_PARAM;
    }

    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        if (port_mode_conf->mode[pit.iport] != IP_SOURCE_GUARD_MGMT_ENABLED &&
            port_mode_conf->mode[pit.iport] != IP_SOURCE_GUARD_MGMT_DISABLED) {
            return IP_SOURCE_GUARD_ERROR_INV_PARAM;
        }
    }

    /* Check port mode change */
    IP_SOURCE_GUARD_CRIT_ENTER();
    global_port_mode_ptr = &ip_source_guard_global.ip_source_guard_conf.port_mode_conf[isid - VTSS_ISID_START];
    if (vtss_memcmp(*global_port_mode_ptr, *port_mode_conf)) {
        changed = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (ip_source_guard_global.ip_source_guard_conf.port_mode_conf[isid - VTSS_ISID_START].mode[pit.iport] != port_mode_conf->mode[pit.iport]) {
                port_conf_change[pit.iport] = TRUE;
                ip_source_guard_global.ip_source_guard_conf.port_mode_conf[isid - VTSS_ISID_START].mode[pit.iport] = port_mode_conf->mode[pit.iport];
            }
        }
    }
    IP_SOURCE_GUARD_CRIT_EXIT();

    if (changed) {
        /* Activate changed configuration */
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (port_conf_change[pit.iport]) {
                ip_source_guard_port_conf_changed_apply(isid, pit.iport, port_mode_conf->mode[pit.iport]);
            }
        }
    }

    T_D("exit");
    return VTSS_RC_OK;
}

mesa_rc ip_source_guard_mgmt_conf_get_port_mode(vtss_isid_t isid, ip_source_guard_port_mode_conf_t *port_mode_conf)
{
    ip_source_guard_port_mode_conf_t    *global_port_mode_ptr;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check switch ID */
    if (!msg_switch_configurable(isid)) {
        T_W("isid: %d isn't configurable switch", isid);
        T_D("exit");
        return IP_SOURCE_GUARD_ERROR_ISID_NON_EXISTING;
    }
    /* Check illegal parameter */
    if (port_mode_conf == NULL) {
        T_W("illegal parameter");
        T_D("exit");
        return IP_SOURCE_GUARD_ERROR_INV_PARAM;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();
    global_port_mode_ptr = &ip_source_guard_global.ip_source_guard_conf.port_mode_conf[isid - VTSS_ISID_START];
    *port_mode_conf = *global_port_mode_ptr;
    IP_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/* Translate IP_SOURCE_GUARD dynamic entries into static entries */
mesa_rc ip_source_guard_mgmt_conf_translate_dynamic_into_static(void)
{
    mesa_rc                     rc = VTSS_RC_OK;
    ip_source_guard_entry_t     entry;
    int                         count = 0;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH;
    }

    IP_SOURCE_GUARD_CRIT_ENTER();

    /* translate dynamic entries into static entries */
    if (_ip_source_guard_mgmt_conf_get_first_dynamic_entry(&entry) == VTSS_RC_OK) {
        if ((rc = _ip_source_guard_mgmt_conf_translate_dynamic_entry_into_static_entry(&entry)) != VTSS_RC_OK) {
            T_D("_ip_source_guard_mgmt_conf_translate_dynamic_entry_into_static_entry() failed");
        } else {
            count++;
        }

        while (_ip_source_guard_mgmt_conf_get_next_dynamic_entry(&entry) == VTSS_RC_OK) {
            if ((rc = _ip_source_guard_mgmt_conf_translate_dynamic_entry_into_static_entry(&entry)) != VTSS_RC_OK) {
                T_D("_ip_source_guard_mgmt_conf_translate_dynamic_entry_into_static_entry() failed");
            } else {
                count++;
            }
        }
    }

    /* save configuration */
    rc = _ip_source_guard_mgmt_conf_save_static_configuration();

    IP_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    if (rc < VTSS_RC_OK) {
        return rc;
    } else {
        return count;
    }
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

/* Read/create IP_SOURCE_GUARD stack configuration */
static void ip_source_guard_conf_read_stack(BOOL create)
{
    int                             changed;
    static ip_source_guard_conf_t   new_ip_source_guard_conf;
    ip_source_guard_conf_t          *old_ip_source_guard_conf_p;

    T_D("enter, create: %d", create);

    changed = FALSE;
    IP_SOURCE_GUARD_CRIT_ENTER();

    /* Use default values */
    _ip_source_guard_default_set(&new_ip_source_guard_conf);

    /* Delete all static entries */
    if (_ip_source_guard_mgmt_conf_del_all_static_entry(TRUE) != VTSS_RC_OK) {
        T_W("_ip_source_guard_mgmt_conf_del_all_static_entry() failed");
    }

    /* Delete all dynamic entries */
    if (_ip_source_guard_mgmt_conf_del_all_dynamic_entry(TRUE) != VTSS_RC_OK) {
        T_W("_ip_source_guard_mgmt_conf_del_all_dynamic_entry() failed");
    }

    /* Clear all IP source guard ACEs */
    _ip_source_guard_ace_clear();

    old_ip_source_guard_conf_p = &ip_source_guard_global.ip_source_guard_conf;
    if (ip_source_guard_conf_changed(old_ip_source_guard_conf_p, &new_ip_source_guard_conf)) {
        changed = TRUE;
    }
    ip_source_guard_global.ip_source_guard_conf = new_ip_source_guard_conf;

    IP_SOURCE_GUARD_CRIT_EXIT();

    if (changed && create) {
        /* Apply all configuration to switch */
        ip_source_guard_conf_apply();
    }

    T_D("exit");
    return;
}

/* Module start */
static void ip_source_guard_start(void)
{
    ip_source_guard_conf_t *conf_p;

    T_D("enter");

    /* Initialize IP_SOURCE_GUARD configuration */
    conf_p = &ip_source_guard_global.ip_source_guard_conf;
    _ip_source_guard_default_set(conf_p);
    _ip_source_guard_default_set_dynamic_entries();

    /* Create semaphore for critical regions */
    critd_init(&ip_source_guard_global.crit, "ip_source_guard", VTSS_MODULE_ID_IP_SOURCE_GUARD, CRITD_TYPE_MUTEX);

    T_D("exit");
    return;
}

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize our private mib */
VTSS_PRE_DECLS void ip_source_guard_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_ip_source_guard_json_init(void);
#endif
extern "C" int ip_source_guard_icli_cmd_register();

/* Initialize module */
mesa_rc ip_source_guard_init(vtss_init_data_t *data)
{
#ifdef VTSS_SW_OPTION_ICFG
    mesa_rc     rc = VTSS_RC_OK;
#endif
    vtss_isid_t isid = data->isid;
    int         i;

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
        ip_source_guard_start();
#ifdef VTSS_SW_OPTION_ICFG
        if ((rc = ip_source_guard_icfg_init()) != VTSS_RC_OK) {
            T_D("Calling ip_source_guard_icfg_init() failed rc = %s", error_txt(rc));
        }
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register private mib */
        ip_source_guard_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_ip_source_guard_json_init();
#endif
        ip_source_guard_icli_cmd_register();
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            ip_source_guard_conf_read_stack(TRUE);
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");
        IP_SOURCE_GUARD_CRIT_ENTER();

        /* create data base for storing static entries */
        if (_ip_source_guard_mgmt_conf_create_static_db() != VTSS_RC_OK) {
            T_W("_ip_source_guard_mgmt_conf_create_static_db() failed");
        }

        /* create data base for storing dynamic entries */
        if (_ip_source_guard_mgmt_conf_create_dynamic_db() != VTSS_RC_OK) {
            T_W("_ip_source_guard_mgmt_conf_create_dynamic_db() failed");
        }

        IP_SOURCE_GUARD_CRIT_EXIT();

        /* Read stack and switch configuration */
        ip_source_guard_conf_read_stack(FALSE);

        IP_SOURCE_GUARD_CRIT_ENTER();
        /* Sync static entries to the efficient data base */
        for (i = 0; i < IP_SOURCE_GUARD_MAX_ENTRY_CNT; i++) {
            if (ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry[i].valid == TRUE) {
                if (_ip_source_guard_mgmt_conf_add_static_entry(&ip_source_guard_global.ip_source_guard_conf.ip_source_guard_static_entry[i], FALSE) != VTSS_RC_OK) {
                    T_W("_ip_source_guard_mgmt_conf_add_static_entry() failed");
                }
            }
        }
        IP_SOURCE_GUARD_CRIT_EXIT();
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");
        /* Apply all configuration to switch */
        ip_source_guard_conf_apply();
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

bool ipSourceGuardStaticIpMaskSupported::get()
{
    vtss_appl_ip_source_guard_capabilities_t cap;
    vtss_appl_ip_source_guard_capabilities_get(&cap);
    return cap.static_ip_mask_supported;
}

bool ipSourceGuardStaticMacAddressSupported::get()
{
    vtss_appl_ip_source_guard_capabilities_t cap;
    vtss_appl_ip_source_guard_capabilities_get(&cap);
    return cap.static_mac_address_supported;
}

/*
==============================================================================

    Public APIs in vtss_appl\include\vtss\appl\ip_source_guard.h

==============================================================================
*/
/**
 * \brief Get capabilities of IP Source Guard which determines function is supported or not.
 *
 * \param cap [OUT] Capabilities of IP Source Guard.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_capabilities_get(
    vtss_appl_ip_source_guard_capabilities_t    *const cap
)
{
    T_D("Enter\n");
    if (cap == NULL) {
        T_W("cap == NULL");
        return VTSS_RC_ERROR;
    }

    cap->static_ip_mask_supported       = FALSE;
    cap->static_mac_address_supported   = TRUE;
    T_D("Exit\n");
    return VTSS_RC_OK;
}


/**
 * \brief Get system configuration of IP Source Guard.
 *
 * \param conf [OUT] System configuration of IP Source Guard.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_system_config_get(
    vtss_appl_ip_source_guard_global_config_t    *const conf
)
{
    ulong   mode;

    /* check parameter */
    if ( conf == NULL ) {
        T_E("conf == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* get global configuration */
    if ( ip_source_guard_mgmt_conf_get_mode(&mode) != VTSS_RC_OK ) {
        T_E("ip_source_guard_mgmt_conf_get_mode()\n");
        return VTSS_RC_ERROR;
    }

    /* pack output */
    conf->mode = mode ? TRUE : FALSE;
    return VTSS_RC_OK;
}


/**
 * \brief Set or modify system configuration of IP Source Guard.
 *
 * \param conf [IN] System configuration of IP Source Guard.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_system_config_set(
    const vtss_appl_ip_source_guard_global_config_t  *const conf
)
{
    /* check parameter */
    if ( conf == NULL ) {
        T_E("conf == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* set global configuration */
    if ( ip_source_guard_mgmt_conf_set_mode(conf->mode) != VTSS_RC_OK ) {
        T_E("ip_source_guard_mgmt_conf_set_mode( %d )\n", conf->mode);
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}


/**
 * \brief Iterator function to allow iteration through all the port configuration.
 *
 * \param prev_ifindex [IN] Null-pointer to get first,
 *                                  otherwise the ifindex to get next of.
 * \param next_ifindex [OUT] First/next ifindex.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_port_config_itr(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
)
{
    return vtss_appl_iterator_ifindex_front_port(prev_ifindex, next_ifindex);
}


/**
 * \brief Get port configuration of IP Source Guard.
 *
 * \param ifindex     [IN] Interface index - the logical interface
 *                                index of the physical port.
 * \param port_conf  [OUT] The current configuration of the port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_port_config_get(
    vtss_ifindex_t  ifindex,
    vtss_appl_ip_source_guard_port_config_t  *const port_conf
)
{
    vtss_ifindex_elm_t                          ife;
    ip_source_guard_port_mode_conf_t            mode_conf;
    ip_source_guard_port_dynamic_entry_conf_t   entry_conf;

    /* check parameter */
    if (port_conf == NULL) {
        T_E("port_conf == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* get mode of port */
    if (ip_source_guard_mgmt_conf_get_port_mode(ife.isid, &mode_conf) != VTSS_RC_OK) {
        T_E("ip_source_guard_mgmt_conf_get_port_mode()\n");
        return VTSS_RC_ERROR;
    }

    /* get dynamic entry count of port */
    if (ip_source_guard_mgmt_conf_get_port_dynamic_entry_cnt(ife.isid, &entry_conf) != VTSS_RC_OK) {
        T_E("ip_source_guard_mgmt_conf_get_port_dynamic_entry_cnt()\n");
        return VTSS_RC_ERROR;
    }

    /* pack output */
    port_conf->mode = (mode_conf.mode[ife.ordinal]) ? TRUE : FALSE;
    port_conf->dynamic_entry_count = entry_conf.entry_cnt[ife.ordinal];

    return VTSS_RC_OK;
}


/**
 * \brief Set or modify port configuration of IP Source Guard.
 *
 * \param ifindex  [IN] Interface index - the logical interface index
 *                            of the physical port.
 * \param port_conf [IN] The configuration set to the port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_port_config_set(
    vtss_ifindex_t  ifindex,
    const vtss_appl_ip_source_guard_port_config_t   *const port_conf
)
{
    vtss_ifindex_elm_t                          ife;
    ip_source_guard_port_mode_conf_t            mode_conf;
    ip_source_guard_port_dynamic_entry_conf_t   entry_conf;

    /* check parameter */
    if ( port_conf == NULL ) {
        T_E("port_conf == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* get mode of port */
    if ( ip_source_guard_mgmt_conf_get_port_mode(ife.isid, &mode_conf) != VTSS_RC_OK ) {
        T_E("ip_source_guard_mgmt_conf_get_port_mode()\n");
        return VTSS_RC_ERROR;
    }

    /* set mode of port */
    mode_conf.mode[ife.ordinal] = port_conf->mode;
    if ( ip_source_guard_mgmt_conf_set_port_mode(ife.isid, &mode_conf) != VTSS_RC_OK ) {
        T_E("ip_source_guard_mgmt_conf_set_port_mode()\n");
        return VTSS_RC_ERROR;
    }

    /* get dynamic entry count of port */
    if ( ip_source_guard_mgmt_conf_get_port_dynamic_entry_cnt(ife.isid, &entry_conf) != VTSS_RC_OK ) {
        T_E("ip_source_guard_mgmt_conf_get_port_dynamic_entry_cnt()\n");
        return VTSS_RC_ERROR;
    }

    /* set dynamic entry count of port */
    entry_conf.entry_cnt[ife.ordinal] = port_conf->dynamic_entry_count;
    if ( ip_source_guard_mgmt_conf_set_port_dynamic_entry_cnt(ife.isid, &entry_conf) != VTSS_RC_OK ) {
        T_D("ip_source_guard_mgmt_conf_set_port_dynamic_entry_cnt()\n");
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}



/**
 * \brief Iterator function to allow iteration through all the static binding entries.
 *
 * \param prev      [IN] Null-pointer to get first,
 *                             otherwise the table index to get next of.
 * \param next      [OUT] First/next table index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc  vtss_appl_ip_source_guard_static_config_itr(
    const vtss_appl_ip_source_guard_static_index_t  *const prev,
    vtss_appl_ip_source_guard_static_index_t        *const next
)
{
    ip_source_guard_entry_t     entry;
    vtss_ifindex_elm_t          ife;
    T_D("Enter(%s):IFX-%u/VID-%u/ADR-%x/MSK-%x",
        prev ? "PREV" : "NULL",
        prev ? VTSS_IFINDEX_PRINTF_ARG(prev->ifindex) : 0,
        prev ? prev->vlan_id : 0,
        prev ? prev->ip_addr : 0,
        prev ? prev->ip_mask : 0);

    if ( next == NULL ) {
        return VTSS_RC_ERROR;
    }

    memset(&entry, 0, sizeof(entry));

    if (prev) {
        /*
            get next
        */

        /* get isid/iport from ifindex and validate them */
        if ( vtss_ifindex_decompose(prev->ifindex, &ife) != VTSS_RC_OK ||
             ife.iftype != VTSS_IFINDEX_TYPE_PORT                      ) {
            return VTSS_RC_ERROR;
        }

        /* key 1 */
        entry.isid    = ife.isid;
        entry.port_no = ife.ordinal;

        /* key 2 */
        entry.vid = prev->vlan_id;

        /* key 3 */
        entry.assigned_ip = prev->ip_addr;

        T_D("GET NEXT prev (%d, %d) vid %d ip 0x%x(0x%x)",
            ife.isid,
            ife.ordinal,
            entry.vid,
            entry.assigned_ip,
            entry.ip_mask);

        /* get next static entry */
        /*
            key 4 is not used, and thus we do GET and GET-NEXt approach
            for the case when given key 4 is not the expected MAX. value
        */
        if (prev->ip_mask != 0xFFFFFFFF) {
            if (ip_source_guard_mgmt_conf_get_static_entry(&entry) != VTSS_RC_OK &&
                ip_source_guard_mgmt_conf_get_next_static_entry(&entry) != VTSS_RC_OK) {
                return VTSS_RC_ERROR;
            }
        } else {
            if (ip_source_guard_mgmt_conf_get_next_static_entry(&entry) != VTSS_RC_OK) {
                return VTSS_RC_ERROR;
            }
        }
    } else {
        /*
            get first
        */
        T_D("GET FITST");
        if ( ip_source_guard_mgmt_conf_get_first_static_entry(&entry) != VTSS_RC_OK ) {
            return VTSS_RC_ERROR;
        }
    }

    /* key 1 */
    if ( vtss_ifindex_from_port(entry.isid, entry.port_no, &(next->ifindex)) != VTSS_RC_OK ) {
        T_E("Fail to get ifindex from (%u, %u)\n", entry.isid, entry.port_no);
        return VTSS_RC_ERROR;
    }

    /* key 2 */
    next->vlan_id = entry.vid;

    /* key 3 */
    next->ip_addr = entry.assigned_ip;

    /* forced ip mask as 255.255.255.255 for ACL_V2*/
    next->ip_mask = 0xffffffff;

    T_D("Exit(NXT): %u(%d, %d) vid %d ip 0x%x(0x%x)",
        VTSS_IFINDEX_PRINTF_ARG(next->ifindex),
        ife.isid,
        ife.ordinal,
        next->vlan_id,
        next->ip_addr,
        next->ip_mask);

    return VTSS_RC_OK;
}


/**
 * \brief Get static binding entry of IP Source Guard.
 *
 * \param static_index   [IN] Table index of static binding entry.
 * \param static_conf    [OUT] Data of static binding entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_static_config_get(
    const vtss_appl_ip_source_guard_static_index_t  *const static_index,
    vtss_appl_ip_source_guard_static_config_t       *const static_conf
)
{
    vtss_ifindex_elm_t          ife;
    ip_source_guard_entry_t     entry;
    vtss_ifindex_t              ifindex;

    if ( static_index == NULL ) {
        T_E("static_index == NULL\n");
        return VTSS_RC_ERROR;
    }

    if (static_index->ip_mask != 0xFFFFFFFF) {
        return VTSS_RC_ERROR;
    }

    /* check parameter */
    if ( static_conf == NULL ) {
        T_E("static_conf == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* clear staticConf */
    memset(static_conf, 0, sizeof(vtss_appl_ip_source_guard_static_config_t));

    ifindex = static_index->ifindex;
    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* clear entry to 0 */
    memset( &entry, 0, sizeof(entry) );

    /* key 1 */
    entry.isid    = ife.isid;
    entry.port_no = ife.ordinal;

    /* key 2 */
    entry.vid = static_index->vlan_id;

    /* key 3 */
    entry.assigned_ip = static_index->ip_addr;

    /* get static entry */
    if ( ip_source_guard_mgmt_conf_get_static_entry(&entry) != VTSS_RC_OK ) {
        return VTSS_RC_ERROR;
    }

    memcpy( &(static_conf->mac_addr), entry.assigned_mac, 6 );

    return VTSS_RC_OK;
}


/**
 * \brief Add or modify static binding entry of IP Source Guard.
 *
 * \param static_index   [IN] Table index of static binding entry.
 * \param static_conf    [IN] Data of static binding entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_static_config_set(
    const vtss_appl_ip_source_guard_static_index_t  *const static_index,
    const vtss_appl_ip_source_guard_static_config_t *const static_conf
)
{
    vtss_ifindex_elm_t          ife;
    ip_source_guard_entry_t     entry;
    vtss_ifindex_t              ifindex;
    mesa_ipv4_t                 ip_addr;

    /* check table data*/
    if ( static_conf == NULL ) {
        T_E("static_conf == NULL\n");
        return VTSS_RC_ERROR;
    }
    if (static_conf->mac_addr.addr[0] & 0x01) {
        T_D("Invalid MAC address (Multicast)!");
        return VTSS_RC_ERROR;
    }


    /* check table index */
    if ( static_index == NULL ) {
        T_E("static_index == NULL\n");
        return VTSS_RC_ERROR;
    }

    ip_addr = static_index->ip_addr;
    if (vtss_ipv4_addr_is_multicast(&ip_addr)) {
        T_D("Invalid IP address (Multicast)!");
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from ifindex and validate them */
    ifindex = static_index->ifindex;
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    if (static_index->ip_mask != 0xffffffff) {
        T_D("Invalid IP mask for ACL V2!");
        return VTSS_RC_ERROR;
    }

    /* clear entry to 0 */
    memset( &entry, 0, sizeof(entry) );

    /* set type to be static entry */
    entry.type  = IP_SOURCE_GUARD_STATIC_TYPE;
    entry.valid = TRUE;

    /* key 1 */
    entry.isid    = ife.isid;
    entry.port_no = ife.ordinal;

    /* key 2 */
    entry.vid = static_index->vlan_id;

    /* key 3 */
    entry.assigned_ip = static_index->ip_addr;

    /* get static entry */
    if ( ip_source_guard_mgmt_conf_get_static_entry(&entry) == VTSS_RC_OK ) {
        // exist then delete
        (void)ip_source_guard_mgmt_conf_del_static_entry( &entry );
    }

    memcpy( entry.assigned_mac, &(static_conf->mac_addr), 6 );

    /* set static entry */
    if ( ip_source_guard_mgmt_conf_set_static_entry(&entry) != VTSS_RC_OK ) {
        T_D("ip_source_guard_mgmt_conf_set_static_entry()\n");
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}


/**
 * \brief Delete static binding entry of IP Source Guard.
 *
 * \param static_index   [IN]    Table index of static binding entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_static_config_del(
    const vtss_appl_ip_source_guard_static_index_t  *const static_index
)
{
    vtss_ifindex_elm_t          ife;
    ip_source_guard_entry_t     entry;
    vtss_ifindex_t              ifindex;

    if ( static_index == NULL ) {
        T_E("static_index == NULL\n");
        return VTSS_RC_ERROR;
    }

    ifindex = static_index->ifindex;
    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* clear entry to 0 */
    memset( &entry, 0, sizeof(entry) );

    /* set type to be static entry */
    entry.type  = IP_SOURCE_GUARD_STATIC_TYPE;
    entry.valid = TRUE;

    /* key 1 */
    entry.isid    = ife.isid;
    entry.port_no = ife.ordinal;

    /* key 2 */
    entry.vid = static_index->vlan_id;

    /* key 3 */
    entry.assigned_ip = static_index->ip_addr;

    /* delete static entry */
    if ( ip_source_guard_mgmt_conf_del_static_entry(&entry) != VTSS_RC_OK ) {
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get default configuration for static binding entry of IP Source Guard.
 *
 * \param static_index  [OUT]   The default table index of static binding entry.
 * \param static_conf   [OUT]   The default configuration of static binding entry.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_ip_source_guard_static_config_default(
    vtss_appl_ip_source_guard_static_index_t        *const static_index,
    vtss_appl_ip_source_guard_static_config_t       *const static_conf
)
{
    if (!static_index || !static_conf) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    memset(static_index, 0, sizeof(vtss_appl_ip_source_guard_static_index_t));
    memset(static_conf, 0, sizeof(vtss_appl_ip_source_guard_static_config_t));

    static_index->vlan_id = VTSS_VID_DEFAULT;

    return VTSS_RC_OK;
}



/**
 * \brief Iterator function to allow iteration through all the dynamic binding entries.
 *
 * \param prev      [IN] Null-pointer to get first,
 *                             otherwise the table index to get next of.
 * \param next      [OUT] First/next table index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_dynamic_status_itr(
    const vtss_appl_ip_source_guard_dynamic_index_t  *const prev,
    vtss_appl_ip_source_guard_dynamic_index_t        *const next
)

{
    ip_source_guard_entry_t     entry;
    vtss_ifindex_elm_t          ife;
    T_D("Enter\n");
    if ( next == NULL) {
        return VTSS_RC_ERROR;
    }

    memset(&entry, 0, sizeof(entry));

    if ( prev) {
        /*
            get next
        */
        /* get isid/iport from ifindex and validate them */
        if ( vtss_ifindex_decompose(prev->ifindex, &ife) != VTSS_RC_OK ||
             ife.iftype != VTSS_IFINDEX_TYPE_PORT                      ) {
            return VTSS_RC_ERROR;
        }

        /* key 1 */
        entry.isid    = ife.isid;
        entry.port_no = ife.ordinal;


        /* key 2 */
        entry.vid = prev->vlan_id;


        /* key 3 */
        entry.assigned_ip = prev->ip_addr;

        T_D("GET NEXT prev (%d, %d) vid %d ip 0x%x\n",
            ife.isid,
            ife.ordinal,
            prev->vlan_id,
            prev->ip_addr);

        /* get next static entry */
        if ( ip_source_guard_mgmt_conf_get_next_dynamic_entry(&entry) != VTSS_RC_OK ) {
            return VTSS_RC_ERROR;
        }
    } else {
        /*
            get first
        */
        T_D("GET FITST\n");
        if ( ip_source_guard_mgmt_conf_get_first_dynamic_entry(&entry) != VTSS_RC_OK ) {
            return VTSS_RC_ERROR;
        }
    }

    /* key 1 */
    if ( vtss_ifindex_from_port(entry.isid, entry.port_no, &(next->ifindex)) != VTSS_RC_OK ) {
        T_E("Fail to get ifindex from (%u, %u)\n", entry.isid, entry.port_no);
        return VTSS_RC_ERROR;
    }

    /* key 2 */
    next->vlan_id = entry.vid;

    /* key 3 */
    next->ip_addr = entry.assigned_ip;

    T_D("next isid %d port_no %d vid %d assigned_ip 0x%x\n",
        ife.isid,
        ife.ordinal,
        next->vlan_id,
        next->ip_addr);

    T_D("Exit\n");

    return VTSS_RC_OK;
}


/**
 * \brief Get dynamic binding entry of IP Source Guard.
 *
 * \param dynamic_index [IN] Table index of dynamic binding entry.
 * \param dynamic_status    [OUT] Data of dynamic binding entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_dynamic_status_get(
    const vtss_appl_ip_source_guard_dynamic_index_t *const dynamic_index,
    vtss_appl_ip_source_guard_dynamic_status_t      *const dynamic_status
)
{
    vtss_ifindex_elm_t          ife;
    ip_source_guard_entry_t     entry;
    vtss_ifindex_t              ifindex;

    /* check parameter */
    if ( dynamic_status == NULL ) {
        T_E("status == NULL\n");
        return VTSS_RC_ERROR;
    }
    if ( dynamic_index == NULL ) {
        T_E("index == NULL\n");
        return VTSS_RC_ERROR;
    }

    ifindex = dynamic_index->ifindex;
    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* clear entry to 0 */
    memset( &entry, 0, sizeof(entry) );

    /* key 1 */
    entry.isid    = ife.isid;
    entry.port_no = ife.ordinal;

    /* key 2 */
    entry.vid = dynamic_index->vlan_id;

    /* key 3 */
    entry.assigned_ip = dynamic_index->ip_addr;

    /* get dynamic entry */
    if ( ip_source_guard_mgmt_conf_get_dynamic_entry(&entry) != VTSS_RC_OK ) {
        return VTSS_RC_ERROR;
    }

    /* clear status */
    memset( dynamic_status, 0, sizeof(vtss_appl_ip_source_guard_dynamic_status_t) );

    /* get assigned MAC address */
    memcpy( &(dynamic_status->mac_addr), entry.assigned_mac, 6 );

    return VTSS_RC_OK;
}

/**
 * \brief Get control action of IP Source Guard for translating dynamic entries to static.
 *
 * This action is active only when SET is involved.
 * It always returns FALSE when get this action data.
 *
 * \param action [OUT] The IP source guard action data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_control_translate_get(
    vtss_appl_ip_source_guard_control_translate_t   *const action
)
{
    /* check parameter */
    if ( action == NULL ) {
        T_E("action == NULL\n");
        return VTSS_RC_ERROR;
    }

    action->translate = FALSE;
    return VTSS_RC_OK;
}

/**
 * \brief Set control action of IP Source Guard for translating dynamic entries to static.
 *
 * Action denotes to translating dynamic binding entries to be static ones.
 * This action is active only when SET is involved and its value is set to be TRUE.
 * When it is active, it means translating process is taken action.
 *
 * \param action [IN] The IP source guard action data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_control_translate_set(
    const vtss_appl_ip_source_guard_control_translate_t     *const action
)
{
    /* check parameter */
    if ( action == NULL ) {
        T_E("action == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* Action !!! */
    if ( action->translate == TRUE ) {
        /* translate dynamic to static */
        if ( ip_source_guard_mgmt_conf_translate_dynamic_into_static() < 0 ) {
            T_E("ip_source_guard_mgmt_conf_translate_dynamic_into_static()\n");
            return VTSS_RC_ERROR;
        }
    }

    return VTSS_RC_OK;
}

