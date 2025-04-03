/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _SYSLOG_API_H_
#define _SYSLOG_API_H_

#include <main.h>
#include "sysutil_api.h"    /* For VTSS_APPL_SYSUTIL_HOSTNAME_LEN */
#include <vtss/appl/syslog.h>
#include <time.h>           /* For time_t                         */

enum {
    VTSS_RC_ERROR_SYSLOG_ALREADY_DEFINED = MODULE_ERROR_START(VTSS_MODULE_ID_SYSLOG),
    VTSS_RC_ERROR_SYSLOG_NOT_FOUND,
    VTSS_RC_ERROR_SYSLOG_NO_SUCH_NOTIFICATION_SOURCE,
};  // Leave it anonymous

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \file syslog_api.h
 * \brief This file defines the APIs for the syslog module
 */

/**
 * Default syslog configuration
 */
#define SYSLOG_MGMT_DEFAULT_MODE        FALSE
#define SYSLOG_MGMT_DEFAULT_PORT_MODE   FALSE
#define SYSLOG_MGMT_DEFAULT_UDP_PORT    514
#define SYSLOG_MGMT_DEFAULT_SYSLOG_LVL  VTSS_APPL_SYSLOG_LVL_INFO

#define SYSLOG_FACILITY_LOCAL7          23  /**< syslog facility: local use 7 (local7), defined by RFC 5424 */
#define SYSLOG_MGMT_DEFAULT_FACILITY    SYSLOG_FACILITY_LOCAL7

/**
 * The start/end level of syslog severity level
 */
#define VTSS_APPL_SYSLOG_LVL_START  VTSS_APPL_SYSLOG_LVL_ERROR
#define VTSS_APPL_SYSLOG_LVL_END    VTSS_APPL_SYSLOG_LVL_INFO

/****************************************************************************/
// These define the category of syslog messages.
// Use bitwise categorizing, so that the user can choose which categories
// to display.
/****************************************************************************/
typedef enum {
    SYSLOG_CAT_DEBUG,  // Used by trace and VTSS_ASSERT()
    SYSLOG_CAT_SYSTEM, // E.g. system re-flashed, or booted, or...
    SYSLOG_CAT_APP,    // E.g. Sprout connectivity lost
    SYSLOG_CAT_ALL,    // Must come last. Don't use when adding messages to the syslog. It must only be used by callers of syslog_flash_print()
} syslog_cat_t;

/* Covert level to string */
const char *syslog_lvl_to_string(vtss_appl_syslog_lvl_t lvl, BOOL lowercase);
vtss_appl_syslog_lvl_t vtss_appl_syslog_lvl(u32 l);

/****************************************************************************/
// syslog_flash_log()
// Write a message to the flash.
/****************************************************************************/
void syslog_flash_log(syslog_cat_t cat, vtss_appl_syslog_lvl_t lvl, const char *msg);

/****************************************************************************/
// syslog_flash_erase()
// Clear the syslog flash. Returns TRUE on success, FALSE on error.
/****************************************************************************/
BOOL syslog_flash_erase(void);

/****************************************************************************/
// syslog_flash_print()
// Print the contents of the syslog flash.
/****************************************************************************/
void syslog_flash_print(syslog_cat_t cat, vtss_appl_syslog_lvl_t lvl, int (*print_function)(const char *fmt, ...));

/****************************************************************************/
// syslog_flash_entry_cnt()
// Returns the number of entries in the syslog's flash.
/****************************************************************************/
int syslog_flash_entry_cnt(syslog_cat_t cat, vtss_appl_syslog_lvl_t lvl);

/*---- RAM System Log ------------------------------------------------------*/

/* Maximum size of one message */
#define SYSLOG_RAM_MSG_MAX          (10*1024)
#define SYSLOG_RAM_MSG_ID_MAX       0x7FFFFFFF

#if SYSLOG_RAM_MSG_MAX < VTSS_APPL_SYSLOG_MIB_MSG_TEXT_LEN_MAX
error SYSLOG_RAM_MSG_MAX MUSt be larger then VTSS_APPL_SYSLOG_MIB_MSG_TEXT_LEN_MAX
#endif
/* define it if we want to limit a max. entry count */
//#define SYSLOG_RAM_MSG_ENTRY_CNT_MAX (100)

/* RAM entry */
typedef struct {
    ulong                   id;                         /* Message ID */
    vtss_appl_syslog_lvl_t  lvl;                        /* Level */
    vtss_module_id_t        mid;                        /* Module ID */
    time_t                  time;                       /* Time stamp */
    char                    msg[SYSLOG_RAM_MSG_MAX];    /* Message */
} syslog_ram_entry_t;

/* Write to RAM system log */
void syslog_ram_log(vtss_appl_syslog_lvl_t lvl, vtss_module_id_t mid, vtss_isid_t isid, mesa_port_no_t iport, const char *fmt, ...) __attribute__ ((format (__printf__, 5, 6)));

/* Macros to write to RAM system log */
#define SL(_lvl, isid, iport, _fmt, ...) syslog_ram_log(_lvl, VTSS_TRACE_MODULE_ID, isid, iport, _fmt, ##__VA_ARGS__)
#define S_E(_fmt, ...)  SL(VTSS_APPL_SYSLOG_LVL_ERROR,     VTSS_ISID_END, fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT), _fmt, ##__VA_ARGS__)
#define S_W(_fmt, ...)  SL(VTSS_APPL_SYSLOG_LVL_WARNING,   VTSS_ISID_END, fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT), _fmt, ##__VA_ARGS__)
#define S_N(_fmt, ...)  SL(VTSS_APPL_SYSLOG_LVL_NOTICE,    VTSS_ISID_END, fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT), _fmt, ##__VA_ARGS__)
#define S_I(_fmt, ...)  SL(VTSS_APPL_SYSLOG_LVL_INFO,      VTSS_ISID_END, fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT), _fmt, ##__VA_ARGS__)
/* Not implement yet
#define S_EM(_fmt, ...) SL(VTSS_APPL_SYSLOG_LVL_EMERGENCY, VTSS_ISID_END, VTSS_PORT_NO_END, _fmt, ##__VA_ARGS__)
#define S_A(_fmt, ...)  SL(VTSS_APPL_SYSLOG_LVL_ALERT,     VTSS_ISID_END, VTSS_PORT_NO_END, _fmt, ##__VA_ARGS__)
#define S_C(_fmt, ...)  SL(VTSS_APPL_SYSLOG_LVL_CRITICAL,  VTSS_ISID_END, VTSS_PORT_NO_END, _fmt, ##__VA_ARGS__)
#define S_D(_fmt, ...)  SL(VTSS_APPL_SYSLOG_LVL_DEBUG,     VTSS_ISID_END, VTSS_PORT_NO_END, _fmt, ##__VA_ARGS__) */

/* The following macros of S_PORT_X() are used on stacking architecture.
 * When the primary switch requests and later receives the syslog from the secondary switch, it will
 * replace all these place holders with the actual/correct USIDs from that secondary switch.
 * Notes that the magic keyword (SYSLOG_PORT_INFO_REPLACE_KEYWORD) must be inclued
 * in the logging message content.
 */
#define SYSLOG_PORT_INFO_REPLACE_TAG_START  "<PORT_INFO_S>"
#define SYSLOG_PORT_INFO_REPLACE_TAG_END    "<PORT_INFO_E>"
#define SYSLOG_PORT_INFO_REPLACE_KEYWORD    SYSLOG_PORT_INFO_REPLACE_TAG_START "XX/YY" SYSLOG_PORT_INFO_REPLACE_TAG_END

#define S_PORT_E(isid, iport, _fmt, ...)    SL(VTSS_APPL_SYSLOG_LVL_ERROR,   isid, iport, _fmt, ##__VA_ARGS__)
#define S_PORT_W(isid, iport, _fmt, ...)    SL(VTSS_APPL_SYSLOG_LVL_WARNING, isid, iport, _fmt, ##__VA_ARGS__)
#define S_PORT_N(isid, iport, _fmt, ...)    SL(VTSS_APPL_SYSLOG_LVL_NOTICE,  isid, iport, _fmt, ##__VA_ARGS__)
#define S_PORT_I(isid, iport, _fmt, ...)    SL(VTSS_APPL_SYSLOG_LVL_INFO,    isid, iport, _fmt, ##__VA_ARGS__)

/* Clear RAM system log */
void syslog_ram_clear(vtss_isid_t isid, vtss_appl_syslog_lvl_t lvl);

/* Get RAM system log entry.
   Note: The newest log can over-write the oldest log when syslog buffer full.
 */
BOOL syslog_ram_get(vtss_isid_t             isid,    /* ISID */
                    BOOL                    next,    /* Next or specific entry */
                    ulong                   id,      /* Entry ID */
                    vtss_appl_syslog_lvl_t  lvl,     /* VTSS_APPL_SYSLOG_LVL_ALL is wildcard */
                    vtss_module_id_t        mid,     /* VTSS_MODULE_ID_NONE is wildcard */
                    syslog_ram_entry_t      *entry); /* Returned data */

/* RAM system log statistics */
typedef struct {
    ulong count[VTSS_APPL_SYSLOG_LVL_ALL]; /* Number of entries at each level */
} syslog_ram_stat_t;

/* Get RAM system log statistics */
mesa_rc syslog_ram_stat_get(vtss_isid_t isid, syslog_ram_stat_t *stat);

/*--------------------------------------------------------------------------*/

/****************************************************************************/
// syslog_init()
// Module initialization function.
/****************************************************************************/
mesa_rc syslog_init(vtss_init_data_t *data);

/**
  * \brief Retrieve an error string based on a return code
  *        from one of the syslog API functions.
  *
  * \param rc [IN]: Error code that must be in the SYSLOG_ERROR_XXX range.
  */
const char *syslog_error_txt(mesa_rc rc);

/**
  * \brief Get the global default syslog configuration.
  *
  * \param glbl_cfg [OUT]: Pointer to structure that contains the
  *                       global configuration to apply to the
  *                       syslog module.
  */
void syslog_mgmt_default_get(vtss_appl_syslog_server_conf_t *glbl_cfg);

/**
 * \brief Get all history by step, we get a entry by one time via the mechanism of recording next entry. It's only used for VTSS_JSON_GET_ALL_PTR.
 * \param isid       [IN]: Switch ID.
 * \param syslog_id [OUT]: Syslog entry ID.
 * \param ptr     [INOUT]: Syslog next entry address.
 * \param history   [OUT]: Syslog entry data.
 * \return Error code.
 */
mesa_rc vtss_appl_syslog_history_get_all_by_step(vtss_isid_t *isid,
                                                 uint32_t *syslog_idx,
                                                 void **ptr,
                                                 vtss_appl_syslog_history_t *history);
#ifdef __cplusplus
}
#endif

#endif /* _SYSLOG_API_H_ */

