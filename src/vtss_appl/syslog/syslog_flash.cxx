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

// This file implements the flash-based syslog. For the RAM-based, please see
// syslog.cxx.

#include "main.h"
#include "syslog_api.h"
#include "led_api.h"
#include "misc_api.h"
#include "flash_mgmt_api.h"
#include "control_api.h"
#include "vtss_trace_api.h"
#include <vtss/basics/notifications/lock-global-subject.hxx>

#define SYSLOG_MAX_WR_CNT                    20
#define SYSLOG_FLASH_HDR_COOKIE              0x474F4C53   /* Cookie 'SLOG'(for "Syslog")        */
#define SYSLOG_FLASH_HDR_VERSION             1            /* Configuration version number       */
#define SYSLOG_FLASH_ENTRY_COOKIE            0x59544E53   /* Cookie 'SNTY' (for "Syslog Entry") */
#define SYSLOG_FLASH_ENTRY_VERSION           1            /* Entry version number               */
#define SYSLOG_FLASH_UNINIT_FLASH_VALUE_LONG (-1)
#define SYSLOG_NEXT_32_BIT_BOUNDARY(sz)      (4 * (((sz) + 3) / 4))
#define VTSS_TRACE_MODULE_ID                 VTSS_MODULE_ID_SYSLOG
#define VTSS_ALLOC_MODULE_ID                 VTSS_MODULE_ID_SYSLOG

/******************************************************************************/
// SL_flash_hdr_t
/******************************************************************************/
typedef struct {
    u32 size;    // Header size in bytes
    u32 cookie;  // Header cookie
    u32 version; // Header version number
} SL_flash_hdr_t;

/******************************************************************************/
// SL_flash_entry_t
/******************************************************************************/
typedef struct {
    u32                    size;    // Entry size in bytes
    u32                    cookie;  // Entry cookie
    u32                    version; // Entry version number
    time_t                 time;    // Time of saving.
    syslog_cat_t           cat;     // Category (either of the SYSLOG_CAT_xxx constants)
    vtss_appl_syslog_lvl_t lvl;     // Level (either of the SYSLOG_LVL_xxx constants)
    // Here follows the NULL-terminated message
} SL_flash_entry_t;

// Flash Syslog Variables
static BOOL                      SL_flash_enabled;
static vtss_flashaddr_t          SL_flash_next_free_entry;
static int                       SL_flash_entry_cnt[SYSLOG_CAT_ALL][VTSS_APPL_SYSLOG_LVL_ALL]; // Counts per category and per level the number of entries in the syslog.
static flash_mgmt_section_info_t SL_flash_info;
static uint SL_wr_cnt = 0;

/******************************************************************************/
// SL_flash_addr_get()
// Find the syslog in the flash. This will update the SL_flash_syslog_XXX
// variables.
/******************************************************************************/
static BOOL SL_flash_addr_get(void)
{
    if (!flash_mgmt_lookup("syslog", &SL_flash_info)) {
        // Unable to obtain info about the "syslog" entry.
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************/
// SL_flash_load()
// Checks the flash for the system log and updates first and next free
// pointers.
// If the syslog is present in flash, it returns TRUE, otherwise FALSE.
/******************************************************************************/
static BOOL SL_flash_load(void)
{
    vtss_flashaddr_t flptr, next_flptr;
    SL_flash_hdr_t hdr_buf, *hdr = &hdr_buf;
    BOOL result = FALSE;

    SL_flash_next_free_entry = 0;

    flptr = SL_flash_info.base_fladdr;
    memset(&SL_flash_entry_cnt[0][0], 0, sizeof(SL_flash_entry_cnt));

    if (control_flash_read(flptr, hdr, sizeof(*hdr)) != VTSS_FLASH_ERR_OK ||
        hdr->size != sizeof(SL_flash_hdr_t) ||
        hdr->cookie != SYSLOG_FLASH_HDR_COOKIE ||
        hdr->version != SYSLOG_FLASH_HDR_VERSION) {
        goto do_exit; // Syslog not found
    }

    // Go to the first entry. Both the header and entries are 32-bit aligned.
    flptr += SYSLOG_NEXT_32_BIT_BOUNDARY(hdr->size);

    while (flptr < SL_flash_info.base_fladdr + SL_flash_info.size_bytes - sizeof(SL_flash_entry_t)) {
        SL_flash_entry_t entry_buf, *entry = &entry_buf;

        // If the entry contains uninitialized flash values, we expect this to be the very first empty entry,
        // and expect this area to be writeable without erasing the whole sector.
        if (control_flash_read(flptr, entry, sizeof(*entry)) != VTSS_FLASH_ERR_OK) {
            goto do_exit;
        }

        if (entry->size    == SYSLOG_FLASH_UNINIT_FLASH_VALUE_LONG &&
            entry->cookie  == SYSLOG_FLASH_UNINIT_FLASH_VALUE_LONG &&
            entry->version == SYSLOG_FLASH_UNINIT_FLASH_VALUE_LONG &&
            entry->time    == (time_t)SYSLOG_FLASH_UNINIT_FLASH_VALUE_LONG &&
            entry->cat     == (syslog_cat_t)SYSLOG_FLASH_UNINIT_FLASH_VALUE_LONG &&
            entry->lvl     == (vtss_appl_syslog_lvl_t)SYSLOG_FLASH_UNINIT_FLASH_VALUE_LONG) {
            SL_flash_next_free_entry = flptr;
            result = TRUE;
            goto do_exit;
        }

        next_flptr = flptr + SYSLOG_NEXT_32_BIT_BOUNDARY(entry->size);

        if (entry->size    <= sizeof(SL_flash_entry_t)         || // Equal sign because the message following the header must be non-empty (at least a NULL character)
            next_flptr     <= flptr                            ||
            next_flptr     >= (SL_flash_info.base_fladdr + SL_flash_info.size_bytes) ||
            entry->cookie  != SYSLOG_FLASH_ENTRY_COOKIE        ||
            entry->version != SYSLOG_FLASH_ENTRY_VERSION       ||
            entry->cat     >= SYSLOG_CAT_ALL                   ||
            entry->lvl     >= VTSS_APPL_SYSLOG_LVL_ALL) {
            memset(&SL_flash_entry_cnt[0][0], 0, sizeof(SL_flash_entry_cnt));
            goto do_exit;
        }

        flptr = next_flptr;
        SL_flash_entry_cnt[entry->cat][entry->lvl]++;
    }

    memset(&SL_flash_entry_cnt[0][0], 0, sizeof(SL_flash_entry_cnt));
    // We dropped out of the area allocated for us.

do_exit:
    return result;
}

/******************************************************************************/
// SL_flash_create()
// Unconditionally erases the flash and creates a new header signature.
// Returns FALSE on error, TRUE on success.
/******************************************************************************/
static BOOL SL_flash_create(void)
{
    SL_flash_hdr_t hdr;

    SL_flash_enabled = FALSE; // Disallow updates.
    memset(&SL_flash_entry_cnt[0][0], 0, sizeof(SL_flash_entry_cnt));
    SL_flash_next_free_entry = 0;

    // Erase the flash and create a new syslog signature.
    if (control_flash_erase(SL_flash_info.base_fladdr, SL_flash_info.size_bytes) != VTSS_FLASH_ERR_OK) {
        return FALSE; // Erase failed. We keep the flash logging disabled.
    }

    hdr.size    = sizeof(SL_flash_hdr_t);
    hdr.cookie  = SYSLOG_FLASH_HDR_COOKIE;
    hdr.version = SYSLOG_FLASH_HDR_VERSION;

    if (control_flash_program(SL_flash_info.base_fladdr, &hdr, sizeof(hdr)) != VTSS_FLASH_ERR_OK) {
        return FALSE; // Program failed. We keep the flash logging disabled.
    }

    // The erase above only writes to RAM, not to the flash. In order to write to the flash,
    // we must call program(). It's OK to provide the same address to write to as
    // the address we write from (1st and 2nd argument).
    if (control_flash_program(SL_flash_info.base_fladdr + sizeof(hdr), SL_flash_info.base_fladdr + sizeof(hdr), SL_flash_info.size_bytes - sizeof(hdr)) != VTSS_FLASH_ERR_OK) {
        return FALSE; // Program
    }

    SL_flash_next_free_entry = SL_flash_info.base_fladdr + SYSLOG_NEXT_32_BIT_BOUNDARY(sizeof(SL_flash_hdr_t));
    SL_flash_enabled = TRUE; // Everything is OK. Allow updates from now on.

    return TRUE;
}

/******************************************************************************/
// SL_flash_open()
// Checks if the flash contains a valid syslog. If not, the flash is erased
// and a valid syslog is created.
/******************************************************************************/
static void SL_flash_open(void)
{
    // Find the syslog in the flash.
    if (!SL_flash_addr_get()) {
        // No syslog. Keep writing to flash disabled.
        SL_flash_enabled = FALSE;
        return;
    }

    if (!SL_flash_load()) {
        // The current contents of the flash wasn't valid. Erase and create new signature.
        if (!SL_flash_create()) {
            T_E("Unable to create an empty syslog flash");
        }
    } else {
        SL_flash_enabled = TRUE;
    }
}

/******************************************************************************/
// SL_cat_to_string()
/******************************************************************************/
static const char *SL_cat_to_string(syslog_cat_t cat)
{
    switch (cat) {
    case SYSLOG_CAT_DEBUG:
        return "Debug";

    case SYSLOG_CAT_SYSTEM:
        return "System";

    case SYSLOG_CAT_APP:
        return "Application";

    default:
        return "Unknown";
    }
}

/******************************************************************************/
// SL_get_time_in_secs()
/******************************************************************************/
static time_t SL_get_time_in_secs(void)
{
    return time(NULL);
}

/******************************************************************************/
// SL_flash_print_header()
/******************************************************************************/
static void SL_flash_print_header(int (*print_function)(const char *fmt, ...))
{
    int i;

    (void)print_function("%-11s | %-13s | %-25s | %s\n", "Category", "Level", "Time", "Message");

    for (i = 0; i < 80; i++) {
        (void)print_function("-");
    }

    (void)print_function("\n");
}

/******************************************************************************/
// SL_flash_log()
/******************************************************************************/
static void SL_flash_log(syslog_cat_t cat, vtss_appl_syslog_lvl_t lvl, const char *msg)
{
    u32              msg_sz, total_sz;
    SL_flash_entry_t entry;

    msg_sz = strlen(msg) + 1; // Include NULL-terminator in size
    total_sz = sizeof(SL_flash_entry_t) + msg_sz;

    // Check if there's room for this message
    if (SL_flash_next_free_entry + total_sz >= SL_flash_info.base_fladdr + SL_flash_info.size_bytes) {
        SL_flash_enabled = FALSE;
        return;
    }

    entry.size    = total_sz;
    entry.cookie  = SYSLOG_FLASH_ENTRY_COOKIE;
    entry.version = SYSLOG_FLASH_ENTRY_VERSION;
    entry.time    = SL_get_time_in_secs();
    entry.cat     = cat;
    entry.lvl     = lvl;

    // Write the entry header to flash.
    if (control_flash_program(SL_flash_next_free_entry, &entry, sizeof(entry)) != VTSS_FLASH_ERR_OK) {
        SL_flash_enabled = FALSE;
        return; // Program failed. We keep the flash logging disabled.
    }

    // Write the message to flash.
    if (control_flash_program((SL_flash_next_free_entry + sizeof(entry)), msg, msg_sz) != VTSS_FLASH_ERR_OK) {
        SL_flash_enabled = FALSE;
    }

    // Update the next free entry pointer
    SL_flash_next_free_entry += SYSLOG_NEXT_32_BIT_BOUNDARY(total_sz);
    SL_flash_entry_cnt[cat][lvl]++;
}

/******************************************************************************/
// SL_init()
// Performs lazy initialization
/******************************************************************************/
static void SL_init(void)
{
    static BOOL initialized;

    if (!initialized) {
        // Open the flash syslog and make it ready for reading and writing.
        SL_flash_open();

        initialized = TRUE;
    }
}

/******************************************************************************/
// syslog_flash_log()
/******************************************************************************/
void syslog_flash_log(syslog_cat_t cat, vtss_appl_syslog_lvl_t lvl, const char *msg)
{
    static const char *too_many_msg = "Too many messages written to syslog. Last messages skipped.";

    if (cat >= SYSLOG_CAT_ALL || lvl >= VTSS_APPL_SYSLOG_LVL_ALL) {
        // Illegal.
        return;
    }

    {
        // Use the lazy-initialized leaf-mutex to protect ourselves.
        vtss::notifications::LockGlobalSubject lock(__FILE__, __LINE__);

        // Perform lazy initialization
        SL_init();

        if (!SL_flash_enabled) {
            return;
        }

        if (++SL_wr_cnt > SYSLOG_MAX_WR_CNT) {
            if (SL_wr_cnt == SYSLOG_MAX_WR_CNT + 1) {
                msg = too_many_msg;
                lvl = VTSS_APPL_SYSLOG_LVL_ERROR;
                cat = SYSLOG_CAT_SYSTEM;
            } else {
                SL_wr_cnt = SYSLOG_MAX_WR_CNT + 2; // Avoid wrap-around
                return;
            }
        }

        SL_flash_log(cat, lvl, msg);
    }
}

/******************************************************************************/
// syslog_flash_erase()
/******************************************************************************/
BOOL syslog_flash_erase(void)
{
    BOOL result;

    {
        // Use the lazy-initialized leaf-mutex to protect ourselves.
        vtss::notifications::LockGlobalSubject lock(__FILE__, __LINE__);

        // Perform lazy initialization
        SL_init();

        if ((result = SL_flash_create()) == FALSE) {
            T_E("Unable to create an empty syslog flash");
        }

        SL_wr_cnt = 0;
    }

    // Clear error of system LED state and back to previous state
    led_front_led_state_clear(LED_FRONT_LED_ERROR);

    return result;
}

/******************************************************************************/
// syslog_flash_print()
/******************************************************************************/
void syslog_flash_print(syslog_cat_t cat, vtss_appl_syslog_lvl_t lvl, int (*print_function)(const char *fmt, ...))
{
    vtss_flashaddr_t flptr, msg;
    SL_flash_entry_t entry_buf;
    SL_flash_entry_t *entry             = &entry_buf;
    BOOL             at_least_one_found = FALSE;
    u8               *msgbuf            = NULL;
    size_t           msgbuf_len         = 0;

    if (!print_function) {
        return;
    }

    if (cat > SYSLOG_CAT_ALL || lvl > VTSS_APPL_SYSLOG_LVL_ALL) {
        (void)print_function("Invalid category or level\n");
        return;
    }

    // Use the lazy-initialized leaf-mutex to protect ourselves.
    vtss::notifications::lock_global_subject.lock(__FILE__, __LINE__);

    // Perform lazy initialization
    SL_init();

    if (!SL_flash_enabled) {
        vtss::notifications::lock_global_subject.unlock(__FILE__, __LINE__);
        (void)print_function("The syslog is not enabled\n");
        return;
    }

    flptr = SL_flash_info.base_fladdr + SYSLOG_NEXT_32_BIT_BOUNDARY(sizeof(SL_flash_hdr_t));
    while (flptr < SL_flash_next_free_entry) { /*lint -e{449} ... We're aware of the realloc hazards */
        if (control_flash_read(flptr, entry, sizeof(*entry)) == VTSS_FLASH_ERR_OK &&
            (cat == SYSLOG_CAT_ALL || entry->cat == cat) &&
            (lvl == VTSS_APPL_SYSLOG_LVL_ALL || entry->lvl == lvl)) {
            size_t msglen = entry->size - sizeof(SL_flash_entry_t); /* Includes NULL */

            if (!at_least_one_found) {
                vtss::notifications::lock_global_subject.unlock(__FILE__, __LINE__);
                SL_flash_print_header(print_function);
                vtss::notifications::lock_global_subject.lock(__FILE__, __LINE__);
            }

            /* Allocate message buffer */
            if (msglen > msgbuf_len) {
                u8 *newbuf = (u8 *)VTSS_REALLOC(msgbuf, msglen);
                if (newbuf) {
                    msgbuf_len = msglen;
                    msgbuf = newbuf; /* May have changed */
                }
            }

            msg = flptr + sizeof(SL_flash_entry_t);
            if (msgbuf && msglen <= msgbuf_len &&
                control_flash_read(msg, msgbuf, msglen) == VTSS_FLASH_ERR_OK) {
                msgbuf[msglen - 1] = '\0'; /* Terminate to be safe (should be unnecessary)  */
                vtss::notifications::lock_global_subject.unlock(__FILE__, __LINE__);
                (void)print_function("%-11s | %-13s | %s | %s\n", SL_cat_to_string(entry->cat), syslog_lvl_to_string(entry->lvl, FALSE), misc_time2str(entry->time), msgbuf);
                vtss::notifications::lock_global_subject.lock(__FILE__, __LINE__);
                at_least_one_found = TRUE;
            }
        }

        flptr += SYSLOG_NEXT_32_BIT_BOUNDARY(entry->size);
    }

    if (msgbuf) {
        VTSS_FREE(msgbuf);
    }

    vtss::notifications::lock_global_subject.unlock(__FILE__, __LINE__);

    if (!at_least_one_found) {
        (void)print_function("No entries found\n");
    }
}

/******************************************************************************/
// syslog_flash_entry_cnt()
/******************************************************************************/
int syslog_flash_entry_cnt(syslog_cat_t cat, vtss_appl_syslog_lvl_t lvl)
{
    // Use the lazy-initialized leaf-mutex to protect ourselves.
    vtss::notifications::LockGlobalSubject lock(__FILE__, __LINE__);

    int entry_cnt = 0, c, l;

    if (cat > SYSLOG_CAT_ALL || lvl > VTSS_APPL_SYSLOG_LVL_ALL) {
        // Illegal
        return 0;
    }

    // Perform lazy initialization
    SL_init();

    if (!SL_flash_enabled) {
        return entry_cnt;
    }

    if (cat == SYSLOG_CAT_ALL) {
        if (lvl == VTSS_APPL_SYSLOG_LVL_ALL) {
            // All entries summed up
            for (c = 0; c < SYSLOG_CAT_ALL; c++) {
                for (l = 0; l < VTSS_APPL_SYSLOG_LVL_ALL; l++) {
                    entry_cnt += SL_flash_entry_cnt[c][l];
                }
            }
        } else {
            // All categories, specific levels
            for (c = 0; c < SYSLOG_CAT_ALL; c++) {
                entry_cnt += SL_flash_entry_cnt[c][lvl];
            }
        }
    } else {
        if (lvl == VTSS_APPL_SYSLOG_LVL_ALL) {
            // Specific category, all levels
            for (l = 0; l < SYSLOG_CAT_ALL; l++) {
                entry_cnt += SL_flash_entry_cnt[cat][l];
            }
        } else {
            // Specific category, specific level
            entry_cnt = SL_flash_entry_cnt[cat][lvl];
        }
    }

    return entry_cnt;
}

