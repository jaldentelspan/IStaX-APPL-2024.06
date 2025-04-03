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

/*
  The Auth module is responsible for authenticate a combination of a username
  and a password. The authentication can be done against a local "database", a
  RADIUS server or a TACACS+ server.

  In order to work across a stack the authentication is partitioned into a client
  and and a server part, which always communicates via the msg module.

  The server runs on the primary switch and the clients run on both primary and
  secondary switches.
*/

#include "main.h"
#include "msg_api.h"
#include "conf_api.h"
#include "sysutil_api.h"
#include "misc_api.h"
#include "vtss_auth_api.h"
#include "critd_api.h"
#include "vtss_auth.h"
#include <netdb.h>
#include "vtss_safe_queue.hxx"

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#else
/* Define dummy syslog macros */
#define S_I(fmt, ...)
#define S_W(fmt, ...)
#define S_E(fmt, ...)
#endif

#ifdef VTSS_SW_OPTION_FAST_CGI
#include "vtss_https.hxx"
#endif

#ifdef VTSS_SW_OPTION_RADIUS
#include "vtss_radius_api.h"
/*
 * We don't want to use the following defines directly from the RADIUS module.
 * Instead we #error if they do not match our expectations, and let the system
 * designer/integrator decide what to do.
 */
#if VTSS_RADIUS_NUMBER_OF_SERVERS != VTSS_APPL_AUTH_NUMBER_OF_SERVERS
#error VTSS_RADIUS_NUMBER_OF_SERVERS != VTSS_APPL_AUTH_NUMBER_OF_SERVERS
#endif
#if VTSS_RADIUS_HOST_LEN != VTSS_APPL_AUTH_HOST_LEN
#error VTSS_RADIUS_HOST_LEN != VTSS_APPL_AUTH_HOST_LEN
#endif
#if VTSS_RADIUS_KEY_LEN != VTSS_APPL_AUTH_UNENCRYPTED_KEY_LEN
#error VTSS_RADIUS_KEY_LEN != VTSS_APPL_AUTH_UNENCRYPTED_KEY_LEN
#endif

#endif /* VTSS_SW_OPTION_RADIUS */

#ifdef VTSS_SW_OPTION_TACPLUS
#include <libtacplus.h>
#endif /* VTSS_SW_OPTION_TACPLUS */

#ifdef VTSS_SW_OPTION_USERS
#include "vtss_users_api.h"
#endif /* VTSS_SW_OPTION_USERS */

#if defined(VTSS_SW_OPTION_CLI_TELNET) || defined(VTSS_APPL_AUTH_ENABLE_CONSOLE)
#include "cli_io_api.h"
#endif /* defined(VTSS_SW_OPTION_CLI_TELNET) || defined(VTSS_APPL_AUTH_ENABLE_CONSOLE) */

#ifdef VTSS_SW_OPTION_ICFG
#include "vtss_auth_icfg.h"
#include "icfg_api.h"
#endif /* VTSS_SW_OPTION_ICFG */

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/stat.h>

#include "ip_utils.hxx"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_AUTH

//#define VTSS_AUTH_CHECK_HEAP      1 /* check for loss of memory on the heap */

#if !defined(VTSS_AUTH_ATTRIBUTE_PACKED)
#define VTSS_AUTH_ATTRIBUTE_PACKED __attribute__((packed)) /* GCC defined */
#endif /* !defined(VTSS_AUTH_ATTRIBUTE_PACKED) */

#define VTSS_AUTH_CLIENT_TIMEOUT         (60 * 60) /* seconds that client waits for reply from primary switch */
#define VTSS_AUTH_RADIUS_ALLOC_MAX_RETRY  10 /* times */
#define VTSS_AUTH_TACPLUS_AVPAIR_MAX      50 /* Maximum number of supported tacacs+ avpairs (including NULL terminator) */

#define VTSS_UPTIME              ((ulong)(VTSS_OS_TICK2MSEC(vtss_current_time()) / 1000)) /* Uptime in seconds */

/* ================================================================= *
 *  Configuration
 * ================================================================= */

#ifdef VTSS_SW_OPTION_RADIUS
typedef struct {
    vtss_appl_auth_radius_global_conf_t global;                                   /* RADIUS global configuration */
    vtss_appl_auth_radius_server_conf_t server[VTSS_APPL_AUTH_NUMBER_OF_SERVERS]; /* RADIUS server configuration */
} vtss_auth_radius_conf_t;
#endif /* VTSS_SW_OPTION_RADIUS */

#ifdef VTSS_SW_OPTION_TACPLUS
typedef struct {
    vtss_appl_auth_tacacs_global_conf_t global;                                   /* TACACS+ global configuration */
    vtss_appl_auth_tacacs_server_conf_t server[VTSS_APPL_AUTH_NUMBER_OF_SERVERS]; /* TACACS+ server configuration */
} vtss_auth_tacacs_conf_t;
#endif /* VTSS_SW_OPTION_TACPLUS */

typedef struct {
    vtss_appl_auth_authen_agent_conf_t authen_agent[VTSS_APPL_AUTH_AGENT_LAST];   /* Authentication agent configuration */
    vtss_appl_auth_author_agent_conf_t author_agent[VTSS_APPL_AUTH_AGENT_LAST];   /* Authorization agent configuration */
    vtss_appl_auth_acct_agent_conf_t   acct_agent[VTSS_APPL_AUTH_AGENT_LAST];     /* Accounting agent configuration */
#ifdef VTSS_SW_OPTION_RADIUS
    vtss_auth_radius_conf_t     radius;                                           /* RADIUS configuration */
#endif /* VTSS_SW_OPTION_RADIUS */
#ifdef VTSS_SW_OPTION_TACPLUS
    vtss_auth_tacacs_conf_t     tacacs;                                           /* TACACS+ configuration */
#endif /* VTSS_SW_OPTION_TACPLUS */
} vtss_auth_conf_t;

/* ================================================================= *
 *  Auth message
 *  This is the messages that is sent beween the client and the server
 *  The client sends a request and waits for a reply.
 *  The server waits for a request, validates it and sends a reply.
 * ================================================================= */

#define VTSS_AUTH_MAX_USERNAME_LENGTH VTSS_SYS_STRING_LEN
#define VTSS_AUTH_MAX_PASSWORD_LENGTH VTSS_SYS_PASSWD_LEN
#define VTSS_AUTH_MAX_COMMAND_LENGTH  256

#define VTSS_AUTH_MSG_VERSION 2

typedef enum {
    VTSS_AUTH_MSG_ID_LOGIN_REQ,
    VTSS_AUTH_MSG_ID_LOGIN_REP,
    VTSS_AUTH_MSG_ID_CMD_REQ,
    VTSS_AUTH_MSG_ID_CMD_REP,
    VTSS_AUTH_MSG_ID_LOGOUT_REQ, /* No reply for logout */
    VTSS_AUTH_MSG_ID_NEW_PRIMARY_SWITCH
} vtss_auth_msg_id_t;

typedef struct {
    u32                version;
    vtss_auth_msg_id_t msg_id;
} vtss_auth_msg_hdr_t;

typedef struct {
    vtss_isid_t            isid;    /* The isid of the client - used for remembering where the request came from */
    u32                    seq_num; /* Used for matching replies with requests */
    vtss_appl_auth_agent_t agent;   /* The agent that logs in */
    char                   hostname[INET6_ADDRSTRLEN];
    char                   username[VTSS_AUTH_MAX_USERNAME_LENGTH];
    char                   password[VTSS_AUTH_MAX_PASSWORD_LENGTH];
} vtss_auth_msg_login_req_t;

typedef struct {
    u32                    seq_num;  /* Used for matching replies with requests */
    u8                     priv_lvl; /* Assigned privilege level */
    u16                    agent_id; /* Assigned agent id */
    mesa_rc                rc;       /* Return code */
} vtss_auth_msg_login_rep_t;

typedef struct {
    vtss_isid_t            isid;         /* The isid of the client - used for remembering where the request came from */
    u32                    seq_num;      /* Used for matching replies with requests */
    vtss_appl_auth_agent_t agent;        /* The agent that issues the command */
    char                   hostname[INET6_ADDRSTRLEN];
    char                   username[VTSS_AUTH_MAX_USERNAME_LENGTH];
    char                   command[VTSS_AUTH_MAX_COMMAND_LENGTH];
    u8                     priv_lvl;     /* Agent privilege level */
    u16                    agent_id;     /* Agent id */
    u8                     cmd_priv_lvl; /* Command privilege level */
    BOOL                   cfg_cmd;      /* TRUE if configuration command */
    BOOL                   execute;      /* If FALSE, agent just wants to check the command (no accounting) */
} vtss_auth_msg_cmd_req_t;

typedef struct {
    u32                    seq_num; /* Used for matching replies with requests */
    mesa_rc                rc;      /* Return code */
} vtss_auth_msg_cmd_rep_t;

typedef struct {
    vtss_appl_auth_agent_t agent;    /* The agent that logs out */
    char                   hostname[INET6_ADDRSTRLEN];
    char                   username[VTSS_AUTH_MAX_USERNAME_LENGTH];
    u8                     priv_lvl; /* Agent privilege level */
    u16                    agent_id; /* Agent id */
} vtss_auth_msg_logout_req_t;

typedef struct {
    vtss_auth_msg_hdr_t hdr;

    union {
        /* VTSS_AUTH_MSG_ID_LOGIN_REQ: */
        vtss_auth_msg_login_req_t  login_req;

        /* VTSS_AUTH_MSG_ID_LOGIN_REP: */
        vtss_auth_msg_login_rep_t  login_rep;

        /* VTSS_AUTH_MSG_ID_CMD_REQ: */
        vtss_auth_msg_cmd_req_t    cmd_req;

        /* VTSS_AUTH_MSG_ID_CMD_REP: */
        vtss_auth_msg_cmd_rep_t    cmd_rep;

        /* VTSS_AUTH_MSG_ID_LOGOUT_REQ: */
        vtss_auth_msg_logout_req_t logout_req;

        /* VTSS_AUTH_MSG_ID_NEW_PRIMARY_SWITCH: */
        /* No extra data */
    } u;
} vtss_auth_msg_t;

/* ================================================================= *
 *  Auth message buffer and pool
 *  The pool contains an array of messages buffers.
 *  Instead of having two pools (a pool of free buffers and a pool of
 *  active buffers) all buffers are located in the same pool, and a
 *  state variable is used to indicate the current state of the buffer.
 * ================================================================= */
typedef enum {
    VTSS_AUTH_CLIENT_BUF_STATE_FREE,
    VTSS_AUTH_CLIENT_BUF_STATE_WAITING,
    VTSS_AUTH_CLIENT_BUF_STATE_RETRY,
    VTSS_AUTH_CLIENT_BUF_STATE_DONE,
} vtss_auth_client_buf_state_t;

typedef struct {
    vtss_mutex_t                  mutex;   /* buffer accesss protection */
    vtss_cond_t                   cond;    /* wait here until we get a reply */
    vtss_auth_client_buf_state_t state;   /* state and condition variable */
    u32                          seq_num; /* used for matching replies with requests */
    vtss_auth_msg_t              msg;     /* the message */
} vtss_auth_client_buf_t;

#define VTSS_AUTH_CLIENT_POOL_LENGTH 2
typedef struct {
    vtss_mutex_t            mutex;   /* pool accesss protection */
    vtss_cond_t             cond;    /* wait here until we get a buffer */
    u32                     seq_num; /* Sequence number - incremented for each client request */
    vtss_auth_client_buf_t  buf[VTSS_AUTH_CLIENT_POOL_LENGTH]; /* the buffers */
} vtss_auth_client_pool_t;


/* ================================================================= *
 *  Thread variables structure
 * ================================================================= */
static vtss::SafeQueue      auth_queue;
typedef struct {
    vtss_handle_t      handle;
    vtss_thread_t      state;
} vtss_auth_thread_t;

static vtss_handle_t service_handle;

#ifdef VTSS_SW_OPTION_RADIUS
#define VTSS_AUTH_RADIUS_VENDOR_VALUE_MAX_LENGTH 64
/* ================================================================= *
 *  RADIUS vendor specific data structure
 *  Used for decoding of vendor specific attributes
 *  See RFC 2865 section 5.26
 *  NOTE : Must be packed
 * ================================================================= */
typedef struct {
    u32 vendor_id;
    u8  vendor_type;
    u8  vendor_length;
    u8  vendor_value[VTSS_AUTH_RADIUS_VENDOR_VALUE_MAX_LENGTH];
} VTSS_AUTH_ATTRIBUTE_PACKED vtss_auth_radius_vendor_specific_data_t;

/* ================================================================= *
 *  RADIUS shared data structure
 *  Used for transferring data from AUTH_radius_rx_callback()
 *  to vtss_auth_radius()
 *  This is how the asynchronius RADIUS interface is converted to a
 *  synchronius interface
 * ================================================================= */
typedef struct {
    BOOL                             ready;   /* condition variable */
    vtss_mutex_t                      mutex;   /* shared data accesss protection */
    vtss_cond_t                       cond;    /* wait here until we get a reply */
    /* shared data follows here */
    u8                               handle;
    ctx_t                            ctx;
    vtss_radius_access_codes_e       code;
    vtss_radius_rx_callback_result_e res;
    u8                               priv_lvl;
} vtss_auth_radius_data_t;
#endif

/* ================================================================= *
 *  HTTPD authentication cache
 * ================================================================= */
#define VTSS_AUTH_HTTPD_CACHE_LIFE_TIME  60 /* seconds */
#define VTSS_AUTH_HTTPD_CACHE_SIZE       10 /* number of concurrent (different) usernames */
typedef struct {
    char  username[VTSS_AUTH_MAX_USERNAME_LENGTH];
    char  password[VTSS_AUTH_MAX_PASSWORD_LENGTH];
    u8    priv_lvl;
    u16   agent_id;
    ulong expires; /* the time in seconds from boot that entry is valid */
} vtss_auth_httpd_cache_t;

/* ================================================================= *
 *  Private variables
 * ================================================================= */
static vtss_auth_thread_t      thread;                /* Thread specific stuff */
static critd_t                 crit_config;           /* Configuration critical region protection */
static critd_t                 crit_cache;            /* HTTPD cache critical region protection */
static vtss_auth_conf_t        config;                /* Current configuration */
static BOOL                    config_changed = TRUE; /* Configuration has changed */

static vtss_auth_client_pool_t pool;                  /* Pool for client messages */

#ifdef VTSS_SW_OPTION_RADIUS
static vtss_auth_radius_data_t radius_data;           /* Radius shared data */
#endif /* VTSS_SW_OPTION_RADIUS */

#ifdef VTSS_SW_OPTION_TACPLUS
static tac_session_t           *tacacs_session = NULL;   /* Reusable TACACS+ session. Only used in AUTH_thread context */
#endif /* VTSS_SW_OPTION_TACPLUS */

// Avoid httpd_cache not used (in configuration where Web module is not included)
/*lint --e{551} */
static vtss_auth_httpd_cache_t httpd_cache[VTSS_AUTH_HTTPD_CACHE_SIZE];

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "Auth", "Authentication Module"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define AUTH_CRIT_CONFIG_ENTER() critd_enter(&crit_config, __FILE__, __LINE__)
#define AUTH_CRIT_CONFIG_EXIT()  critd_exit( &crit_config, __FILE__, __LINE__)
#define AUTH_CRIT_CACHE_ENTER()  critd_enter(&crit_cache,  __FILE__, __LINE__)
#define AUTH_CRIT_CACHE_EXIT()   critd_exit( &crit_cache,  __FILE__, __LINE__)

/* ================================================================= *
 *
 * Local functions starts here
 *
 * ================================================================= */

/* ================================================================= *
 *  AUTH_client_pool_init()
 *  Initialize the pool and buffer management.
 * ================================================================= */
static void AUTH_client_pool_init(void)
{
    T_N("enter");
    memset(&pool, 0, sizeof(pool));
    vtss_mutex_init(&pool.mutex);
    vtss_cond_init(&pool.cond, &pool.mutex);
    T_N("exit");
}

/* ================================================================= *
 *  AUTH_client_buf_find_free()
 *  This is a helper function for vtss_auth_client_buf_alloc() only.
 *  Must be called with pool mutex locked!
 *  Returns a free buffer (or NULL)
 * ================================================================= */
static vtss_auth_client_buf_t *AUTH_client_buf_find_free(void)
{
    int i;

    T_N("enter");
    for (i = 0; i < VTSS_AUTH_CLIENT_POOL_LENGTH; i++) {
        if (pool.buf[i].state == VTSS_AUTH_CLIENT_BUF_STATE_FREE) {
            T_N("exit - free buffer found");
            return &pool.buf[i];
        }
    }
    T_N("exit - no free buffer found");
    return NULL;
}

/* ================================================================= *
 *  AUTH_client_buf_alloc()
 *  Allocates a buffer for a client request.
 *  Blocks until a free buffer is available.
 * ================================================================= */
static vtss_auth_client_buf_t *AUTH_client_buf_alloc(vtss_auth_msg_id_t id)
{
    vtss_auth_client_buf_t *buf;

    T_N("enter");
    (void)vtss_mutex_lock(&pool.mutex);
    while ((buf = AUTH_client_buf_find_free()) == NULL) {
        T_N("waiting");
        (void)vtss_cond_wait(&pool.cond); /* Wait here for a free buffer */
    }

    /* Initialize buffer and part of message */
    vtss_mutex_init(&buf->mutex);
    vtss_cond_init(&buf->cond, &buf->mutex);
    buf->state           = VTSS_AUTH_CLIENT_BUF_STATE_WAITING;
    buf->seq_num         = pool.seq_num++;
    buf->msg.hdr.version = VTSS_AUTH_MSG_VERSION;
    buf->msg.hdr.msg_id  = id;

    (void)vtss_mutex_unlock(&pool.mutex);
    T_N("exit");
    return buf;
}

/* ================================================================= *
 *  AUTH_client_buf_free()
 *  Returns a buffer to the pool.
 * ================================================================= */
static void AUTH_client_buf_free(vtss_auth_client_buf_t *buf )
{
    T_N("enter");
    (void)vtss_mutex_lock(&pool.mutex);
    buf->state = VTSS_AUTH_CLIENT_BUF_STATE_FREE;
    vtss_cond_signal(&pool.cond); /* Wake up clients that waits for a buffer */
    (void)vtss_mutex_unlock(&pool.mutex);
    T_N("exit");
}

/* ================================================================= *
 *  AUTH_client_buf_lookup()
 *  Search the client request buffers, for a matching sequence_number
 *  Returns the buffer or NULL if no matching buffer is found.
 * ================================================================= */
static vtss_auth_client_buf_t *AUTH_client_buf_lookup(u32 seq_num)
{
    vtss_auth_client_buf_t *buf = NULL;
    int                    i;

    T_N("enter");
    (void)vtss_mutex_lock(&pool.mutex);
    for (i = 0; i < VTSS_AUTH_CLIENT_POOL_LENGTH; i++) {
        if ((pool.buf[i].state != VTSS_AUTH_CLIENT_BUF_STATE_FREE) && (pool.buf[i].seq_num == seq_num)) {
            buf = &pool.buf[i];
            T_N("match");
            break;
        }
    }
    (void)vtss_mutex_unlock(&pool.mutex);
    T_N("exit");
    return buf;
}

/* ================================================================= *
 *  AUTH_client_buf_unlock_all()
 *  Unlock all clients and ask them to retry the request.
 * ================================================================= */
static void AUTH_client_buf_unlock_all(void)
{
    vtss_auth_client_buf_t *buf;
    int                    i;

    T_N("enter");
    (void)vtss_mutex_lock(&pool.mutex);
    /* Unlock all waiting clients */
    for (i = 0; i < VTSS_AUTH_CLIENT_POOL_LENGTH; i++) {
        buf = &pool.buf[i];
        (void)vtss_mutex_lock(&buf->mutex);
        if (buf->state == VTSS_AUTH_CLIENT_BUF_STATE_WAITING) {
            buf->state = VTSS_AUTH_CLIENT_BUF_STATE_RETRY; /* Signal waiter to retry tx */
            T_N("vtss_cond_signal");
            vtss_cond_signal(&buf->cond);
        }
        (void)vtss_mutex_unlock(&buf->mutex);
    }
    (void)vtss_mutex_unlock(&pool.mutex);
    T_N("exit");
}

/* ================================================================= *
 *  AUTH_new_primary_switch()
 *  Send new-primary-switch notification to added switch
 * ================================================================= */
static void AUTH_new_primary_switch(vtss_isid_t isid)
{
    vtss_auth_msg_t *m = (vtss_auth_msg_t *)VTSS_MALLOC(sizeof(vtss_auth_msg_t));
    if (m) {
        m->hdr.version = VTSS_AUTH_MSG_VERSION;
        m->hdr.msg_id = VTSS_AUTH_MSG_ID_NEW_PRIMARY_SWITCH;
        msg_tx(VTSS_MODULE_ID_AUTH, isid, m, MSG_TX_DATA_HDR_LEN(vtss_auth_msg_t, u));
    } else {
        T_W("out of memory");
    }
}

/* ================================================================= *
 *  AUTH_msg_tx_done()
 *  Callback invoked when a message transmission is complete,
 *  successfully or unsuccessfully.
 * ================================================================= */
static void AUTH_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
    if (rc != MSG_TX_RC_OK && msg) {
        /* A message tx error occurred.*/
        vtss_auth_msg_t        *m   = (vtss_auth_msg_t *)msg;
        vtss_auth_client_buf_t *buf = NULL;

        T_N("msg_tx error");

        switch (m->hdr.msg_id) {
        case VTSS_AUTH_MSG_ID_LOGIN_REQ:
            buf = AUTH_client_buf_lookup(m->u.login_req.seq_num);
            break;
        case VTSS_AUTH_MSG_ID_CMD_REQ:
            buf = AUTH_client_buf_lookup(m->u.cmd_req.seq_num);
            break;
        default:
            T_E("Unexpected msg_id: %d", m->hdr.msg_id);
            break;
        }

        if (buf) {
            (void)vtss_mutex_lock(&buf->mutex);
            buf->state = VTSS_AUTH_CLIENT_BUF_STATE_RETRY; /* Signal waiter to retry tx */
            vtss_cond_signal(&buf->cond);
            (void)vtss_mutex_unlock(&buf->mutex);
        }
    }
}

/* ================================================================= *
 *  AUTH_msg_rx()
 *  Stack message callback function
 * ================================================================= */
static BOOL AUTH_msg_rx(void *contxt, const void *const rx_msg, const size_t len,
                        const vtss_module_id_t modid, const u32 isid)
{
    vtss_auth_msg_t *msg = (vtss_auth_msg_t *)rx_msg;
    T_N("msg received: len: %d, modid: %d, isid: %d", (int)len, modid, (int)isid);

    // Check if we support this version of the message. If not, print a warning and return.
    if (msg->hdr.version != VTSS_AUTH_MSG_VERSION) {
        T_W("Unsupported version of the message (%u)", msg->hdr.version);
        return TRUE;
    }

    switch (msg->hdr.msg_id) {
    case VTSS_AUTH_MSG_ID_LOGIN_REQ: {
        T_N("msg VTSS_AUTH_MSG_ID_LOGIN_REQ received");
        if (msg_switch_is_primary()) {
            vtss_auth_msg_t *m = (vtss_auth_msg_t *)VTSS_MALLOC(sizeof(vtss_auth_msg_t));
            if (m) {
                memcpy(m, msg, MSG_TX_DATA_HDR_LEN(vtss_auth_msg_t, u) + sizeof(m->u.login_req)); /* make a copy of the message, */
                m->u.login_req.isid = isid;                                                       /* remember where it came from */
                T_N("vtss_safe_queue_put");
                (void)auth_queue.vtss_safe_queue_put(m);                                       /* hand it over to the auth_thread */
            } else {
                T_W("out of memory");
            }
        } else {
            T_W("server request received on a secondary switch");
        }
        break;
    }

    case VTSS_AUTH_MSG_ID_LOGIN_REP: {
        vtss_auth_client_buf_t *buf;

        T_N("msg VTSS_AUTH_MSG_ID_LOGIN_REP received");
        buf = AUTH_client_buf_lookup(msg->u.login_rep.seq_num);

        if (buf) { /* if a match is found then copy return parameters and unblock the client */
            (void)vtss_mutex_lock(&buf->mutex);
            buf->msg.hdr = msg->hdr;
            buf->msg.u.login_rep  = msg->u.login_rep;
            buf->state = VTSS_AUTH_CLIENT_BUF_STATE_DONE; /* Signal waiter we are done */
            T_N("vtss_cond_signal");
            vtss_cond_signal(&buf->cond);
            (void)vtss_mutex_unlock(&buf->mutex);
        }
        break;
    }

    case VTSS_AUTH_MSG_ID_CMD_REQ: {
        T_N("msg VTSS_AUTH_MSG_ID_CMD_REQ received");
        if (msg_switch_is_primary()) {
            vtss_auth_msg_t *m = (vtss_auth_msg_t *)VTSS_MALLOC(sizeof(vtss_auth_msg_t));
            if (m) {
                memcpy(m, msg, MSG_TX_DATA_HDR_LEN(vtss_auth_msg_t, u) + sizeof(m->u.cmd_req)); /* make a copy of the message, */
                m->u.cmd_req.isid = isid;                                                       /* remember where it came from */
                T_N("vtss_safe_queue_put");
                (void)auth_queue.vtss_safe_queue_put(m);                                      /* hand it over to the auth_thread */
            } else {
                T_W("out of memory");
            }
        } else {
            T_W("server request received on a secondary switch");
        }
        break;
    }

    case VTSS_AUTH_MSG_ID_CMD_REP: {
        vtss_auth_client_buf_t *buf;

        T_N("msg VTSS_AUTH_MSG_ID_CMD_REP received");
        buf = AUTH_client_buf_lookup(msg->u.cmd_rep.seq_num);

        if (buf) { /* if a match is found then copy return parameters and unblock the client */
            (void)vtss_mutex_lock(&buf->mutex);
            buf->msg.hdr = msg->hdr;
            buf->msg.u.cmd_rep  = msg->u.cmd_rep;
            buf->state = VTSS_AUTH_CLIENT_BUF_STATE_DONE; /* Signal waiter we are done */
            T_N("vtss_cond_signal");
            vtss_cond_signal(&buf->cond);
            (void)vtss_mutex_unlock(&buf->mutex);
        }
        break;
    }

    case VTSS_AUTH_MSG_ID_LOGOUT_REQ: {
        T_N("msg VTSS_AUTH_MSG_ID_LOGOUT_REQ received");
        if (msg_switch_is_primary()) {
            vtss_auth_msg_t *m = (vtss_auth_msg_t *)VTSS_MALLOC(sizeof(vtss_auth_msg_t));
            if (m) {
                memcpy(m, msg, MSG_TX_DATA_HDR_LEN(vtss_auth_msg_t, u) + sizeof(m->u.logout_req)); /* make a copy of the message, */
                T_N("vtss_safe_queue_put");
                (void)auth_queue.vtss_safe_queue_put(m);                                      /* hand it over to the auth_thread */
            } else {
                T_W("out of memory");
            }
        } else {
            T_W("server request received on a secondary switch");
        }
        break;
    }

    case VTSS_AUTH_MSG_ID_NEW_PRIMARY_SWITCH:
        T_N("msg VTSS_AUTH_MSG_ID_NEW_PRIMARY_SWITCH received");
        AUTH_client_buf_unlock_all();
        break;

    default:
        T_W("Unknown message ID: %d", msg->hdr.msg_id);
        break;
    }
    return TRUE;
}

#if defined(VTSS_SW_OPTION_WEB)
/* ================================================================= *
 *  AUTH_httpd_cache_lookup()
 *  Lookup an entry in the cache.
 *  This function will ALWAYS return an entry.
 *  The return code shows what kind of entry it is:
 *  VTSS_RC_OK:
 *    The entry is valid and has not expired. The caller must update
 *    the expire time.
 *
 *  VTSS_APPL_AUTH_ERROR_CACHE_EXPIRED:
 *    The entry matches username and password but is expired.
 *    The caller must reauthenticate username and password.
 *    If authentication is ok, the caller must update the userlevel
 *    and the expire time.
 *
 *  VTSS_APPL_AUTH_ERROR_CACHE_INVALID:
 *    The entry does not match username and password.
 *    The caller must reauthenticate username and password.
 *    If authentication is ok, the caller must update the username,
 *    the password, the userlevel and the expire time.
 *
 *  By using this strategy, we will only have to lookup once.
 *  Remember that this function is often called more than one time
 *  for each http request.
 * ================================================================= */
static mesa_rc AUTH_httpd_cache_lookup(char *username, char *password, vtss_auth_httpd_cache_t **entry)
{
    int i;
    int oldest_index = 0;
    ulong oldest = ULONG_MAX;
    int username_index = -1;
    ulong time_now = VTSS_UPTIME;

    for (i = 0; i < VTSS_AUTH_HTTPD_CACHE_SIZE; i++) {
        if (httpd_cache[i].expires < oldest) { /* remember the oldest entry in case of no match at all */
            oldest = httpd_cache[i].expires;
            oldest_index = i;
        }
        if (strncmp(httpd_cache[i].username, username, VTSS_AUTH_MAX_USERNAME_LENGTH) == 0) {
            username_index = i; /* remember that we have found a username match */
            if (strncmp(httpd_cache[i].password, password, VTSS_AUTH_MAX_PASSWORD_LENGTH) == 0) {
                *entry = &httpd_cache[i];
                if (time_now <= httpd_cache[i].expires) { /* match and not expired */
                    return VTSS_RC_OK;
                } else { /* match but expired */
                    return VTSS_APPL_AUTH_ERROR_CACHE_EXPIRED;
                }
            }
        }
    }

    if (username_index != -1) { /* we found an entry that matches the username - reuse this one */
        *entry = &httpd_cache[username_index];
    } else { /* no match at all - return the oldest entry in the cache */
        *entry = &httpd_cache[oldest_index];
    }
    return VTSS_APPL_AUTH_ERROR_CACHE_INVALID;
}

/* ================================================================= *
 *  AUTH_httpd_authenticate()
 *  Authenticate http requests via cache
 *  Returns 1 on success, 0 on error
 * ================================================================= */
static int AUTH_httpd_authenticate(char *username, char *password, int *priv_lvl)
{
    vtss_auth_httpd_cache_t *entry;
    u8                      lvl;
    u16                     agent_id = 0;
    mesa_rc                 rc;
    int                     status;

    T_D("Authenticating '%s'", username);

#if !defined(VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR)
    if (!username[0]) {
        T_D("Auth failure (empty username)");
        return 0;
    }
#endif /* !VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR */

    /* The invalid username "~" is reserved for logging out from the browser */
    if ((username[0] == '~') && !username[1]) {
        T_D("Auth failure (reserved username)");
        return 0;
    }

    AUTH_CRIT_CACHE_ENTER();
    if ((rc = AUTH_httpd_cache_lookup(username, password, &entry)) == VTSS_RC_OK) {
        entry->expires = VTSS_UPTIME + VTSS_AUTH_HTTPD_CACHE_LIFE_TIME;
        *priv_lvl = entry->priv_lvl;
        T_D("Match and not expired");
        status = 1;
    } else {
        if (rc == VTSS_APPL_AUTH_ERROR_CACHE_EXPIRED) {
            T_D("Match but expired");
        } else {
            T_D("No match");
        }

        if (vtss_auth_login(VTSS_APPL_AUTH_AGENT_HTTP, NULL, username, password, &lvl, &agent_id) == VTSS_RC_OK) {
            entry->expires = VTSS_UPTIME + VTSS_AUTH_HTTPD_CACHE_LIFE_TIME;
            entry->priv_lvl = lvl;
            entry->agent_id = agent_id;
            *priv_lvl = lvl;
            if (rc == VTSS_APPL_AUTH_ERROR_CACHE_INVALID) {
                misc_strncpyz(entry->username, username, VTSS_AUTH_MAX_USERNAME_LENGTH);
                misc_strncpyz(entry->password, password, VTSS_AUTH_MAX_PASSWORD_LENGTH);
            }
            status = 1;
        } else {
            T_D("Auth failure");
            status = 0;
        }
    }
    AUTH_CRIT_CACHE_EXIT();
    return status;
}
#endif /* VTSS_SW_OPTION_WEB */

/* ================================================================= *
 *  AUTH_httpd_cache_expire()
 *  Expire everything in the cache.
 *  Called when configuration is changed in order to force a
 *  reauthentication in the web browser.
 * ================================================================= */
static void AUTH_httpd_cache_expire(void)
{
    int i;

    T_N("enter");
    AUTH_CRIT_CACHE_ENTER();
    for (i = 0; i < VTSS_AUTH_HTTPD_CACHE_SIZE; i++) {
        httpd_cache[i].expires = 0;
    }
    AUTH_CRIT_CACHE_EXIT();
    T_N("exit");
}

/* ================================================================= *
 *  AUTH_local_authenticate()
 *  Authenticate via the local database.
 * ================================================================= */
static mesa_rc AUTH_local_authenticate(const char *username,
                                       const char *passwd,
                                       u8 *priv_lvl)
{
    mesa_rc rc = VTSS_RC_OK;
#ifdef VTSS_SW_OPTION_USERS
    users_conf_t conf;
#endif

    T_N("enter");

#ifdef VTSS_SW_OPTION_USERS
    memset(&conf, 0, sizeof(conf));
    misc_strncpyz(conf.username, username, sizeof(conf.username));
    misc_strncpyz(conf.password, passwd, sizeof(conf.password));
    conf.encrypted = FALSE; // Identify the password is uncrypted
    if (!vtss_users_mgmt_verify_clear_password(conf.username, conf.password) || vtss_users_mgmt_conf_get(&conf, FALSE) != VTSS_RC_OK) {
        rc = VTSS_APPL_AUTH_ERROR_SERVER_REJECT;
    } else {
        *priv_lvl = conf.privilege_level;
    }
#else
    if ((strncmp(username, VTSS_SYS_ADMIN_NAME, VTSS_AUTH_MAX_USERNAME_LENGTH)) ||
        !system_clear_password_verify((char *)passwd)) {
        rc = VTSS_APPL_AUTH_ERROR_SERVER_REJECT;
    }
    *priv_lvl = 15;
#endif
    T_I("priv_lvl is set to %d", *priv_lvl);

    if (rc == VTSS_RC_OK) {
        S_I("User %s logged in as local user (priv level %d)", username, *priv_lvl);
    }

    T_N("exit");
    return rc;
}

#if defined(VTSS_SW_OPTION_RADIUS) || defined(VTSS_SW_OPTION_TACPLUS) || defined(VTSS_SW_OPTION_SNMP)
/* Encrypt the plain text of secret key with AES256 cryptography.
 * Or Decrypt the encrypted hex string of the secret key to plain text.
 * Return VTSS_RC_OK when encryption successfully, else error code.
 */
mesa_rc AUTH_secret_key_cryptography(BOOL is_encrypt, char *plain_txt, char *hex_str)
{
    char key[VTSS_APPL_AUTH_UNENCRYPTED_KEY_LEN];

    memset(key, 0, sizeof(key));
    strncpy(key, VTSS_SYS_PASSWORD_MAGIC_STR, sizeof(key) - 1);

    if (is_encrypt) {
        return vtss_aes256_encrypt(plain_txt, (u8 *)key, (u32)strlen(key), VTSS_APPL_AUTH_KEY_LEN, hex_str);
    } else {
        return vtss_aes256_decrypt(hex_str, (u8 *)key, (u32)strlen(key), VTSS_APPL_AUTH_UNENCRYPTED_KEY_LEN, plain_txt);
    }
}

/* Validate the secret key is valid or not.
 * Return TRUE when the secret key is valid, otherwise FALSE
 */
BOOL AUTH_validate_secret_key(BOOL is_encrypted, const char *key)
{
    size_t  len = strlen(key);
    int     i;

    if (is_encrypted) {
        if (len % 16 || (len < (VTSS_APPL_AUTH_ENCRYPTED_KEY_LEN(0) - 1))) {
            // Not match the length of AES256 blocks
            return FALSE;
        }
        for (i = 0; i < len; ++i) {
            if (!((key[i] >= '0' && key[i] <= '9') || (key[i] >= 'A' && key[i] <= 'F') || (key[i] >= 'a' && key[i] <= 'f'))) {
                // Not hex character
                return FALSE;
            }
        }
    } else if (len >= VTSS_APPL_AUTH_UNENCRYPTED_KEY_LEN) {
        // Out of the max. length of palin text
        return FALSE;
    }
    return TRUE;
}
#endif /*VTSS_SW_OPTION_RADIUS || VTSS_SW_OPTION_TACPLUS */

#ifdef VTSS_SW_OPTION_RADIUS
/* ================================================================= *
 *  AUTH_radius_init()
 *  Initialize RADIUS specific data
 * ================================================================= */
static void AUTH_radius_init(void)
{
    T_N("enter");
    memset(&radius_data, 0, sizeof(vtss_auth_radius_data_t));
    vtss_mutex_init(&radius_data.mutex);
    vtss_cond_init(&radius_data.cond, &radius_data.mutex);
    T_N("exit");
}

/* ================================================================= *
 *  AUTH_radius_config_update()
 *  Update RADIUS configuration
 * ================================================================= */
static mesa_rc AUTH_radius_config_update(const vtss_auth_radius_conf_t *conf)
{
    vtss_radius_cfg_s radius_cfg;
    mesa_rc rc = VTSS_RC_OK;
    int i;
    char global_plain_txt[VTSS_APPL_AUTH_UNENCRYPTED_KEY_LEN];
    char server_plain_txt[VTSS_APPL_AUTH_UNENCRYPTED_KEY_LEN];
    char encrypted_hex_str[VTSS_APPL_AUTH_KEY_LEN];

    T_N("enter");
    global_plain_txt[0] = 0;        // Empty string, our default key
    memset(&radius_cfg, 0, sizeof(radius_cfg));
    if (conf->global.encrypted) {
        strcpy(encrypted_hex_str, conf->global.key);
        if ((rc = AUTH_secret_key_cryptography(FALSE, global_plain_txt, encrypted_hex_str)) != VTSS_RC_OK) {
            T_N("exit: Radius global key decrypt failed");
            return rc;
        }
    }
    for (i = 0; i < VTSS_APPL_AUTH_NUMBER_OF_SERVERS; i++) {
        if (conf->server[i].host[0]) {
            server_plain_txt[0] = 0;        // Empty string, our default key
            if (conf->server[i].encrypted) {
                strcpy(encrypted_hex_str, conf->server[i].key);
                if ((rc = AUTH_secret_key_cryptography(FALSE, server_plain_txt, encrypted_hex_str)) != VTSS_RC_OK) {
                    T_N("Radius server %d key decrypt failed", i);
                    break;
                }
            }
            if (conf->server[i].auth_port) {
                misc_strncpyz(radius_cfg.servers_auth[i].host, conf->server[i].host, sizeof(radius_cfg.servers_auth[i].host));
                radius_cfg.servers_auth[i].port = conf->server[i].auth_port;
                radius_cfg.servers_auth[i].timeout = conf->server[i].timeout ? conf->server[i].timeout : conf->global.timeout;
                radius_cfg.servers_auth[i].retransmit = conf->server[i].retransmit ? conf->server[i].retransmit : conf->global.retransmit;
                misc_strncpyz(radius_cfg.servers_auth[i].key, server_plain_txt[0] ? server_plain_txt : global_plain_txt, sizeof(radius_cfg.servers_auth[i].key));
            }
            if (conf->server[i].acct_port) {
                misc_strncpyz(radius_cfg.servers_acct[i].host, conf->server[i].host, sizeof(radius_cfg.servers_acct[i].host));
                radius_cfg.servers_acct[i].port = conf->server[i].acct_port;
                radius_cfg.servers_acct[i].timeout = conf->server[i].timeout ? conf->server[i].timeout : conf->global.timeout;
                radius_cfg.servers_acct[i].retransmit = conf->server[i].retransmit ? conf->server[i].retransmit : conf->global.retransmit;
                misc_strncpyz(radius_cfg.servers_acct[i].key, server_plain_txt[0] ? server_plain_txt : global_plain_txt, sizeof(radius_cfg.servers_acct[i].key));
            }
        }
    }
    radius_cfg.dead_time_secs = conf->global.deadtime * 60;
    radius_cfg.nas_ip_address_enable = conf->global.nas_ip_address_enable;
    radius_cfg.nas_ip_address = htonl(conf->global.nas_ip_address);
    radius_cfg.nas_ipv6_address_enable = conf->global.nas_ipv6_address_enable;
    radius_cfg.nas_ipv6_address = conf->global.nas_ipv6_address;
    misc_strncpyz(radius_cfg.nas_identifier, conf->global.nas_identifier, sizeof(radius_cfg.nas_identifier));

    if (msg_switch_is_primary()) {
        rc = vtss_radius_cfg_set(&radius_cfg);
    }
    T_N("exit");
    return rc;
}

/* ================================================================= *
 *  AUTH_radius_rx_callback()
 *  Called when the RADIUS module receives a response
 * ================================================================= */
static void AUTH_radius_rx_callback(u8 handle, ctx_t ctx, vtss_radius_access_codes_e code, vtss_radius_rx_callback_result_e res, u32 server_id)
{
    T_N("enter");
    (void)vtss_mutex_lock(&radius_data.mutex);
    /* copy the data */
    radius_data.ready     = TRUE;
    radius_data.handle    = handle;
    radius_data.ctx       = ctx;
    radius_data.code      = code;
    radius_data.res       = res;
    radius_data.priv_lvl  = 1;

    if ((res == VTSS_RADIUS_RX_CALLBACK_OK) && (code == VTSS_RADIUS_CODE_ACCESS_ACCEPT)) { // Check if the response contains a privilege level
        vtss_radius_attributes_e                       type;
        u8                                             len;
        const vtss_auth_radius_vendor_specific_data_t *val;
        BOOL                                           found = FALSE;
        BOOL                                           first = TRUE;

        while (vtss_radius_tlv_iterate(handle, &type, &len, (const u8 **)&val, first) == VTSS_RC_OK) {
            // Next time, get the next TLV
            first = FALSE;
            if ((type == VTSS_RADIUS_ATTRIBUTE_VENDOR_SPECIFIC) && (len <= sizeof(vtss_auth_radius_vendor_specific_data_t))) {
                char buf[VTSS_AUTH_RADIUS_VENDOR_VALUE_MAX_LENGTH];
                if ((((ntohl(val->vendor_id) == 9) && (val->vendor_type == 1)) ||     // vendor_id 9 is Cisco and vendor_type 1 is cisco-avpair
                     ((ntohl(val->vendor_id) == 890) && (val->vendor_type == 3))) &&  // vendor_id 890 is Zyxel and vendor_type 3 is Zyxel-Privilege-avpair
                    ((val->vendor_length - 2) < (u8)sizeof(buf))) {  // Vendor length is within our limits and there is enough space for null termination
                    // Vendor length includes type and length so the length of value is length - 2
                    int i;
                    memset(buf, 0, sizeof(buf));
                    memcpy(buf, val->vendor_value, val->vendor_length - 2); // Make a modifiable null terminated copy of vendor value
                    for (i = 0; i < (int)strlen(buf); i++) {
                        buf[i] = tolower(buf[i]);
                    }
                    // vendor_value syntax: "shell:priv-lvl=x" where x is an integer from 0 to 15
                    T_D("vi %u, vt %u, vl %u, vv %s", ntohl(val->vendor_id), val->vendor_type, val->vendor_length, buf);
                    if (strstr(buf, "priv-lvl")) {
                        char *c = strrchr(buf, '=');
                        if (c) {
                            i = atoi(++c);                // fetch the number right after the last "="
                            if (i < 0) {
                                T_I("priv_lvl is out of range %d", i);
                                radius_data.priv_lvl = 0;
                            } else if (i > 15) {
                                T_I("priv_lvl is out of range %d", i);
                                radius_data.priv_lvl = 15;
                            } else {
                                T_I("priv_lvl is set to %d", i);
                                radius_data.priv_lvl = i;
                            }
                            found = TRUE;
                        } else {
                            T_I("missing \"=\" in %s", buf);
                        }
                    }
                }
            }
        }
        if (!found) {
            T_I("avpair \"priv-lvl=x\" not returned from RADIUS server");
        }
    }
    /* and wake him up */
    vtss_cond_signal(&radius_data.cond);
    (void)vtss_mutex_unlock(&radius_data.mutex);
    T_N("exit");
}

/* ================================================================= *
 *  AUTH_radius_authenticate()
 *  Authenticate via RADIUS
 * ================================================================= */
static mesa_rc AUTH_radius_authenticate(vtss_appl_auth_agent_t agent,
                                        const char *hostname,
                                        const char *username,
                                        const char *passwd,
                                        u8 *priv_lvl)
{
    int      i;
    mesa_rc  rc;
    u8       handle;

#ifdef VTSS_AUTH_CHECK_HEAP
    struct mallinfo m_before;
    struct mallinfo m_after;
    int m_loss;
    m_before = mallinfo();
#endif

    T_N("enter");

    for (i = 0; i < VTSS_AUTH_RADIUS_ALLOC_MAX_RETRY; i++) {
        // Allocate a RADIUS handle (i.e. a RADIUS ID).
        if ((rc = vtss_radius_alloc(&handle, VTSS_RADIUS_CODE_ACCESS_REQUEST)) == VTSS_RC_OK) {
            T_N("got a RADIUS handle");
            break;
        }
        VTSS_OS_MSLEEP(100); // Allow some handlers to be returned to RADIUS
    }

    if (i == VTSS_AUTH_RADIUS_ALLOC_MAX_RETRY) {
        T_I("Got \"%s\" from vtss_radius_alloc()", error_txt(rc));
        S_I("%s", error_txt(rc));
        return VTSS_APPL_AUTH_ERROR_SERVER_ERROR;
    }

    // Add the required attributes
    if (((rc = vtss_radius_tlv_set(handle, VTSS_RADIUS_ATTRIBUTE_USER_NAME,     strlen(username), (u8 *)username, TRUE)) != VTSS_RC_OK) ||
        ((rc = vtss_radius_tlv_set(handle, VTSS_RADIUS_ATTRIBUTE_USER_PASSWORD, strlen(passwd),   (u8 *)passwd,   TRUE)) != VTSS_RC_OK)) {
        T_I("Got \"%s\" from vtss_tlv_set() on required attribute", error_txt(rc));
        return VTSS_APPL_AUTH_ERROR_SERVER_ERROR;
    }

    (void)vtss_mutex_lock(&radius_data.mutex);
    radius_data.ready = FALSE;

    // Transmit the RADIUS frame and ask to be called back whenever a response arrives.
    // The RADIUS module takes care of retransmitting, changing server, etc.
    if ((rc = vtss_radius_tx(handle, ctx_t(), AUTH_radius_rx_callback, 0)) != VTSS_RC_OK) {
        T_I("vtss_radius_tx() returned \"%s\" (%d)", error_txt(rc), rc);
    } else {
        while (radius_data.ready == FALSE) {
            /* We know that the callback will be called, so it is safe to wait without timeout */
            T_D("waiting for callback");
            (void)vtss_cond_wait(&radius_data.cond);
        }
        if (radius_data.res == VTSS_RADIUS_RX_CALLBACK_OK) {
            if (radius_data.code == VTSS_RADIUS_CODE_ACCESS_ACCEPT) {
                *priv_lvl = radius_data.priv_lvl;
                rc = VTSS_RC_OK;
                S_I("User %s logged in via RADIUS (priv level %d)", username, *priv_lvl);
            } else {
                rc = VTSS_APPL_AUTH_ERROR_SERVER_REJECT;
            }
        } else {
            T_I("AUTH_radius_rx_callback() returned %d", radius_data.res);
            rc = VTSS_APPL_AUTH_ERROR_SERVER_ERROR;
        }
    }
    (void)vtss_mutex_unlock(&radius_data.mutex);

#ifdef VTSS_AUTH_CHECK_HEAP
    sleep(1); // Give the memory a little time to get freed
    m_after = mallinfo();
    m_loss = m_before.fordblks - m_after.fordblks;
    if (m_loss) {
        T_E("Lost %d (0x%x) bytes in Radius client", m_loss, m_loss);
    }
#endif

    T_N("exit");
    return rc;
}
#endif /* VTSS_SW_OPTION_RADIUS */

#ifdef VTSS_SW_OPTION_TACPLUS
/* ================================================================= *
 *  AUTH_tacplus_cmd2av()
 *  Convert a command string to an avpair array.
 *  Uses a temporary buffer to hold the resulting strings.
 *  If id is non NULL, the first av entry is set to task_id=xxxxx.
 *
 *  Example:
 *  cmd: "show running-config all-defaults"
 *  buf: "service=shell\0cmd=show\0cmd-arg=running-config\0cmd-arg=all-defaults\0"
 *  av[0] points to service=shell\0 in buf
 *  av[1] points to cmd=show\0 in buf
 *  av[2] points to cmd-arg=running-config\0 in buf
 *  av[3] points to cmd-arg=all-defaults\0 in buf
 *  av[4] is NULL
 *
 *  Return error if there isn't enough space in buf or av
 * ================================================================= */
static mesa_rc AUTH_tacplus_cmd2av(const u16 *id, const char *cmd, char *buf, u32 buf_len, char **av, u32 av_len)
{
    mesa_rc rc      = VTSS_RC_OK;
    char   *buf_end = buf + buf_len - 1; /* Points to last valid entry */
    char   **av_end = av  + av_len  - 1; /* Points to last valid entry */

    if (id) {
        if (((14 + buf) > buf_end) || (av >= av_end)) { /* 14 is "task_id=xxxxx" including zero termination */
            T_E("Out of buf or av");
            return VTSS_RC_ERROR;
        }
        sprintf(buf, "task_id=%05d", *id);
        *av++ = buf;
        buf += 14;
    }

    if (((14 + buf) > buf_end) || (av >= av_end)) { /* 14 is "service=shell" including zero termination */
        T_E("Out of buf or av");
        return VTSS_RC_ERROR;
    }
    strcpy(buf, "service=shell");
    *av++ = buf;
    buf += 14;

    if (((5 + buf) > buf_end) || (av >= av_end)) { /* 5 is "cmd=" including zero termination */
        T_E("Out of buf or av");
        return VTSS_RC_ERROR;
    }
    strcpy(buf, "cmd=");
    *av++ = buf;
    buf += 4; /* Point right after "=" */

    if (cmd) {
        while (1) {
            if ((*cmd == ' ') || (*cmd == 0)) {
                *buf++ = 0;
                if (*cmd == ' ') {
                    cmd++;
                    if (((9 + buf) > buf_end) || (av >= av_end)) { /* 9 is "cmd-arg=" including zero termination */
                        T_I("Out of buf or av");
                        rc = VTSS_RC_ERROR;
                        break;
                    }
                    strcpy(buf, "cmd-arg=");
                    *av++ = buf;
                    buf += 8; /* Point right after "=" */
                }
                if (*cmd == 0) {
                    T_D("Cmd done");
                    break;
                }
            } else {
                if ((1 + buf) > buf_end) {
                    *buf = 0;
                    T_I("Out of buf");
                    rc = VTSS_RC_ERROR;
                    break;
                }
                *buf++ = *cmd++;
            }
        }
    }

    if (av <= av_end) {
        *av = NULL;
    } else {
        T_E("No room for zero");
        *av_end = NULL;
        rc = VTSS_RC_ERROR;
    }

    return rc;
}

/* ================================================================= *
 *  AUTH_tac_connect()
 *  Connect to one of the configured TACACS+ servers.
 *  Handles dead time.
 *  If only one server is configured, dead time is ignored.
 * ================================================================= */
static tac_session_t *AUTH_tac_connect(const vtss_auth_tacacs_conf_t *conf)
{
    static ulong  deadtime[VTSS_APPL_AUTH_NUMBER_OF_SERVERS];
    ulong         time_now = VTSS_UPTIME;
    int           host_count = 0;
    int           i;
    tac_session_t *session = NULL;
    char          global_plain_txt[VTSS_APPL_AUTH_UNENCRYPTED_KEY_LEN];
    char          server_plain_txt[VTSS_APPL_AUTH_UNENCRYPTED_KEY_LEN];
    char          encrypted_hex_str[VTSS_APPL_AUTH_KEY_LEN];

    T_N("enter");

    global_plain_txt[0] = 0;        // Empty string, our default key
    if (conf->global.encrypted) {
        strcpy(encrypted_hex_str, conf->global.key);
        if (AUTH_secret_key_cryptography(FALSE, global_plain_txt, encrypted_hex_str) != VTSS_RC_OK) {
            T_D("exit: TACACS global key decrypt failed");
            return NULL;
        }
    }

    for (i = 0; i < VTSS_APPL_AUTH_NUMBER_OF_SERVERS; i++) {
        if (conf->server[i].host[0]) {
            host_count++;
        } else {
            break;
        }
    }

    for (i = 0; i < host_count; i++) {
        if (conf->server[i].host[0]) {
            const vtss_appl_auth_tacacs_server_conf_t *h = &conf->server[i];
            T_D("server %d (%s) is configured - time is now " VPRIlu, i + 1, h->host, time_now);

            server_plain_txt[0] = 0;        // Empty string, our default key
            if (h->encrypted) {
                strcpy(encrypted_hex_str, h->key);
                if (AUTH_secret_key_cryptography(FALSE, server_plain_txt, encrypted_hex_str) != VTSS_RC_OK) {
                    T_D("server %d (%s): Failed to decrypt key; skipping", i + 1, h->host);
                    continue;
                }
            }

            if ((host_count == 1) || (time_now >= deadtime[i])) { /* only one server or we have passed the last dead time - try it */
                session = tac_connect(h->host,
                                      h->port,
                                      !h->no_single,
                                      h->timeout ? h->timeout : conf->global.timeout,
                                      server_plain_txt[0] ? server_plain_txt : global_plain_txt[0] ? global_plain_txt : NULL);
                if (session) { /* It's alive */
                    T_D("connected to server %d (%s)", i + 1, h->host);
                    break;
                } else { /* It's dead. Store the next time we are allowed to contact it */
                    T_D("unable to connect to server %d (%s) - will wakeup at " VPRIlu, i + 1, h->host, time_now + (conf->global.deadtime * 60));
                    deadtime[i] = time_now + (conf->global.deadtime * 60);
                }
            } else {
                T_D("server %d (%s) - dead timer is active until " VPRIlu, i + 1, h->host, deadtime[i]);
            }
        }
    }
    if (host_count == 0) {
        T_I("No TACACS+ servers configured");
    }
    T_N("exit");
    return session;
}

/* ================================================================= *
 *  AUTH_tacplus_authenticate()
 *  Authenticate via TACACS+
 * ================================================================= */
static mesa_rc AUTH_tacplus_authenticate(const vtss_auth_tacacs_conf_t *conf,
                                         vtss_appl_auth_agent_t        agent,
                                         const char                    *hostname,
                                         const char                    *username,
                                         const char                    *passwd,
                                         u8                            *priv_lvl)
{
    mesa_rc rc = VTSS_APPL_AUTH_ERROR_SERVER_ERROR;
    int     i, tac_rc;
    int     single = 0; /* No support of single connection unless set to TRUE by first response from the TACACS+ server */
    char    serv_msg[256];
    char    data_msg[256];
    char    *avpair[VTSS_AUTH_TACPLUS_AVPAIR_MAX];
    BOOL    found = FALSE;

#ifdef VTSS_AUTH_CHECK_HEAP
    struct mallinfo m_before;
    struct mallinfo m_after;
    int m_loss;
    m_before = mallinfo();
#endif

    T_N("enter");

    *priv_lvl = 1;

    if (tacacs_session == NULL) { /* Open a session */
        if ((tacacs_session = AUTH_tac_connect(conf)) == NULL) {
            T_I("Unable to connect to TACACS+ server");
            S_I("Unable to connect to TACACS+ server");
            goto do_exit;
        }
    }

    /* Authentication */
    if (tac_authen_send_start(tacacs_session,
                              TACACS_ASCII_LOGIN,
                              username,
                              vtss_appl_auth_agent_name(agent),
                              hostname,
                              "") == 0) {
        T_W("tac_authen_send_start() failure");
        goto do_exit;
    }

    if ((tac_rc = tac_authen_get_reply(tacacs_session,
                                       &single,
                                       serv_msg,
                                       sizeof(serv_msg),
                                       data_msg,
                                       sizeof(data_msg))) <= 0) {
        T_W("tac_authen_get_reply() 1 failure");
        goto do_exit;
    }
    T_D("returned from get reply 1 = %s, \"%s\", \"%s\", %ssingle", tac_print_authen_status(tac_rc), serv_msg, data_msg, single ? "" : "No ");

    if (tac_authen_send_cont(tacacs_session,
                             passwd,
                             "") == 0) {
        T_W("tac_authen_send_cont() failure");
        goto do_exit;
    }
    if ((tac_rc = tac_authen_get_reply(tacacs_session,
                                       NULL,
                                       serv_msg,
                                       sizeof(serv_msg),
                                       data_msg,
                                       sizeof(data_msg))) <= 0) {
        T_W("tac_authen_get_reply() 2 failure");
        goto do_exit;
    }
    T_D("return from get reply 2 = %s, \"%s\", \"%s\"", tac_print_authen_status(tac_rc), serv_msg, data_msg);

    if (tac_rc != TAC_PLUS_AUTHEN_STATUS_PASS) { /* Authentication denied */
        rc = VTSS_APPL_AUTH_ERROR_SERVER_REJECT;
        goto do_exit;
    }

    if (!single && tacacs_session) { /* Reopen the session */
        tac_close(tacacs_session);
        if ((tacacs_session = AUTH_tac_connect(conf)) == NULL) {
            T_I("Unable to reconnect to TACACS+ server");
            goto do_exit;
        }
    }

    if (AUTH_tacplus_cmd2av(NULL,
                            NULL,
                            serv_msg,
                            sizeof(serv_msg),
                            avpair,
                            VTSS_AUTH_TACPLUS_AVPAIR_MAX) != VTSS_RC_OK) {
        T_W("AUTH_tacplus_cmd2av() failure");
        goto do_exit;
    }
    for (i = 0; avpair[i] != NULL; i++) {
        T_D("avpair[%d] \"%s\"", i, avpair[i]);
    }
    T_D("avpair[%d] NULL", i);

    if (tac_author_send_request(tacacs_session,
                                TAC_PLUS_AUTHEN_METH_TACACSPLUS,
                                TAC_PLUS_PRIV_LVL_MIN,
                                TAC_PLUS_AUTHEN_TYPE_ASCII,
                                TAC_PLUS_AUTHEN_SVC_LOGIN,
                                username,
                                vtss_appl_auth_agent_name(agent),
                                hostname,
                                avpair,
                                VTSS_AUTH_TACPLUS_AVPAIR_MAX) == 0) {
        T_W("tac_author_send_request() failure");
        goto do_exit;
    }

    if ((tac_rc = tac_author_get_response(tacacs_session,
                                          NULL,
                                          serv_msg,
                                          sizeof(serv_msg),
                                          data_msg,
                                          sizeof(data_msg),
                                          avpair,
                                          VTSS_AUTH_TACPLUS_AVPAIR_MAX)) <= 0) {
        T_W("tac_author_get_response() failure");
        goto do_exit;
    }
    T_D("returned from get response = %s, \"%s\", \"%s\"", tac_print_author_status(tac_rc), serv_msg, data_msg);

    for (i = 0; avpair[i] != NULL; i++) {
        int lvl;
        T_D("avpair[%d] \"%s\"", i, avpair[i]);
        // avpair syntax: "priv-lvl=x" where x is an integer from 0 to 15
        if (sscanf(avpair[i], "priv-lvl=%d", &lvl) == 1) {
            if (lvl < 0) {
                T_I("priv-lvl is out of range %d", lvl);
                *priv_lvl = 0;
            } else if (lvl > 15) {
                T_I("priv-lvl is out of range %d", lvl);
                *priv_lvl = 15;
            } else {
                T_I("priv-lvl is %d", lvl);
                *priv_lvl = lvl;
            }
            found = TRUE;
            break;
        }
    }
    tac_free_avpairs(avpair);
    if (!found) {
        T_I("avpair \"priv-lvl=x\" not returned from TACACS+ server");
    }

    rc = VTSS_RC_OK;

    S_I("User %s logged in via TACACS (priv level %d)", username, *priv_lvl);

do_exit:

    if (!single && tacacs_session) {
        tac_close(tacacs_session);
        tacacs_session = NULL;
    }

#ifdef VTSS_AUTH_CHECK_HEAP
    sleep(1); // Give the memory a little time to get freed
    m_after = mallinfo();
    m_loss = m_before.fordblks - m_after.fordblks;
    if (m_loss) {
        T_E("Lost %d (0x%x) bytes in Tacacs authen/author library", m_loss, m_loss);
    }
#endif

    T_N("exit");
    return rc;
}

/* ================================================================= *
 *  AUTH_tacplus_cmd_authorize()
 *  Authorization via TACACS+
 *  Return:
 *    VTSS_RC_OK if command is allowed.
 * ================================================================= */
static mesa_rc AUTH_tacplus_cmd_authorize(const vtss_auth_tacacs_conf_t *conf,
                                          vtss_appl_auth_agent_t        agent,
                                          const char                    *hostname,
                                          const char                    *username,
                                          const char                    *command,
                                          u8                            priv_lvl)
{
    mesa_rc rc = VTSS_APPL_AUTH_ERROR_SERVER_ERROR;
    int     i, tac_rc;
    int     single = 0; /* No support of single connection unless set to TRUE by first response from the TACACS+ server */
    char    serv_msg[256];
    char    data_msg[256];
    char    *avpair[VTSS_AUTH_TACPLUS_AVPAIR_MAX];

#ifdef VTSS_AUTH_CHECK_HEAP
    struct mallinfo m_before;
    struct mallinfo m_after;
    int m_loss;
    m_before = mallinfo();
#endif

    T_N("enter");

    if (tacacs_session == NULL) { /* Open a session */
        if ((tacacs_session = AUTH_tac_connect(conf)) == NULL) {
            T_I("Unable to connect to TACACS+ server");
            S_I("Unable to connect to TACACS+ server");
            goto do_exit;
        }
    }

    if (AUTH_tacplus_cmd2av(NULL,
                            command,
                            serv_msg,
                            sizeof(serv_msg),
                            avpair,
                            VTSS_AUTH_TACPLUS_AVPAIR_MAX) != VTSS_RC_OK) {
        T_W("AUTH_tacplus_cmd2av() failure");
        goto do_exit;
    }
    for (i = 0; avpair[i] != NULL; i++) {
        T_D("avpair[%d] \"%s\"", i, avpair[i]);
    }
    T_D("avpair[%d] NULL", i);

    if (tac_author_send_request(tacacs_session,
                                TAC_PLUS_AUTHEN_METH_TACACSPLUS,
                                TAC_PLUS_PRIV_LVL_MIN,
                                TAC_PLUS_AUTHEN_TYPE_ASCII,
                                TAC_PLUS_AUTHEN_SVC_LOGIN,
                                username,
                                vtss_appl_auth_agent_name(agent),
                                hostname,
                                avpair,
                                VTSS_AUTH_TACPLUS_AVPAIR_MAX) == 0) {
        T_W("tac_author_send_request() failure");
        goto do_exit;
    }

    if ((tac_rc = tac_author_get_response(tacacs_session,
                                          &single,
                                          serv_msg,
                                          sizeof(serv_msg),
                                          data_msg,
                                          sizeof(data_msg),
                                          avpair,
                                          VTSS_AUTH_TACPLUS_AVPAIR_MAX)) <= 0) {
        T_W("tac_author_get_response() failure");
        goto do_exit;
    }
    T_D("returned from get response = %s, \"%s\", \"%s\"", tac_print_author_status(tac_rc), serv_msg, data_msg);

    for (i = 0; avpair[i] != NULL; i++) {
        T_D("avpair[%d] \"%s\"", i, avpair[i]);
    }
    tac_free_avpairs(avpair);

    if ((tac_rc != TAC_PLUS_AUTHOR_STATUS_PASS_ADD) && (tac_rc != TAC_PLUS_AUTHOR_STATUS_PASS_REPL)) {
        rc = VTSS_APPL_AUTH_ERROR_SERVER_REJECT; /* Command NOT permitted */
    } else {
        rc = VTSS_RC_OK; /* Command permitted */
    }

do_exit:

    if (!single && tacacs_session) {
        tac_close(tacacs_session);
        tacacs_session = NULL;
    }

#ifdef VTSS_AUTH_CHECK_HEAP
    sleep(1); // Give the memory a little time to get freed
    m_after = mallinfo();
    m_loss = m_before.fordblks - m_after.fordblks;
    if (m_loss) {
        T_E("Lost %d (0x%x) bytes in Tacacs account library", m_loss, m_loss);
    }
#endif

    T_N("exit");
    return rc;
}

/* ================================================================= *
 *  AUTH_tacplus_acconting()
 *  Accounting via TACACS+
 * ================================================================= */
static void AUTH_tacplus_accouting(const vtss_auth_tacacs_conf_t *conf,
                                   vtss_appl_auth_agent_t        agent,
                                   const char                    *hostname,
                                   const char                    *username,
                                   const char                    *command, /* NULL or empty == exec accounting, otherwise command accounting */
                                   u8                            priv_lvl,
                                   u16                           agent_id,
                                   BOOL                          stop)    /* FALSE: Start record, TRUE: Stop record */
{
    int     i, tac_rc;
    int     single = 0; /* No support of single connection unless set to TRUE by first response from the TACACS+ server */
    char    serv_msg[256];
    char    data_msg[256];
    char    *avpair[VTSS_AUTH_TACPLUS_AVPAIR_MAX];

#ifdef VTSS_AUTH_CHECK_HEAP
    struct mallinfo m_before;
    struct mallinfo m_after;
    int m_loss;
    m_before = mallinfo();
#endif

    T_N("enter");

    if (tacacs_session == NULL) { /* Open a session */
        if ((tacacs_session = AUTH_tac_connect(conf)) == NULL) {
            T_I("Unable to connect to TACACS+ server");
            S_I("Unable to connect to TACACS+ server");
            goto do_exit;
        }
    }

    if (AUTH_tacplus_cmd2av(&agent_id,
                            command,
                            serv_msg,
                            sizeof(serv_msg),
                            avpair,
                            VTSS_AUTH_TACPLUS_AVPAIR_MAX) != VTSS_RC_OK) {
        T_W("AUTH_tacplus_cmd2av() failure");
        goto do_exit;
    }
    for (i = 0; avpair[i] != NULL; i++) {
        T_D("avpair[%d] \"%s\"", i, avpair[i]);
    }
    T_D("avpair[%d] NULL", i);

    if (tac_account_send_request(tacacs_session,
                                 stop ? TAC_PLUS_ACCT_FLAG_STOP : TAC_PLUS_ACCT_FLAG_START,
                                 TAC_PLUS_AUTHEN_METH_TACACSPLUS,
                                 priv_lvl,
                                 TAC_PLUS_AUTHEN_TYPE_ASCII,
                                 TAC_PLUS_AUTHEN_SVC_LOGIN,
                                 username,
                                 vtss_appl_auth_agent_name(agent),
                                 hostname,
                                 avpair,
                                 VTSS_AUTH_TACPLUS_AVPAIR_MAX) == 0) {
        T_W("tac_account_send_request() failure");
        goto do_exit;
    }

    if ((tac_rc = tac_account_get_reply(tacacs_session,
                                        &single,
                                        serv_msg,
                                        sizeof(serv_msg),
                                        data_msg,
                                        sizeof(data_msg))) <= 0) {
        T_W("tac_account_get_reply() failure");
        goto do_exit;
    }
    T_D("returned from get reply = %s, \"%s\", \"%s\"", tac_print_account_status(tac_rc), serv_msg, data_msg);

do_exit:

    if (!single && tacacs_session) {
        tac_close(tacacs_session);
        tacacs_session = NULL;
    }

#ifdef VTSS_AUTH_CHECK_HEAP
    sleep(1); // Give the memory a little time to get freed
    m_after = mallinfo();
    m_loss = m_before.fordblks - m_after.fordblks;
    if (m_loss) {
        T_E("Lost %d (0x%x) bytes in Tacacs account library", m_loss, m_loss);
    }
#endif

    T_N("exit");
}
#endif /* VTSS_SW_OPTION_TACPLUS */

/* ================================================================= *
 *  AUTH_login()
 *  Handles authentication and exec authorization of a user login.
 *  Return:
 *    VTSS_RC_OK                         : Login is successful.
 *    VTSS_APPL_AUTH_ERROR_SERVER_REJECT : Login is rejected.
 * ================================================================= */
static mesa_rc AUTH_login(const vtss_auth_conf_t          *conf,
                          const vtss_auth_msg_login_req_t *req,
                          u8                              *priv_lvl)
{
    int     m;
    mesa_rc rc;
    u8      lvl = 0;

    if (req->agent >= VTSS_APPL_AUTH_AGENT_LAST) {
        T_E("Invalid client (%d)", req->agent);
        return VTSS_APPL_AUTH_ERROR_SERVER_REJECT;
    }

    const vtss_appl_auth_authen_agent_conf_t *client = &conf->authen_agent[req->agent];
    for (m = 0; m <= VTSS_APPL_AUTH_METHOD_PRI_IDX_MAX; m++) {
        switch (client->method[m]) {
        case VTSS_APPL_AUTH_AUTHEN_METHOD_LOCAL:
            rc = AUTH_local_authenticate(req->username, req->password, &lvl);
            break;

        case VTSS_APPL_AUTH_AUTHEN_METHOD_RADIUS:
#ifdef VTSS_SW_OPTION_RADIUS
            rc = AUTH_radius_authenticate(req->agent, req->hostname, req->username, req->password, &lvl);
#else
            rc = VTSS_APPL_AUTH_ERROR_SERVER_ERROR;
#endif /* VTSS_SW_OPTION_RADIUS */
            break;

        case VTSS_APPL_AUTH_AUTHEN_METHOD_TACACS:
#ifdef VTSS_SW_OPTION_TACPLUS
            rc = AUTH_tacplus_authenticate(&conf->tacacs, req->agent, req->hostname, req->username, req->password, &lvl);
#else
            rc = VTSS_APPL_AUTH_ERROR_SERVER_ERROR;
#endif /* VTSS_SW_OPTION_TACPLUS */
            break;

        default:
            rc = VTSS_APPL_AUTH_ERROR_SERVER_REJECT; /* Login not allowed */
            break;
        }

        if ((rc == VTSS_RC_OK) || (rc == VTSS_APPL_AUTH_ERROR_SERVER_REJECT)) {
            break; /* We are done - exit the for loop */
        }
    }
    if (m == VTSS_APPL_AUTH_METHOD_PRI_IDX_MAX + 1) {
        rc = VTSS_APPL_AUTH_ERROR_SERVER_REJECT; /* Login not allowed */
    }

    if (rc == VTSS_RC_OK) {
        *priv_lvl = lvl;
    }

    return rc;
}

/* ================================================================= *
 *  AUTH_login_accounting()
 *  Handles exec accouting when a user logs in.
 * ================================================================= */
static void AUTH_login_accounting(const vtss_auth_conf_t         *conf,
                                  const vtss_auth_msg_login_req_t *req,
                                  u8                              priv_lvl,
                                  u16                             agent_id)
{
    if ((req->agent == VTSS_APPL_AUTH_AGENT_CONSOLE) ||
        (req->agent == VTSS_APPL_AUTH_AGENT_TELNET) ||
        (req->agent == VTSS_APPL_AUTH_AGENT_SSH)) {
        const vtss_appl_auth_acct_agent_conf_t *client = &conf->acct_agent[req->agent];

        if (client->exec_enable) {
            if (client->method == VTSS_APPL_AUTH_ACCT_METHOD_TACACS) {
#ifdef VTSS_SW_OPTION_TACPLUS
                AUTH_tacplus_accouting(&conf->tacacs, req->agent, req->hostname, req->username, NULL, priv_lvl, agent_id, FALSE);
#endif /* VTSS_SW_OPTION_TACPLUS */
            }
        }
    }
}

/* ================================================================= *
 *  AUTH_cmd_authorize()
 *  Handles authorization of cli commands.
 *  Return:
 *    VTSS_RC_OK                         : Command is permitted.
 *    VTSS_APPL_AUTH_ERROR_SERVER_REJECT : Command is not permitted.
 *    VTSS_APPL_AUTH_ERROR_SERVER_ERROR  : General error. Fallback to default local authorization.
 * ================================================================= */
static mesa_rc AUTH_cmd_authorize(const vtss_auth_conf_t        *conf,
                                  const vtss_auth_msg_cmd_req_t *req)
{
    mesa_rc rc = VTSS_APPL_AUTH_ERROR_SERVER_ERROR;

    if ((req->agent != VTSS_APPL_AUTH_AGENT_CONSOLE) &&
        (req->agent != VTSS_APPL_AUTH_AGENT_TELNET) &&
        (req->agent != VTSS_APPL_AUTH_AGENT_SSH)) {
        T_D("Invalid client (%d)", req->agent);
        return rc;
    }

    const vtss_appl_auth_author_agent_conf_t *client = &conf->author_agent[req->agent];

    if (client->cmd_enable &&
        (req->cmd_priv_lvl >= client->cmd_priv_lvl) &&
        (!req->cfg_cmd || client->cfg_cmd_enable)) {
        if (client->method == VTSS_APPL_AUTH_AUTHOR_METHOD_TACACS) {
#ifdef VTSS_SW_OPTION_TACPLUS
            rc = AUTH_tacplus_cmd_authorize(&conf->tacacs, req->agent, req->hostname, req->username, req->command, req->priv_lvl);
#endif /* VTSS_SW_OPTION_TACPLUS */
        }
    }

    return rc;
}

/* ================================================================= *
 *  AUTH_cmd_accounting()
 *  Handles accouting of cli commands.
 * ================================================================= */
static void AUTH_cmd_accounting(const vtss_auth_conf_t        *conf,
                                const vtss_auth_msg_cmd_req_t *req)
{
    if ((req->agent == VTSS_APPL_AUTH_AGENT_CONSOLE) ||
        (req->agent == VTSS_APPL_AUTH_AGENT_TELNET) ||
        (req->agent == VTSS_APPL_AUTH_AGENT_SSH)) {
        const vtss_appl_auth_acct_agent_conf_t *client = &conf->acct_agent[req->agent];

        if (req->execute && client->cmd_enable && (req->cmd_priv_lvl >= client->cmd_priv_lvl)) {
            if (client->method == VTSS_APPL_AUTH_ACCT_METHOD_TACACS) {
#ifdef VTSS_SW_OPTION_TACPLUS
                AUTH_tacplus_accouting(&conf->tacacs, req->agent, req->hostname, req->username, req->command, req->priv_lvl, req->agent_id, TRUE);
#endif /* VTSS_SW_OPTION_TACPLUS */
            }
        }
    }
}

/* ================================================================= *
 *  AUTH_logout_accounting()
 *  Handles exec accouting when a user logs out.
 * ================================================================= */
static void AUTH_logout_accounting(const vtss_auth_conf_t           *conf,
                                   const vtss_auth_msg_logout_req_t *req)
{
    if ((req->agent == VTSS_APPL_AUTH_AGENT_CONSOLE) ||
        (req->agent == VTSS_APPL_AUTH_AGENT_TELNET) ||
        (req->agent == VTSS_APPL_AUTH_AGENT_SSH)) {
        const vtss_appl_auth_acct_agent_conf_t *client = &conf->acct_agent[req->agent];

        if (client->exec_enable) {
            if (client->method == VTSS_APPL_AUTH_ACCT_METHOD_TACACS) {
#ifdef VTSS_SW_OPTION_TACPLUS
                AUTH_tacplus_accouting(&conf->tacacs, req->agent, req->hostname, req->username, NULL, req->priv_lvl, req->agent_id, TRUE);
#endif /* VTSS_SW_OPTION_TACPLUS */
            }
        }
    }
}

static void AUTH_conf_default(void);
/* ================================================================= *
 *  AUTH_thread()
 *  Runs the server part of the auth module
 *  The server keeps a local copy of the configuration, in order to
 *  avoid locking of the configuration several times during a lengthy
 *  operation.
 * ================================================================= */
static void AUTH_thread(vtss_addrword_t data)
{
    vtss_auth_conf_t local_config;
    u16              agent_id = 0;
    msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_AUTH);

#ifdef VTSS_SW_OPTION_RADIUS
    AUTH_radius_init();
#endif /* VTSS_SW_OPTION_RADIUS */

    AUTH_CRIT_CONFIG_ENTER();
    local_config = config;
    AUTH_CRIT_CONFIG_EXIT();

    for (;;) {
        T_N("begin server");
        vtss_auth_msg_t *msg = (vtss_auth_msg_t *)auth_queue.vtss_safe_queue_get();

        AUTH_CRIT_CONFIG_ENTER();
        if (config_changed) {
            T_D("config changed");
            config_changed = FALSE;
            local_config = config;
        }
        AUTH_CRIT_CONFIG_EXIT();

        if (msg_switch_is_primary()) {
            mesa_rc rc;

            switch (msg->hdr.msg_id) {
            case VTSS_AUTH_MSG_ID_LOGIN_REQ: {
                u8          priv_lvl = 0;
                vtss_isid_t isid     = msg->u.login_req.isid;
                u32         seq_num  = msg->u.login_req.seq_num;

                T_N("msg VTSS_AUTH_MSG_ID_LOGIN_REQ received");

                agent_id++;
                if (agent_id == 0) {
                    agent_id++; /* 0 is NOT allowed! */
                }

                if ((rc = AUTH_login(&local_config, &msg->u.login_req, &priv_lvl)) == VTSS_RC_OK) {
                    /* Suppress 'Highest operation, function 'AUTH_login_accounting', lacks side-effects' when TACACS+ is not included in build */
                    /*lint --e{522} */
                    AUTH_login_accounting(&local_config, &msg->u.login_req, priv_lvl, agent_id);
                }

                /* Reuse the msg - AUTH_msg_rx always allocate a maximum size msg */
                msg->hdr.msg_id           = VTSS_AUTH_MSG_ID_LOGIN_REP;
                msg->u.login_rep.seq_num  = seq_num;
                msg->u.login_rep.priv_lvl = priv_lvl;
                msg->u.login_rep.agent_id = agent_id;
                msg->u.login_rep.rc       = rc;
                msg_tx(VTSS_MODULE_ID_AUTH, isid, msg, MSG_TX_DATA_HDR_LEN(vtss_auth_msg_t, u) + sizeof(msg->u.login_rep));
                break;
            }

            case VTSS_AUTH_MSG_ID_CMD_REQ: {
                vtss_isid_t isid    = msg->u.cmd_req.isid;
                u32         seq_num = msg->u.cmd_req.seq_num;

                T_N("msg VTSS_AUTH_MSG_ID_CMD_REQ received");

                if ((rc = AUTH_cmd_authorize(&local_config, &msg->u.cmd_req)) != VTSS_APPL_AUTH_ERROR_SERVER_REJECT) {
                    /* Suppress 'Highest operation, function 'AUTH_cmd_accounting', lacks side-effects' when TACACS+ is not included in build */
                    /*lint --e{522} */
                    AUTH_cmd_accounting(&local_config, &msg->u.cmd_req);
                }

                /* Reuse the msg - AUTH_msg_rx always allocate a maximum size msg */
                msg->hdr.msg_id         = VTSS_AUTH_MSG_ID_CMD_REP;
                msg->u.cmd_rep.seq_num  = seq_num;
                msg->u.cmd_rep.rc       = rc;
                msg_tx(VTSS_MODULE_ID_AUTH, isid, msg, MSG_TX_DATA_HDR_LEN(vtss_auth_msg_t, u) + sizeof(msg->u.cmd_rep));
                break;
            }

            case VTSS_AUTH_MSG_ID_LOGOUT_REQ: {
                T_N("msg VTSS_AUTH_MSG_ID_LOGOUT_REQ received");

                /* Suppress 'Highest operation, function 'AUTH_logout_accounting', lacks side-effects' when TACACS+ is not included in build */
                /*lint --e{522} */
                AUTH_logout_accounting(&local_config, &msg->u.logout_req);
                /* No reply here */
                break;
            }

            default:
                T_W("Unknown message ID: %d", msg->hdr.msg_id);
                VTSS_FREE(msg);
                break;
            }
        } else {
            T_W("server request received on a secondary switch");
            VTSS_FREE(msg);
        }
#ifdef VTSS_SW_OPTION_TACPLUS
        if (tacacs_session) { /* Close the tacacs session that has been left open (which is completely legal) */
            T_N("Closing tacacs+ session");
            tac_close(tacacs_session);
            tacacs_session = NULL;
        }
#endif /* VTSS_SW_OPTION_TACPLUS */
        T_N("end server");
    }
}

// Agent name 2 id
vtss_appl_auth_agent_t agent_name2id(const char *agent)
{
    if (strcasecmp(agent, "console") == 0) {
        return VTSS_APPL_AUTH_AGENT_CONSOLE;
    }
    if (strcasecmp(agent, "telnet") == 0) {
        return VTSS_APPL_AUTH_AGENT_TELNET;
    }
    if (strcasecmp(agent, "ssh") == 0) {
        return VTSS_APPL_AUTH_AGENT_SSH;
    }
    if (strcasecmp(agent, "http") == 0) {
        return VTSS_APPL_AUTH_AGENT_HTTP;
    }
    return VTSS_APPL_AUTH_AGENT_LAST;
}

static int split(char sep, char *string, char **tokens, int maxtokens)
{
    int i, n_token = 0, len;

    len = strlen(string);

    // Skip leading separators
    for (i = 0; i < len && string[i] == sep; i++);
    if (i == len) {
        return 0;
    }

    tokens[n_token++] = &string[i];

    // Find next seperator
    for (; i < len && n_token < maxtokens; ++i) {
        if ( string[i] == sep) {
            string[i] = 0;
            tokens[n_token++] = &string[++i];
        }
        // It is important to check for n_token==maxtokens. The password is the last token
        // and if the password contains a space it shall not be cut up in two tokens.
        if (n_token == maxtokens) {
            break;
        }
    }

    return n_token;
}

/* ================================================================= *
 *  AUTH_service_thread()
 *  Receives local requests for services in external processes
 * ================================================================= */
static void AUTH_service_thread(vtss_addrword_t data)
{
    const char *path = VTSS_FS_RUN_DIR "auth.socket";
    struct sockaddr_un saddr;
    struct sockaddr_in  client_addr;
    socklen_t addr_size;
    int srvr_sock;
    char authreq[200];
#define ARGCT 4    // agent host user passwd
    char *auth_args[ARGCT];

    unlink(path);

    /* Create the socket: */
    if ((srvr_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        T_E("accept: %s", strerror(errno));
        return;
    }

    /* Bind a name to the socket: */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sun_family = AF_UNIX;
    strncpy(saddr.sun_path, path, sizeof(saddr.sun_path));

    if (bind(srvr_sock, (struct sockaddr *) &saddr, SUN_LEN(&saddr)) < 0) {
        T_E("bind: %s", strerror(errno));
        close(srvr_sock);
        return;
    }

    if (chmod(path, 0776)) {
        T_E("chmod: %s", strerror(errno));
        close(srvr_sock);
        return;
    }

    if (listen(srvr_sock, 5)) {
        T_E("listen: %s", strerror(errno));
        close(srvr_sock);
        return;
    }

    for (;;) {
        int client_sock = accept(srvr_sock, (struct sockaddr *) &client_addr, &addr_size);
        if (client_sock < 0) {
            T_E("accept: %s", strerror(errno));
            sleep(1);
        } else {
            int len = read(client_sock, authreq, sizeof(authreq) - 1);
            if (len >= 0) {
                // Chomp end of line
                if (len > 1 && authreq[len - 1] == '\n') {
                    len--;
                }
                // nul terminate request
                authreq[len] = '\0';
                if (split(' ', authreq, auth_args, ARRSZ(auth_args)) == ARRSZ(auth_args)) {
                    u8 priv_lvl;
                    u16 agent_id;
                    mesa_rc rc;
                    if ((rc = vtss_auth_login(agent_name2id(auth_args[0]), // Agent
                                              auth_args[1],    // host
                                              auth_args[2],    // user
                                              auth_args[3],    // passwd
                                              &priv_lvl,
                                              &agent_id)) == VTSS_RC_OK) {
                        T_D("Accepted request: host %s user %s", auth_args[1], auth_args[2]);
                        sprintf(authreq, "ACCEPTED LEVEL=%d ID=%d\n", priv_lvl, agent_id);
                        T_D("Accepted level: %s", authreq);
                        write(client_sock, authreq, strlen(authreq));
                    } else {
                        T_D("Denied request: host %s user %s: %s", auth_args[1], auth_args[2], error_txt(rc));
                        sprintf(authreq, "DENIED - %s\n", error_txt(rc));
                        write(client_sock, authreq, strlen(authreq));
                    }
                } else {
                    const char *resp = "BAD REQUEST\n";
                    T_D("Garbled request: %s", authreq);
                    write(client_sock, resp, strlen(resp));
                }
            }
            close(client_sock);
        }
    }
}

/* ================================================================= *
 *  AUTH_conf_default()
 *  Create default configuration.
 * ================================================================= */
static void AUTH_conf_default(void)
{
    int  i;

    AUTH_CRIT_CONFIG_ENTER();

    memset(&config, 0, sizeof(config));
    for (i = 0; i < VTSS_APPL_AUTH_AGENT_LAST; i++ ) {
        (void)vtss_appl_auth_authen_agent_conf_default(&config.authen_agent[i]);
    }

    /* For TACPLUS, RADIUS:
     *
     * The default unencrypted key is the empty string. We signal this by
     * treating it as unencrypted, length 0. Thus, we don't store an encrypted
     * form of the empty string. This is for performance reasons, given that
     * each cryptography op takes around 0.8 secs.
     *
     * The only slight downside to this approach is that we now expose the fact
     * that we have no key in non-encrypted fashion.
     */

#ifdef VTSS_SW_OPTION_TACPLUS
    config.tacacs.global.timeout     = VTSS_APPL_AUTH_TIMEOUT_DEFAULT;
    config.tacacs.global.deadtime    = VTSS_APPL_AUTH_DEADTIME_DEFAULT;
    strcpy(config.tacacs.global.key, "");
    config.tacacs.global.encrypted   = FALSE;
#endif /* VTSS_SW_OPTION_TACPLUS */

#ifdef VTSS_SW_OPTION_RADIUS
    config.radius.global.timeout     = VTSS_APPL_AUTH_TIMEOUT_DEFAULT;
    config.radius.global.retransmit  = VTSS_APPL_AUTH_RETRANSMIT_DEFAULT;
    config.radius.global.deadtime    = VTSS_APPL_AUTH_DEADTIME_DEFAULT;
    strcpy(config.radius.global.key, "");
    config.radius.global.encrypted   = FALSE;
    (void)AUTH_radius_config_update(&config.radius);
#endif /* VTSS_SW_OPTION_RADIUS */
    config_changed = TRUE; /* Notify auth thread */

    AUTH_CRIT_CONFIG_EXIT();
}

/* ================================================================= *
 *
 * Global functions starts here
 *
 * ================================================================= */

/* ================================================================= *
 * vtss_auth_error_txt()
 * ================================================================= */
const char *vtss_auth_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_AUTH_ERROR_SERVER_REJECT:
        return "Server request rejected";
    case VTSS_APPL_AUTH_ERROR_SERVER_ERROR:
        return "Server request error";
    case VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH:
        return "Operation only valid on primary switch";
    case VTSS_APPL_AUTH_ERROR_CFG_TIMEOUT:
        return "Invalid timeout configuration parameter";
    case VTSS_APPL_AUTH_ERROR_CFG_RETRANSMIT:
        return "Invalid retransmit configuration parameter";
    case VTSS_APPL_AUTH_ERROR_CFG_DEADTIME:
        return "Invalid deadtime configuration parameter";
    case VTSS_APPL_AUTH_ERROR_CFG_SECRET_KEY:
        return "Invalid secret key configuration parameter";
    case VTSS_APPL_AUTH_ERROR_CFG_HOST:
        return "Invalid host name or IP address";
    case VTSS_APPL_AUTH_ERROR_CFG_PORT:
        return "Invalid port configuration parameter";
    case VTSS_APPL_AUTH_ERROR_CFG_HOST_PORT:
        return "Invalid host and port combination";
    case VTSS_APPL_AUTH_ERROR_CFG_HOST_TABLE_FULL:
        return "Host table is full";
    case VTSS_APPL_AUTH_ERROR_CFG_HOST_NOT_FOUND:
        return "Host not found";
    case VTSS_APPL_AUTH_ERROR_CFG_AGENT:
        return "Invalid agent";
    case VTSS_APPL_AUTH_ERROR_CFG_AGENT_METHOD:
        return "Invalid agent method";
    case VTSS_APPL_AUTH_ERROR_CFG_PRIV_LVL:
        return "Invalid privilege level";
    case VTSS_APPL_AUTH_ERROR_CACHE_EXPIRED:
        return "HTTPD cache entry is expired";
    case VTSS_APPL_AUTH_ERROR_CACHE_INVALID:
        return "HTTPD cache has no valid entry";
    case VTSS_APPL_AUTH_ERROR_CFG_RADIUS_NAS_IPV4:
        return "Invalid RADIUS NAS-IP-Address configuration parameter";
    case VTSS_APPL_AUTH_ERROR_CFG_RADIUS_NAS_IPV6:
        return "Invalid RADIUS NAS-IPv6-Address configuration parameter";
    default:
        return "Unknown auth error code";
    }
}

/* ================================================================= *
 *  vtss_auth_login()
 *  Make a client login request, send it to the server and wait for
 *  a reply.
 * ================================================================= */
mesa_rc vtss_auth_login(vtss_appl_auth_agent_t agent,
                        const char             *hostname,
                        const char             *username,
                        const char             *password,
                        u8                     *priv_lvl,
                        u16                    *agent_id)
{
    vtss_auth_client_buf_t *buf;
    vtss_tick_count_t       timeout;
    mesa_rc                rc = VTSS_APPL_AUTH_ERROR_SERVER_REJECT;

    T_I("ag: %s, hn: %s, us: '%s'",
        vtss_appl_auth_agent_name(agent),
        hostname ? hostname : "<none>",
        username);

    if (agent >= VTSS_APPL_AUTH_AGENT_LAST) {
        T_D("Invalid client (%d)", agent);
        goto do_exit;
    }

#if !defined(VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR)
    if (!username[0]) {
        T_D("No username");
        goto do_exit;
    }
#endif /* !VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR */

    buf = AUTH_client_buf_alloc(VTSS_AUTH_MSG_ID_LOGIN_REQ);

    /* Initialize message */
    buf->msg.u.login_req.seq_num = buf->seq_num;
    buf->msg.u.login_req.agent   = agent;
    if (hostname) {
        misc_strncpyz(buf->msg.u.login_req.hostname, hostname, INET6_ADDRSTRLEN);
    }
    misc_strncpyz(buf->msg.u.login_req.username, username, VTSS_AUTH_MAX_USERNAME_LENGTH);
    misc_strncpyz(buf->msg.u.login_req.password, password, VTSS_AUTH_MAX_PASSWORD_LENGTH);

    timeout = vtss_current_time() +  VTSS_OS_MSEC2TICK(VTSS_AUTH_CLIENT_TIMEOUT * 1000);
    buf->state = VTSS_AUTH_CLIENT_BUF_STATE_WAITING; /* Init condition variable */

    /* Transmit request and wait for reply from server */
    (void)vtss_mutex_lock(&buf->mutex);
    while (buf->state == VTSS_AUTH_CLIENT_BUF_STATE_WAITING) {

        /* Transmit message to the server on the primary switch */
        msg_tx_adv(NULL, AUTH_msg_tx_done, MSG_TX_OPT_DONT_FREE, VTSS_MODULE_ID_AUTH, 0, &buf->msg,
                   MSG_TX_DATA_HDR_LEN(vtss_auth_msg_t, u) + sizeof(buf->msg.u.login_req));

        T_N("Waiting with timeout");
        if (vtss_cond_timed_wait(&buf->cond, timeout) == FALSE) {
            T_N("Timeout");
            buf->state = VTSS_AUTH_CLIENT_BUF_STATE_DONE;
            break; /* Terminate while loop */
        } else if (buf->state == VTSS_AUTH_CLIENT_BUF_STATE_RETRY) {
            T_N("msg_tx error");
            /* Try again after a while */
            buf->state = VTSS_AUTH_CLIENT_BUF_STATE_WAITING;
            VTSS_OS_MSLEEP(1000);
            /* Stay in while loop */
        } else if ((buf->state == VTSS_AUTH_CLIENT_BUF_STATE_DONE) &&
                   (buf->msg.hdr.msg_id == VTSS_AUTH_MSG_ID_LOGIN_REP)) {
            T_N("Got valid reply: %s", error_txt(buf->msg.u.login_rep.rc));
            *priv_lvl = buf->msg.u.login_rep.priv_lvl;
            *agent_id = buf->msg.u.login_rep.agent_id;
            rc        = buf->msg.u.login_rep.rc;
            break; /* Terminate while loop */
        } else {
            T_E("Invalid state (%d) or msg_id (%d)", buf->state, buf->msg.hdr.msg_id);
            break; /* Terminate while loop */
        }
    }
    (void)vtss_mutex_unlock(&buf->mutex);

    AUTH_client_buf_free(buf);

do_exit:
    T_I("return: %s", error_txt(rc));
    return rc;
}

mesa_rc vtss_auth_integrity_log(void)
{
    char cmd_out_buf[128];
    char csum_running_config[64];
    char csum_startup_config[64];
    FILE *pFile;

    pFile = popen("md5sum /usr/bin/switch_app | cut -d' ' -f1", "r");
    if (!pFile) {
        return VTSS_APPL_AUTH_ERROR_CHECKSUM_TO_SYSLOG_ERROR;
    }
    while (fgets(cmd_out_buf, sizeof(cmd_out_buf), pFile) != 0) {
        T_D("Checksum for /usr/bin/switch_app: %s", cmd_out_buf);
        S_I("csum for application is %s", cmd_out_buf);
    }
    pclose(pFile);

    T_D("Generate temp file with running-config");
    pFile = fopen("/tmp/runconf.tmp", "w");
    if (pFile) {
        vtss_icfg_query_result_t res;
        vtss_icfg_query_result_buf_t *buf;
        char *p;
        u32  len;
        char tmp[128];
        u32  n;

        if (vtss_icfg_query_all(false, &res) != VTSS_RC_OK) {
            return VTSS_APPL_AUTH_ERROR_CHECKSUM_TO_SYSLOG_ERROR;
        }
        buf = res.head;
        n = 0;
        while (buf != NULL  &&  buf->used > 0) {
            len = buf->used;
            p   = buf->text;
            while (len > 0) {
                tmp[n++] = *p;
                if ((*p == '\n')  ||  (n == (127))) {
                    tmp[n] = 0;
                    fprintf(pFile, "%s", tmp);
                    n = 0;
                }
                p++;
                len--;
            }
            buf = buf->next;
        }
        if (n > 0) {
            tmp[n] = 0;
            fprintf(pFile, "%s", tmp);
        }
        vtss_icfg_free_query_result(&res);
        fclose(pFile);
        T_D("File /tmp/runconf.tmp now created");

        pFile = popen("md5sum /tmp/runconf.tmp | cut -d' ' -f1", "r");
        if (!pFile) {
            return VTSS_APPL_AUTH_ERROR_CHECKSUM_TO_SYSLOG_ERROR;
        }
        while (fgets(cmd_out_buf, sizeof(cmd_out_buf), pFile) != 0) {
            T_D("Checksum for config: %s", cmd_out_buf);
            S_I("Checksum for running-config is %s", cmd_out_buf);
            strcpy(csum_running_config, cmd_out_buf);
        }
        pclose(pFile);
        remove("/tmp/runconf.tmp");
        T_D("File /tmp/runconf.tmp now deleted");

        // attempt to open startup-config
        pFile = fopen("/switch/icfg/startup-config", "r");
        if (pFile) {
            // startup config exists - go ahead and caluclate sum on this
            fclose(pFile);
            pFile = popen("md5sum /switch/icfg/startup-config | cut -d' ' -f1", "r");
            if (!pFile) {
                return VTSS_APPL_AUTH_ERROR_CHECKSUM_TO_SYSLOG_ERROR;
            }
            while (fgets(cmd_out_buf, sizeof(cmd_out_buf), pFile) != 0) {
                strcpy(csum_startup_config, cmd_out_buf);
            }
            if (strcmp(csum_startup_config, csum_running_config)) {
                T_D("Checksum for running-config differs from startup-config");
                S_I("Checksum for running-config differs from startup-config");
            }
            pclose(pFile);
        } else {
            // no startup config exist - submit log about this
            T_D("Startup-config does not exist");
            S_I("Startup-config does not exist");
        }
    } else {
        return VTSS_APPL_AUTH_ERROR_CHECKSUM_TO_SYSLOG_ERROR;
    }

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  vtss_auth_cmd()
 *  Make a client command request, send it to the server and wait
 *  for a reply.
 *  Supported for console, Telnet and SSH.
 *  This is for authorization and accounting.
 * ================================================================= */
mesa_rc vtss_auth_cmd(vtss_appl_auth_agent_t agent,
                      const char             *hostname,
                      const char             *username,
                      const char             *command,
                      u8                     priv_lvl,
                      u16                    agent_id,
                      BOOL                   execute,
                      u8                     cmd_priv_lvl,
                      BOOL                   cfg_cmd)
{
    vtss_auth_client_buf_t *buf;
    vtss_tick_count_t       timeout;
    mesa_rc                rc = VTSS_APPL_AUTH_ERROR_SERVER_ERROR;

    T_I("ag: %s, hn: %s, us: '%s', cm: '%s', pl: %u, ai: %u, ex: %d, cpl: %u, cfg: %d",
        vtss_appl_auth_agent_name(agent),
        hostname ? hostname : "<none>",
        username,
        command,
        priv_lvl,
        agent_id,
        execute,
        cmd_priv_lvl,
        cfg_cmd);

    if (agent_id == 0) {
        goto do_exit; /* Never do authorization and accounting for agent_id 0. */
    }

    if ((agent != VTSS_APPL_AUTH_AGENT_CONSOLE) &&
        (agent != VTSS_APPL_AUTH_AGENT_TELNET) &&
        (agent != VTSS_APPL_AUTH_AGENT_SSH)) {
        T_D("Invalid client (%d)", agent);
        goto do_exit;
    }

#if !defined(VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR)
    if (!username[0]) {
        T_D("No username");
        goto do_exit;
    }
#endif /* !VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR */

    if (msg_switch_is_primary()) { /* We are called on primary switch. Check if we really need to do anything */
        AUTH_CRIT_CONFIG_ENTER();
        if (((config.author_agent[agent].method == VTSS_APPL_AUTH_AUTHOR_METHOD_NONE) || !config.author_agent[agent].cmd_enable) &&
            ((config.acct_agent[agent].method == VTSS_APPL_AUTH_ACCT_METHOD_NONE) || !config.acct_agent[agent].cmd_enable)) {
            AUTH_CRIT_CONFIG_EXIT();
            T_D("Authorization and accounting not enabled for this agent.");
            goto do_exit;
        }
        AUTH_CRIT_CONFIG_EXIT();
    }

    buf = AUTH_client_buf_alloc(VTSS_AUTH_MSG_ID_CMD_REQ);

    /* Initialize message */
    buf->msg.u.cmd_req.seq_num      = buf->seq_num;
    buf->msg.u.cmd_req.agent        = agent;
    buf->msg.u.cmd_req.priv_lvl     = priv_lvl;
    buf->msg.u.cmd_req.agent_id     = agent_id;
    buf->msg.u.cmd_req.execute      = execute;
    buf->msg.u.cmd_req.cmd_priv_lvl = cmd_priv_lvl;
    buf->msg.u.cmd_req.cfg_cmd      = cfg_cmd;
    if (hostname) {
        misc_strncpyz(buf->msg.u.cmd_req.hostname, hostname, sizeof(buf->msg.u.cmd_req.hostname));
    }
    misc_strncpyz(buf->msg.u.cmd_req.username, username, sizeof(buf->msg.u.cmd_req.username));
    misc_strncpyz(buf->msg.u.cmd_req.command, command, sizeof(buf->msg.u.cmd_req.command));

    timeout = vtss_current_time() +  VTSS_OS_MSEC2TICK(VTSS_AUTH_CLIENT_TIMEOUT * 1000);
    buf->state = VTSS_AUTH_CLIENT_BUF_STATE_WAITING; /* Init condition variable */

    /* Transmit request and wait for reply from server */
    (void)vtss_mutex_lock(&buf->mutex);
    while (buf->state == VTSS_AUTH_CLIENT_BUF_STATE_WAITING) {

        /* Transmit message to the server on the primary switch */
        msg_tx_adv(NULL, AUTH_msg_tx_done, MSG_TX_OPT_DONT_FREE, VTSS_MODULE_ID_AUTH, 0, &buf->msg,
                   MSG_TX_DATA_HDR_LEN(vtss_auth_msg_t, u) + sizeof(buf->msg.u.cmd_req));

        T_N("Waiting with timeout");
        if (vtss_cond_timed_wait(&buf->cond, timeout) == FALSE) {
            T_N("Timeout");
            buf->state = VTSS_AUTH_CLIENT_BUF_STATE_DONE;
            break; /* Terminate while loop */
        } else if (buf->state == VTSS_AUTH_CLIENT_BUF_STATE_RETRY) {
            T_N("msg_tx error");
            // Try again after a while
            buf->state = VTSS_AUTH_CLIENT_BUF_STATE_WAITING;
            VTSS_OS_MSLEEP(1000);
            /* Stay in while loop */
        } else if ((buf->state == VTSS_AUTH_CLIENT_BUF_STATE_DONE) &&
                   (buf->msg.hdr.msg_id == VTSS_AUTH_MSG_ID_CMD_REP)) {
            T_N("Got valid reply: %s", error_txt(buf->msg.u.cmd_rep.rc));
            rc = buf->msg.u.cmd_rep.rc;
            break; /* Terminate while loop */
        } else {
            T_E("Invalid state (%d) or msg_id (%d)", buf->state, buf->msg.hdr.msg_id);
            break; /* Terminate while loop */
        }
    }
    (void)vtss_mutex_unlock(&buf->mutex);

    AUTH_client_buf_free(buf);

do_exit:
    T_I("return: %s", error_txt(rc));
    return rc;
}

/* ================================================================= *
 *  vtss_auth_logout()
 *  Make a client logout request, send it to the server.
 *  No reply needed.
 *  Supported for console, Telnet and SSH.
 *  This is for accounting.
 * ================================================================= */
mesa_rc vtss_auth_logout(vtss_appl_auth_agent_t agent,
                         const char             *hostname,
                         const char             *username,
                         u8                     priv_lvl,
                         u16                    agent_id)
{
    mesa_rc         rc = VTSS_APPL_AUTH_ERROR_SERVER_ERROR;
    vtss_auth_msg_t *m = NULL;

    T_I("ag: %s, hn: %s, us: '%s', pl: %u, ai: %u",
        vtss_appl_auth_agent_name(agent),
        hostname ? hostname : "<none>",
        username,
        priv_lvl,
        agent_id);

    if (agent_id == 0) {
        goto do_exit; /* Never do accounting for agent_id 0. */
    }

    if ((agent != VTSS_APPL_AUTH_AGENT_CONSOLE) &&
        (agent != VTSS_APPL_AUTH_AGENT_TELNET) &&
        (agent != VTSS_APPL_AUTH_AGENT_SSH)) {
        T_D("Invalid client (%d)", agent);
        goto do_exit;
    }

#if !defined(VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR)
    if (!username[0]) {
        T_D("No username");
        goto do_exit;
    }
#endif /* !VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR */

    if (msg_switch_is_primary()) { /* We are called on the primary switch. Check if we really need to do anything */
        AUTH_CRIT_CONFIG_ENTER();
        if ((config.acct_agent[agent].method == VTSS_APPL_AUTH_ACCT_METHOD_NONE) || !config.acct_agent[agent].exec_enable) {
            AUTH_CRIT_CONFIG_EXIT();
            T_D("Accounting not enabled for this agent - nothing to do.");
            goto do_exit;
        }
        AUTH_CRIT_CONFIG_EXIT();
    }

    m = (vtss_auth_msg_t *)VTSS_MALLOC(sizeof(vtss_auth_msg_t));
    if (!m) {
        T_W("out of memory");
        goto do_exit;
    }

    /* Initialize message */
    m->hdr.version           = VTSS_AUTH_MSG_VERSION;
    m->hdr.msg_id            = VTSS_AUTH_MSG_ID_LOGOUT_REQ;
    m->u.logout_req.agent    = agent;
    m->u.logout_req.priv_lvl = priv_lvl;
    m->u.logout_req.agent_id = agent_id;
    if (hostname) {
        misc_strncpyz(m->u.logout_req.hostname, hostname, INET6_ADDRSTRLEN);
    }
    misc_strncpyz(m->u.logout_req.username, username, VTSS_AUTH_MAX_USERNAME_LENGTH);

    msg_tx(VTSS_MODULE_ID_AUTH, 0, m, MSG_TX_DATA_HDR_LEN(vtss_auth_msg_t, u) + sizeof(m->u.logout_req));
    rc = VTSS_RC_OK;

do_exit:
    T_I("return: %s", error_txt(rc));
    return rc;
}

/* ================================================================= *
 *  vtss_appl_auth_authen_agent_conf_get()
 *  Returns the current authentication agent configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_authen_agent_conf_get(vtss_appl_auth_agent_t agent, vtss_appl_auth_authen_agent_conf_t *const conf)
{
    VTSS_ASSERT(conf);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if (agent >= VTSS_APPL_AUTH_AGENT_LAST) {
        return VTSS_APPL_AUTH_ERROR_CFG_AGENT;
    }

    AUTH_CRIT_CONFIG_ENTER();
    *conf = config.authen_agent[agent];
    AUTH_CRIT_CONFIG_EXIT();
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  vtss_appl_auth_authen_agent_conf_set()
 *  Set the current authentication agent configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_authen_agent_conf_set(vtss_appl_auth_agent_t agent, const vtss_appl_auth_authen_agent_conf_t *const conf)
{
    int  i;
    BOOL changed = FALSE;
#ifdef VTSS_APPL_AUTH_ENABLE_CONSOLE
    BOOL close_serial = FALSE;
#endif
#ifdef VTSS_SW_OPTION_CLI_TELNET
    BOOL close_telnet = FALSE;
#endif
#if defined(VTSS_SW_OPTION_WEB)
    BOOL close_http = FALSE;
#endif

    VTSS_ASSERT(conf);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if (agent >= VTSS_APPL_AUTH_AGENT_LAST) {
        return VTSS_APPL_AUTH_ERROR_CFG_AGENT;
    }

    /* verify agent methods */
    for (i = 0; i <= (int)VTSS_APPL_AUTH_METHOD_PRI_IDX_MAX; i++ ) {
        if (conf->method[i] >= VTSS_APPL_AUTH_AUTHEN_METHOD_LAST) {
            return VTSS_APPL_AUTH_ERROR_CFG_AGENT_METHOD;
        }
    }

    AUTH_CRIT_CONFIG_ENTER();
    if (memcmp(&config.authen_agent[agent], conf, sizeof(*conf))) {
        config.authen_agent[agent] = *conf;
        changed = TRUE;
        config_changed = TRUE; /* Notify auth thread */

#ifdef VTSS_APPL_AUTH_ENABLE_CONSOLE
        if (agent == VTSS_APPL_AUTH_AGENT_CONSOLE) {
            close_serial = TRUE;
        }
#endif
#ifdef VTSS_SW_OPTION_CLI_TELNET
        if (agent == VTSS_APPL_AUTH_AGENT_TELNET) {
            close_telnet = TRUE;
        }
#endif
#if defined(VTSS_SW_OPTION_WEB)
        if (agent == VTSS_APPL_AUTH_AGENT_HTTP) {
            close_http = TRUE;
        }
#endif
    }
    AUTH_CRIT_CONFIG_EXIT();

    if (changed) {
        /* Closing down sessions must be done outside critical section as it might be activated from one of the closing sessions */
#if defined(VTSS_APPL_AUTH_ENABLE_CONSOLE)
        if (close_serial) {
            cli_serial_close();
        }
#endif
#ifdef VTSS_SW_OPTION_CLI_TELNET
        if (close_telnet) {
            cli_telnet_close();
        }
#endif
#if defined(VTSS_SW_OPTION_WEB)
        if (close_http) {
            AUTH_httpd_cache_expire(); /* force the httpd cache to be expired */
        }
#endif
#if defined(VTSS_SW_OPTION_FAST_CGI) && defined(VTSS_SW_OPTION_WEB)
        if (close_http) {
            (void)https_mgmt_conf_reload();
        }
#endif
    }

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  vtss_appl_auth_author_agent_conf_get()
 *  Returns the current authorization agent configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_author_agent_conf_get(vtss_appl_auth_agent_t agent, vtss_appl_auth_author_agent_conf_t *const conf)
{
    VTSS_ASSERT(conf);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if ((agent != VTSS_APPL_AUTH_AGENT_CONSOLE) &&
        (agent != VTSS_APPL_AUTH_AGENT_TELNET) &&
        (agent != VTSS_APPL_AUTH_AGENT_SSH)) {
        return VTSS_APPL_AUTH_ERROR_CFG_AGENT;
    }

    AUTH_CRIT_CONFIG_ENTER();
    *conf = config.author_agent[agent];
    AUTH_CRIT_CONFIG_EXIT();
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  vtss_appl_auth_author_agent_conf_set()
 *  Set the current authorization agent configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_author_agent_conf_set(vtss_appl_auth_agent_t agent, const vtss_appl_auth_author_agent_conf_t *const conf)
{
    VTSS_ASSERT(conf);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if ((agent != VTSS_APPL_AUTH_AGENT_CONSOLE) &&
        (agent != VTSS_APPL_AUTH_AGENT_TELNET) &&
        (agent != VTSS_APPL_AUTH_AGENT_SSH)) {
        return VTSS_APPL_AUTH_ERROR_CFG_AGENT;
    }

    if ((conf->method != VTSS_APPL_AUTH_AUTHOR_METHOD_NONE) &&
        (conf->method != VTSS_APPL_AUTH_AUTHOR_METHOD_TACACS)) {
        return VTSS_APPL_AUTH_ERROR_CFG_AGENT_METHOD;
    }

    if (conf->cmd_priv_lvl > 15) {
        return VTSS_APPL_AUTH_ERROR_CFG_PRIV_LVL;
    }

    AUTH_CRIT_CONFIG_ENTER();
    if (memcmp(&config.author_agent[agent], conf, sizeof(*conf))) {
        config.author_agent[agent] = *conf;
        config_changed = TRUE; /* Notify auth thread */
    }
    AUTH_CRIT_CONFIG_EXIT();

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  vtss_appl_auth_acct_agent_conf_get()
 *  Returns the current accounting agent configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_acct_agent_conf_get(vtss_appl_auth_agent_t agent, vtss_appl_auth_acct_agent_conf_t *const conf)
{
    VTSS_ASSERT(conf);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if ((agent != VTSS_APPL_AUTH_AGENT_CONSOLE) &&
        (agent != VTSS_APPL_AUTH_AGENT_TELNET) &&
        (agent != VTSS_APPL_AUTH_AGENT_SSH)) {
        return VTSS_APPL_AUTH_ERROR_CFG_AGENT;
    }

    AUTH_CRIT_CONFIG_ENTER();
    *conf = config.acct_agent[agent];
    AUTH_CRIT_CONFIG_EXIT();
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  vtss_appl_auth_acct_agent_conf_set()
 *  Set the current accounting agent configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_acct_agent_conf_set(vtss_appl_auth_agent_t agent, const vtss_appl_auth_acct_agent_conf_t *const conf)
{
    VTSS_ASSERT(conf);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if ((agent != VTSS_APPL_AUTH_AGENT_CONSOLE) &&
        (agent != VTSS_APPL_AUTH_AGENT_TELNET) &&
        (agent != VTSS_APPL_AUTH_AGENT_SSH)) {
        return VTSS_APPL_AUTH_ERROR_CFG_AGENT;
    }

    if ((conf->method != VTSS_APPL_AUTH_ACCT_METHOD_NONE) &&
        (conf->method != VTSS_APPL_AUTH_ACCT_METHOD_TACACS)) {
        return VTSS_APPL_AUTH_ERROR_CFG_AGENT_METHOD;
    }

    if (conf->cmd_priv_lvl > 15) {
        return VTSS_APPL_AUTH_ERROR_CFG_PRIV_LVL;
    }

    AUTH_CRIT_CONFIG_ENTER();
    if (memcmp(&config.acct_agent[agent], conf, sizeof(*conf))) {
        config.acct_agent[agent] = *conf;
        config_changed = TRUE; /* Notify auth thread */
    }
    AUTH_CRIT_CONFIG_EXIT();

    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_RADIUS
/* ================================================================= *
 *  vtss_appl_auth_radius_global_conf_get()
 *  Returns the current global RADIUS configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_radius_global_conf_get(vtss_appl_auth_radius_global_conf_t *const conf)
{
    mesa_rc rc = VTSS_RC_OK;

    VTSS_ASSERT(conf);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    AUTH_CRIT_CONFIG_ENTER();
    *conf = config.radius.global;
    AUTH_CRIT_CONFIG_EXIT();
    return rc;
}

static mesa_rc validate_and_encrypt_key(bool is_encrypted, const char *conf_key, char *encrypted_hex_str)
{
    char plain_txt[VTSS_APPL_AUTH_UNENCRYPTED_KEY_LEN];

    // First check format of new key, but don't (yet) check it for cryptographic validity:

    if (!AUTH_validate_secret_key(is_encrypted, conf_key)) {
        return VTSS_APPL_AUTH_ERROR_CFG_SECRET_KEY;
    }

    // If we got   encrypted key: Try to decrypt to verify validity
    // If we got unencrypted key: Encrypt it for subsequent storage UNLESS it's the empty string (default value)

    if (is_encrypted) {
        strcpy(encrypted_hex_str, conf_key);
        if (AUTH_secret_key_cryptography(FALSE, plain_txt, encrypted_hex_str) != VTSS_RC_OK) {
            T_D("Provided encrypted key is invalid");
            return VTSS_APPL_AUTH_ERROR_CFG_SECRET_KEY;
        }
    } else {
        strcpy(plain_txt, conf_key);
        if (plain_txt[0] && AUTH_secret_key_cryptography(TRUE, plain_txt, encrypted_hex_str) != VTSS_RC_OK) {
            T_D("Provided unencrypted key could not be encrypted");
            return VTSS_APPL_AUTH_ERROR_CFG_SECRET_KEY;
        }
    }
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  vtss_appl_auth_radius_global_conf_set()
 *  Set the current global RADIUS configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_radius_global_conf_set(const vtss_appl_auth_radius_global_conf_t *const conf)
{
    mesa_rc rc = VTSS_RC_OK;
    char    encrypted_hex_str[VTSS_APPL_AUTH_KEY_LEN];

    VTSS_ASSERT(conf);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if ((conf->timeout < VTSS_APPL_AUTH_TIMEOUT_MIN) || (conf->timeout > VTSS_APPL_AUTH_TIMEOUT_MAX)) {
        return VTSS_APPL_AUTH_ERROR_CFG_TIMEOUT;
    }
    if ((conf->retransmit < VTSS_APPL_AUTH_RETRANSMIT_MIN) || (conf->retransmit > VTSS_APPL_AUTH_RETRANSMIT_MAX)) {
        return VTSS_APPL_AUTH_ERROR_CFG_RETRANSMIT;
    }
    // If VTSS_APPL_AUTH_XXXX_MIN is 0, lint will complain
    /*lint --e{685, 568} */
    if ((conf->deadtime < VTSS_APPL_AUTH_DEADTIME_MIN) || (conf->deadtime > VTSS_APPL_AUTH_DEADTIME_MAX)) {
        return VTSS_APPL_AUTH_ERROR_CFG_DEADTIME;
    }
    if (conf->nas_ip_address_enable) {
        u8 a = (conf->nas_ip_address >> 24) & 0xff;
        /* Check that it is a legal unicast address */
        if ((conf->nas_ip_address == 0) || (a == 127) || (a > 223)) {
            return VTSS_APPL_AUTH_ERROR_CFG_RADIUS_NAS_IPV4;
        }
    }
    if (conf->nas_ipv6_address_enable) {
        /* Check that it is a legal unicast address */
        if (vtss_ipv6_addr_is_zero(&conf->nas_ipv6_address)       ||
            vtss_ipv6_addr_is_loopback(&conf->nas_ipv6_address)   ||
            vtss_ipv6_addr_is_multicast(&conf->nas_ipv6_address)  ||
            vtss_ipv6_addr_is_link_local(&conf->nas_ipv6_address)) {
            return VTSS_APPL_AUTH_ERROR_CFG_RADIUS_NAS_IPV6;
        }
    }
    if ((rc = validate_and_encrypt_key(conf->encrypted, conf->key, encrypted_hex_str)) != VTSS_RC_OK) {
        return rc;
    }

    AUTH_CRIT_CONFIG_ENTER();
    if (memcmp(&config.radius.global, conf, sizeof(*conf))) {
        config.radius.global = *conf;
        if (!config.radius.global.encrypted && config.radius.global.key[0]) {
            // Always saving encrypted hex string in local database if it's non-default (not empty string)
            strcpy(config.radius.global.key, encrypted_hex_str);
            config.radius.global.encrypted = TRUE;
        }
        rc = AUTH_radius_config_update(&config.radius);
        config_changed = TRUE; /* Notify auth thread */
    }
    AUTH_CRIT_CONFIG_EXIT();
    return rc;
}

/* ================================================================= *
 *  vtss_appl_auth_radius_server_add()
 *  Add a RADIUS server configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_radius_server_add(vtss_appl_auth_radius_server_conf_t *const conf)
{
    int     i;
    mesa_rc rc = VTSS_RC_OK;
    char    encrypted_hex_str[VTSS_APPL_AUTH_KEY_LEN];

    VTSS_ASSERT(conf);

    T_D("h: %s, au: %u, ac: %u, t: %u, r: %u, k: %s", conf->host, conf->auth_port, conf->acct_port, conf->timeout, conf->retransmit, conf->key);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (!conf->host[0] || vtss_appl_auth_server_address_valid(conf->host) != VTSS_RC_OK) {
        return VTSS_APPL_AUTH_ERROR_CFG_HOST;
    }
    if (conf->auth_port > 0xFFFF) {
        return VTSS_APPL_AUTH_ERROR_CFG_PORT;
    }
    if (conf->acct_port > 0xFFFF) {
        return VTSS_APPL_AUTH_ERROR_CFG_PORT;
    }
    if (conf->timeout && ((conf->timeout < VTSS_APPL_AUTH_TIMEOUT_MIN) || (conf->timeout > VTSS_APPL_AUTH_TIMEOUT_MAX))) {
        return VTSS_APPL_AUTH_ERROR_CFG_TIMEOUT;
    }
    if (conf->retransmit && ((conf->retransmit < VTSS_APPL_AUTH_RETRANSMIT_MIN) || (conf->retransmit > VTSS_APPL_AUTH_RETRANSMIT_MAX))) {
        return VTSS_APPL_AUTH_ERROR_CFG_RETRANSMIT;
    }
    if ((rc = validate_and_encrypt_key(conf->encrypted, conf->key, encrypted_hex_str)) != VTSS_RC_OK) {
        return rc;
    }

    AUTH_CRIT_CONFIG_ENTER();
    for (i = 0; i < VTSS_APPL_AUTH_NUMBER_OF_SERVERS; i++) {
        vtss_appl_auth_radius_server_conf_t *c = &config.radius.server[i];
        if (!(c->host[0])) {
            *c = *conf; // Entry was empty
            break;
        } else if (strcmp(c->host, conf->host) == 0) {
            if ((c->auth_port == conf->auth_port) && (c->acct_port == conf->acct_port)) {
                if ((conf->timeout)) {
                    c->timeout = conf->timeout;
                }
                if ((conf->retransmit)) {
                    c->retransmit = conf->retransmit;
                }
                if ((strlen(conf->key))) {
                    strcpy(c->key, conf->key);
                    config.radius.server[i].encrypted = FALSE;
                }
                break;
            } else if ((c->auth_port == conf->auth_port) || (c->acct_port == conf->acct_port) ||
                       (c->auth_port == conf->acct_port) || (c->acct_port == conf->auth_port)) {
                rc = VTSS_APPL_AUTH_ERROR_CFG_HOST_PORT;
                break;
            }
        }
    }
    if (i == VTSS_APPL_AUTH_NUMBER_OF_SERVERS) {
        rc = VTSS_APPL_AUTH_ERROR_CFG_HOST_TABLE_FULL;
    }
    if (rc == VTSS_RC_OK) {
        if (!config.radius.server[i].encrypted && config.radius.server[i].key[0]) {
            // Always saving encrypted hex string in local database if it's non-default (not empty string)
            strcpy(config.radius.server[i].key, encrypted_hex_str);
            config.radius.server[i].encrypted = TRUE;
        }
        rc = AUTH_radius_config_update(&config.radius);
        config_changed = TRUE; /* Notify auth thread */
    }
    AUTH_CRIT_CONFIG_EXIT();
    return rc;
}

/* ================================================================= *
 *  vtss_appl_auth_radius_server_del()
 *  Delete a RADIUS configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_radius_server_del(vtss_appl_auth_radius_server_conf_t *const conf)
{
    int     i;
    mesa_rc rc = VTSS_RC_OK;

    VTSS_ASSERT(conf);

    T_D("h: %s, au: %u, ac: %u", conf->host, conf->auth_port, conf->acct_port);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (!conf->host[0]) {
        return VTSS_APPL_AUTH_ERROR_CFG_HOST;
    }

    AUTH_CRIT_CONFIG_ENTER();
    for (i = 0; i < VTSS_APPL_AUTH_NUMBER_OF_SERVERS; i++) {
        vtss_appl_auth_radius_server_conf_t *c = &config.radius.server[i];
        if ((strcmp(c->host, conf->host) == 0) && (c->auth_port == conf->auth_port) && (c->acct_port == conf->acct_port)) {
            memset(c, 0, sizeof(*c)); // Delete entry
            break;
        }
    }
    if (i == VTSS_APPL_AUTH_NUMBER_OF_SERVERS) {
        rc = VTSS_APPL_AUTH_ERROR_CFG_HOST_NOT_FOUND;
    }
    if (rc == VTSS_RC_OK) {
        int j;
        for (j = i = 0; i < VTSS_APPL_AUTH_NUMBER_OF_SERVERS; i++) {
            vtss_appl_auth_radius_server_conf_t *c = &config.radius.server[i];
            if (strlen(c->host) > 0) {  /* Entry valid ? */
                if (i != j) {
                    vtss_appl_auth_radius_server_conf_t *hcg = &config.radius.server[j];
                    *hcg = *c;  /* Copy up */
                    memset(c, 0, sizeof(*c));  /* Nuke copied entry */
                }
                j++;  /* One more good entry */
            }
        }
        rc = AUTH_radius_config_update(&config.radius);
        config_changed = TRUE; /* Notify auth thread */
    }
    AUTH_CRIT_CONFIG_EXIT();
    return rc;
}

/* ================================================================= *
 *  vtss_appl_auth_radius_server_get()
 *  Get a RADIUS configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_radius_server_get(vtss_auth_host_index_t ix,
                                         vtss_appl_auth_radius_server_conf_t *conf)
{
    mesa_rc rc = VTSS_RC_OK;

    VTSS_ASSERT(conf);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if (ix < VTSS_APPL_AUTH_NUMBER_OF_SERVERS) {
        AUTH_CRIT_CONFIG_ENTER();
        *conf = config.radius.server[ix];
        AUTH_CRIT_CONFIG_EXIT();
    }
    return rc;
}

/* ================================================================= *
 *  vtss_appl_auth_radius_server_set()
 *  Set a RADIUS configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_radius_server_set(vtss_auth_host_index_t ix,
                                         const vtss_appl_auth_radius_server_conf_t *conf)
{
    mesa_rc rc = VTSS_RC_ERROR;
    char    encrypted_hex_str[VTSS_APPL_AUTH_KEY_LEN];

    VTSS_ASSERT(conf);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (strlen(conf->host) && vtss_appl_auth_server_address_valid(conf->host) != VTSS_RC_OK) {
        return VTSS_APPL_AUTH_ERROR_CFG_HOST;
    }
    if (conf->auth_port > 0xFFFF) {
        return VTSS_APPL_AUTH_ERROR_CFG_PORT;
    }
    if (conf->acct_port > 0xFFFF) {
        return VTSS_APPL_AUTH_ERROR_CFG_PORT;
    }
    if (conf->timeout && ((conf->timeout < VTSS_APPL_AUTH_TIMEOUT_MIN) || (conf->timeout > VTSS_APPL_AUTH_TIMEOUT_MAX))) {
        return VTSS_APPL_AUTH_ERROR_CFG_TIMEOUT;
    }
    if (conf->retransmit && ((conf->retransmit < VTSS_APPL_AUTH_RETRANSMIT_MIN) || (conf->retransmit > VTSS_APPL_AUTH_RETRANSMIT_MAX))) {
        return VTSS_APPL_AUTH_ERROR_CFG_RETRANSMIT;
    }
    if ((rc = validate_and_encrypt_key(conf->encrypted, conf->key, encrypted_hex_str)) != VTSS_RC_OK) {
        return rc;
    }

    if (ix < VTSS_APPL_AUTH_NUMBER_OF_SERVERS) {
        AUTH_CRIT_CONFIG_ENTER();
        config.radius.server[ix] = *conf;
        if (!config.radius.server[ix].encrypted && config.radius.server[ix].key[0]) {
            // Always saving encrypted hex string in local database if it's non-default (not empty string)
            strcpy(config.radius.server[ix].key, encrypted_hex_str);
            config.radius.server[ix].encrypted = TRUE;
        }
        rc = AUTH_radius_config_update(&config.radius);
        config_changed = TRUE; /* Notify auth thread */
        AUTH_CRIT_CONFIG_EXIT();
    }
    return rc;
}
#endif /* VTSS_SW_OPTION_RADIUS */

/* ================================================================= *
 *  vtss_appl_auth_tacacs_global_conf_get()
 *  Returns the current global TACACS+ configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_tacacs_global_conf_get(vtss_appl_auth_tacacs_global_conf_t *const conf)
{
#ifdef VTSS_SW_OPTION_TACPLUS
    mesa_rc rc = VTSS_RC_OK;

    VTSS_ASSERT(conf);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    AUTH_CRIT_CONFIG_ENTER();
    *conf = config.tacacs.global;
    AUTH_CRIT_CONFIG_EXIT();
    return rc;
#else
    return VTSS_RC_ERROR;
#endif
}

/* ================================================================= *
 *  vtss_appl_auth_tacacs_global_conf_set()
 *  Set the current global TACACS+ configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_tacacs_global_conf_set(const vtss_appl_auth_tacacs_global_conf_t *const conf)
{
#ifdef VTSS_SW_OPTION_TACPLUS
    char encrypted_hex_str[VTSS_APPL_AUTH_KEY_LEN];
    mesa_rc rc;

    VTSS_ASSERT(conf);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if ((conf->timeout < VTSS_APPL_AUTH_TIMEOUT_MIN) || (conf->timeout > VTSS_APPL_AUTH_TIMEOUT_MAX)) {
        return VTSS_APPL_AUTH_ERROR_CFG_TIMEOUT;
    }
    // If VTSS_APPL_AUTH_XXXX_MIN is 0, lint will complain
    /*lint --e{685, 568} */
    if ((conf->deadtime < VTSS_APPL_AUTH_DEADTIME_MIN) || (conf->deadtime > VTSS_APPL_AUTH_DEADTIME_MAX)) {
        return VTSS_APPL_AUTH_ERROR_CFG_DEADTIME;
    }
    if ((rc = validate_and_encrypt_key(conf->encrypted, conf->key, encrypted_hex_str)) != VTSS_RC_OK) {
        return rc;
    }

    AUTH_CRIT_CONFIG_ENTER();
    if (memcmp(&config.tacacs.global, conf, sizeof(*conf))) {
        config.tacacs.global = *conf;
        if (!config.tacacs.global.encrypted && config.tacacs.global.key[0]) {
            // Always saving encrypted hex string in local database if it's non-default (not empty string)
            strcpy(config.tacacs.global.key, encrypted_hex_str);
            config.tacacs.global.encrypted = TRUE;
        }
        config_changed = TRUE; /* Notify auth thread */
    }
    AUTH_CRIT_CONFIG_EXIT();
    return VTSS_RC_OK;
#else
    return VTSS_RC_ERROR;
#endif
}

/* ================================================================= *
 *  vtss_appl_auth_tacacs_server_add()
 *  Add a TACACS+ server configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_tacacs_server_add(vtss_appl_auth_tacacs_server_conf_t *const conf)
{
#ifdef VTSS_SW_OPTION_TACPLUS
    int     i;
    mesa_rc rc = VTSS_RC_OK;
    char    encrypted_hex_str[VTSS_APPL_AUTH_KEY_LEN];

    VTSS_ASSERT(conf);

    T_D("h: %s, p: %u, t: %u, k: %s", conf->host, conf->port, conf->timeout, conf->key);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (!conf->host[0] || vtss_appl_auth_server_address_valid(conf->host) != VTSS_RC_OK) {
        return VTSS_APPL_AUTH_ERROR_CFG_HOST;
    }
    if (conf->port > 0xFFFF) {
        return VTSS_APPL_AUTH_ERROR_CFG_PORT;
    }
    if (conf->timeout && ((conf->timeout < VTSS_APPL_AUTH_TIMEOUT_MIN) || (conf->timeout > VTSS_APPL_AUTH_TIMEOUT_MAX))) {
        return VTSS_APPL_AUTH_ERROR_CFG_TIMEOUT;
    }
    if ((rc = validate_and_encrypt_key(conf->encrypted, conf->key, encrypted_hex_str)) != VTSS_RC_OK) {
        return rc;
    }

    AUTH_CRIT_CONFIG_ENTER();
    for (i = 0; i < VTSS_APPL_AUTH_NUMBER_OF_SERVERS; i++) {
        vtss_appl_auth_tacacs_server_conf_t *c = &config.tacacs.server[i];
        if (!(c->host[0])) {
            *c = *conf; // Entry was empty
            break;
        } else if (strcmp(c->host, conf->host) == 0) {
            if (c->port == conf->port) {
                if ((conf->timeout)) {
                    c->timeout = conf->timeout;
                }

                strcpy(c->key, conf->key);
                c->encrypted = FALSE;
                break;
            }
        }
    }

    if (i == VTSS_APPL_AUTH_NUMBER_OF_SERVERS) {
        rc = VTSS_APPL_AUTH_ERROR_CFG_HOST_TABLE_FULL;
    }

    if (rc == VTSS_RC_OK) {
        if (!config.tacacs.server[i].encrypted && config.tacacs.server[i].key[0]) {
            // Always saving encrypted hex string in local database if it's non-default (not empty string)
            strcpy(config.tacacs.server[i].key, encrypted_hex_str);
            config.tacacs.server[i].encrypted = TRUE;
        }
        config_changed = TRUE; /* Notify auth thread */
    }
    AUTH_CRIT_CONFIG_EXIT();
    return rc;
#else
    return VTSS_RC_ERROR;
#endif
}

/* ================================================================= *
 *  vtss_appl_auth_tacacs_server_del()
 *  Delete a TACACS+ configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_tacacs_server_del(vtss_appl_auth_tacacs_server_conf_t *const conf)
{
#ifdef VTSS_SW_OPTION_TACPLUS
    int     i;
    mesa_rc rc = VTSS_RC_OK;

    VTSS_ASSERT(conf);

    T_D("h: %s, p: %u", conf->host, conf->port);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (!conf->host[0]) {
        return VTSS_APPL_AUTH_ERROR_CFG_HOST;
    }

    AUTH_CRIT_CONFIG_ENTER();
    for (i = 0; i < VTSS_APPL_AUTH_NUMBER_OF_SERVERS; i++) {
        vtss_appl_auth_tacacs_server_conf_t *c = &config.tacacs.server[i];
        if ((strcmp(c->host, conf->host) == 0) && (c->port == conf->port)) {
            memset(c, 0, sizeof(*c)); // Delete entry
            break;
        }
    }
    if (i == VTSS_APPL_AUTH_NUMBER_OF_SERVERS) {
        rc = VTSS_APPL_AUTH_ERROR_CFG_HOST_NOT_FOUND;
    }
    if (rc == VTSS_RC_OK) {
        int j;
        for (j = i = 0; i < VTSS_APPL_AUTH_NUMBER_OF_SERVERS; i++) {
            vtss_appl_auth_tacacs_server_conf_t *c = &config.tacacs.server[i];
            if (strlen(c->host) > 0) {  /* Entry valid ? */
                if (i != j) {
                    vtss_appl_auth_tacacs_server_conf_t *hcg = &config.tacacs.server[j];
                    *hcg = *c;  /* Copy up */
                    memset(c, 0, sizeof(*c));  /* Nuke copied entry */
                }
                j++;  /* One more good entry */
            }
        }
        config_changed = TRUE; /* Notify auth thread */
    }
    AUTH_CRIT_CONFIG_EXIT();
    return rc;
#else
    return VTSS_RC_ERROR;
#endif
}

/* ================================================================= *
 *  vtss_appl_auth_tacacs_server_get()
 *  Get a TACACS+ configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_tacacs_server_get(vtss_auth_host_index_t ix,
                                         vtss_appl_auth_tacacs_server_conf_t *conf)
{
#ifdef VTSS_SW_OPTION_TACPLUS
    mesa_rc rc = VTSS_RC_OK;

    VTSS_ASSERT(conf);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if (ix < VTSS_APPL_AUTH_NUMBER_OF_SERVERS) {
        AUTH_CRIT_CONFIG_ENTER();
        *conf = config.tacacs.server[ix];
        AUTH_CRIT_CONFIG_EXIT();
    }
    return rc;
#else
    return VTSS_RC_ERROR;
#endif
}

/* ================================================================= *
 *  vtss_appl_auth_tacacs_server_set()
 *  Set a TACACS+ configuration
 * ================================================================= */
mesa_rc vtss_appl_auth_tacacs_server_set(vtss_auth_host_index_t ix,
                                         const vtss_appl_auth_tacacs_server_conf_t *conf)
{
#ifdef VTSS_SW_OPTION_TACPLUS
    mesa_rc rc = VTSS_RC_ERROR;
    char    encrypted_hex_str[VTSS_APPL_AUTH_KEY_LEN];

    VTSS_ASSERT(conf);

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (strlen(conf->host) && vtss_appl_auth_server_address_valid(conf->host) != VTSS_RC_OK) {
        return VTSS_APPL_AUTH_ERROR_CFG_HOST;
    }
    if (conf->port > 0xFFFF) {
        return VTSS_APPL_AUTH_ERROR_CFG_PORT;
    }
    if (conf->timeout && ((conf->timeout < VTSS_APPL_AUTH_TIMEOUT_MIN) || (conf->timeout > VTSS_APPL_AUTH_TIMEOUT_MAX))) {
        return VTSS_APPL_AUTH_ERROR_CFG_TIMEOUT;
    }
    if ((rc = validate_and_encrypt_key(conf->encrypted, conf->key, encrypted_hex_str)) != VTSS_RC_OK) {
        return rc;
    }

    if (ix < VTSS_APPL_AUTH_NUMBER_OF_SERVERS) {
        AUTH_CRIT_CONFIG_ENTER();
        config.tacacs.server[ix] = *conf;
        if (!config.tacacs.server[ix].encrypted && config.tacacs.server[ix].key[0]) {
            // Always saving encrypted hex string in local database if it's non-default (not empty string)
            strcpy(config.tacacs.server[ix].key, encrypted_hex_str);
            config.tacacs.server[ix].encrypted = TRUE;
        }
        config_changed = TRUE; /* Notify auth thread */
        AUTH_CRIT_CONFIG_EXIT();
        rc = VTSS_RC_OK;
    }
    return rc;
#else
    return VTSS_RC_ERROR;
#endif
}

/**
 * \brief Identify the AUTH server address string.
 *
 * The valid host address string can only be 'EMPTY' or valid IPv4/IPv6 unicast address
 * or valid DNS name.  Otherwise, the address string is treated as invalid input.
 *
 * \param srv   [IN]  Host address string
 *
 * \return VTSS_RC_OK on valid server address string, otherwise return the error code.
 */
mesa_rc vtss_appl_auth_server_address_valid(const char *const srv)
{
    if (!srv || strlen(srv) > VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN) {
        return VTSS_RC_ERROR;
    }
    if (strlen(srv) == 0) {
        return VTSS_RC_OK;
    }

    if (misc_str_is_hostname(srv) != VTSS_RC_OK) {
#ifdef VTSS_SW_OPTION_IPV6
        struct sockaddr_in6 host;

        memset(&host, 0x0, sizeof(struct sockaddr_in6));
        if (inet_pton(AF_INET6, srv, (char *)&host.sin6_addr) <= 0 ||
            vtss_ipv6_addr_is_loopback((mesa_ipv6_t *)&host.sin6_addr) ||
            vtss_ipv6_addr_is_link_local((mesa_ipv6_t *)&host.sin6_addr) ||
            vtss_ipv6_addr_is_multicast((mesa_ipv6_t *)&host.sin6_addr)) {
            return VTSS_APPL_AUTH_ERROR_CFG_HOST;
        }
#else
        return VTSS_APPL_AUTH_ERROR_CFG_HOST;
#endif /* VTSS_SW_OPTION_IPV6 */
    }

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  vtss_auth_mgmt_httpd_cache_expire()
 *  Clear the HTTPD cache.
 * ================================================================= */
void vtss_auth_mgmt_httpd_cache_expire(void)
{
    AUTH_httpd_cache_expire();
}

/* ================================================================= *
 *  vtss_auth_mgmt_httpd_authenticate()
 *  Authenticate HTTP requests.
 * ================================================================= */
int vtss_auth_mgmt_httpd_authenticate(char *username, char *password, int *priv_lvl)
{
#if defined(VTSS_SW_OPTION_WEB)
    return AUTH_httpd_authenticate(username, password, priv_lvl);
#else
    return 0;
#endif
}

/* ================================================================= *
 *  vtss_auth_init()
 *  Initialize the auth module
 * ================================================================= */
VTSS_PRE_DECLS void auth_mib_init(void);
VTSS_PRE_DECLS void vtss_appl_auth_json_init(void);
extern "C" int vtss_auth_icli_cmd_register();
mesa_rc vtss_auth_init(vtss_init_data_t *data)
{
    vtss_isid_t     isid = data->isid;
    msg_rx_filter_t filter;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_N("INIT");

        // The lock time for crit_cache depends on the host timeout/retransmit configuration.
        // Disable critd's check for "deadlock" by allowing a very high lock time.
        crit_cache.max_lock_time = 60 * 60 * 24; // 24 hours

        critd_init(&crit_config, "auth.config", VTSS_MODULE_ID_AUTH, CRITD_TYPE_MUTEX);
        critd_init(&crit_cache,  "auth.cache",  VTSS_MODULE_ID_AUTH, CRITD_TYPE_MUTEX);

        AUTH_client_pool_init();
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        auth_mib_init();  /* Register our private mib */
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_auth_json_init();
#endif
        vtss_auth_icli_cmd_register();

#ifdef VTSS_SW_OPTION_ICFG
        if (vtss_auth_icfg_init() != VTSS_RC_OK) {
            T_E("ICFG not initialized correctly");
        }
#endif

        /* Initialize thread data */
        memset(&thread, 0, sizeof(thread));
        break;

    case INIT_CMD_START:
        T_N("START");
        /* Register for stack messages */
        memset(&filter, 0, sizeof(filter));
        filter.cb = AUTH_msg_rx;
        filter.modid = VTSS_MODULE_ID_AUTH;
        (void)msg_rx_filter_register(&filter);

        AUTH_conf_default(); /* Create default configuration */

        /* Create thread */
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           AUTH_thread,
                           0,
                           "Authentication",
                           nullptr,
                           0,
                           &thread.handle,
                           &thread.state);

        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           AUTH_service_thread,
                           0,
                           "auth-service",
                           nullptr,
                           0,
                           &service_handle,
                           0);

        break;

    case INIT_CMD_CONF_DEF:
        T_N("CONF_DEF (isid = %d)", isid);
        if (isid == VTSS_ISID_GLOBAL) {
            AUTH_conf_default(); /* Create default configuration */
            AUTH_httpd_cache_expire(); /* Force the httpd cache to be expired */
        }
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_N("ICFG_LOADING_POST");
        AUTH_new_primary_switch(isid); /* Send new-primary-switch-notification */
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

