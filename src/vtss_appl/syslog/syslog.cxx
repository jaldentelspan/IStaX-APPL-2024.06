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

// This file implements the RAM-based syslog. For the flash-based, please see
// syslog_flash.cxx.

#include "main.h"
#include "syslog_api.h"
#include "critd_api.h"
#include "led_api.h"
#include "misc_api.h"
#include "vtss_trace_api.h"
#include "msg_api.h"
#include "syslog_icfg.h"
#include "icli_porting_util.h"
#include "vtss_hostaddr.h"

#include <vtss/appl/syslog.h>
#include <vtss/basics/map.hxx>
#include <vtss/basics/synchronized.hxx>
#include <vtss/basics/notifications.hxx>

#ifdef VTSS_SW_OPTION_JSON_RPC_NOTIFICATION
#include "json_rpc_api.hxx"
#include <vtss/basics/json/stream-parser.hxx>
#include <vtss/basics/json/stream-parser-callback.hxx>
#endif

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_SYSLOG
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_SYSLOG
#define SYSLOG_RAM_SIZE      (1024 * 1024)
#define NTP_DELAY_SEC        15    /* 15 seconds */
#define SL_THREAD_DELAY_SEC  2     /* 2 seconds  */

/******************************************************************************/
// Stack message IDs
/******************************************************************************/
typedef enum {
    SL_MSG_ID_ENTRY_GET_REQ,    /* Get entry request */
    SL_MSG_ID_ENTRY_GET_REP,    /* Get entry reply */
    SL_MSG_ID_STAT_GET_REQ,     /* Get statistics request */
    SL_MSG_ID_STAT_GET_REP,     /* Get statistics reply */
    SL_MSG_ID_CLR_REQ,          /* Clear request (no reply) */
    SL_MSG_ID_CONF_SET_REQ      /* syslog configuration set request (no reply) */
} SL_msg_id_t;

/******************************************************************************/
// Stack request message
/******************************************************************************/
typedef struct {
    SL_msg_id_t msg_id; /* Message ID */

    union {
        /* SL_MSG_ID_ENTRY_GET_REQ */
        struct {
            BOOL                    next;
            ulong                   id;
            vtss_appl_syslog_lvl_t  lvl;
            vtss_module_id_t        mid;
        } entry_get;

        /* SL_MSG_ID_STAT_GET_REQ: No data */

        /* SL_MSG_ID_CLR_REQ */
        struct {
            vtss_appl_syslog_lvl_t lvl;
        } entry_clear;

        /* SL_MSG_ID_CONF_SET_REQ */
        struct {
            vtss_appl_syslog_server_conf_t conf;
        } conf_set;
    } data;
} SL_msg_req_t;

/******************************************************************************/
// Stack reply message
/******************************************************************************/
typedef struct {
    SL_msg_id_t msg_id; /* Message ID */
    union {
        /* SL_MSG_ID_ENTRY_GET_REP */
        struct {
            BOOL               found;
            syslog_ram_entry_t entry;
        } entry_get;

        /* SL_MSG_ID_STAT_GET_REP */
        struct {
            syslog_ram_stat_t stat;
        } stat_get;
    } data;
} SL_msg_rep_t;

/******************************************************************************/
// RAM entry
/******************************************************************************/
typedef struct SL_ram_entry_t {
    struct SL_ram_entry_t  *next;
    ulong                  id;                         /* Message ID */
    vtss_appl_syslog_lvl_t lvl;                        /* Level */
    vtss_module_id_t       mid;                        /* Module ID */
    time_t                 time;                       /* Time stamp */
    char                   msg[SYSLOG_RAM_MSG_MAX];    /* Message */
} SL_ram_entry_t;

/******************************************************************************/
// Variables for RAM system log
/******************************************************************************/
typedef struct {
    uchar             log[SYSLOG_RAM_SIZE];
    syslog_ram_stat_t stat[VTSS_ISID_END];
    vtss_flag_t       stat_flags;
    SL_ram_entry_t    *first;       /* First entry in list */
    SL_ram_entry_t    *last;        /* Last entry in list */
    ulong             current_id;   /* current ID */

    /* Request buffer */
    void *request;

    /* Reply buffer */
    void *reply;

    /* Management reply buffer */
    struct {
        vtss_sem_t         sem;
        vtss_flag_t        flags;
        BOOL               found;
        syslog_ram_entry_t *entry;
    } mgmt_reply;
} SL_ram_t;

/******************************************************************************/
// Private global structure
/******************************************************************************/
static struct {
    critd_t                 crit;
    vtss_appl_syslog_server_conf_t conf;
    vtss_flag_t             conf_flags;
    vtss_mtimer_t           conf_timer[VTSS_ISID_END];
    ulong                   send_msg_id[VTSS_ISID_END];     /* Record message ID that already send to syslog server */
    time_t                  current_time;
} SL_global;

static BOOL SYSLOG_init = FALSE;

/* Thread variables */
static vtss_handle_t SYSLOG_thread_handle;
static vtss_thread_t SYSLOG_thread_block;

static SL_ram_t SL_ram;
static critd_t  SL_ram_crit;

/******************************************************************************/
// Trace variables
/******************************************************************************/
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "Syslog", "Syslog Module"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define SYSLOG_RAM_CRIT_ENTER() critd_enter(&SL_ram_crit,    __FILE__, __LINE__)
#define SYSLOG_RAM_CRIT_EXIT()  critd_exit( &SL_ram_crit,    __FILE__, __LINE__)
#define SYSLOG_GLB_CRIT_ENTER() critd_enter(&SL_global.crit, __FILE__, __LINE__)
#define SYSLOG_GLB_CRIT_EXIT()  critd_exit( &SL_global.crit, __FILE__, __LINE__)

#ifdef VTSS_SW_OPTION_JSON_RPC_NOTIFICATION
namespace vtss
{
extern vtss::expose::json::RootNode JSON_RPC_ROOT;
}

namespace vtss
{
namespace notifications
{
extern vtss::notifications::SubjectRunner subject_main_thread;

}  // namespace notifications
}  // namespace vtss

/******************************************************************************/
// operator<()
/******************************************************************************/
bool operator<(const vtss_appl_syslog_notif_name_t &x,
               const vtss_appl_syslog_notif_name_t &y)
{
    std::string x_(x.notif_name);
    std::string y_(y.notif_name);
    return x_ < y_;
}

using namespace vtss;
using namespace notifications;

namespace vtss
{
namespace appl
{
namespace syslog
{

/******************************************************************************/
// SyslogPrinter
/******************************************************************************/
struct SyslogPrinter : public json::StreamParserCallback {
    enum class SyslogPrinterState {
        Method,
        Id,
        Params,
        SyslogEntry,
        SyslogEntryCmd,
        SyslogEntryKey,
        SyslogEntryVal,
    };

    enum class SyslogPrinterCommand {
        Add, Delete, Modify
    };

    SyslogPrinter(std::string name, vtss_appl_syslog_lvl_t level)
        : state(SyslogPrinterState::Method), name_(name), level_(level) {}

    Action array_start() override
    {
        if (state == SyslogPrinterState::Params) {
            state = SyslogPrinterState::SyslogEntry;
        }
        return StreamParserCallback::Action::ACCEPT;
    }

    void array_end() override
    {
    }

    Action object_start() override
    {
        switch (state) {
        case SyslogPrinterState::SyslogEntryKey:
            key_ += "{";
            parse_level++;
            break;
        case SyslogPrinterState::SyslogEntryVal:
            val_ += "{";
            parse_level++;
            break;
        default:
            break;
        }
        return StreamParserCallback::Action::ACCEPT;
    }

    void object_end() override
    {
        switch (state) {
        case SyslogPrinterState::SyslogEntry:
            switch (cmd) {
            case SyslogPrinterCommand::Add:
                SL(level_,
                   VTSS_ISID_END,
                   fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT),
                   "%s: Created %s, Value: %s",
                   name_.c_str(), key_.c_str(), val_.c_str());
                break;
            case SyslogPrinterCommand::Delete:
                SL(level_,
                   VTSS_ISID_END,
                   fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT),
                   "%s: Deleted %s",
                   name_.c_str(), key_.c_str());
                break;
            case SyslogPrinterCommand::Modify:
                SL(level_,
                   VTSS_ISID_END,
                   fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT),
                   "%s: Updated %s, Value: %s",
                   name_.c_str(), key_.c_str(), val_.c_str());
                break;
            }
            break;
        case SyslogPrinterState::SyslogEntryKey:
            key_ += "}";
            parse_level--;
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        case SyslogPrinterState::SyslogEntryVal:
            first_element--;
            val_ += "}";
            parse_level--;
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        default:
            break;
        }
    }

    Action object_element_start(const std::string &s) override
    {
        switch (state) {
        case SyslogPrinterState::SyslogEntry:
            if (s == "event-type") {
                state = SyslogPrinterState::SyslogEntryCmd;
            } else if (s == "val") {
                state = SyslogPrinterState::SyslogEntryVal;
                val_.clear();
                first_element = 0;
                parse_level = 0;
            } else if (s == "key") {
                state = SyslogPrinterState::SyslogEntryKey;
                key_.clear();
                first_element = 0;
                parse_level = 0;
            }
            return StreamParserCallback::Action::ACCEPT;
        case SyslogPrinterState::Method:
            state = SyslogPrinterState::Id;
            return StreamParserCallback::Action::SKIP;
        case SyslogPrinterState::Id:
            state = SyslogPrinterState::Params;
            return StreamParserCallback::Action::SKIP;
        case SyslogPrinterState::SyslogEntryKey:
            if (first_element < parse_level) {
                first_element++;
            } else {
                key_ += ",";
            }
            key_ += "\"" + s + "\":";
            return StreamParserCallback::Action::ACCEPT;
        case SyslogPrinterState::SyslogEntryVal:
            if (first_element < parse_level) {
                first_element++;
            } else {
                val_ += ",";
            }
            val_ += "\"" + s + "\":";
            return StreamParserCallback::Action::ACCEPT;
        default:
            break;
        }
        return StreamParserCallback::Action::ACCEPT;
    }

    void object_element_end() override
    {
        switch (state) {
        default:
            break;
        }
    }

    void null() override
    {
        switch (state) {
        case SyslogPrinterState::SyslogEntryKey:
            key_ += "null";
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        case SyslogPrinterState::SyslogEntryVal:
            val_ += "null";
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        default:
            break;
        }
    }

    void boolean(bool val) override
    {
        switch (state) {
        case SyslogPrinterState::SyslogEntryKey:
            key_ += (val ? "true" : "false");
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        case SyslogPrinterState::SyslogEntryVal:
            val_ += (val ? "true" : "false");
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        default:
            break;
        }
    }

    void number_value(uint32_t i) override
    {
        switch (state) {
        case SyslogPrinterState::SyslogEntryKey:
            key_ += std::to_string(i);
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        case SyslogPrinterState::SyslogEntryVal:
            val_ += std::to_string(i);
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        default:
            break;
        }
    }

    void number_value(int32_t i) override
    {
        switch (state) {
        case SyslogPrinterState::SyslogEntryKey:
            key_ += std::to_string(i);
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        case SyslogPrinterState::SyslogEntryVal:
            val_ += std::to_string(i);
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        default:
            break;
        }
    }

    void number_value(uint64_t i) override
    {
        switch (state) {
        case SyslogPrinterState::SyslogEntryKey:
            key_ += std::to_string(i);
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        case SyslogPrinterState::SyslogEntryVal:
            val_ += std::to_string(i);
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        default:
            break;
        }
    }

    void number_value(int64_t i) override
    {
        switch (state) {
        case SyslogPrinterState::SyslogEntryKey:
            key_ += std::to_string(i);
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        case SyslogPrinterState::SyslogEntryVal:
            val_ += std::to_string(i);
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        default:
            break;
        }
    }

    void string_value(const std::string &&s) override
    {
        switch (state) {
        case SyslogPrinterState::SyslogEntryCmd:
            if (s == "add") {
                cmd = SyslogPrinterCommand::Add;
            } else if (s == "modify") {
                cmd = SyslogPrinterCommand::Modify;
            } else if (s == "delete") {
                cmd = SyslogPrinterCommand::Delete;
            }
            state = SyslogPrinterState::SyslogEntry;
            break;
        case SyslogPrinterState::SyslogEntryKey:
            key_ += "\"" + s + "\"";
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        case SyslogPrinterState::SyslogEntryVal:
            val_ += "\"" + s + "\"";
            if (parse_level == 0) {
                state = SyslogPrinterState::SyslogEntry;
            }
            break;
        default:
            break;
        }

    }

    void stream_error() override
    {
    }

    bool ok()
    {
        if (err != 0) {
            return false;
        }

        return true;
    }

    uint32_t err = 0;
    SyslogPrinterState state;
    SyslogPrinterCommand cmd;
    int first_element;
    std::string key_;
    std::string val_;
    int parse_level;
    std::string name_;
    vtss_appl_syslog_lvl_t level_;
};

/******************************************************************************/
// SyslogNotificationHandler
/******************************************************************************/
struct SyslogNotificationHandler : public vtss::notifications::EventHandler {
    SyslogNotificationHandler(const std::string &name,
                              const std::string &notif_name,
                              vtss_appl_syslog_lvl_t level)
        : EventHandler(&subject_main_thread), name_(name), event(this), level_(level)
    {
        T_D("Create SyslogNotificationHandler for %s", name.c_str());
        expose::json::Node *node = JSON_RPC_ROOT.lookup(notif_name);

        if (node == nullptr) {
            T_D("Null pointer met: node: %s:%s", name.c_str(), notif_name.c_str());
            return;
        }

        expose::json::Node *update = node->lookup(str("update"));
        if (update == nullptr || !update->is_notification()) {
            T_E("No update found: %s:%s", name.c_str(), notif_name.c_str());
            return;
        }
        notif = static_cast<expose::json::Notification *>(update);
        notif->observer_new(&event);
    }

    ~SyslogNotificationHandler()
    {
        if (notif) {
            notif->observer_del(&event);
        }
    }

    virtual void execute(vtss::notifications::Event *e)
    {
        T_D("SyslogNotificationHandler for %s level %d", name_.c_str(), level_);
        std::string s;
        auto rc = notif->observer_get(&event, s);
        if (rc != VTSS_RC_OK) {
            s = "Failed to get value";
            SL(level_, VTSS_ISID_END, fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT), "%s: %s", name_.c_str(), s.c_str());
            return;
        }
        SyslogPrinter slp(name_, level_);
        json::StreamParser stream_parser(&slp);
        stream_parser.process(s);
    }

    std::string name_;
    expose::json::Notification *notif;
    notifications::Event event;
    vtss_appl_syslog_lvl_t level_;
};

/******************************************************************************/
// syslog_notif_conf_t
/******************************************************************************/
struct syslog_notif_conf_t : public vtss_appl_syslog_notif_conf_t {
    syslog_notif_conf_t(const vtss_appl_syslog_notif_name_t &nm,
                        const vtss_appl_syslog_notif_conf_t &conf) :
        handler(new SyslogNotificationHandler(std::string(nm.notif_name),
                                              std::string(conf.notification),
                                              conf.level))
    {
        memcpy(notification, conf.notification, sizeof(notification));
        level = conf.level;
    }
    std::unique_ptr<SyslogNotificationHandler> handler;
};

/******************************************************************************/
// State
/******************************************************************************/
struct State {
    vtss::Map<std::string, syslog_notif_conf_t> the_syslog_notif_conf;
};

Synchronized<State, VTSS_TRACE_MODULE_ID> syslog_notif_conf;

} // namespace syslog
} // namespace appl
} // namespace vtss

using namespace appl;
using namespace syslog;
#endif // VTSS_SW_OPTION_JSON_RPC_NOTIFICATION

/******************************************************************************/
// SL_msg_id_txt()
/******************************************************************************/
static const char *SL_msg_id_txt(SL_msg_id_t msg_id)
{
    const char *txt;

    switch (msg_id) {
    case SL_MSG_ID_ENTRY_GET_REQ:
        txt = "SL_MSG_ID_ENTRY_GET_REQ";
        break;
    case SL_MSG_ID_ENTRY_GET_REP:
        txt = "SL_MSG_ID_ENTRY_GET_REP";
        break;
    case SL_MSG_ID_STAT_GET_REQ:
        txt = "SL_MSG_ID_STAT_GET_REQ";
        break;
    case SL_MSG_ID_STAT_GET_REP:
        txt = "SL_MSG_ID_STAT_GET_REP";
        break;
    case SL_MSG_ID_CONF_SET_REQ:
        txt = "SL_MSG_ID_CONF_SET_REQ";
        break;
    default:
        txt = "?";
        break;
    }
    return txt;
}

/******************************************************************************/
// SL_get_time_in_secs()
/******************************************************************************/
static time_t SL_get_time_in_secs(void)
{
    return time(NULL);
}

/******************************************************************************/
// SL_msg_req_alloc()
// Allocate request buffer
/******************************************************************************/
static SL_msg_req_t *SL_msg_req_alloc(SL_msg_id_t msg_id)
{
    SL_msg_req_t *msg = (SL_msg_req_t *)msg_buf_pool_get(SL_ram.request);
    VTSS_ASSERT(msg);
    msg->msg_id = msg_id;
    return msg;
}

/******************************************************************************/
// SL_msg_req_alloc()
// Allocate reply buffer
/******************************************************************************/
static SL_msg_rep_t *SL_msg_rep_alloc(SL_msg_id_t msg_id)
{
    SL_msg_rep_t *msg = (SL_msg_rep_t *)msg_buf_pool_get(SL_ram.reply);
    VTSS_ASSERT(msg);
    msg->msg_id = msg_id;
    return msg;
}

/******************************************************************************/
// SL_msg_tx_done()
/******************************************************************************/
static void SL_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
    (void)msg_buf_pool_put(msg);
}

/******************************************************************************/
// SL_msg_tx()
/******************************************************************************/
static void SL_msg_tx(void *msg, vtss_isid_t isid, size_t len)
{
    SL_msg_id_t msg_id = *(SL_msg_id_t *)msg;
    size_t total_len = len + MSG_TX_DATA_HDR_LEN_MAX(SL_msg_req_t, data, SL_msg_rep_t, data);

    T_D("isid: %d, msg_id: %d(%s), len: %zu", isid, msg_id, SL_msg_id_txt(msg_id), total_len);
    T_R_HEX((const uchar *)msg, total_len);
    msg_tx_adv(NULL, SL_msg_tx_done, (msg_tx_opt_t)(MSG_TX_OPT_DONT_FREE | MSG_TX_OPT_NO_ALLOC_ON_LOOPBACK), VTSS_MODULE_ID_SYSLOG, isid, msg, total_len);
}

/******************************************************************************/
// SL_port_info_insert()
//
// Insert port info.(isid/iport) in the magic keyword
// (SYSLOG_PORT_INFO_REPLACE_KEYWORD). When the primary switch requests and
// later receives the syslog from the secondary switch, it will replace all
// these place-holders with the actual/correct USIDs from that secondary switch.
/******************************************************************************/
static void SL_port_info_insert(vtss_isid_t isid, mesa_port_no_t iport, char *msg)
{
    char *search_msg = msg, *ptr;
    char replace_msg[strlen(SYSLOG_PORT_INFO_REPLACE_KEYWORD) + 1];

    /* Found the magic keyword for the replacement */
    sprintf(replace_msg, "%s%2d/%2d%s", SYSLOG_PORT_INFO_REPLACE_TAG_START, isid, iport, SYSLOG_PORT_INFO_REPLACE_TAG_END);
    while ((ptr = strstr(search_msg, SYSLOG_PORT_INFO_REPLACE_KEYWORD)) != NULL) {
        // Replace XX/YY to real isid/iport
        strncpy(ptr, replace_msg, strlen(SYSLOG_PORT_INFO_REPLACE_KEYWORD));

        // Start next lookup
        search_msg = ptr + strlen(SYSLOG_PORT_INFO_REPLACE_KEYWORD);
    }
}

/******************************************************************************/
// SL_port_info_replace()
//
// Replace the text in port info. tag to real port interface text.
// For example, "<PORT_INFO_S>03/15<PORT_INFO_E>" will be replaced with
// "GigabitEthernet 4/16".
/******************************************************************************/
static void SL_port_info_replace(vtss_isid_t current_isid, char *ori_msg)
{
    vtss_isid_t     isid;
    mesa_port_no_t  iport;
    char            *search_msg = ori_msg, *tag_start, *tag_end;
    char            *temp_buff = NULL, *new_msg = NULL, *temp_ptr;
    char            replace_msg[64], isid_txt[4], iport_txt[4];
    BOOL            found_keyword = FALSE;
    int             i;

    /* Found magic keyword for the replacement */
    while ((tag_start = strstr(search_msg, SYSLOG_PORT_INFO_REPLACE_TAG_START)) != NULL &&
           (tag_end = strstr(search_msg, SYSLOG_PORT_INFO_REPLACE_TAG_END)) != NULL) {
        if (found_keyword == FALSE) {
            if ((VTSS_MALLOC_CAST(temp_buff, SYSLOG_RAM_MSG_MAX)) == NULL) {
                break;
            }
            new_msg = temp_buff;

            // Found magic keyword first time, copy unreplaced text
            strncpy(new_msg, ori_msg, tag_start - ori_msg);
            new_msg += (tag_start - ori_msg);

            found_keyword = TRUE;
        }

        // Parse isid/iport from magic keyword (format: XX/YY)
        i = 0;
        temp_ptr = tag_start + sizeof(SYSLOG_PORT_INFO_REPLACE_TAG_START);
        do {
            isid_txt[i++] = *temp_ptr;
            temp_ptr++;
        } while (i < 3 && (*temp_ptr != '/'));
        isid_txt[i] = '\0';
        isid = (vtss_isid_t) atoi(isid_txt);

        i = 0;
        temp_ptr++;
        do {
            iport_txt[i++] = *temp_ptr;
            temp_ptr++;
        } while (i < 3 && (temp_ptr != tag_end));
        iport_txt[i] = '\0';
        iport = (mesa_port_no_t) atoi(iport_txt);
        T_D("isid=%d, iport=%d", isid, iport);

        (void)icli_port_info_txt(VTSS_USID_LOCAL, iport2uport(iport), replace_msg);

        // Place interface text
        sprintf(new_msg, "%s", replace_msg);
        new_msg += strlen(replace_msg);

        // Start next lookup
        search_msg = tag_end + strlen(SYSLOG_PORT_INFO_REPLACE_TAG_END);
    }

    if (found_keyword) {
        // Copy remained message
        if (strlen(search_msg)) {
            strcpy(new_msg, search_msg);
        }

        // Update to the original message
        if (temp_buff) {
            strcpy(ori_msg, temp_buff);
            VTSS_FREE(temp_buff);
        }
    }
}

/******************************************************************************/
// SL_ram_get()
// Get RAM system log entry
/******************************************************************************/
static BOOL SL_ram_get(vtss_isid_t              isid,    /* ISID */
                       BOOL                     next,    /* Next or specific entry */
                       ulong                    id,      /* Entry ID */
                       vtss_appl_syslog_lvl_t   lvl,     /* SYSLOG_LVL_ALL is wildcard */
                       vtss_module_id_t         mid,     /* VTSS_MODULE_ID_NONE is wildcard */
                       syslog_ram_entry_t       *entry,  /* Returned data */
                       BOOL                     convert)
{
    SL_ram_entry_t *cur;
    BOOL           is_wrap, is_in_wrap;

    if (isid != VTSS_ISID_LOCAL && !msg_switch_is_local(isid)) {
        SL_msg_req_t     *req;
        vtss_flag_value_t flag;
        BOOL             found;

        /* Wait for reply buffer semaphore and setup pointer */
        vtss_sem_wait(&SL_ram.mgmt_reply.sem);
        SYSLOG_RAM_CRIT_ENTER();
        SL_ram.mgmt_reply.entry = entry;
        SL_ram.mgmt_reply.found = 0;
        flag = (1 << 0);
        vtss_flag_maskbits(&SL_ram.mgmt_reply.flags, ~flag);
        SYSLOG_RAM_CRIT_EXIT();

        /* Send request message */
        req = SL_msg_req_alloc(SL_MSG_ID_ENTRY_GET_REQ);
        req->data.entry_get.next = next;
        req->data.entry_get.id = id;
        req->data.entry_get.lvl = lvl;
        req->data.entry_get.mid = mid;
        SL_msg_tx(req, isid, sizeof(req->data.entry_get));

        /* Wait for reply */
        (void)vtss_flag_timed_wait(&SL_ram.mgmt_reply.flags, flag, VTSS_FLAG_WAITMODE_OR, vtss_current_time() + VTSS_OS_MSEC2TICK(5000));

        /* Clear pointer and release post reply buffer semaphore */
        SYSLOG_RAM_CRIT_ENTER();
        SL_ram.mgmt_reply.entry = NULL;
        found = SL_ram.mgmt_reply.found;
        if (found) {
            entry->time = msg_abstime_get(isid, entry->time);
        }
        SYSLOG_RAM_CRIT_EXIT();
        vtss_sem_post(&SL_ram.mgmt_reply.sem);

        return found;
    }

    /* Local log access */
    SYSLOG_RAM_CRIT_ENTER();

    // Check NULL point
    if (!SL_ram.first || !SL_ram.last) {
        SYSLOG_RAM_CRIT_EXIT();
        return FALSE;
    }

    is_wrap = SL_ram.first->id > SL_ram.last->id;
    is_in_wrap = id < SL_ram.first->id;
    for (cur = (SL_ram_entry_t *)SL_ram.first;
         cur != NULL; cur = cur->next) {
        /* Check ID for GET_NEXT operation */
        if (next && id) {
            if ((!is_wrap && cur->id <= id) ||
                (is_wrap && ((is_in_wrap && (cur->id <= id || cur->id >= SL_ram.first->id)) || (!is_in_wrap && cur->id <= id && cur->id >= SL_ram.first->id)))) {
                continue;
            }
        }

        /* Check ID for GET operation */
        if (!next && cur->id != id) {
            continue;
        }

        /* Check level */
        if (lvl != VTSS_APPL_SYSLOG_LVL_ALL && cur->lvl != lvl) {
            continue;
        }

        /* Check module ID */
        if (mid != VTSS_MODULE_ID_NONE && cur->mid != mid) {
            continue;
        }

        /* Copy data */
        entry->id = cur->id;
        entry->lvl = cur->lvl;
        entry->mid = cur->mid;
        entry->time = (convert ? msg_abstime_get(isid, cur->time) : cur->time);

        strcpy(entry->msg, cur->msg);
        break;
    }
    SYSLOG_RAM_CRIT_EXIT();

    return (cur == NULL ? 0 : 1);
}

/******************************************************************************/
// SL_ram_stat_get()
/******************************************************************************/
static mesa_rc SL_ram_stat_get(vtss_isid_t isid, syslog_ram_stat_t *stat)
{
    vtss_flag_value_t flag;
    SL_msg_req_t     *req;

    T_D("enter, isid: %d", isid);

    req = SL_msg_req_alloc(SL_MSG_ID_STAT_GET_REQ);
    flag = (1 << isid);
    vtss_flag_maskbits(&SL_ram.stat_flags, ~flag);
    SL_msg_tx(req, isid, 0);
    if (vtss_flag_timed_wait(&SL_ram.stat_flags, flag, VTSS_FLAG_WAITMODE_OR, vtss_current_time() + VTSS_OS_MSEC2TICK(5000)) & flag) {
        SYSLOG_RAM_CRIT_ENTER();
        *stat = SL_ram.stat[isid];
        SYSLOG_RAM_CRIT_EXIT();
    }
    return VTSS_RC_OK;
}

/******************************************************************************/
// SL_msg_rx()
/******************************************************************************/
static BOOL SL_msg_rx(void *contxt, const void *const rx_msg, const size_t len, const vtss_module_id_t modid, const u32 isid)
{
    SL_msg_id_t msg_id = *(SL_msg_id_t *)rx_msg;

    T_D("isid: %d, msg_id: %d(%s), len: %zu", isid, msg_id, SL_msg_id_txt(msg_id), len);
    T_R_HEX((const uchar *)rx_msg, len);
    switch (msg_id) {
    case SL_MSG_ID_ENTRY_GET_REQ: {
        SL_msg_req_t *req;
        SL_msg_rep_t *rep;

        req = (SL_msg_req_t *)rx_msg;
        T_D("ENTRY_GET_REQ, next: %d, id: " VPRIlu, req->data.entry_get.next, req->data.entry_get.id);
        rep = SL_msg_rep_alloc(SL_MSG_ID_ENTRY_GET_REP);
        rep->data.entry_get.found = SL_ram_get(VTSS_ISID_LOCAL,
                                               req->data.entry_get.next,
                                               req->data.entry_get.id,
                                               req->data.entry_get.lvl,
                                               req->data.entry_get.mid,
                                               &rep->data.entry_get.entry,
                                               0);
        SL_msg_tx(rep, isid,
                  sizeof(rep->data.entry_get) - SYSLOG_RAM_MSG_MAX +
                  (rep->data.entry_get.found ?
                   (strlen(rep->data.entry_get.entry.msg) + 1) : 0));
        break;
    }
    case SL_MSG_ID_ENTRY_GET_REP: {
        SL_msg_rep_t *rep;

        rep = (SL_msg_rep_t *)rx_msg;
        T_D("ENTRY_GET_REP, found: %d, id: " VPRIlu, rep->data.entry_get.found, rep->data.entry_get.entry.id);
        SYSLOG_RAM_CRIT_ENTER();
        if (SL_ram.mgmt_reply.entry != NULL) {
            SL_ram.mgmt_reply.found = rep->data.entry_get.found;
            // length of entry is calculated as:  number of received bytes - offset of entry (skip "msg_id" and "found")
            memcpy(SL_ram.mgmt_reply.entry, &rep->data.entry_get.entry, len - offsetof(SL_msg_rep_t, data.entry_get.entry));
            vtss_flag_setbits(&SL_ram.mgmt_reply.flags, 1 << 0);
        }
        SYSLOG_RAM_CRIT_EXIT();
        break;
    }
    case SL_MSG_ID_STAT_GET_REQ: {
        SL_msg_rep_t *rep;

        T_D("STAT_GET_REQ");
        rep = SL_msg_rep_alloc(SL_MSG_ID_STAT_GET_REP);
        SYSLOG_RAM_CRIT_ENTER();
        rep->data.stat_get.stat = SL_ram.stat[VTSS_ISID_LOCAL];
        SYSLOG_RAM_CRIT_EXIT();
        SL_msg_tx(rep, isid, sizeof(rep->data.stat_get));
        break;
    }
    case SL_MSG_ID_STAT_GET_REP: {
        SL_msg_rep_t *rep;

        rep = (SL_msg_rep_t *)rx_msg;
        T_D("STAT_GET_REP");
        SYSLOG_RAM_CRIT_ENTER();
        SL_ram.stat[isid] = rep->data.stat_get.stat;
        SYSLOG_RAM_CRIT_EXIT();
        vtss_flag_setbits(&SL_ram.stat_flags, 1 << isid);
        break;
    }
    case SL_MSG_ID_CLR_REQ: {
        SL_msg_req_t *msg = (SL_msg_req_t *)rx_msg;
        T_D("CLR_REQ");
        syslog_ram_clear(VTSS_ISID_LOCAL, msg->data.entry_clear.lvl);
        break;
    }
    case SL_MSG_ID_CONF_SET_REQ: {
        SL_msg_req_t *msg = (SL_msg_req_t *)rx_msg;

        if (!msg_switch_is_primary()) {
            SYSLOG_GLB_CRIT_ENTER();
            SL_global.conf = msg->data.conf_set.conf;
            SYSLOG_GLB_CRIT_EXIT();
        }
        break;
    }
    default:
        break;
    }
    return TRUE;
}

/******************************************************************************/
// SL_ram_clear()
// Clear RAM system log
/******************************************************************************/
static void SL_ram_clear(vtss_appl_syslog_lvl_t lvl)
{
    SL_ram_entry_t *cur, *prev;

    if (lvl == VTSS_APPL_SYSLOG_LVL_ALL) {
        SL_ram.first = SL_ram.last = NULL;
        memset(SL_ram.stat, 0, sizeof(SL_ram.stat));
    } else {
        /* The syslog poll maybe exists huge messages, we don't want to take much time for moving messages.
           To process delete individual messages, only re-structure the link list point here. */
        for (cur = (SL_ram_entry_t *)SL_ram.first, prev = NULL;
             cur != NULL;) {
            if (cur->lvl != lvl) {
                prev = cur;
                cur = cur->next;
                continue;
            }
            if (cur == (SL_ram_entry_t *)SL_ram.first) {
                if (cur->next) {
                    SL_ram.first = SL_ram.first->next;
                } else {
                    SL_ram.first = SL_ram.last = NULL;
                    break;
                }
            } else if (cur == SL_ram.last) {
                if (prev) {
                    prev->next = NULL;
                }
                SL_ram.last = prev;
                break;
            } else if (cur->next) {
                if (prev) {
                    prev->next = cur->next;
                }
                cur = cur->next;
            }
        }
        SL_ram.stat[VTSS_ISID_LOCAL].count[lvl] = 0;
    }
}

/******************************************************************************/
// SL_ram_init()
// Initialize RAM system log
/******************************************************************************/
static void SL_ram_init(BOOL init)
{
    msg_rx_filter_t filter;

    if (init) {
        SL_ram_clear(VTSS_APPL_SYSLOG_LVL_ALL);
        critd_init(&SL_ram_crit, "syslog.ram", VTSS_MODULE_ID_SYSLOG, CRITD_TYPE_MUTEX);
        SL_ram.request = msg_buf_pool_create(VTSS_MODULE_ID_SYSLOG, "Request", 2, sizeof(SL_msg_req_t));
        SL_ram.reply   = msg_buf_pool_create(VTSS_MODULE_ID_SYSLOG, "Reply",   2, sizeof(SL_msg_rep_t));
        vtss_flag_init(&SL_ram.mgmt_reply.flags);
        vtss_sem_init(&SL_ram.mgmt_reply.sem, 1);
        SL_ram.mgmt_reply.entry = NULL;
        vtss_flag_init(&SL_ram.stat_flags);
    } else {
        /* Register for stack messages */
        memset(&filter, 0, sizeof(filter));
        filter.cb = SL_msg_rx;
        filter.modid = VTSS_MODULE_ID_SYSLOG;
        (void)msg_rx_filter_register(&filter);
    }
}

/******************************************************************************/
// SL_conf_changed()
// Determine if syslog configuration has changed
/******************************************************************************/
static int SL_conf_changed(const vtss_appl_syslog_server_conf_t *const old, const vtss_appl_syslog_server_conf_t *const new_)
{
    return (memcmp(new_, old, sizeof(*new_)));
}

/******************************************************************************/
// SL_default_set()
// Set syslog defaults.
/******************************************************************************/
static void SL_default_set(vtss_appl_syslog_server_conf_t *conf)
{
    memset(conf, 0, sizeof(*conf)); // assure that also padding bytes are initialized
    conf->server_mode = SYSLOG_MGMT_DEFAULT_MODE;
    memset(&conf->syslog_server, 0, sizeof(conf->syslog_server));
    conf->udp_port = SYSLOG_MGMT_DEFAULT_UDP_PORT;
    conf->syslog_level = SYSLOG_MGMT_DEFAULT_SYSLOG_LVL;
}

/******************************************************************************/
// SL_stack_conf_set()
// Set stack SYSLOG configuration
/******************************************************************************/
static void SL_stack_conf_set(vtss_isid_t isid_add)
{
    SL_msg_req_t    *msg;
    vtss_isid_t     isid;

    T_D("enter, isid_add: %d", isid_add);

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if ((isid_add != VTSS_ISID_GLOBAL && isid_add != isid) || (!msg_switch_exists(isid))) {
            continue;
        }
        msg = SL_msg_req_alloc(SL_MSG_ID_CONF_SET_REQ);
        SYSLOG_GLB_CRIT_ENTER();
        msg->data.conf_set.conf = SL_global.conf;
        SL_global.send_msg_id[isid] = 0;
        SYSLOG_GLB_CRIT_EXIT();
        SL_msg_tx(msg, isid, sizeof(msg->data.conf_set.conf));
    }

    T_D("exit, isid_add: %d", isid_add);
}

/******************************************************************************/
// SL_thread()
/******************************************************************************/
static void SL_thread(vtss_addrword_t data)
{
    int                     sock;
    struct sockaddr_in      server_addr;
    static char             send_data[SYSLOG_RAM_MSG_MAX + 200];
    vtss_isid_t             isid;
    syslog_ram_stat_t       stat;
    syslog_ram_entry_t      entry;
    vtss_appl_syslog_lvl_t  level;
    BOOL                    server_mode, link_down_first_round = TRUE;
    time_t                  current_time;
    vtss_domain_name_t      tmp;

#define SRC_IP_BUF_SIZE 41
    struct sockaddr_in      my_addr;
    socklen_t               my_addr_len;
    char                    my_addr_buf[SRC_IP_BUF_SIZE];

    msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_SYSLOG);

    SYSLOG_GLB_CRIT_ENTER();
    SL_global.current_time = 0;
    SYSLOG_GLB_CRIT_EXIT();

    /* Wait for SNTP/NTP process */
    VTSS_OS_MSLEEP(NTP_DELAY_SEC * 1000);

    while (1) {
        /* Process the task every 2 seconds */
        VTSS_OS_MSLEEP(SL_THREAD_DELAY_SEC * 1000);
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_SYSLOG);

        SYSLOG_GLB_CRIT_ENTER();
        tmp.name[0] = '\0';
        if (SL_global.conf.syslog_server.type == VTSS_INET_ADDRESS_TYPE_IPV4) {
            (void)misc_ipv4_txt(SL_global.conf.syslog_server.address.ipv4, tmp.name);
        } else if (SL_global.conf.syslog_server.type == VTSS_INET_ADDRESS_TYPE_IPV6) {
            (void)misc_ipv6_txt(&SL_global.conf.syslog_server.address.ipv6, tmp.name);
        } else if (SL_global.conf.syslog_server.type == VTSS_INET_ADDRESS_TYPE_DNS) {
            strcpy(tmp.name, SL_global.conf.syslog_server.address.domain_name.name);
        }
        server_mode = SL_global.conf.server_mode && tmp.name[0] != '\0';
        current_time = SL_global.current_time;
        SYSLOG_GLB_CRIT_EXIT();

        /* Do nothing when server mode is disabled or syslog server isn't configured */
        if (!server_mode) {
            continue;
        }

        SYSLOG_GLB_CRIT_ENTER();
        level = SL_global.conf.syslog_level;

        /* Fill server address information */
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SL_global.conf.udp_port);

        /* Look up the DNS host name or IPv4 address */
        if ((server_addr.sin_addr.s_addr = inet_addr(tmp.name)) == 0xFFFFFFFF) {
            int rc;
            char errcode[64] = {0};
            if ( (rc = vtss_getaddrinfo(tmp.name, &server_addr, AI_ALL, server_addr.sin_family, errcode)) != VTSS_RC_OK ) {
                T_D("getaddrinfo() fail: %s [%s]\n", tmp.name, errcode);
            }
        }

        SYSLOG_GLB_CRIT_EXIT();
        if (server_addr.sin_addr.s_addr == 0xFFFFFFFF || server_addr.sin_addr.s_addr == 0) {
            continue;
        }

        /* Create socket */
        if ((sock = vtss_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
            T_D("Create syslog socket failed: %s", strerror(errno));
            continue;
        }

        /* Connect socket */
        if (connect(sock, (struct sockaddr *)&server_addr,
                    sizeof(struct sockaddr)) != 0) {
            close(sock);
            T_D("Connect syslog socket failed: %s", strerror(errno));
            continue;
        }

        /* Get my address */
        my_addr_len = sizeof(my_addr);
        if (getsockname(sock, (struct sockaddr *)&my_addr,
                        &my_addr_len) != 0) {
            close(sock);
            T_D("Get syslog my address sockname failed: %s", strerror(errno));
            continue;
        }

        /* Convert to my address string */
        my_addr_buf[0] = '\0';
        (void) inet_ntop(my_addr.sin_family,
                         (const char *)&my_addr.sin_addr,
                         my_addr_buf, SRC_IP_BUF_SIZE);

        /* Get syslog messages */
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            if (!msg_switch_exists(isid) || syslog_ram_stat_get(isid, &stat) != VTSS_RC_OK) {
                continue;
            }

            SYSLOG_GLB_CRIT_ENTER();
            entry.id = SL_global.send_msg_id[isid];
            SYSLOG_GLB_CRIT_EXIT();

            while (syslog_ram_get(isid, TRUE, entry.id, VTSS_APPL_SYSLOG_LVL_ALL, VTSS_MODULE_ID_NONE, &entry)) {
                if (entry.lvl <= level && entry.time >= current_time) {
                    ssize_t     msg_size;
                    ssize_t     send_rc;
                    u8          pri;
                    char        buf2[32];
                    const char  *link_down_str = "Link down";

                    /* To avoid missing the link-down log, we will send it on the second round */
                    if (strncmp(entry.msg, link_down_str, strlen(link_down_str)) == 0) {
                        if (link_down_first_round) {
                            link_down_first_round = FALSE;
                            break;
                        }
                        link_down_first_round = TRUE;
                    }

                    /* Calculate PRI field */
                    pri = SYSLOG_FACILITY_LOCAL7 << 3;
                    pri += entry.lvl;

                    sprintf(buf2, "%s", VTSS_PRODUCT_NAME);

                    /* Fill syslog packet contents: HEADER + STRUCTURED-DATA
                       HEADER: <PRI> VERSION TIMESTAMP HOSTNAME APP-NAME RPOCID MSGID
                       STRUCTURED-DATA: [SD-ELEMENT SD-ID SD-PARM]

                       Notice:
                       HEADER: RPOCID and TRUCTURED-DATA: SD-ID aren't used in our system currently.

                       Example:
                       <14>1 2011-01-14T14:24:00Z+00:00 10.9.52.169 vtss_syslog - ID1
                       [SMBStaX usid="1"]
                       Switch just made a cold boot.
                     */
                    sprintf(send_data, "<%d>1 %s %s syslog - ID" VPRIlu" [%s] %s",
                            pri,
                            misc_time2str(entry.time),
                            my_addr_buf,
                            entry.id,
                            buf2,
                            entry.msg);

                    /* Send out packet to syslog server */
                    msg_size = strlen(send_data);
                    send_rc = send(sock, send_data, msg_size, 0);

                    if (send_rc < 0) {
                        T_D("Send syslog socket failed: %s", strerror(errno));
                        break;  // try it on the next cycle.
                    } else if (send_rc != msg_size) {
                        T_D("Send syslog socket failed: sendout length is short - sendout length = " VPRIsz", expected length = " VPRIsz, send_rc, msg_size);
                        break;  // try it on the next cycle.
                    }
                }

                /* Update send_msg_id */
                SYSLOG_GLB_CRIT_ENTER();
                SL_global.send_msg_id[isid] = entry.id;
                SYSLOG_GLB_CRIT_EXIT();
            }
        }

        /* Close socket */
        close(sock);
    }
}

/******************************************************************************/
// SL_conf_read_stack()
// Read/create SYSLOG stack configuration
/******************************************************************************/
static void SL_conf_read_stack(BOOL create)
{
    int                     changed = FALSE;
    vtss_appl_syslog_server_conf_t *old_syslog_conf_p, new_syslog_conf;

    T_D("enter, create: %d", create);

    changed = 0;
    SYSLOG_GLB_CRIT_ENTER();
    /* Use default values */
    SL_default_set(&new_syslog_conf);
#ifdef VTSS_SW_OPTION_JSON_RPC_NOTIFICATION
    SYNCHRONIZED(syslog_notif_conf) {
        syslog_notif_conf.the_syslog_notif_conf.erase(
            syslog_notif_conf.the_syslog_notif_conf.begin(),
            syslog_notif_conf.the_syslog_notif_conf.end());
    }
#endif

    old_syslog_conf_p = &SL_global.conf;
    changed = SL_conf_changed(old_syslog_conf_p, &new_syslog_conf);
    SL_global.conf = new_syslog_conf;
    if (changed && SL_global.conf.server_mode) {
        /* Update current timer */
        SL_global.current_time = SL_get_time_in_secs();
    }
    SYSLOG_GLB_CRIT_EXIT();

    if (changed && create) {
        SL_stack_conf_set(VTSS_ISID_GLOBAL);
    }

    T_D("exit");
}

/******************************************************************************/
// SL_ram_start()
// Module start
/******************************************************************************/
static void SL_ram_start(void)
{
    vtss_appl_syslog_server_conf_t *conf_p;

    T_D("enter");

    /* Initialize SYSLOG configuration */
    conf_p = &SL_global.conf;
    SL_default_set(conf_p);

    /* Initialize msg_id */
    memset(SL_global.send_msg_id, 0, sizeof(SL_global.send_msg_id));

    /* Create semaphore for critical regions */
    critd_init(&SL_global.crit, "syslog.global", VTSS_MODULE_ID_SYSLOG, CRITD_TYPE_MUTEX);

    /* Create SYSLOG thread */
    vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                       SL_thread,
                       0,
                       "syslog",
                       nullptr,
                       0,
                       &SYSLOG_thread_handle,
                       &SYSLOG_thread_block);

    T_D("exit");
}

/******************************************************************************/
// syslog_lvl_to_string()
/******************************************************************************/
const char *syslog_lvl_to_string(vtss_appl_syslog_lvl_t lvl, BOOL lowercase)
{
    if (lowercase) {
        switch (lvl) {
        case VTSS_APPL_SYSLOG_LVL_ERROR:
            return "error";

        case VTSS_APPL_SYSLOG_LVL_WARNING:
            return "warning";

        case VTSS_APPL_SYSLOG_LVL_NOTICE:
            return "notice";

        case VTSS_APPL_SYSLOG_LVL_INFO:
            return "informational";

        case VTSS_APPL_SYSLOG_LVL_ALL:
            return "all";

        default:
            return "unknown";
        }
    } else {
        switch (lvl) {
        case VTSS_APPL_SYSLOG_LVL_ERROR:
            return "Error";

        case VTSS_APPL_SYSLOG_LVL_WARNING:
            return "Warning";

        case VTSS_APPL_SYSLOG_LVL_NOTICE:
            return "Notice";

        case VTSS_APPL_SYSLOG_LVL_INFO:
            return "Informational";

        case VTSS_APPL_SYSLOG_LVL_ALL:
            return "All";

        default:
            return "Unknown";
        }
    }
}

/******************************************************************************/
// syslog_ram_log()
/******************************************************************************/
void syslog_ram_log(vtss_appl_syslog_lvl_t lvl, vtss_module_id_t mid, vtss_isid_t isid, mesa_port_no_t iport, const char *fmt, ...)
{
    va_list        args;
    int            n;
    SL_ram_entry_t *new_;
    BOOL           buf_full = FALSE;
    char           *temp_msg = NULL;
    size_t         temp_msg_len = 0, ram_entry_header_len = sizeof(SL_ram_entry_t) - SYSLOG_RAM_MSG_MAX;
    SYSLOG_RAM_CRIT_ENTER();

    if (SYSLOG_init == FALSE) {
        SYSLOG_RAM_CRIT_EXIT();
        return;
    }

    /* Add entry to list */
    if (SL_ram.last == NULL) {
        /* Insert entry first in list */
        new_ = (SL_ram_entry_t *)&SL_ram.log[0];
        SL_ram.first = SL_ram.last = new_;
        if (SL_ram.current_id == SYSLOG_RAM_MSG_ID_MAX) { // syslog ID wrap-around
            new_->id = SL_ram.current_id = 1;
        } else {
            new_->id = ++SL_ram.current_id;
        }
    } else {
        /* Next entry is on 4-byte aligned address, need to consider one byye of '\0' */
        n = (strlen(SL_ram.last->msg) + 3 + 1 + ram_entry_header_len);
        new_ = (SL_ram_entry_t *)((uchar *)SL_ram.last + n - (n & 3));

        /* Check if log is full */
        if ((&SL_ram.log[SYSLOG_RAM_SIZE] - (uchar *)new_) < (int)sizeof(SL_ram_entry_t)) {
            new_ = (SL_ram_entry_t *)&SL_ram.log[0];
            buf_full = TRUE;
        }

        /* Move 'SL_ram.first' flag for saving the new_ message */
        if (buf_full || ((int)((uchar *)SL_ram.first - (uchar *)SL_ram.last) > 0)) {
            VTSS_MALLOC_CAST(temp_msg, SYSLOG_RAM_MSG_MAX);
            if (temp_msg) {
                temp_msg[0] = '\0';
                va_start(args, fmt);
                (void)vsnprintf(temp_msg + strlen(temp_msg), SYSLOG_RAM_MSG_MAX - strlen(temp_msg), fmt, args);
                va_end(args);

                /* Use 4-byte aligned length, need to consider one byye of '\0' */
                temp_msg_len = (strlen(temp_msg) + 3 + 1 + ram_entry_header_len);
                temp_msg_len = temp_msg_len - (temp_msg_len & 3);
            } else {
                SYSLOG_RAM_CRIT_EXIT();
                return;
            }
            while (SL_ram.first && ((int)((uchar *)new_ + temp_msg_len - (uchar *)SL_ram.first) > 0)) {
                if (SL_ram.stat[VTSS_ISID_LOCAL].count[SL_ram.first->lvl]) {
                    SL_ram.stat[VTSS_ISID_LOCAL].count[SL_ram.first->lvl]--;
                }
                if (SL_ram.first->next && (int)((uchar *)SL_ram.first - (uchar *)SL_ram.first->next) > 0) { // first flag wrap-around
                    SL_ram.first = SL_ram.first->next;
                    break;
                }
                SL_ram.first = SL_ram.first->next;
            }
        }

        if (SL_ram.current_id == SYSLOG_RAM_MSG_ID_MAX) { // syslog ID wrap-around
            new_->id = SL_ram.current_id = 1;
            while (SL_ram.first && SL_ram.first->id <= new_->id) {
                if (SL_ram.stat[VTSS_ISID_LOCAL].count[SL_ram.first->lvl]) {
                    SL_ram.stat[VTSS_ISID_LOCAL].count[SL_ram.first->lvl]--;
                }
                SL_ram.first = SL_ram.first->next;
            }
        } else {
            new_->id = ++SL_ram.current_id;
        }

        SL_ram.last->next = new_;
        if (SL_ram.first == NULL) {
            SL_ram.first = new_;
        }
    }

    /* Store entry data */
    SL_ram.stat[VTSS_ISID_LOCAL].count[lvl]++;
    SL_ram.last = new_;
    new_->next = NULL;
    new_->lvl = lvl;
    new_->mid = mid;
    new_->time = msg_uptime_get(VTSS_ISID_LOCAL);
    if (temp_msg) {
        strcpy(new_->msg, temp_msg);
        VTSS_FREE(temp_msg);
    } else {
        new_->msg[0] = '\0';
        va_start(args, fmt);
        (void)vsnprintf(new_->msg + strlen(new_->msg), SYSLOG_RAM_MSG_MAX - strlen(new_->msg), fmt, args);
        va_end(args);
    }

#if defined(SYSLOG_RAM_MSG_ENTRY_CNT_MAX)
    /* Limit the max log entry number */
    {
        ulong total_count = 0;
        for (n = 0; n < VTSS_APPL_SYSLOG_LVL_ALL; n++) {
            total_count += SL_ram.stat[VTSS_ISID_LOCAL].count[n];
        }
        while (total_count > SYSLOG_RAM_MSG_ENTRY_CNT_MAX) {
            SL_ram.stat[VTSS_ISID_LOCAL].count[SL_ram.first->lvl]--;
            SL_ram.first = SL_ram.first->next;
            total_count--;
        }
    }
#endif /* SYSLOG_RAM_MSG_CNT_MAX */

    if (isid != VTSS_ISID_END) {
        SL_port_info_insert(isid, iport, new_->msg);
    }
    SYSLOG_RAM_CRIT_EXIT();

    // Light system LED RED while error level
    if (lvl == VTSS_APPL_SYSLOG_LVL_ERROR) {
        led_front_led_state(LED_FRONT_LED_ERROR, FALSE);
    }
}

/******************************************************************************/
// syslog_ram_clear()
// Clear RAM system log
/******************************************************************************/
void syslog_ram_clear(vtss_isid_t isid, vtss_appl_syslog_lvl_t lvl)
{
    SL_msg_req_t *req;

    if (isid == VTSS_ISID_LOCAL) {
        SYSLOG_RAM_CRIT_ENTER();
        SL_ram_clear(lvl);
        SYSLOG_RAM_CRIT_EXIT();

        if (lvl == VTSS_APPL_SYSLOG_LVL_ALL || lvl <= VTSS_APPL_SYSLOG_LVL_ERROR) {
            // Cler generic software error of system LED state and back to previous state
            led_front_led_state_clear(LED_FRONT_LED_ERROR);
        }
    } else {
        req = SL_msg_req_alloc(SL_MSG_ID_CLR_REQ);
        req->data.entry_clear.lvl = lvl;
        SL_msg_tx(req, isid, sizeof(req->data.entry_clear));
        VTSS_OS_MSLEEP(100); // Delay for sending the clear message
    }

    SYSLOG_GLB_CRIT_ENTER();
    SL_global.send_msg_id[isid] = 0;
    SYSLOG_GLB_CRIT_EXIT();
}

/******************************************************************************/
// syslog_ram_get()
// Get RAM system log entry.
// Note: The newest entry can overwrite the oldest when buffer runs full.
/******************************************************************************/
BOOL syslog_ram_get(vtss_isid_t             isid,    /* ISID */
                    BOOL                    next,    /* Next or specific entry */
                    ulong                   id,      /* Entry ID */
                    vtss_appl_syslog_lvl_t  lvl,     /* VTSS_APPL_SYSLOG_LVL_ALL is wildcard */
                    vtss_module_id_t        mid,     /* VTSS_MODULE_ID_NONE is wildcard */
                    syslog_ram_entry_t      *entry)  /* Returned data */
{
    BOOL found = FALSE;

    if (isid < VTSS_ISID_END &&
        (found = SL_ram_get(isid, next, id, lvl, mid, entry, 1)) == TRUE) {
        SL_port_info_replace(isid, entry->msg);
    }

    return found;
}

/******************************************************************************/
// syslog_ram_stat_get()
// Get RAM system log statistics */
/******************************************************************************/
mesa_rc syslog_ram_stat_get(vtss_isid_t isid, syslog_ram_stat_t *stat)
{
    mesa_rc rc = VTSS_RC_OK;

    if (isid == VTSS_ISID_LOCAL) {
        SYSLOG_RAM_CRIT_ENTER();
        *stat = SL_ram.stat[VTSS_ISID_LOCAL];
        SYSLOG_RAM_CRIT_EXIT();
    } else {
        rc = SL_ram_stat_get(isid, stat);
    }
    return rc;
}

/******************************************************************************/
// syslog_lvl_to_string()
/******************************************************************************/
vtss_appl_syslog_lvl_t vtss_appl_syslog_lvl(u32 l)
{
    return static_cast<vtss_appl_syslog_lvl_t>(l);
}

/******************************************************************************/
// syslog_error_txt()
/******************************************************************************/
const char *syslog_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_SYSLOG_ERROR_MUST_BE_PRIMARY_SWITCH:
        return "Operation only valid on the primary switch";

    case VTSS_APPL_SYSLOG_ERROR_ISID:
        return "Invalid Switch ID";

    case VTSS_APPL_SYSLOG_ERROR_INV_PARAM:
        return "Invalid parameter supplied to function";

    case VTSS_APPL_SYSLOG_ERROR_LOG_ENTRY_NOT_EXIST:
        return "Syslog entry isn't existing";

    default:
        return "SYSLOG: Unknown error code";
    }
}

/******************************************************************************/
// vtss_appl_syslog_server_conf_get()
//
// Get syslog server configuration. It is a global configuration. The primary
// switch will automatically apply the same configuration to all secondary
// switches.
/******************************************************************************/
mesa_rc vtss_appl_syslog_server_conf_get(vtss_appl_syslog_server_conf_t *const server_conf)
{
    T_D("enter");

    if (server_conf == NULL) {
        T_E("Input parameter is NULL");
        T_D("exit");
        return VTSS_APPL_SYSLOG_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return VTSS_APPL_SYSLOG_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    SYSLOG_GLB_CRIT_ENTER();
    *server_conf = SL_global.conf;
    SYSLOG_GLB_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_syslog_server_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_syslog_server_conf_set(const vtss_appl_syslog_server_conf_t *const server_conf)
{
    mesa_rc rc = VTSS_RC_OK;
    int     changed = 0;

    T_D("enter, server_mode: %d", server_conf->server_mode);

    if (server_conf == NULL) {
        T_E("Input parameter is NULL");
        T_D("exit");
        return VTSS_APPL_SYSLOG_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_E("not primary switch");
        T_D("exit");
        return VTSS_APPL_SYSLOG_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    /* Check illegal parameter */
    if (server_conf->syslog_level < VTSS_APPL_SYSLOG_LVL_START ||
        server_conf->syslog_level >= VTSS_APPL_SYSLOG_LVL_ALL) {
        T_D("exit, invalid parameters: syslog_level");
        return VTSS_APPL_SYSLOG_ERROR_INV_PARAM;
    }
    if (server_conf->syslog_server.type == VTSS_INET_ADDRESS_TYPE_IPV4 &&
        (server_conf->syslog_server.address.ipv4 == 0x0 ||
         server_conf->syslog_server.address.ipv4 == 0x7F000001 ||
         server_conf->syslog_server.address.ipv4 >= 0xE0000000)) {
        T_D("exit, IPv4 address 0.0.0.0 or 127.0.0.1 or multicast");
        return VTSS_APPL_SYSLOG_ERROR_INV_PARAM;
    }
    if (server_conf->syslog_server.type == VTSS_INET_ADDRESS_TYPE_IPV6) {
        T_D("exit, IPv6 address type not supported");
        return VTSS_APPL_SYSLOG_ERROR_INV_PARAM;
    }
#if defined(VTSS_SW_OPTION_DNS)
    if (server_conf->syslog_server.type == VTSS_INET_ADDRESS_TYPE_DNS &&
        misc_str_is_domainname(server_conf->syslog_server.address.domain_name.name) != VTSS_RC_OK) {
        T_D("exit, not a valod domain name");
        return VTSS_APPL_SYSLOG_ERROR_INV_PARAM;
    }
#else
    if (server_conf->syslog_server.type == VTSS_INET_ADDRESS_TYPE_DNS) {
        T_D("exit, DNS address type not supported");
        return VTSS_APPL_SYSLOG_ERROR_INV_PARAM;
    }
#endif /* VTSS_SW_OPTION_DNS */

    SYSLOG_GLB_CRIT_ENTER();
    changed = SL_conf_changed(&SL_global.conf, server_conf);
    SL_global.conf = *server_conf;
    if (changed && SL_global.conf.server_mode) {
        /* Update current timer */
        SL_global.current_time = SL_get_time_in_secs() > (NTP_DELAY_SEC + SL_THREAD_DELAY_SEC) ? SL_get_time_in_secs() : 0;
    }
    SYSLOG_GLB_CRIT_EXIT();

    if (changed) {
        /* Activate changed configuration */
        SL_stack_conf_set(VTSS_ISID_GLOBAL);
    }

    T_D("exit");
    return rc;
}

/******************************************************************************/
// syslog_id_itr()
/******************************************************************************/
mesa_rc syslog_id_itr(const u32   *const prev_syslog_idx, u32 *const next_syslog_idx, vtss_usid_t usid)
{
    u32                 syslog_id = prev_syslog_idx ? *prev_syslog_idx : 0;
    syslog_ram_entry_t  syslog_entry;

    T_D("enter: prev_syslog_idx = %d, next_syslog_idx = %d",
        prev_syslog_idx ? *prev_syslog_idx : 0, next_syslog_idx ? *next_syslog_idx : 0);

    /* Check illegal parameter */
    if (next_syslog_idx == NULL) {
        T_E("Input parameter is NULL");
        return VTSS_APPL_SYSLOG_ERROR_INV_PARAM;
    }

    if (syslog_id > SYSLOG_RAM_MSG_ID_MAX) {
        T_D("exit: Switch ID out of range");
        return VTSS_APPL_SYSLOG_ERROR_LOG_ENTRY_NOT_EXIST;
    }

    if (syslog_ram_get(topo_usid2isid(usid), TRUE, syslog_id, VTSS_APPL_SYSLOG_LVL_ALL, VTSS_MODULE_ID_NONE, &syslog_entry)) {
        *next_syslog_idx = syslog_entry.id;
    } else {
        T_D("exit: Cannot found next valid syslog ID");
        return VTSS_APPL_SYSLOG_ERROR_LOG_ENTRY_NOT_EXIST;
    }

    T_D("exit: next_syslog_idx = %d", next_syslog_idx ? *next_syslog_idx : 0);

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_syslog_history_get()
/******************************************************************************/
mesa_rc vtss_appl_syslog_history_get(vtss_usid_t usid, u32 syslog_id, vtss_appl_syslog_history_t *const history)
{
    mesa_rc     rc = VTSS_APPL_SYSLOG_ERROR_INV_PARAM;
    vtss_isid_t isid = VTSS_ISID_START;
    syslog_ram_entry_t syslog_entry;

    /* Check illegal parameters */
    if (usid != VTSS_USID_START) {
        T_D("exit: Invalid USID = %d", usid);
        return rc;
    }
    if (history == NULL) {
        T_E("Input parameter is NULL");
        return VTSS_APPL_SYSLOG_ERROR_INV_PARAM;
    }

    if (syslog_ram_get(isid, FALSE, syslog_id, VTSS_APPL_SYSLOG_LVL_ALL, VTSS_MODULE_ID_NONE, &syslog_entry)) {
        memset(history, 0, sizeof(*history));
        history->lvl = syslog_entry.lvl;
        strncpy(history->msg, syslog_entry.msg, VTSS_APPL_SYSLOG_MIB_MSG_TEXT_LEN_MAX);
        history->time = msg_abstime_get(isid, syslog_entry.time);
        if (strlen(syslog_entry.msg) > (VTSS_APPL_SYSLOG_MIB_MSG_TEXT_LEN_MAX - 3)) {
            history->msg[VTSS_APPL_SYSLOG_MIB_MSG_TEXT_LEN_MAX - 3] = '\0';
            strcat(history->msg, "...");
        }

        return VTSS_RC_OK;
    }

    return VTSS_APPL_SYSLOG_ERROR_LOG_ENTRY_NOT_EXIST;
}

/******************************************************************************/
// vtss_appl_syslog_history_control_get()
/******************************************************************************/
mesa_rc vtss_appl_syslog_history_control_get(vtss_usid_t usid, vtss_appl_syslog_lvl_t lvl, vtss_appl_syslog_history_control_t  *const control)
{
    mesa_rc rc = VTSS_APPL_SYSLOG_ERROR_INV_PARAM;

    T_D("enter: usid = %d, lvl = %d,  control->clear_syslog = %s", usid, lvl, control->clear_syslog ? "T" : "F");

    /* Check illegal parameter */
    if (usid >= VTSS_USID_END) {
        T_D("exit: Invalid USID = %d", usid);
        return rc;
    }

    if (control == NULL) {
        T_E("Input parameter is NULL");
        return rc;
    }
    if (lvl > VTSS_APPL_SYSLOG_LVL_END && lvl != VTSS_APPL_SYSLOG_LVL_ALL) {
        T_E("Input parameter is NULL");
        return rc;
    }

    memset(control, 0, sizeof(*control));

    T_D("exit");
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_syslog_history_control_set()
/******************************************************************************/
mesa_rc vtss_appl_syslog_history_control_set(vtss_usid_t usid, vtss_appl_syslog_lvl_t lvl, const vtss_appl_syslog_history_control_t *const control)
{
    mesa_rc     rc = VTSS_APPL_SYSLOG_ERROR_INV_PARAM;
    vtss_isid_t isid = VTSS_ISID_START;

    T_D("enter: usid = %d, lvl = %d,  control->clear_syslog = %s", usid, lvl, control->clear_syslog ? "T" : "F");

    /* Check illegal parameter */
    if (usid >= VTSS_USID_END) {
        T_D("exit: Invalid USID = %d", usid);
        return rc;
    }

    if (control == NULL) {
        T_E("Input parameter is NULL");
        return rc;
    }
    if (lvl > VTSS_APPL_SYSLOG_LVL_END && lvl != VTSS_APPL_SYSLOG_LVL_ALL) {
        T_E("Input parameter is NULL");
        return rc;
    }

    if (control->clear_syslog == TRUE) {
        if (usid == VTSS_APPL_SYSLOG_ALL_SWITCHES) {
            vtss_isid_t isid_idx;
            for (isid_idx = VTSS_ISID_START; isid_idx < VTSS_ISID_END; isid_idx++) {
                if (!msg_switch_exists(isid_idx)) {
                    continue;
                }
                syslog_ram_clear(isid_idx, lvl);
            }
        } else if (msg_switch_exists(isid)) {
            syslog_ram_clear(isid, lvl);
        }
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_syslog_history_get_all_by_step()
/******************************************************************************/
mesa_rc vtss_appl_syslog_history_get_all_by_step(vtss_isid_t *isid, uint32_t *syslog_id, void **ptr, vtss_appl_syslog_history_t *history)
{
    SL_ram_entry_t *cur, *tmp;

    if (!msg_switch_is_primary() || !msg_switch_is_local(*isid)) {
        /* TO_DO : We don't consider stacking case now */
    }

    // Check NULL point
    if (*ptr == NULL && SL_ram.first == NULL) {
        return VTSS_RC_ERROR;
    }

    /*
        Check ptr is null or not
        1. YES, we have to find first entry
        2. NO,  get now entry data and return next entry
    */
    cur = (SL_ram_entry_t *)*ptr;
    if (cur == NULL) {
        SYSLOG_RAM_CRIT_ENTER();
        cur = SL_ram.first;
    }

    if (cur != NULL) {
        memset(history, 0, sizeof(vtss_appl_syslog_history_t));
        history->lvl = cur->lvl;
        strncpy(history->msg, cur->msg, VTSS_APPL_SYSLOG_MIB_MSG_TEXT_LEN_MAX);
        SL_port_info_replace(*isid, history->msg);
        history->time = msg_abstime_get(*isid, cur->time);
        if (strlen(cur->msg) > (VTSS_APPL_SYSLOG_MIB_MSG_TEXT_LEN_MAX - 3)) {
            history->msg[VTSS_APPL_SYSLOG_MIB_MSG_TEXT_LEN_MAX - 3] = '\0';
            strcat(history->msg, "...");
        }
        /* return syslog id */
        *syslog_id = cur->id;
        /* record the next entry address, next time we can continue to get next via this record */
        tmp = cur->next;
        *ptr = (void *)tmp;
    }

    /* We finally search all the entries */
    if (*ptr == NULL) {
        SYSLOG_RAM_CRIT_EXIT();
    }

    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_JSON_RPC_NOTIFICATION
/******************************************************************************/
// vtss_appl_syslog_notif_itr()
/******************************************************************************/
mesa_rc vtss_appl_syslog_notif_itr(const vtss_appl_syslog_notif_name_t *const in, vtss_appl_syslog_notif_name_t *const out)
{
    if (!out) {
        return VTSS_RC_ERROR;
    }

    std::string iter("");
    const std::string *p_iter = &iter;
    if (in) {
        iter = in->notif_name;
    }

    SYNCHRONIZED(syslog_notif_conf) {
        auto p = syslog_notif_conf.the_syslog_notif_conf.greater_than(*p_iter);
        if (p == syslog_notif_conf.the_syslog_notif_conf.end()) {
            return VTSS_RC_ERROR;
        }
        strncpy(out->notif_name, p->first.c_str(), VTSS_APPL_SYSLOG_ENTRY_NAME_SIZE - 1);
    }
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_syslog_notif_get()
/******************************************************************************/
mesa_rc vtss_appl_syslog_notif_get(const vtss_appl_syslog_notif_name_t *const nm,
                                   vtss_appl_syslog_notif_conf_t *conf)
{
    if (!nm) {
        return VTSS_RC_ERROR;
    }
    std::string iter(nm->notif_name);
    SYNCHRONIZED(syslog_notif_conf) {
        auto p_elem = syslog_notif_conf.the_syslog_notif_conf.find(iter);
        if (p_elem == syslog_notif_conf.the_syslog_notif_conf.end()) {
            return VTSS_RC_ERROR_SYSLOG_NOT_FOUND;
        }
        if (conf) {
            *conf = p_elem->second;
            strncpy(conf->notification,
                    p_elem->second.notification, ALARM_NAME_SIZE);
            conf->level = p_elem->second.level;
        }
    }
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_syslog_notif_add()
/******************************************************************************/
mesa_rc vtss_appl_syslog_notif_add(const vtss_appl_syslog_notif_name_t *const nm,
                                   const vtss_appl_syslog_notif_conf_t *const conf)
{
    // TODO, the alarm is not created
    if (!nm || !conf) {
        return VTSS_RC_ERROR;
    }

    if (vtss_appl_syslog_notif_get(nm, nullptr) == VTSS_RC_OK) {
        return VTSS_RC_ERROR_SYSLOG_ALREADY_DEFINED;
    }

    std::string iter(nm->notif_name);

    std::string node_name(conf->notification);
    expose::json::Node *node = JSON_RPC_ROOT.lookup(node_name);

    if (node == nullptr) {
        T_D("Null pointer met: node: %s", node_name.c_str());
        return VTSS_RC_ERROR_SYSLOG_NO_SUCH_NOTIFICATION_SOURCE;
    }

    expose::json::Node *update = node->lookup(str("update"));
    if (update == nullptr || !update->is_notification()) {
        T_D("No update found: %s", node_name.c_str());
        return VTSS_RC_ERROR_SYSLOG_NO_SUCH_NOTIFICATION_SOURCE;
    }

    mesa_rc rc;
    SYNCHRONIZED(syslog_notif_conf) {
        auto res = syslog_notif_conf.the_syslog_notif_conf.emplace(iter, syslog_notif_conf_t(*nm, *conf));
        rc = res.second ? VTSS_RC_OK : VTSS_RC_ERROR;
    }
    return rc;
}

/******************************************************************************/
// vtss_appl_syslog_notif_del()
/******************************************************************************/
mesa_rc vtss_appl_syslog_notif_del(const vtss_appl_syslog_notif_name_t *const nm)
{
    if (!nm) {
        return VTSS_RC_ERROR;
    }

    if (vtss_appl_syslog_notif_get(nm, nullptr) != VTSS_RC_OK) {
        return VTSS_RC_ERROR_SYSLOG_NOT_FOUND;
    }

    bool res;
    std::string iter(nm->notif_name);
    SYNCHRONIZED(syslog_notif_conf) {
        res = syslog_notif_conf.the_syslog_notif_conf.erase(iter);
    }
    return res ? VTSS_RC_OK : VTSS_RC_ERROR;
}

#endif //VTSS_SW_OPTION_JSON_RPC_NOTIFICATION

/******************************************************************************/
// syslog_mgmt_default_get()
/******************************************************************************/
void syslog_mgmt_default_get(vtss_appl_syslog_server_conf_t *glbl_cfg)
{
    SL_default_set(glbl_cfg);
}

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
/* Initialize private mib */
VTSS_PRE_DECLS void syslog_mib_init(void);
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_syslog_json_init(void);
#endif

extern "C" int syslog_icli_cmd_register();

/******************************************************************************/
// syslog_init()
/******************************************************************************/
mesa_rc syslog_init(vtss_init_data_t *data)
{
    mesa_rc rc = VTSS_RC_OK;
    vtss_isid_t isid = data->isid;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
        SL_ram_start();

        (void)syslog_icfg_init();

        /* Initialize RAM log */
        SL_ram_init(1);

#ifdef VTSS_SW_OPTION_JSON_RPC_NOTIFICATION
        /* Initialize synchronized data */
        vtss::appl::syslog::syslog_notif_conf.init(__LINE__, "syslog");
#endif

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
        /* Register private mib */
        syslog_mib_init();
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_syslog_json_init();
#endif
        syslog_icli_cmd_register();
        break;

    case INIT_CMD_START:
        T_D("START");
        /* Initialize RAM log */
        SL_ram_init(0);

        SYSLOG_RAM_CRIT_ENTER();
        SYSLOG_init = TRUE;
        SYSLOG_RAM_CRIT_EXIT();
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            SL_conf_read_stack(1);
        }

        // Cler generic software error of system LED state and back to previous state
        led_front_led_state_clear(LED_FRONT_LED_ERROR);
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");
        /* Read stack and switch configuration */
        SL_conf_read_stack(0);

        /* Update current timer */
        SYSLOG_GLB_CRIT_ENTER();
        SL_global.current_time = SL_get_time_in_secs();
        SYSLOG_GLB_CRIT_EXIT();
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");
        /* Apply all configuration to switch */
        SL_stack_conf_set(isid);
        break;

    default:
        break;
    }

    return rc;
}

