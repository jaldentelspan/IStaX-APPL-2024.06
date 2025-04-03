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

/**
 * \file
 * \brief Module Ids
 * \details This header file describes module IDs
 */

#ifndef _VTSS_APPL_MODULE_ID_H_
#define _VTSS_APPL_MODULE_ID_H_

/** First module specific error code. Can be used as first entry in error type enumeration */
#define MODULE_ERROR_START(module_id) (-((module_id<<16) | 0xffff))

/** Decompose return code into module_id */
#define VTSS_RC_GET_MODULE_ID(rc)     (((-rc) >> 16) & 0xffff)

/** Decomppse return code in an error code */
#define VTSS_RC_GET_MODULE_CODE(rc)   (-(rc & 0xffff))

/** Module IDs
 * !!!!! IMPORTANT !!!!!
 * ---------------------
 * When adding new module IDs, these MUST be added at the end of the current
 * list. Also module IDs MUST NEVER be deleted from the list.
 * This is necessary to ensure that the Msg protocol can rely on consistent
 * module IDs between different SW versions.
 */
enum {
    /* Switch API */
    VTSS_MODULE_ID_API_IO               =   0,  /* API I/O Layer */
    VTSS_MODULE_ID_API_CI               =   1,  /* API Chip Interface Layer */
    VTSS_MODULE_ID_API_AI               =   2,  /* API Application Interface Layer */
    VTSS_MODULE_ID_SPROUT               =   3,  /* SPROUT (3) */
    VTSS_MODULE_ID_MAIN                 =   4,
    VTSS_MODULE_ID_TOPO                 =   5,
    VTSS_MODULE_ID_CONF                 =   6,
    VTSS_MODULE_ID_MSG                  =   7,
    VTSS_MODULE_ID_PACKET               =   8,
    VTSS_MODULE_ID_TRACE                =   9,
    VTSS_MODULE_ID_IP_STACK_GLUE        =  10,
    VTSS_MODULE_ID_PORT                 =  11,
    VTSS_MODULE_ID_MAC                  =  12,
    VTSS_MODULE_ID_VLAN                 =  13,
    VTSS_MODULE_ID_QOS                  =  14,
    VTSS_MODULE_ID_MIRROR               =  15,
    VTSS_MODULE_ID_MISC                 =  16,
    VTSS_MODULE_ID_ACL                  =  17,
    VTSS_MODULE_ID_SYMREG               =  18,
    VTSS_MODULE_ID_AGGR                 =  19,
    VTSS_MODULE_ID_RSTP                 =  20,
    VTSS_MODULE_ID_DOT1X                =  21,
    VTSS_MODULE_ID_AFI                  =  22,
    VTSS_MODULE_ID_PVLAN                =  23,
    VTSS_MODULE_ID_SYSTEM               =  24,
    VTSS_MODULE_ID_CLI                  =  25,
    VTSS_MODULE_ID_WEB                  =  26,
    VTSS_MODULE_ID_PING                 =  27,
    VTSS_MODULE_ID_FIRMWARE             =  28,
    VTSS_MODULE_ID_UNMGD                =  29,  /* Appl. code for unmanaged, single-threaded switch */
    VTSS_MODULE_ID_MSG_TEST             =  30,  /* 30. Used for test-purposes only. */
    VTSS_MODULE_ID_LED                  =  31,  /* LED handling */
    VTSS_MODULE_ID_CRITD                =  32,  /* Critical regions with debug facilities */
    VTSS_MODULE_ID_L2PROTO              =  33,
    VTSS_MODULE_ID_LLDP                 =  34,
    VTSS_MODULE_ID_LACP                 =  35,
    VTSS_MODULE_ID_SNMP                 =  36,
    VTSS_MODULE_ID_SYSLOG               =  37,
    VTSS_MODULE_ID_IPMC_LIB             =  38,
    VTSS_MODULE_ID_PERF_MON_CONF_UNUSED =  39,
    VTSS_MODULE_ID_VTSS_LB              =  40,
    VTSS_MODULE_ID_INTERRUPT            =  41,
    VTSS_MODULE_ID_SYNCE                =  42,
    VTSS_MODULE_ID_POE                  =  43,
    VTSS_MODULE_ID_MODULE_CONFIG        =  44,
    VTSS_MODULE_ID_EPS_UNUSED           =  45,
    VTSS_MODULE_ID_MEP_UNUSED           =  46,
    VTSS_MODULE_ID_HTTPS                =  47,
    VTSS_MODULE_ID_AUTH                 =  48,
    VTSS_MODULE_ID_SSH                  =  49,
    VTSS_MODULE_ID_RADIUS               =  50,
    VTSS_MODULE_ID_ACCESS_MGMT          =  51,
    VTSS_MODULE_ID_UPNP                 =  52,
    VTSS_MODULE_ID_IP_DNS               =  53,
    VTSS_MODULE_ID_DHCP_HELPER          =  54,
    VTSS_MODULE_ID_DHCP_RELAY           =  55,
    VTSS_MODULE_ID_DHCP_SNOOPING        =  56,
    VTSS_MODULE_ID_NTP                  =  57,
    VTSS_MODULE_ID_USERS                =  58,
    VTSS_MODULE_ID_PRIV_LVL             =  59,
    VTSS_MODULE_ID_SECURITY             =  60,
    VTSS_MODULE_ID_DEBUG                =  61,
    VTSS_MODULE_ID_EVC_UNUSED           =  62,
    VTSS_MODULE_ID_ARP_INSPECTION       =  63,
    VTSS_MODULE_ID_IP_SOURCE_GUARD      =  64,
    VTSS_MODULE_ID_PTP                  =  65,
    VTSS_MODULE_ID_PSEC                 =  66,
    VTSS_MODULE_ID_PSEC_LIMIT           =  67,
    VTSS_MODULE_ID_MVR                  =  68,
    VTSS_MODULE_ID_IPMC                 =  69,
    VTSS_MODULE_ID_VOICE_VLAN           =  70,
    VTSS_MODULE_ID_LLDPMED              =  71,
    VTSS_MODULE_ID_ERPS                 =  72,
    VTSS_MODULE_ID_ETH_LINK_OAM         =  73,
    VTSS_MODULE_ID_EEE                  =  74,
    VTSS_MODULE_ID_FAN                  =  75,
    VTSS_MODULE_ID_TOD                  =  76,
    VTSS_MODULE_ID_LED_POW_REDUC        =  77,
    VTSS_MODULE_ID_THERMAL_PROTECT      =  78,
    VTSS_MODULE_ID_VCL                  =  79,
    VTSS_MODULE_ID_UFDMA_AIL            =  80,
    VTSS_MODULE_ID_UFDMA_CIL            =  81,
    VTSS_MODULE_ID_THREAD_LOAD_MONITOR  =  82,
    VTSS_MODULE_ID_SFLOW                =  83,
    VTSS_MODULE_ID_Y1564_UNUSED         =  84,
    VTSS_MODULE_ID_VLAN_TRANSLATION     =  85,
    VTSS_MODULE_ID_MRP                  =  86,
    VTSS_MODULE_ID_MVRP                 =  87,
    VTSS_MODULE_ID_SYNCE_DPLL           =  88,
    VTSS_MODULE_ID_XXRP                 =  89,
    VTSS_MODULE_ID_IP_ROUTING           =  90,
    VTSS_MODULE_ID_LOOP_PROTECT         =  91,
    VTSS_MODULE_ID_RMON                 =  92,
    VTSS_MODULE_ID_TIMER                =  93,
    VTSS_MODULE_ID_SECURITY_NETWORK     =  94,
    VTSS_MODULE_ID_ICLI                 =  95,
    VTSS_MODULE_ID_REMOTE_TS_PHY_UNUSED =  96,
    VTSS_MODULE_ID_DAYLIGHT_SAVING      =  97,
    VTSS_MODULE_ID_PHY                  =  98,
    VTSS_MODULE_ID_CONSOLE              =  99,
    VTSS_MODULE_ID_GREEN_ETHERNET       = 100,
    VTSS_MODULE_ID_ICFG                 = 101,
    VTSS_MODULE_ID_IP                   = 102,
    VTSS_MODULE_ID_IP_CHIP_UNUSED       = 103,
    VTSS_MODULE_ID_DHCP_CLIENT          = 104,
    VTSS_MODULE_ID_SNTP                 = 105,
    VTSS_MODULE_ID_MEBA                 = 106,
    VTSS_MODULE_ID_ZL_3034X_API         = 107,
    VTSS_MODULE_ID_ZL_3034X_PDV         = 108,
    VTSS_MODULE_ID_DHCP_SERVER          = 109,
    VTSS_MODULE_ID_RFC2544_UNUSED       = 110,
    VTSS_MODULE_ID_JSON_RPC             = 111,
    VTSS_MODULE_ID_MACSEC               = 112,
    VTSS_MODULE_ID_BASICS               = 113,
    VTSS_MODULE_ID_DHCP                 = 114,
    VTSS_MODULE_ID_DBGFS                = 115,
    VTSS_MODULE_ID_SNMP_DEMO            = 116,
    VTSS_MODULE_ID_PERF_MON_UNUSED      = 117,
    VTSS_MODULE_ID_POST                 = 118,
    VTSS_MODULE_ID_ZTP                  = 119,
    VTSS_MODULE_ID_RMIRROR              = 120,
    VTSS_MODULE_ID_DDMI                 = 121,
    VTSS_MODULE_ID_SUBJECT              = 122,
    VTSS_MODULE_ID_UDLD                 = 123,
    VTSS_MODULE_ID_DIAG_UNUSED          = 124,
    VTSS_MODULE_ID_HQOS_UNUSED          = 125,
    VTSS_MODULE_ID_DHCP6C               = 126,
    VTSS_MODULE_ID_MPLS_TP_UNUSED       = 127,
    VTSS_MODULE_ID_TT_LOOP_UNUSED       = 128,
    VTSS_MODULE_ID_JSON_RPC_NOTIFICATION= 129,
    VTSS_MODULE_ID_FLEX_LINKS           = 130,
    VTSS_MODULE_ID_MRP_UNUSED           = 131,
    VTSS_MODULE_ID_JSON_API_EXPOSE      = 132,
    VTSS_MODULE_ID_FAST_CGI             = 133,
    VTSS_MODULE_ID_NTP_LINUX            = 134,
    VTSS_MODULE_ID_EXPRESS_SETUP        = 135,
    VTSS_MODULE_ID_ALARM                = 136,
    VTSS_MODULE_ID_SR_UNUSED            = 137,
    VTSS_MODULE_ID_JSON_IPC             = 138,
    VTSS_MODULE_ID_CIP                  = 139,
    VTSS_MODULE_ID_ERRDISABLE           = 140,
    VTSS_MODULE_ID_SW_PUSH_BUTTON       = 141,
    VTSS_MODULE_ID_OPTIONAL_MODULES     = 142,
    VTSS_MODULE_ID_FRR                  = 143,
    VTSS_MODULE_ID_TRACEROUTE           = 144,
    VTSS_MODULE_ID_DHCP6_RELAY          = 145,
    VTSS_MODULE_ID_DHCP6_SNOOPING       = 146,
    VTSS_MODULE_ID_IPV6_SOURCE_GUARD    = 147,
    VTSS_MODULE_ID_CFM                  = 148,
    VTSS_MODULE_ID_CPUPORT              = 149,
    VTSS_MODULE_ID_APS                  = 150,
    VTSS_MODULE_ID_TSN                  = 151,
    VTSS_MODULE_ID_FRR_RIP              = 152,
    VTSS_MODULE_ID_FRR_ROUTER           = 153,
    VTSS_MODULE_ID_FRR_OSPF             = 154,
    VTSS_MODULE_ID_PSFP                 = 155,
    VTSS_MODULE_ID_FRR_OSPF6            = 156,
    VTSS_MODULE_ID_FRER                 = 157,
    VTSS_MODULE_ID_KR                   = 158,
    VTSS_MODULE_ID_IEC_MRP              = 159,
    VTSS_MODULE_ID_CUST_0               = 160, // Customer Module ID #0. Not used by MCHP-distributed code.
    VTSS_MODULE_ID_CUST_1               = 161, // Customer Module ID #1. Not used by MCHP-distributed code
    VTSS_MODULE_ID_CUST_2               = 162, // Customer Module ID #2. Not used by MCHP-distributed code
    VTSS_MODULE_ID_CUST_3               = 163, // Customer Module ID #3. Not used by MCHP-distributed code
    VTSS_MODULE_ID_CUST_4               = 164, // Customer Module ID #4. Not used by MCHP-distributed code
    VTSS_MODULE_ID_REDBOX               = 165,
    VTSS_MODULE_ID_STREAM               = 166,

    /*
     * INSERT NEW MODULE IDS HERE. AND ONLY HERE!!!
     *
     * REMEMBER to add a new entry in the module id database on our twiki
     * before adding the entry here!!!
     *
     * Assign the module ID number from the database to the enum value here
     * like shown in VTSS_MODULE_ID_DHCP_SERVER above.
     * This will allow for 'holes' in the enum ranges on different products/branches.
     *
     * REMEMBER ALSO TO ADD ENTRY IN \vtss_appl\util\vtss_module_id.c\vtss_module_names[] !!!
     * REMEMBER ALSO TO ADD ENTRY IN \vtss_appl\util\vtss_module_id.c vtss_priv_lvl_groups_filter[] !!!
     */

    /* Last entry, default */
    VTSS_MODULE_ID_NONE
};

/**
 * We'd like to know when e.g. an arg is passed as a module ID - hence this typedef
 */
typedef int vtss_module_id_t;

/**
 * Array - defined in util/vtss_module_id.h - that allows the application to directly
 * convert a module ID to a textual representation.
 */
extern const char *const vtss_module_names[VTSS_MODULE_ID_NONE + 1];

/**
 * Array - defined in util/vtss_module_id.h - that conveys a given module's
 * privilege requirements.
 */
extern const int vtss_priv_lvl_groups_filter[VTSS_MODULE_ID_NONE + 1];

#endif /* VTSS_APPL_MODULE_ID_H_ */

