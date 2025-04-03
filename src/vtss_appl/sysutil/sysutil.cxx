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

#include <time.h>
#include "main.h"
#include "conf_api.h"
#include "msg_api.h"
#include "critd_api.h"
#include "misc_api.h"
#include "sysutil_api.h"
#include "monitor_api.h"
#include "control_api.h"
#include "vtss/basics/expose/snmp/iterator-compose-static-range.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-range.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-N.hxx"
#include "vtss_common_iterator.hxx"
#include <zlib.h>   /* For gzopen() and friends */
#ifdef VTSS_SW_OPTION_LLDP
#include "lldp_api.h"
#endif /* VTSS_SW_OPTION_LLDP */
#ifdef VTSS_SW_OPTION_UDLD
#include "udld_api.h"
#endif /* VTSS_SW_OPTION_UDLD */
#include "sysutil_trace.h"
#ifdef VTSS_SW_OPTION_AUTH
#include "vtss_auth_api.h"
#endif /* VTSS_SW_OPTION_AUTH */
#ifdef VTSS_SW_OPTION_WEB
#include "web_api.h"
#endif /* VTSS_SW_OPTION_WEB */

#ifdef VTSS_SW_OPTION_ICLI /* CP, 04/08/2013 12:54, Bugzilla#11469 */
#include "icli_api.h"
#endif
#if defined(VTSS_SW_OPTION_ICFG) && (!defined(VTSS_SW_OPTION_USERS) || defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED))
#include "sysutil_icfg.h"
#endif

#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
#include "vtss_eth_link_oam_api.h"
#endif

#include "standalone_api.h"   //topo_usid2isid(), topo_isid2usid()

#include "led_api.h"    //led_front_led_state(), led_front_led_state_txt()

#include "vtss_timer_api.h"      /* For vtss_timer_XXX() functions */

#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
#include "tmp_43x_api.h"         /* For temperature monitor functions */

/* JSON notification */
#include "vtss/basics/expose/table-status.hxx"  // For vtss::expose::TableStatus
#include "vtss/basics/memcmp-operator.hxx"      // For VTSS_BASICS_MEMCMP_OPERATOR
#endif /* defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED) */

static void cpu_util_init();  /* pre-declaration */
static void cpu_util_start();

#define MAX_SAMPLES (10*10)    // 10 seconds, 100ms intervals

typedef struct {
    int busy;
    int idle;
} load_sample_t;

static load_sample_t samples[MAX_SAMPLES], last;

typedef struct {
    unsigned int usr, nic, sys, idle;
    unsigned int iowait, irq, softirq;
    unsigned int busy;
} jiffy_counts_t;

typedef struct {
    critd_t crit;
    vtss::Timer timer;  /* Timer for cpu util */
    int s_index;
} cpu_util_t;

/* Global structure */
static cpu_util_t cpu;

/* Trace registration. Initialized by system_init() */
#define CPU_CRIT_ENTER() critd_enter(&cpu.crit, __FILE__, __LINE__)
#define CPU_CRIT_EXIT()  critd_exit( &cpu.crit, __FILE__, __LINE__)

/* ================================================================= *
 *  system stack messages
 * ================================================================= */
/* Ssystem request message timeout in seconds */
#define SYSTEM_REQ_TIMEOUT              5

/* System counters timer */
#define SYSTEM_LED_GET_STATUE_TIMER     1000

/* system messages IDs */
typedef enum {
    SYSTEM_MSG_ID_CONF_SET_REQ,         /* System configuration set request (no reply) */
    SYSTEM_MSG_ID_LED_STATE_GET_REQ,    /* System LED state get request */
    SYSTEM_MSG_ID_LED_STATE_GET_REP,    /* System LED state get reply */
    SYSTEM_MSG_ID_LED_STATE_CLEAR       /* System LED state clear (no reply) */
} system_msg_id_t;

/* Request message */
typedef struct {
    /* Message ID */
    system_msg_id_t         msg_id;
    system_conf_t           system_conf;
    BOOL                    system_led_state_clear_all;
    led_front_led_state_t   system_led_state_clear;
} system_msg_req_t;

/* Reply message */
typedef struct {
    /* Message ID */
    system_msg_id_t         msg_id;
    led_front_led_state_t   system_led_state;
} system_msg_rep_t;

/* system message buffer */
typedef struct {
    vtss_sem_t *sem; /* Semaphore */
    void       *msg; /* Message */
} system_msg_buf_t;

/* ================================================================= *
 *  system global structure
 * ================================================================= */

/* system global structure */
typedef struct {
    critd_t                 crit;
    system_conf_t           system_conf;
    led_front_led_state_t   system_led_state[VTSS_ISID_CNT];        /* System LED state  */
    vtss_mtimer_t           system_led_state_timer[VTSS_ISID_CNT];  /* System LED state timer */
    vtss_flag_t             system_led_state_flags;                 /* System LED state flag */
#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
    critd_t                 tm_crit;
    vtss_appl_sysutil_tm_config_t tm_config[VTSS_APPL_SYSUTIL_TEMP_MONITOR_SENSOR_CNT]; /* temperature monitor config db */
    vtss_appl_sysutil_tm_status_t tm_status[VTSS_APPL_SYSUTIL_TEMP_MONITOR_SENSOR_CNT]; /* temperature monitor status db */
    int tm_debug[VTSS_APPL_SYSUTIL_TEMP_MONITOR_SENSOR_CNT];                            /* temperature monitor debug db */
#endif

    /* Request buffer and semaphore */
    struct {
        vtss_sem_t       sem;
        system_msg_req_t msg;
    } request;

    /* Reply buffer and semaphore */
    struct {
        vtss_sem_t       sem;
        system_msg_rep_t msg;
    } reply;
} system_global_t;

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Global structure */
static system_global_t system_global;
static char tz_display[13];
static char system_descr[256];

#if defined(VTSS_SW_OPTION_PSU)
// PSU thread variables
static vtss_handle_t SYSUTIL_psu_thread_handle;
static vtss_thread_t SYSUTIL_psu_thread_state;
#endif /* VTSS_SW_OPTION_PSU */

#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
// Temperature monitor thread variables
static vtss_handle_t SYSUTIL_tm_thread_handle;
static vtss_thread_t SYSUTIL_tm_thread_state;
#endif

/* Trace registration. Initialized by system_init() */
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "system", "system (configuration)"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_SYSUTIL_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_SYSUTIL_GRP_NETLINK] = {
        "netlink",
        "Netlink",
        VTSS_TRACE_LVL_WARNING
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define SYSTEM_CRIT_ENTER() critd_enter(&system_global.crit, __FILE__, __LINE__)
#define SYSTEM_CRIT_EXIT()  critd_exit( &system_global.crit, __FILE__, __LINE__)

#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
#define SYSTEM_TM_CRIT_ENTER() critd_enter(&system_global.tm_crit, __FILE__, __LINE__)
#define SYSTEM_TM_CRIT_EXIT()  critd_exit( &system_global.tm_crit, __FILE__, __LINE__)
#endif

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/

#ifndef VTSS_SW_OPTION_USERS
/* Determine if password has changed */
static int system_password_changed(system_conf_t *old, system_conf_t *new_)
{
    return (strcmp(new_->sys_passwd, old->sys_passwd));
}
#endif /* VTSS_SW_OPTION_USERS */

/* Determine if system configuration has changed */
static int system_conf_changed(system_conf_t *old, system_conf_t *new_)
{
    return (strcmp(new_->sys_contact, old->sys_contact) ||
            strcmp(new_->sys_name, old->sys_name) ||
            strcmp(new_->sys_location, old->sys_location) ||
#ifndef VTSS_SW_OPTION_USERS
            strcmp(new_->sys_passwd, old->sys_passwd) ||
#endif /* VTSS_SW_OPTION_USERS */
            new_->sys_services != old->sys_services ||
            new_->tz_off != old->tz_off);
}

/* Calculate a hash value for checking the encrypted password format */
static void vtss_sys_password_checking_code_calculate(const char *username, const char *encrypted_pwd, char *salt_padding, char *output)
{
    char    hash_key[sizeof(VTSS_SYS_PASSWORD_MAGIC_STR) + VTSS_SYS_INPUT_PASSWD_LEN + VTSS_SYS_PASSWD_SALT_PADDING_LEN * 2 + 8 + 3]; // 8 bytes for crc str, 2 bytes for "::", 1 byte for end of string
    u8      hash_value[VTSS_SYS_HASH_SHA1_LEN];
    u32     idx, crc = 0;

    // The checking code format: SHA1[password_magic_str:username:pwd_hash_crc:salt_padding]
    for (idx = 0; idx < VTSS_SYS_HASH_DIGEST_LEN * 2; idx++) {
        crc = vtss_crc32_accumulate(crc, (u8 *)&encrypted_pwd[idx], 1);
    }
    sprintf(hash_key, "%s:%s:%x:%s", VTSS_SYS_PASSWORD_MAGIC_STR, username, crc, salt_padding);
    (void)vtss_sha1_calc((u8 *)hash_key, (u32)strlen(hash_key), hash_value);
    for (idx = 0; idx < VTSS_SYS_HASH_SHA1_LEN; idx++) {
        sprintf(&output[idx * 2], "%02x", (u8)hash_value[idx]);
    }
}

/* Encode the password.
 * Parameter:
 * username      [IN]  - username.
 * clear_pwd     [IN]  - Clear password.
 * salt_padding  [IN]  - The salt for the hash function.
 * encrypted_pwd [OUT] - The encrypted password.
 *
 * Notes that the hash process only affect when the input parameter 'conf->encrypted == FALSE',
 * and it will generate a random salt padding when the input parameter 'salt_padding == NULL'.
 */
void sysutil_password_encode(const char username[VTSS_SYS_USERNAME_LEN], const char clear_pwd[VTSS_SYS_PASSWD_LEN], const char salt_padding[VTSS_SYS_PASSWD_SALT_PADDING_LEN * 2 + 1], char encrypted_pwd[VTSS_SYS_PASSWD_ENCRYPTED_LEN])
{
    char    hash_key[VTSS_SYS_USERNAME_LEN + VTSS_SYS_PASSWD_ENCRYPTED_LEN + 2 + 1]; // 2 bytes for "::", 1 byte for end of string
    char    b64_username[VTSS_SYS_BASE64_ARRAY_SIZE(VTSS_SYS_INPUT_USERNAME_LEN)], b64_pwd[VTSS_SYS_BASE64_ARRAY_SIZE(VTSS_SYS_INPUT_PASSWD_LEN)];
    u8      hash_value[VTSS_SYS_HASH_DIGEST_LEN];
    char    random_salt_padding[VTSS_SYS_PASSWD_SALT_PADDING_LEN * 2 + 1];
    u32     idx;

    // Refer to http://www.codeproject.com/Articles/704865/Salted-Password-Hashing-Doing-it-Right
    // The hash key format: [base64<username>:base64<password>:<random_salt_padding>]
    // The encrypted string format: [password_hash_value][salt_padding][checking_code]

    // Generate a random salt padding (if needed)
    if (salt_padding == NULL) {
        (void) vtss_generate_random_hex_str(VTSS_SYS_PASSWD_SALT_PADDING_LEN, random_salt_padding);
    } else {
        misc_strncpyz(random_salt_padding, salt_padding, sizeof(random_salt_padding));
    }

    // Calculate password hash value.
    // Fill [password_hash_value]
    memset(hash_key, 0, sizeof(hash_key));
    memset(b64_username, 0, sizeof(b64_username));
    memset(b64_pwd, 0, sizeof(b64_pwd));
    (void)vtss_httpd_base64_encode(b64_username, sizeof(b64_username), username, strlen(username));
    (void)vtss_httpd_base64_encode(b64_pwd, sizeof(b64_pwd), clear_pwd, strlen(clear_pwd));
    sprintf(hash_key, "%s:%s:%s", b64_username, b64_pwd, random_salt_padding);
#if VTSS_SYS_HASH_DIGEST_LEN == VTSS_SYS_HASH_SHA256_LEN
    (void)vtss_sha256_calc((u8 *)hash_key, (u32)strlen(hash_key), hash_value);
#elif VTSS_SYS_HASH_DIGEST_LEN == VTSS_SYS_HASH_SHA1_LEN
    (void)vtss_sha1_calc((u8 *)hash_key, (u32)strlen(hash_key), hash_value);
#else
#error "Unsupported HASH algorithm"
#endif
    for (idx = 0; idx < VTSS_SYS_HASH_DIGEST_LEN; idx++) {
        sprintf(&encrypted_pwd[idx * 2], "%02x", (u8)hash_value[idx]);
    }

    // Fill [salt_padding]
    sprintf(&encrypted_pwd[VTSS_SYS_HASH_DIGEST_LEN * 2], "%s", random_salt_padding);

    // Calculate calculate format checking code
    // Fill [checking_code]
    vtss_sys_password_checking_code_calculate(username, encrypted_pwd, random_salt_padding, &encrypted_pwd[(VTSS_SYS_HASH_DIGEST_LEN + VTSS_SYS_PASSWD_SALT_PADDING_LEN) * 2]);

    // Fill end of string
    encrypted_pwd[VTSS_SYS_PASSWD_ENCRYPTED_LEN - 1] = '\0';
}

/* A compare function with slower process time.
 * It is to make the compare function slower, so that the brute-force attacks
 * are too slow to be worthwhile. The process speed is slow enough for the
 * attacker, but still fast enough for the normal comparing.
 *
 * Return TRUE when equal, FALSE otherwise.
*/
BOOL sysutil_slow_equals(u8 *a, u32 len_a, u8 *b, u32 len_b)
{
    u32 idx;
    unsigned int diff = (unsigned int)len_a ^ (unsigned int)len_b;

    for (idx = 0; idx < len_a && idx < len_b; idx++) {
        diff |= (unsigned int)(a[idx] ^ b[idx]);
    }

    return (diff == 0) ? TRUE : FALSE;
}

/* Verify the encrypted password format is valid.
 *
 * Return TRUE when equal, FALSE otherwise.
 */
BOOL sysutil_encrypted_password_fomat_is_valid(const char username[VTSS_SYS_USERNAME_LEN], const char encrypted_password[VTSS_SYS_PASSWD_ENCRYPTED_LEN])
{
    char    salt_padding[VTSS_SYS_PASSWD_SALT_PADDING_LEN * 2 + 1];
    char    hash_str[VTSS_SYS_HASH_SHA1_LEN * 2 + 1];
    u32     idx;

    if (strlen(encrypted_password) != (VTSS_SYS_PASSWD_ENCRYPTED_LEN - 1)) {
        return FALSE;
    }

    // Check format (must hex characters)
    for (idx = 0; idx < (VTSS_SYS_PASSWD_ENCRYPTED_LEN - 1); idx++) {
        const char *c = &encrypted_password[idx];
        if (!((*c >= '0' && *c <= '9') || (*c >= 'A' && *c <= 'F') || (*c >= 'a' && *c <= 'f'))) {
            return FALSE;
        }
    }

    // Retrieve salt padding and calculate format checking code
    misc_strncpyz(salt_padding, &encrypted_password[VTSS_SYS_HASH_DIGEST_LEN * 2], sizeof(salt_padding));
    vtss_sys_password_checking_code_calculate(username, encrypted_password, salt_padding, hash_str);

    // Compare to local database
    if (sysutil_slow_equals((u8 *)hash_str, VTSS_SYS_HASH_SHA1_LEN * 2, (u8 *)&encrypted_password[(VTSS_SYS_HASH_DIGEST_LEN + VTSS_SYS_PASSWD_SALT_PADDING_LEN) * 2], VTSS_SYS_HASH_SHA1_LEN * 2)) {
        // The password are matched
        return TRUE;
    }

    return FALSE;
}

#ifndef VTSS_SW_OPTION_USERS
/* Encode the password */
void system_password_encode(const char clear_pwd[VTSS_SYS_PASSWD_LEN], char encrypted_pwd[VTSS_SYS_PASSWD_ENCRYPTED_LEN])
{
    char username[VTSS_SYS_USERNAME_LEN];

    misc_strncpyz(username, VTSS_SYS_ADMIN_NAME, sizeof(username));
    sysutil_password_encode(username, clear_pwd, NULL, encrypted_pwd);
}

/* Verify system password  */
BOOL system_clear_password_verify(const char clear_pwd[VTSS_SYS_PASSWD_LEN])
{
    char username[VTSS_SYS_USERNAME_LEN];
    char encrypted_pwd[VTSS_SYS_PASSWD_ENCRYPTED_LEN];
    char salt_padding[VTSS_SYS_PASSWD_SALT_PADDING_LEN * 2 + 1];
    const char *system_pwd = system_get_encrypted_passwd();

    misc_strncpyz(username, VTSS_SYS_ADMIN_NAME, sizeof(username));
    memset(encrypted_pwd, 0, sizeof(encrypted_pwd));
    misc_strncpyz(salt_padding, &system_pwd[VTSS_SYS_HASH_DIGEST_LEN * 2], sizeof(salt_padding));
    sysutil_password_encode(username, clear_pwd, salt_padding, encrypted_pwd);
    if (sysutil_slow_equals((u8 *)encrypted_pwd, VTSS_SYS_PASSWD_ENCRYPTED_LEN - 1, (u8 *)system_pwd, VTSS_SYS_PASSWD_ENCRYPTED_LEN - 1)) {
        // The password are matched
        return TRUE;
    }

    return FALSE;
}
#endif /* VTSS_SW_OPTION_USERS */

/* Set system defaults */
static void system_default_set(system_conf_t *conf)
{
#ifndef VTSS_SW_OPTION_USERS
    char default_clear_pwd[VTSS_SYS_PASSWD_LEN];
#endif /* VTSS_SW_OPTION_USERS */

    conf->sys_contact[0] = '\0'; /* Empty system contact by default */

#ifdef VTSS_SW_OPTION_ICLI /* CP, 04/08/2013 12:54, Bugzilla#11469 */
    strcpy(conf->sys_name, ICLI_DEFAULT_DEVICE_NAME);
#else
    conf->sys_name[0] = '\0'; /* Empty system name by default */
#endif

    conf->sys_location[0] = '\0'; /* Empty system location by default */
#ifndef VTSS_SW_OPTION_USERS
    strcpy(default_clear_pwd, VTSS_SYS_ADMIN_PASSWORD);
    system_password_encode(default_clear_pwd, conf->sys_passwd);
#endif /* VTSS_SW_OPTION_USERS */
    conf->sys_services = SYSTEM_SERVICES_PHYSICAL | SYSTEM_SERVICES_DATALINK;
    conf->tz_off = 0x0;
}

static void system_update_tz_display(int tz_off)
{
    (void)snprintf(tz_display, sizeof(tz_display) - 1, "%c%02d%02d",
                   tz_off < 0 ? '-' : '+',
                   abs(tz_off) / 60,
                   abs(tz_off) % 60);
}

static void _system_set_config(system_conf_t *conf)
{
    system_update_tz_display(conf->tz_off);
}

int                             /* TZ offset in minutes */
system_get_tz_off(void)
{
    int  tz_off;

    /* This is is not protected by SYSTEM_CRIT_ENTER/EXIT, since this would create
       a deadlock for trace statements inside critical regions of this module */
    tz_off = system_global.system_conf.tz_off;

    return tz_off;
}

const char*                     /* TZ offset for display per ISO8601: +-hhmm */
system_get_tz_display(void)
{
    return tz_display;
}

char *system_get_descr(void)
{
    return system_descr;
}


/* check string is administratively name */
BOOL system_name_is_administratively(char string[VTSS_SYS_STRING_LEN])
{
    ulong i;
    size_t len = strlen(string);

    /* allow null string */
    if (string[0] == '\0') {
        T_D("String is NULL");
        return TRUE;
    }

    /* The first or last character must not be a minus sign */
    if (string[0] == '-' || string[len-1] == '-') {
        return FALSE;
    }

    /* The first character must be an alpha character */
    if (!((string[0] >= 'A' && string[0] <= 'Z') ||
            (string[0] >= 'a' && string[0] <= 'z'))) {
        return FALSE;
    }

    for (i = 0; i < len; i++) {
        /* No blank or space characters are permitted as part of a name */
        if (string[i] == '\0') {
            return FALSE;
        }

        /* A name is a text string drawn from the alphabet (A-Za-z),
           digits (0-9), minus sign (-) */
        if (!((string[i] >= '0' && string[i] <= '9') ||
                (string[i] >= 'A' && string[i] <= 'Z') ||
                (string[i] >= 'a' && string[i] <= 'z') ||
                (string[i] == '-'))) {
            return FALSE;
        }
    }
    T_D("Name is valid:%s", string);
    return TRUE;
}

#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
/* JSON notification */
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_sysutil_tm_status_t);
vtss::expose::TableStatus <
    vtss::expose::ParamKey<vtss_appl_sysutil_tm_sensor_type_t>,
    vtss::expose::ParamVal<vtss_appl_sysutil_tm_status_t *>
> tm_status_event_update("tm_status_event_update", VTSS_MODULE_ID_SYSTEM);
static void system_send_temperature_notification(
    vtss_appl_sysutil_tm_sensor_type_t sensor,
    vtss_appl_sysutil_tm_status_t *status);
#endif
/****************************************************************************/
/*  Stack/switch functions                                                  */
/****************************************************************************/
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
static const char *system_msg_id_txt(system_msg_id_t msg_id)
{
    const char *txt;

    switch (msg_id) {
    case SYSTEM_MSG_ID_CONF_SET_REQ:
        txt = "SYSTEM_CONF_SET_REQ";
        break;
    default:
        txt = "?";
        break;
    }
    return txt;
}
#endif /* VTSS_TRACE_LVL_DEBUG */

/* Allocate request buffer */
static system_msg_req_t *system_msg_req_alloc(system_msg_buf_t *buf, system_msg_id_t msg_id)
{
    system_msg_req_t *msg = &system_global.request.msg;

    buf->sem = &system_global.request.sem;
    buf->msg = msg;
    vtss_sem_wait(buf->sem);
    msg->msg_id = msg_id;
    return msg;
}

/* Allocate request buffer */
static system_msg_rep_t *system_msg_rep_alloc(system_msg_buf_t *buf, system_msg_id_t msg_id)
{
    system_msg_rep_t *msg = &system_global.reply.msg;

    buf->sem = &system_global.reply.sem;
    buf->msg = msg;
    vtss_sem_wait(buf->sem);
    msg->msg_id = msg_id;
    return msg;
}

static void system_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
    system_msg_id_t msg_id = *(system_msg_id_t *)msg;
#endif /* VTSS_TRACE_LVL_DEBUG */

    T_D("msg_id: %d, %s", msg_id, system_msg_id_txt(msg_id));
    vtss_sem_post((vtss_sem_t *)contxt);
}

static void system_msg_tx(system_msg_buf_t *buf, vtss_isid_t isid, size_t len)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
    system_msg_id_t msg_id = *(system_msg_id_t *)buf->msg;
#endif /* VTSS_TRACE_LVL_DEBUG */

    T_D("msg_id: %d, %s, len: %zd, isid: %d", msg_id, system_msg_id_txt(msg_id), len, isid);
    msg_tx_adv(buf->sem, system_msg_tx_done, MSG_TX_OPT_DONT_FREE,
               VTSS_MODULE_ID_SYSTEM, isid, buf->msg, len);
}

static BOOL system_msg_rx(void *contxt, const void *const rx_msg, const size_t len, const vtss_module_id_t modid, const u32 isid)
{
    system_msg_req_t  *msg;
    system_msg_id_t msg_id = *(system_msg_id_t *)rx_msg;

    T_D("msg_id: %d, %s, len: %zd, isid: %u", msg_id, system_msg_id_txt(msg_id), len, isid);

    msg = (system_msg_req_t *)rx_msg;

    switch (msg_id) {
    case SYSTEM_MSG_ID_CONF_SET_REQ: {
        _system_set_config(&msg->system_conf);
        break;
    }
    case SYSTEM_MSG_ID_LED_STATE_GET_REQ: {
        system_msg_buf_t buf;
        system_msg_rep_t *msg;

        msg = system_msg_rep_alloc(&buf, SYSTEM_MSG_ID_LED_STATE_GET_REP);
        led_front_led_state_get(&msg->system_led_state);
        system_msg_tx(&buf, isid, sizeof(system_msg_rep_t));
        break;
    }
    case SYSTEM_MSG_ID_LED_STATE_GET_REP: {
        system_msg_rep_t *msg;

        msg = (system_msg_rep_t *)rx_msg;
        if (msg_switch_is_primary()) {
            SYSTEM_CRIT_ENTER();
            system_global.system_led_state[isid - VTSS_ISID_START] = msg->system_led_state;
            VTSS_MTIMER_START(&system_global.system_led_state_timer[isid - VTSS_ISID_START], SYSTEM_LED_GET_STATUE_TIMER);
            vtss_flag_setbits(&system_global.system_led_state_flags, 1 << isid);
            SYSTEM_CRIT_EXIT();
        }
        break;
    case SYSTEM_MSG_ID_LED_STATE_CLEAR: {
        system_msg_req_t *msg;

        msg = (system_msg_req_t *)rx_msg;
        if (msg->system_led_state_clear_all) {
            led_front_led_state_clear_all();
        }
            led_front_led_state_clear(msg->system_led_state_clear);
        }
        break;
    }
    default:
        T_W("unknown message ID: %d", msg_id);
        break;
    }
    return TRUE;
}

static mesa_rc system_stack_register(void)
{
    msg_rx_filter_t filter;

    memset(&filter, 0, sizeof(filter));
    filter.cb = system_msg_rx;
    filter.modid = VTSS_MODULE_ID_SYSTEM;
    return msg_rx_filter_register(&filter);
}

/* Set stack system configuration */
static mesa_rc system_stack_conf_set(vtss_isid_t isid_add)
{
    system_msg_req_t  *msg;
    system_msg_buf_t  buf;
    vtss_isid_t     isid;

    T_D("enter, isid_add: %d", isid_add);
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if ((isid_add != VTSS_ISID_GLOBAL && isid_add != isid) ||
                !msg_switch_exists(isid)) {
            continue;
        }
        msg = system_msg_req_alloc(&buf, SYSTEM_MSG_ID_CONF_SET_REQ);
        SYSTEM_CRIT_ENTER();
        msg->system_conf = system_global.system_conf;
        SYSTEM_CRIT_EXIT();
        system_msg_tx(&buf, isid, sizeof(system_msg_req_t));
    }

    T_D("exit, isid_add: %d", isid_add);
    return VTSS_RC_OK;
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/* Get system configuration */
extern "C"
mesa_rc system_get_config(system_conf_t *conf)
{
    T_D("enter");
    SYSTEM_CRIT_ENTER();
    *conf = system_global.system_conf;
    SYSTEM_CRIT_EXIT();
    T_D("exit");

    return VTSS_RC_OK;
}

/* Set system configuration */
mesa_rc system_set_config(system_conf_t *conf)
{
    mesa_rc rc      = VTSS_RC_OK;
    int     changed = 0;
#ifndef VTSS_SW_OPTION_USERS
    int     passwd_changed = 0;
#endif /* VTSS_SW_OPTION_USERS */

    T_D("enter");

    SYSTEM_CRIT_ENTER();
    if (msg_switch_is_primary()) {
        if (system_name_is_administratively(conf->sys_name)) {
            changed = system_conf_changed(&system_global.system_conf, conf);
#ifndef VTSS_SW_OPTION_USERS
            passwd_changed = system_password_changed(&system_global.system_conf, conf);
#endif /* VTSS_SW_OPTION_USERS */
            system_global.system_conf = *conf;
        } else {
            rc = SYSTEM_ERROR_NOT_ADMINISTRATIVELY_NAME;
        }
    } else {
        T_W("not primary switch");
        rc = SYSTEM_ERROR_STACK_STATE;
    }
    SYSTEM_CRIT_EXIT();

    if (changed) {
        /* Activate changed configuration */
        rc = system_stack_conf_set(VTSS_ISID_GLOBAL);
#ifdef VTSS_SW_OPTION_LLDP
        lldp_something_has_changed();
#endif /* VTSS_SW_OPTION_LLDP */

#ifdef VTSS_SW_OPTION_UDLD
        udld_something_has_changed();
#endif /* VTSS_SW_OPTION_UDLD */
    }
#if !defined(VTSS_SW_OPTION_USERS)
#if defined(VTSS_SW_OPTION_AUTH)
    if (passwd_changed) {
        vtss_auth_mgmt_httpd_cache_expire(); // Clear httpd cache
    }
#else
    // Avoid warnings
    passwd_changed = passwd_changed;
#endif /* defined(VTSS_SW_OPTION_AUTH) */
#endif /* !defined(VTSS_SW_OPTION_USERS) */
    T_D("exit");

#ifdef VTSS_SW_OPTION_ICLI /* CP, 04/08/2013 12:54, Bugzilla#11469 */
    if ( rc == VTSS_RC_OK ) {
        if ( icli_dev_name_set(conf->sys_name) != ICLI_RC_OK ) {
            T_E("%% Fail to set device name to ICLI engine.\n\n");
        }
    }
#endif

    return rc;
}

#ifndef VTSS_SW_OPTION_USERS
/* Get system passwd */
const char *system_get_encrypted_passwd()
{
    return system_global.system_conf.sys_passwd;
}

/* Set system passwd */
mesa_rc system_set_passwd(BOOL is_encrypted, const char *pass)
{
    system_conf_t   conf;
    char            username[VTSS_SYS_USERNAME_LEN];
    char            temp_passwd[VTSS_SYS_PASSWD_ARRAY_SIZE];
    char            encrypted_passwd[VTSS_SYS_PASSWD_ENCRYPTED_LEN];

    if ((!is_encrypted && strlen(pass) >= VTSS_SYS_PASSWD_LEN) ||
        (is_encrypted && strlen(pass) != VTSS_SYS_PASSWD_ENCRYPTED_LEN - 1)) {
        return VTSS_INVALID_PARAMETER;
    }

    misc_strncpyz(temp_passwd, pass, sizeof(temp_passwd));
    if (!is_encrypted) {
        system_password_encode(temp_passwd, encrypted_passwd);
    } else {
        misc_strncpyz(username, VTSS_SYS_ADMIN_NAME, sizeof(username));
        if (!sysutil_encrypted_password_fomat_is_valid(username, temp_passwd)) {
            return VTSS_INVALID_PARAMETER;
        }
        misc_strncpyz(encrypted_passwd, pass, sizeof(encrypted_passwd));
    }

    if (system_get_config(&conf) == VTSS_RC_OK && strcmp(encrypted_passwd, conf.sys_passwd) != 0) {
        SYSTEM_CRIT_ENTER();
        misc_strncpyz(conf.sys_passwd, encrypted_passwd, sizeof(conf.sys_passwd));
        SYSTEM_CRIT_EXIT();
        return system_set_config(&conf);
    }

    return VTSS_RC_OK;             /* Unchanged */
}
#endif /* VTSS_SW_OPTION_USERS */

#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
static BOOL system_check_temperature_config(
    const int l, const int h, const int c, const int t)
{
    if (!((l >= VTSS_APPL_SYSUTIL_TEMP_MONITOR_RANGE_LOW) && (l <= VTSS_APPL_SYSUTIL_TEMP_MONITOR_RANGE_HIGH))) {
        T_D("Low temp error!");
        return FALSE;
    }
    if (!((h >= VTSS_APPL_SYSUTIL_TEMP_MONITOR_RANGE_LOW) && (h <= VTSS_APPL_SYSUTIL_TEMP_MONITOR_RANGE_HIGH))) {
        T_D("High temp error!");
        return FALSE;
    }
    if (!((h > l) && (c > h))) {
        T_D("Low or High range error!");
        return FALSE;
    }
    if ((c < VTSS_APPL_SYSUTIL_TEMP_MONITOR_CRITICAL_LOW) || (c > VTSS_APPL_SYSUTIL_TEMP_MONITOR_CRITICAL_HIGH)) {
        T_D("Critical temp error!");
        return FALSE;
    }
    if (!((t >= VTSS_APPL_SYSUTIL_TEMP_MONITOR_HYSTERESIS_LOW) && (t <= VTSS_APPL_SYSUTIL_TEMP_MONITOR_HYSTERESIS_HIGH))) {
        T_D("Hysteresis error!");
        return FALSE;
    }
    return TRUE;
}

/* Init temperature monitor config defaults */
static mesa_rc system_init_temperature_status_default(
    vtss_appl_sysutil_tm_status_t *status)
{
    T_D("enter");
    memset(status, 0, sizeof(vtss_appl_sysutil_tm_status_t));
    T_D("exit");
    return VTSS_RC_OK;
}

/* Init temperature monitor config defaults by sensor */
mesa_rc system_init_temperature_config_default(
    vtss_appl_sysutil_tm_sensor_type_t sensor,
    vtss_appl_sysutil_tm_config_t      *config)
{
    int low, high, critical, hysteresis = VTSS_APPL_SYSUTIL_TEMP_MONITOR_HYSTERESIS_DEFAULT;

    switch (sensor) {
        case VTSS_APPL_SYSUTIL_TM_SENSOR_JUNCTION:
            low      = VTSS_APPL_SYSUTIL_TEMP_MONITOR_JUNCTION_LOW_DEFAULT;
            high     = VTSS_APPL_SYSUTIL_TEMP_MONITOR_JUNCTION_HIGH_DEFAULT;
            critical = VTSS_APPL_SYSUTIL_TEMP_MONITOR_JUNCTION_CRITICAL_DEFAULT;
            break;
        case VTSS_APPL_SYSUTIL_TM_SENSOR_BOARD:
        default:
            low      = VTSS_APPL_SYSUTIL_TEMP_MONITOR_BOARD_LOW_DEFAULT;
            high     = VTSS_APPL_SYSUTIL_TEMP_MONITOR_BOARD_HIGH_DEFAULT;
            critical = VTSS_APPL_SYSUTIL_TEMP_MONITOR_BOARD_CRITICAL_DEFAULT;
            break;
    }

    T_D("enter");

    config->low        = low;
    config->high       = high;
    config->critical   = critical;
    config->hysteresis = hysteresis;

    T_D("exit");
    return VTSS_RC_OK;
}

/* Set temperature monitor all config defaults */
mesa_rc system_set_temperature_config_default_all()
{
    T_D("enter");
    system_init_temperature_config_default(
        VTSS_APPL_SYSUTIL_TM_SENSOR_BOARD,  &system_global.tm_config[VTSS_APPL_SYSUTIL_TM_SENSOR_BOARD]);
    system_init_temperature_config_default(
        VTSS_APPL_SYSUTIL_TM_SENSOR_JUNCTION, &system_global.tm_config[VTSS_APPL_SYSUTIL_TM_SENSOR_JUNCTION]);
    T_D("exit");
    return VTSS_RC_OK;
}

/* Set temperature monitor all status defaults */
static mesa_rc system_set_temperature_status_default_all()
{
    T_D("enter");
    system_init_temperature_status_default(&system_global.tm_status[VTSS_APPL_SYSUTIL_TM_SENSOR_BOARD]);
    system_init_temperature_status_default(&system_global.tm_status[VTSS_APPL_SYSUTIL_TM_SENSOR_JUNCTION]);
    T_D("exit");
    return VTSS_RC_OK;
}

mesa_rc system_get_temperature_config(
    vtss_appl_sysutil_tm_sensor_type_t sensor,
    vtss_appl_sysutil_tm_config_t      *config)
{
    T_D("enter");

    SYSTEM_TM_CRIT_ENTER();
    memcpy(config, &system_global.tm_config[sensor], sizeof(vtss_appl_sysutil_tm_config_t));
    SYSTEM_TM_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

static mesa_rc system_get_temperature_status(
    vtss_appl_sysutil_tm_sensor_type_t sensor,
    vtss_appl_sysutil_tm_status_t      *status)
{
    T_D("enter");

    SYSTEM_TM_CRIT_ENTER();
    memcpy(status, &system_global.tm_status[sensor], sizeof(vtss_appl_sysutil_tm_status_t));
    SYSTEM_TM_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

static mesa_rc system_set_temperature_status(
    vtss_appl_sysutil_tm_sensor_type_t sensor,
    vtss_appl_sysutil_tm_status_t      *status)
{
    T_D("enter");

    SYSTEM_TM_CRIT_ENTER();
    memcpy(&system_global.tm_status[sensor], status, sizeof(vtss_appl_sysutil_tm_status_t));
    SYSTEM_TM_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

static mesa_rc system_set_temperature_config(
    vtss_appl_sysutil_tm_sensor_type_t  sensor,
    const vtss_appl_sysutil_tm_config_t *config)
{
    T_D("enter");

    if (system_check_temperature_config(
            config->low, config->high, config->critical, config->hysteresis)) {
        SYSTEM_TM_CRIT_ENTER();
        memcpy(&system_global.tm_config[sensor], config, sizeof(vtss_appl_sysutil_tm_config_t));
        SYSTEM_TM_CRIT_EXIT();
    } else {
        return SYSTEM_ERROR_PARM;
    }

    T_D("exit");
    return VTSS_RC_OK;
}

mesa_rc system_get_temperature_debug(
    vtss_appl_sysutil_tm_sensor_type_t sensor, int *temp)
{
    T_D("enter");

    SYSTEM_TM_CRIT_ENTER();
    *temp = system_global.tm_debug[sensor];
    SYSTEM_TM_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

mesa_rc system_set_temperature_debug(
    vtss_appl_sysutil_tm_sensor_type_t sensor, int temp)
{
    T_D("enter");

    SYSTEM_TM_CRIT_ENTER();
    system_global.tm_debug[sensor] = temp;
    SYSTEM_TM_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/* JSON notification */
static void system_send_temperature_notification(
    vtss_appl_sysutil_tm_sensor_type_t sensor,
    vtss_appl_sysutil_tm_status_t *status)
{
    /* Send notification */
    tm_status_event_update.set(sensor, status);
    T_D("Send notification: sensor=%d, low=%d, high=%d, critical=%d, temp=%d",
        sensor, status->low, status->high, status->critical, status->temp);
}

BOOL system_compare_temperature_gt(int a, int b, int c)
{
    if (a >= c){
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL system_compare_temperature_lt(int a, int b, int c)
{
    if (a > c){
        return FALSE;
    } else {
        return TRUE;
    }
}

void system_split_temperature(char *temp, int *i_i, int *i_d)
{
    if (i_i == NULL || i_d == NULL || temp == NULL) {
        T_E("Unexpected NULL pointer");
        return;
    }
    char *c_i, *c_d;
    char *saveptr; // Local strtok_r() context
    /* Split integer and decimal */
    c_i  = strtok_r(temp, ".", &saveptr);
    c_d  = strtok_r(NULL, ".", &saveptr);

    if (c_i == NULL || c_d == NULL) {
        T_E("Unexpected NULL pointer");
        return;
    }

    *i_i = atoi(c_i);
    *i_d = atoi(c_d);
}

static void system_process_temperature(char *board_temp, char *switch_temp)
{
    vtss_appl_sysutil_tm_status_t status[VTSS_APPL_SYSUTIL_TEMP_MONITOR_SENSOR_CNT];
    vtss_appl_sysutil_tm_config_t config[VTSS_APPL_SYSUTIL_TEMP_MONITOR_SENSOR_CNT];
    int int_part, decimal_part, debug_temp[VTSS_APPL_SYSUTIL_TEMP_MONITOR_SENSOR_CNT];
    vtss_appl_sysutil_tm_sensor_type_t *now_sensor_p = NULL, now_sensor, next_sensor;
    char *run_temp;

    while (!vtss_appl_sysutil_temperature_monitor_sensor_itr(now_sensor_p, &next_sensor)) {
        system_get_temperature_config(next_sensor,  &config[next_sensor]);
        system_get_temperature_status(next_sensor,  &status[next_sensor]);
        system_get_temperature_debug(next_sensor,  &debug_temp[next_sensor]);
        now_sensor = next_sensor;
        now_sensor_p = &now_sensor;

        /* Temperature hanlder */
        {
            if (VTSS_APPL_SYSUTIL_TM_SENSOR_BOARD == now_sensor) {
                run_temp = board_temp;
            } else {
                run_temp = switch_temp;
            }

            /* Split integer and decimal */
            system_split_temperature(run_temp, &int_part, &decimal_part);
            /* Debug used */
            if (debug_temp[now_sensor]) {
                int_part = debug_temp[now_sensor];
            }

            /* Update temperature first */
            status[now_sensor].temp = int_part;
            system_set_temperature_status(now_sensor, &status[now_sensor]);

            /* Check alarm raise */
            if (!status[now_sensor].critical && system_compare_temperature_gt(int_part, decimal_part, config[now_sensor].critical)) {
                status[now_sensor].critical = TM_ALARM_RAISE;
            }
            if (!status[now_sensor].high && system_compare_temperature_gt(int_part, decimal_part, config[now_sensor].high)) {
                status[now_sensor].high = TM_ALARM_RAISE;
            }
            if (!status[now_sensor].low && system_compare_temperature_lt(int_part, decimal_part, config[now_sensor].low)) {
                status[now_sensor].low = TM_ALARM_RAISE;
            }

            /* Check alarm clear */
            if (status[now_sensor].critical && system_compare_temperature_lt(int_part, decimal_part, config[now_sensor].critical - 1)) {
                status[now_sensor].critical = TM_ALARM_CLEAR;
            }
            if (status[now_sensor].high && system_compare_temperature_lt(int_part, decimal_part, config[now_sensor].high - config[now_sensor].hysteresis - 1)) {
                status[now_sensor].high = TM_ALARM_CLEAR;
            }
            if (status[now_sensor].low && system_compare_temperature_gt(int_part, decimal_part, config[now_sensor].low + config[now_sensor].hysteresis + 1)) {
                status[now_sensor].low = TM_ALARM_CLEAR;
            }
            status[now_sensor].temp = int_part;
        }

        // Need to be able to see status even when no alarm has ever been present
        system_set_temperature_status(now_sensor, &status[now_sensor]);
        system_send_temperature_notification(now_sensor, &status[now_sensor]);
    }
}
#endif

/****************************************************************************/
/*  Local functions                                                         */
/****************************************************************************/
/* Read/create system stack configuration */
static mesa_rc system_conf_read_stack()
{
    int               changed = 0;
    system_conf_t     *old_conf_p, new_conf;
    mesa_rc           rc = VTSS_RC_OK;

    T_D("enter");

    SYSTEM_CRIT_ENTER();
    /* Use default values */
    system_default_set(&new_conf);

    old_conf_p = &system_global.system_conf;
    if (system_conf_changed(old_conf_p, &new_conf)) {
        changed = 1;
    }
    system_global.system_conf = new_conf;
    SYSTEM_CRIT_EXIT();

    if (changed) {
        rc = system_stack_conf_set(VTSS_ISID_GLOBAL);
    }

    T_D("exit");

    return rc;
}

#if defined(VTSS_SW_OPTION_PSU)
static BOOL psu_active_get( monitor_sensor_id_t *active_sensor )
{
    monitor_sensor_status_t status;

    if ( FALSE == monitor_sensor_status (MONITOR_MAIN_PSU, &status) ) {
        return FALSE;
    }

    T_D( "Main PSU is  %s", monitor_state_txt(status));
    if (MONITOR_STATE_NORMAL == status) {
        *active_sensor = MONITOR_MAIN_PSU;
        T_D( "Main PSU is ACTIVE");
    } else if ( FALSE == monitor_sensor_status (MONITOR_REDUNDANT_PSU, &status) ){
        return FALSE;
    } else if (MONITOR_STATE_NORMAL == status) {
        *active_sensor = MONITOR_REDUNDANT_PSU;
        T_D( "Redundant PSU is ACTIVE");
    } else {
        return FALSE;
    }
    return TRUE;
}

static void SYSUTIL_psu_thread(vtss_addrword_t data)
{
    /* Make sure LED state will be set at the first time,
     * so we assigned different intial value to previous_sensor and current_sensor
     */
    static monitor_sensor_id_t  previous_sensor = MONITOR_MAIN_PSU;
    monitor_sensor_id_t         current_sensor = MONITOR_REDUNDANT_PSU;

    // Make a trick here in order to force set LED state in the initial state
    if (psu_active_get(&current_sensor) && current_sensor == MONITOR_MAIN_PSU) {
        previous_sensor = MONITOR_REDUNDANT_PSU;
    } else {
        previous_sensor = MONITOR_MAIN_PSU;
    }

    while (1) {
        if (psu_active_get(&current_sensor) && (previous_sensor != current_sensor)) {
            previous_sensor = current_sensor;
            if (current_sensor == MONITOR_MAIN_PSU) {
                led_front_led_state(LED_FRONT_LED_MAIN_PSU, FALSE);
            } else {
                led_front_led_state(LED_FRONT_LED_REDUNDANT_PSU, FALSE);
            }
        }

        VTSS_OS_MSLEEP(1000);
    }
}
#endif /* VTSS_SW_OPTION_PSU */

#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
static void SYSUTIL_tm_thread(vtss_addrword_t data)
{
    tmp43x_status_t temp;

    /* Start to polling temperature for board/switch */
    while (1) {
        if (!tmp43x_status_get(&temp)) {
            system_process_temperature(temp.local_temperature, temp.remote_temperature);
        } else {
            T_D("tmp43x_status_get() fail!");
        }
        VTSS_OS_MSLEEP(1000);
    }
}
#endif

/* Module start */
static void system_start(BOOL init)
{
    system_conf_t *conf_p;
    vtss_isid_t   isid;

    T_D("enter, init: %d", init);

    if (init) {
        /* Initialize system configuration */
        conf_p = &system_global.system_conf;
        system_default_set(conf_p);
#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
        system_set_temperature_config_default_all();
        system_set_temperature_status_default_all();
#endif
        /* Initialize message buffers */
        vtss_sem_init(&system_global.request.sem, 1);
        vtss_sem_init(&system_global.reply.sem, 1);

        /* Initialize system LED state timer */
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            VTSS_MTIMER_START(&system_global.system_led_state_timer[isid - VTSS_ISID_START], 1);
        }

        /* Initialize system LED state timer */
        vtss_flag_init(&system_global.system_led_state_flags);

        /* Create semaphore for critical regions */
        critd_init(&system_global.crit, "system", VTSS_MODULE_ID_SYSTEM, CRITD_TYPE_MUTEX);
#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
        /* Create semaphore for critical regions */
        critd_init(&system_global.tm_crit, "system.tm", VTSS_MODULE_ID_SYSTEM, CRITD_TYPE_MUTEX);
#endif
        /* Create semaphore for cpu util regions */
        critd_init(&cpu.crit, "system.cpu", VTSS_MODULE_ID_SYSTEM, CRITD_TYPE_MUTEX);

        /* place any other initialization junk you need here,
           Initialize system description */
        if (vtss_switch_stackable()) {
            (void) snprintf(system_descr, sizeof(system_descr), "%s Stackable GigaBit Ethernet Switch", VTSS_PRODUCT_NAME);
        } else {
            (void) snprintf(system_descr, sizeof(system_descr), "%s GigaBit Ethernet Switch", VTSS_PRODUCT_NAME);
        }
    } else {
        /* Register for stack messages */
        (void) system_stack_register();
    }
    T_D("exit");
}

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize private mib */
VTSS_PRE_DECLS void sysutil_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_sysutil_json_init(void);
#endif
extern "C" int sysutil_icli_cmd_register();

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
/* Initialize module */
mesa_rc system_init(vtss_init_data_t *data)
{
    mesa_rc     rc = VTSS_RC_OK;
    vtss_isid_t isid = data->isid;
    int fd = -1;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
        system_start(1);
#if defined(VTSS_SW_OPTION_ICFG) && (!defined(VTSS_SW_OPTION_USERS) || defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED))
        if ((rc = sysutil_icfg_init()) != VTSS_RC_OK) {
            T_D("fail to init icfg registration, rc = %s", error_txt(rc));
        }
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register private mib */
        sysutil_mib_init();
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_sysutil_json_init();
#endif
        sysutil_icli_cmd_register();
        break;

    case INIT_CMD_START:
        T_D("START");
        system_start(0);
#if defined(VTSS_SW_OPTION_PSU)
        // Create and start thread
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           SYSUTIL_psu_thread,
                           0,
                           "PSU",
                           nullptr,
                           0,
                           &SYSUTIL_psu_thread_handle,
                           &SYSUTIL_psu_thread_state);
#endif /* VTSS_SW_OPTION_PSU */
        cpu_util_init();
#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           SYSUTIL_tm_thread,
                           0,
                           "TEMPERATURE_MONITOR",
                           nullptr,
                           0,
                           &SYSUTIL_tm_thread_handle,
                           &SYSUTIL_tm_thread_state);
#endif
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            rc = system_conf_read_stack();
#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
            (void) system_set_temperature_config_default_all();
#endif
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");
        /* Read stack and switch configuration */
        if ((rc = system_conf_read_stack()) != VTSS_RC_OK) {
            return rc;
        }

        cpu_util_start();
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");
        /* Apply all configuration to switch */
        rc = system_stack_conf_set(isid);
        /* Touch /tmp/switch_app.ready to indicate the switch app is ready */
        fd = open("/tmp/switch_app.ready", O_RDWR | O_CREAT, 0644);
        if (fd == -1) {
            T_W("Touch %s failed!", "/tmp/switch_app.ready");
            break;
        }

        close(fd);
        break;

    default:
        break;
    }

    return rc;
}

/*
==============================================================================

    Public APIs in vtss_appl\include\vtss\appl\firmware.h

==============================================================================
*/
static BOOL _usid_exist(
    vtss_usid_t     usid
)
{
    vtss_isid_t     isid;

    T_D("usid_exists: %u", usid);

    if (usid >= VTSS_USID_END) {
        return FALSE;
    }

    isid = topo_usid2isid(usid);
    if (msg_switch_is_local(isid) || msg_switch_exists(isid)) {
        return TRUE;
    }
    return FALSE;
}

/**
 * \brief Get capabilities
 *
 * \param capabilities [OUT] The capabilities of sysutil.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_capabilities_get(
    vtss_appl_sysutil_capabilities_t    *const capabilities
)
{
    if (capabilities == NULL) {
        T_E("capabilities == NULL\n");
        return VTSS_RC_ERROR;
    }




    capabilities->warmReboot = FALSE;


    capabilities->post = FALSE;

#if defined(VTSS_SW_OPTION_ZTP)
    capabilities->ztp = TRUE;
#else
    capabilities->ztp = FALSE;
#endif /* VTSS_SW_OPTION_ZTP */

#if defined(VTSS_SW_OPTION_SPROUT) && defined(VTSS_SPROUT_FW_VER_CHK)
    capabilities->stack_fw_chk = TRUE;
#else
    capabilities->stack_fw_chk = FALSE;
#endif /*VTSS_SW_OPTION_SPROUT && VTSS_SPROUT_FW_VER_CHK */

    return VTSS_RC_OK;
}

/**
 * \brief Get average CPU loads in different periods
 *
 * \param cpuLoad [OUT] The average CPU loads.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_status_cpu_load_get(vtss_appl_sysutil_status_cpu_load_t *const cpuLoad)
{
    BOOL    b;
    u32     average100msec;
    u32     average1sec;
    u32     average10sec;

    /* check parameter */
    if (cpuLoad == NULL) {
        T_E("cpuLoad == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* get CPU load */
    b = control_sys_get_cpuload(&average100msec, &average1sec, &average10sec);
    if (b == FALSE) {
        return VTSS_RC_ERROR;
    }

    cpuLoad->average100msec = average100msec;
    cpuLoad->average1sec    = average1sec;
    cpuLoad->average10sec   = average10sec;

    return VTSS_RC_OK;
}

/**
 * \brief Reboot iterate function
 *
 * \param prev_usid [IN] : previous switch ID.
 * \param next_usid [OUT]: next switch ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_control_reboot_itr(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid
)
{
    return vtss_appl_iterator_switch(prev_usid, next_usid);
}

/**
 * \brief Get reboot parameters of sysutil control
 *
 * \param usid   [IN]  Switch ID.
 * \param reboot [OUT] The reboot parameters.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_control_reboot_get(
    vtss_usid_t                             usid,
    vtss_appl_sysutil_control_reboot_t      *const reboot
)
{
    /* check usid exists or not */
    if (_usid_exist(usid) == FALSE) {
        return VTSS_RC_ERROR;
    }

    /* check parameter */
    if (reboot == NULL) {
        T_E("reboot == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* get data */
    memset(reboot, 0, sizeof(vtss_appl_sysutil_control_reboot_t));

    return VTSS_RC_OK;
}

/**
 * \brief Set reboot parameters of sysutil control
 *
 * \param usid   [IN] Switch ID.
 * \param reboot [IN] The reboot parameters.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_control_reboot_set(
    vtss_usid_t                                 usid,
    const vtss_appl_sysutil_control_reboot_t    *const reboot,
    uint32_t                                    wait_before_callback_msec
)
{
    mesa_restart_t      restart_type;
    int                 r;

    /* check usid exists or not */
    if (_usid_exist(usid) == FALSE) {
        return VTSS_RC_ERROR;
    }

    /* check parameter */
    if (reboot == NULL) {
        T_E("reboot == NULL\n");
        return VTSS_RC_ERROR;
    }




    if (reboot->type > VTSS_APPL_SYSUTIL_REBOOT_TYPE_COLD) {

        return VTSS_RC_ERROR;
    }

    /* Do action -> Reboot */

    if (reboot->type == VTSS_APPL_SYSUTIL_REBOOT_TYPE_NONE) {
        return VTSS_RC_OK;
    }

    restart_type = (reboot->type == VTSS_APPL_SYSUTIL_REBOOT_TYPE_COLD) ? MESA_RESTART_COOL : MESA_RESTART_WARM;

    r = control_system_reset(TRUE, VTSS_USID_ALL, restart_type, wait_before_callback_msec);

    if (r != 0) {
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Power supply iterate function, it is used to get first and get next indexes.
 *
 * \param prev_usid   [IN]  previous switch ID.
 * \param next_usid   [OUT] next switch ID.
 * \param prev_psuid  [IN]  previous precedence.
 * \param next_psuid  [OUT] next precedence.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_psu_status_itr(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid,
    const u32           *const prev_psuid,
    u32                 *const next_psuid
)
{
    vtss::IteratorComposeN<vtss_usid_t, vtss_usid_t> itr(
            &vtss_appl_iterator_switch,
            &vtss::expose::snmp::IteratorComposeStaticRange<u32, 1, VTSS_APPL_SYSUTIL_PSU_MAX_ID>);

    return itr(prev_usid, next_usid, prev_psuid, next_psuid);
}

/**
 * \brief Get power supply status
 *
 * \param usid    [IN]  switch ID for user view (The value starts from 1)
 * \param psuid   [IN]  The index of power supply
 * \param status  [OUT] The power supply status
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_psu_status_get(
    vtss_usid_t                     usid,
    u32                             psuid,
    vtss_appl_sysutil_psu_status_t  *const status
)
{
#ifdef VTSS_SW_OPTION_PSU
    monitor_sensor_id_t         sensor_id;
    monitor_sensor_id_t         active_psu;
    monitor_sensor_status_t     sensor_status;
#endif

    if (_usid_exist(usid) == FALSE) {
        return VTSS_RC_ERROR;
    }

    if (psuid == 0 || psuid > VTSS_APPL_SYSUTIL_PSU_MAX_ID) {
        return VTSS_RC_ERROR;
    }

    if (status == NULL) {
        T_E("status == NULL\n");
        return VTSS_RC_ERROR;
    }

#ifdef VTSS_SW_OPTION_PSU

    sensor_id = (psuid == 1) ? MONITOR_MAIN_PSU : MONITOR_REDUNDANT_PSU;

    if (psu_active_get(&active_psu) == FALSE || monitor_sensor_status(sensor_id, &sensor_status) == FALSE) {
        return VTSS_RC_ERROR;
    }

    memset(status, 0, sizeof(vtss_appl_sysutil_psu_status_t));

    if ( active_psu == MONITOR_MAIN_PSU ) {
        status->state = (active_psu == sensor_id) ? VTSS_APPL_SYSUTIL_PSU_STATE_ACTIVE :
                        (sensor_status == MONITOR_STATE_NORMAL) ? VTSS_APPL_SYSUTIL_PSU_STATE_STANDBY : VTSS_APPL_SYSUTIL_PSU_STATE_NOT_PRESENT;
    } else {
        status->state = (active_psu == sensor_id) ? VTSS_APPL_SYSUTIL_PSU_STATE_ACTIVE : VTSS_APPL_SYSUTIL_PSU_STATE_NOT_PRESENT;
    }

    if (sensor_id == MONITOR_MAIN_PSU) {
        strncpy(status->descr, monitor_descr_txt(MONITOR_MAIN_PSU), VTSS_APPL_SYSUTIL_PSU_DESCR_MAX_LEN);
    } else {
        strncpy(status->descr, monitor_descr_txt(MONITOR_REDUNDANT_PSU), VTSS_APPL_SYSUTIL_PSU_DESCR_MAX_LEN);
    }

#else

    status->state = (psuid == 1) ? VTSS_APPL_SYSUTIL_PSU_STATE_ACTIVE :VTSS_APPL_SYSUTIL_PSU_STATE_NOT_PRESENT;
    strcpy(status->descr, (psuid == 1) ? "Main power supply" : "Redundant power supply");

#endif /*   VTSS_SW_OPTION_PSU */

    return VTSS_RC_OK;
}

/**
 * \brief User switch iterate function, it is used to get first and get next indexes.
 *
 * \param prev_usid   [IN]  previous switch ID.
 * \param next_usid   [OUT] next switch ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_usid_itr(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid
)
{
    return vtss_appl_iterator_switch(prev_usid, next_usid);
}

/* Wait for reply to request */
static BOOL SYSUTIL_req_led_state_timeout(vtss_isid_t       isid,
                                          system_msg_id_t   msg_id,
                                          vtss_mtimer_t     *timer,
                                          vtss_flag_t        *flags)
{
    BOOL              timeout;
    vtss_flag_value_t flag;
    vtss_tick_count_t time_tick;
    system_msg_buf_t  buf;

    SYSTEM_CRIT_ENTER();
    timeout = VTSS_MTIMER_TIMEOUT(timer);
    SYSTEM_CRIT_EXIT();

    if (timeout) {
        (void)system_msg_req_alloc(&buf, SYSTEM_MSG_ID_LED_STATE_GET_REQ);
        flag = (1 << isid);
        vtss_flag_maskbits(flags, ~flag);
        system_msg_tx(&buf, isid, sizeof(system_msg_req_t));
        time_tick = vtss_current_time() + VTSS_OS_MSEC2TICK(SYSTEM_REQ_TIMEOUT * 1000);
        return (vtss_flag_timed_wait(flags, flag, VTSS_FLAG_WAITMODE_OR, time_tick) & flag ? 0 : 1);
    }

    return FALSE;
}

/**
 * \brief Get system LED status
 *
 * \param usid    [IN]  switch ID for user view (The value starts from 1)
 * \param status  [OUT] Thesystem LED status
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_system_led_status_get(
    vtss_usid_t                             usid,
    vtss_appl_sysutil_system_led_status_t   *const status
)
{
    vtss_isid_t isid = VTSS_ISID_START;

    /* check usid exists or not */
    if (!msg_switch_is_primary() || _usid_exist(usid) == FALSE) {
        return VTSS_RC_ERROR;
    }

    /* check parameter */
    if (status == NULL) {
        T_E("status == NULL\n");
        return VTSS_RC_ERROR;
    }


    if (msg_switch_is_local(isid)) {    // Bypass Message module
        led_front_led_state_t state;
        led_front_led_state_get(&state);
        strcpy(status->descr, led_front_led_state_txt(state));
    } else {
        if (SYSUTIL_req_led_state_timeout(isid,
                                          SYSTEM_MSG_ID_LED_STATE_GET_REQ,
                                          &system_global.system_led_state_timer[isid - VTSS_ISID_START],
                                          &system_global.system_led_state_flags)) {
            T_W("timeout, SYSTEM_MSG_ID_LED_STATE_GET_REQ");
            strcpy(status->descr, "Unknown");
            return VTSS_RC_ERROR;
        }

        SYSTEM_CRIT_ENTER();
        strcpy(status->descr, led_front_led_state_txt(system_global.system_led_state[isid - VTSS_ISID_START]));
        SYSTEM_CRIT_EXIT();
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get reboot parameters of sysutil control
 *
 * \param usid   [IN]  Switch ID.
 * \param reboot [OUT] The clear system LED parameters.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_control_system_led_get(
    vtss_usid_t                             usid,
    vtss_appl_sysutil_control_system_led_t  *const clear
)
{
    /* check usid exists or not */
    if (_usid_exist(usid) == FALSE) {
        return VTSS_RC_ERROR;
    }

    /* check parameter */
    if (clear == NULL) {
        T_E("clear == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* get data */
    memset(clear, 0, sizeof(vtss_appl_sysutil_control_system_led_t));

    return VTSS_RC_OK;
}

/**
 * \brief Set clear system LED parameters of sysutil control
 *
 * \param usid  [IN] Switch ID.
 * \param clear [IN] The clear system LED parameters.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_control_system_led_set(
    vtss_usid_t                                     usid,
    const vtss_appl_sysutil_control_system_led_t    *const clear
)
{
    vtss_isid_t             isid = VTSS_ISID_START;
    led_front_led_state_t   state = LED_FRONT_LED_NORMAL;
    BOOL                    clear_all = FALSE;

    /* check usid exists or not */
    if (!msg_switch_is_primary() || _usid_exist(usid) == FALSE) {
        return VTSS_RC_ERROR;
    }

    /* check parameter */
    if (clear == NULL) {
        T_E("clear == NULL\n");
        return VTSS_RC_ERROR;
    }
    if (clear->type > VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_STACK_FW_CHK) {
        return VTSS_RC_ERROR;
    }


    /* Do action -> clear system LED state */
    switch (clear->type) {
    case VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_ALL:
        clear_all = TRUE;
        break;
    case VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_FATAL:
        state = LED_FRONT_LED_FATAL;
        break;
    case VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_SW:
        state = LED_FRONT_LED_ERROR;
        break;
#if defined(VTSS_SW_OPTION_ZTP)
    case VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_ZTP:
        state = LED_FRONT_LED_ZTP_ERROR;
        break;
#endif /* VTSS_SW_OPTION_ZTP */
#if defined(VTSS_SW_OPTION_SPROUT) && defined(VTSS_SPROUT_FW_VER_CHK)
    case VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_STACK_FW_CHK:
        state = LED_FRONT_LED_STACK_FW_CHK_ERROR;
        break;
#endif /*VTSS_SW_OPTION_SPROUT && VTSS_SPROUT_FW_VER_CHK */
    default:
        break;
    }

    if (!clear_all && state == LED_FRONT_LED_NORMAL) {  // Not supported
        return VTSS_RC_ERROR;
    }

    if (msg_switch_is_local(isid)) {    // Bypass Message module
        if (clear_all) {
            led_front_led_state_clear_all();
        } else {
            led_front_led_state_clear(state);
        }
    } else {
        system_msg_req_t  *msg;
        system_msg_buf_t  buf;

        msg = system_msg_req_alloc(&buf, SYSTEM_MSG_ID_LED_STATE_CLEAR);
        SYSTEM_CRIT_ENTER();
        if (clear_all) {
            msg->system_led_state_clear_all = TRUE;
        } else {
            msg->system_led_state_clear_all = FALSE;
            msg->system_led_state_clear = state;
        }
        SYSTEM_CRIT_EXIT();
        system_msg_tx(&buf, isid, sizeof(system_msg_req_t));
    }

    return VTSS_RC_OK;
}

static inline void cpy_tag(char *dest, size_t bufsiz, const char *name)
{
    const char *str = conf_mgmt_board_tag_get(name);
    if (str) {
        misc_strncpyz(dest, str, bufsiz);
    } else {
        dest[0] = '\0';
    }
}

/****************************************************************************/
/* board information                                                        */
/****************************************************************************/
mesa_rc vtss_appl_sysutil_board_info_get(vtss_appl_sysutil_board_info_t *board_info)
{
    conf_board_t conf;

    if (board_info == NULL) {
        T_E("Invalid argument [NULL pointer]");
        return VTSS_RC_ERROR;
    }

    if (conf_mgmt_board_get(&conf) != 0) {
        return VTSS_RC_ERROR;
    }

    board_info->mac = conf.mac_address;
    board_info->board_id = conf.board_id;
    cpy_tag(board_info->board_serial, sizeof(board_info->board_serial), "SYSTEM_SERIAL_NUM");
    cpy_tag(board_info->board_type, sizeof(board_info->board_type), "MODEL_NUM");
    // Fallback value for board type
    if (strlen(board_info->board_type) == 0) {
        strncpy(board_info->board_type, vtss_board_name(), sizeof(board_info->board_type));
    }

    return VTSS_RC_OK;
}

/****************************************************************************/
/* System time get/set                                                      */
/****************************************************************************/
mesa_rc vtss_appl_sysutil_system_time_get(
    vtss_appl_sysutil_system_time_t *const system_time)
{
    time_t t;
    char *tmp;
    char buf[30]; // for ctime. Must be at least 26 long.
    char format[] = "%a %b  %e %H:%M:%S %Y";  // default format

    // sanity check
    if (system_time == NULL) {
        T_E("Invalid argument [NULL pointer]");
        return VTSS_RC_ERROR;
    }

    // return the number of seconds since the Epoch
    t = time(NULL);

    // convert time_t to printable form as
    // "Wed Jun 30 21:49:08 1993\n"  (NOTE: '\n')
    tmp = ctime_r(&t, buf);
    if (tmp == NULL) {
        T_E("Failed at ctime");
        return VTSS_RC_ERROR;
    }

    tmp[strlen(tmp)-1] = '\0';  // remove '\n'
    strcpy(system_time->sys_curtime, tmp);  // strcpy will auto include '\0'
    strcpy(system_time->sys_curtime_format, format);

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_sysutil_system_time_set(
    const vtss_appl_sysutil_system_time_t *const system_time)
{
    // Broken-down time structure
    struct tm tm;
    timeval t;

    // NOTE: initialize tm before calling strptime
    memset(&tm, 0x0, sizeof(struct tm));
    memset(&t, 0x0, sizeof(struct timeval));

    // sanity check
    if (system_time == NULL) {
        T_E("Invalid argument [NULL pointer]");
        return VTSS_RC_ERROR;
    }

    // convert from printable form to broken-down form
    if (strptime(system_time->sys_curtime, system_time->sys_curtime_format, &tm)
        == NULL) {
        T_E("Failed at strptime(), please check date/time, format again");
        return VTSS_RC_ERROR;
    }

    // tm_isdst is not set by strptime();
    // less than 0, will make mktime() to try to determine if DST is in effect
    tm.tm_isdst = -1;

    // convert broken-down time to time_t value
    t.tv_sec = mktime(&tm);

    if (t.tv_sec == -1) {
        T_E("Failed at mktime()");
        return VTSS_RC_ERROR;
    }

    // requir CAP_SYS_TIME to call stime()
    if (settimeofday(&t, NULL) == -1) {
        T_E("Failed at stime() [%s]", strerror(errno));
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}



/****************************************************************************/
/* System Uptime information                                                        */
/****************************************************************************/
static const char *system_time_txt(time_t time_val)
{
    const char *s;

    s = misc_time2interval(time_val);
    while (*s == ' ') {
        s++;
    }
    if (!strncmp(s, "0d ", 3)) {
        s += 3;
    }

    return s;
}

mesa_rc vtss_appl_sysutil_sytem_uptime_get(vtss_appl_sysutil_sys_uptime_t *const status)
{
    if (status == NULL) {
        T_E("System Uptime parameter is NULL\n");
        return VTSS_RC_ERROR;
    }

    memcpy(status->sys_uptime, system_time_txt(VTSS_OS_TICK2MSEC(vtss_current_time())/1000), sizeof(status->sys_uptime));

    return VTSS_RC_OK;
}

/****************************************************************************/
/* System Info                                                              */
/****************************************************************************/
mesa_rc vtss_appl_sysutil_system_config_get(vtss_appl_sysutil_sys_conf_t *const info)
{
    system_conf_t conf;

    if (info == NULL) {
        T_E("System Configuration parameter is NULL\n");
        return VTSS_RC_ERROR;
    }

    VTSS_RC(system_get_config(&conf));

    memcpy(info->sys_name, conf.sys_name, sizeof(conf.sys_name));
    memcpy(info->sys_contact, conf.sys_contact, sizeof(conf.sys_contact));
    memcpy(info->sys_location, conf.sys_location, sizeof(conf.sys_location));

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_sysutil_system_config_set(const vtss_appl_sysutil_sys_conf_t *const config)
{
    system_conf_t conf;

    memset(&conf, 0 ,sizeof(conf));

    VTSS_RC(system_get_config(&conf));

    memcpy(conf.sys_name, config->sys_name, sizeof(config->sys_name));
    memcpy(conf.sys_contact, config->sys_contact, sizeof(config->sys_contact));
    memcpy(conf.sys_location, config->sys_location, sizeof(config->sys_location));

    VTSS_RC(system_set_config(&conf));

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_sysutil_temperature_monitor_config_get(
    vtss_appl_sysutil_tm_sensor_type_t sensor,
    vtss_appl_sysutil_tm_config_t      *const config)
{
#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
    if (sensor != VTSS_APPL_SYSUTIL_TM_SENSOR_BOARD && sensor != VTSS_APPL_SYSUTIL_TM_SENSOR_JUNCTION) {
        T_D("Fail, %s\n", system_error_txt(SYSTEM_ERROR_KEY));
        return SYSTEM_ERROR_KEY;
    }

    if (config == NULL) {
        T_D("Fail, %s\n", system_error_txt(SYSTEM_ERROR_PARM_NULL));
        return SYSTEM_ERROR_PARM_NULL;
    }

    VTSS_RC(system_get_temperature_config(sensor, config));

    return VTSS_RC_OK;
#else
    return SYSTEM_ERROR_TM_NOT_SUPPORT;
#endif
}

mesa_rc vtss_appl_sysutil_temperature_monitor_config_set(
    vtss_appl_sysutil_tm_sensor_type_t  sensor,
    const vtss_appl_sysutil_tm_config_t *const config)
{
#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
    if (sensor != VTSS_APPL_SYSUTIL_TM_SENSOR_BOARD && sensor != VTSS_APPL_SYSUTIL_TM_SENSOR_JUNCTION) {
        T_D("Fail, %s\n", system_error_txt(SYSTEM_ERROR_KEY));
        return SYSTEM_ERROR_KEY;
    }

    if (config == NULL) {
        T_D("Fail, %s\n", system_error_txt(SYSTEM_ERROR_PARM_NULL));
        return SYSTEM_ERROR_PARM_NULL;
    }

    VTSS_RC(system_set_temperature_config(sensor, config));

    return VTSS_RC_OK;
#else
    return SYSTEM_ERROR_TM_NOT_SUPPORT;
#endif
}

mesa_rc vtss_appl_sysutil_temperature_monitor_status_get(
    vtss_appl_sysutil_tm_sensor_type_t sensor,
    vtss_appl_sysutil_tm_status_t      *const status)
{
#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
    if (sensor != VTSS_APPL_SYSUTIL_TM_SENSOR_BOARD && sensor != VTSS_APPL_SYSUTIL_TM_SENSOR_JUNCTION) {
        T_D("Fail, %s\n", system_error_txt(SYSTEM_ERROR_KEY));
        return SYSTEM_ERROR_KEY;
    }

    if (status == NULL) {
        T_D("Fail, %s\n", system_error_txt(SYSTEM_ERROR_PARM_NULL));
        return SYSTEM_ERROR_PARM_NULL;
    }

    VTSS_RC(system_get_temperature_status(sensor, status));

    return VTSS_RC_OK;
#else
    return SYSTEM_ERROR_TM_NOT_SUPPORT;
#endif
}

mesa_rc vtss_appl_sysutil_temperature_monitor_sensor_itr(
    const vtss_appl_sysutil_tm_sensor_type_t *const prev,
    vtss_appl_sysutil_tm_sensor_type_t       *const next)
{
#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
    if (next == NULL) {
        T_D("Fail, %s\n", system_error_txt(SYSTEM_ERROR_PARM));
        return SYSTEM_ERROR_PARM;
    }

    if (!prev) {
        *next = VTSS_APPL_SYSUTIL_TM_SENSOR_BOARD;
    } else {
        switch(*prev) {
            case VTSS_APPL_SYSUTIL_TM_SENSOR_BOARD:
                *next = VTSS_APPL_SYSUTIL_TM_SENSOR_JUNCTION;
                break;
            case VTSS_APPL_SYSUTIL_TM_SENSOR_JUNCTION:
            default:
                return VTSS_RC_ERROR;
        }
    }
    return VTSS_RC_OK;
#else
    return SYSTEM_ERROR_TM_NOT_SUPPORT;
#endif
}

/**
 * \brief Get temperature monitor event status.
 *
 * \param sensor    [IN]    (key) Temperature monitor sensor ID.
 * \param status    [OUT]   The temperature monitor status of the temperature monitor sensor.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_temperature_monitor_event_get(
    vtss_appl_sysutil_tm_sensor_type_t sensor,
    vtss_appl_sysutil_tm_status_t      *const status
)
{
#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
    return tm_status_event_update.get(sensor, status);
#else
    return SYSTEM_ERROR_TM_NOT_SUPPORT;
#endif
}

#define LICENSE_FILENAME "/etc/mscc/licenses.txt.gz"
mesa_rc system_license_open(void **handle)
{
    gzFile file;

    if ((file = gzopen(LICENSE_FILENAME, "rb")) == NULL) {
        T_E("Unable to open %s for reading: %s", LICENSE_FILENAME, strerror(errno));
        return SYSTEM_ERROR_LICENSE_OPEN;
    }

    *handle = file;
    return VTSS_RC_OK;
}

mesa_rc system_license_gets(void *handle, char *line, size_t line_length, bool include_details)
{
    const char *stop_pattern = "Raw licenses";
    int        errnum;
    gzFile     file = (gzFile)handle;

    // gzgets() will always zero-terminate line.
    while (gzgets(file, line, line_length) != NULL) {
        if (!include_details && strncmp(line, stop_pattern, strlen(stop_pattern)) == 0) {
            // Signal end-of-license
            line[0] = '\0';
        }

        return VTSS_RC_OK;
    }

    // Either EOF or an error occurred.
    line[0] = '\0';

    (void)gzerror(file, &errnum);
    if (errnum != Z_OK) {
        T_E("During decompress of %s: %s", LICENSE_FILENAME, errnum == Z_ERRNO ? strerror(errno) : gzerror(file, &errnum));
        return SYSTEM_ERROR_LICENSE_GETS;
    }

    return VTSS_RC_OK;
}

mesa_rc system_license_close(void *handle)
{
    gzFile file = (gzFile)handle;
    int    errnum;

    if ((errnum = gzclose_r(file)) != Z_OK) {
        if (errnum == Z_ERRNO) {
            T_E("During close of %s: %s", LICENSE_FILENAME, strerror(errno));
        } else {
            // Cannot use gzerror(file, &errnum), because file is closed.
            T_E("During close of %s: GZ Error %d", LICENSE_FILENAME, errnum);
        }
        return SYSTEM_ERROR_LICENSE_CLOSE;
    }

    return VTSS_RC_OK;
}

/****************************************************************************/
/*  CPU utilization support                                                 */
/****************************************************************************/

static bool get_cpu_jiffy(FILE *fp, jiffy_counts_t *p_jif)
{
    static const char fmt[] = "cpu %u %u %u %u %u %u %u";
    bool ret;
    char line_buf[128];

    if (!fgets(line_buf, sizeof(line_buf), fp) || line_buf[0] != 'c'  /* not "cpu" */)
        return 0;
    ret = (sscanf(line_buf, fmt,
                  &p_jif->usr, &p_jif->nic, &p_jif->sys, &p_jif->idle,
                  &p_jif->iowait, &p_jif->irq, &p_jif->softirq) == 7);
    if (ret) {
        p_jif->busy =
                p_jif->usr + p_jif->nic + p_jif->sys +
                p_jif->iowait + p_jif->irq + p_jif->sys + p_jif->softirq;
    }
    return ret;
}

static void push_sample(const jiffy_counts_t *ct)
{
    int i = cpu.s_index;
    samples[i].busy = ct->busy - last.busy;
    samples[i].idle = ct->idle - last.idle;
    if (cpu.s_index == MAX_SAMPLES-1) {
        cpu.s_index = 0;
    } else {
        cpu.s_index++;
    }
}

static void get_jiffy_counts(void)
{
    const char *filename = "/proc/stat";
    FILE *fp;
    jiffy_counts_t ct;

    fp = fopen(filename, "r");
    if (fp) {
        if (!get_cpu_jiffy(fp, &ct)) {
            T_D("cann't read %s", filename);
        } else {
            push_sample(&ct);
            last.busy = ct.busy;
            last.idle = ct.idle;
        }
        fclose(fp);
    }
    return;
}

#define PREV_SAMPLE(x) (x == 0 ? MAX_SAMPLES-1 : x-1)
#define SAFE_DIV(x, y) ((y) == 0 ? 0 : (x) / (y))

BOOL vtss_cpuload_get(unsigned int *average_point1s,
                      unsigned int *average_1s,
                      unsigned int *average_10s)
{
    int oldest, current, j, pct;
    int total, busy;
    load_sample_t *ct;
    CPU_CRIT_ENTER();

    oldest = cpu.s_index;          // *Next* slot to be filled = oldest sample
    current = PREV_SAMPLE(oldest); // Newest data

    // Now
    ct = &samples[current];
    pct = SAFE_DIV((ct->busy * 100), (ct->busy + ct->idle));
    ct = &samples[current];
    *average_point1s = pct;

    // Last 10 samples
    for (total = busy = 0, j = 0, current = PREV_SAMPLE(oldest); j < 10; j++, current = PREV_SAMPLE(current)) {
        ct = &samples[current];
        total += (ct->busy + ct->idle);
        busy += ct->busy;
    }
    pct = (int) SAFE_DIV((busy * 100), total);
    *average_1s = pct;

    // All samples
    for (total = busy = 0, j = 0, current = PREV_SAMPLE(oldest); j < MAX_SAMPLES; j++, current = PREV_SAMPLE(current)) {
        ct = &samples[current];
        total += (ct->busy + ct->idle);
        busy += ct->busy;
    }
    pct = (int) SAFE_DIV((busy * 100), total);
    *average_10s = pct;

    CPU_CRIT_EXIT();
    return true;
}

/* CPU utilization timer callback function */
static void cpu_util_timeout(vtss::Timer *timer)
{
    CPU_CRIT_ENTER();
    get_jiffy_counts();
    CPU_CRIT_EXIT();
}

/* Start CPU utilization timer */
static void cpu_util_start(void)
{
    T_D("enter");
    (void)vtss_timer_cancel(&cpu.timer);
    if (vtss_timer_start(&cpu.timer) != VTSS_RC_OK) {
        T_E("Unable to start timer");
    }
}

/* Initialize CPU utilization timer */
static void cpu_util_init(void)
{
    T_D("enter");

    cpu.timer.modid = VTSS_MODULE_ID_SYSTEM;
    cpu.timer.set_repeat(TRUE);  /* Never ending */
    cpu.timer.set_period(vtss::milliseconds(100));
    cpu.timer.callback = cpu_util_timeout;
}

/* system error text */
const char *system_error_txt(mesa_rc rc)
{
    const char *txt;

    switch (rc) {
    case SYSTEM_ERROR_GEN:
        txt = "system generic error";
        break;
    case SYSTEM_ERROR_PARM:
        txt = "system parameter error";
        break;
    case SYSTEM_ERROR_STACK_STATE:
        txt = "system stack state error";
        break;
    case SYSTEM_ERROR_PARM_NULL:
        txt = "system parameter is null";
        break;
    case SYSTEM_ERROR_KEY:
        txt = "system key error";
        break;
    case SYSTEM_ERROR_TM_NOT_SUPPORT:
        txt = "system temperature monitor feature is not supported";
        break;
    case SYSTEM_ERROR_LICENSE_OPEN:
        txt = "Unable to open \"" LICENSE_FILENAME "\" for reading";
        break;
    case SYSTEM_ERROR_LICENSE_GETS:
        txt = "Unable to get line from \"" LICENSE_FILENAME "\"";
        break;
    case SYSTEM_ERROR_LICENSE_CLOSE:
        txt = "Unable to close \"" LICENSE_FILENAME "\"";
        break;
    default:
        txt = "system unknown error";
        break;
    }
    return txt;
}

