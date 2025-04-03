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

#include "icfg.h"
#include "icfg_trace.h"
#include "icfg_api.h"
#if defined(VTSS_SW_OPTION_FIRMWARE)
#include "firmware_api.h"
#endif
#include "msg_api.h"
#include "critd_api.h"
#include "misc_api.h"
#include "vtss_usb.h"
#include "vtss/appl/interface.h"
#include "control_api.h"
#include "crashhandler.hxx"
#include "vtss_tftp_api.h"
#include "vtss_mtd_api.hxx"
#include "vtss/basics/memory.hxx"
#ifdef VTSS_SW_OPTION_SNMP
#include "vtss_os_wrapper_snmp.h"
#endif
#include <sys/statvfs.h>

#ifdef VTSS_SW_OPTION_TIMER
#include "vtss_timer_api.h"
#endif

#ifdef VTSS_SW_OPTION_VLAN
#include "vlan_api.h"
#endif

#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#if defined(CYGPKG_FS_RAM)
#include "os_file_api.h"
#include <zlib.h>
#endif /* defined(CYGPKG_FS_RAM) */
#include "conf_api.h"

#if defined(VTSS_SW_OPTION_IP)
#include "ip_api.h"
#include "ip_utils.hxx"
#endif /* VTSS_SW_OPTION_IP */

#include "mgmt_api.h"

#if defined(VTSS_SW_OPTION_IPMC_LIB)
#include <vtss/appl/ipmc_lib.h>
#endif /* VTSS_SW_OPTION_IPMC_LIB */

#if defined(VTSS_SW_OPTION_SNMP)
#include "vtss_snmp_api.h"
#endif /* VTSS_SW_OPTION_SNMP */

#if defined(VTSS_SW_OPTION_HTTPS)
#include "vtss_https_api.hxx"
#endif

#if defined(VTSS_SW_OPTION_DHCP_SERVER)
#include "dhcp_server_api.h"
#endif

#include "lock.hxx"
#if defined(VTSS_SW_OPTION_JSON_RPC_NOTIFICATION)
#include "json_rpc_notification_icli_priv.h"
#endif

#if defined(VTSS_SW_OPTION_QOS)
#include "qos_api.h"
#endif /* VTSS_SW_OPTION_QOS */

#if defined(VTSS_SW_OPTION_AGGR)
#include "aggr_api.h"
#endif /* VTSS_SW_OPTION_AGGR */

#ifdef VTSS_SW_OPTION_CFM
#include <vtss/appl/cfm.hxx>
#endif

#ifdef VTSS_SW_OPTION_APS
#include <vtss/appl/aps.h>
#endif

#ifdef VTSS_SW_OPTION_ERPS
#include <vtss/appl/erps.h>
#endif

#ifdef VTSS_SW_OPTION_IEC_MRP
#include <vtss/appl/iec_mrp.h>
#endif

#ifdef VTSS_SW_OPTION_REDBOX
#include <vtss/appl/redbox.h>
#endif

#if defined(VTSS_SW_OPTION_FRR)
#include "frr_daemon.hxx" /* For frr_has_XXX() */
#endif

#if defined(VTSS_SW_OPTION_FRR_ROUTER)
#include "frr_router_api.hxx"
#endif

#if defined(VTSS_SW_OPTION_FRR_OSPF)
#include "frr_ospf_api.hxx"
#endif

#if defined(VTSS_SW_OPTION_FRR_OSPF6)
#include "frr_ospf6_api.hxx"
#endif

#if defined(VTSS_SW_OPTION_FRR_RIP)
#include "frr_rip_api.hxx"
#endif

#ifdef VTSS_SW_OPTION_POE
#include "poe_api.h"
#endif

#if defined(VTSS_SW_OPTION_VCL)
#include <vtss/appl/vcl.h>
#endif

#if defined(VTSS_SW_OPTION_STREAM)
#include <vtss/appl/stream.h>
#endif

#ifdef VTSS_SW_OPTION_TSN
#include <vtss/appl/tsn.h>
#endif

#ifdef VTSS_SW_OPTION_TSN_PSFP
#include <vtss/appl/psfp.h>
#endif

#ifdef VTSS_SW_OPTION_TSN_FRER
#include <vtss/appl/frer.hxx>
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_ICFG

#define VTSS_RC(expr)   { mesa_rc __rc__ = (expr); if (__rc__ < VTSS_RC_OK) return __rc__; }
#define ICFG_ICLI_RC(expr)   { i32     __rc__ = (expr); if (__rc__ < ICLI_RC_OK) return VTSS_RC_ERROR; }

// Allocation quant for query results
#define DEFAULT_TEXT_BLOCK_SIZE (1024*1024)

// Size of static buffer for ICLI command output
#define OUTPUT_BUFFER_SIZE      (4*1024)

// Maximum number of syntax errors to report when validating a configuration
#define MAX_ERROR_CNT   (5)

/* Map: Mode => { first, last } */
typedef struct {
    icli_cmd_mode_t      mode;
    vtss_icfg_ordering_t first;
    vtss_icfg_ordering_t last;
} icfg_map_t;

/* Adapt this table when new modes are introduced.
 * IMPORTANT INVARIANT: table[i].last < table[i+1].first
 * If this table is re-ordered, reorder vtss_icfg_ordering_t as well.
 *  */
static const icfg_map_t mode_to_order_map[] = {
    { ICLI_CMD_MODE_CONFIG_VLAN,         VTSS_ICFG_VLAN_BEGIN,                    VTSS_ICFG_VLAN_END                    },
#if defined(VTSS_SW_OPTION_IPMC_LIB)
    { ICLI_CMD_MODE_IPMC_PROFILE,        VTSS_ICFG_IPMC_PROFILE_BEGIN,            VTSS_ICFG_IPMC_PROFILE_END            },
#endif
#if defined(VTSS_SW_OPTION_SNMP)
    { ICLI_CMD_MODE_SNMPS_HOST,          VTSS_ICFG_SNMPSERVER_HOST_BEGIN,         VTSS_ICFG_SNMPSERVER_HOST_END         },
#endif
#if defined(VTSS_SW_OPTION_JSON_RPC_NOTIFICATION)
    { ICLI_CMD_MODE_JSON_NOTI_HOST,      VTSS_ICFG_JSON_NOTI_HOST_BEGIN,          VTSS_ICFG_JSON_NOTI_HOST_END          },
#endif
#if defined(VTSS_SW_OPTION_QOS_ADV)
    { ICLI_CMD_MODE_QOS_INGRESS_MAP,     VTSS_ICFG_QOS_INGRESS_MAP_BEGIN,         VTSS_ICFG_QOS_INGRESS_MAP_END         },
    { ICLI_CMD_MODE_QOS_EGRESS_MAP,      VTSS_ICFG_QOS_EGRESS_MAP_BEGIN,          VTSS_ICFG_QOS_EGRESS_MAP_END          },
#endif
#if defined(VTSS_SW_OPTION_STREAM)
    { ICLI_CMD_MODE_STREAM,              VTSS_ICFG_STREAM_BEGIN,                  VTSS_ICFG_STREAM_END                  },
    { ICLI_CMD_MODE_STREAM_COLLECTION,   VTSS_ICFG_STREAM_COLLECTION_BEGIN,       VTSS_ICFG_STREAM_COLLECTION_END       },
#endif
    { ICLI_CMD_MODE_LLAG,                VTSS_ICFG_INTERFACE_LLAG_BEGIN,          VTSS_ICFG_INTERFACE_LLAG_END          },
#if defined(VTSS_SW_OPTION_CFM)
    { ICLI_CMD_MODE_CFM_MD,              VTSS_ICFG_CFM_MD_BEGIN,                  VTSS_ICFG_CFM_MD_END                  },
#endif
    { ICLI_CMD_MODE_INTERFACE_PORT_LIST, VTSS_ICFG_INTERFACE_ETHERNET_BEGIN,      VTSS_ICFG_INTERFACE_ETHERNET_END      },
    { ICLI_CMD_MODE_INTERFACE_VLAN,      VTSS_ICFG_INTERFACE_VLAN_BEGIN,          VTSS_ICFG_INTERFACE_VLAN_END          },
#if defined(VTSS_SW_OPTION_APS)
    { ICLI_CMD_MODE_APS,                 VTSS_ICFG_APS_BEGIN,                     VTSS_ICFG_APS_END                     },
#endif
#if defined(VTSS_SW_OPTION_ERPS)
    { ICLI_CMD_MODE_ERPS,                VTSS_ICFG_ERPS_BEGIN,                    VTSS_ICFG_ERPS_END                    },
#endif
#if defined(VTSS_SW_OPTION_IEC_MRP)
    { ICLI_CMD_MODE_IEC_MRP,             VTSS_ICFG_IEC_MRP_BEGIN,                 VTSS_ICFG_IEC_MRP_END                 },
#endif
#if defined(VTSS_SW_OPTION_REDBOX)
    { ICLI_CMD_MODE_REDBOX,              VTSS_ICFG_REDBOX_BEGIN,                  VTSS_ICFG_REDBOX_END                  },
#endif
    { ICLI_CMD_MODE_STP_AGGR,            VTSS_ICFG_STP_AGGR_BEGIN,                VTSS_ICFG_STP_AGGR_END                },
#if defined(VTSS_SW_OPTION_DHCP_SERVER)
    { ICLI_CMD_MODE_DHCP_POOL,           VTSS_ICFG_DHCP_POOL_BEGIN,               VTSS_ICFG_DHCP_POOL_END               },
#endif
#if defined(VTSS_SW_OPTION_FRR_ROUTER)
    { ICLI_CMD_MODE_CONFIG_ROUTER_KEYCHAIN, VTSS_ICFG_ROUTER_KEYCHAIN_BEGIN,      VTSS_ICFG_ROUTER_KEYCHAIN_END         },
#endif
#if defined(VTSS_SW_OPTION_FRR_OSPF)
    { ICLI_CMD_MODE_CONFIG_ROUTER_OSPF,     VTSS_ICFG_OSPF_ROUTER_BEGIN,          VTSS_ICFG_OSPF_ROUTER_END             },
#endif
#if defined(VTSS_SW_OPTION_FRR_OSPF6)
    { ICLI_CMD_MODE_CONFIG_ROUTER_OSPF6,    VTSS_ICFG_OSPF6_ROUTER_BEGIN,         VTSS_ICFG_OSPF6_ROUTER_END            },
#endif
#if defined(VTSS_SW_OPTION_FRR_RIP)
    { ICLI_CMD_MODE_CONFIG_ROUTER_RIP,      VTSS_ICFG_RIP_ROUTER_BEGIN,           VTSS_ICFG_RIP_ROUTER_END              },
#endif
#if defined(VTSS_SW_OPTION_TSN_PSFP)
    { ICLI_CMD_MODE_TSN_PSFP_FLOW_METER, VTSS_ICFG_TSN_PSFP_FLOW_METER_BEGIN,    VTSS_ICFG_TSN_PSFP_FLOW_METER_END   },
    { ICLI_CMD_MODE_TSN_PSFP_GATE,       VTSS_ICFG_TSN_PSFP_GATE_BEGIN,          VTSS_ICFG_TSN_PSFP_GATE_END         },
    { ICLI_CMD_MODE_TSN_PSFP_FILTER,     VTSS_ICFG_TSN_PSFP_FILTER_BEGIN,        VTSS_ICFG_TSN_PSFP_FILTER_END       },
#endif
#if defined(VTSS_SW_OPTION_TSN_FRER)
    { ICLI_CMD_MODE_TSN_FRER,            VTSS_ICFG_TSN_FRER_BEGIN,                VTSS_ICFG_TSN_FRER_END                },
#endif
    { ICLI_CMD_MODE_CONFIG_LINE,         VTSS_ICFG_LINE_BEGIN,                    VTSS_ICFG_LINE_END                    }
};

#define MAP_TABLE_CNT   (sizeof(mode_to_order_map)/sizeof(icfg_map_t))

typedef struct {
    vtss_icfg_query_func_t  func;
    const char              *feature_name;
} icfg_callback_data_t;

// List of callbacks. We waste a few entries for the ...BEGIN and ...END values,
// but that's so little we don't want to exchange it for more complex code.
static icfg_callback_data_t icfg_callbacks[VTSS_ICFG_LAST];

// Critical regions
static critd_t icfg_crit;         // General access to data
static vtss::Lock icfgThreadLock; // Lock to keep track of INIT_CMD_xxx events

// ICFG thread.

#define ICFG_THREAD_FLAG_COMMIT_FILE            VTSS_BIT(0)    // Begin commit
#define ICFG_THREAD_FLAG_COMMIT_DONE            VTSS_BIT(1)    // Commit completed
#if 1 /* CP, 01/07/2014 18:05, public header */
/*
    These flags are used for reload default.
    If the API do reload default by itself, the SNMP reply will be dropped and MIB browser will fail.
    So, to avoid this issue, API sets flag to indicate ICFG thread to do reload default after sleeping 200ms
    and then exits immediately and successfully. Therefore, we have time to reply to MIB browser.
*/
#define ICFG_THREAD_FLAG_RELOAD_DEFAULT             VTSS_BIT(2)    // reload default
#define ICFG_THREAD_FLAG_RELOAD_DEFAULT_KEEP_IP     VTSS_BIT(3)    // reload default keep IP
#define ICFG_THREAD_FLAG_COPY_TO_RUNNING            VTSS_BIT(4)    // copy config to running

static BOOL                         g_copy_to_running = FALSE;
static BOOL                         g_copy_merge;
static char                         g_copy_source_path[VTSS_APPL_ICFG_FILE_STR_LEN + 1];
static vtss_icfg_query_result_t     g_copy_query_buf;
#endif

static vtss_handle_t            icfg_thread_handle;
static vtss_thread_t            icfg_thread_block;
static vtss_flag_t              icfg_thread_flag;

// Shared between threads; protected by icfg_crit.
static char                     icfg_commit_filename[PATH_MAX];     // [0] != 0 => load is initiated; == 0 => ready for next file
static vtss_icfg_query_result_t icfg_commit_buf;
static BOOL                     icfg_commit_abort_flag;
static char                     icfg_commit_output_buffer[OUTPUT_BUFFER_SIZE];
static BOOL                     icfg_commit_echo_to_console;
static u32                      icfg_commit_output_buffer_length;
static u32                      icfg_commit_error_cnt;
static icfg_commit_state_t      icfg_commit_state;

#if 1 /* CP, 01/07/2014 18:05, public header */
static vtss_appl_icfg_copy_config_t  g_copy_config;
#endif

#if defined(VTSS_SW_OPTION_IP)
static struct {
    struct {
        vtss_appl_ip_if_conf_ipv4_t ip;
        vtss_appl_ip_route_key_t    route;
        vtss_appl_ip_route_conf_t   route_conf;
        BOOL                        ip_ok;
        BOOL                        route_ok;
    } v4;
    struct {
        vtss_appl_ip_if_conf_ipv6_t ip;
        vtss_appl_ip_route_key_t    route;
        vtss_appl_ip_route_conf_t   route_conf;
        BOOL                        ip_ok;
        BOOL                        route_ok;
    } v6;
} icfg_ip_save;
#endif /* defined(VTSS_SW_OPTION_IP) */

// State for protecting whole-file load/save ops: Only one can be in
// progress at any time; overlapping requests must be denied with an error
// message.
static BOOL                     icfg_io_in_progress;

// Well-known file names
#define STARTUP_CONFIG         "startup-config"
#define STARTUP_CONFIG_CREATED "../startup-config-created"    // In parent directory
#define DEFAULT_CONFIG         "default-config"

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "icfg", "Industrial Configuration Engine"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define ICFG_CRIT_ENTER() critd_enter(&icfg_crit, __FILE__, __LINE__)
#define ICFG_CRIT_EXIT()  critd_exit( &icfg_crit, __FILE__, __LINE__)

//-----------------------------------------------------------------------------
// Query Registration
//-----------------------------------------------------------------------------
mesa_rc vtss_icfg_query_register(vtss_icfg_ordering_t   order,
                                 const char *const      feature_name,
                                 vtss_icfg_query_func_t query_cb)
{
    /*lint --e{459} */
    mesa_rc rc = VTSS_RC_OK;

    if (order >= VTSS_ICFG_LAST  ||  icfg_callbacks[order].func != NULL) {
        rc = VTSS_RC_ERROR;
    } else {
        icfg_callbacks[order].func         = query_cb;
        icfg_callbacks[order].feature_name = feature_name;
    }

    return rc;
}

//-----------------------------------------------------------------------------
// Query Request
//-----------------------------------------------------------------------------

static mesa_rc icfg_format_mode_header(vtss_icfg_query_request_t *req, vtss_icfg_query_result_t  *result)
{
    switch (req->cmd_mode) {
#if defined(VTSS_SW_OPTION_IPMC_LIB)
    case ICLI_CMD_MODE_IPMC_PROFILE:
        T_N(                             "ipmc profile %s",   req->instance_id.string);
        VTSS_RC(vtss_icfg_printf(result, "ipmc profile %s\n", req->instance_id.string));
        break;
#endif /* VTSS_SW_OPTION_IPMC_LIB */

#if defined(VTSS_SW_OPTION_SNMP)
    case ICLI_CMD_MODE_SNMPS_HOST:
        T_N(                             "snmp-server host %s",   req->instance_id.string);
        VTSS_RC(vtss_icfg_printf(result, "snmp-server host %s\n", req->instance_id.string));
        break;
#endif /* VTSS_SW_OPTION_SNMP */

#if defined(VTSS_SW_OPTION_JSON_RPC_NOTIFICATION)
    case ICLI_CMD_MODE_JSON_NOTI_HOST:
        T_N(                             "json notification host %s", req->instance_id.string);
        VTSS_RC(vtss_icfg_printf(result, "json notification host %s\n", req->instance_id.string));
        break;
#endif /* VTSS_SW_OPTION_SNMP */

    case ICLI_CMD_MODE_INTERFACE_PORT_LIST:
        T_N("interface %s %u/%u",
            icli_port_type_get_name((icli_port_type_t)req->instance_id.port.port_type),
            req->instance_id.port.switch_id,
            req->instance_id.port.begin_port);
        VTSS_RC(vtss_icfg_printf(result, "interface %s %u/%u\n",
                                 icli_port_type_get_name((icli_port_type_t)req->instance_id.port.port_type),
                                 req->instance_id.port.switch_id,
                                 req->instance_id.port.begin_port));
        break;

    case ICLI_CMD_MODE_INTERFACE_VLAN:
        T_N(                             "interface vlan %u",   req->instance_id.vlan);
        VTSS_RC(vtss_icfg_printf(result, "interface vlan %u\n", req->instance_id.vlan));
        break;

#if defined(VTSS_SW_OPTION_CFM)
    case ICLI_CMD_MODE_CFM_MD: {
        vtss_appl_cfm_md_conf_t md_conf;
        vtss_appl_cfm_md_key_t  key;
        mesa_rc                 rc;

        key.md = req->instance_id.string;
        if ((rc = vtss_appl_cfm_md_conf_get(key, &md_conf)) != VTSS_RC_OK) {
            T_E("No such domain: %s. Error = %s", key, error_txt(rc));
            return rc;
        }

        VTSS_RC(vtss_icfg_printf(result, "cfm domain %s\n", key.md.c_str()));
        break;
    }
#endif /* VTSS_SW_OPTION_CFM */

#if defined(VTSS_SW_OPTION_APS)
    case ICLI_CMD_MODE_APS: {
        vtss_appl_aps_conf_t aps_conf;
        uint32_t             inst;
        mesa_rc              rc;

        inst = req->instance_id.generic_u32;
        if ((rc = vtss_appl_aps_conf_get(inst, &aps_conf)) != VTSS_RC_OK) {
            T_E("No such APS instance: %u. Error = %s", inst, error_txt(rc));
            return rc;
        }

        VTSS_RC(vtss_icfg_printf(result, "aps %u\n", inst));
        break;
    }
#endif /* VTSS_SW_OPTION_APS */

#if defined(VTSS_SW_OPTION_ERPS)
    case ICLI_CMD_MODE_ERPS: {
        vtss_appl_erps_conf_t erps_conf;
        uint32_t              inst;
        mesa_rc               rc;

        inst = req->instance_id.generic_u32;
        if ((rc = vtss_appl_erps_conf_get(inst, &erps_conf)) != VTSS_RC_OK) {
            T_E("No such ERPS instance: %u. Error = %s", inst, error_txt(rc));
            return rc;
        }

        VTSS_RC(vtss_icfg_printf(result, "erps %u\n", inst));
        break;
    }
#endif /* VTSS_SW_OPTION_ERPS */

#if defined(VTSS_SW_OPTION_IEC_MRP)
    case ICLI_CMD_MODE_IEC_MRP: {
        vtss_appl_iec_mrp_conf_t mrp_conf;
        uint32_t                 inst;
        mesa_rc                  rc;

        inst = req->instance_id.generic_u32;
        if ((rc = vtss_appl_iec_mrp_conf_get(inst, &mrp_conf)) != VTSS_RC_OK) {
            T_E("No such media-redundancy instance: %u. Error = %s", inst, error_txt(rc));
            return rc;
        }

        VTSS_RC(vtss_icfg_printf(result, "media-redundancy %u\n", inst));
        break;
    }
#endif /* VTSS_SW_OPTION_IEC_MRP */

#if defined(VTSS_SW_OPTION_REDBOX)
    case ICLI_CMD_MODE_REDBOX: {
        vtss_appl_redbox_conf_t mrp_conf;
        uint32_t                inst;
        mesa_rc                 rc;

        inst = req->instance_id.generic_u32;
        if ((rc = vtss_appl_redbox_conf_get(inst, &mrp_conf)) != VTSS_RC_OK) {
            T_E("No such redbox instance: %u. Error = %s", inst, error_txt(rc));
            return rc;
        }

        VTSS_RC(vtss_icfg_printf(result, "redbox %u\n", inst));
        break;
    }
#endif /* VTSS_SW_OPTION_REDBOX */

#if defined(VTSS_SW_OPTION_STREAM)
    case ICLI_CMD_MODE_STREAM: {
        vtss_appl_stream_conf_t stream_conf;
        uint32_t                inst;
        mesa_rc                 rc;

        inst = req->instance_id.generic_u32;
        if ((rc = vtss_appl_stream_conf_get(inst, &stream_conf)) != VTSS_RC_OK) {
            T_E("No such stream instance: %u. Error = %s", inst, error_txt(rc));
            return rc;
        }

        VTSS_RC(vtss_icfg_printf(result, "stream %u\n", inst));
        break;
    }

    case ICLI_CMD_MODE_STREAM_COLLECTION: {
        vtss_appl_stream_collection_conf_t stream_collection_conf;
        uint32_t                           inst;
        mesa_rc                            rc;

        inst = req->instance_id.generic_u32;
        if ((rc = vtss_appl_stream_collection_conf_get(inst, &stream_collection_conf)) != VTSS_RC_OK) {
            T_E("No such stream collection_instance: %u. Error = %s", inst, error_txt(rc));
            return rc;
        }

        VTSS_RC(vtss_icfg_printf(result, "stream-collection %u\n", inst));
        break;
    }
#endif /* VTSS_SW_OPTION_STREAM */

#if defined(VTSS_SW_OPTION_TSN_PSFP)
    case ICLI_CMD_MODE_TSN_PSFP_FLOW_METER: {
        vtss_appl_psfp_flow_meter_conf_t flow_meter_conf;
        vtss_appl_psfp_flow_meter_id_t   flow_meter_id;
        mesa_rc                          rc;

        flow_meter_id = req->instance_id.generic_u32;
        if ((rc = vtss_appl_psfp_flow_meter_conf_get(flow_meter_id, &flow_meter_conf)) != VTSS_RC_OK) {
            T_E("No such PSFP flow meter instance: %u. Error = %s", flow_meter_id, error_txt(rc));
            return rc;
        }

        VTSS_RC(vtss_icfg_printf(result, "tsn flow meter %u\n", flow_meter_id));
        break;
    }
#endif /* defined VTSS_SW_OPTION_TSN_PSFP */

#if defined(VTSS_SW_OPTION_TSN_PSFP)
    case ICLI_CMD_MODE_TSN_PSFP_GATE: {
        vtss_appl_psfp_gate_conf_t gate_conf;
        vtss_appl_psfp_gate_id_t   gate_id;
        mesa_rc                    rc;

        gate_id = req->instance_id.generic_u32;
        if ((rc = vtss_appl_psfp_gate_conf_get(gate_id, &gate_conf)) != VTSS_RC_OK) {
            T_E("No such PSFP stream gate instance: %u. Error = %s", gate_id, error_txt(rc));
            return rc;
        }

        VTSS_RC(vtss_icfg_printf(result, "tsn stream gate %u\n", gate_id));
        break;
    }
#endif /* defined VTSS_SW_OPTION_TSN_PSFP */

#if defined(VTSS_SW_OPTION_TSN_PSFP)
    case ICLI_CMD_MODE_TSN_PSFP_FILTER: {
        vtss_appl_psfp_filter_conf_t filter_conf;
        vtss_appl_psfp_filter_id_t   filter_id;
        mesa_rc                      rc;

        filter_id = req->instance_id.generic_u32;
        if ((rc = vtss_appl_psfp_filter_conf_get(filter_id, &filter_conf)) != VTSS_RC_OK) {
            T_E("No such PSFP stream filter instance: %u. Error = %s", filter_id, error_txt(rc));
            return rc;
        }

        VTSS_RC(vtss_icfg_printf(result, "tsn stream filter %u\n", filter_id));
        break;
    }
#endif /* defined VTSS_SW_OPTION_TSN_PSFP */

#if defined(VTSS_SW_OPTION_TSN_FRER)
    case ICLI_CMD_MODE_TSN_FRER: {
        vtss_appl_frer_conf_t  frer_conf;
        uint32_t               inst;
        mesa_rc                rc;

        inst = req->instance_id.generic_u32;
        if ((rc = vtss_appl_frer_conf_get(inst, &frer_conf)) != VTSS_RC_OK) {
            T_E("No such FRER instance: %u. Error = %s", inst, error_txt(rc));
            return rc;
        }

        VTSS_RC(vtss_icfg_printf(result, "tsn frer %u\n", inst));
        break;
    }
#endif /* defined(VTSS_SW_OPTION_TSN_FRER) */

    case ICLI_CMD_MODE_STP_AGGR:
        T_N(                             "spanning-tree aggregation");
        VTSS_RC(vtss_icfg_printf(result, "spanning-tree aggregation\n"));
        break;

    case ICLI_CMD_MODE_CONFIG_LINE:
        if (req->instance_id.line == 0) {
            T_N(                             "line console 0");
            VTSS_RC(vtss_icfg_printf(result, "line console 0\n"));
        } else {
            T_N(                             "line vty %u",   req->instance_id.line - 1);
            VTSS_RC(vtss_icfg_printf(result, "line vty %u\n", req->instance_id.line - 1));
        }
        break;

    case ICLI_CMD_MODE_LLAG:
        T_N(                             "interface llag %u",   req->instance_id.llag_no);
        VTSS_RC(vtss_icfg_printf(result, "interface llag %u\n", req->instance_id.llag_no));
        break;

#if defined(VTSS_SW_OPTION_DHCP_SERVER)
    case ICLI_CMD_MODE_DHCP_POOL:
        T_N(                             "ip dhcp pool %s",   req->instance_id.string);
        VTSS_RC(vtss_icfg_printf(result, "ip dhcp pool %s\n", req->instance_id.string));
        break;
#endif

#if defined(VTSS_SW_OPTION_QOS_ADV)
    case ICLI_CMD_MODE_QOS_INGRESS_MAP:
        T_N(                             "qos map ingress %u",   req->instance_id.map_id);
        VTSS_RC(vtss_icfg_printf(result, "qos map ingress %u\n", req->instance_id.map_id));
        break;

    case ICLI_CMD_MODE_QOS_EGRESS_MAP:
        T_N(                             "qos map egress %u",   req->instance_id.map_id);
        VTSS_RC(vtss_icfg_printf(result, "qos map egress %u\n", req->instance_id.map_id));
        break;
#endif /* VTSS_SW_OPTION_QOS_ADV */

#if defined(VTSS_SW_OPTION_FRR_ROUTER)
    case ICLI_CMD_MODE_CONFIG_ROUTER_KEYCHAIN:
        T_N(                             "key chain %s",   req->instance_id.string);
        VTSS_RC(vtss_icfg_printf(result, "key chain %s\n", req->instance_id.string));
        break;
#endif /* VTSS_SW_OPTION_FRR_ROUTER */

#if defined(VTSS_SW_OPTION_FRR_OSPF)
    case ICLI_CMD_MODE_CONFIG_ROUTER_OSPF:
        T_N(                             "router ospf %u",   req->instance_id.generic_u32);
        VTSS_RC(vtss_icfg_printf(result, "router ospf\n"));
        // TODO, OSPF. Modify this line when multiple OSPF instances are
        // supported
        // VTSS_RC(vtss_icfg_printf(result, "router ospf %u\n", req->instance_id.generic_u32));
        break;
#endif /* VTSS_SW_OPTION_FRR_OSPF */

#if defined(VTSS_SW_OPTION_FRR_OSPF6)
    case ICLI_CMD_MODE_CONFIG_ROUTER_OSPF6:
        T_N(                             "router ospf6 %u",   req->instance_id.generic_u32);
        VTSS_RC(vtss_icfg_printf(result, "router ospf6\n"));
        // TODO, OSPF6. Modify this line when multiple OSPF6 instances are
        // supported
        // VTSS_RC(vtss_icfg_printf(result, "router ospf6 %u\n", req->instance_id.generic_u32));
        break;
#endif /* VTSS_SW_OPTION_FRR_OSPF6 */

#if defined(VTSS_SW_OPTION_FRR_RIP)
    case ICLI_CMD_MODE_CONFIG_ROUTER_RIP:
        T_N(                             "router rip");
        VTSS_RC(vtss_icfg_printf(result, "router rip\n"));
        break;
#endif /* VTSS_SW_OPTION_FRR_RIP */

    default:
        // Ignore
        break;
    }

    return VTSS_RC_OK;
}

static mesa_rc icfg_iterate_callbacks(vtss_icfg_query_request_t *req,
                                      vtss_icfg_ordering_t      first,
                                      vtss_icfg_ordering_t      last,
                                      const char *const        feature_name,
                                      vtss_icfg_query_result_t  *result)
{
    mesa_rc                rc = VTSS_RC_OK;
    vtss_icfg_query_func_t f;

    T_N("Beginning iteration, [%d;%d[", first, last);
    ICFG_CRIT_ENTER();
    for (; (first < last)  &&  (rc == VTSS_RC_OK); ++first) {
        BOOL invoke = icfg_callbacks[first].func != NULL;
        if (feature_name != NULL) {
            invoke = invoke  &&  (icfg_callbacks[first].feature_name != NULL)  &&  !strcmp(feature_name, icfg_callbacks[first].feature_name);
        }

        if (invoke) {
            req->order = first;
            f = icfg_callbacks[first].func;
            T_N("Invoking callback for %s, order %d ptr: %p", icfg_callbacks[first].feature_name, first, f);
            ICFG_CRIT_EXIT();
            rc = (f)(req, result);
            T_N("Callback done, rc %d", rc);
            if (rc != VTSS_RC_OK) {
                T_D("ICFG Synth. failure for %s, order %d, rc = %d", icfg_callbacks[first].feature_name, first, rc);
                rc = VTSS_RC_OK;
            }
            ICFG_CRIT_ENTER();
        }
    }
    ICFG_CRIT_EXIT();
    T_N("Iteration done, rc %d", rc);

    return rc;
}

static bool icfg_callback_required(vtss_icfg_ordering_t first,
                                   vtss_icfg_ordering_t last,
                                   const char *const    feature_name)
{
    bool result = false;

    // This function checks whether a callback will be invoked. If not, there's
    // no reason to print-out header and footer for each section.

    if (feature_name == NULL) {
        return true;
    }

    ICFG_CRIT_ENTER();
    for (; (first < last); ++first) {
        icfg_callback_data_t *cb = &icfg_callbacks[first];
        if (cb->func && cb->feature_name && strcmp(cb->feature_name, feature_name) == 0) {
            // Callback required
            result = true;
            break;
        }
    }
    ICFG_CRIT_EXIT();
    return result;
}

#define ICFG_VTSS_RC(x) {                          \
    mesa_rc rc = (x);                              \
    if (rc != VTSS_RC_OK) {                        \
        T_D("ICFG Synth. failure, rc = %d", rc);   \
    }                                              \
}

static mesa_rc icfg_process_entity(vtss_icfg_query_request_t *req,
                                   vtss_icfg_ordering_t       first,
                                   vtss_icfg_ordering_t       last,
                                   const char *const          feature_name,
                                   vtss_icfg_query_result_t   *result)
{

    if (!icfg_callback_required(first, last, feature_name)) {
        T_N("No reason to process entity, as no callbacks will be invoked");
        return VTSS_RC_OK;
    }

    switch (req->cmd_mode) {
    case ICLI_CMD_MODE_GLOBAL_CONFIG:
    case ICLI_CMD_MODE_CONFIG_VLAN:
    case ICLI_CMD_MODE_SNMPS_HOST:
    case ICLI_CMD_MODE_JSON_NOTI_HOST:
    case ICLI_CMD_MODE_IPMC_PROFILE:
    case ICLI_CMD_MODE_STP_AGGR:
    case ICLI_CMD_MODE_CONFIG_LINE:
    case ICLI_CMD_MODE_DHCP_POOL:
    case ICLI_CMD_MODE_QOS_INGRESS_MAP:
    case ICLI_CMD_MODE_QOS_EGRESS_MAP:
    case ICLI_CMD_MODE_CFM_MD:
    case ICLI_CMD_MODE_APS:
    case ICLI_CMD_MODE_ERPS:
    case ICLI_CMD_MODE_IEC_MRP:
    case ICLI_CMD_MODE_REDBOX:
    case ICLI_CMD_MODE_CONFIG_ROUTER_KEYCHAIN:
    case ICLI_CMD_MODE_CONFIG_ROUTER_OSPF:
    case ICLI_CMD_MODE_CONFIG_ROUTER_OSPF6:
    case ICLI_CMD_MODE_CONFIG_ROUTER_RIP:
    case ICLI_CMD_MODE_INTERFACE_VLAN:
    case ICLI_CMD_MODE_LLAG:
    case ICLI_CMD_MODE_STREAM:
    case ICLI_CMD_MODE_STREAM_COLLECTION:
    case ICLI_CMD_MODE_TSN_PSFP_FLOW_METER:
    case ICLI_CMD_MODE_TSN_PSFP_GATE:
    case ICLI_CMD_MODE_TSN_PSFP_FILTER:
    case ICLI_CMD_MODE_TSN_FRER:
        ICFG_VTSS_RC(icfg_format_mode_header(req, result));
        ICFG_VTSS_RC(icfg_iterate_callbacks(req, first, last, feature_name, result));
        ICFG_VTSS_RC(vtss_icfg_printf(result, VTSS_ICFG_COMMENT_LEADIN"\n"));
        break;

    case ICLI_CMD_MODE_INTERFACE_PORT_LIST: {
        ICFG_VTSS_RC(icfg_format_mode_header(req, result));
        ICFG_VTSS_RC(icfg_iterate_callbacks(req, first, last, feature_name, result));
        ICFG_VTSS_RC(vtss_icfg_printf(result, VTSS_ICFG_COMMENT_LEADIN"\n"));
    }
    break;

    default:
        T_E("That's really odd; shouldn't get here. Porting issue?");
        break;
    }

    return VTSS_RC_OK;
}

static mesa_rc icfg_process_range(icli_cmd_mode_t          mode,
                                  vtss_icfg_ordering_t     first,
                                  vtss_icfg_ordering_t     last,
                                  BOOL                     all_defaults,
                                  const char *const        feature_name,
                                  vtss_icfg_query_result_t *result)
{
    vtss_icfg_query_request_t req;

    T_N("Entry: mode %s (%d), [%d;%d[", icli_mode_name_get(mode), mode, first, last);

    req.cmd_mode     = mode;
    req.all_defaults = all_defaults;

    switch (mode) {
    case ICLI_CMD_MODE_GLOBAL_CONFIG:
    case ICLI_CMD_MODE_STP_AGGR:
        // The above are global/single-instance, or handle the iteration themselves, so no enumeration is required
        (void)icfg_process_entity(&req, first, last, feature_name, result);
        break;

    case ICLI_CMD_MODE_CONFIG_VLAN:
        // Get running-config for all VLANs.
        req.instance_id.vlan_list.cnt = 1;
        req.instance_id.vlan_list.range[0].min = VTSS_APPL_VLAN_ID_MIN;
        req.instance_id.vlan_list.range[0].max = VTSS_APPL_VLAN_ID_MAX;
        (void)icfg_process_entity(&req, first, last, feature_name, result);
        break;

    case ICLI_CMD_MODE_SNMPS_HOST: {
#if defined(VTSS_SW_OPTION_SNMP)
        vtss_trap_entry_t trap_entry;

        memset(trap_entry.trap_conf_name, 0, sizeof(trap_entry.trap_conf_name));
        while (VTSS_RC_OK == trap_mgmt_conf_get_next (&trap_entry)) {
            strncpy((char *)req.instance_id.string, (const char *)trap_entry.trap_conf_name, TRAP_MAX_NAME_LEN);
            req.instance_id.string[TRAP_MAX_NAME_LEN] = 0;
            (void) icfg_process_entity(&req, first, last, feature_name, result);
        }
#endif
    }
    break;

    case ICLI_CMD_MODE_JSON_NOTI_HOST: {
#if defined(VTSS_SW_OPTION_JSON_RPC_NOTIFICATION)
        req.instance_id.string[0] = 0;
        while (VTSS_RC_OK == JSON_RPC_icli_event_host_next(req.instance_id.string)) {
            (void) icfg_process_entity(&req, first, last, feature_name, result);
        }
#endif
    }
    break;

    case ICLI_CMD_MODE_IPMC_PROFILE: {
#if defined(VTSS_SW_OPTION_IPMC_LIB) && defined(VTSS_SW_OPTION_SMB_IPMC)
        vtss_appl_ipmc_lib_profile_key_t profile_key = {};

        while (vtss_appl_ipmc_lib_profile_itr(&profile_key, &profile_key) == VTSS_RC_OK) {
            strncpy(req.instance_id.string, profile_key.name, sizeof(req.instance_id.string));
            (void)icfg_process_entity(&req, first, last, feature_name, result);
        }
#endif
    }
    break;

    case ICLI_CMD_MODE_INTERFACE_PORT_LIST: {
        icli_switch_port_range_t switch_range;
        BOOL                     good = icli_port_get_first(&switch_range);

        while (good) {
            req.instance_id.port = switch_range;
            (void) icfg_process_entity(&req, first, last, feature_name, result);
            good = icli_port_get_next(&switch_range);
        }
    }
    break;

    case ICLI_CMD_MODE_INTERFACE_VLAN: {
#if defined(VTSS_SW_OPTION_IP)
        vtss_ifindex_t prev_ifindex = VTSS_IFINDEX_NONE, ifindex;
        while (vtss_appl_ip_if_itr(&prev_ifindex, &ifindex) == VTSS_RC_OK) {
            prev_ifindex = ifindex;

            if ((req.instance_id.vlan = vtss_ifindex_as_vlan(ifindex)) == 0) {
                // Not a VLAN IPinterface
                continue;
            }

            (void)icfg_process_entity(&req, first, last, feature_name, result);
        }
#endif
    }
    break;

    case ICLI_CMD_MODE_CFM_MD: {
#ifdef VTSS_SW_OPTION_CFM
        vtss_appl_cfm_md_key_t prev_key, next_key;
        bool frst = true;

        while (vtss_appl_cfm_md_itr(frst ? NULL : &prev_key, &next_key) == VTSS_RC_OK) {
            prev_key = next_key;
            strcpy(req.instance_id.string, next_key.md.c_str());
            (void)icfg_process_entity(&req, first, last, feature_name, result);
            frst = false;
        }
#endif
    }
    break;

    case ICLI_CMD_MODE_APS: {
#ifdef VTSS_SW_OPTION_APS
        uint32_t prev_inst, next_inst;
        bool frst = true;

        while (vtss_appl_aps_itr(frst ? NULL : &prev_inst, &next_inst) == VTSS_RC_OK) {
            prev_inst = next_inst;
            req.instance_id.generic_u32 = next_inst;
            (void)icfg_process_entity(&req, first, last, feature_name, result);
            frst = false;
        }
#endif
    }
    break;

    case ICLI_CMD_MODE_ERPS: {
#ifdef VTSS_SW_OPTION_ERPS
        uint32_t prev_inst, next_inst;
        bool frst = true;

        while (vtss_appl_erps_itr(frst ? NULL : &prev_inst, &next_inst) == VTSS_RC_OK) {
            prev_inst = next_inst;
            req.instance_id.generic_u32 = next_inst;
            (void)icfg_process_entity(&req, first, last, feature_name, result);
            frst = false;
        }
#endif
    }
    break;

    case ICLI_CMD_MODE_IEC_MRP: {
#ifdef VTSS_SW_OPTION_IEC_MRP
        uint32_t prev_inst, next_inst;
        bool frst = true;

        while (vtss_appl_iec_mrp_itr(frst ? NULL : &prev_inst, &next_inst) == VTSS_RC_OK) {
            prev_inst = next_inst;
            req.instance_id.generic_u32 = next_inst;
            (void)icfg_process_entity(&req, first, last, feature_name, result);
            frst = false;
        }
#endif
    }
    break;

    case ICLI_CMD_MODE_REDBOX: {
#ifdef VTSS_SW_OPTION_REDBOX
        uint32_t prev_inst, next_inst;
        bool frst = true;

        while (vtss_appl_redbox_itr(frst ? NULL : &prev_inst, &next_inst) == VTSS_RC_OK) {
            prev_inst = next_inst;
            req.instance_id.generic_u32 = next_inst;
            (void)icfg_process_entity(&req, first, last, feature_name, result);
            frst = false;
        }
#endif
    }
    break;

    case ICLI_CMD_MODE_CONFIG_LINE: {
        u32                 max = icli_session_max_get();
        u32                 i;
        icli_session_data_t ses;

        for (i = 0; i < max; ++i) {
            ses.session_id = i;
            ICFG_ICLI_RC(icli_session_data_get(&ses));
            req.instance_id.line = i;
            (void) icfg_process_entity(&req, first, last, feature_name, result);
        }
    }
    break;

    case ICLI_CMD_MODE_LLAG: {
#if defined(VTSS_SW_OPTION_AGGR)
        mesa_rc                  rc;
        vtss_ifindex_t           ifindex;
        vtss_appl_aggr_group_status_t grp_status;
        aggr_mgmt_group_no_t     aggr_no;

        for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < AGGR_MGMT_GROUP_NO_END_; aggr_no++) {
            VTSS_RC(vtss_ifindex_from_llag(VTSS_ISID_START, aggr_no, &ifindex));
            if ((rc = vtss_appl_aggregation_status_get(ifindex, &grp_status)) != VTSS_RC_OK) {
                continue;
            }
            req.instance_id.llag_no = (u32)aggr_no;
            (void) icfg_process_entity(&req, first, last, feature_name, result);
        }
#endif
    }
    break;

    case ICLI_CMD_MODE_DHCP_POOL: {
#if defined(VTSS_SW_OPTION_DHCP_SERVER)
        dhcp_server_pool_t  pool;

        memset(&pool, 0, sizeof(dhcp_server_pool_t));
        while ( dhcp_server_pool_get_next(&pool) == VTSS_RC_OK ) {
            memset(req.instance_id.string, 0, sizeof(req.instance_id.string));
            strcpy(req.instance_id.string, pool.pool_name);
            (void) icfg_process_entity(&req, first, last, feature_name, result);
        }
#endif
    }
    break;

    case ICLI_CMD_MODE_QOS_INGRESS_MAP:
#if defined(VTSS_SW_OPTION_QOS_ADV)
        if (vtss_appl_qos_capabilities->has_ingress_map) {
            mesa_qos_ingress_map_id_t id;
            for (id = vtss_appl_qos_capabilities->ingress_map_id_min; id <= vtss_appl_qos_capabilities->ingress_map_id_max; id++) {
                vtss_appl_qos_imap_entry_t c, dc;
                VTSS_RC(vtss_appl_qos_imap_entry_get(id, &c));
                VTSS_RC(vtss_appl_qos_imap_entry_get_default(id, &dc));
                if (all_defaults || (c.used != dc.used)) {
                    if (c.used) {
                        req.instance_id.map_id = id;
                        (void) icfg_process_entity(&req, first, last, feature_name, result);
                    } else if (dc.used) { /* show those that are enabled by default but manually disabled */
                        VTSS_RC(vtss_icfg_printf(result, "no qos map ingress %u\n!\n", id));
                    }
                }
            }
        }
#endif
        break;

    case ICLI_CMD_MODE_QOS_EGRESS_MAP:
#if defined(VTSS_SW_OPTION_QOS_ADV)
        if (vtss_appl_qos_capabilities->has_egress_map) {
            mesa_qos_egress_map_id_t id;
            for (id = vtss_appl_qos_capabilities->egress_map_id_min; id <= vtss_appl_qos_capabilities->egress_map_id_max; id++) {
                vtss_appl_qos_emap_entry_t c, dc;
                VTSS_RC(vtss_appl_qos_emap_entry_get(id, &c));
                VTSS_RC(vtss_appl_qos_emap_entry_get_default(id, &dc));
                if (all_defaults || (c.used != dc.used)) {
                    if (c.used) {
                        req.instance_id.map_id = id;
                        (void) icfg_process_entity(&req, first, last, feature_name, result);
                    } else if (dc.used) { /* show those that are enabled by default but manually disabled */
                        VTSS_RC(vtss_icfg_printf(result, "no qos map egress %u\n!\n", id));
                    }
                }
            }
        }
#endif /* VTSS_SW_OPTION_QOS_ADV */
        break;

    case ICLI_CMD_MODE_CONFIG_ROUTER_KEYCHAIN: {
#if defined(VTSS_SW_OPTION_FRR_ROUTER)
        if (frr_has_router()) {
            vtss_appl_router_key_chain_name_t key_chain_name = {}, next_key_chain_name;

            if (sizeof(key_chain_name) > sizeof(req.instance_id.string)) {
                T_E("Router key-chain name size overflow!");
            }

            while (vtss_appl_router_key_chain_name_itr(&key_chain_name, &next_key_chain_name) == VTSS_RC_OK) {
                // Next loop key
                strcpy(key_chain_name.name, next_key_chain_name.name);

                strcpy(req.instance_id.string, key_chain_name.name);
                (void) icfg_process_entity(&req, first, last, feature_name, result);
            }
        }
#endif /* VTSS_SW_OPTION_FRR_ROUTER */
    }
    break;

    case ICLI_CMD_MODE_CONFIG_ROUTER_OSPF: {
#if defined(VTSS_SW_OPTION_FRR_OSPF)
        if (frr_has_ospfd()) {
            vtss_appl_ospf_id_t id, next_id;
            vtss_appl_ospf_id_t *current_id = NULL;
            while (vtss_appl_ospf_inst_itr(current_id, &next_id) == VTSS_RC_OK) {
                req.instance_id.generic_u32 = next_id;
                (void) icfg_process_entity(&req, first, last, feature_name, result);

                // Update instance ID for the next loop
                if (!current_id) {
                    current_id = &id;
                }
                *current_id = next_id;
            }
        }
#endif /* VTSS_SW_OPTION_FRR_OSPF */
    }
    break;

    case ICLI_CMD_MODE_CONFIG_ROUTER_OSPF6: {
#if defined(VTSS_SW_OPTION_FRR_OSPF6)
        if (frr_has_ospf6d()) {
            vtss_appl_ospf6_id_t id, next_id;
            vtss_appl_ospf6_id_t *current_id = NULL;
            while (vtss_appl_ospf6_inst_itr(current_id, &next_id) == VTSS_RC_OK) {
                req.instance_id.generic_u32 = next_id;
                (void) icfg_process_entity(&req, first, last, feature_name, result);

                // Update instance ID for the next loop
                if (!current_id) {
                    current_id = &id;
                }
                *current_id = next_id;
            }
        }
#endif /* VTSS_SW_OPTION_FRR_OSPF6 */
    }
    break;

    case ICLI_CMD_MODE_CONFIG_ROUTER_RIP: {
#if defined(VTSS_SW_OPTION_FRR_RIP)
        if (frr_has_ripd()) {
            vtss_appl_rip_router_conf_t conf;

            /* Enter to RIP router mode when RIP is enabled */
            if (vtss_appl_rip_router_conf_get(&conf) == VTSS_RC_OK && conf.router_mode) {
                (void) icfg_process_entity(&req, first, last, feature_name, result);
            }
        }
#endif /* VTSS_SW_OPTION_FRR_RIP */
    }
    break;

    case ICLI_CMD_MODE_STREAM: {
#if defined(VTSS_SW_OPTION_STREAM)
        vtss_appl_stream_id_t stream_id = VTSS_APPL_STREAM_ID_NONE;

        while (vtss_appl_stream_itr(&stream_id, &stream_id) == VTSS_RC_OK) {
            req.instance_id.generic_u32 = stream_id;
            (void)icfg_process_entity(&req, first, last, feature_name, result);
        }
#endif /* defined VTSS_SW_OPTION_STREAM */
    }
    break;

    case ICLI_CMD_MODE_STREAM_COLLECTION: {
#if defined(VTSS_SW_OPTION_STREAM)
        vtss_appl_stream_collection_id_t stream_collection_id = VTSS_APPL_STREAM_COLLECTION_ID_NONE;

        while (vtss_appl_stream_collection_itr(&stream_collection_id, &stream_collection_id) == VTSS_RC_OK) {
            req.instance_id.generic_u32 = stream_collection_id;
            (void)icfg_process_entity(&req, first, last, feature_name, result);
        }
#endif /* defined VTSS_SW_OPTION_STREAM */
    }
    break;

    case ICLI_CMD_MODE_TSN_PSFP_FLOW_METER: {
#if defined(VTSS_SW_OPTION_TSN_PSFP)
        vtss_appl_psfp_flow_meter_id_t flow_meter_id = VTSS_APPL_PSFP_FLOW_METER_ID_NONE;

        while (vtss_appl_psfp_flow_meter_itr(&flow_meter_id, &flow_meter_id) == VTSS_RC_OK) {
            req.instance_id.generic_u32 = flow_meter_id;
            (void)icfg_process_entity(&req, first, last, feature_name, result);
        }
#endif /* defined VTSS_SW_OPTION_TSN_PSFP */
    }
    break;

    case ICLI_CMD_MODE_TSN_PSFP_GATE: {
#if defined(VTSS_SW_OPTION_TSN_PSFP)
        vtss_appl_psfp_gate_id_t gate_id = VTSS_APPL_PSFP_GATE_ID_NONE;
        while (vtss_appl_psfp_gate_itr(&gate_id, &gate_id) == VTSS_RC_OK) {
            req.instance_id.generic_u32 = gate_id;
            (void)icfg_process_entity(&req, first, last, feature_name, result);
        }
#endif /* defined VTSS_SW_OPTION_TSN_PSFP */
    }
    break;

    case ICLI_CMD_MODE_TSN_PSFP_FILTER: {
#if defined(VTSS_SW_OPTION_TSN_PSFP)
        vtss_appl_psfp_filter_id_t filter_id = VTSS_APPL_PSFP_FILTER_ID_NONE;
        while (vtss_appl_psfp_filter_itr(&filter_id, &filter_id) == VTSS_RC_OK) {
            req.instance_id.generic_u32 = filter_id;
            (void)icfg_process_entity(&req, first, last, feature_name, result);
        }
#endif /* defined VTSS_SW_OPTION_TSN_PSFP */
    }
    break;

    case ICLI_CMD_MODE_TSN_FRER: {
#if defined(VTSS_SW_OPTION_TSN_FRER)
        uint32_t prev_inst, next_inst;
        bool     frst = true;

        while (vtss_appl_frer_itr(frst ? NULL : &prev_inst, &next_inst) == VTSS_RC_OK) {
            prev_inst = next_inst;
            req.instance_id.generic_u32 = next_inst;
            (void)icfg_process_entity(&req, first, last, feature_name, result);
            frst = false;
        }
#endif /* defined(VTSS_SW_OPTION_TSN_FRER) */
    }
    break;

    default:
        T_E("That's really odd; shouldn't get here. Porting issue? (mode = %d)", mode);
        break;
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_icfg_query_feature(BOOL                     all_defaults,
                                const char *const       feature_name,
                                vtss_icfg_query_result_t *result)
{
    u32                  map_idx = 0;
    vtss_icfg_ordering_t next = (vtss_icfg_ordering_t)0;

    if (!msg_switch_is_primary()) {
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_icfg_init_query_result(0, result));

    while (next < VTSS_ICFG_LAST) {
        if (map_idx < MAP_TABLE_CNT) {
            if (next < mode_to_order_map[map_idx].first) {
                /* Global section prior to/in-between submode(s) */
                VTSS_RC(icfg_process_range(ICLI_CMD_MODE_GLOBAL_CONFIG, next, mode_to_order_map[map_idx].first, all_defaults, feature_name, result));
                next = (vtss_icfg_ordering_t)((int)mode_to_order_map[map_idx].first + 1);
            } else {
                /* Submode */
                VTSS_RC(icfg_process_range(mode_to_order_map[map_idx].mode, next, mode_to_order_map[map_idx].last, all_defaults, feature_name, result));
                next = (vtss_icfg_ordering_t)((int)mode_to_order_map[map_idx].last + 1);
                map_idx++;
            }
        } else {
            /* Global section after last submode */
            VTSS_RC(icfg_process_range(ICLI_CMD_MODE_GLOBAL_CONFIG, next, VTSS_ICFG_LAST, all_defaults, feature_name, result));
            next = VTSS_ICFG_LAST;
        }
    }

    VTSS_RC(vtss_icfg_printf(result, "end\n"));
    return VTSS_RC_OK;
}

mesa_rc vtss_icfg_query_all(BOOL all_defaults, vtss_icfg_query_result_t *result)
{
    return vtss_icfg_query_feature(all_defaults, NULL, result);
}

mesa_rc vtss_icfg_query_specific(vtss_icfg_query_request_t *req,
                                 vtss_icfg_query_result_t  *result)
{
    u32 i;

    if (!msg_switch_is_primary()) {
        return VTSS_RC_ERROR;
    }

    for (i = 0; i < MAP_TABLE_CNT  &&  mode_to_order_map[i].mode != req->cmd_mode; ++i) {
        // search
    }

    if (i < MAP_TABLE_CNT) {
        (void) icfg_process_entity(req, (vtss_icfg_ordering_t)(mode_to_order_map[i].first + 1), mode_to_order_map[i].last, NULL, result);
    }

    return VTSS_RC_OK;
}



//-----------------------------------------------------------------------------
// Query Result
//-----------------------------------------------------------------------------

static vtss_icfg_query_result_buf_t *icfg_alloc_buf(u32 initial_size)
{
    vtss_icfg_query_result_buf_t *buf;

    if (!initial_size) {
        initial_size = DEFAULT_TEXT_BLOCK_SIZE;
    }

    buf = (vtss_icfg_query_result_buf_t *)VTSS_MALLOC(sizeof(vtss_icfg_query_result_buf_t));

    if (buf) {
        buf->text      = (char *) VTSS_MALLOC(initial_size);
        buf->free_text = TRUE;
        buf->used      = 0;
        buf->size      = initial_size;
        buf->next      = NULL;
    }

    return buf;
}

mesa_rc vtss_icfg_init_query_result(u32 initial_size,
                                    vtss_icfg_query_result_t *res)
{
    if (!msg_switch_is_primary()) {
        res->head = NULL;
        res->tail = NULL;
        return VTSS_RC_ERROR;
    }

    res->head = icfg_alloc_buf(initial_size);
    res->tail = res->head;

    return res->head ? VTSS_RC_OK : VTSS_RC_ERROR;
}

void vtss_icfg_free_query_result(vtss_icfg_query_result_t *res)
{
    if (!msg_switch_is_primary()) {
        return;
    }

    vtss_icfg_query_result_buf_t *current, *next;

    current = res->head;
    while (current != NULL) {
        next = current->next;
        if (current->free_text  &&  current->text) {
            VTSS_FREE(current->text);
        }
        VTSS_FREE(current);
        current = next;
    }

    res->head = NULL;
    res->tail = NULL;
}

mesa_rc vtss_icfg_overlay_query_result(char *in_buf, u32 length,
                                       vtss_icfg_query_result_t *res)
{
    vtss_icfg_query_result_buf_t *buf;

    if (!msg_switch_is_primary()) {
        res->head = NULL;
        res->tail = NULL;
        return VTSS_RC_ERROR;
    }

    buf = (vtss_icfg_query_result_buf_t *)VTSS_MALLOC(sizeof(vtss_icfg_query_result_buf_t));

    if (buf) {
        buf->text      = in_buf;
        buf->free_text = FALSE;
        buf->used      = length;
        buf->size      = length;
        buf->next      = NULL;
    }

    res->head = buf;
    res->tail = res->head;

    return res->head ? VTSS_RC_OK : VTSS_RC_ERROR;
}

static BOOL icfg_extend_buf(vtss_icfg_query_result_t *res)
{
    char *buf;

    if (!res || !res->tail || !res->tail->text) {
        T_D("Bug: NULL input");
        return FALSE;
    }

    if (!res->tail->free_text) {
        T_D("Cannot extend read-only buffer");
        return FALSE;
    }

    // Try to realloc the tail. If that fails, alloc a new block and append it.

    buf = (char *)VTSS_REALLOC(res->tail->text, res->tail->size + DEFAULT_TEXT_BLOCK_SIZE);
    if (buf) {
        T_D("Realloc: %d => %d bytes, 0x%8p => 0x%8p", res->tail->size, res->tail->size + DEFAULT_TEXT_BLOCK_SIZE, res->tail->text, buf);
        res->tail->text =  buf;
        res->tail->size += DEFAULT_TEXT_BLOCK_SIZE;
    } else {
        T_D("Appending buffer");
        vtss_icfg_query_result_buf_t *next = icfg_alloc_buf(0);
        if (next == NULL) {
            return FALSE;
        }
        res->tail->next = next;
        res->tail       = next;
    }

    return TRUE;
}

// Function for initialize vtss_icfg_conf_print_t struct
// In - conf_print - Pointer to the struct to initialize
void vtss_icfg_conf_print_init(vtss_icfg_conf_print_t *conf_print, const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    conf_print->is_default          = FALSE;
    conf_print->print_no_arguments  = TRUE;
    conf_print->show_default_values = FALSE;
    conf_print->force_no_keyword    = FALSE;
    conf_print->req                 = req;
    conf_print->result              = result;
}

// Function for print a configuration to ICFG.
// This function only prints default values if the all->default parameter is set. It also takes into account if the
// configuration is a global configuration or an interface configuration, and adds the extra space that is needed for interfaces configurations.
// One detail is that it is that if you give a "print" format, but don't give any parameter no printout is done. This can be used if a command has multiple parameter, where some of them can be the default values while others are not. See example in "dot1x global" command.
mesa_rc vtss_icfg_conf_print(vtss_icfg_conf_print_t *conf_print,
                             const char             *cmd_string,
                             const char             *format, ...)
{
    va_list args;
    char    cmd_param[512];
    BOOL    print_no_keyword = FALSE;

    if (conf_print->is_default && !conf_print->req->all_defaults) {
        // Nothing to print.
        return VTSS_RC_OK;
    }

    // Some commands require us to output a "no" in front of the command
    // when req->all_defaults is set, because they cannot be synthesized
    // otherwise (synce is an example).
    print_no_keyword = conf_print->req->all_defaults && conf_print->is_default && conf_print->force_no_keyword;

    if (print_no_keyword && !conf_print->print_no_arguments) {
        // We're going to print "no" in front of the command, but we should not
        // print any parameters.
        cmd_param[0] = '\0';
    } else {
        /*lint -e{530} ... 'args' is initialized by va_start() */
        va_start(args, format);
        (void)vsnprintf(cmd_param, sizeof(cmd_param) - 1, format, args);
        va_end(args);

        // Be sure to NULL-terminate.
        cmd_param[sizeof(cmd_param) - 1] = '\0';

        // If the resulting parameter string is 0 bytes long, it must be a
        // plain keyword, we're printing, and the only way to print defaults
        // is through the use of the no-keyword.
        if (cmd_param[0] == '\0' && conf_print->req->all_defaults && conf_print->is_default) {
            print_no_keyword = TRUE;
        }
    }

    VTSS_RC(vtss_icfg_printf(conf_print->result, "%s%s%s%s%s\n",
                             conf_print->req->cmd_mode == ICLI_CMD_MODE_GLOBAL_CONFIG ? "" : " ", // Extra space for non-global configuration
                             print_no_keyword ? "no " : "",
                             cmd_string,
                             cmd_param[0] == '\0' ? "" : " ",
                             cmd_param));

    return VTSS_RC_OK;
}

mesa_rc vtss_icfg_printf(vtss_icfg_query_result_t *res, const char *format, ...)
{
    va_list va;
    int n;
    int bytes_available;

    if (!msg_switch_is_primary()) {
        return VTSS_RC_ERROR;
    }

    /* The eCos implementation of vsnprintf() has a weakness: The return value
     * doesn't indicate whether the function ran out of destination buffer
     * space or not. It could return -1 if it did, or return the number of
     * characters that _would_ have been written to the destination (not
     * including the trailing '\0' byte).
     *
     * But it doesn't. If called with a size of N bytes, it will always write
     * up to N-1 bytes and return that value, no matter if the desired result
     * would have been longer than that.
     *
     * So the only way to determine if we're running out is this:
     *
     *   * Supply buffer with capacity N
     *   * Max. return value from vsnprintf is then N-1
     *   * So if the return value is N-2 or less, then we know that there was
     *     room for at least one more char in the buffer, i.e. we printed all
     *     there was to print.
     */

    while (TRUE) {
        bytes_available = res->tail->size - res->tail->used;

        if (bytes_available < 3) {   // 3 == One byte for output, one for '\0', one to work-around non-C99 eCos vsnprintf()
            if (!icfg_extend_buf(res)) {
                return VTSS_RC_ERROR;
            }
            bytes_available = res->tail->size - res->tail->used;
        }

        /*lint -e{530} ... 'va' is initialized by va_start() */
        va_start(va, format);
        n = vsnprintf(res->tail->text + res->tail->used, bytes_available, format, va);
        va_end(va);

        if (n >= 0  &&  n < (bytes_available - 1)) {    // -1 due to eCos workaround
            res->tail->used += n;
            return VTSS_RC_OK;
        } else if (n >= DEFAULT_TEXT_BLOCK_SIZE - 1 - 1) {   // -1 for '\0', -1 for eCos workaround
            /* Not enough contiguous mem in our buffer _and_ the string is too
             * long for our (fairly large) buffer: Give up with an error
             * message
             */
            T_E("vtss_icfg_printf cannot print string of length %u; too long", n);
            return VTSS_RC_ERROR;
        } else {
            /* Not enough contiguous mem in our buffer, but the string will fit
             * inside an empty one: Allocate more space and vsnprintf() again.
             *
             * Note that this strategy is, of course, subject to an attack where
             * the requested buffer capacity is always half the max buffer size
             * plus one.
             */
            if (!icfg_extend_buf(res)) {
                return VTSS_RC_ERROR;
            }
        }
    }  /* while (TRUE) */
}



//-----------------------------------------------------------------------------
// Feature list utilities
//-----------------------------------------------------------------------------

void vtss_icfg_feature_list_get(const u32 cnt, const char *list[])
{
    u32 i, n;

    // Insert unique entries into list. We do that by searching for existing
    // matching entries. That's expensive, of course, but it's a rare operation
    // so we live with it.

    ICFG_CRIT_ENTER();
    for (i = n = 0; i < VTSS_ICFG_LAST  &&  n < cnt - 1; i++) {
        if (icfg_callbacks[i].feature_name) {
            u32 k;
            for (k = 0; k < n  &&  strcmp(icfg_callbacks[i].feature_name, list[k]); k++)
                /* loop */ {
                ;
            }
            if (k == n) {
                list[n++] = icfg_callbacks[i].feature_name;
            }
        }
    }
    ICFG_CRIT_EXIT();
    if (i < VTSS_ICFG_LAST  &&  n == cnt - 1) {
        T_E("ICFG feature list full; truncating. i = %d, n = %d", i, n);
    }
    list[n] = NULL;
}

//-----------------------------------------------------------------------------
// Flash I/O utilities
//-----------------------------------------------------------------------------

/* Our user-visible FS model doesn't support subdirs, so we don't allow them in user-supplied filenames */
static BOOL is_valid_filename_format(const char *path)
{
    while (*path && *path != '/') {
        path++;
    }
    return *path == '\0';
}

int icfg_file_unlink(const char *path, bool is_flash_file)
{
    char filename[VTSS_ICONF_FILE_NAME_LEN_MAX];
    (void) snprintf(filename, sizeof(filename), "%s%s", is_flash_file ? VTSS_FS_FILE_DIR : USB_DEVICE_DIR, path);
    return unlink(filename);
}

int icfg_file_stat(const char *path, struct stat *buf, off_t *compressed_size, bool is_flash_file)
{
    int               res = -1;
    char              filename[VTSS_ICONF_FILE_NAME_LEN_MAX];

    memset(filename, 0x0, sizeof(filename));
    if (is_flash_file) {
        if (snprintf(filename, sizeof(filename), "%s%s",
                     VTSS_FS_FILE_DIR, path) < 1) {
            goto out;
        }
    } else {
        if (snprintf(filename, sizeof(filename), "%s/%s",
                     USB_DEVICE_DIR, path) < 1) {
            goto out;
        }
    }

    if (!is_valid_filename_format(path)) {
        T_D("Invalid filename format: %s", path);
        goto out;
    }

    if (stat(filename, buf) < 0) {
        if (errno == ENOSYS) {
            T_D("%s: <no status available>", path);
        } else {
            T_D("Cannot retrieve stat info for %s: %s", path, strerror(errno));
        }
        goto out;
    }

    if (compressed_size) {
        *compressed_size = buf->st_size;
    }

    // default-config is read-only, but the eCos FS doesn't correctly support that attribute:
    if (!strcmp(path, DEFAULT_CONFIG)) {
        buf->st_mode = S_IRUSR;
    }

    res = 0;
out:
    return res;
}

// Return FALSE == error, don't trust results
BOOL icfg_get_flash_file_count(u32 *ro_count, u32 *rw_count)
{
    DIR  *dirp;
    BOOL rc = FALSE;

    *ro_count = 0;
    *rw_count = 0;

    dirp = opendir(VTSS_FS_FILE_DIR);
    if (dirp == NULL) {
        T_D("Cannot list directory: %s", strerror(errno));
        return FALSE;
    }

    for (;;) {
        struct dirent *entry = readdir(dirp);
        struct stat   sbuf;

        if (entry == NULL) {
            break;
        }

        if (!strcmp(entry->d_name, ".")  ||  !strcmp(entry->d_name, "..")) {
            continue;
        }

        if (icfg_file_stat(entry->d_name, &sbuf, NULL) < 0) {
            goto out;
        }

        if ((sbuf.st_mode & S_IWUSR)) {
            (*rw_count)++;
        } else if ((sbuf.st_mode & S_IRUSR)) {
            (*ro_count)++;
        }
    }
    rc = TRUE;

out:
    if (closedir(dirp) < 0) {
        T_D("closedir: %s", strerror(errno));
        rc = FALSE;
    }

    return rc;
}

//-----------------------------------------------------------------------------
// VLAN 1 IP Address Save/Restore
//-----------------------------------------------------------------------------

static void icfg_vlan1_ip_save_zerodata(void)
{
#if defined(VTSS_SW_OPTION_IP)
    memset(&icfg_ip_save, 0, sizeof(icfg_ip_save));
#endif /* defined(VTSS_SW_OPTION_IP) */
}

#if defined(VTSS_SW_OPTION_IP)
static void icfg_vlan1_ip_v4_def_route(const vtss_appl_ip_route_key_t *rt)
{
    mesa_ipv4_t mask;
    mesa_rc     rc;

    T_D("Considering %s", rt);

    // Look for a default route
    if (rt->route.ipv4_uc.network.prefix_size != 0 || rt->route.ipv4_uc.network.address != 0) {
        return;
    }

    T_D("Default route found");

    // Default route destination and VLAN 1 IP address in the same network?
    mask = vtss_ipv4_prefix_to_mask(icfg_ip_save.v4.ip.network.prefix_size);
    if ((rt->route.ipv4_uc.destination & mask) != (icfg_ip_save.v4.ip.network.address & mask)) {
        return;
    }

    T_D("Default route in the same subnet");

    if ((rc = vtss_appl_ip_route_conf_get(rt, &icfg_ip_save.v4.route_conf)) != VTSS_RC_OK) {
        T_D("Cannot get the route conf: %s", error_txt(rc));
        return;
    }

    icfg_ip_save.v4.route    = *rt;
    icfg_ip_save.v4.route_ok = TRUE;
}
#endif /* defined(VTSS_SW_OPTION_IP) */

#if defined(VTSS_SW_OPTION_IP)
static void icfg_vlan1_ip_v6_def_route(const vtss_appl_ip_route_key_t *rt)
{
    mesa_ipv6_t mask;
    u8          maskb;
    size_t      i;
    mesa_rc     rc;

    T_D("Considering %s", rt);

    // Look for a default route
    if (rt->route.ipv6_uc.network.prefix_size == 0) {
        for (i = 0; i < sizeof(mesa_ipv6_t); i++) {
            if (rt->route.ipv6_uc.network.address.addr[i] != 0) {
                return;
            }
        }
    }

    T_D("Default route found");

    // Default route destination and VLAN 1 IP address in the same network?
    (void)vtss_conv_prefix_to_ipv6mask(icfg_ip_save.v6.ip.network.prefix_size, &mask);
    for (i = 0; i < sizeof(mask.addr); i++) {
        maskb = mask.addr[i];

        // Compare network (masked) only
        if ((rt->route.ipv6_uc.destination.addr[i] & maskb) != (icfg_ip_save.v6.ip.network.address.addr[i] & maskb)) {
            return;
        }
    }

    T_D("Default route in the same subnet");

    if ((rc = vtss_appl_ip_route_conf_get(rt, &icfg_ip_save.v6.route_conf)) != VTSS_RC_OK) {
        T_D("Cannot get the route conf: %s", error_txt(rc));
        return;
    }

    icfg_ip_save.v6.route    = *rt;
    icfg_ip_save.v6.route_ok = TRUE;
}
#endif /* defined(VTSS_SW_OPTION_IP) */

static void icfg_vlan1_ip_save(void)
{
#if defined(VTSS_SW_OPTION_IP)
    if (!msg_switch_is_primary()) {
        T_D("Cannot save VLAN 1 setup; not primary switch");
        return;
    }

    T_D("Attempting to save VLAN 1 IP configuration");

    // First get IP address setup

    vtss_ifindex_t ifidx;
    (void)vtss_ifindex_from_vlan(1, &ifidx);
    icfg_ip_save.v4.ip_ok = vtss_appl_ip_if_conf_ipv4_get(ifidx, &icfg_ip_save.v4.ip) == VTSS_RC_OK;
    icfg_ip_save.v6.ip_ok = vtss_appl_ip_if_conf_ipv6_get(ifidx, &icfg_ip_save.v6.ip) == VTSS_RC_OK;

    // Then try to find a default route that's up and statically configured. Do
    // this for both IPv4 and 6.
    {
        vtss_appl_ip_route_key_t rt, next_rt;
        bool                     first = true;

        while (vtss_appl_ip_route_conf_itr(first ? NULL : &rt, &next_rt) == VTSS_RC_OK) {
            first = false;
            rt = next_rt;

            if (rt.type == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC) {
                if (!icfg_ip_save.v4.route_ok) {
                    // Check if the default route in VLAN 1
                    icfg_vlan1_ip_v4_def_route(&rt);
                }
            } else if (rt.type == VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC) {
                if (!icfg_ip_save.v6.route_ok) {
                    // Check if the default route in VLAN 1
                    icfg_vlan1_ip_v6_def_route(&rt);
                }
            }
        }
    }

#endif /* defined(VTSS_SW_OPTION_IP) */
}

static void icfg_vlan1_ip_restore(void)
{
#if defined(VTSS_SW_OPTION_IP)
    vtss_ifindex_t ifidx;
    mesa_rc        rc;

    if (!msg_switch_is_primary()) {
        T_D("Cannot restore VLAN 1 setup; not primary switch");
        return;
    }

    (void) vtss_ifindex_from_vlan(1, &ifidx);

    ICFG_CRIT_ENTER();
    if (icfg_ip_save.v4.ip_ok  ||  icfg_ip_save.v6.ip_ok) {
        T_D("Restoring VLAN 1 IP settings");

        if (vtss_appl_ip_if_conf_set(ifidx) != VTSS_RC_OK) {
            T_E("Failed to re-create VLAN 1");
            goto out;
        }

        if (icfg_ip_save.v4.ip_ok) {
            if ((rc = vtss_appl_ip_if_conf_ipv4_set(ifidx, &icfg_ip_save.v4.ip)) == VTSS_RC_OK) {
                if (icfg_ip_save.v4.route_ok && (rc = vtss_appl_ip_route_conf_set(&icfg_ip_save.v4.route, &icfg_ip_save.v4.route_conf)) != VTSS_RC_OK) {
                    T_E("Failed to restore IPv4 default route: %s", error_txt(rc));
                }
            } else {
                T_E("Failed to restore VLAN 1 IPv4 configuration %s", error_txt(rc));
            }
        }

        if (icfg_ip_save.v6.ip_ok) {
            if ((rc = vtss_appl_ip_if_conf_ipv6_set(ifidx, &icfg_ip_save.v6.ip)) == VTSS_RC_OK) {
                if (icfg_ip_save.v6.route_ok && (rc = vtss_appl_ip_route_conf_set(&icfg_ip_save.v6.route, &icfg_ip_save.v6.route_conf)) != VTSS_RC_OK) {
                    T_D("Failed to restore IPv6 default route: %s", error_txt(rc));
                }
            } else {
                T_D("Failed to restore VLAN 1 IPv6 configuration: %s", error_txt(rc));
            }
        }
    }

out:
    // Always clear saved config so we don't apply it again inadvertently
    icfg_vlan1_ip_save_zerodata();
    ICFG_CRIT_EXIT();
#endif /* defined(VTSS_SW_OPTION_IP) */
}

//=============================================================================
// Configuration File Loading
//=============================================================================

//-----------------------------------------------------------------------------
// ICLI Output Buffer
//-----------------------------------------------------------------------------

// Signature due to ICLI requirements.
static BOOL icfg_commit_output_buffer_char_put(icli_addrword_t _id, char ch)
{
    ICFG_CRIT_ENTER();
    if (icfg_commit_echo_to_console) {
        (void) putc(ch, stdout);
    }
    if (icfg_commit_output_buffer_length < OUTPUT_BUFFER_SIZE) {
        icfg_commit_output_buffer[icfg_commit_output_buffer_length++] = ch;
    }
    // Silently discard output if buffer is full
    ICFG_CRIT_EXIT();
    return TRUE;
}

// Signature due to ICLI requirements.
static BOOL icfg_commit_output_buffer_str_put(icli_addrword_t _id, const char *str)
{
    ICFG_CRIT_ENTER();
    if (str) {
        if (icfg_commit_echo_to_console) {
            (void) fputs(str, stdout);
        }
        for (; *str && (icfg_commit_output_buffer_length < OUTPUT_BUFFER_SIZE); ++str, ++icfg_commit_output_buffer_length) {
            icfg_commit_output_buffer[icfg_commit_output_buffer_length] = *str;
        }
    }
    ICFG_CRIT_EXIT();
    return TRUE;
}

static void icfg_commit_output_buffer_clear(void)
{
    T_D("Entry");
    ICFG_CRIT_ENTER();
    icfg_commit_output_buffer_length = 0;
    ICFG_CRIT_EXIT();
}

const char *icfg_commit_output_buffer_get(u32 *length)

{
    const char *res;        // Need a temp to avoid lint thread warnings
    ICFG_CRIT_ENTER();
    res = icfg_commit_output_buffer;
    if (length) {
        *length = icfg_commit_output_buffer_length;
    }
    ICFG_CRIT_EXIT();
    return res;
}

void icfg_commit_output_buffer_append(const char *str)
{
    (void) icfg_commit_output_buffer_str_put(0, (char *) str);
}


//-----------------------------------------------------------------------------
// ICLI Line Commit
//-----------------------------------------------------------------------------

/* Commit one line to ICLI. */
static BOOL icfg_commit_one_line_to_icli(u32        session_id,
                                         const char *cmd,
                                         const char *source_name,
                                         BOOL       syntax_check_only,
                                         u32        line_cnt,
                                         u32        *err_cnt,
                                         u32        max_err_cnt)
{
    i32 rc;
    i32 exec_rc;

    T_N("%5d %s", line_cnt, cmd);
    rc = ICLI_CMD_EXEC_ERR_FILE_LINE_RC((char *)cmd, !syntax_check_only, (char *)source_name, line_cnt, TRUE, &exec_rc);
    if (rc == ICLI_RC_ERR_PARSING_END) {
        T_D("ICFG reached the end symbol in file %s, line %d", source_name, line_cnt);
        return FALSE;
    }
    if (rc != ICLI_RC_OK || exec_rc != VTSS_RC_OK) {
        ++(*err_cnt);
        T_D("ICLI cmd exec err: rc=%d, file %s, line %d", rc, source_name, line_cnt);
        T_D("                   cmd '%s'", cmd);
        if (rc == ICLI_RC_OK) {
            T_D("                   cmd result = %d", exec_rc);
        } else {
            T_D("                   cmd result = <not-invoked>");
        }

        if (syntax_check_only  &&  *err_cnt == max_err_cnt) {
            ICLI_PRINTF("%% Too many errors (%d), processing canceled.\n", max_err_cnt);
            return FALSE;
        }
    }
    T_N("Cmd exec done, rc = %d", rc);
    return TRUE;
}

//-----------------------------------------------------------------------------
// ICLI Buffer Commit
//-----------------------------------------------------------------------------

BOOL vtss_icfg_commit(u32                            session_id,
                      const char                     *source_name,
                      BOOL                           syntax_check_only,
                      BOOL                           use_output_buffer,
                      const vtss_icfg_query_result_t *res)
{
#define CMD_LINE_LEN (4*1024)
    const BOOL                         open_local_session = session_id == ICLI_SESSION_ID_NONE;
    icli_session_open_data_t           open_data;          // Session data if open_local_session
    const vtss_icfg_query_result_buf_t *buf = NULL;        // Current result buffer block
    const char                         *p;                 // Pointer into buffer; current char being processed
    u32                                len;                // Remaining bytes in current buffer
    char                               cmd[CMD_LINE_LEN];  // Command line being built for ICLI
    char                               *pcmd;              // Pointer into cmd[]
    u32                                err_cnt;            // Error count: How many times ICLI has complained.
    u32                                line_num = 0;       // Line number of current line
    u32                                line_cnt = 0;       // Total number of lines in the file
    i32                                rc;                 // Return code from ICLI calls
    i32                                cmd_mode_level = 0; // ICLI command mode level
    BOOL                               do_abort;           // Copy of icfg_commit_abort_flag
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
    struct timeval                     t0, t1;             // Measure time per chunk of lines submitted to ICLI
#endif
    icli_cmd_mode_t                    current_mode;
    u32                                i;

    if (!msg_switch_is_primary()) {
        T_D("Cannot commit %s; not primary switch", source_name);
        return FALSE;
    }

    T_D("Session %d, file %s, %s mode, ICLI output => %s", session_id, source_name,
        (syntax_check_only ? "syntax check" : "commit"),
        (use_output_buffer ? "buffer" : "console"));

    // err_cnt must start at 1 to ensure early exit via label 'done' leads to
    // return value FALSE
    err_cnt = 1;

    icfg_commit_output_buffer_clear();     // Always clear, even if we don't use it

    if (open_local_session) {
        /* Open new ICLI session and enter global config mode */

        memset(&open_data, 0, sizeof(open_data));

        open_data.name     = "ICFG";
        open_data.way      = ICLI_SESSION_WAY_APP_EXEC;
        open_data.char_put = use_output_buffer ? icfg_commit_output_buffer_char_put : NULL;
        open_data.str_put  = use_output_buffer ? icfg_commit_output_buffer_str_put  : NULL;

        if ((rc = icli_session_open(&open_data, &session_id)) != ICLI_RC_OK) {
            T_E("Cannot open ICFG session; configuration aborted. rc=%d", rc);
            return FALSE;
        }

        if ((rc = icli_session_privilege_set(session_id, (icli_privilege_t)((int)ICLI_PRIVILEGE_MAX - 1))) != ICLI_RC_OK) {
            ICLI_PRINTF("%% Failed to configure session; configuration aborted.\n");
            T_E("Cannot set ICFG session privilege; rc=%d", rc);
            goto done;
        }
    }

    if ((cmd_mode_level = icli_session_mode_enter(session_id, ICLI_CMD_MODE_GLOBAL_CONFIG)) < 0) {
        ICLI_PRINTF("%% Failed to enter configuration mode; configuration aborted.\n");
        T_E("Cannot enter ICLI config mode; rc=%d", cmd_mode_level);
        return FALSE;
    }

    if (syntax_check_only) {
        if (icli_session_cmd_parsing_begin(session_id) != ICLI_RC_OK) {
            T_E("Cannot enter ICLI syntax check mode");
            goto done;
        }
    }

    err_cnt = 0;

    ICFG_CRIT_ENTER();
    icfg_commit_state     = ICFG_COMMIT_RUNNING;
    icfg_commit_error_cnt = 0;
    ICFG_CRIT_EXIT();

    // Count number of lines in the buffer; used for progress reporting.
    // We recognize CR, LF, CR+LF, LF+CR as one end-of-line sequence.
    buf = res->head;
    while (buf != NULL  &&  buf->used > 0) {
        len = buf->used;
        p   = buf->text;
        while (len > 0) {
            /* If the len equal to 1, the (p + 1) is out of range for accecing. */
            if ( len > 1 && ((*p == '\n'  &&  *(p + 1) == '\r')  ||  (*p == '\r'  &&  *(p + 1) == '\n')) ) {
                p++;
                len--;
            }
            if (*p == '\n'  ||  *p == '\r') {
                line_cnt++;
            }
            p++;
            len--;
        }
        buf = buf->next;
    }
    T_D("About to process %d lines", line_cnt);

    // We need to feed ICLI one command line at a time.

    buf  = res->head;
    pcmd = cmd;
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
    (void) gettimeofday(&t0, NULL);
#endif
    while (buf != NULL  &&  buf->used > 0) {
        len = buf->used;
        p   = buf->text;
        while (len > 0) {
            /* If the len equal to 1, the (p + 1) is out of range for accessing. */
            if ( len > 1 && ((*p == '\n'  &&  *(p + 1) == '\r')  ||  (*p == '\r'  &&  *(p + 1) == '\n')) ) {
                p++;
                len--;
            }
            if (*p == '\n'  ||  *p == '\r') {
                line_num++;
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
                if ((line_num & 0xff) == 0) {
                    long delta;
                    (void) gettimeofday(&t1, NULL);
                    delta = (t1.tv_sec - t0.tv_sec) * (1000 * 1000) + (t1.tv_usec - t0.tv_usec);
                    t0 = t1;
                    T_D("Processed %d out of %d lines, %ld msec", line_num, line_cnt, delta / 1000);
                }
#endif
            } else {
                *pcmd++ = *p;
            }
            if (*p == '\n'  ||  *p == '\r'  ||  (pcmd == cmd + CMD_LINE_LEN - 1)) {
                *pcmd = '\0';
                pcmd = cmd;
                ICFG_CRIT_ENTER();
                do_abort = icfg_commit_abort_flag;
                ICFG_CRIT_EXIT();

                if (do_abort || !icfg_commit_one_line_to_icli(session_id, cmd, source_name, syntax_check_only, line_num, &err_cnt, MAX_ERROR_CNT)) {
                    goto done;
                }
            }
            p++;
            len--;
        }
        buf = buf->next;
    }

    // Handle case where last line isn't newline terminated
    if (pcmd != cmd) {
        *pcmd = '\0';
        line_num++;
        ICFG_CRIT_ENTER();
        do_abort = icfg_commit_abort_flag;
        ICFG_CRIT_EXIT();
        if (!do_abort) {
            (void) icfg_commit_one_line_to_icli(session_id, cmd, source_name, syntax_check_only, line_num, &err_cnt, MAX_ERROR_CNT);
        }
    }

done:
    if (err_cnt) {
        if (syntax_check_only) {
            ICLI_PRINTF("%% Syntax check done, %u problem%s found.\n", err_cnt, (err_cnt == 1 ? "" : "s"));
        } else {
            ICLI_PRINTF("%% %u problem%s found during configuration.\n", err_cnt, (err_cnt == 1 ? "" : "s"));
        }
    }

    if (syntax_check_only) {
        (void) icli_session_cmd_parsing_end(session_id);
    }

    if (open_local_session) {
        if ((rc = icli_session_close(session_id)) != ICLI_RC_OK) {
            T_E("Cannot close ICFG session; rc=%d\n", rc);
        }
    } else {
        for (i = 0; i < ICLI_MODE_MAX_LEVEL; ++i) {
            /* get current mode */
            if (ICLI_MODE_GET(&current_mode) != ICLI_RC_OK) {
                T_E("Cannot get current command mode\n");
                break;
            }

            /* check current mode */
            if (current_mode == ICLI_CMD_MODE_EXEC) {
                break;
            }

            /* exit current mode */
            (void)ICLI_MODE_EXIT();
        }

        if (i == ICLI_MODE_MAX_LEVEL) {
            T_E("Cannot go back to EXEC mode\n");
        }
    }

    ICFG_CRIT_ENTER();
    icfg_commit_error_cnt       = err_cnt;
    icfg_commit_state           = err_cnt == 0 ? ICFG_COMMIT_DONE : (syntax_check_only ? ICFG_COMMIT_SYNTAX_ERROR : ICFG_COMMIT_ERROR);
    icfg_commit_echo_to_console = FALSE;
    ICFG_CRIT_EXIT();

    return err_cnt == 0;
#undef CMD_LINE_LEN
}

void icfg_commit_status_get(icfg_commit_state_t *state, u32 *error_cnt)
{
    ICFG_CRIT_ENTER();
    if (state) {
        *state = icfg_commit_state;
    }
    if (error_cnt) {
        *error_cnt = icfg_commit_error_cnt;
    }
    ICFG_CRIT_EXIT();
}



//-----------------------------------------------------------------------------
// Configuration Thread
//-----------------------------------------------------------------------------

// Set filename and kick thread. Takes ownership of buffer.
const char *icfg_commit_trigger(const char *filename, vtss_icfg_query_result_t *buf)
{
    const char *msg = NULL;

    ICFG_CRIT_ENTER();

    if (!msg_switch_is_primary()) {
        msg = "Not primary switch; cannot trigger load.";
        T_D("%s: %s", filename, msg);
        goto out;
    }

    if (icfg_commit_filename[0]) {
        msg = "Cannot load; another operation is currently in progress.";
        T_D("%s: %s (Current: %s)", filename, msg, icfg_commit_filename);
        goto out;
    }

    misc_strncpyz(icfg_commit_filename, filename, sizeof(icfg_commit_filename));
    icfg_commit_buf = *buf;
    memset(buf, 0, sizeof(*buf));

    T_D("Triggering load of file %s", filename);
    vtss_flag_maskbits(&icfg_thread_flag, 0);
    vtss_flag_setbits(&icfg_thread_flag, ICFG_THREAD_FLAG_COMMIT_FILE);

out:
    vtss_icfg_free_query_result(buf);   // Free buf in case of error
    ICFG_CRIT_EXIT();
    return msg;
}

// Load file into buffer, then kick thread.
static const char *icfg_commit_load_and_trigger(const char *filename)
{
    const char               *msg;
    vtss_icfg_query_result_t buf = { NULL, NULL };

    if (!msg_switch_is_primary()) {
        msg = "Not primary switch; cannot trigger load.";
        T_D("%s: %s", filename, msg);
        return msg;
    }

    msg = icfg_file_read(filename, &buf);
    if (msg) {
        vtss_icfg_free_query_result(&buf);
        return msg;
    }

    // When committing a file due to INIT_CMD_CONF_DEF or INIT_CMD_ICFG_LOADING_PRE we want to
    // output problems to the console.
    ICFG_CRIT_ENTER();
    icfg_commit_echo_to_console = TRUE;
    ICFG_CRIT_EXIT();

    msg = icfg_commit_trigger(filename, &buf);  // Takes ownership of buf

    return msg;
}

void icfg_commit_complete_wait(void)
{
    T_D("Waiting for commit to complete");
    (void) vtss_flag_wait(&icfg_thread_flag, ICFG_THREAD_FLAG_COMMIT_DONE, VTSS_FLAG_WAITMODE_OR);
    T_D("Commit complete");
}

/* We make an assumption about always being called from the initfuns, and hence
 * only from one thread. If this assumption is broken in the future, we have a
 * race on icfg_commit_abort_flag.
 */
static void icfg_commit_abort(void)
{
    T_D("Aborting commit (if in progress)");

    ICFG_CRIT_ENTER();
    icfg_commit_abort_flag = TRUE;
    ICFG_CRIT_EXIT();

    icfg_commit_complete_wait();

    ICFG_CRIT_ENTER();
    icfg_commit_abort_flag = FALSE;
    ICFG_CRIT_EXIT();
}

static void icfg_thread(vtss_addrword_t data)
{
    char                     filename[sizeof(icfg_commit_filename)];
    vtss_icfg_query_result_t buf = { NULL, NULL };
    vtss_flag_value_t        flag;
    BOOL                     b_commit = FALSE;

    icfgThreadLock.wait();

    while (1) {
        T_D("Config Commit thread is running.");
        while (msg_switch_is_primary()) {
            flag = vtss_flag_wait(  &icfg_thread_flag,
                                    ICFG_THREAD_FLAG_COMMIT_FILE | ICFG_THREAD_FLAG_RELOAD_DEFAULT | ICFG_THREAD_FLAG_RELOAD_DEFAULT_KEEP_IP | ICFG_THREAD_FLAG_COPY_TO_RUNNING,
                                    VTSS_FLAG_WAITMODE_OR_CLR );
            T_D("Config Load thread is activated, flag = 0x%08x.", flag);

            if (flag & ICFG_THREAD_FLAG_COMMIT_FILE) {
                ICFG_CRIT_ENTER();
                memcpy(filename, icfg_commit_filename, sizeof(filename));
                buf = icfg_commit_buf;
                memset(&icfg_commit_buf, 0, sizeof(icfg_commit_buf));
                ICFG_CRIT_EXIT();

                if (!icfg_try_lock_io_mutex()) {
                    icfg_commit_output_buffer_clear();
                    (void) icfg_commit_output_buffer_str_put(0, "Cannot commit configuration; another operation is in progress. Please try again later.");
                    b_commit = FALSE;
                    goto skip;
                }

                // No syntax check before application; the buffer is all we've got so we
                // have to try:
                (void) vtss_icfg_commit(ICLI_SESSION_ID_NONE, filename, FALSE, TRUE, &buf);

                icfg_vlan1_ip_restore();

                icfg_unlock_io_mutex();
                T_D("Commit of %s done", filename);
                b_commit = TRUE;

skip:
                vtss_icfg_free_query_result(&buf);

                ICFG_CRIT_ENTER();
                icfg_commit_filename[0] = 0;      // Clear filename == done, ready for next
                ICFG_CRIT_EXIT();

                // Must always be set when not actively committing:
                vtss_flag_setbits(&icfg_thread_flag, ICFG_THREAD_FLAG_COMMIT_DONE);

                if (g_copy_to_running) {
                    vtss_flag_setbits(&icfg_thread_flag, ICFG_THREAD_FLAG_COPY_TO_RUNNING);
                }
            }
#define __SLEEP_TIME    200 //ms
            else if (flag & ICFG_THREAD_FLAG_RELOAD_DEFAULT) {
                // clear flag first because later reset will use the flag
                vtss_flag_setbits(&icfg_thread_flag, ICFG_THREAD_FLAG_COMMIT_DONE);

                /* sleep for replying SNMP */
                VTSS_OS_MSLEEP( __SLEEP_TIME );

                /* reload default */
                control_config_reset(VTSS_USID_ALL, 0);
            } else if (flag & ICFG_THREAD_FLAG_RELOAD_DEFAULT_KEEP_IP) {
                // clear flag first because later reset will use the flag
                vtss_flag_setbits(&icfg_thread_flag, ICFG_THREAD_FLAG_COMMIT_DONE);

                /* sleep for replying SNMP */
                VTSS_OS_MSLEEP( __SLEEP_TIME );

                /* reload default keep-ip */
                control_config_reset(VTSS_USID_ALL, INIT_CMD_PARM2_FLAGS_IP);
            } else if (flag & ICFG_THREAD_FLAG_COPY_TO_RUNNING) {
                // clear flag first because later reset will use the flag
                vtss_flag_setbits(&icfg_thread_flag, ICFG_THREAD_FLAG_COMMIT_DONE);

                /* sleep for replying SNMP */
                VTSS_OS_MSLEEP( __SLEEP_TIME );

                if (b_commit == FALSE) {
                    ICFG_CRIT_ENTER();
                    g_copy_config.copyStatus = VTSS_APPL_ICFG_COPY_STATUS_FAILED_SAVE_DST;
                    ICFG_CRIT_EXIT();

                    goto _ICFG_THREAD_CONTINUE;
                }

                if (icfg_try_lock_io_mutex() == FALSE) {
                    ICFG_CRIT_ENTER();
                    g_copy_config.copyStatus = VTSS_APPL_ICFG_COPY_STATUS_FAILED_OTHER_IN_PROCESSING;
                    ICFG_CRIT_EXIT();

                    goto _ICFG_THREAD_CONTINUE;
                }

                /* commit new config */
                if (vtss_icfg_commit(ICLI_SESSION_ID_NONE, g_copy_source_path, FALSE, FALSE, &g_copy_query_buf) == FALSE) {
                    ICFG_CRIT_ENTER();
                    g_copy_config.copyStatus = VTSS_APPL_ICFG_COPY_STATUS_FAILED_SAVE_DST;
                    ICFG_CRIT_EXIT();

                    goto _ICFG_THREAD_CONTINUE;
                }

                ICFG_CRIT_ENTER();
                g_copy_config.copyStatus = VTSS_APPL_ICFG_COPY_STATUS_SUCCESS;
                ICFG_CRIT_EXIT();

_ICFG_THREAD_CONTINUE:
                g_copy_to_running = FALSE;
                vtss_icfg_free_query_result(&g_copy_query_buf);
                icfg_unlock_io_mutex();
                continue;
            }
#undef __SLEEP_TIME
        } // while
        T_D("Config Commit thread is suspending.");
        icfgThreadLock.wait();
    }
}

//-----------------------------------------------------------------------------
// Read/Write Configuration File
//-----------------------------------------------------------------------------

// Read file. Allocate buffer of sufficient size. Caller must release buffer,
// even if an error occurs.
// \return NULL if load was OK, pointer to constant string error message otherwise


const char *icfg_file_read(const char *filename, vtss_icfg_query_result_t *res, bool is_flash_file)
{
    struct stat   sbuf;
    ssize_t       bytes_read;
    int           fd              = -1;
    off_t         compressed_size = 0;
    void          *compressed_buf = NULL;
    const char    *msg            = NULL;
    char          fs_filename[VTSS_ICONF_FILE_NAME_LEN_MAX];

    memset(res, 0, sizeof(*res));

    if (!msg_switch_is_primary()) {
        msg = "Not primary switch.";
        T_D("%s: %s", filename, msg);
        goto out;
    }

    if (!is_valid_filename_format(filename)) {
        msg = "Invalid file name.";
        T_D("Invalid filename format: %s", filename);
        goto out;
    }

    memset(fs_filename, 0x0, sizeof(fs_filename));
    if (is_flash_file == true) {
        if (snprintf(fs_filename, sizeof(fs_filename), "%s%s",
                     VTSS_FS_FILE_DIR, filename) < 1) {
            goto out;
        }
    } else {
        if (snprintf(fs_filename, sizeof(fs_filename), "%s/%s",
                     USB_DEVICE_DIR, filename) < 1) {
            goto out;
        }
    }

    if (icfg_file_stat(filename, &sbuf, &compressed_size, is_flash_file) < 0) {
        msg = "Cannot read file status.";
        T_D("%s: %s %s", filename, msg, strerror(errno));
        goto out;
    }

    if (vtss_icfg_init_query_result(sbuf.st_size & 0xffffffffL, res) != VTSS_RC_OK) {
        msg = "Out of memory.";
        T_D("%s: %s (needed %lld bytes)", filename, msg, sbuf.st_size);
        goto out;
    }

    if ((fd = open(fs_filename, O_RDONLY)) < 0) {
        msg = "Cannot open file.";
        T_D("%s: %s %s", filename, msg, strerror(errno));
        goto out;
    }

    T_D("Uncompressed size: %llu  Compressed size: %llu", sbuf.st_size, compressed_size);

    if (sbuf.st_size != compressed_size) {  // Compressed file
    } else { // Uncompressed file
        bytes_read = read(fd, res->head->text, res->head->size);

        if (bytes_read < 0) {
            msg = "Cannot read file.";
            T_D("%s: %s %s", filename, msg, strerror(errno));
            goto out;
        }

        if (bytes_read < (ssize_t) res->head->size) {
            msg = "Cannot read entire file.";
            T_D("%s: %s Got " VPRIsz" bytes out of %u; %s", filename, msg, bytes_read, res->head->size, strerror(errno));
            goto out;
        }

        res->head->used = bytes_read;
    }

out:
    if (compressed_buf) {
        VTSS_FREE(compressed_buf);
    }
    if (fd >= 0) {
        (void) close(fd);
    }

    return msg;
}

mesa_rc icfg_flash_free_get(const struct statvfs &fs_buf, u32 &flash_free, u32 &total_flash_free)
{
    /* Due to BZ#20869, the firmware update will now download the firmware
     * image to intermediate storage on the NAND flash. This will almost
     * completely avoid of dynamic memory during update.
     *
     * Here is a protected mechanism to avoid no space for the firmware
     * update process.
     * The maximum firmware image is set to 20MB by default.
     * And one more reserved size(4MB) for other or further processes.
     */
#define ICFG_NAND_FLASH_RESERVED_SIZE    ((20 + 4) * 1024 * 1024) /* 24MB */

    flash_free = total_flash_free = fs_buf.f_bsize * fs_buf.f_bfree;
#if defined(VTSS_SW_OPTION_FIRMWARE)
    if (!firmware_is_nor_only()) {
        flash_free = flash_free <= ICFG_NAND_FLASH_RESERVED_SIZE ? 0 : (flash_free - ICFG_NAND_FLASH_RESERVED_SIZE);
    }
#endif

    return VTSS_RC_OK;
}

// \return NULL if write was OK, pointer to constant string error message otherwise
const char *icfg_file_write(const char *filename, vtss_icfg_query_result_t *res, bool is_flash_file)
{
    u32                          total = 0;
    int                          fd    = -1;
    vtss_icfg_query_result_buf_t *buf;
    struct stat                  stat_buf;
    void                         *compressed_buf = NULL;
    const char                   *msg   = NULL, *str;
    char                         fs_filename[VTSS_ICONF_FILE_NAME_LEN_MAX];
    BOOL                         file_existing = FALSE, unlink_flag = FALSE;
    struct statvfs               fs_buf;
    u32                          flash_free = 0, total_flash_free = 0;

    if ((str = strrchr(filename, '/')) != NULL) {
        filename = (str + 1);
    }

    if (!msg_switch_is_primary()) {
        msg = "Not primary switch.";
        T_D("%s: %s\n", filename, msg);
        return msg;
    }

    if (!is_valid_filename_format(filename)) {
        msg = "Invalid file name.";
        T_D("Invalid filename format: %s", filename);
        return msg;
    }

    // CRASHFILE is special, we don't allow writing to it here
    if (!strcmp(filename, CRASHFILE)) {
        msg = "Invalid file name.";
        T_D("Invalid filename: %s", filename);
        return msg;
    }

    memset(fs_filename, 0x0, sizeof(fs_filename));
    if (is_flash_file == true) {
        if (snprintf(fs_filename, sizeof(fs_filename), "%s%s",
                     VTSS_FS_FILE_DIR, filename) < 1) {
            msg = "Failed to convert file name.";
            T_D("Failed to convert filename: %s", filename);
            return msg;
        }
    } else {
        if (snprintf(fs_filename, sizeof(fs_filename), "%s/%s",
                     USB_DEVICE_DIR, filename) < 1) {
            msg = "Failed to convert file name.";
            T_D("Failed to convert filename: %s", filename);
            return msg;
        }
        if (!usb_is_device_present()) {
            msg = "No USB device present";
            T_D("Attempt to copy to usb, but device not present");
            return msg;
        }
    }

    // Test if file exists. Yes => we'll be overwriting and don't need to check
    // if there are available R/W slots in the FS.
    // Also check if the file is R/O; yes => deny the request.

    if (icfg_file_stat(filename, &stat_buf, NULL, is_flash_file) == 0) {
        file_existing = TRUE;
        if ((stat_buf.st_mode & S_IWUSR) == 0) {
            msg = "File is read-only.";
            T_D("%s: %s", filename, msg);
            return msg;
        }
    } else {
        u32 ro_count, rw_count;
        if (!icfg_get_flash_file_count(&ro_count, &rw_count)) {
            msg = "Cannot access flash file system.";
            T_E("%s: %s", filename, msg);
            return msg;
        }
        // Don't include CRASHFILE in the RW count; it's special
        if (icfg_file_stat(CRASHFILE, &stat_buf, NULL) == 0) {
            rw_count--;
        }
        if (rw_count >= ICFG_MAX_WRITABLE_FILES_IN_FLASH_CNT) {
            msg = "File system table is full.";
            T_D("%s: %s", filename, msg);
            return msg;
        }
    }

    buf = res->head;
    while (buf != NULL  &&  buf->used > 0) {
        total += buf->used;
        buf = buf->next;
    }

    {
        /* Check if enough space for saving the new file */
        statvfs(VTSS_FS_FILE_DIR, &fs_buf);
        icfg_flash_free_get(fs_buf, flash_free, total_flash_free);

        T_D("File name: %s, File size: %u bytes, Flash free: %u bytes\n", filename, total, flash_free);
        if ((file_existing && total > stat_buf.st_size && ((total - stat_buf.st_size) >= flash_free)) || /* existing file */
            (!file_existing && total >= flash_free) /* new file */) {
            msg = "Exceed the maximum stored space for saving the configured file.";
            T_D("%s: %s", filename, msg);
            return msg;
        }
    }

    T_D("Saving %u bytes to flash:%s", total, filename);

    // From here on we do destructive changes to the RAM FS and have resources to free
    // upon error -- "goto considered useful".

    if ((fd = open(fs_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        msg = "Cannot create file.";
        T_D("%s: %s %s", filename, msg, strerror(errno));
        goto err_out;
    }

    buf = res->head;
    while (buf != NULL  &&  buf->used > 0) {
        if (write(fd, (void *)buf->text, buf->used) < (ssize_t)buf->used) {
            unlink_flag = TRUE;
            msg = "Save failed during write; file may be corrupt.";
            T_D("%s: %s %s", filename, msg, strerror(errno));
            goto err_out;
        }

        buf = buf->next;
    }

    if (close(fd) < 0) {
        unlink_flag = TRUE;
        msg = "Cannot close.";
        T_D("%s: %s %s", filename, msg, strerror(errno));
    }
    fd = -1;

#if defined(CYGPKG_FS_RAM)
    if (os_file_fs2flash() != VTSS_RC_OK) {
        unlink_flag = TRUE;
        msg = "Cannot commit changes to flash.";
    }
#endif  /* CYGPKG_FS_RAM */

err_out:
    if (compressed_buf) {
        VTSS_FREE(compressed_buf);
    }

    if (fd >= 0) {
        (void) close(fd);
    }

    if (msg) {
        T_D("%s: %s %s", filename, msg, strerror(errno));
        T_D("Removing file from RAM and reloading FS from flash");
        if (unlink_flag && unlink(fs_filename) < 0) {
            T_D("Failed to write file but cannot unlink incorrectly-written file %s. Reason: %s", filename, strerror(errno));
        }
#if defined(CYGPKG_FS_RAM)
        os_file_flash2fs();
#endif  /* CYGPKG_FS_RAM */
    }

    return msg;
}



// \return NULL if delete succeeded, pointer to constant string error message otherwise
const char *icfg_file_delete(const char *filename, bool is_flash_file)
{
    struct stat buf;

    if (!msg_switch_is_primary()) {
        return "Not primary switch.";
    }

    if (!is_valid_filename_format(filename)) {
        return "Invalid file name.";
    }

    if (icfg_file_stat(filename, &buf, NULL) < 0) {
        return "Cannot access file.";
    }

    if (buf.st_mode & S_IWUSR) {
        if (icfg_file_unlink(filename, is_flash_file) < 0) {
            return "File system error.";
        }
#if defined(CYGPKG_FS_RAM)
        if (os_file_fs2flash() != VTSS_RC_OK) {
            return "Cannot commit changes to flash.";
        }
#endif  /* CYGPKG_FS_RAM */
        return NULL;
    }

    return "File is read-only.";
}


// Helper function
static void ifcg_record_saved_startup(void)
{
    int fno;
    char fs_filename[VTSS_ICONF_FILE_NAME_LEN_MAX];
    (void) snprintf(fs_filename, sizeof(fs_filename), "%s/%s", VTSS_FS_FILE_DIR, STARTUP_CONFIG_CREATED);
    if (access(fs_filename, F_OK) != 0 && (fno = creat(fs_filename, 0644)) >= 0) {
        close(fno);
    }
}

//-----------------------------------------------------------------------------
// "copy running-config startup-config"
//-----------------------------------------------------------------------------

// \return NULL if save was OK, pointer to constant string error message otherwise
const char *vtss_icfg_running_config_save(void)
{
    vtss_icfg_query_result_t res  = { NULL, NULL };
    const char               *msg = NULL;

    if (!icfg_try_lock_io_mutex()) {
        msg = "Cannot save " STARTUP_CONFIG "; another I/O operation is in progress.";
        T_D("%s", msg);
        return msg;
    }

    T_D("Building configuration...");

    if (vtss_icfg_query_all(FALSE, &res) != VTSS_RC_OK) {
        msg = "Failed to generate configuration for " STARTUP_CONFIG ".";
        goto out;
    }

    msg = icfg_file_write(STARTUP_CONFIG, &res);
    if (!msg) {
        // Avoid recreating if missing at boot
        ifcg_record_saved_startup();
    }

#if defined(VTSS_SW_OPTION_HTTPS)
    if (!msg) {
        https_mgmt_cert_save();
    }
#endif
#if defined(VTSS_SW_OPTION_POE)
    if (!msg && poe_init_done == TRUE) {
        poe_save_command();
    }
#endif
out:
    vtss_icfg_free_query_result(&res);
    icfg_unlock_io_mutex();

    if (msg) {
        T_W("%s", msg);
    }

    return msg;
}



//-----------------------------------------------------------------------------
// I/O Mutex
//-----------------------------------------------------------------------------

BOOL icfg_try_lock_io_mutex(void)
{
    BOOL res;
    ICFG_CRIT_ENTER();
    if (icfg_io_in_progress) {
        T_D("I/O in progress; lock denied");
        res = FALSE;
    } else {
        T_D("No I/O in progress; locking");
        icfg_io_in_progress = TRUE;
        res                 = TRUE;
    }
    ICFG_CRIT_EXIT();
    return res;
}

void icfg_unlock_io_mutex(void)
{
    ICFG_CRIT_ENTER();
    if (icfg_io_in_progress) {
        T_D("Unlocking I/O");
        icfg_io_in_progress = FALSE;
    }
    ICFG_CRIT_EXIT();
}



//-----------------------------------------------------------------------------
// Old-style 'conf' cleanup
//-----------------------------------------------------------------------------

static void icfg_unused_conf_purge(void)
{
    conf_blk_id_t i;

    // Mainly global blocks; most locals stay in place

    i = CONF_BLK_IP_CONF;
    T_D("Purging unused local conf block id %d", i);
    (void) conf_sec_create(CONF_SEC_LOCAL, i, 0);

    for (i = CONF_BLK_CONF_HDR; i < CONF_BLK_COUNT; i++) {
        if ((i != CONF_BLK_TOPO)              && (i != CONF_BLK_OS_FILE_CONF)    &&
            (i != CONF_BLK_MSG)               && (i != CONF_BLK_SSH_CONF)        &&
            (i != CONF_BLK_HTTPS_CONF)        && (i != CONF_BLK_PHY_CONF)        &&
            (i != CONF_BLK_POST_CONF)         && (i != CONF_BLK_BOARD_PORT_CFG)  &&
            (i != CONF_BLK_SNMPV3_USERS_CONF) && (i != CONF_BLK_TRACE)           &&
            (i != CONF_BLK_QUEUE_CONF)
           ) {
            T_D("Purging unused global conf block id %d", i);
            (void)conf_sec_create(CONF_SEC_GLOBAL, i, 0);
        }
    }
}

//-----------------------------------------------------------------------------
// ICFG module init
//-----------------------------------------------------------------------------

// Read linker section containing contents of 'default-config' file and create
// it in the file system
static void icfg_default_config_write(void)
{
    // check if "/switch/icfg/default-config" exists, if yes, delete it!
    char filename[VTSS_ICONF_FILE_NAME_LEN_MAX];
    char default_conf_fs[VTSS_ICONF_FILE_NAME_LEN_MAX];
    int fd;

    if (snprintf(filename, sizeof(filename), "%s%s", VTSS_FS_FILE_DIR, DEFAULT_CONFIG) < sizeof(filename)) {
        if ((fd = open(filename, O_RDONLY)) >= 0) {
            close(fd);
            if (remove(filename)) {
                T_W("Remove %s failed! [%s]", filename, strerror(errno));
                return;
            }
        }

        // symlink to "/etc/mscc/icfg/default-config"
        if (snprintf(default_conf_fs, sizeof(default_conf_fs), "%s%s", "/etc/mscc/icfg/", DEFAULT_CONFIG) < sizeof(default_conf_fs)) {
            if (symlink(default_conf_fs, filename)) {
                T_W("Symlink %s failed! [%s]", default_conf_fs, strerror(errno));
            }
        }
    }
}

extern "C" int icfg_icli_cmd_register();

mesa_rc vtss_icfg_early_init(vtss_init_data_t *data)
{
    switch (data->cmd) {
    case INIT_CMD_INIT:
        critd_init(&icfg_crit, "icfg", VTSS_MODULE_ID_ICFG, CRITD_TYPE_MUTEX);
        ICFG_CRIT_ENTER();

        /* On Linux, we have a real FS, and assume its in place */
        icfg_default_config_write();

        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           icfg_thread,
                           0,
                           "ICFG Loader",
                           nullptr,
                           0,
                           &icfg_thread_handle,
                           &icfg_thread_block);

        vtss_flag_init(&icfg_thread_flag);
        vtss_flag_setbits(&icfg_thread_flag, ICFG_THREAD_FLAG_COMMIT_DONE);

        memset(&g_copy_config, 0, sizeof(vtss_appl_icfg_copy_config_t));

        ICFG_CRIT_EXIT();
        break;

    case INIT_CMD_START:
        ICFG_CRIT_ENTER();
        // Clean out unused blocks. This is mainly in case someone has booted images in this version sequence:
        //   >= 3.40 -- generates file system block, purges unused blocks
        //   2.80    -- ignores file system block, generates conf blocks for almost all modules
        //   >= 3.60 -- flash now contains both file system + 2.80-era blocks. The latter must be purged
        icfg_unused_conf_purge();
        ICFG_CRIT_EXIT();
        break;

    case INIT_CMD_CONF_DEF:
        if (data->isid == VTSS_ISID_GLOBAL) {
            icfg_commit_abort();
            ICFG_CRIT_ENTER();
            icfg_vlan1_ip_save_zerodata();
            if (data->flags & INIT_CMD_PARM2_FLAGS_IP) {
                icfg_vlan1_ip_save();
            }
            ICFG_CRIT_EXIT();
        }
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize private mib */
VTSS_PRE_DECLS void icfg_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_icfg_json_init(void);
#endif

static BOOL vtss_icfg_must_create_startup_config(void)
{
    BOOL ret = FALSE;    // By default, don't create
    char fs_filename[VTSS_ICONF_FILE_NAME_LEN_MAX];
    (void) snprintf(fs_filename, sizeof(fs_filename), "%s%s", VTSS_FS_FILE_DIR, STARTUP_CONFIG);
    ret = (access(fs_filename, F_OK) != 0);    // Missing STARTUP_CONFIG?
    if (ret) {                                 // If so, check STARTUP_CONFIG_CREATED
        T_D("%s missing", fs_filename);
        (void) snprintf(fs_filename, sizeof(fs_filename), "%s/%s", VTSS_FS_FILE_DIR, STARTUP_CONFIG_CREATED);
        // If STARTUP_CONFIG_CREATED do not exist, create STARTUP_CONFIG (and vice versa)
        ret = (access(fs_filename, F_OK) != 0);
        T_D("%s %s", STARTUP_CONFIG_CREATED, (ret ? "also missing" : "present"));
    }
    T_D("Do %s create %s", (ret ? "go ahead and" : "NOT"), STARTUP_CONFIG);
    return ret;
}

mesa_rc vtss_icfg_late_init(vtss_init_data_t *data)
{
    BOOL save_startup_config;

    T_D("Entry: Late Init: %s, isid %d", control_init_cmd2str(data->cmd), data->isid);
    switch (data->cmd) {
    case INIT_CMD_INIT:
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register private mib */
        icfg_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_icfg_json_init();
#endif
        icfg_icli_cmd_register();
        break;

    case INIT_CMD_CONF_DEF:
        if (data->isid == VTSS_ISID_LOCAL  &&  !(data->flags & INIT_CMD_PARM2_FLAGS_NO_DEFAULT_CONFIG)) {  // Last ISID and loading of default-config hasn't been disabled
            (void)icfg_commit_load_and_trigger(DEFAULT_CONFIG);
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        save_startup_config = vtss_icfg_must_create_startup_config();

        T_D("ICFG Loading Pre: Resuming thread");
        vtss_flag_maskbits(&icfg_thread_flag, 0);
        vtss_flag_setbits(&icfg_thread_flag, ICFG_THREAD_FLAG_COMMIT_DONE);
        icfgThreadLock.lock(false);

        T_D("Attempting to load " STARTUP_CONFIG ", fallback to " DEFAULT_CONFIG);
        if (icfg_commit_load_and_trigger(STARTUP_CONFIG) != NULL  &&
            icfg_commit_load_and_trigger(DEFAULT_CONFIG) != NULL) {
            T_E("Load failed for both " STARTUP_CONFIG " and " DEFAULT_CONFIG);
        } else {
            icfg_commit_complete_wait();
        }

        if (save_startup_config) {
            T_D("Must save running config");
            (void)vtss_icfg_running_config_save();
        }

        break;

    default:
        break;
    }

    T_D("Exit:  Late Init: %s", control_init_cmd2str(data->cmd));
    return VTSS_RC_OK;
}

/*
==============================================================================

    Public APIs in vtss_appl\include\vtss\appl\icfg.h

==============================================================================
*/
#include <sys/types.h>

typedef struct {
    char        fileName[VTSS_APPL_ICFG_FILE_STR_LEN + 1];
} vtss_appl_icfg_file_name_t;

static mesa_rc _file_statistics_get(
    vtss_appl_icfg_file_statistics_t    *statistics
)
{
    DIR             *dirp;
    u32             file_count;
    u32             byte_count;
    struct dirent   *entry;
    struct stat     sbuf;

    if ( ! msg_switch_is_primary() ) {
        return VTSS_RC_ERROR;
    }

    if ( ! icfg_try_lock_io_mutex() ) {
        return VTSS_RC_ERROR;
    }

    dirp = opendir(VTSS_FS_FILE_DIR);
    if ( dirp == NULL ) {
        icfg_unlock_io_mutex();
        return VTSS_RC_ERROR;
    }

    file_count = 0;
    byte_count = 0;

    for (;;) {
        entry = readdir( dirp );
        if ( entry == NULL ) {
            break;
        }

        if ( !strcmp(entry->d_name, ".")  ||  !strcmp(entry->d_name, "..") ) {
            continue;
        }

        if (icfg_file_stat(entry->d_name, &sbuf, NULL) == 0) {
            file_count += 1;
            byte_count += sbuf.st_size;
        }
    }

    (void)closedir(dirp);

    icfg_unlock_io_mutex();

    //
    // Flash size
    //
    struct statvfs fs_buf;
    statvfs(VTSS_FS_FILE_DIR, &fs_buf);

    statistics->flashSize = fs_buf.f_bsize * fs_buf.f_blocks;
    statistics->flashFree = fs_buf.f_bsize * fs_buf.f_bfree;

    statistics->numberOfFiles = file_count;
    statistics->totalBytes    = byte_count;

    return VTSS_RC_OK;
}

static void _file_entry_encap(
    vtss_appl_icfg_file_name_t      *fileName,
    struct stat                     *st,
    vtss_appl_icfg_file_entry_t     *fileEntry
)
{
    struct tm       timeinfo;

    memset( fileEntry, 0, sizeof(vtss_appl_icfg_file_entry_t) );

    strcpy( fileEntry->fileName, fileName->fileName);

    fileEntry->bytes = st->st_size;

    (void)localtime_r(&(st->st_mtime), &timeinfo);
    (void)strftime(fileEntry->modifiedTime, VTSS_APPL_ICFG_TIME_STR_LEN, "%Y-%m-%d %H:%M:%S", &timeinfo);

    /* owner */
    fileEntry->attribute[0] = (st->st_mode & S_IRUSR) ? 'r' : '-';
    fileEntry->attribute[1] = (st->st_mode & S_IWUSR) ? 'w' : '-';
}

static mesa_rc _file_entry_get(
    vtss_appl_icfg_file_name_t      *fileName,
    vtss_appl_icfg_file_entry_t     *fileEntry
)
{
    struct stat     sbuf;

    if ( fileName == NULL ) {
        return VTSS_RC_ERROR;
    }

    if ( ! msg_switch_is_primary() ) {
        return VTSS_RC_ERROR;
    }

    if ( ! icfg_try_lock_io_mutex() ) {
        return VTSS_RC_ERROR;
    }

    if ( icfg_file_stat(fileName->fileName, &sbuf, NULL) ) {
        icfg_unlock_io_mutex();
        return VTSS_RC_ERROR;
    }

    icfg_unlock_io_mutex();

    if ( fileEntry ) {
        _file_entry_encap( fileName, &sbuf, fileEntry );
    }

    return VTSS_RC_OK;
}

/*
    if strlen(file name) == 0 then get first otherwise get next
*/
static mesa_rc _file_entry_get_next(
    vtss_appl_icfg_file_name_t      *fileName,
    vtss_appl_icfg_file_entry_t     *fileEntry
)
{
    DIR             *dirp;
    struct dirent   *entry;
    struct stat     sbuf;
    char            next_name[PATH_MAX];

    if ( fileName == NULL ) {
        return VTSS_RC_ERROR;
    }

    if ( ! msg_switch_is_primary() ) {
        return VTSS_RC_ERROR;
    }

    if ( ! icfg_try_lock_io_mutex() ) {
        return VTSS_RC_ERROR;
    }

    dirp = opendir(VTSS_FS_FILE_DIR);
    if ( dirp == NULL ) {
        icfg_unlock_io_mutex();
        return VTSS_RC_ERROR;
    }

    next_name[0] = 0;
    /* get next file name */
    for (;;) {
        entry = readdir( dirp );
        if ( entry == NULL ) {
            break;
        }

        if ( !strcmp(entry->d_name, ".")  ||  !strcmp(entry->d_name, "..") ) {
            continue;
        }

        if (strcmp(entry->d_name, fileName->fileName) <= 0) {
            continue;
        }
        if (next_name[0] && strcmp(entry->d_name, next_name) >= 0) {
            continue;
        }
        strncpy(next_name, entry->d_name, PATH_MAX - 1);
        next_name[PATH_MAX - 1] = 0;
    }

    (void)closedir( dirp );

    if ( next_name[0] == 0 ) {
        icfg_unlock_io_mutex();
        return VTSS_RC_ERROR;
    }

    if ( icfg_file_stat(next_name, &sbuf, NULL) ) {
        icfg_unlock_io_mutex();
        return VTSS_RC_ERROR;
    }

    icfg_unlock_io_mutex();

    memset(fileName, 0, sizeof(vtss_appl_icfg_file_name_t));
    strncpy(fileName->fileName, next_name, VTSS_APPL_ICFG_FILE_STR_LEN);

    if ( fileEntry ) {
        _file_entry_encap( fileName, &sbuf, fileEntry );
    }

    return VTSS_RC_OK;
}

/* Test if filename refers to specific file.
 *
 * The code skips any path off before checking; this is to avoid things like
 * /./././default-config going undetected. This only works because we don't
 * really support subdirs in our user-exposed file system.
 */
static BOOL _is_specific_filename(const char *p, const char *expected)
{
    const char *res = p;
    if (!p) {
        return FALSE;
    }
    for (; *p; p++) {
        if (*p == '/') {
            res = p + 1;
        }
    }
    return !strcmp(res, expected);
}


/* We need to check for write operations against "default-config" because eCos
 * doesn't support file permissions; we can't set a file read-only and be done
 * with it.
 */
static BOOL _is_default_config(const char *p)
{
    return _is_specific_filename(p, "default-config");
}

static void _copy_config_update(
    const vtss_appl_icfg_copy_config_t  *const  config,
    vtss_appl_icfg_copy_status_t                status
)
{
    ICFG_CRIT_ENTER();
    memcpy( &g_copy_config, config, sizeof(g_copy_config) );
    g_copy_config.copy        = FALSE;
    g_copy_config.copyStatus = status;
    ICFG_CRIT_EXIT();
}

static BOOL _decompose_url(
    const char       *path,
    misc_url_parts_t *url
)
{
    misc_url_parts_init(url, MISC_URL_PROTOCOL_TFTP | MISC_URL_PROTOCOL_FLASH);

    if ( misc_url_decompose(path, url) == FALSE ) {
        return FALSE;
    }

    return TRUE;
}

static BOOL _save_to_flash(
    vtss_icfg_query_result_t    *res,
    const char                  *filename
)
{
    const char  *msg;

    msg = icfg_file_write(filename, res);
    if ( msg ) {
        return FALSE;
    }

    return TRUE;
}

static BOOL _save_to_tftp(
    vtss_icfg_query_result_t    *res,
    const misc_url_parts_t      *url_parts
)
{
    vtss_icfg_query_result_buf_t *buf     = res->head;
    u32                          total    = 0;
    char                         *tmp_buf = NULL;
    int                          status;
    int                          tftp_err;

    // Our TFTP put function expects a contiguous buffer, so we may have to
    // create one and copy all the blocks into it. We do try to avoid it,
    // though.

    if (buf->next  &&  buf->next->used > 0) {
        char *p;

        while (buf != NULL  &&  buf->used > 0) {
            total += buf->used;
            buf = buf->next;
        }

        tmp_buf = (char *)VTSS_MALLOC(total);
        if (!tmp_buf) {
            return FALSE;
        }

        buf = res->head;
        p = tmp_buf;
        while (buf != NULL  &&  buf->used > 0) {
            memcpy(p, buf->text, buf->used);
            p += buf->used;
            buf = buf->next;
        }
    } else {
        total = buf->used;
    }

    buf        = res->head;
    tftp_err   = 0;

    status = vtss_tftp_put(url_parts->path,
                           url_parts->host,
                           url_parts->port,
                           tmp_buf ? tmp_buf : buf->text,
                           total,
                           true,
                           &tftp_err);

    VTSS_FREE(tmp_buf);

    if ( status < 0 ) {
        T_W("TFTP put %s/%s: %s\n", url_parts->host, url_parts->path, vtss_tftp_err2str(tftp_err));
        return FALSE;
    }
    return TRUE;
}

static BOOL _save_config(
    vtss_icfg_query_result_t    *res,
    const misc_url_parts_t      *url_parts
)
{
    return url_parts->host[0] ?
           _save_to_tftp (res, url_parts) :
           _save_to_flash(res, url_parts->path);
}

/* Load from flash. Allocates result buffer; caller must free it even upon
 * error
 */
static BOOL _load_from_flash(
    vtss_icfg_query_result_t    *res,
    const char                  *filename
)
{
    const char *msg = icfg_file_read(filename, res);

    if ( msg ) {
        return FALSE;
    }
    return TRUE;
}

#define TFTP_BUFFER_SIZE (4*1024*1024)

static BOOL _load_from_tftp(
    vtss_icfg_query_result_t    *res,
    const misc_url_parts_t      *url_parts
)
{
    int status = 0;
    int tftp_err;

    if (vtss_icfg_init_query_result(TFTP_BUFFER_SIZE, res) != VTSS_RC_OK) {
        return FALSE;
    }

    status = vtss_tftp_get(url_parts->path,
                           url_parts->host,
                           url_parts->port,
                           res->tail->text,
                           res->tail->size,
                           true,
                           &tftp_err);

    if (status < 0) {
        T_W("TFTP get %s/%s: %s\n", url_parts->host, url_parts->path, vtss_tftp_err2str(tftp_err));
        return FALSE;
    }
    res->tail->used = (u32) status;

    return TRUE;
}



static BOOL _load_config(
    vtss_icfg_query_result_t    *res,
    const misc_url_parts_t      *url_parts
)
{
    return url_parts->host[0] ?
           _load_from_tftp (res, url_parts) :
           _load_from_flash(res, url_parts->path);
}

/**
 * \brief Get file statistics
 *
 * To read current statistics of all fills in flash.
 *
 * \param statistics [OUT] The statistics of all files in flash
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_file_statistics_get(
    vtss_appl_icfg_file_statistics_t    *const statistics
)
{
    /* check parameter */
    if ( statistics == NULL ) {
        T_E("statistics == NULL\n");
        return VTSS_RC_ERROR;
    }

    return _file_statistics_get( statistics );
}

/**
 * \brief Iterate function of file table
 *
 * To get first or get next index.
 *
 * \param prev_fileNo [IN]  previous file number.
 * \param next_fileNo [OUT] next file number.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_file_entry_itr(
    const u32    *const prev_fileNo,
    u32          *const next_fileNo
)
{
    vtss_appl_icfg_file_name_t  fileName;
    u32                         next_no;
    u32                         n;
    BOOL                        b_found;

    /* check parameter */
    if ( next_fileNo == NULL ) {
        T_E("next_fileNo == NULL\n");
        return VTSS_RC_ERROR;
    }

    if ( prev_fileNo ) {
        /* get next */
        next_no = *prev_fileNo + 1;
    } else {
        /* get first */
        next_no = 1;
    }

    /* get next */
    n = 0;
    b_found = FALSE;
    fileName.fileName[0] = 0;
    while ( _file_entry_get_next(&fileName, NULL) == VTSS_RC_OK ) {
        if ( ++n == next_no ) {
            b_found = TRUE;
            break;
        }
    }

    if ( b_found == FALSE ) {
        return VTSS_RC_ERROR;
    }

    /* next file number */
    *next_fileNo = next_no;

    return VTSS_RC_OK;
}

/**
 * \brief Get file entry
 *
 * To read status of each file in flash.
 *
 * \param fileNo    [IN]  (key) File number
 * \param fileEntry [OUT] The current property of the file
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_file_entry_get(
    u32                             fileNo,
    vtss_appl_icfg_file_entry_t     *const fileEntry
)
{
    u32                             n;
    BOOL                            b_found;
    vtss_appl_icfg_file_name_t      fileName;
    vtss_appl_icfg_file_entry_t     fe;

    /* check parameter */
    if ( fileNo == 0 ) {
        T_E("fileNo == 0\n");
        return VTSS_RC_ERROR;
    }

    if ( fileEntry == NULL ) {
        T_E("fileEntry == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* get file name */
    n = 0;
    b_found = FALSE;
    fileName.fileName[0] = 0;
    while ( _file_entry_get_next(&fileName, NULL) == VTSS_RC_OK ) {
        if ( ++n == fileNo ) {
            b_found = TRUE;
            break;
        }
    }

    if ( b_found == FALSE ) {
        return VTSS_RC_ERROR;
    }

    /* get */
    if ( _file_entry_get(&fileName, &fe) != VTSS_RC_OK ) {
        return VTSS_RC_ERROR;
    }

    /* file entry information */
    memcpy( fileEntry, &fe, sizeof(vtss_appl_icfg_file_entry_t) );

    return VTSS_RC_OK;
}

/**
 * \brief Get ICFG status of copy config
 *
 * To get the status of current copy config operation.
 *
 * \param status [OUT] The status of current copy operation.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_status_copy_config_get(
    vtss_appl_icfg_status_copy_config_t     *const status
)
{
    /* check parameter */
    if ( status == NULL ) {
        T_E("status == NULL\n");
        return VTSS_RC_ERROR;
    }

    ICFG_CRIT_ENTER();

    status->status = g_copy_config.copyStatus;

    ICFG_CRIT_EXIT();

    return VTSS_RC_OK;
}

/**
 * \brief Get ICFG Globals Control
 *
 * To read default values of actions.
 *
 * \param control [OUT] The actions
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_control_get(
    vtss_appl_icfg_control_t        *const control
)
{
    /* check parameter */
    if ( control == NULL ) {
        T_E("control == NULL\n");
        return VTSS_RC_ERROR;
    }

    memset( control, 0, sizeof(vtss_appl_icfg_control_t) );
    return VTSS_RC_OK;
}

/**
 * \brief Set ICFG Globals Control
 *
 * To do actions on ICFG.
 *
 * \param control [IN] The actions
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_control_set(
    const vtss_appl_icfg_control_t  *const control
)
{
    misc_url_parts_t    url;

    /* check parameter */
    if ( control == NULL ) {
        T_E("control == NULL\n");
        return VTSS_RC_ERROR;
    }

    if ( ! msg_switch_is_primary() ) {
        return VTSS_RC_ERROR;
    }

    /* reload default */
    switch ( control->reloadDefault ) {
    case VTSS_APPL_ICFG_RELOAD_DEFAULT_NONE:
    default:
        /* do nothing */
        break;

    case VTSS_APPL_ICFG_RELOAD_DEFAULT:
        vtss_flag_maskbits(&icfg_thread_flag, 0);
        vtss_flag_setbits(&icfg_thread_flag, ICFG_THREAD_FLAG_RELOAD_DEFAULT);
        break;

    case VTSS_APPL_ICFG_RELOAD_DEFAULT_KEEP_IP:
        vtss_flag_maskbits(&icfg_thread_flag, 0);
        vtss_flag_setbits(&icfg_thread_flag, ICFG_THREAD_FLAG_RELOAD_DEFAULT_KEEP_IP);
        break;
    }

    /* delete file in flash */
    if ( control->deleteFile[0] ) {
        if ( ! icfg_try_lock_io_mutex() ) {
            return VTSS_RC_ERROR;
        }

        misc_url_parts_init( &url, MISC_URL_PROTOCOL_FLASH );
        if ( misc_url_decompose(control->deleteFile, &url) == FALSE ) {
            icfg_unlock_io_mutex();
            return VTSS_RC_ERROR;
        }

        if ( strcmp(url.protocol, "flash") != 0 ) {
            icfg_unlock_io_mutex();
            return VTSS_RC_ERROR;
        }

        if (icfg_file_delete(url.path) != NULL) {
            icfg_unlock_io_mutex();
            return VTSS_RC_ERROR;
        }

        icfg_unlock_io_mutex();
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get ICFG Copy Config Control
 *
 * To get the values of copy configuration.
 *
 * \param config [OUT] The currnet values to copy configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_copy_config_get(
    vtss_appl_icfg_copy_config_t        *const config
)
{
    /* check parameter */
    if ( config == NULL ) {
        T_E("control == NULL\n");
        return VTSS_RC_ERROR;
    }

    ICFG_CRIT_ENTER();
    memcpy( config, &g_copy_config, sizeof(g_copy_config) );
    ICFG_CRIT_EXIT();

    return VTSS_RC_OK;
}

#define __RETURN_ERROR_STATUS(status) \
    vtss_icfg_free_query_result( &buf ); \
    icfg_unlock_io_mutex(); \
    ICFG_CRIT_ENTER(); \
    g_copy_config.copyStatus = status; \
    ICFG_CRIT_EXIT(); \
    return VTSS_RC_ERROR

#define __RETURN_ERROR() \
    vtss_icfg_free_query_result( &buf ); \
    icfg_unlock_io_mutex(); \
    return VTSS_RC_ERROR

#define __RETURN_OK(status) \
    vtss_icfg_free_query_result( &buf ); \
    icfg_unlock_io_mutex(); \
    _copy_config_update(config, status); \
    return VTSS_RC_OK

/**
 * \brief Set ICFG Copy Config Control
 *
 * To copy configuration between device and files.
 *
 * \param config [IN] The configuration to copy
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_copy_config_set(
    const vtss_appl_icfg_copy_config_t  *const config
)
{
    misc_url_parts_t            src_url;
    misc_url_parts_t            dest_url;
    const char                  *source_path          = NULL;
    const char                  *destination_path     = NULL;
    BOOL                        b_src_running_config  = FALSE;
    BOOL                        b_dest_running_config = FALSE;
    BOOL                        b_dest_startup_config = FALSE;

    vtss_icfg_query_result_t    buf       = { NULL, NULL };
    BOOL                        load_ok   = FALSE;

    /* check parameter */
    if ( config == NULL ) {
        T_E("control == NULL\n");
        return VTSS_RC_ERROR;
    }

    if (msg_switch_is_primary() == FALSE) {
        return VTSS_RC_ERROR;
    }

    if ( icfg_try_lock_io_mutex() == FALSE ) {
        /*
            other in progress
            the status is currently used by the processing copy
            so it can not be modified.
        */
        return VTSS_RC_ERROR;
    }

    /* no action, so just buffer the data for get API */
    if ( config->copy == FALSE ) {
        __RETURN_OK( VTSS_APPL_ICFG_COPY_STATUS_NONE );
    }

    /*
        perform COPY
    */
    memset(&src_url, 0, sizeof(src_url));
    memset(&dest_url, 0, sizeof(dest_url));

    switch ( config->sourceConfigType ) {
    case VTSS_APPL_ICFG_CONFIG_TYPE_NONE:
        __RETURN_ERROR_STATUS( VTSS_APPL_ICFG_COPY_STATUS_FAILED_NO_SUCH_FILE );

    case VTSS_APPL_ICFG_CONFIG_TYPE_RUNNING:
        b_src_running_config = TRUE;
        break;

    case VTSS_APPL_ICFG_CONFIG_TYPE_STARTUP:
        source_path = "flash:startup-config";
        if ( _decompose_url(source_path, &src_url) == FALSE ) {
            __RETURN_ERROR_STATUS( VTSS_APPL_ICFG_COPY_STATUS_FAILED_NO_SUCH_FILE );
        }
        break;

    case VTSS_APPL_ICFG_CONFIG_TYPE_FILE:
        source_path = (char *)(config->sourceConfigFile);
        if ( _decompose_url(source_path, &src_url) == FALSE ) {
            __RETURN_ERROR_STATUS( VTSS_APPL_ICFG_COPY_STATUS_FAILED_NO_SUCH_FILE );
        }
        break;

    default:
        icfg_unlock_io_mutex();
        T_E("invalid sourceConfigType %u", config->sourceConfigType);
        return VTSS_RC_ERROR;
    }

    switch ( config->destinationConfigType ) {
    case VTSS_APPL_ICFG_CONFIG_TYPE_NONE:
        __RETURN_ERROR_STATUS( VTSS_APPL_ICFG_COPY_STATUS_FAILED_NO_SUCH_FILE );

    case VTSS_APPL_ICFG_CONFIG_TYPE_RUNNING:
        b_dest_running_config = TRUE;
        break;

    case VTSS_APPL_ICFG_CONFIG_TYPE_STARTUP:
        destination_path = "flash:startup-config";
        if ( _decompose_url(destination_path, &dest_url) == FALSE ) {
            __RETURN_ERROR_STATUS( VTSS_APPL_ICFG_COPY_STATUS_FAILED_NO_SUCH_FILE );
        }
        break;

    case VTSS_APPL_ICFG_CONFIG_TYPE_FILE:
        destination_path = (char *)(config->destinationConfigFile);
        if ( _decompose_url(destination_path, &dest_url) == FALSE ) {
            __RETURN_ERROR_STATUS( VTSS_APPL_ICFG_COPY_STATUS_FAILED_NO_SUCH_FILE );
        }
        break;

    default:
        icfg_unlock_io_mutex();
        T_E("invalid destinationConfigType %u", config->destinationConfigType);
        return VTSS_RC_ERROR;
    }

    // Check for identical source and destination, i.e. one of
    //   * running-config to running-config
    //   * same protocol + path (covers flash:x to flash:x)
    //   * same protocol + path + host
    if ( b_src_running_config && b_dest_running_config ) {
        __RETURN_ERROR_STATUS( VTSS_APPL_ICFG_COPY_STATUS_FAILED_SAME_SRC_DST );
    }

    if ( strcmp(src_url.protocol, dest_url.protocol) == 0 &&
         strcmp(src_url.path,     dest_url.path)     == 0 &&
         strcmp(src_url.host,     dest_url.host)     == 0  ) {
        __RETURN_ERROR_STATUS( VTSS_APPL_ICFG_COPY_STATUS_FAILED_SAME_SRC_DST );
    }

    // Deny copies to flash:default-config. eCos file permissions aren't fully
    // functional, so we have to catch it here
    if ( strcmp(dest_url.protocol, "flash") == 0 && _is_default_config(dest_url.path) ) {
        __RETURN_ERROR_STATUS( VTSS_APPL_ICFG_COPY_STATUS_FAILED_PERMISSION_DENIED );
    }

    /* start processing */
    ICFG_CRIT_ENTER();
    g_copy_config.copyStatus = VTSS_APPL_ICFG_COPY_STATUS_IN_PROGRESS;
    ICFG_CRIT_EXIT();

    if ( b_src_running_config ) {
        /* build running config */
        load_ok = vtss_icfg_query_all(FALSE, &buf) == VTSS_RC_OK;
    } else {
        /* load config file */
        load_ok = _load_config(&buf, &src_url);
    }

    if ( load_ok == FALSE ) {
        __RETURN_ERROR_STATUS( VTSS_APPL_ICFG_COPY_STATUS_FAILED_LOAD_SRC );
    }

    if ( b_dest_running_config ) {
        if (g_copy_to_running) {
            __RETURN_ERROR_STATUS( VTSS_APPL_ICFG_COPY_STATUS_FAILED_OTHER_IN_PROCESSING );
        }
        g_copy_to_running = TRUE;
        g_copy_merge = config->merge;
        strncpy(g_copy_source_path, source_path, VTSS_APPL_ICFG_FILE_STR_LEN);
        g_copy_source_path[VTSS_APPL_ICFG_FILE_STR_LEN] = 0;
        g_copy_query_buf = buf;

        vtss_flag_maskbits(&icfg_thread_flag, 0);
        vtss_flag_setbits(&icfg_thread_flag, g_copy_merge ? ICFG_THREAD_FLAG_COPY_TO_RUNNING : ICFG_THREAD_FLAG_RELOAD_DEFAULT);

        _copy_config_update(config, VTSS_APPL_ICFG_COPY_STATUS_IN_PROGRESS);

        /*
            g_copy_query_buf will be freed in ICFG_THREAD_FLAG_COPY_TO_RUNNING
        */

        icfg_unlock_io_mutex();
        return VTSS_RC_OK;
    } else {
        if ( _save_config(&buf, &dest_url) == FALSE ) {
            __RETURN_ERROR_STATUS( VTSS_APPL_ICFG_COPY_STATUS_FAILED_SAVE_DST );
        }

        b_dest_startup_config = ( ( ! strcmp(dest_url.protocol, "flash") ) && _is_specific_filename(dest_url.path, "startup-config") );
        if ( b_src_running_config && b_dest_startup_config ) {
#if defined(VTSS_SW_OPTION_HTTPS)
            /* save HTTPS certificate */
            https_mgmt_cert_save();
#endif
        }
    }

    __RETURN_OK( VTSS_APPL_ICFG_COPY_STATUS_SUCCESS );
}
