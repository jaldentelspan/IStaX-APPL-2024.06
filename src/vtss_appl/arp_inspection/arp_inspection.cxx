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

#include "main.h"
#include "msg_api.h"
#include "critd_api.h"
#include "vtss_common_iterator.hxx"
#include "misc_api.h"
#include "arp_inspection_api.h"
#include "arp_inspection.h"

#include "vtss_avl_tree_api.h"

#include "../ip/ip_api.h"
#include "ip_utils.hxx"
#include "packet_api.h"
#include "port_api.h"
#include "port_iter.hxx"
#include "acl_api.h"
#include "dhcp_snooping_api.h"
#include "vtss_os_wrapper_network.h"

#ifdef VTSS_SW_OPTION_ICFG
#include "arp_inspection_icfg.h"
#endif

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif

#include "vtss_bip_buffer_api.h"

/* JSON notification */
#include "vtss/basics/expose/table-status.hxx"  //For vtss::expose::TableStatus
#include "vtss/basics/memcmp-operator.hxx"      // For VTSS_BASICS_MEMCMP_OPERATOR

#include "ip_filter_api.hxx"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_ARP_INSPECTION


/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "arp_insp", "ARP_INSPECTION"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define ARP_INSPECTION_CRIT_ENTER()     critd_enter(&arp_inspection_global.crit,     __FILE__, __LINE__)
#define ARP_INSPECTION_CRIT_EXIT()      critd_exit( &arp_inspection_global.crit,     __FILE__, __LINE__)
#define ARP_INSPECTION_BIP_CRIT_ENTER() critd_enter(&arp_inspection_global.bip_crit, __FILE__, __LINE__)
#define ARP_INSPECTION_BIP_CRIT_EXIT()  critd_exit(&arp_inspection_global.bip_crit,  __FILE__, __LINE__)

#define ARP_INSPECTION_MAC_LENGTH       6
#define ARP_INSPECTION_BUF_LENGTH       40

/* Global structure */

/* BIP buffer event declaration */
#define ARP_INSPECTION_EVENT_PKT_RECV      0x00000001
#define ARP_INSPECTION_EVENT_ANY           ARP_INSPECTION_EVENT_PKT_RECV  /* Any possible bit */
static vtss_flag_t   arp_inspection_bip_buffer_thread_events;

/* BIP buffer Thread variables */
static vtss_handle_t arp_inspection_bip_buffer_thread_handle;
static vtss_thread_t arp_inspection_bip_buffer_thread_block;

/* BIP buffer data declaration */
#define ARP_INSPECTION_BIP_BUF_PKT_SIZE    1520  /* 4 bytes alignment */
#define ARP_INSPECTION_BIP_BUF_CNT         ARP_INSPECTION_FRAME_INFO_MAX_CNT

typedef struct {
    char            pkt[ARP_INSPECTION_BIP_BUF_PKT_SIZE];
    size_t          len;
    mesa_vid_t      vid;
    u16             dummy;
    vtss_isid_t     isid;
    mesa_port_no_t  port_no;
} arp_inspection_bip_buf_t;

#define ARP_INSPECTION_BIP_BUF_TOTAL_SIZE          (ARP_INSPECTION_BIP_BUF_CNT * sizeof(arp_inspection_bip_buf_t))
static vtss_bip_buffer_t arp_inspection_bip_buf;
static arp_inspection_global_t arp_inspection_global;

struct ARP_lock {
    ARP_lock(int line)
    {
        critd_enter(&arp_inspection_global.crit, __FILE__, line);
    }
    ~ARP_lock()
    {
        critd_exit( &arp_inspection_global.crit, __FILE__, 0);
    }
};

#define CRIT_SCOPE() ARP_lock __lock_guard__(__LINE__)


using namespace vtss::appl::ip::filter;
static Owner arp_inspection_rule_owner = { .module_id = VTSS_MODULE_ID_ARP_INSPECTION, .name = "arp_inspection" };


static i32 _arp_inspection_entry_compare_func(void *elm1, void *elm2);
static mesa_rc _vtss_appl_arp_inspection_set_crossed_event(void);
static mesa_rc _vtss_appl_arp_inspection_clear_crossed_event(void);


static mesa_rc vtss_appl_arp_inspection_filter_entry_add(arp_inspection_entry_t *entry);
static mesa_rc vtss_appl_arp_inspection_filter_entry_del(arp_inspection_entry_t *entry);
static mesa_rc vtss_appl_arp_inspection_filter_set(uint8_t enabled, PortMask untrusted_port_list);
static mesa_rc vtss_appl_arp_inspection_filter_update(void);
static mesa_rc vtss_appl_arp_inspection_filter_clear_all(void);

/* create library variables for data struct */
VTSS_AVL_TREE(static_arp_entry_list_avlt, "ARP_Inspection_static_avlt", VTSS_MODULE_ID_ARP_INSPECTION, _arp_inspection_entry_compare_func, ARP_INSPECTION_MAX_ENTRY_CNT)
static  int     static_arp_entry_list_created_done = 0;

VTSS_AVL_TREE(dynamic_arp_entry_list_avlt, "ARP_Inspection_dynamic_avlt", VTSS_MODULE_ID_ARP_INSPECTION, _arp_inspection_entry_compare_func, ARP_INSPECTION_MAX_ENTRY_CNT)
static  int     dynamic_arp_entry_list_created_done = 0;


/* packet rx filter */
static packet_rx_filter_t arp_inspection_rx_filter;
static void              *arp_inspection_filter_id = NULL; // Filter id for subscribing arp_inspection packet.

/* RX loopback on primary switch */
static vtss_isid_t primary_isid = VTSS_ISID_LOCAL;

/* JSON notification */
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_arp_inspection_status_event_t);
vtss::expose::StructStatus <
vtss::expose::ParamVal<vtss_appl_arp_inspection_status_event_t *>
> arp_inspection_status_event_update;

/****************************************************************************/
/*  Various local functions with no semaphore protection                    */
/****************************************************************************/

/* ARP_INSPECTION compare function */
static i32 _arp_inspection_entry_compare_func(void *elm1, void *elm2)
{
    arp_inspection_entry_t *element1, *element2;

    /* BODY
     */
    element1 = (arp_inspection_entry_t *)elm1;
    element2 = (arp_inspection_entry_t *)elm2;
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
    } else if (memcmp(element1->mac, element2->mac, ARP_INSPECTION_MAC_LENGTH * sizeof(u8)) > 0) {
        return 1;
    } else if (memcmp(element1->mac, element2->mac, ARP_INSPECTION_MAC_LENGTH * sizeof(u8)) < 0) {
        return -1;
    } else if (element1->assigned_ip > element2->assigned_ip) {
        return 1;
    } else if (element1->assigned_ip < element2->assigned_ip) {
        return -1;
    } else {
        return 0;
    }
}

/* Add ARP_INSPECTION static entry */
static mesa_rc _arp_inspection_mgmt_conf_add_static_entry(arp_inspection_entry_t *entry, BOOL allocated)
{
    arp_inspection_entry_t  *entry_p;
    mesa_rc                 rc = VTSS_RC_OK;
    int                     i;

    /* allocated memory */
    if (allocated) {
        /* Find an unused entry */
        for (i = 0; i < ARP_INSPECTION_MAX_ENTRY_CNT; i++) {
            if (arp_inspection_global.arp_inspection_conf.arp_inspection_static_entry[i].valid) {
                continue;
            }
            /* insert the entry on global memory for saving configuration */
            memcpy(&arp_inspection_global.arp_inspection_conf.arp_inspection_static_entry[i], entry, sizeof(arp_inspection_entry_t));
            entry_p = &(arp_inspection_global.arp_inspection_conf.arp_inspection_static_entry[i]);

            /* add the entry into static DB */
            if (vtss_avl_tree_add(&static_arp_entry_list_avlt, entry_p) != TRUE) {
                memset(&arp_inspection_global.arp_inspection_conf.arp_inspection_static_entry[i], 0x0, sizeof(arp_inspection_entry_t));
                T_D("add the entry into static DB failed");
                rc = ARP_INSPECTION_ERROR_DATABASE_ADD;
            }

            break;
        }
    } else {
        /* only insert the entry into link */
        if (vtss_avl_tree_add(&static_arp_entry_list_avlt, entry) != TRUE) {
            rc = ARP_INSPECTION_ERROR_DATABASE_ADD;
        }
    }

    return rc;
}

/* Delete ARP_INSPECTION static entry */
static mesa_rc _arp_inspection_mgmt_conf_del_static_entry(arp_inspection_entry_t *entry, BOOL free_node)
{
    arp_inspection_entry_t  *entry_p;
    mesa_rc                 rc = VTSS_RC_OK;
    int                     i;

    if (free_node) {
        /* Find the exist entry */
        for (i = 0; i < ARP_INSPECTION_MAX_ENTRY_CNT; i++) {
            if (!arp_inspection_global.arp_inspection_conf.arp_inspection_static_entry[i].valid) {
                continue;
            }

            if (_arp_inspection_entry_compare_func(&arp_inspection_global.arp_inspection_conf.arp_inspection_static_entry[i], entry) == 0) {
                entry_p = &(arp_inspection_global.arp_inspection_conf.arp_inspection_static_entry[i]);
                /* delete the entry on static DB */
                if (vtss_avl_tree_delete(&static_arp_entry_list_avlt, (void **) &entry_p) != TRUE) {
                    rc = ARP_INSPECTION_ERROR_DATABASE_DEL;
                } else {
                    /* clear global cache memory */
                    memset(&arp_inspection_global.arp_inspection_conf.arp_inspection_static_entry[i], 0x0, sizeof(arp_inspection_entry_t));
                }

                break;
            }
        }
    } else {
        /* only delete the entry on link */
        if (vtss_avl_tree_delete(&static_arp_entry_list_avlt, (void **) &entry_p) != TRUE) {
            rc = ARP_INSPECTION_ERROR_DATABASE_DEL;
        }
    }

    return rc;
}

/* Delete All ARP_INSPECTION static entry */
static mesa_rc _arp_inspection_mgmt_conf_del_all_static_entry(BOOL free_node)
{
    mesa_rc rc = VTSS_RC_OK;

    if (free_node) {
        vtss_avl_tree_destroy(&static_arp_entry_list_avlt);
        if (vtss_avl_tree_init(&static_arp_entry_list_avlt) != TRUE) {
            T_W("vtss_avl_tree_init() failed");
        }
        /* clear global cache memory */
        memset(arp_inspection_global.arp_inspection_conf.arp_inspection_static_entry, 0x0, ARP_INSPECTION_MAX_ENTRY_CNT * sizeof(arp_inspection_entry_t));
    } else {
        vtss_avl_tree_destroy(&static_arp_entry_list_avlt);
        if (vtss_avl_tree_init(&static_arp_entry_list_avlt) != TRUE) {
            T_W("vtss_avl_tree_init() failed");
        }
    }

    return rc;
}

/* Get ARP_INSPECTION static entry */
static mesa_rc _arp_inspection_mgmt_conf_get_static_entry(arp_inspection_entry_t *entry)
{
    mesa_rc                 rc = VTSS_RC_OK;
    arp_inspection_entry_t  *entry_p;

    entry_p = entry;
    if (vtss_avl_tree_get(&static_arp_entry_list_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET) != TRUE) {
        rc = ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND;
    } else {
        memcpy(entry, entry_p, sizeof(arp_inspection_entry_t));
    }

    return rc;
}

/* Get First ARP_INSPECTION static entry */
static mesa_rc _arp_inspection_mgmt_conf_get_first_static_entry(arp_inspection_entry_t *entry)
{
    mesa_rc                 rc = VTSS_RC_OK;
    arp_inspection_entry_t  *entry_p;

    entry_p = entry;
    if (vtss_avl_tree_get(&static_arp_entry_list_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET_FIRST) != TRUE) {
        rc = ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND;
    } else {
        memcpy(entry, entry_p, sizeof(arp_inspection_entry_t));
    }

    return rc;
}

/* Get Next ARP_INSPECTION static entry */
static mesa_rc _arp_inspection_mgmt_conf_get_next_static_entry(arp_inspection_entry_t *entry)
{
    mesa_rc                 rc = VTSS_RC_OK;
    arp_inspection_entry_t  *entry_p;

    entry_p = entry;
    if (vtss_avl_tree_get(&static_arp_entry_list_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET_NEXT) != TRUE) {
        rc = ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND;
    } else {
        memcpy(entry, entry_p, sizeof(arp_inspection_entry_t));
    }

    return rc;
}

/* Create ARP_INSPECTION static data base */
static mesa_rc _arp_inspection_mgmt_conf_create_static_db(void)
{
    mesa_rc rc = VTSS_RC_OK;

    if (!static_arp_entry_list_created_done) {
        /* create data base for storing static arp entry */
        if (vtss_avl_tree_init(&static_arp_entry_list_avlt)) {
            static_arp_entry_list_created_done = TRUE;
        } else {
            T_W("vtss_avl_tree_init() failed");
            rc = ARP_INSPECTION_ERROR_DATABASE_CREATE;
        }
    }

    return rc;
}

/* Add ARP_INSPECTION dynamic entry */
static mesa_rc _arp_inspection_mgmt_conf_add_dynamic_entry(arp_inspection_entry_t *entry, BOOL allocated)
{
    arp_inspection_entry_t  *entry_p;
    mesa_rc                 rc = VTSS_RC_OK;
    int                     i;

    /* allocated memory */
    if (allocated) {
        /* Find an unused entry */
        for (i = 0; i < ARP_INSPECTION_MAX_ENTRY_CNT; i++) {
            if (arp_inspection_global.arp_inspection_dynamic_entry[i].valid) {
                continue;
            }
            /* insert the entry on global memory */
            memcpy(&arp_inspection_global.arp_inspection_dynamic_entry[i], entry, sizeof(arp_inspection_entry_t));
            entry_p = &(arp_inspection_global.arp_inspection_dynamic_entry[i]);

            /* add the entry into dynamic DB */
            if (vtss_avl_tree_add(&dynamic_arp_entry_list_avlt, entry_p) != TRUE) {
                memset(&arp_inspection_global.arp_inspection_dynamic_entry[i], 0x0, sizeof(arp_inspection_entry_t));
                T_D("add the entry into dynamic DB failed");
                rc = ARP_INSPECTION_ERROR_DATABASE_ADD;
            }

            break;
        }
    } else {
        /* only insert the entry into link */
        if (vtss_avl_tree_add(&dynamic_arp_entry_list_avlt, entry) != TRUE) {
            rc = ARP_INSPECTION_ERROR_DATABASE_ADD;
        }
    }

    return rc;
}

/* Delete ARP_INSPECTION dynamic entry */
static mesa_rc _arp_inspection_mgmt_conf_del_dynamic_entry(arp_inspection_entry_t *entry, BOOL free_node)
{
    arp_inspection_entry_t  *entry_p;
    mesa_rc                 rc = VTSS_RC_OK;
    int                     i;

    if (free_node) {
        /* Find the exist entry */
        for (i = 0; i < ARP_INSPECTION_MAX_ENTRY_CNT; i++) {
            if (!arp_inspection_global.arp_inspection_dynamic_entry[i].valid) {
                continue;
            }

            if (_arp_inspection_entry_compare_func(&arp_inspection_global.arp_inspection_dynamic_entry[i], entry) == 0) {
                entry_p = &(arp_inspection_global.arp_inspection_dynamic_entry[i]);

                /* delete the entry on dynamic DB */
                if (vtss_avl_tree_delete(&dynamic_arp_entry_list_avlt, (void **) &entry_p) != TRUE) {
                    rc = ARP_INSPECTION_ERROR_DATABASE_DEL;
                } else {
                    /* clear global cache memory */
                    memset(&arp_inspection_global.arp_inspection_dynamic_entry[i], 0x0, sizeof(arp_inspection_entry_t));
                }

                break;
            }
        }
    } else {
        /* only delete the entry on link */
        if (vtss_avl_tree_delete(&dynamic_arp_entry_list_avlt, (void **) &entry_p) != TRUE) {
            rc = ARP_INSPECTION_ERROR_DATABASE_DEL;
        }
    }

    return rc;
}

/* Delete All ARP_INSPECTION dynamic entry */
static mesa_rc _arp_inspection_mgmt_conf_del_all_dynamic_entry(BOOL free_node)
{
    mesa_rc rc = VTSS_RC_OK;

    if (free_node) {
        vtss_avl_tree_destroy(&dynamic_arp_entry_list_avlt);
        if (vtss_avl_tree_init(&dynamic_arp_entry_list_avlt) != TRUE) {
            T_W("vtss_avl_tree_init() failed");
        }
        /* clear global cache memory */
        memset(arp_inspection_global.arp_inspection_dynamic_entry, 0x0, ARP_INSPECTION_MAX_ENTRY_CNT * sizeof(arp_inspection_entry_t));
    } else {
        vtss_avl_tree_destroy(&dynamic_arp_entry_list_avlt);
        if (vtss_avl_tree_init(&dynamic_arp_entry_list_avlt) != TRUE) {
            T_W("vtss_avl_tree_init() failed");
        }
    }

    return rc;
}

/* Get ARP_INSPECTION dynamic entry */
static mesa_rc _arp_inspection_mgmt_conf_get_dynamic_entry(arp_inspection_entry_t *entry)
{
    mesa_rc                 rc = VTSS_RC_OK;
    arp_inspection_entry_t  *entry_p;

    entry_p = entry;
    if (vtss_avl_tree_get(&dynamic_arp_entry_list_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET) != TRUE) {
        rc = ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND;
    } else {
        memcpy(entry, entry_p, sizeof(arp_inspection_entry_t));
    }

    return rc;
}

/* Get First ARP_INSPECTION dynamic entry */
static mesa_rc _arp_inspection_mgmt_conf_get_first_dynamic_entry(arp_inspection_entry_t *entry)
{
    mesa_rc                 rc = VTSS_RC_OK;
    arp_inspection_entry_t  *entry_p;

    entry_p = entry;
    if (vtss_avl_tree_get(&dynamic_arp_entry_list_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET_FIRST) != TRUE) {
        rc = ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND;
    } else {
        memcpy(entry, entry_p, sizeof(arp_inspection_entry_t));
    }

    return rc;
}

/* Get Next ARP_INSPECTION dynamic entry */
static mesa_rc _arp_inspection_mgmt_conf_get_next_dynamic_entry(arp_inspection_entry_t *entry)
{
    mesa_rc                 rc = VTSS_RC_OK;
    arp_inspection_entry_t  *entry_p;

    entry_p = entry;
    if (vtss_avl_tree_get(&dynamic_arp_entry_list_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET_NEXT) != TRUE) {
        rc = ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND;
    } else {
        memcpy(entry, entry_p, sizeof(arp_inspection_entry_t));
    }

    return rc;
}

/* Create ARP_INSPECTION dynamic data base */
static mesa_rc _arp_inspection_mgmt_conf_create_dynamic_db(void)
{
    mesa_rc rc = VTSS_RC_OK;

    if (!dynamic_arp_entry_list_created_done) {
        /* create data base for storing dynamic arp entry */
        if (vtss_avl_tree_init(&dynamic_arp_entry_list_avlt)) {
            dynamic_arp_entry_list_created_done = TRUE;
        } else {
            T_W("vtss_avl_tree_init() failed");
            rc = ARP_INSPECTION_ERROR_DATABASE_CREATE;
        }
    }

    return rc;
}

/* Translate ARP_INSPECTION dynamic entries into static entries */
static mesa_rc _arp_inspection_mgmt_conf_translate_dynamic_entry_into_static_entry(arp_inspection_entry_t *entry)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter");

    /* add the entry into static DB */
    if ((rc = _arp_inspection_mgmt_conf_add_static_entry(entry, TRUE)) != VTSS_RC_OK) {
        T_D("_arp_inspection_mgmt_conf_add_static_entry() failed");
        T_D("exit");
        return rc;
    } else {

        /* delete the entry on dynamic DB */
        if ((rc = _arp_inspection_mgmt_conf_del_dynamic_entry(entry, TRUE)) != VTSS_RC_OK) {
            T_D("_arp_inspection_mgmt_conf_del_dynamic_entry() failed");
        }
    }

    T_D("exit");
    return rc;
}

/* Save ARP_INSPECTION configuration */
static mesa_rc _arp_inspection_mgmt_conf_save_static_configuration(void)
{
    mesa_rc                     rc = VTSS_RC_OK;

    T_D("exit");
    return rc;
}

/* ARP_INSPECTION entry count */
static mesa_rc _arp_inspection_entry_count(void)
{
    mesa_rc rc = VTSS_RC_OK;
    int     i;

    /* Find the exist entry on static DB */
    for (i = 0; i < ARP_INSPECTION_MAX_ENTRY_CNT; i++) {
        if (!arp_inspection_global.arp_inspection_conf.arp_inspection_static_entry[i].valid) {
            continue;
        }
        rc++;
    }

    /* Find the exist entry on dynamic DB */
    for (i = 0; i < ARP_INSPECTION_MAX_ENTRY_CNT; i++) {
        if (!arp_inspection_global.arp_inspection_dynamic_entry[i].valid) {
            continue;
        }
        rc++;
    }

    return rc;
}

/* Get ARP_INSPECTION static count */
static mesa_rc _arp_inspection_get_static_count(void)
{
    mesa_rc rc = VTSS_RC_OK;
    int     i;

    /* Find the exist entry on static DB */
    for (i = 0; i < ARP_INSPECTION_MAX_ENTRY_CNT; i++) {
        if (!arp_inspection_global.arp_inspection_conf.arp_inspection_static_entry[i].valid) {
            continue;
        }
        rc++;
    }

    return rc;
}

static mesa_rc _arp_inspection_get_dynamic_count(void)
{
    mesa_rc rc = VTSS_RC_OK;
    int     i;

    /* Find the exist entry on dynamic DB */
    for (i = 0; i < ARP_INSPECTION_MAX_ENTRY_CNT; i++) {
        if (!arp_inspection_global.arp_inspection_dynamic_entry[i].valid) {
            continue;
        }
        rc++;
    }

    return rc;
}

#if ARP_INSPECTION_PROCESS_VLAN_TAGGING_ISSUE
/* Pack Utility */
static void pack16(u16 v, u8 *buf)
{
    buf[0] = (v >> 8) & 0xff;
    buf[1] = v & 0xff;
}
#endif

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/

/* Set ARP_INSPECTION defaults */
void arp_inspection_default_set(arp_inspection_conf_t *conf)
{
    int isid, j;

    memset(conf, 0x0, sizeof(*conf));
    conf->mode = ARP_INSPECTION_DEFAULT_MODE;

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        for (j = VTSS_PORT_NO_START; j < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); j++) {
            conf->port_mode_conf[isid - VTSS_ISID_START].mode[j]       = ARP_INSPECTION_DEFAULT_PORT_MODE;
            conf->port_mode_conf[isid - VTSS_ISID_START].check_VLAN[j] = ARP_INSPECTION_DEFAULT_PORT_VLAN_MODE;
            conf->port_mode_conf[isid - VTSS_ISID_START].log_type[j]   = ARP_INSPECTION_DEFAULT_LOG_TYPE;
        }
    }

    /* clear global cache memory for static entries */
    memset(conf->arp_inspection_static_entry, 0x0, ARP_INSPECTION_MAX_ENTRY_CNT * sizeof(arp_inspection_entry_t));
    T_D("clear static entries on global cache memory");

    /* clear JSON notification event */
    _vtss_appl_arp_inspection_clear_crossed_event();

    return;
}

/* Set ARP_INSPECTION defaults dynamic entry */
static void arp_inspection_default_set_dynamic_entry(void)
{
    /* clear global cache memory for dynamic entries */
    memset(arp_inspection_global.arp_inspection_dynamic_entry, 0x0, ARP_INSPECTION_MAX_ENTRY_CNT * sizeof(arp_inspection_entry_t));
    T_D("clear dynamic entries on global cache memory");

    return;
}

/****************************************************************************/
/*  Compare Function                                                        */
/****************************************************************************/
/* ARP_INSPECTION compare function */
static int arp_inspection_entry_compare_func(void *elm1, void *elm2)
{
    arp_inspection_entry_t *element1, *element2;

    /* BODY
     */
    element1 = (arp_inspection_entry_t *)elm1;
    element2 = (arp_inspection_entry_t *)elm2;
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
    } else if (memcmp(element1->mac, element2->mac, ARP_INSPECTION_MAC_LENGTH * sizeof(u8)) > 0) {
        return 1;
    } else if (memcmp(element1->mac, element2->mac, ARP_INSPECTION_MAC_LENGTH * sizeof(u8)) < 0) {
        return -1;
    } else if (element1->assigned_ip > element2->assigned_ip) {
        return 1;
    } else if (element1->assigned_ip < element2->assigned_ip) {
        return -1;
    } else {
        return 0;
    }
}



static mesa_rc arp_inspection_is_trusted(uint32_t isid, uint32_t iport, uint32_t vid)
{
    auto &port_conf = arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START];
    if (port_conf.mode[iport] == ARP_INSPECTION_MGMT_DISABLED) {
        // Arp inspection is disabled, i.e. (port,vlan) is trusted
        T_D("ARP inspection not enabled for port %d, i.e. port trusted\n", iport);
        return VTSS_RC_OK;
    }

    if (port_conf.check_VLAN[iport] == ARP_INSPECTION_MGMT_VLAN_DISABLED) {
        // Do arp inspection for all vlans, i.e. (port,vlan) is not trusted
        T_D("ARP inspection enabled for all vlans on port %d, i.e. port untrusted\n", iport);
        return VTSS_RC_ERROR;
    }

    /* Do arp inspection for some vlans */
    if (arp_inspection_global.arp_inspection_conf.vlan_mode_conf[vid].flags & ARP_INSPECTION_VLAN_MODE) {
        // VLAN in DB, i.e. (port,vlan) is not trusted
        T_D("ARP inspection enabled for vlan %d on port %d, i.e. (port,vid) untrusted\n", vid, iport);
        return VTSS_RC_ERROR;
    }
    // no checking for this vlan, i.e. (port,vlan) is trusted
    T_D("ARP inspection not enabled for vlan %d on port %d, i.e. (port,vid) trusted\n", vid, iport);
    return VTSS_RC_OK;
}

/* ARP_INSPECTION log entry */
static void arp_inspection_log_entry(uint32_t isid,
                                     uint32_t port_no,
                                     uint32_t vid,
                                     uint8_t *mac,
                                     mesa_ipv4_t assigned_ip,
                                     arp_inspection_log_type_t type)
{
#ifdef VTSS_SW_OPTION_SYSLOG
    char ip_txt[ARP_INSPECTION_BUF_LENGTH], mac_txt[ARP_INSPECTION_BUF_LENGTH];
    char syslog_txt[512], *syslog_txt_p;
#endif /* VTSS_SW_OPTION_SYSLOG */

    if (VTSS_RC_OK == arp_inspection_is_trusted(isid, port_no, vid)) {
        // Port is trusted, do not do arp inspection logging
        return;
    }

    CRIT_SCOPE();
    switch (type) {
    case ARP_INSPECTION_LOG_DENY:
        if (arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START].log_type[port_no] == ARP_INSPECTION_LOG_DENY ||
            arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START].log_type[port_no] == ARP_INSPECTION_LOG_ALL) {
#ifdef VTSS_SW_OPTION_SYSLOG
            syslog_txt_p = &syslog_txt[0];
            syslog_txt_p += sprintf(syslog_txt_p, "ARP_INSPECTION-ACCESS_DENIED: ARP packet is denied on");
            syslog_txt_p += sprintf(syslog_txt_p, " Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
            syslog_txt_p += sprintf(syslog_txt_p, ", vlan %u, mac %s, sip %s.",
                                    vid,
                                    misc_mac_txt(mac, mac_txt),
                                    misc_ipv4_txt(assigned_ip, ip_txt));
            S_PORT_N(isid, port_no, "%s", syslog_txt);
#endif /* VTSS_SW_OPTION_SYSLOG */
        }
        break;
    case ARP_INSPECTION_LOG_PERMIT:
        if (arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START].log_type[port_no] == ARP_INSPECTION_LOG_PERMIT ||
            arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START].log_type[port_no] == ARP_INSPECTION_LOG_ALL) {
#ifdef VTSS_SW_OPTION_SYSLOG
            syslog_txt_p = &syslog_txt[0];
            syslog_txt_p += sprintf(syslog_txt_p, "ARP_INSPECTION-ACCESS_DENIED: ARP packet is permitted on");
            syslog_txt_p += sprintf(syslog_txt_p, " Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
            syslog_txt_p += sprintf(syslog_txt_p, ", vlan %u, mac %s, sip %s.",
                                    vid,
                                    misc_mac_txt(mac, mac_txt),
                                    misc_ipv4_txt(assigned_ip, ip_txt));
            S_PORT_N(isid, port_no, "%s", syslog_txt);
#endif /* VTSS_SW_OPTION_SYSLOG */
        }
        break;
    default:
        break;
    }

    return;
}

/****************************************************************************/
/*  Reserved ACEs functions                                                 */
/****************************************************************************/

/* Add reserved ACE */
static mesa_rc arp_inspection_ace_add(void)
{
    mesa_rc             rc;
    acl_entry_conf_t    conf;
    ulong               arp_flag;

    if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_ARP, &conf)) != VTSS_RC_OK) {
        return rc;
    }
    conf.id = ARP_INSPECTION_ACE_ID;

    conf.isdx_disable = TRUE;

    conf.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
    memset(conf.action.port_list, 0, sizeof(conf.action.port_list));
    conf.action.inject_manual = TRUE;
    conf.action.inject_into_ip_stack = TRUE;
    conf.action.force_cpu = TRUE;
    conf.action.cpu_once = FALSE;
    conf.isid = VTSS_ISID_LOCAL;
    arp_flag = (ulong) ACE_FLAG_ARP_ARP;
    VTSS_BF_SET(conf.flags.mask, arp_flag, 1);
    arp_flag = (ulong) ACE_FLAG_ARP_ARP;
    VTSS_BF_SET(conf.flags.value, arp_flag, 1);
    arp_flag = (ulong) ACE_FLAG_ARP_UNKNOWN;
    VTSS_BF_SET(conf.flags.mask, arp_flag, 1);
    arp_flag = (ulong) ACE_FLAG_ARP_UNKNOWN;
    VTSS_BF_SET(conf.flags.value, arp_flag, 0);

    return (acl_mgmt_ace_add(ACL_USER_ARP_INSPECTION, ACL_MGMT_ACE_ID_NONE, &conf));
}

/* Delete reserved ACE */
static mesa_rc arp_inspection_ace_del(void)
{
    return (acl_mgmt_ace_del(ACL_USER_ARP_INSPECTION, ARP_INSPECTION_ACE_ID));
}

/****************************************************************************/
/*  ARP inspection Allocate functions                                       */
/****************************************************************************/

/* Allocate request buffer */
static arp_inspection_msg_req_t *arp_inspection_alloc_pkt_message(size_t size, arp_inspection_msg_id_t msg_id)
{
    arp_inspection_msg_req_t *msg = (arp_inspection_msg_req_t *)VTSS_MALLOC(size);

    if (msg) {
        msg->msg_id = msg_id;
    }
    T_D("msg len %zd, type %d => %p", size, msg_id, msg);

    return msg;
}

/* Allocate request buffer for sending packets */
static arp_inspection_msg_req_t *arp_inspection_alloc_message(size_t size, arp_inspection_msg_id_t msg_id)
{
    arp_inspection_msg_req_t *msg = (arp_inspection_msg_req_t *)VTSS_MALLOC(size);

    if (msg) {
        msg->msg_id = msg_id;
    }
    T_D("msg len %zd, type %d => %p", size, msg_id, msg);

    return msg;
}

/* Alloc memory for transmit ARP frame */
static u8 *arp_inspection_alloc_xmit(size_t len,
                                     unsigned long vid,
                                     unsigned long isid,
                                     BOOL *members,
                                     void **pbufref)
{
    u8 *p = NULL;

    if (msg_switch_is_local(isid)) {
        p = packet_tx_alloc(len);
        *pbufref = NULL;    /* Local operation */
    } else {                /* Remote */
        arp_inspection_msg_req_t *msg = arp_inspection_alloc_message(sizeof(arp_inspection_msg_req_t) + len, ARP_INSPECTION_MSG_ID_FRAME_TX_REQ);

        if (msg) {
            msg->req.tx_req.len = len;
            msg->req.tx_req.vid = vid;
            msg->req.tx_req.isid = isid;
            memcpy(msg->req.tx_req.port_list, members, fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) * sizeof(BOOL));
            *pbufref = (void *) msg; /* Remote op */
            p = ((unsigned char *) msg) + sizeof(*msg);
        } else {
            T_E("Allocation failure, TX length %zd", len);
        }
    }

    T_D("%s(%zd) ret %p", __FUNCTION__, len, p);

    return p;
}

/****************************************************************************/
/*  ARP inspection transmit functions                                       */
/****************************************************************************/

/* Transmit ARP frame
   Return 0  : Success
   Return -1 : Fail */
static int arp_inspection_xmit(u8 *frame,
                               size_t len,
                               unsigned long vid,
                               unsigned long isid,
                               BOOL *members,
                               void *bufref,
                               int is_relay)
{
    arp_inspection_msg_req_t    *msg = (arp_inspection_msg_req_t *)bufref;
    port_iter_t                 pit;
#if ARP_INSPECTION_PROCESS_VLAN_TAGGING_ISSUE
    mesa_packet_port_info_t     info;
    CapArray<mesa_packet_port_filter_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> filter;
#endif

    T_D("%s(%p, %zd, %ld, %ld)", __FUNCTION__, frame, len, isid, vid);

    if (msg) {
        if (msg_switch_is_local(msg->req.tx_req.isid)) {
            T_E("ISID became local (%ld)?", msg->req.tx_req.isid);
            VTSS_FREE(msg);
            return -1;
        } else {
            msg_tx(VTSS_MODULE_ID_ARP_INSPECTION,
                   msg->req.tx_req.isid, msg, len + sizeof(*msg));
        }
    } else {

        if (!frame) {
            T_W("no packet need to send");
            return 0;
        }

#if ARP_INSPECTION_PROCESS_VLAN_TAGGING_ISSUE
        // get port information by vid
        (void) mesa_packet_port_info_init(&info);
        info.vid = vid;
        (void) mesa_packet_port_filter_get(NULL, &info, filter.size(), filter.data());
#endif

        // transmit frame
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT);
        while (port_iter_getnext(&pit)) {
            if (members[pit.iport]) {
                uchar *buffer;

#if ARP_INSPECTION_PROCESS_VLAN_TAGGING_ISSUE
                T_D("VID %lu, filter %d, tpid %x", vid, filter[pit.iport].filter, filter[pit.iport].tpid);

                switch (filter[pit.iport].filter) {
                case MESA_PACKET_FILTER_TAGGED:
                    buffer = packet_tx_alloc(len + 4);
                    if (buffer) {
                        packet_tx_props_t   tx_props;
                        uchar               *frame_ptr = frame;
                        uchar               vlan_tag[4];

                        /* Fill out VLAN tag */
                        memset(vlan_tag, 0x0, sizeof(vlan_tag));
                        pack16(filter[pit.iport].tpid, vlan_tag);
                        pack16(vid, vlan_tag + 2);

                        /* process VLAN tagging issue */
                        memcpy(buffer, frame_ptr, 12); // DMAC & SMAC
                        memcpy(buffer + 12, vlan_tag, 4); // VLAN Header
                        memcpy(buffer + 12 + 4, frame_ptr + 12, len - 12); // Remainder of frame

                        packet_tx_props_init(&tx_props);
                        tx_props.packet_info.modid     = VTSS_MODULE_ID_ARP_INSPECTION;
                        tx_props.packet_info.frm       = buffer;
                        tx_props.packet_info.len       = len + 4;
                        tx_props.tx_info.dst_port_mask = VTSS_BIT64(pit.iport);
                        if (packet_tx(&tx_props) != VTSS_RC_OK) {
                            T_E("Frame transmit on port %d failed", pit.iport);
                            return -1;
                        }
                    } else {
                        T_W("allocation failure, length %zd", len + 4);
                    }
                    break;
                case MESA_PACKET_FILTER_UNTAGGED:
                    buffer = packet_tx_alloc(len);
                    if (buffer) {
                        packet_tx_props_t tx_props;
                        memcpy(buffer, frame, len);
                        packet_tx_props_init(&tx_props);
                        tx_props.packet_info.modid     = VTSS_MODULE_ID_ARP_INSPECTION;
                        tx_props.packet_info.frm       = buffer;
                        tx_props.packet_info.len       = len;
                        tx_props.tx_info.dst_port_mask = VTSS_BIT64(pit.iport);
                        if (packet_tx(&tx_props) != VTSS_RC_OK) {
                            T_E("Frame transmit on port %d failed", pit.iport);
                            return -1;
                        }
                    } else {
                        T_W("allocation failure, length %zd", len);
                    }
                    break;
                case MESA_PACKET_FILTER_DISCARD:
                    T_D("MESA_PACKET_FILTER_DISCARD");
                    break;
                default:
                    T_E("unknown ID: %d", filter[pit.iport].filter);
                    break;
                }
#else
                buffer = packet_tx_alloc(len);
                if (buffer) {
                    packet_tx_props_t tx_props;
                    memcpy(buffer, frame, len);
                    packet_tx_props_init(&tx_props);
                    tx_props.packet_info.modid     = VTSS_MODULE_ID_ARP_INSPECTION;
                    tx_props.packet_info.frm       = buffer;
                    tx_props.packet_info.len       = len;
                    tx_props.tx_info.dst_port_mask = VTSS_BIT64(pit.iport);
                    if (packet_tx(&tx_props) != VTSS_RC_OK) {
                        T_E("Frame transmit on port %ld failed", pit.iport);
                        return -1;
                    }
                } else {
                    T_W("allocation failure, length %zd", len);
                }
#endif
            }
        }

        // release packet
        if (frame) {
            packet_tx_free(frame);
        }
    }

    return 0;
}

/****************************************************************************/
/*  MSG/Debug Function                                                      */
/****************************************************************************/

#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
/* ARP_INSPECTION msg text */
static const char *arp_inspection_msg_id_txt(arp_inspection_msg_id_t msg_id)
{
    const char *txt;

    switch (msg_id) {
    case ARP_INSPECTION_MSG_ID_ARP_INSPECTION_CONF_SET_REQ:
        txt = "ARP_INSPECTION_MSG_ID_ARP_INSPECTION_CONF_SET_REQ";
        break;
    case ARP_INSPECTION_MSG_ID_FRAME_RX_IND:
        txt = "ARP_INSPECTION_MSG_ID_FRAME_RX_IND";
        break;
    case ARP_INSPECTION_MSG_ID_FRAME_TX_REQ:
        txt = "ARP_INSPECTION_MSG_ID_FRAME_TX_REQ";
        break;
    default:
        txt = "?";
        break;
    }
    return txt;
}
#endif /* VTSS_TRACE_LVL_DEBUG */

/* ARP_INSPECTION error text */
const char *arp_inspection_error_txt(mesa_rc rc)
{
    switch (rc) {
    case ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH:
        return "ARP Inspection: operation only valid on primary switch.";

    case ARP_INSPECTION_ERROR_ISID:
        return "ARP Inspection: invalid Switch ID.";

    case ARP_INSPECTION_ERROR_ISID_NON_EXISTING:
        return "Switch ID is non-existing";

    case ARP_INSPECTION_ERROR_INV_PARAM:
        return "ARP Inspection: invalid parameter supplied to function.";

    case ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND:
        return "ARP Inspection: databse access error.";

    case ARP_INSPECTION_ERROR_ENTRY_EXIST_ON_DB:
        return "ARP Inspection: the entry already exists in the database.";

    case ARP_INSPECTION_ERROR_TABLE_FULL:
        return "ARP Inspection: table is full.";

    default:
        return "ARP Inspection: unknown error code.";
    }
}

/****************************************************************************/
/*  Static Function                                                         */
/****************************************************************************/

/* Get ARP_INSPECTION static entry */
mesa_rc arp_inspection_mgmt_conf_static_entry_get(arp_inspection_entry_t *entry, BOOL next)
{
    mesa_rc rc = VTSS_RC_OK;

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not ready");
        T_D("exit");
        return ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    ARP_INSPECTION_CRIT_ENTER();
    if (next) {
        rc = _arp_inspection_mgmt_conf_get_next_static_entry(entry);
    } else {
        if (entry->vid == 0) {
            rc = _arp_inspection_mgmt_conf_get_first_static_entry(entry);
        } else {
            rc = _arp_inspection_mgmt_conf_get_static_entry(entry);
        }
    }
    ARP_INSPECTION_CRIT_EXIT();

    return rc;
}

/* Set ARP_INSPECTION static entry */
mesa_rc arp_inspection_mgmt_conf_static_entry_set(arp_inspection_entry_t *entry)
{
    mesa_rc     rc = VTSS_RC_OK;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not ready");
        T_D("exit");
        return ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check switch ID */
    if (!msg_switch_configurable(entry->isid)) {
        T_W("isid: %d isn't configurable switch", entry->isid);
        T_D("exit");
        return ARP_INSPECTION_ERROR_ISID_NON_EXISTING;
    }

    /* Check illegal parameter */
    // check vlan
    if (entry->vid >= VTSS_VIDS) {
        T_D("illegal vid: %u", entry->vid);
        T_D("exit");
        return ARP_INSPECTION_ERROR_INV_PARAM;
    }

    /* Check illegal parameter */
    // check mac address
    if (entry->mac[0] & 0x01) {
        T_D("Invalid MAC address (Multicast)!");
        T_D("exit");
        return ARP_INSPECTION_ERROR_INV_PARAM;
    }

    /* Check illegal parameter */
    // check ip address
    if (entry->assigned_ip == 0) {
        T_W("illegal ip: %u", entry->assigned_ip);
        T_D("exit");
        return ARP_INSPECTION_ERROR_INV_PARAM;
    }
    if (vtss_ipv4_addr_is_multicast(&entry->assigned_ip)) {
        T_D("Invalid IP address (Multicast)!");
        T_D("exit");
        return ARP_INSPECTION_ERROR_INV_PARAM;
    }
    /* IPMC/BCast check */
    if (((entry->assigned_ip >> 24) & 0xff) >= 224) {
        T_D("Invalid IP address (Multicast, Broadcast)!");
        T_D("exit");
        return ARP_INSPECTION_ERROR_INV_PARAM;
    }

    ARP_INSPECTION_CRIT_ENTER();

    /* Check the entry exist or not ? */
    if ((rc = _arp_inspection_mgmt_conf_get_dynamic_entry(entry)) != ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND) {
        //if existing, transfer the dynamic entry into static entry
        T_D("the entry existing on dynamic db, transfer, rc=%d", rc);
        if ((rc = _arp_inspection_mgmt_conf_translate_dynamic_entry_into_static_entry(entry)) == VTSS_RC_OK) {
            // save configuration
            rc = _arp_inspection_mgmt_conf_save_static_configuration();
        }
        ARP_INSPECTION_CRIT_EXIT();
        T_D("exit");
        return rc;
    }
    if ((rc = _arp_inspection_mgmt_conf_get_static_entry(entry)) != ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND) {
        //if existing, return the event
        T_D("the entry existing on static db, exit, rc=%d", rc);
        ARP_INSPECTION_CRIT_EXIT();
        T_D("exit");
        return ARP_INSPECTION_ERROR_ENTRY_EXIST_ON_DB;
    }

    /* Check total count reach the max value or not? */
    if (_arp_inspection_entry_count() >= ARP_INSPECTION_MAX_ENTRY_CNT) {
        T_D("total count, rc=%d", _arp_inspection_entry_count());
        ARP_INSPECTION_CRIT_EXIT();
        T_D("exit");
        T_W("arp inspection: table is full.(static)");
        // set json notification on crossed threshold event
        _vtss_appl_arp_inspection_set_crossed_event();
        return ARP_INSPECTION_ERROR_TABLE_FULL;
    }

    /*add in filter rx*/
    if ((rc = vtss_appl_arp_inspection_filter_entry_add(entry)) != VTSS_RC_OK) {
        T_D("vtss_appl_arp_inspection_filter_entry_add() failed");
    }

    /* add the entry into static DB */
    if ((rc = _arp_inspection_mgmt_conf_add_static_entry(entry, TRUE)) == VTSS_RC_OK) {
        /* save configuration */
        rc = _arp_inspection_mgmt_conf_save_static_configuration();
    }

    ARP_INSPECTION_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Del ARP_INSPECTION static entry */
mesa_rc arp_inspection_mgmt_conf_static_entry_del(arp_inspection_entry_t *entry)
{
    mesa_rc     rc = VTSS_RC_OK;
    vtss_appl_arp_inspection_status_event_t status;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not ready");
        T_D("exit");
        return ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check switch ID */
    if (!msg_switch_configurable(entry->isid)) {
        T_W("isid: %d isn't configurable switch", entry->isid);
        T_D("exit");
        return ARP_INSPECTION_ERROR_ISID_NON_EXISTING;
    }

    ARP_INSPECTION_CRIT_ENTER();

    /* Check if entry exist? */
    rc = _arp_inspection_mgmt_conf_get_static_entry(entry);
    if (rc == ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND) {
        //if not existing, return
        ARP_INSPECTION_CRIT_EXIT();
        T_D("exit");
        return rc;
    }

    /*delete from filter rx*/
    if ((rc = vtss_appl_arp_inspection_filter_entry_del(entry)) != VTSS_RC_OK) {
        T_D("vtss_appl_arp_inspection_filter_entry_del() failed");
    }

    /* delete the entry on static DB */
    if ((rc = _arp_inspection_mgmt_conf_del_static_entry(entry, TRUE)) != VTSS_RC_OK) {
        T_W("_arp_inspection_mgmt_conf_del_static_entry failed");
    } else {
        /* save configuration */
        rc = _arp_inspection_mgmt_conf_save_static_configuration();
    }

    /* for JSON notification */
    /* Check total count smaller than the max value or not? */
    arp_inspection_status_event_update.get(&status);
    if (status.crossed_maximum_entries) {
        if (_arp_inspection_entry_count() < ARP_INSPECTION_MAX_ENTRY_CNT) {
            _vtss_appl_arp_inspection_clear_crossed_event();
        }
    }

    ARP_INSPECTION_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Delete all ARP_INSPECTION static entry */
mesa_rc arp_inspection_mgmt_conf_all_static_entry_del(void)
{
    mesa_rc     rc = VTSS_RC_OK;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not ready");
        T_D("exit");
        return ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    ARP_INSPECTION_CRIT_ENTER();

    /* Check current entry count */
    if (!_arp_inspection_get_static_count()) {
        ARP_INSPECTION_CRIT_EXIT();
        T_D("exit");
        return VTSS_RC_OK;
    }

    /* Delete all static entries */
    if (_arp_inspection_mgmt_conf_del_all_static_entry(TRUE) != VTSS_RC_OK) {
        T_W("_arp_inspection_mgmt_conf_del_all_static_entry() failed");
    }

    /* save configuration */
    rc = _arp_inspection_mgmt_conf_save_static_configuration();

    ARP_INSPECTION_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Reset ARP_INSPECTION VLAN database */
mesa_rc arp_inspection_mgmt_conf_vlan_entry_del(void)
{
    mesa_rc     rc = VTSS_RC_OK;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not ready");
        T_D("exit");
        return ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    ARP_INSPECTION_CRIT_ENTER();

    /* clear global cache memory */
    memset(arp_inspection_global.arp_inspection_conf.vlan_mode_conf, 0x0, VTSS_VIDS * sizeof(arp_inspection_vlan_mode_conf_t));

    ARP_INSPECTION_CRIT_EXIT();

    /* save configuration */
    rc = arp_inspection_mgmt_conf_vlan_mode_save();

    T_D("exit");
    return rc;
}

/****************************************************************************/
/*  Dynamic Function                                                        */
/****************************************************************************/

/* Get ARP_INSPECTION dynamic count */
mesa_rc arp_inspection_mgmt_conf_dynamic_entry_count_get(void)
{
    mesa_rc rc = VTSS_RC_OK;

    ARP_INSPECTION_CRIT_ENTER();

    rc = _arp_inspection_get_dynamic_count();

    ARP_INSPECTION_CRIT_EXIT();

    return rc;
}

/* Get ARP_INSPECTION dynamic entry */
mesa_rc arp_inspection_mgmt_conf_dynamic_entry_get(arp_inspection_entry_t *entry, BOOL next)
{
    mesa_rc rc = VTSS_RC_OK;

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not ready");
        T_D("exit");
        return ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    ARP_INSPECTION_CRIT_ENTER();
    if (next) {
        rc = _arp_inspection_mgmt_conf_get_next_dynamic_entry(entry);
    } else {
        if (entry->vid == 0) {
            rc = _arp_inspection_mgmt_conf_get_first_dynamic_entry(entry);
        } else {
            rc = _arp_inspection_mgmt_conf_get_dynamic_entry(entry);
        }
    }
    ARP_INSPECTION_CRIT_EXIT();

    return rc;
}

/* Set ARP_INSPECTION dynamic entry */
mesa_rc arp_inspection_mgmt_conf_dynamic_entry_set(arp_inspection_entry_t *entry)
{
    mesa_rc     rc = VTSS_RC_OK;

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not ready");
        T_D("exit");
        return ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check switch ID */
    if (!msg_switch_exists(entry->isid)) {
        T_D("isid: %d not exist", entry->isid);
        T_D("exit");
        return ARP_INSPECTION_ERROR_ISID;
    }

    ARP_INSPECTION_CRIT_ENTER();

    /* Check system mode is enabled */
    if (arp_inspection_global.arp_inspection_conf.mode == ARP_INSPECTION_MGMT_DISABLED) {
        ARP_INSPECTION_CRIT_EXIT();
        T_D("exit");
        return rc;
    }

    /* Check port mode is enabled */
    if (arp_inspection_global.arp_inspection_conf.port_mode_conf[entry->isid - VTSS_ISID_START].mode[entry->port_no] == ARP_INSPECTION_MGMT_DISABLED) {
        ARP_INSPECTION_CRIT_EXIT();
        T_D("exit");
        return rc;
    }

    /* Check the entry exist or not ? */
    if ((rc = _arp_inspection_mgmt_conf_get_dynamic_entry(entry)) != ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND) {
        //if existing, return the event
        T_D("the entry existing on dynamic db, exit, rc=%d", rc);
        ARP_INSPECTION_CRIT_EXIT();
        T_D("exit");
        return ARP_INSPECTION_ERROR_ENTRY_EXIST_ON_DB;
    }
    if ((rc = _arp_inspection_mgmt_conf_get_static_entry(entry)) != ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND) {
        //if existing, return the event
        T_D("the entry existing on static db, exit, rc=%d", rc);
        ARP_INSPECTION_CRIT_EXIT();
        T_D("exit");
        return ARP_INSPECTION_ERROR_ENTRY_EXIST_ON_DB;
    }

    /* Check total count reach the max value or not? */
    if (_arp_inspection_entry_count() >= ARP_INSPECTION_MAX_ENTRY_CNT) {
        T_D("dynamic count, rc=%d", _arp_inspection_entry_count());
        ARP_INSPECTION_CRIT_EXIT();
        T_D("exit");
        T_W("arp inspection: table is full.(dynamic)");
        // set json notification on crossed threshold event
        _vtss_appl_arp_inspection_set_crossed_event();
        return ARP_INSPECTION_ERROR_TABLE_FULL;
    }

    /*add in filter rx*/
    if ((rc = vtss_appl_arp_inspection_filter_entry_add(entry)) != VTSS_RC_OK) {
        T_D("vtss_appl_arp_inspection_filter_entry_add() failed");
    }

    /* add the entry into dynamic DB */
    if ((rc = _arp_inspection_mgmt_conf_add_dynamic_entry(entry, TRUE)) != VTSS_RC_OK) {
        T_D("_arp_inspection_mgmt_conf_add_dynamic_entry() failed");
    }

    ARP_INSPECTION_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Delete ARP_INSPECTION dynamic entry */
mesa_rc arp_inspection_mgmt_conf_dynamic_entry_del(arp_inspection_entry_t *entry)
{
    mesa_rc     rc = VTSS_RC_OK;
    vtss_appl_arp_inspection_status_event_t status;

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not ready");
        T_D("exit");
        return ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check switch ID */
    if (!msg_switch_exists(entry->isid)) {
        T_D("isid: %d not exist", entry->isid);
        T_D("exit");
        return ARP_INSPECTION_ERROR_ISID;
    }

    ARP_INSPECTION_CRIT_ENTER();

    /* Check system mode is enabled */
    if (arp_inspection_global.arp_inspection_conf.mode == ARP_INSPECTION_MGMT_DISABLED) {
        ARP_INSPECTION_CRIT_EXIT();
        T_D("exit");
        return rc;
    }

    /* Check port mode is enabled */
    if (arp_inspection_global.arp_inspection_conf.port_mode_conf[entry->isid - VTSS_ISID_START].mode[entry->port_no] == ARP_INSPECTION_MGMT_DISABLED) {
        ARP_INSPECTION_CRIT_EXIT();
        T_D("exit");
        return rc;
    }

    /* Check if entry exist? */
    if (_arp_inspection_mgmt_conf_get_dynamic_entry(entry) == VTSS_RC_OK) {
        /*delete from filter rx*/
        if ((rc = vtss_appl_arp_inspection_filter_entry_del(entry)) != VTSS_RC_OK) {
            T_D("vtss_appl_arp_inspection_filter_entry_del() failed");
        }
    }

    /* delete the entry on dynamic DB */
    if ((rc = _arp_inspection_mgmt_conf_del_dynamic_entry(entry, TRUE)) != VTSS_RC_OK) {
        T_D("_arp_inspection_mgmt_conf_del_dynamic_entry() failed");
    }

    /* for JSON notification */
    /* Check total count smaller than the max value or not? */
    arp_inspection_status_event_update.get(&status);
    if (status.crossed_maximum_entries) {
        if (_arp_inspection_entry_count() < ARP_INSPECTION_MAX_ENTRY_CNT) {
            _vtss_appl_arp_inspection_clear_crossed_event();
        }
    }

    ARP_INSPECTION_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Check ARP_INSPECTION dynamic entry */
mesa_rc arp_inspection_mgmt_conf_dynamic_entry_check(arp_inspection_entry_t *check_entry)
{
    arp_inspection_entry_t      entry;

    memset(&entry, 0x0, sizeof(arp_inspection_entry_t));
    if (arp_inspection_mgmt_conf_dynamic_entry_get(&entry, FALSE) == VTSS_RC_OK) {

        if (arp_inspection_entry_compare_func(&entry, check_entry) == 0) {
            return VTSS_RC_OK;
        }
        while (arp_inspection_mgmt_conf_dynamic_entry_get(&entry, TRUE) == VTSS_RC_OK) {
            if (arp_inspection_entry_compare_func(&entry, check_entry) == 0) {
                return VTSS_RC_OK;
            }
        }
    }

    return VTSS_INCOMPLETE;
}

/* del all ARP_INSPECTION dynamic entry */
static void arp_inspection_mgmt_conf_all_dynamic_entry_del(void)
{
    ARP_INSPECTION_CRIT_ENTER();

    /* Delete all dynamic entries */
    if (_arp_inspection_mgmt_conf_del_all_dynamic_entry(TRUE) != VTSS_RC_OK) {
        T_W("_arp_inspection_mgmt_conf_del_all_dynamic_entry() failed");
    }

    ARP_INSPECTION_CRIT_EXIT();

    return;
}

/* flush ARP_INSPECTION dynamic entry by port */
static mesa_rc arp_inspection_mgmt_conf_flush_dynamic_entry_by_port(vtss_isid_t isid, mesa_port_no_t port_no)
{
    arp_inspection_entry_t      entry;

    memset(&entry, 0x0, sizeof(arp_inspection_entry_t));
    if (arp_inspection_mgmt_conf_dynamic_entry_get(&entry, FALSE) == VTSS_RC_OK) {
        if ((entry.isid == isid) && (entry.port_no == port_no)) {
            if (arp_inspection_mgmt_conf_dynamic_entry_del(&entry)) {
                T_W("arp_inspection_mgmt_conf_dynamic_entry_del() failed");
            }
        }
        while (arp_inspection_mgmt_conf_dynamic_entry_get(&entry, TRUE) == VTSS_RC_OK) {
            if ((entry.isid == isid) && (entry.port_no == port_no)) {
                if (arp_inspection_mgmt_conf_dynamic_entry_del(&entry)) {
                    T_W("arp_inspection_mgmt_conf_dynamic_entry_del() failed");
                }
            }
        }
    }

    return VTSS_RC_OK;
}

/* Translate ARP_INSPECTION dynamic entries into static entries */
mesa_rc arp_inspection_mgmt_conf_translate_dynamic_into_static(void)
{
    mesa_rc                     rc = VTSS_RC_OK;
    arp_inspection_entry_t      entry;
    int                         count = 0;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not ready");
        T_D("exit");
        return ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    ARP_INSPECTION_CRIT_ENTER();

    /* translate dynamic entries into static entries */
    if (_arp_inspection_mgmt_conf_get_first_dynamic_entry(&entry) == VTSS_RC_OK) {
        if ((rc = _arp_inspection_mgmt_conf_translate_dynamic_entry_into_static_entry(&entry)) != VTSS_RC_OK) {
            T_D("_arp_inspection_mgmt_conf_translate_dynamic_entry_into_static_entry() failed");
        } else {
            count++;
        }

        while (_arp_inspection_mgmt_conf_get_next_dynamic_entry(&entry) == VTSS_RC_OK) {
            if ((rc = _arp_inspection_mgmt_conf_translate_dynamic_entry_into_static_entry(&entry)) != VTSS_RC_OK) {
                T_D("_arp_inspection_mgmt_conf_translate_dynamic_entry_into_static_entry() failed");
            } else {
                count++;
            }
        }
    }

    /* save configuration */
    rc = _arp_inspection_mgmt_conf_save_static_configuration();

    ARP_INSPECTION_CRIT_EXIT();

    T_D("exit");
    if (rc < VTSS_RC_OK) {
        return rc;
    } else {
        return count;
    }
}

/* Translate ARP_INSPECTION dynamic entry into static entry */
mesa_rc arp_inspection_mgmt_conf_translate_dynamic_entry_into_static_entry(arp_inspection_entry_t *entry)
{
    mesa_rc                     rc = VTSS_RC_OK;
    int                         count = 0;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not ready");
        T_D("exit");
        return ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    ARP_INSPECTION_CRIT_ENTER();

    /* translate dynamic entry into static entry */
    if ((rc = _arp_inspection_mgmt_conf_translate_dynamic_entry_into_static_entry(entry)) != VTSS_RC_OK) {
        T_D("_arp_inspection_mgmt_conf_translate_dynamic_entry_into_static_entry() failed");
    } else {
        count++;
    }

    /* save configuration */
    rc = _arp_inspection_mgmt_conf_save_static_configuration();

    ARP_INSPECTION_CRIT_EXIT();

    T_D("exit");
    if (rc < VTSS_RC_OK) {
        return rc;
    } else {
        return count;
    }
}

/****************************************************************************/
/*  Callback Function                                                       */
/****************************************************************************/
static const uint8_t broadcast_mac[ARP_INSPECTION_MAC_LENGTH] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
static const uint8_t null_mac[ARP_INSPECTION_MAC_LENGTH] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static mesa_rc arp_inspection_ingress_check(const vtss_eth_header *eh,
                                            const vtss_arp_header *ar,
                                            uint32_t isid,
                                            uint32_t iport,
                                            uint32_t vid)
{
    mesa_rc         rc = VTSS_RC_OK;
    char            smac_txt[32];
    char            sha_txt[32];
    arp_inspection_entry_t entry;


    CRIT_SCOPE();
    if (VTSS_RC_OK == arp_inspection_is_trusted(isid, iport, vid)) {
        return VTSS_RC_OK;
    }
    if (memcmp(ar->sha, eh->smac.addr, ARP_INSPECTION_MAC_LENGTH) != 0) {
        // smac != sha
        T_D("smac (%s) != sha (%s)", misc_mac_txt(eh->smac.addr, smac_txt), misc_mac_txt(ar->sha, sha_txt));
        return VTSS_RC_ERROR;
    }

    if (memcmp(broadcast_mac, eh->dmac.addr, ARP_INSPECTION_MAC_LENGTH) != 0 &&
        memcmp(eh->dmac.addr, ar->tha, ARP_INSPECTION_MAC_LENGTH) != 0) {
        char dmac_buf[32];
        char tha_buf[32];
        T_D("In unicast arp, dmac (%s) is different from tha (%s)", misc_mac_txt(eh->dmac.addr, dmac_buf), misc_mac_txt(ar->tha, tha_buf));
        return VTSS_RC_ERROR;
    }

    memset(&entry, 0, sizeof(entry) );
    memcpy(entry.mac, ar->sha, ARP_INSPECTION_MAC_LENGTH * sizeof(uchar));
    entry.vid = vid;
    entry.assigned_ip = ar->spa != 0 ? htonl(ar->spa) : htonl(ar->tpa);
    entry.isid = isid;
    entry.port_no = iport;

    /* Check the entry exist or not ? */
    if ((rc = _arp_inspection_mgmt_conf_get_dynamic_entry(&entry)) != ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND) {
        //if existing, return the event
        T_D("the entry existing on dynamic db, exit, rc=%d", rc);
        return VTSS_RC_OK;
    }
    if ((rc = _arp_inspection_mgmt_conf_get_static_entry(&entry)) != ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND) {
        //if existing, return the event
        T_D("the entry existing on static db, exit, rc=%d", rc);
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

static mesa_rc arp_inspection_egress_check(const vtss_eth_header *eh,
                                           const vtss_arp_header *ar,
                                           uint32_t isid,
                                           uint32_t ingress_port,
                                           uint32_t egress_port,
                                           uint32_t vid)
{
    vtss_ifindex_t             ifindex;
    vtss_appl_port_status_t    port_module_status;
    mesa_packet_filter_t       filter = MESA_PACKET_FILTER_DISCARD;
    mesa_packet_frame_info_t   info;

    (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, egress_port, &ifindex);
    if (vtss_appl_port_status_get(ifindex, &port_module_status) != VTSS_RC_OK ||
        !port_module_status.link) {
        return VTSS_RC_ERROR;
    }

    /* Fill out frame information for filtering */
    mesa_packet_frame_info_init(&info);
    info.port_no = ingress_port;    /* Ingress port number or zero */
    info.vid = vid;
    info.port_tx = egress_port;

    if (mesa_packet_frame_filter(NULL, &info, &filter) != VTSS_RC_OK ||
        filter == MESA_PACKET_FILTER_DISCARD) {
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/* ARP_INSPECTION callback function for primary switch */
static void arp_inspection_do_rx_callback(const u8 *packet, size_t len, uint32_t vid, uint32_t isid, uint32_t port_no)
{
    mesa_rc                    rc = VTSS_RC_OK;
    void                       *bufref = NULL;
    uchar                      *pkt_buf;
    uchar                      *ptr = (uchar *)(packet);
    BOOL                       port_list[VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD];
    BOOL                       send_packet = FALSE;
    port_iter_t                pit;
    char                       debugtext[256];
    char                       *p_debugtext = debugtext;
    vtss_eth_header            *eh = (vtss_eth_header *)packet;
    vtss_arp_header            *ar = (vtss_arp_header *)(packet + sizeof(vtss_eth_header));
    /*
      A (port,vlanid) may be trusted or untrusted:

      - If ARP inspedtion not is enabled for the port, (port, vlanid)
      is trusted for any vlanid.

      - If ARP inspection is enabled for the port and VLAN checking
      not is enabled for the port, then (port, vlanid) is untrusted
      for any vlanid

      - If ARP inspection is enabled for the port and VLAN checking
      also is enabled for the port, then (port, vlanid) is trusted
      if vlanid is registered as a trusted vlan. Otherwise (port,
      vlanid) is untrusted.

      Ingress checking:

      - If (ingress port, vlanid) is trusted, then there is no more
      testing to do.

      - verify that smac == sha

      - If spa != 0.0.0.0 verify that sha is registered for (ingress
      portid, vlanid, spa) in the database

      - If spa == 0.0.0.0 (arp probing) verify that sha is registered
      for (ingress portid, vlanid, tpa) in the database.

      - If dmac != broadcast, (a unicast arp request or an arp reply)
      verify that dmac == tha

      Egress checking (for each port):

      - Verify there is link on the egress port

      - Verify that forwarding from ingress port to egress port for
      vlanid is allowed by mesa_packet_frame_filter

      Forward the frame on all ports where both ingress and egress checking has passed.

    */

    if (!VTSS_ISID_LEGAL(isid)) {
        return;
    }
    rc = arp_inspection_ingress_check(eh, ar, isid, port_no, vid);
    if (rc != VTSS_RC_OK) {
        arp_inspection_log_entry(isid, port_no, vid, ar->sha, ntohl(ar->spa), ARP_INSPECTION_LOG_DENY);
        return;
    }
    // If source address is existing, forwarding frames
    memset(port_list, 0x0, sizeof(port_list));
    send_packet = FALSE;

    (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT);
    while (port_iter_getnext(&pit)) {
        rc = arp_inspection_egress_check(eh, ar, isid, port_no, pit.iport, vid);
        if (VTSS_RC_OK == rc) {
            port_list[pit.iport] = TRUE;
            send_packet = TRUE;
            p_debugtext += sprintf(p_debugtext, "%d ", pit.iport);
        }
    }

    if (send_packet) {
        T_D("Forward arp rx port %d, tx ports %s", port_no, debugtext);
        /* Alloc memory for transmit ARP frame */
        if ((pkt_buf = arp_inspection_alloc_xmit(len, vid, VTSS_ISID_START, port_list, &bufref)) != NULL) {
            memcpy(pkt_buf, ptr, len);
            if (arp_inspection_xmit(pkt_buf, len, vid, VTSS_ISID_START, port_list, bufref, TRUE)) {
                T_W("arp_inspection_xmit() transmit failed");
            }
        }

    }
    // log here, permit log
    arp_inspection_log_entry(isid, port_no, vid, ar->sha, ntohl(ar->spa), ARP_INSPECTION_LOG_PERMIT);
}

static void arp_inspection_bip_buffer_enqueue(const u8 *const packet,
                                              size_t len,
                                              mesa_vid_t vid,
                                              vtss_isid_t isid,
                                              mesa_port_no_t port_no)
{
    arp_inspection_bip_buf_t    *bip_buf;
    int                         i, data_offset = 12; // DMAC + SMAC
    u16                         *vlan_hdr = (u16 *)(packet + data_offset);
    BOOL                        single_tagged = FALSE, double_tagged = FALSE;
    mesa_etype_t                tpid;

    if (vtss_appl_vlan_s_custom_etype_get(&tpid) != VTSS_RC_OK) {
        return;
    }

    /* Check input parameters */
    if (packet == NULL || len == 0) {
        return;
    }

    // Skip VLAN tags
    for (i = 0; i < 2; i++) {
        if (ntohs(*vlan_hdr) == 0x8100 || ntohs(*vlan_hdr) == 0x88A8 || ntohs(*vlan_hdr) == tpid) {
            if (single_tagged == FALSE) {
                single_tagged = TRUE;
            } else {
                single_tagged = TRUE;
                double_tagged = TRUE;
            }
            data_offset += 4;
            vlan_hdr = (u16 *)(packet + data_offset);
        }
    }

    ARP_INSPECTION_BIP_CRIT_ENTER();
    bip_buf = (arp_inspection_bip_buf_t *) vtss_bip_buffer_reserve(&arp_inspection_bip_buf, sizeof(*bip_buf));
    ARP_INSPECTION_BIP_CRIT_EXIT();
    if (bip_buf == NULL) {
        T_D("Failure in reserving DHCP Helper BIP buffer");
        return;
    }

    memcpy(bip_buf->pkt, packet, 12);
    if (single_tagged) {
        data_offset = 4;
    } else if (double_tagged) {
        data_offset = 8;
    } else {
        data_offset = 0;
    }
    memcpy(bip_buf->pkt + 12, packet + 12, len - 12 - data_offset);
    bip_buf->len     = len - data_offset;
    bip_buf->vid     = vid;
    bip_buf->isid    = isid;
    bip_buf->port_no = port_no;

    ARP_INSPECTION_BIP_CRIT_ENTER();
    vtss_bip_buffer_commit(&arp_inspection_bip_buf);
    vtss_flag_setbits(&arp_inspection_bip_buffer_thread_events, ARP_INSPECTION_EVENT_PKT_RECV);
    ARP_INSPECTION_BIP_CRIT_EXIT();
}

static void arp_inspection_bip_buffer_dequeue(void)
{
    arp_inspection_bip_buf_t   *bip_buf;
    int                        buf_size;
    static u32                 cnt = 0;

    do {
        ARP_INSPECTION_BIP_CRIT_ENTER();
        bip_buf = (arp_inspection_bip_buf_t *) vtss_bip_buffer_get_contiguous_block(&arp_inspection_bip_buf, &buf_size);
        ARP_INSPECTION_BIP_CRIT_EXIT();
        if (bip_buf) {
            arp_inspection_do_rx_callback((u8 *)bip_buf->pkt, bip_buf->len, bip_buf->vid, bip_buf->isid, bip_buf->port_no);
            ARP_INSPECTION_BIP_CRIT_ENTER();
            vtss_bip_buffer_decommit_block(&arp_inspection_bip_buf, sizeof(arp_inspection_bip_buf_t));
            ARP_INSPECTION_BIP_CRIT_EXIT();
        }

        // To avoid the busy-loop process that cannot access DUT
        if ((++cnt % 100) == 0) {
            cnt = 0;
            VTSS_OS_MSLEEP(10);
        }
    } while (bip_buf);
}

/****************************************************************************/
/*  ARP_INSPECTION receive functions                                        */
/****************************************************************************/

/* ARP_INSPECTION receive function for secondary switch */
static void arp_inspection_receive_indication(const u8 *packet,
                                              size_t len,
                                              mesa_port_no_t switchport,
                                              mesa_vid_t vid,
                                              mesa_glag_no_t glag_no)
{
    T_D("len %zd port %u vid %d glag %u", len, switchport, vid, glag_no);

    if (msg_switch_is_primary() && VTSS_ISID_LEGAL(primary_isid)) {   /* Bypass message module! */
        arp_inspection_bip_buffer_enqueue(packet, len, vid, primary_isid, switchport);
    } else {
        size_t msg_len = sizeof(arp_inspection_msg_req_t) + len;
        arp_inspection_msg_req_t *msg = arp_inspection_alloc_pkt_message(msg_len, ARP_INSPECTION_MSG_ID_FRAME_RX_IND);

        if (msg) {
            msg->req.rx_ind.len = len;
            msg->req.rx_ind.vid = vid;
            msg->req.rx_ind.port_no = switchport;
            memcpy(&msg[1], packet, len); /* Copy frame */
            // These frames are subject to shaping.
            msg_tx_adv(NULL, NULL, (msg_tx_opt_t)(MSG_TX_OPT_NO_ALLOC_ON_LOOPBACK | MSG_TX_OPT_SHAPE), VTSS_MODULE_ID_ARP_INSPECTION, 0, msg, msg_len);
        } else {
            T_W("Unable to allocate %zd bytes, tossing frame on port %u", msg_len, switchport);
        }
    }

    return;
}

/****************************************************************************/
/*  Rx filter register functions                                            */
/****************************************************************************/

/* Local port packet receive indication - forward through arp_inspection */
static BOOL arp_inspection_rx_packet_callback(void *contxt, const uchar *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    /* If this a secondary switch, use the message to pack the packet and then
       transmit to the primary switch */
    T_D("enter, port_no: %u len %d vid %d", rx_info->port_no, rx_info->length, rx_info->tag.vid);

    // NB: Null out the GLAG (port is 1st in aggr)
    arp_inspection_receive_indication(frm, rx_info->length, rx_info->port_no, rx_info->tag.vid,
                                      (mesa_glag_no_t)(VTSS_GLAG_NO_START - 1));

    T_D("exit");
    return TRUE; // Do not allow other subscribers to receive the packet
}

/* ARP_INSPECTION rx register function */
static void arp_inspection_rx_filter_register(BOOL registerd)
{
    ARP_INSPECTION_CRIT_ENTER();

    if (!arp_inspection_filter_id) {
        packet_rx_filter_init(&arp_inspection_rx_filter);
    }

    arp_inspection_rx_filter.modid = VTSS_MODULE_ID_ARP_INSPECTION;
    arp_inspection_rx_filter.match = PACKET_RX_FILTER_MATCH_ACL | PACKET_RX_FILTER_MATCH_ETYPE;
    arp_inspection_rx_filter.etype = 0x0806; // ARP
    arp_inspection_rx_filter.prio = PACKET_RX_FILTER_PRIO_NORMAL;
    arp_inspection_rx_filter.cb = arp_inspection_rx_packet_callback;

    if (registerd && !arp_inspection_filter_id) {
        if (packet_rx_filter_register(&arp_inspection_rx_filter, &arp_inspection_filter_id)) {
            T_W("packet_rx_filter_register() failed");
        }
    } else if (!registerd && arp_inspection_filter_id) {
        if (packet_rx_filter_unregister(arp_inspection_filter_id) == VTSS_RC_OK) {
            arp_inspection_filter_id = NULL;
        }
    }

    ARP_INSPECTION_CRIT_EXIT();

    return;
}

/****************************************************************************/
/*  Receive register functions                                              */
/****************************************************************************/
/* Register link status change callback */
static void arp_inspection_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    vtss_isid_t isid = VTSS_ISID_START;

    if (msg_switch_is_primary()) {
        T_D("port_no: [%d,%u] link %s", isid, port_no, status->link ? "up" : "down");
        if (!msg_switch_exists(isid)) { /* IP interface maybe change, don't send trap */
            return;
        }
        if (!status->link) {
            if (arp_inspection_mgmt_conf_flush_dynamic_entry_by_port(isid, port_no)) {
                T_W("arp_inspection_mgmt_conf_flush_dynamic_entry_by_port() failed");
            }
        }
    }

    return;
}

/* Register DHCP receive */
static void arp_inspection_dhcp_pkt_receive(dhcp_snooping_ip_assigned_info_t *info, dhcp_snooping_info_reason_t reason)
{
    arp_inspection_entry_t entry;

    memcpy(entry.mac, info->mac, ARP_INSPECTION_MAC_LENGTH);
    entry.vid = info->vid;
    entry.assigned_ip = info->assigned_ip;
    entry.isid = info->isid;
    entry.port_no = info->port_no;
    entry.type = ARP_INSPECTION_DYNAMIC_TYPE;

    if (reason == DHCP_SNOOPING_INFO_REASON_ASSIGN_COMPLETED) {
        entry.valid = TRUE;
        if (arp_inspection_mgmt_conf_dynamic_entry_set(&entry)) {
            T_D("arp_inspection_mgmt_conf_dynamic_entry_set() failed");
        }
    } else {
        entry.valid = FALSE;
        if (arp_inspection_mgmt_conf_dynamic_entry_del(&entry)) {
            T_D("arp_inspection_mgmt_conf_dynamic_entry_del() failed");
        }
    }
}

/* Register ARP Inspection receive */
static void arp_inspection_receive_register(void)
{
    uchar                               mac[ARP_INSPECTION_MAC_LENGTH], null_mac[ARP_INSPECTION_MAC_LENGTH] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    mesa_vid_t                          vid;
    dhcp_snooping_ip_assigned_info_t    info;
    arp_inspection_entry_t              entry;

    if (msg_switch_is_primary()) {

        memcpy(mac, null_mac, sizeof(mac));
        vid = 0;
        while (dhcp_snooping_ip_assigned_info_getnext(mac, vid, &info)) {
            memcpy(entry.mac, info.mac, ARP_INSPECTION_MAC_LENGTH);
            entry.vid = info.vid;
            entry.assigned_ip = info.assigned_ip;
            entry.isid = info.isid;
            entry.port_no = info.port_no;
            entry.type = ARP_INSPECTION_DYNAMIC_TYPE;
            entry.valid = TRUE;

            if (arp_inspection_mgmt_conf_dynamic_entry_set(&entry)) {
                T_D("arp_inspection_mgmt_conf_dynamic_entry_set() failed");
            }
            memcpy(mac, info.mac, ARP_INSPECTION_MAC_LENGTH);
            vid = info.vid;
        };

        dhcp_snooping_ip_assigned_info_register(arp_inspection_dhcp_pkt_receive);
    }

    arp_inspection_rx_filter_register(TRUE);
    if (arp_inspection_ace_add()) {
        T_W("arp_inspection_ace_add() failed");
    }
}

/* Unregister arp_inspection receive */
static void arp_inspection_receive_unregister(void)
{
    if (arp_inspection_ace_del()) {
        T_D("arp_inspection_ace_del() failed");
    }
    arp_inspection_rx_filter_register(FALSE);

    if (msg_switch_is_primary()) {
        dhcp_snooping_ip_assigned_info_unregister(arp_inspection_dhcp_pkt_receive);

        /* Clear all dynamic entries */
        arp_inspection_mgmt_conf_all_dynamic_entry_del();
    }
}

/****************************************************************************/
/*  MSG Function                                                            */
/****************************************************************************/

/* ARP_INSPECTION msg done */
static void arp_inspection_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
    arp_inspection_msg_id_t msg_id = *(arp_inspection_msg_id_t *)msg;
#endif /* VTSS_TRACE_LVL_DEBUG */

    T_D("msg_id: %d, %s", msg_id, arp_inspection_msg_id_txt(msg_id));
    vtss_sem_post((vtss_sem_t *)contxt);
}

/* ARP_INSPECTION msg tx */
static void arp_inspection_msg_tx(arp_inspection_msg_buf_t *buf, vtss_isid_t isid, size_t len)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
    arp_inspection_msg_id_t msg_id = *(arp_inspection_msg_id_t *)buf->msg;
#endif /* VTSS_TRACE_LVL_DEBUG */

    T_D("msg_id: %d, %s, len: %zd, isid: %d", msg_id, arp_inspection_msg_id_txt(msg_id), len, isid);
    msg_tx_adv(buf->sem, arp_inspection_msg_tx_done, MSG_TX_OPT_DONT_FREE, VTSS_MODULE_ID_ARP_INSPECTION, isid, buf->msg, len + MSG_TX_DATA_HDR_LEN(arp_inspection_msg_req_t, req));
}

/* ARP_INSPECTION msg rx */
static BOOL arp_inspection_msg_rx(void *contxt, const void *const rx_msg, size_t len, vtss_module_id_t modid, u32 isid)
{
    arp_inspection_msg_id_t     msg_id = *(arp_inspection_msg_id_t *)rx_msg;
    arp_inspection_msg_req_t    *msg = (arp_inspection_msg_req_t *)rx_msg;
    vtss_appl_port_status_t     port_status;
    port_iter_t                 pit;
    vtss_ifindex_t              ifindex;
#if ARP_INSPECTION_PROCESS_VLAN_TAGGING_ISSUE
    mesa_packet_port_info_t     info;
    CapArray<mesa_packet_port_filter_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> filter;
#endif

    switch (msg_id) {
    case ARP_INSPECTION_MSG_ID_ARP_INSPECTION_CONF_SET_REQ: {
        T_D("SET msg_id: %d, %s, len: %zd, isid: %u", msg_id, arp_inspection_msg_id_txt(msg_id), len, isid);
        if (msg->req.conf_set.conf.mode == ARP_INSPECTION_MGMT_ENABLED) {
            arp_inspection_receive_register();
        } else {
            arp_inspection_receive_unregister();
        }
        break;
    }
    case ARP_INSPECTION_MSG_ID_FRAME_RX_IND: {
        T_D("RX msg_id: %d, %s, len: %zd, isid: %u", msg_id, arp_inspection_msg_id_txt(msg_id), len, isid);
        arp_inspection_bip_buffer_enqueue((u8 *)&msg[1], msg->req.rx_ind.len, msg->req.rx_ind.vid, isid, msg->req.rx_ind.port_no);
        break;
    }
    case ARP_INSPECTION_MSG_ID_FRAME_TX_REQ: {
        T_D("TX msg_id: %d, %s, len: %zd, isid: %u, msg_isid: %lu", msg_id, arp_inspection_msg_id_txt(msg_id), msg->req.tx_req.len, isid, msg->req.tx_req.isid);

#if ARP_INSPECTION_PROCESS_VLAN_TAGGING_ISSUE
        // get port information by vid
        (void)mesa_packet_port_info_init(&info);
        info.vid = msg->req.tx_req.vid;
        (void)mesa_packet_port_filter_get(NULL, &info, filter.size(), filter.data());
#endif

        /* check which port needs to send the packet */
        (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT);
        while (port_iter_getnext(&pit)) {
            if (msg->req.tx_req.port_list[pit.iport]) {
                /* discard un-wanted vlan packet */
                if (filter[pit.iport].filter == MESA_PACKET_FILTER_DISCARD) {
                    continue;
                }

                /* check the port is link or not */
                (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, pit.iport, &ifindex);
                if (vtss_appl_port_status_get(ifindex, &port_status) != VTSS_RC_OK || !port_status.link) {
                    continue;
                }

#if ARP_INSPECTION_PROCESS_VLAN_TAGGING_ISSUE
                T_D("Secondary switch TX, port %u, VID %lu, filter %d, tpid %x", pit.iport, msg->req.tx_req.vid, filter[pit.iport].filter, filter[pit.iport].tpid);

                switch (filter[pit.iport].filter) {
                case MESA_PACKET_FILTER_TAGGED: {
                    void *frame = packet_tx_alloc(msg->req.tx_req.len + 4);
                    if (frame) {
                        packet_tx_props_t   tx_props;
                        void                *tr_msg = &msg[1];
                        uchar               *frame_ptr = (u8 *)tr_msg;
                        uchar               *buffer_ptr = (u8 *)frame;
                        uchar               vlan_tag[4];

                        /* Fill out VLAN tag */
                        memset(vlan_tag, 0x0, sizeof(vlan_tag));
                        pack16(filter[pit.iport].tpid, vlan_tag);
                        pack16(msg->req.tx_req.vid, vlan_tag + 2);

                        /* process VLAN tagging issue */
                        memcpy(buffer_ptr, frame_ptr, 12); // DMAC & SMAC
                        memcpy(buffer_ptr + 12, vlan_tag, 4); // VLAN Header
                        memcpy(buffer_ptr + 12 + 4, frame_ptr + 12, msg->req.tx_req.len - 12); // Remainder of frame

                        //memcpy(frame, &msg[1], msg->req.tx_req.len);
                        packet_tx_props_init(&tx_props);
                        tx_props.packet_info.modid     = VTSS_MODULE_ID_ARP_INSPECTION;
                        tx_props.packet_info.frm       = (u8 *)frame;
                        tx_props.packet_info.len       = msg->req.tx_req.len + 4;
                        tx_props.tx_info.dst_port_mask = VTSS_BIT64(pit.iport);
                        if (packet_tx(&tx_props) != VTSS_RC_OK) {
                            T_W("packet_tx() failed");
                        }
                    } else {
                        T_W("allocation failure, length %zd", msg->req.tx_req.len + 4);
                    }
                }
                break;
                case MESA_PACKET_FILTER_UNTAGGED: {
                    void *frame = packet_tx_alloc(msg->req.tx_req.len);
                    if (frame) {
                        packet_tx_props_t tx_props;
                        memcpy(frame, &msg[1], msg->req.tx_req.len);
                        packet_tx_props_init(&tx_props);
                        tx_props.packet_info.modid     = VTSS_MODULE_ID_ARP_INSPECTION;
                        tx_props.packet_info.frm       = (u8 *)frame;
                        tx_props.packet_info.len       = msg->req.tx_req.len;
                        tx_props.tx_info.dst_port_mask = VTSS_BIT64(pit.iport);
                        if (packet_tx(&tx_props) != VTSS_RC_OK) {
                            T_W("packet_tx() failed");
                        }
                    } else {
                        T_W("allocation failure, length %zd", msg->req.tx_req.len);
                    }
                }
                break;
                case MESA_PACKET_FILTER_DISCARD:
                    T_E("MESA_PACKET_FILTER_DISCARD");
                    break;
                default:
                    T_E("unknown ID: %d", filter[pit.iport].filter);
                    break;
                }
#else
                T_D("Secondary switch TX, port %lu, VID %lu", pit.iport, msg->req.tx_req.vid);

                void *frame = packet_tx_alloc(msg->req.tx_req.len);
                if (frame) {
                    packet_tx_props_t tx_props;
                    memcpy(frame, &msg[1], msg->req.tx_req.len);
                    packet_tx_props_init(&tx_props);
                    tx_props.packet_info.modid     = VTSS_MODULE_ID_ARP_INSPECTION;
                    tx_props.packet_info.frm       = frame;
                    tx_props.packet_info.len       = msg->req.tx_req.len;
                    tx_props.tx_info.dst_port_mask = VTSS_BIT64(pit.iport);
                    if (packet_tx(&tx_props) != VTSS_RC_OK) {
                        T_W("packet_tx() failed");
                    }
                } else {
                    T_W("allocation failure, length %zd", msg->req.tx_req.len);
                }
#endif
            }
        }

        break;
    }
    default:
        T_W("unknown message ID: %d", msg_id);
        break;
    }

    return TRUE;
}

/* Allocate request buffer */
static arp_inspection_msg_req_t *arp_inspection_msg_req_alloc(arp_inspection_msg_buf_t *buf, arp_inspection_msg_id_t msg_id)
{
    arp_inspection_msg_req_t *msg = &arp_inspection_global.request.msg;

    buf->sem = &arp_inspection_global.request.sem;
    buf->msg = msg;
    vtss_sem_wait(buf->sem);
    msg->msg_id = msg_id;

    return msg;
}

/****************************************************************************/
/*  Stack Register Function                                                 */
/****************************************************************************/

/* ARP_INSPECTION stack register */
static mesa_rc arp_inspection_stack_register(void)
{
    msg_rx_filter_t filter;

    memset(&filter, 0x0, sizeof(filter));
    filter.cb = arp_inspection_msg_rx;
    filter.modid = VTSS_MODULE_ID_ARP_INSPECTION;

    return msg_rx_filter_register(&filter);
}

/* Set stack ARP_INSPECTION configuration */
static void arp_inspection_stack_arp_inspection_conf_set(vtss_isid_t isid_add)
{
    arp_inspection_msg_req_t    *msg;
    arp_inspection_msg_buf_t    buf;
    switch_iter_t               sit;

    T_D("enter, isid_add: %d", isid_add);

    (void) switch_iter_init(&sit, isid_add, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        ARP_INSPECTION_CRIT_ENTER();
        msg = arp_inspection_msg_req_alloc(&buf, ARP_INSPECTION_MSG_ID_ARP_INSPECTION_CONF_SET_REQ);

        /* copy all configurations to stacking msg */
        //msg->req.conf_set.conf = arp_inspection_global.arp_inspection_conf;
        msg->req.conf_set.conf.mode = arp_inspection_global.arp_inspection_conf.mode;
        memcpy(msg->req.conf_set.conf.port_mode_conf, arp_inspection_global.arp_inspection_conf.port_mode_conf, VTSS_ISID_CNT * sizeof(arp_inspection_port_mode_conf_t));

        ARP_INSPECTION_CRIT_EXIT();

        arp_inspection_msg_tx(&buf, sit.isid, sizeof(msg->req.conf_set.conf));
    }

    T_D("exit, isid_add: %d", isid_add);
    return;
}

/****************************************************************************/
/*  Configuration Function                                                  */
/****************************************************************************/

/* Set ARP_INSPECTION configuration */
mesa_rc arp_inspection_mgmt_conf_mode_set(u32 *mode)
{
    mesa_rc rc      = VTSS_RC_OK;
    int     changed = FALSE;

    T_D("enter, mode: %d", *mode);

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not ready");
        T_D("exit");
        return ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check illegal parameter */
    if (*mode != ARP_INSPECTION_MGMT_ENABLED && *mode != ARP_INSPECTION_MGMT_DISABLED) {
        return ARP_INSPECTION_ERROR_INV_PARAM;
    }

    ARP_INSPECTION_CRIT_ENTER();
    if (arp_inspection_global.arp_inspection_conf.mode != *mode) {
        arp_inspection_global.arp_inspection_conf.mode = *mode;
        changed = TRUE;
    }
    ARP_INSPECTION_CRIT_EXIT();

    if (changed) {
        /* Activate changed configuration */
        arp_inspection_stack_arp_inspection_conf_set(VTSS_ISID_GLOBAL);

        if (vtss_appl_arp_inspection_filter_update() != VTSS_RC_OK) {
            T_D("update filter failed");
        }
    }

    T_D("exit");
    return rc;
}

/* Get ARP_INSPECTION configuration */
mesa_rc arp_inspection_mgmt_conf_mode_get(u32 *mode)
{
    T_D("enter");

    ARP_INSPECTION_CRIT_ENTER();
    *mode = arp_inspection_global.arp_inspection_conf.mode;
    ARP_INSPECTION_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/* Set ARP_INSPECTION configuration for port mode */
mesa_rc arp_inspection_mgmt_conf_port_mode_set(vtss_isid_t isid, arp_inspection_port_mode_conf_t *port_mode_conf)
{
    mesa_rc                             rc = VTSS_RC_OK;
    BOOL                                ports[VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD];
    port_iter_t                         pit;
    arp_inspection_port_mode_conf_t     *global_port_mode_ptr = NULL;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not ready");
        T_D("exit");
        return ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check switch ID */
    if (!msg_switch_configurable(isid)) {
        T_W("isid: %d isn't configurable switch", isid);
        T_D("exit");
        return ARP_INSPECTION_ERROR_ISID_NON_EXISTING;
    }
    /* Check illegal parameter */
    if (port_mode_conf == NULL) {
        T_W("illegal parameter");
        T_D("exit");
        return ARP_INSPECTION_ERROR_INV_PARAM;
    }

    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        if (port_mode_conf->mode[pit.iport] != ARP_INSPECTION_MGMT_ENABLED &&
            port_mode_conf->mode[pit.iport] != ARP_INSPECTION_MGMT_DISABLED) {
            return ARP_INSPECTION_ERROR_INV_PARAM;
        }
    }

    memset(ports, 0x0, sizeof(ports));

    // find which port needs to flush
    ARP_INSPECTION_CRIT_ENTER();
    global_port_mode_ptr = &arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START];
    if (memcmp(global_port_mode_ptr, port_mode_conf, sizeof(arp_inspection_port_mode_conf_t))) {

        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START].mode[pit.iport]  != port_mode_conf->mode[pit.iport]) {
                if (port_mode_conf->mode[pit.iport] == ARP_INSPECTION_MGMT_DISABLED) {
                    ports[pit.iport] = TRUE;
                }
                //arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START].mode[i] = port_mode_conf->mode[i];
            }
        }
    }
    ARP_INSPECTION_CRIT_EXIT();

    /* clear all dynamic entries with port disabled */
    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        if (ports[pit.iport]) {
            if (arp_inspection_mgmt_conf_flush_dynamic_entry_by_port(isid, pit.iport)) {
                T_W("arp_inspection_mgmt_conf_flush_dynamic_entry_by_port() failed");
            }
        }
    }

    ARP_INSPECTION_CRIT_ENTER();
    global_port_mode_ptr = &arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START];
    if (memcmp(global_port_mode_ptr, port_mode_conf, sizeof(arp_inspection_port_mode_conf_t))) {

        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            // set port mode
            if (arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START].mode[pit.iport] != port_mode_conf->mode[pit.iport]) {
                arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START].mode[pit.iport] = port_mode_conf->mode[pit.iport];
            }
            // set VLAN mode
            if (arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START].check_VLAN[pit.iport] != port_mode_conf->check_VLAN[pit.iport]) {
                arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START].check_VLAN[pit.iport] = port_mode_conf->check_VLAN[pit.iport];
            }
            // set port log type
            if (arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START].log_type[pit.iport] != port_mode_conf->log_type[pit.iport]) {
                arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START].log_type[pit.iport] = port_mode_conf->log_type[pit.iport];
            }
        }
    }
    ARP_INSPECTION_CRIT_EXIT();

    // update ip rx filter
    if (vtss_appl_arp_inspection_filter_update() != VTSS_RC_OK) {
        T_D("update filter failed");
    }

    T_D("exit");
    return rc;
}

/* Get ARP_INSPECTION configuration for port mode */
mesa_rc arp_inspection_mgmt_conf_port_mode_get(vtss_isid_t isid, arp_inspection_port_mode_conf_t *port_mode_conf)
{
    arp_inspection_port_mode_conf_t     *global_port_mode_ptr;

    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not ready");
        T_D("exit");
        return ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check switch ID */
    if (!msg_switch_configurable(isid)) {
        T_W("isid: %d isn't configurable switch", isid);
        T_D("exit");
        return ARP_INSPECTION_ERROR_ISID_NON_EXISTING;
    }
    /* Check illegal parameter */
    if (port_mode_conf == NULL) {
        T_W("illegal parameter");
        T_D("exit");
        return ARP_INSPECTION_ERROR_INV_PARAM;
    }

    ARP_INSPECTION_CRIT_ENTER();
    global_port_mode_ptr = &arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START];
    memcpy(port_mode_conf, global_port_mode_ptr, sizeof(arp_inspection_port_mode_conf_t));
    ARP_INSPECTION_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/* Set ARP_INSPECTION configuration for VLAN mode */
mesa_rc arp_inspection_mgmt_conf_vlan_mode_set(mesa_vid_t vid, arp_inspection_vlan_mode_conf_t *vlan_mode_conf)
{
    T_D("enter");

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not ready");
        T_D("exit");
        return ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check vid */
    if (vid >= VTSS_VIDS) {
        T_W("illegal vid: %u", vid);
        T_D("exit");
        return ARP_INSPECTION_ERROR_INV_PARAM;
    }
    /* Check illegal parameter */
    if (vlan_mode_conf == NULL) {
        T_W("illegal parameter");
        T_D("exit");
        return ARP_INSPECTION_ERROR_INV_PARAM;
    }

    ARP_INSPECTION_CRIT_ENTER();
    memcpy(&arp_inspection_global.arp_inspection_conf.vlan_mode_conf[vid], vlan_mode_conf, sizeof(arp_inspection_vlan_mode_conf_t));
    ARP_INSPECTION_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/* Get ARP_INSPECTION configuration for VLAN mode */
mesa_rc arp_inspection_mgmt_conf_vlan_mode_get(mesa_vid_t vid, arp_inspection_vlan_mode_conf_t *vlan_mode_conf, BOOL next)
{
    mesa_rc rc = VTSS_RC_OK;

    /* Check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not ready");
        T_D("exit");
        return ARP_INSPECTION_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    /* Check vid */
    if (vid >= VTSS_VIDS) {
        T_W("illegal vid: %u", vid);
        T_D("exit");
        return ARP_INSPECTION_ERROR_INV_PARAM;
    }
    /* Check illegal parameter */
    if (vlan_mode_conf == NULL) {
        T_W("illegal parameter");
        T_D("exit");
        return ARP_INSPECTION_ERROR_INV_PARAM;
    }

    ARP_INSPECTION_CRIT_ENTER();
    if (next) {
        vid++;
        if (vid >= VTSS_VIDS) {
            rc = ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND;
        } else {
            memcpy(vlan_mode_conf, &arp_inspection_global.arp_inspection_conf.vlan_mode_conf[vid], sizeof(arp_inspection_vlan_mode_conf_t));
        }
    } else {
        memcpy(vlan_mode_conf, &arp_inspection_global.arp_inspection_conf.vlan_mode_conf[vid], sizeof(arp_inspection_vlan_mode_conf_t));
    }
    ARP_INSPECTION_CRIT_EXIT();

    return rc;
}

/* Save ARP_INSPECTION configuration for VLAN mode */
mesa_rc arp_inspection_mgmt_conf_vlan_mode_save(void)
{

    T_D("exit");
    return VTSS_RC_OK;
}

/* Determine if ARP_INSPECTION configuration has changed */
static int arp_inspection_conf_changed(arp_inspection_conf_t *old, arp_inspection_conf_t *new_conf)
{
    return (memcmp(old, new_conf, sizeof(*new_conf)));
}

/* Read/create ARP_INSPECTION stack configuration */
static void arp_inspection_conf_read_stack(BOOL create)
{
    int                             changed;
    static arp_inspection_conf_t    new_arp_inspection_conf;
    arp_inspection_conf_t           *old_arp_inspection_conf_p;

    T_D("enter, create: %d", create);

    changed = FALSE;

    ARP_INSPECTION_CRIT_ENTER();

    /* Use default values */
    arp_inspection_default_set(&new_arp_inspection_conf);
    /* Delete all static entries */
    if (_arp_inspection_mgmt_conf_del_all_static_entry(TRUE) != VTSS_RC_OK) {
        T_W("_arp_inspection_mgmt_conf_del_all_static_entry() failed");
    }

    old_arp_inspection_conf_p = &arp_inspection_global.arp_inspection_conf;
    if (arp_inspection_conf_changed(old_arp_inspection_conf_p, &new_arp_inspection_conf)) {
        changed = TRUE;
    }
    arp_inspection_global.arp_inspection_conf = new_arp_inspection_conf;

    /* Delete all dynamic entries */
    if (_arp_inspection_mgmt_conf_del_all_dynamic_entry(TRUE) != VTSS_RC_OK) {
        T_W("_arp_inspection_mgmt_conf_del_all_dynamic_entry() failed");
    }

    ARP_INSPECTION_CRIT_EXIT();

    if (changed && create) {
        /* Apply all configuration to switch */
        arp_inspection_stack_arp_inspection_conf_set(VTSS_ISID_GLOBAL);
    }

    vtss_appl_arp_inspection_filter_clear_all();

    T_D("exit");
    return;
}

static void arp_inspection_bip_buffer_thread(vtss_addrword_t data)
{
    vtss_flag_value_t events;

    while (1) {
        if (msg_switch_is_primary()) {
            while (msg_switch_is_primary()) {
                events = vtss_flag_wait(&arp_inspection_bip_buffer_thread_events,
                                        ARP_INSPECTION_EVENT_ANY,
                                        VTSS_FLAG_WAITMODE_OR_CLR);
                if (events & ARP_INSPECTION_EVENT_PKT_RECV) {
                    arp_inspection_bip_buffer_dequeue();
                }
            } //while(msg_switch_is_primary())
        } //if(msg_switch_is_primary())

        // No reason for using CPU ressources when we're a secondary switch
        T_D("Suspending DHCP helper bip buffer thread");
        ARP_INSPECTION_BIP_CRIT_ENTER();
        vtss_bip_buffer_clear(&arp_inspection_bip_buf);
        ARP_INSPECTION_BIP_CRIT_EXIT();
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_ARP_INSPECTION);
        T_D("Resumed DHCP helper bip buffer thread");
    } //while(1)
}

/****************************************************************************/
/*  Start functions                                                         */
/****************************************************************************/

/* Module start */
static void arp_inspection_start(BOOL init)
{
    arp_inspection_conf_t *conf_p;

    T_D("enter, init: %d", init);

    if (init) {
        /* Initialize ARP_INSPECTION configuration */
        conf_p = &arp_inspection_global.arp_inspection_conf;
        arp_inspection_default_set(conf_p);
        arp_inspection_default_set_dynamic_entry();

        /* Initialize message buffers */
        vtss_sem_init(&arp_inspection_global.request.sem, 1);

        /* Initialize BIP buffer */
        if (!vtss_bip_buffer_init(&arp_inspection_bip_buf, ARP_INSPECTION_BIP_BUF_TOTAL_SIZE)) {
            T_E("vtss_bip_buffer_init failed!");
        }

        /* Initialize counter flag */
        vtss_flag_init(&arp_inspection_bip_buffer_thread_events);

        /* Create semaphore for critical regions */
        critd_init(&arp_inspection_global.crit,     "arp_inspection",     VTSS_MODULE_ID_ARP_INSPECTION, CRITD_TYPE_MUTEX);
        critd_init(&arp_inspection_global.bip_crit, "arp_inspection.bip", VTSS_MODULE_ID_ARP_INSPECTION, CRITD_TYPE_MUTEX);

        /* Create ARP Inspection bip buffer thread */
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           arp_inspection_bip_buffer_thread,
                           0,
                           "ARP inspection bip buffer",
                           nullptr,
                           0,
                           &arp_inspection_bip_buffer_thread_handle,
                           &arp_inspection_bip_buffer_thread_block);

    } else {
        /* Register for stack messages */
        if (arp_inspection_stack_register()) {
            T_W("arp_inspection_stack_register() failed");
        }
        if (port_change_register(VTSS_MODULE_ID_ARP_INSPECTION, arp_inspection_state_change_callback)) {
            T_E("port_change_register() failed");
        }
    }

    T_D("exit");
    return;
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void arp_inspection_mib_init(void);
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_arp_inspection_json_init(void);
#endif
extern "C" int arp_inspection_icli_cmd_register();

/* Initialize module */
mesa_rc arp_inspection_init(vtss_init_data_t *data)
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
        arp_inspection_start(TRUE);
#ifdef VTSS_SW_OPTION_ICFG
        if ((rc = arp_inspection_icfg_init()) != VTSS_RC_OK) {
            T_D("Calling arp_inspection_icfg_init() failed rc = %s", error_txt(rc));
        }
#endif
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        arp_inspection_mib_init();  /* Register ARP Inspection private mib */
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_arp_inspection_json_init();
#endif
        arp_inspection_icli_cmd_register();
        break;

    case INIT_CMD_START:
        T_D("START");
        arp_inspection_start(FALSE);
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            arp_inspection_conf_read_stack(TRUE);
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");
        ARP_INSPECTION_CRIT_ENTER();

        /* create data base for storing static arp entry */
        if (_arp_inspection_mgmt_conf_create_static_db() != VTSS_RC_OK) {
            T_W("_arp_inspection_mgmt_conf_create_static_db() failed");
        }

        /* create data base for storing dynamic arp entry */
        if (_arp_inspection_mgmt_conf_create_dynamic_db() != VTSS_RC_OK) {
            T_W("_arp_inspection_mgmt_conf_create_dynamic_db() failed");
        }

        ARP_INSPECTION_CRIT_EXIT();

        /* Read stack and switch configuration */
        arp_inspection_conf_read_stack(FALSE);

        ARP_INSPECTION_CRIT_ENTER();
        /* sync static arp entries to the efficient data base */
        for (i = 0; i < ARP_INSPECTION_MAX_ENTRY_CNT; i++) {
            if (arp_inspection_global.arp_inspection_conf.arp_inspection_static_entry[i].valid == TRUE) {
                if (_arp_inspection_mgmt_conf_add_static_entry(&(arp_inspection_global.arp_inspection_conf.arp_inspection_static_entry[i]), FALSE) != VTSS_RC_OK) {
                    T_W("_arp_inspection_mgmt_conf_add_static_entry() failed");
                }
            }
        }

        ARP_INSPECTION_CRIT_EXIT();
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");
        if (msg_switch_is_primary()) {
            if (msg_switch_is_local(isid)) {
                primary_isid = isid;
            }
        }

        /* Apply all configuration to switch */
        arp_inspection_stack_arp_inspection_conf_set(isid);
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

mesa_rc arp_inspection_mgmt_vlan_conf_index_get_next(
    const mesa_vid_t    *const vidx,
    mesa_vid_t          *const vnxt
)
{
    mesa_vid_t                      vid;
    arp_inspection_vlan_mode_conf_t *p;

    if (!msg_switch_is_primary() ||
        !vnxt) {
        return VTSS_RC_ERROR;
    }

    if (vidx && *vidx) {    /* Next */
        if (*vidx >= VTSS_VIDS ||
            (vid = *vidx + 1) >= VTSS_VIDS) {
            return ARP_INSPECTION_ERROR_INV_PARAM;
        }
    } else {                /* First */
        vid = VTSS_VID_NULL + 1;
    }

    ARP_INSPECTION_CRIT_ENTER();
    while (vid < VTSS_VIDS) {
        p = &arp_inspection_global.arp_inspection_conf.vlan_mode_conf[vid];
        if (p->flags & ARP_INSPECTION_VLAN_MODE) {
            ARP_INSPECTION_CRIT_EXIT();
            *vnxt = vid;
            return VTSS_RC_OK;
        }

        vid++;
    }
    ARP_INSPECTION_CRIT_EXIT();

    return ARP_INSPECTION_ERROR_DATABASE_NOT_FOUND;
}

/*****************************************************************************
    Public API section for ARP Inspection
    from vtss_appl/include/vtss/appl/arp_inspection.h
*****************************************************************************/
static mesa_rc _vtss_appl_arp_inspection_entry_itr(
    const BOOL                              static_entry,
    const vtss_ifindex_t                    *const ifx_prev,
    vtss_ifindex_t                          *const ifx_next,
    const mesa_vid_t                        *const vid_prev,
    mesa_vid_t                              *const vid_next,
    const mesa_mac_t                        *const mac_prev,
    mesa_mac_t                              *const mac_next,
    const mesa_ipv4_t                       *const ipa_prev,
    mesa_ipv4_t                             *const ipa_next
)
{
    vtss_ifindex_elm_t      ifex;
    mesa_vid_t              vidx;
    mesa_mac_t              macx;
    mesa_ipv4_t             ipax;
    arp_inspection_entry_t  entry;
    mesa_rc                 rc = VTSS_RC_ERROR;

    if (!ifx_next || !vid_next || !mac_next || !ipa_next) {
        return rc;
    }

    if (ifx_prev) {
        /* get isid/iport from given IfIndex */
        if (vtss_ifindex_decompose(*ifx_prev, &ifex) != VTSS_RC_OK ||
            ifex.iftype != VTSS_IFINDEX_TYPE_PORT) {
            return rc;
        }
    } else {
        memset(&ifex, 0x0, sizeof(vtss_ifindex_elm_t));
        ifex.iftype = VTSS_IFINDEX_TYPE_ILLEGAL;
    }
    if (vid_prev) {
        vidx = *vid_prev;
    } else {
        vidx = VTSS_VID_NULL;
    }
    if (mac_prev) {
        memcpy(&macx.addr[0], mac_prev, sizeof(mesa_mac_t));
    } else {
        memset(&macx.addr[0], 0x0, sizeof(mesa_mac_t));
    }
    if (ipa_prev) {
        ipax = *ipa_prev;
    } else {
        ipax = 0;
    }

    memset(&entry, 0x0, sizeof(arp_inspection_entry_t));
    if (ifex.iftype == VTSS_IFINDEX_TYPE_PORT) {
        entry.isid = ifex.isid;
        entry.port_no = ifex.ordinal;
        entry.vid = vidx;
        memcpy(&entry.mac[0], &macx.addr[0], sizeof(mesa_mac_t));
        entry.assigned_ip = ipax;
    }

    if (static_entry) {
        rc = arp_inspection_mgmt_conf_static_entry_get(&entry, TRUE);
    } else {
        rc = arp_inspection_mgmt_conf_dynamic_entry_get(&entry, TRUE);
    }

    if (rc == VTSS_RC_OK) {
        /* convert isid/iport to IfIndex */
        if ((rc = vtss_ifindex_from_port(entry.isid, entry.port_no, ifx_next)) == VTSS_RC_OK) {
            *vid_next = entry.vid;
            memcpy(mac_next, &entry.mac[0], sizeof(mesa_mac_t));
            *ipa_next = entry.assigned_ip;
        } else {
            T_D("Failed to convert IfIndex from %u/%u\n\r", entry.isid, entry.port_no);
        }
    }

    return rc;
}

/**
 * Set ARP Inspection Crossed Threshold Event
 *
 * To read current system parameters in ARP Inspection.
 *
 * \param conf [OUT]    The ARP Inspection system configuration data.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
static mesa_rc _vtss_appl_arp_inspection_set_crossed_event(void)
{
    vtss_appl_arp_inspection_status_event_t status;

    /* set the crossed event as TRUE */
    status.crossed_maximum_entries = TRUE;
    arp_inspection_status_event_update.set(&status);

    T_D("exit");
    return VTSS_RC_OK;
}


/**
 * Clear ARP Inspection Crossed Threshold Event
 *
 * To read current system parameters in ARP Inspection.
 *
 * \param conf [OUT]    The ARP Inspection system configuration data.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
static mesa_rc _vtss_appl_arp_inspection_clear_crossed_event(void)
{
    vtss_appl_arp_inspection_status_event_t status;

    /* set the crossed event as FALSE */
    status.crossed_maximum_entries = FALSE;
    arp_inspection_status_event_update.set(&status);

    T_D("exit");
    return VTSS_RC_OK;
}

/**
 * Get ARP Inspection Parameters
 *
 * To read current system parameters in ARP Inspection.
 *
 * \param conf [OUT]    The ARP Inspection system configuration data.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc
vtss_appl_arp_inspection_system_config_get(
    vtss_appl_arp_inspection_conf_t          *const conf
)
{
    u32     mgmt_mode;
    mesa_rc rc = VTSS_RC_ERROR;

    if (!conf) {
        T_D("Invalid Input!");
        return rc;
    }

    if ((rc = arp_inspection_mgmt_conf_mode_get(&mgmt_mode)) == VTSS_RC_OK) {
        conf->mode = (mgmt_mode == ARP_INSPECTION_MGMT_ENABLED);
    }

    return rc;
}

/**
 * Set ARP Inspection Parameters
 *
 * To modify current system parameters in ARP Inspection.
 *
 * \param conf [IN]     The ARP Inspection system configuration data.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc
vtss_appl_arp_inspection_system_config_set(
    const vtss_appl_arp_inspection_conf_t    *const conf
)
{
    u32     mgmt_mode;
    mesa_rc rc = VTSS_RC_ERROR;

    if (!conf) {
        T_D("Invalid Input!");
        return rc;
    }

    if ((rc = arp_inspection_mgmt_conf_mode_get(&mgmt_mode)) == VTSS_RC_OK) {
        mgmt_mode = conf->mode ? ARP_INSPECTION_MGMT_ENABLED : ARP_INSPECTION_MGMT_DISABLED;
        rc = arp_inspection_mgmt_conf_mode_set(&mgmt_mode);
    }

    return rc;
}

/**
 * ARP Inspection Control ACTION
 *
 * Action flag to denote translating dynamic ARP entries to be static ones.
 * This flag is active only when SET is involved and its value is set to be TRUE.
 * When it is active, it means we should take action for converting all dynamic
 * entries to be static entries.
 *
 * \param act_flag  [IN]    The ARP Inspection action to be taken.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc
vtss_appl_arp_inspection_control_translate_dynamic_to_static_act(
    const BOOL                              *act_flag
)
{
    if (act_flag && *act_flag) {
        if (arp_inspection_mgmt_conf_translate_dynamic_into_static() < 0) {
            return VTSS_RC_ERROR;
        }
    }

    return VTSS_RC_OK;
}

/**
 * Iterator for retrieving ARP Inspection port configuration key/index
 *
 * To walk configuration index of the port in ARP Inspection.
 *
 * \param prev      [IN]    Interface index to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_arp_inspection_port_config_itr(
    const vtss_ifindex_t                    *const prev,
    vtss_ifindex_t                          *const next
)
{
    return vtss_appl_iterator_ifindex_front_port_exist(prev, next);
}

/**
 * Get ARP Inspection specific port configuration
 *
 * To read configuration of the specific port in ARP Inspection.
 *
 * \param ifindex   [IN]    (key)   Interface index - the logical interface
 *                                  index of the physical port.
 * \param port_conf [OUT]   The current configuration of the port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_port_config_get(
    vtss_ifindex_t                          ifindex,
    vtss_appl_arp_inspection_port_config_t       *const port_conf
)
{
    vtss_ifindex_elm_t              ife;
    arp_inspection_port_mode_conf_t port_mode_conf;
    u32                             ptx;
    mesa_rc                         rc = VTSS_RC_ERROR;

    /* get isid/iport from given IfIndex */
    if (!port_conf ||
        vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        T_D("Invalid Input!");
        return rc;
    }

    T_D("GET ISID:%u PORT:%u", ife.isid, ife.ordinal);
    if ((ptx = ife.ordinal) < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) &&
        (rc = arp_inspection_mgmt_conf_port_mode_get(ife.isid, &port_mode_conf)) == VTSS_RC_OK) {
        port_conf->mode = port_mode_conf.mode[ptx];
        port_conf->check_vlan = port_mode_conf.check_VLAN[ptx];
        port_conf->log_type = port_mode_conf.log_type[ptx];
    }

    return rc;
}

/**
 * Set ARP Inspection specific port configuration
 *
 * To modify configuration of the specific port in ARP Inspection.
 *
 * \param ifindex   [IN]    (key)   Interface index - the logical interface index
 *                                  of the physical port.
 * \param port_conf [IN]    The configuration set to the port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_port_config_set(
    vtss_ifindex_t                          ifindex,
    const vtss_appl_arp_inspection_port_config_t *const port_conf
)
{
    vtss_ifindex_elm_t              ife;
    arp_inspection_port_mode_conf_t port_mode_conf;
    u32                             ptx;
    mesa_rc                         rc = VTSS_RC_ERROR;

    /* get isid/iport from given IfIndex */
    if (!port_conf ||
        vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        T_D("Invalid Input!");
        return rc;
    }

    T_D("SET ISID:%u PORT:%u", ife.isid, ife.ordinal);
    if ((ptx = ife.ordinal) < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) &&
        (rc = arp_inspection_mgmt_conf_port_mode_get(ife.isid, &port_mode_conf)) == VTSS_RC_OK) {
        port_mode_conf.mode[ptx] = port_conf->mode;
        port_mode_conf.check_VLAN[ptx] = port_conf->check_vlan;
        port_mode_conf.log_type[ptx] = port_conf->log_type;

        rc = arp_inspection_mgmt_conf_port_mode_set(ife.isid, &port_mode_conf);
    }

    return rc;
}

/**
 * Iterator for retrieving ARP Inspection VLAN configuration key/index
 *
 * To walk configuration index of the VLAN in ARP Inspection.
 *
 * \param prev      [IN]    VLAN ID to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_arp_inspection_vlan_config_itr(
    const mesa_vid_t                        *const prev,
    mesa_vid_t                              *const next
)
{
    return arp_inspection_mgmt_vlan_conf_index_get_next(prev, next);
}

/**
 * Get ARP Inspection specific VLAN configuration
 *
 * To read configuration of the specific VLAN in ARP Inspection.
 *
 * \param vlan_id   [IN]    (key)   VLAN ID - VID of the VLAN.
 * \param vlan_conf [OUT]   The current configuration of the VLAN.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_vlan_config_get(
    mesa_vid_t                              vlan_id,
    vtss_appl_arp_inspection_vlan_config_t       *const vlan_conf
)
{
    mesa_rc                         rc = VTSS_RC_ERROR;
    arp_inspection_vlan_mode_conf_t vlan_mode_conf;

    if (!vlan_conf) {
        T_D("Invalid Input!");
        return rc;
    }

    T_D("GET VID:%u", vlan_id);
    if ((rc = arp_inspection_mgmt_conf_vlan_mode_get(vlan_id, &vlan_mode_conf, FALSE)) == VTSS_RC_OK) {
        if (vlan_mode_conf.flags & ARP_INSPECTION_VLAN_MODE) {
            if ((vlan_mode_conf.flags & ARP_INSPECTION_VLAN_LOG_PERMIT) &&
                (vlan_mode_conf.flags & ARP_INSPECTION_VLAN_LOG_DENY)) {
                vlan_conf->log_type = VTSS_APPL_ARP_INSPECTION_LOG_ALL;
            } else {
                if (vlan_mode_conf.flags & ARP_INSPECTION_VLAN_LOG_PERMIT) {
                    vlan_conf->log_type = VTSS_APPL_ARP_INSPECTION_LOG_PERMIT;
                } else if (vlan_mode_conf.flags & ARP_INSPECTION_VLAN_LOG_DENY) {
                    vlan_conf->log_type = VTSS_APPL_ARP_INSPECTION_LOG_DENY;
                } else {
                    vlan_conf->log_type = VTSS_APPL_ARP_INSPECTION_LOG_NONE;
                }
            }
        } else {
            rc = VTSS_RC_ERROR;
        }
    }

    return rc;
}

/**
 * Set ARP Inspection specific VLAN configuration
 *
 * To modify configuration of the specific VLAN in ARP Inspection.
 *
 * \param vlan_id   [IN]    (key)   VLAN ID - VID of the VLAN.
 * \param vlan_conf [IN]    The configuration set to the VLAN.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_vlan_config_set(
    mesa_vid_t                              vlan_id,
    const vtss_appl_arp_inspection_vlan_config_t *const vlan_conf
)
{
    mesa_rc                         rc = VTSS_RC_ERROR;
    arp_inspection_vlan_mode_conf_t vlan_mode_conf;

    if (!vlan_conf) {
        T_D("Invalid Input!");
        return rc;
    }

    if ((vlan_id < VTSS_APPL_VLAN_ID_MIN) || (vlan_id > VTSS_APPL_VLAN_ID_MAX)) {
        T_D("Invalid VLAN ID!");
        return rc;
    }

    T_D("SET VID:%u", vlan_id);
    if ((rc = arp_inspection_mgmt_conf_vlan_mode_get(vlan_id, &vlan_mode_conf, FALSE)) == VTSS_RC_OK) {
        if (vlan_mode_conf.flags & ARP_INSPECTION_VLAN_MODE) {
            vlan_mode_conf.flags = 0;
            vlan_mode_conf.flags |= ARP_INSPECTION_VLAN_MODE;
            switch ( vlan_conf->log_type ) {
            case VTSS_APPL_ARP_INSPECTION_LOG_NONE:
                break;
            case VTSS_APPL_ARP_INSPECTION_LOG_DENY:
                vlan_mode_conf.flags |= ARP_INSPECTION_VLAN_LOG_DENY;
                break;
            case VTSS_APPL_ARP_INSPECTION_LOG_PERMIT:
                vlan_mode_conf.flags |= ARP_INSPECTION_VLAN_LOG_PERMIT;
                break;
            case VTSS_APPL_ARP_INSPECTION_LOG_ALL:
                vlan_mode_conf.flags |= ARP_INSPECTION_VLAN_LOG_DENY;
                vlan_mode_conf.flags |= ARP_INSPECTION_VLAN_LOG_PERMIT;
                break;
            }

            rc = arp_inspection_mgmt_conf_vlan_mode_set(vlan_id, &vlan_mode_conf);
        } else {
            T_D("Set non-existing entry!");
            rc = VTSS_RC_ERROR;
        }
    }

    return rc;
}

/**
 * Delete ARP Inspection specific VLAN configuration
 *
 * To delete configuration of the specific VLAN in ARP Inspection.
 *
 * \param vlan_id   [IN]    (key)   VLAN ID - VID of the VLAN.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_vlan_config_del(
    mesa_vid_t                              vlan_id
)
{
    mesa_rc                         rc;
    arp_inspection_vlan_mode_conf_t vlan_mode_conf;

    T_D("DEL VID:%u", vlan_id);
    if ((rc = arp_inspection_mgmt_conf_vlan_mode_get(vlan_id, &vlan_mode_conf, FALSE)) == VTSS_RC_OK) {
        if (vlan_mode_conf.flags & ARP_INSPECTION_VLAN_MODE) {
            vlan_mode_conf.flags = 0;
            rc = arp_inspection_mgmt_conf_vlan_mode_set(vlan_id, &vlan_mode_conf);
        } else {
            T_D("Delete non-existing entry!");
            rc = VTSS_RC_ERROR;
        }
    }

    return rc;
}

/**
 * Add ARP Inspection specific VLAN configuration
 *
 * To add configuration of the specific VLAN in ARP Inspection.
 *
 * \param vlan_id   [IN]    (key)   VLAN ID - VID of the VLAN.
 * \param vlan_conf [IN]    The configuration set to the new created instance.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_vlan_config_add(
    mesa_vid_t                              vlan_id,
    const vtss_appl_arp_inspection_vlan_config_t *const vlan_conf
)
{
    mesa_rc                         rc = VTSS_RC_ERROR;
    arp_inspection_vlan_mode_conf_t vlan_mode_conf;

    if (!vlan_conf) {
        T_D("Invalid Input!");
        return rc;
    }


    if ((vlan_id < VTSS_APPL_VLAN_ID_MIN) || (vlan_id > VTSS_APPL_VLAN_ID_MAX)) {
        T_D("Invalid VLAN ID!");
        return rc;
    }

    T_D("ADD VID:%u", vlan_id);
    if ((rc = arp_inspection_mgmt_conf_vlan_mode_get(vlan_id, &vlan_mode_conf, FALSE)) != VTSS_RC_OK ||
        !(vlan_mode_conf.flags & ARP_INSPECTION_VLAN_MODE)) {
        memset(&vlan_mode_conf, 0x0, sizeof(arp_inspection_vlan_mode_conf_t));
        vlan_mode_conf.flags |= ARP_INSPECTION_VLAN_MODE;
        switch ( vlan_conf->log_type ) {
        case VTSS_APPL_ARP_INSPECTION_LOG_NONE:
            break;
        case VTSS_APPL_ARP_INSPECTION_LOG_DENY:
            vlan_mode_conf.flags |= ARP_INSPECTION_VLAN_LOG_DENY;
            break;
        case VTSS_APPL_ARP_INSPECTION_LOG_PERMIT:
            vlan_mode_conf.flags |= ARP_INSPECTION_VLAN_LOG_PERMIT;
            break;
        case VTSS_APPL_ARP_INSPECTION_LOG_ALL:
            vlan_mode_conf.flags |= ARP_INSPECTION_VLAN_LOG_DENY;
            vlan_mode_conf.flags |= ARP_INSPECTION_VLAN_LOG_PERMIT;
            break;
        }

        rc = arp_inspection_mgmt_conf_vlan_mode_set(vlan_id, &vlan_mode_conf);
    } else {
        T_D("Add existing entry!");
        rc = VTSS_RC_ERROR;
    }

    return rc;
}

/**
 * \brief Get ARP Inspection default VLAN
 *
 * This is way to fill the RowEditor with values that are within
 * the expected range (e.g. VlanId between 1 - 4095). Without this,
 * fields are set to zero which is outside the expected range.
 *
 * \param vlan_id   [OUT]   (key)   VLAN ID - VID of the VLAN.
 * \param vlan_conf [OUT]   The current configuration of the VLAN.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_vlan_config_default (
    mesa_vid_t                              *const vlan_id,
    vtss_appl_arp_inspection_vlan_config_t  *const vlan_conf
)
{
    mesa_rc                         rc = VTSS_RC_OK;

    memset(vlan_conf, 0, sizeof(vtss_appl_arp_inspection_vlan_config_t));

    *vlan_id = VTSS_VID_DEFAULT;

    return rc;
}

/**
 * Iterator for retrieving ARP Inspection static entry table key/index
 *
 * To walk configuration index of static entry table in ARP Inspection.
 *
 * \param ifx_prev  [IN]    Interface index to be used for indexing determination.
 * \param vid_prev  [IN]    VLAN ID to be used for indexing determination.
 * \param mac_prev  [IN]    MAC address to be used for indexing determination.
 * \param ipa_prev  [IN]    IPv4 address to be used for indexing determination.
 *
 * \param ifx_next  [OUT]   The key/index of Interface index should be used for the GET operation.
 * \param vid_next  [OUT]   The key/index of VLAN ID should be used for the GET operation.
 * \param mac_next  [OUT]   The key/index of MAC address should be used for the GET operation.
 * \param ipa_next  [OUT]   The key/index of IPv4 address should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *                          The precedence of IN key/index is in given sequential order.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_arp_inspection_static_entry_itr(
    const vtss_ifindex_t                    *const ifx_prev,
    vtss_ifindex_t                          *const ifx_next,
    const mesa_vid_t                        *const vid_prev,
    mesa_vid_t                              *const vid_next,
    const mesa_mac_t                        *const mac_prev,
    mesa_mac_t                              *const mac_next,
    const mesa_ipv4_t                       *const ipa_prev,
    mesa_ipv4_t                             *const ipa_next
)
{
    return _vtss_appl_arp_inspection_entry_itr(TRUE,
                                               ifx_prev, ifx_next,
                                               vid_prev, vid_next,
                                               mac_prev, mac_next,
                                               ipa_prev, ipa_next);
}

/**
 * Get ARP Inspection specific static entry configuration
 *
 * To read configuration of the specific static entry in ARP Inspection.
 *
 * \param ifindex   [IN]    (key 1) Interface index to be used of the physical port.
 * \param vlan_id   [IN]    (key 2) VLAN ID - VID of the VLAN.
 * \param mac_addr  [IN]    (key 3) Assigned MAC address.
 * \param ip_addr   [IN]    (key 4) Assigned IPv4 address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_static_entry_get(
    vtss_ifindex_t                          ifindex,
    mesa_vid_t                              vlan_id,
    mesa_mac_t                              mac_addr,
    mesa_ipv4_t                             ip_addr
)
{
    vtss_ifindex_elm_t      ife;
    arp_inspection_entry_t  ai_entry;

    /* get isid/iport from given IfIndex */
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    memset(&ai_entry, 0x0, sizeof(arp_inspection_entry_t));
    ai_entry.isid = ife.isid;
    ai_entry.port_no = ife.ordinal;
    ai_entry.vid = vlan_id;
    memcpy(&ai_entry.mac[0], &mac_addr.addr[0], sizeof(mesa_mac_t));
    ai_entry.assigned_ip = ip_addr;
    T_D("GET ISID:%u PORT:%u VID:%u MAC:0x%2u%2u%2u%2u%2u%2u IPA:%d",
        ai_entry.isid, ai_entry.port_no,
        ai_entry.vid,
        ai_entry.mac[0], ai_entry.mac[1], ai_entry.mac[2],
        ai_entry.mac[3], ai_entry.mac[4], ai_entry.mac[5],
        ai_entry.assigned_ip);
    return arp_inspection_mgmt_conf_static_entry_get(&ai_entry, FALSE);
}

/**
 * Set ARP Inspection specific static entry configuration
 *
 * To modify configuration of the specific static entry in ARP Inspection.
 *
 * \param ifindex   [IN]    (key 1) Interface index to be used of the physical port.
 * \param vlan_id   [IN]    (key 2) VLAN ID - VID of the VLAN.
 * \param mac_addr  [IN]    (key 3) Assigned MAC address.
 * \param ip_addr   [IN]    (key 4) Assigned IPv4 address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_static_entry_set(
    vtss_ifindex_t                          ifindex,
    mesa_vid_t                              vlan_id,
    mesa_mac_t                              mac_addr,
    mesa_ipv4_t                             ip_addr
)
{
    vtss_ifindex_elm_t      ife;
    arp_inspection_entry_t  ai_entry;
    mesa_rc                 rc = VTSS_RC_ERROR;

    /* get isid/iport from given IfIndex */
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        T_D("Invalid Input!");
        return rc;
    }
    if (mac_addr.addr[0] & 0x01) {
        T_D("Invalid MAC address (Multicast)!");
        return rc;
    }
    if (vtss_ipv4_addr_is_multicast(&ip_addr)) {
        T_D("Invalid IP address (Multicast)!");
        return rc;
    }

    memset(&ai_entry, 0x0, sizeof(arp_inspection_entry_t));
    ai_entry.isid = ife.isid;
    ai_entry.port_no = ife.ordinal;
    ai_entry.vid = vlan_id;
    memcpy(&ai_entry.mac[0], &mac_addr.addr[0], sizeof(mesa_mac_t));
    ai_entry.assigned_ip = ip_addr;
    ai_entry.type = VTSS_APPL_ARP_INSPECTION_STATIC_TYPE;
    ai_entry.valid = TRUE;

    T_D("SET ISID:%u PORT:%u VID:%u MAC:0x%2u%2u%2u%2u%2u%2u IPA:%d",
        ai_entry.isid, ai_entry.port_no,
        ai_entry.vid,
        ai_entry.mac[0], ai_entry.mac[1], ai_entry.mac[2],
        ai_entry.mac[3], ai_entry.mac[4], ai_entry.mac[5],
        ai_entry.assigned_ip);
    if ((rc = arp_inspection_mgmt_conf_static_entry_get(&ai_entry, FALSE)) == VTSS_RC_OK) {
        T_D("Set existing entry!");
        rc = VTSS_RC_OK;
    } else {
        T_D("Set non-existing entry!");
        rc = arp_inspection_mgmt_conf_static_entry_set(&ai_entry);
    }

    return rc;
}

/**
 * Delete ARP Inspection specific static entry configuration
 *
 * To delete configuration of the specific static entry in ARP Inspection.
 *
 * \param ifindex   [IN]    (key 1) Interface index to be used of the physical port.
 * \param vlan_id   [IN]    (key 2) VLAN ID - VID of the VLAN.
 * \param mac_addr  [IN]    (key 3) Assigned MAC address.
 * \param ip_addr   [IN]    (key 4) Assigned IPv4 address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_static_entry_del(
    vtss_ifindex_t                          ifindex,
    mesa_vid_t                              vlan_id,
    mesa_mac_t                              mac_addr,
    mesa_ipv4_t                             ip_addr
)
{
    vtss_ifindex_elm_t      ife;
    arp_inspection_entry_t  ai_entry;
    mesa_rc                 rc = VTSS_RC_ERROR;

    /* get isid/iport from given IfIndex */
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        T_D("Invalid Input!");
        return rc;
    }

    memset(&ai_entry, 0x0, sizeof(arp_inspection_entry_t));
    ai_entry.isid = ife.isid;
    ai_entry.port_no = ife.ordinal;
    ai_entry.vid = vlan_id;
    memcpy(&ai_entry.mac[0], &mac_addr.addr[0], sizeof(mesa_mac_t));
    ai_entry.assigned_ip = ip_addr;
    T_D("DEL ISID:%u PORT:%u VID:%u MAC:0x%2u%2u%2u%2u%2u%2u IPA:%d",
        ai_entry.isid, ai_entry.port_no,
        ai_entry.vid,
        ai_entry.mac[0], ai_entry.mac[1], ai_entry.mac[2],
        ai_entry.mac[3], ai_entry.mac[4], ai_entry.mac[5],
        ai_entry.assigned_ip);
    if ((rc = arp_inspection_mgmt_conf_static_entry_get(&ai_entry, FALSE)) == VTSS_RC_OK) {
        rc = arp_inspection_mgmt_conf_static_entry_del(&ai_entry);
    } else {
        T_D("Delete non-existing entry!");
    }

    return rc;
}

/**
 * Add ARP Inspection specific static entry configuration
 *
 * To add configuration of the specific static entry in ARP Inspection.
 *
 * \param ifindex   [IN]    (key 1) Interface index to be used of the physical port.
 * \param vlan_id   [IN]    (key 2) VLAN ID - VID of the VLAN.
 * \param mac_addr  [IN]    (key 3) Assigned MAC address.
 * \param ip_addr   [IN]    (key 4) Assigned IPv4 address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_static_entry_add(
    vtss_ifindex_t                          ifindex,
    mesa_vid_t                              vlan_id,
    mesa_mac_t                              mac_addr,
    mesa_ipv4_t                             ip_addr
)
{
    vtss_ifindex_elm_t      ife;
    arp_inspection_entry_t  ai_entry;
    mesa_rc                 rc = VTSS_RC_ERROR;

    /* get isid/iport from given IfIndex */
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        T_D("Invalid Input!");
        return rc;
    }

    if (mac_addr.addr[0] & 0x01) {
        T_D("Invalid MAC address (Multicast)!");
        return rc;
    }
    if (vtss_ipv4_addr_is_multicast(&ip_addr)) {
        T_D("Invalid IP address (Multicast)!");
        return rc;
    }

    memset(&ai_entry, 0x0, sizeof(arp_inspection_entry_t));
    ai_entry.isid = ife.isid;
    ai_entry.port_no = ife.ordinal;
    ai_entry.vid = vlan_id;
    memcpy(&ai_entry.mac[0], &mac_addr.addr[0], sizeof(mesa_mac_t));
    ai_entry.assigned_ip = ip_addr;
    ai_entry.type = VTSS_APPL_ARP_INSPECTION_STATIC_TYPE;
    ai_entry.valid = TRUE;

    T_D("ADD ISID:%u PORT:%u VID:%u MAC:0x%2u%2u%2u%2u%2u%2u IPA:%d",
        ai_entry.isid, ai_entry.port_no,
        ai_entry.vid,
        ai_entry.mac[0], ai_entry.mac[1], ai_entry.mac[2],
        ai_entry.mac[3], ai_entry.mac[4], ai_entry.mac[5],
        ai_entry.assigned_ip);
    if ((rc = arp_inspection_mgmt_conf_static_entry_get(&ai_entry, FALSE)) != VTSS_RC_OK) {
        T_D("Set non-existing entry!");
        rc = arp_inspection_mgmt_conf_static_entry_set(&ai_entry);
    } else {
        T_D("Add existing entry!");
        rc = VTSS_RC_ERROR;
    }

    return rc;
}

/**
 * \brief Get ARP Inspection static entry default VLAN
 *
 * This is way to fill the RowEditor with values that are within
 * the expected range (e.g. VlanId between 1 - 4095). Without this,
 * fields are set to zero which is outside the expected range.
 *
 * \param ifindex   [OUT]    (key 1) Interface index to be used of the physical port.
 * \param vlan_id   [OUT]    (key 2) VLAN ID - VID of the VLAN.
 * \param mac_addr  [OUT]    (key 3) Assigned MAC address.
 * \param ip_addr   [OUT]    (key 4) Assigned IPv4 address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_static_entry_default(
    vtss_ifindex_t                          *const ifindex,
    mesa_vid_t                              *const vlan_id,
    mesa_mac_t                              *const mac_addr,
    mesa_ipv4_t                             *const ip_addr
)
{
    mesa_rc                         rc = VTSS_RC_OK;

    *vlan_id = VTSS_VID_DEFAULT;
    return rc;
}

/**
 * Iterator for retrieving ARP Inspection dynamic entry table key/index
 *
 * To walk configuration index of the dynamic entry table in ARP Inspection.
 *
 * \param ifx_prev  [IN]    Interface index to be used for indexing determination.
 * \param vid_prev  [IN]    VLAN ID to be used for indexing determination.
 * \param mac_prev  [IN]    MAC address to be used for indexing determination.
 * \param ipa_prev  [IN]    IPv4 address to be used for indexing determination.
 *
 * \param ifx_next  [OUT]   The key/index of Interface index should be used for the GET operation.
 * \param vid_next  [OUT]   The key/index of VLAN ID should be used for the GET operation.
 * \param mac_next  [OUT]   The key/index of MAC address should be used for the GET operation.
 * \param ipa_next  [OUT]   The key/index of IPv4 address should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *                          The precedence of IN key/index is in given sequential order.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_arp_inspection_dynamic_entry_itr(
    const vtss_ifindex_t                    *const ifx_prev,
    vtss_ifindex_t                          *const ifx_next,
    const mesa_vid_t                        *const vid_prev,
    mesa_vid_t                              *const vid_next,
    const mesa_mac_t                        *const mac_prev,
    mesa_mac_t                              *const mac_next,
    const mesa_ipv4_t                       *const ipa_prev,
    mesa_ipv4_t                             *const ipa_next
)
{
    return _vtss_appl_arp_inspection_entry_itr(FALSE,
                                               ifx_prev, ifx_next,
                                               vid_prev, vid_next,
                                               mac_prev, mac_next,
                                               ipa_prev, ipa_next);
}

/**
 * Get ARP Inspection specific dynamic entry status
 *
 * To read the status of specific dynamic entry in ARP Inspection.
 *
 * \param ifindex   [IN]    (key 1) Interface index to be used of the physical port.
 * \param vlan_id   [IN]    (key 2) VLAN ID - VID of the VLAN.
 * \param mac_addr  [IN]    (key 3) Assigned MAC address.
 * \param ip_addr   [IN]    (key 4) Assigned IPv4 address.
 * \param entry     [OUT]   The current status of the dynamic entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_dynamic_entry_get(
    vtss_ifindex_t                          ifindex,
    mesa_vid_t                              vlan_id,
    mesa_mac_t                              mac_addr,
    mesa_ipv4_t                             ip_addr,
    vtss_appl_arp_inspection_entry_t             *const entry
)
{
    vtss_ifindex_elm_t      ife;
    arp_inspection_entry_t  ai_entry;
    mesa_rc                 rc = VTSS_RC_ERROR;

    /* get isid/iport from given IfIndex */
    if (!entry ||
        vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        T_D("Invalid Input!");
        return rc;
    }

    memset(&ai_entry, 0x0, sizeof(arp_inspection_entry_t));
    ai_entry.isid = ife.isid;
    ai_entry.port_no = ife.ordinal;
    ai_entry.vid = vlan_id;
    memcpy(&ai_entry.mac[0], &mac_addr.addr[0], sizeof(mesa_mac_t));
    ai_entry.assigned_ip = ip_addr;
    T_D("GET ISID:%u PORT:%u VID:%u MAC:0x%2u%2u%2u%2u%2u%2u IPA:%d",
        ai_entry.isid, ai_entry.port_no,
        ai_entry.vid,
        ai_entry.mac[0], ai_entry.mac[1], ai_entry.mac[2],
        ai_entry.mac[3], ai_entry.mac[4], ai_entry.mac[5],
        ai_entry.assigned_ip);
    if ((rc = arp_inspection_mgmt_conf_dynamic_entry_get(&ai_entry, FALSE)) == VTSS_RC_OK) {
        entry->reg_status = ai_entry.type;
    }

    return rc;
}

/*
  ==============================================================================
  Bugzilla#19161 - Filtering of ACL-copy-to-cpu frames
  Under Linux the frame-flow has been changed such that frames are received in
  the kernel, and then injected into the application and the IP stack at the
  same time. This means that the application can not consume frames and thereby
  prevent them from being injected into the IP stack.

  Since all packets will be injected into IP stack at Linux platform,
  IP RX filter table provides the deny and allow list to determine
  which packets are allowed to be injected into IP stack.

  When ARP inspection is enabled globally, create one filter rule in deny list,
  which qualifies the ARP request frames from untrust ports with rule action
  as check-allow-list.
  In addition, one filter rule in allow list is also created at the same time,
  which qualifies all gratuitous ARP frames.

  ARP inspection maintains a database includes static and dynamic entries.
  This database lists all valid user to access network.
  Create filter rule in allow list for each valid user in ARP inspection database.
  ==============================================================================*/

static mesa_rc vtss_appl_arp_inspection_filter_entry_add(arp_inspection_entry_t *entry)
{
    mesa_rc rc = VTSS_RC_OK;
    int id = VTSS_IP_FILTER_ID_NONE;
    mesa_mac_t rule_mac;
    mesa_ipv4_network_t rule_ip;

    // initialize
    memset(&rule_ip, 0, sizeof(rule_ip));
    memset(&rule_mac, 0, sizeof(rule_mac));
    memcpy(&rule_mac.addr[0], &entry->mac[0], sizeof(mesa_mac_t));
    rule_ip.address = entry->assigned_ip;
    rule_ip.prefix_size = 32; //IP mask is 255.255.255.255

    // set allow rule entry
    Rule r_allow;
    r_allow.emplace_back(element_ether_type(0x0806));
    r_allow.emplace_back(element_port_mask(PortMask(entry->port_no)));
    r_allow.emplace_back(element_vlan(entry->vid));
    r_allow.emplace_back(element_arp_hw_sender(rule_mac));
    r_allow.emplace_back(element_arp_proto_sender(rule_ip));

    rc = allow_list_rule_add(&id, &arp_inspection_rule_owner, r_allow);
    T_I("rule_add(AllowList, owner = %p, port_no = %u = 0b%s, vid = %u, mac = %s, ip = %s) => id = %d, rc = %s", &arp_inspection_rule_owner, entry->port_no, PortMask(entry->port_no).to_string().c_str(), entry->vid, rule_mac, rule_ip, id, error_txt(rc));
    entry->filter_rule_id = id;

    return rc;
}


static mesa_rc vtss_appl_arp_inspection_filter_entry_del(arp_inspection_entry_t *entry)
{
    mesa_rc rc = VTSS_RC_OK;
    int id = entry->filter_rule_id;

    rc = rule_del(id);
    T_I("rule_del(AllowList, owner = %p, id = %d) => rc = %s", &arp_inspection_rule_owner, id, error_txt(rc));
    return rc;
}


static mesa_rc vtss_appl_arp_inspection_filter_set(uint8_t enabled, PortMask untrusted_port_list)
{
    mesa_rc rc = VTSS_RC_OK;
    Rule r_deny, w_rule_1;
    int id;

    // printf("filter is %s, with port list: b%s\n", enabled ? "enabled" : "disabled", untrusted_port_list.to_string().c_str());
    if (enabled) {
        //ARP inspection is enabled

        //deny all ARP from untrusted ports
        r_deny.emplace_back(element_ether_type(0x0806)); //ether type ARP
        r_deny.emplace_back(element_port_mask(untrusted_port_list)); // untrusted ports

        //allow gratutious ARP for all ports
        w_rule_1.emplace_back(element_ether_type(0x0806)); //ether type ARP
        w_rule_1.emplace_back(element_arg_gratuitous()); // gratuitous ARP

        ARP_INSPECTION_CRIT_ENTER();

        // get the current filter rule id if existed
        id = arp_inspection_global.arp_inspection_conf.filter_rule_id[0];
        if (id == VTSS_IP_FILTER_ID_NONE) {
            // not existed yet, add the deny all ARP
            if ((rc = deny_list_rule_add(&id, &arp_inspection_rule_owner, r_deny, Action::check_allow_list)) == VTSS_RC_OK) {
                // record rule id in cache
                arp_inspection_global.arp_inspection_conf.filter_rule_id[0] = id;
            }

            T_I("rule_add(DenyList, owner = %p, PortList = 0b%s) => id = %d, rc = %s", &arp_inspection_rule_owner, untrusted_port_list.to_string().c_str(), id, error_txt(rc));
        } else {
            // rules are already existed, update the untrust port list only.
            rc = rule_update(id, r_deny);
            T_I("rule_update(DenyList, owner = %p, id = %d, PortList = 0b%s) => rc = %s", &arp_inspection_rule_owner, id, untrusted_port_list.to_string().c_str(), error_txt(rc));
        }

        // get the current filter rule id if existed
        id = arp_inspection_global.arp_inspection_conf.filter_rule_id[1];
        if (id == VTSS_IP_FILTER_ID_NONE) {
            //  not existed yet, add allow rule for gratuitous ARP
            if ((rc = allow_list_rule_add(&id, &arp_inspection_rule_owner, w_rule_1)) == VTSS_RC_OK) {
                // record rule id in cache
                arp_inspection_global.arp_inspection_conf.filter_rule_id[1] = id;
            }

            T_I("rule_add(AllowList, GratuitousARP, owner = %p, PortList = 0b%s) => id = %d, rc = %s", &arp_inspection_rule_owner, untrusted_port_list.to_string().c_str(), id, error_txt(rc));

        } else {
            // rules are already existed, do nothing
        }
        ARP_INSPECTION_CRIT_EXIT();
    } else {
        // ARP inspection is disabled

        ARP_INSPECTION_CRIT_ENTER();
        // delete rule for deny ARP from untrusted ports
        id = arp_inspection_global.arp_inspection_conf.filter_rule_id[0];
        if (id != 0) {
            if ((rc = rule_del(id)) == VTSS_RC_OK) {
                // reset rule id cache
                arp_inspection_global.arp_inspection_conf.filter_rule_id[0] = 0;
            }

            T_I("rule_del(DenyList, owner = %p, id = %d) => rc = %s", &arp_inspection_rule_owner, id, error_txt(rc));
        } else {
            T_D("non-existed deny all ARP rule can't be deleted!");
        }

        // delete rule for gratuitous ARP
        id = arp_inspection_global.arp_inspection_conf.filter_rule_id[1];
        if (id != 0) {
            if ((rc = rule_del(id)) == VTSS_RC_OK) {
                // reset rule id cache
                arp_inspection_global.arp_inspection_conf.filter_rule_id[1] = 0;
            }

            T_I("rule_del(AllowList, GratuitousARP, owner = %p, id = %d) => rc = %s", &arp_inspection_rule_owner, id, error_txt(rc));
        } else {
            T_D("non-existed gratuitous ARP rule can't be deleted!");
        }
        ARP_INSPECTION_CRIT_EXIT();
    }

    T_D("filter is %s, with port list: 0b%s rc %d", enabled ? "enabled" : "disabled", untrusted_port_list.to_string().c_str(), rc);

    return rc;
}

static mesa_rc vtss_appl_arp_inspection_filter_update(void)
{
    mesa_rc rc = VTSS_RC_OK;
    port_iter_t pit;
    int isid;
    uint8_t mode;
    PortMask untrusted_port_list;

    ARP_INSPECTION_CRIT_ENTER();
    if ((mode = arp_inspection_global.arp_inspection_conf.mode) == ARP_INSPECTION_MGMT_ENABLED) {
        // if arp inspection is enabled, find all untrusted port
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (arp_inspection_global.arp_inspection_conf.port_mode_conf[isid - VTSS_ISID_START].mode[pit.iport] == ARP_INSPECTION_MGMT_ENABLED) {
                    // Till now, slot id in IP rx filter is not considered yet.
                    untrusted_port_list.set(pit.iport);
                }
            }
        }
    } else {
        // if arp inspection is disabled, zero untrusted port.
        // Do nothing.
    }
    ARP_INSPECTION_CRIT_EXIT();

    rc = vtss_appl_arp_inspection_filter_set(mode, untrusted_port_list);

    return rc;
}

static mesa_rc vtss_appl_arp_inspection_filter_clear_all(void)
{
    mesa_rc rc = VTSS_RC_OK;
    uint32_t cnt;
    rc = rule_del(&arp_inspection_rule_owner, &cnt);
    T_I("rule_del(All, owner = %p) => rc = %s", &arp_inspection_rule_owner, error_txt(rc));

    return rc;
}

