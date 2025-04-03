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

#ifndef _VTSS_SNMP_API_H_
#define _VTSS_SNMP_API_H_

#include "ip_api.h"
#include "sysutil_api.h"    // For VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN
#include "mgmt_api.h"       // For mgmt_txt2ipv6()
#include "vtss/appl/snmp.h"
#include "vtss/appl/auth.h"
#include "vtss_os_wrapper_snmp.h"
#ifdef __cplusplus
#include "vtss/basics/map.hxx"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Thread variables, if using trap API snmp_send_trap(),
   we should increase the stack size */
#define VTSS_TRAP_CONF_MAX      4
#define VTSS_TRAP_CONF_ID_MIN   0
#define VTSS_TRAP_CONF_ID_MAX   (VTSS_TRAP_CONF_MAX - 1)

#define VTSS_TRAP_SOURCE_MAX    32
#define VTSS_TRAP_SOURCE_ID_MIN 0
#define VTSS_TRAP_SOURCE_ID_MAX (VTSS_TRAP_SOURCE_MAX - 1)

#define VTSS_TRAP_FILTER_MAX    VTSS_APPL_SNMP_TRAP_FILTER_MAX
#define VTSS_TRAP_FILTER_ID_MIN 0
#define VTSS_TRAP_FILTER_ID_MAX (VTSS_TRAP_SOURCE_MAX - 1)

#define RFC3414_SUPPORTED_USMSTATS                      1 // Handled by NetSNMP
#define RFC3414_SUPPORTED_USMUSER                       1 // Handled by NetSNMP
#define RFC3415_SUPPORTED_VACMCONTEXTTABLE              1 // Handled by NetSNMP
#define RFC3415_SUPPORTED_VACMSECURITYTOGROUPTABLE      1 // Handled by NetSNMP
#define RFC3415_SUPPORTED_VACMACCESSTABLE               1 // Handled by NetSNMP
#define RFC3415_SUPPORTED_VACMMIBVIEWS                  1 // Handled by NetSNMP

#define VTSS_TRAP_MSG_TRAP2     VTSS_APPL_TRAP_NOTIFY_TRAP
#define VTSS_TRAP_MSG_INFORM    VTSS_APPL_TRAP_NOTIFY_INFORM

/* SNMP management switch enable/disable */
#define SNMP_MGMT_ENABLED       1
#define SNMP_MGMT_DISABLED      0

/* SNMP version */
#define SNMP_MGMT_VERSION_1     VTSS_APPL_SNMP_VERSION_1
#define SNMP_MGMT_VERSION_2C    VTSS_APPL_SNMP_VERSION_2C
#define SNMP_MGMT_VERSION_3     VTSS_APPL_SNMP_VERSION_3

/* SNMP max OID length. */
#define SNMP_MGMT_MAX_OID_LEN               VTSS_APPL_SNMP_MAX_OID_LEN + 1
#define SNMP_MGMT_MAX_SUBTREE_LEN           VTSS_APPL_SNMP_MAX_SUBTREE_LEN
#define SNMP_MGMT_MAX_SUBTREE_STR_LEN       VTSS_APPL_SNMP_MAX_SUBTREE_STR_LEN

/* SNMP max name length. */
#define SNMP_MGMT_MAX_NAME_LEN              128

/* SNMP max community length. */
#define SNMP_MGMT_INPUT_COMMUNITY_LEN       VTSS_APPL_SNMP_COMMUNITY_LEN
#define SNMP_MGMT_MAX_COMMUNITY_LEN         SNMP_MGMT_INPUT_COMMUNITY_LEN + 1
#define TRAP_MAX_NAME_LEN                   VTSS_APPL_SNMP_MAX_NAME_LEN
#define TRAP_MIN_NAME_LEN                   1
#define TRAP_MAX_TABLE_NAME_LEN             VTSS_APPL_TRAP_TABLE_NAME_SIZE

/* SNMP max trap inform retry times */
#define SNMP_MGMT_MAX_TRAP_INFORM_TIMEOUT   2147 //0x7FFFFFFF usec

/* SNMP max trap inform retry times */
#define SNMP_MGMT_MAX_TRAP_INFORM_RETRIES   255

/* Row storage values. */
#define SNMP_MGMT_STORAGE_OTHER         1
#define SNMP_MGMT_STORAGE_VOLATILE      2
#define SNMP_MGMT_STORAGE_NONVOLATILE   3
#define SNMP_MGMT_STORAGE_PERMANENT     4
#define SNMP_MGMT_STORAGE_READONLY      5

/* Row status values. */
#define SNMP_MGMT_ROW_NONEXISTENT       0
#define SNMP_MGMT_ROW_ACTIVE            1
#define SNMP_MGMT_ROW_NOTINSERVICE      2
#define SNMP_MGMT_ROW_NOTREADY          3
#define SNMP_MGMT_ROW_CREATEANDGO       4
#define SNMP_MGMT_ROW_CREATEANDWAIT     5
#define SNMP_MGMT_ROW_DESTROY           6

/* SNMPv3 max engine ID length */
#define SNMPV3_MIN_ENGINE_ID_LEN                VTSS_APPL_SNMP_ENGINE_ID_MIN_LEN
#define SNMPV3_MAX_ENGINE_ID_LEN                VTSS_APPL_SNMP_ENGINE_ID_MAX_LEN

/* SNMPv3 max name length */
#define SNMPV3_MIN_NAME_LEN                     1
#define SNMPV3_MAX_NAME_LEN                     VTSS_APPL_SNMP_MAX_NAME_LEN

/* SNMPv3 max password length */
#define SNMPV3_MIN_PASSWORD_LEN                 8
#define SNMPV3_MAX_MD5_PASSWORD_LEN             32
#define SNMPV3_MAX_SHA_PASSWORD_LEN             VTSS_APPL_SNMP_MAX_SHA_PASSWORD_LEN
#define SNMPV3_MAX_DES_PASSWORD_LEN             VTSS_APPL_SNMP_MAX_DES_PASSWORD_LEN

#define SNMPV3_MIN_EVENT_DESC_LEN           1
#define SNMPV3_MAX_EVENT_DESC_NAME_LEN      127

#define SNMP_DEFAULT_MODE                           SNMP_MGMT_ENABLED
#define SNMP_DEFAULT_RO_COMMUNITY                   "public"
#define SNMP_DEFAULT_RW_COMMUNITY                   "private"

/* SNMPv3 default configuration */
#define SNMPV3_DEFAULT_ENGINE_ID_LEN            11 /* 4 bytes IANA PEN, 1 byte type, 6 bytes MAC */
#define SNMPV3_DEFAULT_COMMUNITY_SIP_TYPE       MESA_IP_TYPE_IPV4
#define SNMPV3_DEFAULT_COMMUNITY_SIP_IPV4       0
#define SNMPV3_DEFAULT_COMMUNITY_SIP_PREFIX     0
#define SNMPV3_DEFAULT_USER                     "default_user"
#define SNMPV3_DEFAULT_RO_GROUP                 "default_ro_group"
#define SNMPV3_DEFAULT_RW_GROUP                 "default_rw_group"
#define SNMPV3_DEFAULT_VIEW                     "default_view"

/* SNMP TRAPS default configuration */
#define TRAP_CONF_DEFAULT_ENABLE            FALSE
#define TRAP_CONF_DEFAULT_DPORT             162
#define TRAP_CONF_DEFAULT_VER               SNMP_MGMT_VERSION_2C
#define TRAP_CONF_DEFAULT_COMM              "public"
#define TRAP_CONF_DEFAULT_INFORM_MODE       VTSS_TRAP_MSG_TRAP2
#define TRAP_CONF_DEFAULT_INFORM_RETRIES    5
#define TRAP_CONF_DEFAULT_INFORM_TIMEOUT    3
#define TRAP_CONF_DEFAULT_SEC_NAME          SNMPV3_NONAME

#define TRAP_NAME_COLD_START        "coldStart"
#define TRAP_NAME_WARM_START        "warmStart"
#define TRAP_NAME_LINK_UP           "linkUp"
#define TRAP_NAME_LINK_DOWN         "linkDown"
#define TRAP_NAME_AUTH_FAIL         "authenticationFailure"
#define TRAP_NAME_NEW_ROOT          "newRoot"
#define TRAP_NAME_TOPO_CHNG         "topologyChange"
#define TRAP_NAME_LLDP_REM_TBL_CHNG "lldpRemTablesChange"
#define TRAP_NAME_RMON_RISING       "risingAlarm"
#define TRAP_NAME_RMON_FALLING      "fallingAlarm"
#define TRAP_NAME_ENTITY_CONF_CHNG  "entConfigChange"

#define SEQUENCE_TAG            0x30
#define TimeTicks_TAG           0x43

#define SIZEOFARRAY(trapOID) (sizeof(trapOID)/sizeof(trapOID[0]))
#define OID_ROOT_SET(var, Oid) var = 40 * Oid[0] + Oid[1];

#define VTSS_OBJ_ID_SERIALIZE(PAYLOAD, LENGTH, Oid) \
    do {\
        PAYLOAD[LENGTH++] = ASN_OBJECT_ID;\
        int len_index = LENGTH;\
        LENGTH += 1;\
        OID_ROOT_SET(PAYLOAD[LENGTH++], Oid);\
        for (int i = 2; i < SIZEOFARRAY(Oid); i++) {\
            LENGTH += encode_base128(Oid[i], &PAYLOAD[LENGTH]);\
        }\
        PAYLOAD[len_index] = (LENGTH - len_index - 1);\
    } while (0)

#define VTSS_VAR_BIND_TLV(PAYLOAD, LENGTH, T, L, V) \
    do {\
        PAYLOAD[LENGTH++] = T;\
        PAYLOAD[LENGTH++] = L;\
        memcpy(&PAYLOAD[LENGTH], V, L);\
        LENGTH += L;\
    } while(0)

#define VTSS_SEQ_TAG_START(PAYLOAD, LENGTH, TagID) do {\
    PAYLOAD[LENGTH++] = TagID;\
    PAYLOAD[LENGTH++] = 0x82;\
    int len_index = LENGTH;\
    LENGTH += 2;

#define VTSS_SEQ_TAG_END(PAYLOAD, LENGTH) \
        u16 len = htons(LENGTH - len_index - 2);\
        memcpy(&PAYLOAD[len_index], &len, sizeof(len));\
    } while(0)

#define VTSS_VAR_BIND(PAYLOAD, LENGTH, OID, T, L, V) \
    do {\
        VTSS_SEQ_TAG_START(PAYLOAD, LENGTH, SEQUENCE_TAG);\
        {\
            VTSS_OBJ_ID_SERIALIZE(PAYLOAD, LENGTH, OID);\
            VTSS_VAR_BIND_TLV(PAYLOAD, LENGTH, T, L, V);\
        }\
        VTSS_SEQ_TAG_END(PAYLOAD, LENGTH);\
    } while(0)

#define VTSS_OID_BIND(PAYLOAD, LENGTH, OID1, OID2) \
    do {\
        VTSS_SEQ_TAG_START(PAYLOAD, LENGTH, SEQUENCE_TAG);\
        {\
            VTSS_OBJ_ID_SERIALIZE(PAYLOAD, LENGTH, OID1);\
            VTSS_OBJ_ID_SERIALIZE(PAYLOAD, LENGTH, OID2);\
        }\
        VTSS_SEQ_TAG_END(PAYLOAD, LENGTH);\
    } while(0)

#define VTSS_SNMP_SEQ_BLOCK(PAYLOAD, LENGTH, TagID, BLOCK) \
    do {\
            VTSS_SEQ_TAG_START(PAYLOAD, LENGTH, TagID);\
            {\
                BLOCK\
            }\
            VTSS_SEQ_TAG_END(PAYLOAD, LENGTH);\
    } while(0)

/* SNMP error codes (mesa_rc) */
typedef enum {
    SNMP_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_SNMP), /* Generic error code */
    SNMP_ERROR_PARM,                                          /* Illegal parameter */
    SNMP_ERROR_STACK_STATE,                                   /* Illegal primary/secondary switch state */
    SNMP_ERROR_ENGINE_FAIL,                                   /* SNMP engine occur fail */
    SNMP_ERROR_ROW_STATUS_INCONSISTENT,                       /* Illegal operation inconsistent with row status */
    SNMP_ERROR_SMON_STAT_TABLE_FULL,                          /* SMON Stat table is full */
    SNMP_ERROR_NULL,                                          /* SNMP unexpect NULL pointer */
    SNMPV3_ERROR_COMMUNITIES_TABLE_FULL,                      /* SNMP communities table full */
    SNMPV3_ERROR_USERS_TABLE_FULL,                            /* SNMPv3 users table full */
    SNMPV3_ERROR_GROUPS_TABLE_FULL,                           /* SNMPv3 groups table full */
    SNMPV3_ERROR_VIEWS_TABLE_FULL,                            /* SNMPv3 views table full */
    SNMPV3_ERROR_ACCESSES_TABLE_FULL,                         /* SNMPv3 accesses table full */
    SNMPV3_ERROR_ENGINE_ID_LEN_EXCEEDED,                      /* SNMPv3 Engine ID length exceeded */
    SNMPV3_ERROR_USER_NAME_LEN_EXCEEDED,                      /* SNMPv3 user name length exceeded */
    SNMPV3_ERROR_PASSWORD_LEN,                                /* SNMPv3 Invalid password length */
    SNMPV3_ERROR_COMMUNITY_ALREADY_EXIST,                     /* SNMP Community already exist */
    SNMPV3_ERROR_USER_ALREADY_EXIST,                          /* SNMPv3 User already exist */
    SNMPV3_ERROR_GROUP_ALREADY_EXIST,                         /* SNMPv3 Group already exist */
    SNMPV3_ERROR_VIEW_ALREADY_EXIST,                          /* SNMPv3 View already exist */
    SNMPV3_ERROR_ACCESS_ALREADY_EXIST,                        /* SNMPv3 Access already exist */
    SNMPV3_ERROR_COMMUNITY_NOT_EXIST,                         /* SNMP Community does not exist */
    SNMPV3_ERROR_USER_NOT_EXIST,                              /* SNMPv3 User does not exist */
    SNMPV3_ERROR_GROUP_NOT_EXIST,                             /* SNMPv3 Group does not exist */
    SNMPV3_ERROR_VIEW_NOT_EXIST,                              /* SNMPv3 View does not exist */
    SNMPV3_ERROR_ACCESS_NOT_EXIST,                            /* SNMPv3 Access does not exist */
    SNMP_ERROR_TRAP_RECV_ALREADY_EXIST,                       /* SNMP Trap receiver already exist */
    SNMP_ERROR_TRAP_SOURCE_ALREADY_EXIST,                     /* SNMP Trap source already exist */
    SNMP_ERROR_TRAP_RECV_NOT_EXIST,                           /* SNMP Trap receiver does not exist */
    SNMP_ERROR_TRAP_SOURCE_NOT_EXIST,                         /* SNMP Trap source does not exist */
    SNMPV3_ERROR_NO_USER_FOUND,                               /* SNMPv3 No such user found */
    SNMPV3_ERROR_COMMUNITY_TOO_LONG,                          /* SNMPv3 Community name too long */
    SNMPV3_ERROR_SEC_NAME_TOO_LONG,                           /* SNMPv3 Security name too long */
    SNMPV3_ERROR_COMMUNITIES_IP_OVERLAP,                      /* SNMPv3 Community has overlapping IP */
    SNMPV3_ERROR_COMMUNITIES_IP_INVALID,                      /* SNMPv3 Community has invalid IP address or prefix length */
    SNMP_TRAP_NO_SUCH_TRAP,
    SNMP_TRAP_NO_SUCH_SUBSCRIPTION,
    SNMP_TRAP_FILTER_TABLE_FULL,
} snmp_error_t;


/* SNMP configuration */
typedef struct {
    BOOL                        mode;
    u8                          engineid[SNMPV3_MAX_ENGINE_ID_LEN];
    u32                         engineid_len;
} snmp_conf_t;

/* SNMP port configuration */
typedef struct {
    BOOL  linkupdown_trap_enable;    /* linkUp/linkDown trap enable */
} snmp_port_conf_t;


/* SNMP statistics row entry */
typedef struct {
    BOOL  valid;
    ulong ctrl_index;
    ulong if_index;
} snmp_rmon_stat_entry_t;
/* SNMP statistics row entry */
typedef struct {
    BOOL  valid;
    ulong source_index;
    ulong dest_index;
    ulong ctrl_index;
    ulong copydirection;
} snmp_port_copy_entry_t;

/* SNMP vars trap entry,
   Note: the struct only suit to SNMP/eCos module */
typedef struct {
    char  name[TRAP_MAX_TABLE_NAME_LEN];
    u32   index_len;
    ulong index[SNMP_MGMT_MAX_OID_LEN];
    ulong oid[SNMP_MGMT_MAX_OID_LEN];
    ulong oid_len;
    struct variable_list *vars;
} snmp_vars_trap_entry_t;

typedef struct {
    i32        conf_id;
    BOOL       enable;
    vtss_inet_address_t dip;
    u16        trap_port;
    u32        trap_version;
    char       trap_communitySecret[SNMP_MGMT_INPUT_COMMUNITY_LEN + 1];
    char       trap_encryptedSecret[VTSS_APPL_AUTH_KEY_LEN];
    u32        trap_inform_mode;
    u32        trap_inform_timeout;
    u32        trap_inform_retries;
    u8         trap_engineid[SNMPV3_MAX_ENGINE_ID_LEN];
    u32        trap_engineid_len;
    char       trap_security_name[SNMPV3_MAX_NAME_LEN + 1];
} vtss_trap_conf_t;

typedef struct {
    u32        filter_type;                                            /**< The filter type of the entry; included or excluded */
    u32        index_filter_len;                                       /**< Length of index_filter */
    ulong      index_filter[SNMP_MGMT_MAX_OID_LEN];                    /**< Subtree to match for this filter */
    u32        index_mask_len;                                         /**< Length of index_mask */
    u8         index_mask[SNMP_MGMT_MAX_SUBTREE_LEN];                  /**< Mask for the subtree to match for this filter */
} vtss_trap_filter_item_t;

/* SNMP conf trap entry */
typedef struct {
    /* Index */
    char                trap_conf_name[TRAP_MAX_NAME_LEN + 1];
    vtss_trap_conf_t    trap_conf;
    BOOL                valid;
} vtss_trap_entry_t;

typedef struct {
    /* Index */
    i32                 conf_id;
    u32                 active_cnt;
    vtss_trap_filter_item_t *item[VTSS_TRAP_FILTER_MAX];
} vtss_trap_filter_t;

/* SNMP filter trap entry */
typedef struct {
    /* Index */
    char                source_name[TRAP_MAX_TABLE_NAME_LEN + 1];
    vtss_trap_filter_t  trap_filter;
    BOOL                valid;
} vtss_trap_source_t;

typedef struct {
    vtss_trap_entry_t  trap_entry[VTSS_TRAP_CONF_MAX];
    vtss_trap_source_t trap_source[VTSS_TRAP_SOURCE_MAX];
} vtss_trap_sys_conf_t;

/* SNMP error text */
const char *snmp_error_txt(snmp_error_t rc);

const uchar *default_engine_id(void);

/* Get SNMP configuration */
mesa_rc snmp_mgmt_snmp_conf_get(snmp_conf_t *conf);

/* Set SNMP configuration */
mesa_rc snmp_mgmt_snmp_conf_set(snmp_conf_t *conf);

/* Get SNMP defaults */
void snmp_default_get(snmp_conf_t *conf);

/* Determine if SNMP configuration has changed */
int snmp_conf_changed(snmp_conf_t *old, snmp_conf_t *_new);

/* Get SNMP port configuration */
mesa_rc snmp_mgmt_snmp_port_conf_get(vtss_isid_t isid,
                                     mesa_port_no_t port_no,
                                     snmp_port_conf_t *conf);

/* Set SNMP port configuration */
mesa_rc snmp_mgmt_snmp_port_conf_set(vtss_isid_t isid,
                                     mesa_port_no_t port_no,
                                     snmp_port_conf_t *conf);

mesa_rc snmp_mgmt_smon_stat_entry_get(snmp_rmon_stat_entry_t *entry, BOOL next);
mesa_rc snmp_mgmt_port_copy_entry_get(snmp_port_copy_entry_t *entry, BOOL next);
mesa_rc snmp_mgmt_smon_stat_entry_set(snmp_rmon_stat_entry_t *entry);
mesa_rc snmp_mgmt_port_copy_entry_set(snmp_port_copy_entry_t *entry);

struct variable_list *
snmp_bind_var(struct variable_list *prev,
              void *value, int type, size_t sz_val, oid *oidVar, size_t sz_oid);

/* Initialize module */
mesa_rc snmp_init(vtss_init_data_t *data);

#define SNMPV3_MGMT_CONTEX_MATCH_EXACT          1
#define SNMPV3_MGMT_CONTEX_MATCH_PREFIX         2

/* SNMPv3 none group view */
#define SNMPV3_NONAME                           "None"

#define SNMP_MGMT_SEC_MODEL_ANY                 VTSS_APPL_SNMP_SECURITY_MODEL_ANY
#define SNMP_MGMT_SEC_MODEL_SNMPV1              VTSS_APPL_SNMP_SECURITY_MODEL_SNMPV1
#define SNMP_MGMT_SEC_MODEL_SNMPV2C             VTSS_APPL_SNMP_SECURITY_MODEL_SNMPV2C
#define SNMP_MGMT_SEC_MODEL_USM                 VTSS_APPL_SNMP_SECURITY_MODEL_USM

#define SNMP_MGMT_SEC_LEVEL_NOAUTH              VTSS_APPL_SNMP_SECURITY_LEVEL_NOAUTH
#define SNMP_MGMT_SEC_LEVEL_AUTHNOPRIV          VTSS_APPL_SNMP_SECURITY_LEVEL_AUTHNOPRIV
#define SNMP_MGMT_SEC_LEVEL_AUTHPRIV            VTSS_APPL_SNMP_SECURITY_LEVEL_AUTHPRIV

#define SNMPV3_MGMT_VIEW_INCLUDED               VTSS_APPL_SNMP_VIEW_TYPE_INCLUDED
#define SNMPV3_MGMT_VIEW_EXCLUDED               VTSS_APPL_SNMP_VIEW_TYPE_EXCLUDED

/* SNMPv3 max table size */
#define SNMPV3_MAX_COMMUNITIES                  16

/* BZ#19900 - Unlimited the maximum entries count before we has a new patch on SNMP kernel */
#define SNMPV3_TABLES_UNLIMITED_ENTRIES_CNT
#if !defined(SNMPV3_TABLES_UNLIMITED_ENTRIES_CNT)
#define SNMPV3_MAX_USERS                        32
#define SNMPV3_MAX_GROUPS                       (SNMPV3_MAX_COMMUNITIES * 2 + SNMPV3_MAX_USERS)
#define SNMPV3_MAX_VIEWS                        32
#define SNMPV3_MAX_ACCESSES                     32
#endif /* SNMPV3_TABLES_UNLIMITED_ENTRIES_CNT */

/* SNMPv3 configuration access keys */
#define SNMPV3_CONF_ACESS_GETFIRST              ""

/* SNMPv3 authencation protocol */
#define SNMP_MGMT_AUTH_PROTO_NONE   VTSS_APPL_SNMP_AUTH_PROTOCOL_NONE
#define SNMP_MGMT_AUTH_PROTO_MD5    VTSS_APPL_SNMP_AUTH_PROTOCOL_MD5
#define SNMP_MGMT_AUTH_PROTO_SHA    VTSS_APPL_SNMP_AUTH_PROTOCOL_SHA

/* SNMPv3 privacy protocol */
#define SNMP_MGMT_PRIV_PROTO_NONE   VTSS_APPL_SNMP_PRIV_PROTOCOL_NONE
#define SNMP_MGMT_PRIV_PROTO_DES    VTSS_APPL_SNMP_PRIV_PROTOCOL_DES
#define SNMP_MGMT_PRIV_PROTO_AES    VTSS_APPL_SNMP_PRIV_PROTOCOL_AES

/* SNMPv3 communities entry */
typedef struct {
    ulong       idx;
    BOOL        valid;
    char        community[SNMPV3_MAX_NAME_LEN + 1];   /* key */
    mesa_ip_network_t sip;
    char        encryptedSecret[VTSS_APPL_AUTH_KEY_LEN];
    char        communitySecret[VTSS_APPL_AUTH_UNENCRYPTED_KEY_LEN];
    ulong       storage_type;
    ulong       status;
} snmpv3_communities_conf_t;

/* SNMPv3 users entry */
typedef struct {
    ulong   idx;
    BOOL    valid;
    uchar   engineid[SNMPV3_MAX_ENGINE_ID_LEN];   /* key */
    u32     engineid_len;                         /* key */
    char    user_name[SNMPV3_MAX_NAME_LEN + 1];   /* key */
    u32     security_level;
    u32     auth_protocol;
    char    auth_password[SNMPV3_MAX_SHA_PASSWORD_LEN + 1];
    u32     auth_password_len;
    BOOL    auth_password_encrypted;
    u32     priv_protocol;
    char    priv_password[SNMPV3_MAX_DES_PASSWORD_LEN + 1];
    BOOL    priv_password_encrypted;
    u32     priv_password_len;
    ulong   storage_type;
    ulong   status;
} snmpv3_users_conf_t;

/* SNMPv3 groups entry */
typedef struct {
    BOOL    valid;
    ulong   security_model;                            /* key */
    char    security_name[SNMPV3_MAX_NAME_LEN + 1];    /* key */
    char    group_name[SNMPV3_MAX_NAME_LEN + 1];
    ulong   storage_type;
    ulong   status;
} snmpv3_groups_conf_t;

/* SNMPv3 views entry */
typedef struct {
    BOOL    valid;
    char    view_name[SNMPV3_MAX_NAME_LEN + 1];   /* key */
    char    subtree[VTSS_APPL_SNMP_MAX_SUBTREE_STR_LEN + 1]; /*key */
    u32     view_type;
    ulong   storage_type;
    ulong   status;
} snmpv3_views_conf_t;

/* SNMPv3 accesses entry */
typedef struct {
    BOOL    valid;
    char    group_name[SNMPV3_MAX_NAME_LEN + 1];       /* key */
    char    context_prefix[SNMPV3_MAX_NAME_LEN + 1];   /* key */
    ulong   security_model;                            /* key */
    u32     security_level;                            /* key */
    ulong   context_match;
    char    read_view_name[SNMPV3_MAX_NAME_LEN + 1];
    char    write_view_name[SNMPV3_MAX_NAME_LEN + 1];
    char    notify_view_name[SNMPV3_MAX_NAME_LEN + 1];
    ulong   storage_type;
    ulong   status;
} snmpv3_accesses_conf_t;

/* check is valod engine ID */
BOOL snmpv3_is_valid_engineid(uchar *engineid, ulong engineid_len);

/* check is SNMP admin string format */
BOOL snmpv3_is_admin_string(const char *str);

int snmpv3_communities_conf_changed(snmpv3_communities_conf_t *old, snmpv3_communities_conf_t *new_conf);

/* check SNMPv3 communities configuration */
mesa_rc snmpv3_mgmt_communities_conf_check(snmpv3_communities_conf_t *conf);

/* Get SNMPv3 communities configuration,
fill community = SNMPV3_CONF_ACESS_GETFIRST will get first entry */
mesa_rc snmpv3_mgmt_communities_conf_get(snmpv3_communities_conf_t *conf, BOOL next);

/* Set SNMPv3 communities configuration,
fill valid = 0 or status = SNMP_MGMT_ROW_DESTROY will destroy the entry */
mesa_rc snmpv3_mgmt_communities_conf_set(snmpv3_communities_conf_t *conf);

void snmpv3_default_communities_get(u32 *conf_num, snmpv3_communities_conf_t *conf);

/* check SNMPv3 users configuration */
mesa_rc snmpv3_mgmt_users_conf_check(snmpv3_users_conf_t *conf);

int snmpv3_users_conf_changed(snmpv3_users_conf_t *old, snmpv3_users_conf_t *new_conf);

/* Get SNMPv3 users configuration,
fill user_name = SNMPV3_CONF_ACESS_GETFIRST will get first entry */
mesa_rc snmpv3_mgmt_users_conf_get(snmpv3_users_conf_t *conf, BOOL next);

void snmpv3_default_users_get(u32 *conf_num, snmpv3_users_conf_t *conf);

/* Set SNMPv3 users configuration,
fill valid = 0 or status = SNMP_MGMT_ROW_DESTROY will destroy the entry */
mesa_rc snmpv3_mgmt_users_conf_set(snmpv3_users_conf_t *conf);
char *password_as_hex_string(const char *password, u32 len, char *password_hex, u8 max_len);

/* check SNMPv3 groups configuration */
mesa_rc snmpv3_mgmt_groups_conf_check(snmpv3_groups_conf_t *conf);

int snmpv3_groups_conf_changed(snmpv3_groups_conf_t *old, snmpv3_groups_conf_t *new_conf);

/* Get SNMPv3 groups configuration,
fill security_name = SNMPV3_CONF_ACESS_GETFIRST will get first entry */
mesa_rc snmpv3_mgmt_groups_conf_get(snmpv3_groups_conf_t *conf, BOOL next);

/* Get SNMPv3 groups defaults */
void snmpv3_default_groups_get(u32 *conf_num, snmpv3_groups_conf_t *conf);

/* Set SNMPv3 groups configuration,
fill valid = 0 or status = SNMP_MGMT_ROW_DESTROY will destroy the entry */
mesa_rc snmpv3_mgmt_groups_conf_set(snmpv3_groups_conf_t *conf);

int snmpv3_views_conf_changed(snmpv3_views_conf_t *old, snmpv3_views_conf_t *new_conf);
/* check SNMPv3 views configuration */
mesa_rc snmpv3_mgmt_views_conf_check(snmpv3_views_conf_t *conf);

/* Get SNMPv3 views configuration,
fill view_name = SNMPV3_CONF_ACESS_GETFIRST will get first entry */
mesa_rc snmpv3_mgmt_views_conf_get(snmpv3_views_conf_t *conf, BOOL next);

/* Set SNMPv3 views configuration,
fill valid = 0 or status = SNMP_MGMT_ROW_DESTROY will destroy the entry */
mesa_rc snmpv3_mgmt_views_conf_set(snmpv3_views_conf_t *conf);

void snmpv3_default_views_get(u32 *conf_num, snmpv3_views_conf_t *conf);

int snmpv3_accesses_conf_changed(snmpv3_accesses_conf_t *old, snmpv3_accesses_conf_t *new_conf);
/* check SNMPv3 accesses configuration */
mesa_rc snmpv3_mgmt_accesses_conf_check(snmpv3_accesses_conf_t *conf);

/* Get SNMPv3 accesses configuration,
fill group_name = SNMPV3_CONF_ACESS_GETFIRST will get first entry */
mesa_rc snmpv3_mgmt_accesses_conf_get(snmpv3_accesses_conf_t *conf, BOOL next);

/* Set SNMPv3 accesses configuration,
fill valid = 0 or status = SNMP_MGMT_ROW_DESTROY will destroy the entry */
mesa_rc snmpv3_mgmt_accesses_conf_set(snmpv3_accesses_conf_t *conf);

void snmpv3_default_accesses_get(u32 *conf_num, snmpv3_accesses_conf_t *conf);

/* Post configure if using NTP */
void snmpv3_mgmt_ntp_post_conf(void);

BOOL str2oid(const char *name, oid *oidSubTree,  u32 *oid_len, u8 *oid_mask, u32 *oid_mask_len);

/**
  * \brief Get trap configuration entry
  *
  * \param trap_entry   [IN] trap_conf_name: Name of the trap configuration
  * \return
  *    VTSS_RC_OK indicates it is successful, otherwise is failed.
  */
mesa_rc trap_mgmt_conf_get (vtss_trap_entry_t  *trap_entry);

/**
  * \brief Get next trap configuration entry
  *
  * \param trap_entry   [INOUT] trap_conf_name: Name of the trap configuration
  * \return
  *    VTSS_RC_OK indicates it is successful, otherwise is failed.
  */
mesa_rc trap_mgmt_conf_get_next (vtss_trap_entry_t  *trap_entry);

/**
  * \brief Set trap configuration entry
  *
  * \param trap_entry   [IN] : The trap configuration
  * \return
  *    VTSS_RC_OK indicates it is successful, otherwise is failed.
  */
mesa_rc trap_mgmt_conf_set (vtss_trap_entry_t  *trap_entry);

/**
  * \brief Get trap source entry
  *
  * \param trap_source  [IN] source_name: Name of the trap source
  * \return
  *    VTSS_RC_OK indicates it is successful, otherwise is failed.
  */
mesa_rc trap_mgmt_source_get (vtss_trap_source_t *trap_source);

/**
  * \brief Get next trap source entry
  *
  * \param trap_source  [INOUT] source_name: Name of the trap source
  * \return
  *    VTSS_RC_OK indicates it is successful, otherwise is failed.
  */
mesa_rc trap_mgmt_source_get_next (vtss_trap_source_t *trap_source);

/**
  * \brief Set trap source entry
  *
  * \param trap_source  [IN] : The trap source
  * \param trap_filter  [IN] : The trap filter
  * \param filter_id    [IN] : ID of trap filter; -1: Skip ID
  * \return
  *    VTSS_RC_OK indicates it is successful, otherwise is failed.
  */
mesa_rc trap_mgmt_source_set(vtss_trap_source_t *trap_source, vtss_trap_filter_item_t *trap_filter, int filter_id);

/**
  * \brief Get trap default configuration entry
  *
  * \param trap_entry   [OUT] : The trap configuration
  */
void trap_mgmt_conf_default_get(vtss_trap_entry_t  *trap_entry);

/**
  * \brief Send SNMP vars trap
  *
  * \param entry         [IN]: the event OID and variable binding
  *
 */
void snmp_send_vars_trap(snmp_vars_trap_entry_t *entry);

/**
  * \brief Send dying gasp trap
  *
  * \param specific [IN]:none
  *
 */
void vtss_snmp_dying_gasp_trap_send_handler(void);

void vtss_debug_send_trap();
bool snmp_module_enabled();


// Copy from an uint64_t to a struct counter64
void vtss_snmp_u64_to_counter64(struct counter64 *c64, uint64_t uint64);
#ifdef __cplusplus
}
#endif
#endif /* _VTSS_SNMP_API_H_ */

