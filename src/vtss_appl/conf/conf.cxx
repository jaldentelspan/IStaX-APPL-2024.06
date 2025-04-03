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

#define __VTSS_CONF_C__

#include "main.h"
#include "conf_api.h"
#include "msg_api.h"
#include "flash_mgmt_api.h"
#include "control_api.h" /* For control_flash_XXX() */
#include "critd_api.h"
#include "misc_api.h"
#include "vtss_mtd_api.hxx"
#include "conf.h"
#include "vtss_uboot.hxx"
#include "sysutil_api.h"
#include "otp_interface.hxx"
#ifdef VTSS_SW_OPTION_ICFG
#include "icfg_api.h"
#endif

#include <zlib.h>
#include <vtss/basics/map.hxx>
#include <vtss/basics/optional.hxx>
#include <vtss/basics/stream.hxx>
#include <vtss/basics/parser_impl.hxx>
#include <fcntl.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_CONF
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_CONF

#define FLASH_CONF_BOARD_SIZE (8*1024)   /* 8k board configuration */
#define FLASH_CONF_LOCAL_SIZE (128*1024) /* Local section size */

/* Board configuration signatures */
#define FLASH_CONF_BOARD_SIG      "#@(#)VtssConfig"
#define FLASH_CONF_MAC_TAG        "MAC"
#define FLASH_CONF_MAC2_TAG       "MAC_ADDR"
#define FLASH_CONF_MAC3_TAG       "ethaddr"
#define FLASH_CONF_BOARD_ID_TAG   "BOARDID"
#define FLASH_CONF_PASSWORD_TAG   "password"

#define MAX_TAG_NAME_LENGTH       32    // Arbitrarily set

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Variables containing address and size of where to retrieve/store configuration from/in the flash */
static flash_mgmt_section_info_t conf_flash_info_local;
static flash_mgmt_section_info_t conf_flash_info_stack;

static vtss_flag_t control_flags; /* CONF thread control */

/* Global structure */
static conf_global_t conf;

vtss::Map<std::string, std::string> conf_board_tags;

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "conf", "Configuration"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/****************************************************************************/
/*  Common functions                                                        */
/****************************************************************************/

#define CONF_CRIT_ENTER() critd_enter(&conf.crit, __FILE__, __LINE__)
#define CONF_CRIT_EXIT()  critd_exit( &conf.crit, __FILE__, __LINE__)

bool conf_is_uboot()
{
    FILE *procmtd;
    char mtdentry[128] = {0};
    const char *mtdname1 = "redboot";

    if ((procmtd = fopen("/proc/mtd", "r"))) {
        while (fgets(mtdentry, sizeof(mtdentry), procmtd)) {
            if (strcasestr(mtdentry, mtdname1)) {
                fclose(procmtd);
                return false;   // RedBoot, in fact
            }
        }
        fclose(procmtd);
    }
    return true;        // No RedBoot, must be U-Boot
}

/* Update flash section info */
static void conf_flash_get_section_info(void)
{
    if (!conf_is_uboot()) {
        if (!flash_mgmt_lookup("conf", &conf_flash_info_local)) {
            T_E("Unable to locate 'conf' section in flash");
        }
    }

    if (!flash_mgmt_lookup("stackconf", &conf_flash_info_stack)) {
        T_E("Unable to locate 'stackconf' section in flash");
    }
}

/* Get base address and size of Flash section */
static vtss_flashaddr_t conf_flash_section_base(conf_sec_t sec, size_t *size)
{
    /* Global section is located at the beginning */
    vtss_flashaddr_t base = conf_flash_info_stack.base_fladdr;
    *size = (conf_flash_info_stack.size_bytes - FLASH_CONF_LOCAL_SIZE);

    /* Local section is located at the end */
    if (sec == CONF_SEC_LOCAL) {
        base += *size;
        *size = FLASH_CONF_LOCAL_SIZE;
    }

    return base;
}

/* Write data to flash, optionally erasing */
static int conf_flash_write(vtss_flashaddr_t dest, const void *data, size_t data_len, int erase, int erase_len)
{
    vtss_flashaddr_t base, addr;
    int   rc;

    /* Check that the destination area is valid */
    addr = dest;
    base = conf_flash_info_local.base_fladdr;
    if (addr >= base && addr < (base + conf_flash_info_local.size_bytes)) {
        /* Inside board section */
    } else {
        base = conf_flash_info_stack.base_fladdr;
        if (addr < base || addr > (base + conf_flash_info_stack.size_bytes)) {
            /* Outsize stack section */
            T_E("illegal dest: 0x%p", (void *)dest);
            return -1;
        }
    }

    /* Erase flash */
    if (erase && ((rc = control_flash_erase(dest, erase_len)) != VTSS_FLASH_ERR_OK)) {
        return -1;
    }

    /* Program flash */
    if ((rc = control_flash_program(dest, data, data_len)) != VTSS_FLASH_ERR_OK) {
        return -1;
    }
    return 0;
}

/****************************************************************************/
/*  Board configuration                                                     */
/****************************************************************************/

int conf_mgmt_board_get(conf_board_t *board)
{
    T_D("enter");

    CONF_CRIT_ENTER();
    *board = conf.board;
    CONF_CRIT_EXIT();

    T_D("exit");
    return 0;
}

mesa_rc conf_mgmt_board_tag_set(const char *name_, const char *value_)
{
    std::string name(name_);
    std::string value(value_);
    if (name.length() == 0 ||
        name.length() > MAX_TAG_NAME_LENGTH ||
        name.find_first_of(" =") != std::string::npos) {
        return VTSS_INVALID_PARAMETER;
    }
    if (value.find_first_of("=") != std::string::npos) {
        return VTSS_INVALID_PARAMETER;
    }
    conf_board_tags.set(name, value);
    return VTSS_RC_OK;
}

mesa_rc conf_mgmt_board_tag_remove(const char *name)
{
    std::string name_(name);
    if (name_.length() == 0 ||
        name_.length() > MAX_TAG_NAME_LENGTH ||
        name_.find_first_of(" =") != std::string::npos) {
        return VTSS_RC_ERR_PARM;
    }
    if (conf_board_tags.erase(name_) > 0) {
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

const char *conf_mgmt_board_tag_get(const char *name)
{
    auto it = conf_board_tags.find(name);
    if (it != conf_board_tags.end()) {
        return it->second.c_str();
    }
    return NULL;
}

#if defined(VTSS_SW_OPTION_ICLI)
mesa_rc conf_mgmt_board_tag_list(u32 session_id)
{
    icli_session_printf(session_id, "Board tags:\n");
    for (const auto &e : conf_board_tags) {
        icli_session_printf(session_id, "%s=%s\n", e.first.c_str(), e.second.c_str());
    }
    return VTSS_RC_OK;
}
#endif

static mesa_rc conf_mgmt_board_tag_save_redboot(void)
{
    std::string buf(FLASH_CONF_BOARD_SIG);

    for (const auto &e : conf_board_tags) {
        buf.append("\n");
        buf.append(e.first);
        buf.append("=");
        buf.append(e.second);
    }

    return conf_flash_write(conf_flash_info_local.base_fladdr, buf.c_str(), buf.length() + 1,
                            1, conf_flash_info_local.size_bytes);
}

static mesa_rc save_tag(const std::string &key, const std::string &val)
{
    return vtss_uboot_set_env(key.data(), val.data());
}

static mesa_rc conf_mgmt_board_tag_save_uboot(void)
{
    for (const auto &e : conf_board_tags) {
        if (save_tag(e.first, e.second) != VTSS_RC_OK) {
            return VTSS_RC_ERROR;
        }
    }
    return VTSS_RC_OK;
}

mesa_rc conf_mgmt_board_tag_save(void)
{
    if (conf_is_uboot()) {
        return conf_mgmt_board_tag_save_uboot();
    } else {
        return conf_mgmt_board_tag_save_redboot();
    }
}

/* Internal conf_mgmt_board_set function */
static int conf_mgmt_board_set_(conf_board_t *board)
{
    vtss::BufStream<vtss::SBuf128> buf;

    /* Set binary config */
    if (&conf.board != board) {
        conf.board = *board;
    }

    if (board->mac_address.addr[0] & (VTSS_BIT(0) | VTSS_BIT(1))) {  // bit 0: bcast. bit 1: Locally administered
        T_W("Not saving board config: MAC is locally administered address or broadcast address");
        return VTSS_INVALID_PARAMETER;
    }

    /* Stringify config */
    buf << board->mac_address;
    if (conf_is_uboot()) {
        conf_mgmt_board_tag_set(FLASH_CONF_MAC3_TAG, buf.cstring());
    } else {
        conf_mgmt_board_tag_set(FLASH_CONF_MAC_TAG, buf.cstring());
        conf_mgmt_board_tag_set(FLASH_CONF_MAC2_TAG, buf.cstring());
    }

    buf.clear();
    buf << board->board_id;
    conf_mgmt_board_tag_set(FLASH_CONF_BOARD_ID_TAG, buf.cstring());

    /* Save to FLASH */
    return conf_mgmt_board_tag_save();
}

/* Set board configuration */
int conf_mgmt_board_set(conf_board_t *board)
{
    int rc;

    T_D("enter");

    CONF_CRIT_ENTER();
    rc = conf_mgmt_board_set_(board);
    CONF_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Get MAC address (index 0 is the base MAC address). */
int conf_mgmt_mac_addr_get(uchar *mac, uint index)
{

    T_N("enter");
    if (index > fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        T_E("index %d out of range", index);
        return -1;
    }

    CONF_CRIT_ENTER();
    misc_instantiate_mac(mac, conf.board.mac_address.addr, index);
    CONF_CRIT_EXIT();

    T_N("exit");
    return 0;
}

static void conf_board_parse_tags(char *base, const char *sig)
{
    const char *delim = "\n";
    char *p, *equal;
    char *saveptr; // Local strtok_r() context
    for (p = strtok_r(base + strlen(sig), delim, &saveptr); p != NULL; p = strtok_r(NULL, delim, &saveptr)) {
        if ((equal = strchr(p, '='))) {
            conf_board_tags.insert(
                vtss::Pair<std::string, std::string>(
                    std::string(p, equal - p),
                    std::string(equal + 1)));
        }
    }
}

static void parse_mac(const char *entry, conf_board_t *board, BOOL &found)
{
    vtss::parser::MacAddress mac;
    if (parser_cstr(entry, mac)) {
        board->mac_address = mac.mac.as_api_type();
        found = TRUE;
    }
}

template <typename T>
static void parse_long(const char *entry, const char *format,
                       T *res, BOOL &found)
{
    if (sscanf(entry, format, res) == 1) {
        found = TRUE;
    }
}

static void conf_board_start_redboot(BOOL &mac_found, BOOL &board_found)
{
    char         *base;
    const char   *sig = FLASH_CONF_BOARD_SIG "\n";
    conf_board_t *board = &conf.board;

    T_D("enter");
    VTSS_MALLOC_CAST(base, FLASH_CONF_BOARD_SIZE);
    if (base &&
        (VTSS_FLASH_ERR_OK == vtss_flash_read(conf_flash_info_local.base_fladdr, base, FLASH_CONF_BOARD_SIZE, NULL )) &&
        strncmp(sig, base, strlen(sig)) == 0) {
        base[FLASH_CONF_BOARD_SIZE - 1] = 0; /* Zero terminate */
        conf_board_parse_tags(base, sig);
        auto it = conf_board_tags.find(FLASH_CONF_MAC_TAG);
        if (it == conf_board_tags.end()) {
            // Retry with new name..
            conf_board_tags.find(FLASH_CONF_MAC2_TAG);
        }
        if (it != conf_board_tags.end()) {
            parse_mac(it->second.c_str(), board, mac_found);
            T_D("Board MAC address %s", board->mac_address);
        }
        it = conf_board_tags.find(FLASH_CONF_BOARD_ID_TAG);
        if (it != conf_board_tags.end()) {
            parse_long(it->second.c_str(), VPRIlu, &board->board_id,
                       board_found);
            T_D("Board ID: " VPRIlu, board->board_id);
        }
        it = conf_board_tags.find(FLASH_CONF_PASSWORD_TAG);
        if (it != conf_board_tags.end()) {
            strncpy(board->default_password, it->second.c_str(), sizeof(board->default_password));
            T_D("Default Password: %s", board->default_password);
        } else {
            strcpy(board->default_password, VTSS_SYS_ADMIN_PASSWORD); /* Refer to \webstax2\vtss_appl\sysutil\sysutil_api.h */
        }
    } else {
        strcpy(board->default_password, VTSS_SYS_ADMIN_PASSWORD); /* Refer to \webstax2\vtss_appl\sysutil\sysutil_api.h */
        T_D("Invalid configuration detected (Signature Check Failed)");
    }
    if (base) {
        VTSS_FREE(base);
        base = NULL;
    }
}

static void conf_board_start_uboot(BOOL &mac_found, BOOL &board_found)
{
    auto res = vtss_uboot_get_env(FLASH_CONF_MAC3_TAG);
    if (res.valid()) {
        parse_mac(res.get().c_str(), &conf.board, mac_found);
        T_D("Board MAC address %s", conf.board.mac_address);
    }
    if (!mac_found) {
        mac_found = (VTSS_RC_OK == otp_get_mac_address(&conf.board.mac_address));
    }

    res = vtss_uboot_get_env(FLASH_CONF_BOARD_ID_TAG);
    if (res.valid()) {
        parse_long(res.get().c_str(), VPRIlu, &conf.board.board_id,
                   board_found);
        T_D("Board ID: " VPRIlu, conf.board.board_id);
    }
    res = vtss_uboot_get_env(FLASH_CONF_PASSWORD_TAG);
    if (res.valid()) {
        strncpy(conf.board.default_password, res.get().c_str(), sizeof(conf.board.default_password));
    } else if (VTSS_RC_OK != otp_get_password(sizeof(conf.board.default_password),
                                              conf.board.default_password)) {
        strcpy(conf.board.default_password, VTSS_SYS_ADMIN_PASSWORD); /* Refer to \webstax2\vtss_appl\sysutil\sysutil_api.h */
    }
}

static void conf_board_start(void)
{
    BOOL mac_found = FALSE, board_found = FALSE;

    if (conf_is_uboot()) {
        conf_board_start_uboot(mac_found, board_found);
    } else {
        conf_board_start_redboot(mac_found, board_found);
    }

    if (!mac_found) {
        int fd;
        conf.board.mac_address.addr[0] = VTSS_BIT(1);    // Locally administered
        conf.board.mac_address.addr[1] = 0x00;           // VTSS OUI
        conf.board.mac_address.addr[2] = 0xc1;           // VTSS OUI
        conf.board.mac_address.addr[3] = 0x33;           // Anything, then
        conf.board.mac_address.addr[4] = 0x44;
        conf.board.mac_address.addr[5] = 0x55;
        // Make remainder 3 bytes random
        if ((fd = open("/dev/urandom", O_RDONLY | O_NONBLOCK)) != -1) {
            // coverity[check_return:SUPPRESS]
            (void)read(fd, &conf.board.mac_address.addr[3], 3);
            close(fd);
        }
        T_W("MAC address not set, using random: %s", conf.board.mac_address);
    }

    if (!board_found) {
        T_D("Board ID not found");
        conf.board.board_id = 0;
    }

    T_D("exit");
}

/****************************************************************************/
/*  Application configuration                                                */
/****************************************************************************/

/* Check if section is valid */
static BOOL conf_sec_valid(conf_sec_t sec)
{
    /* Check that the section is valid */
    if (sec >= CONF_SEC_CNT) {
        T_E("illegal sec: %d", sec);
        return 0;
    }
    return 1;
}

mesa_rc conf_mgmt_sec_blk_get(conf_sec_t sec, conf_blk_id_t id,
                              conf_mgmt_blk_t *mgmt_blk, BOOL next)
{
    conf_blk_t *blk;

    if (!conf_sec_valid(sec)) {
        return -1;
    }

    CONF_CRIT_ENTER();
    for (blk = conf.section[sec].blk_list; blk != NULL; blk = blk->next) {
        if ((blk->hdr.id == id && !next) || (blk->hdr.id > id && next)) {
            break;
        }
    }
    if (blk != NULL) {
        mgmt_blk->id = blk->hdr.id;
        mgmt_blk->size = blk->hdr.size;
        mgmt_blk->data = blk->data;
        mgmt_blk->crc = blk->crc;
        mgmt_blk->change_count = blk->change_count;
        sprintf(mgmt_blk->name, "%s",
                blk->hdr.id < (sizeof(conf_blk_name) / sizeof(char *)) &&
                conf_blk_name[blk->hdr.id] != NULL ? conf_blk_name[blk->hdr.id] : "?");
    }
    CONF_CRIT_EXIT();

    return (blk == NULL ? -1 : VTSS_RC_OK);
}

/* Get module configuration */
mesa_rc conf_mgmt_conf_get(conf_mgmt_conf_t *data)
{
    CONF_CRIT_ENTER();
    *data = conf.conf;
    CONF_CRIT_EXIT();

    return VTSS_RC_OK;
}

static void conf_sec_changed(conf_section_t *section)
{
    section->changed = 1;
    vtss_flag_setbits(&control_flags, CONF_THREAD_FLAG_COMMIT);
}

void conf_blk_crc_update(conf_section_t *section, conf_blk_t *blk)
{
    ulong crc = vtss_crc32((const unsigned char *)blk->data, blk->hdr.size);

    if (crc != blk->crc) {
        /* Block changed, mark section changed and lock reset */
        T_D("section: %s, id: %d changed, size: " VPRIlu, section->name, blk->hdr.id,  blk->hdr.size);
        blk->crc = crc;
        blk->change_count++;
        conf_sec_changed(section);
    }
}

/* Set module configuration */
mesa_rc conf_mgmt_conf_set(conf_mgmt_conf_t *data)
{
    conf_sec_t     sec;
    conf_section_t *section;
    conf_blk_t     *blk;

    CONF_CRIT_ENTER();
    if (data->change_detect && !conf.conf.change_detect) {
        /* Enable change detection */
        for (sec = CONF_SEC_LOCAL; sec < CONF_SEC_CNT; sec++) {
            section = &conf.section[sec];
            for (blk = section->blk_list; blk != NULL; blk = blk->next) {
                conf_blk_crc_update(section, blk);
            }
        }
    }
    conf.conf = *data;
    CONF_CRIT_EXIT();

    return VTSS_RC_OK;
}

static conf_blk_t *conf_blk_lookup(conf_sec_t sec, conf_blk_id_t id)
{
    conf_blk_t *blk;

    for (blk = conf.section[sec].blk_list; blk != NULL; blk = blk->next) {
        if (blk->hdr.id == id) {
            break;
        }
    }
    return blk;
}

/* Internal conf_create function */
static conf_blk_t *conf_create_(conf_sec_t sec, conf_blk_id_t id, ulong size)
{
    conf_blk_t     *blk, *prev, *new_;
    conf_section_t *section;
    ulong          max_size;

    section = &conf.section[sec];
    T_D("enter, section: %s, id: %d, size: " VPRIlu, section->name, id, size);

    max_size = (sec == CONF_SEC_LOCAL ? CONF_BLK_MAX_LOCAL : CONF_BLK_MAX_GLOBAL);
    if (size >= max_size) {
        T_E("illegal block size: " VPRIlu", max: " VPRIlu", id: %d", size, max_size, id);
        return NULL;
    }

    /* Look for block or insertion point */
    for (blk = section->blk_list, prev = NULL; blk != NULL; prev = blk, blk = blk->next) {
        if (blk->hdr.id == id) {
            /* Found block ID */
            if (blk->hdr.size == size) {
                T_D("exit, unchanged size");
                return blk;
            }
            T_D("changed size, old size: " VPRIlu, blk->hdr.size);
            break;
        }
        if (blk->hdr.id > id) {
            /* Found insertion point */
            break;
        }
    }

    /* Delete block */
    if (size == 0) {
        if (blk != NULL && blk->hdr.id == id) {
            if (prev == NULL) {
                section->blk_list = blk->next;
            } else {
                prev->next = blk->next;
            }
            VTSS_FREE(blk);

            /* Mark section changed and lock reset */
            conf_sec_changed(section);
        }
        T_D("exit, block deleted");
        return NULL;
    }

    /* Create/resize the block */
    if ((VTSS_MALLOC_CAST(new_, sizeof(conf_blk_t) + size)) == NULL) {
        T_E("exit, malloc failed");
        return NULL;
    }

    memset(new_, 0, sizeof(conf_blk_t) + size);
    new_->hdr.id = id;
    new_->hdr.size = size;
    new_->data = (new_ + 1);
    new_->change_count = 0;
    new_->crc = 0;

    if (blk != NULL && blk->hdr.id == id) {
        /* Resizing existing block */
        new_->next = blk->next;
        new_->change_count = blk->change_count;
        memcpy(new_->data, blk->data, blk->hdr.size > size ? size : blk->hdr.size);
        VTSS_FREE(blk);
    } else {
        new_->next = blk;
    }
    if (prev == NULL) {
        section->blk_list = new_;
    } else {
        prev->next = new_;
    }

    T_D("exit, new_/resized block");
    return new_;
}

/* Create/resize configuration block, returns NULL on error */
void *conf_sec_create(conf_sec_t sec, conf_blk_id_t id, ulong size)
{
    conf_blk_t *blk;

    if (!conf_sec_valid(sec)) {
        return NULL;
    }

    CONF_CRIT_ENTER();
    blk = conf_create_(sec, id, size);
    CONF_CRIT_EXIT();

    return (blk == NULL ? NULL : blk->data);
}

/* Open configuration block for read/write, returns NULL on error */
void *conf_sec_open(conf_sec_t sec, conf_blk_id_t id, ulong *size)
{
    conf_blk_t *blk;

    if (!conf_sec_valid(sec)) {
        return NULL;
    }

    CONF_CRIT_ENTER();
    T_D("enter, section: %s, id: %d", conf.section[sec].name, id);
    blk = conf_blk_lookup(sec, id);
    CONF_CRIT_EXIT();
    if (blk != NULL && size != NULL) {
        *size = blk->hdr.size;
    }
    T_D("exit, block %sfound", blk == NULL ? "not " : "");
    return (blk == NULL ? NULL : blk->data);
}

/* Close configuration block */
void conf_sec_close(conf_sec_t sec, conf_blk_id_t id)
{
    conf_blk_t     *blk;
    conf_section_t *section;

    if (!conf_sec_valid(sec)) {
        return;
    }

    CONF_CRIT_ENTER();
    section = &conf.section[sec];
    T_D("enter, section: %s, id: %d", section->name, id);

    /* Search for block in RAM */
    if (conf.conf.change_detect && (blk = conf_blk_lookup(sec, id)) != NULL) {
        /* Block exists, check if CRC has changed */
        conf_blk_crc_update(section, blk);
    }

    CONF_CRIT_EXIT();
    T_D("exit");
}

/* Get configuration section information */
void conf_sec_get(conf_sec_t sec, conf_sec_info_t *info)
{
    conf_section_t *section;

    if (!conf_sec_valid(sec)) {
        return;
    }

    CONF_CRIT_ENTER();
    section = &conf.section[sec];
    T_D("enter, section: %s", section->name);
    info->save_count = section->save_count;
    info->flash_size = section->flash_size;
    CONF_CRIT_EXIT();

    T_D("exit");
}

/* Renew configuration section */
void conf_sec_renew(conf_sec_t sec)
{
    conf_section_t *section;

    if (!conf_sec_valid(sec)) {
        return;
    }

    CONF_CRIT_ENTER();
    section = &conf.section[sec];
    T_D("enter, section: %s", section->name);
    /* Change section save count and force save */
    conf_sec_changed(section);
    section->timer_started = 0;
    section->save_count = 0;
    CONF_CRIT_EXIT();

    T_D("exit");
}

/* Create/resize local section configuration block, returns NULL on error */
void *conf_create(conf_blk_id_t id, ulong size)
{
    return conf_sec_create(CONF_SEC_LOCAL, id, size);
}

/* Open local section configuration block for read/write, returns NULL on error */
void *conf_open(conf_blk_id_t id, ulong *size)
{
    return conf_sec_open(CONF_SEC_LOCAL, id, size);
}

/* Close local section configuration block */
void conf_close(conf_blk_id_t id)
{
    conf_sec_close(CONF_SEC_LOCAL, id);
}

/* Force immediate configuration flushing to flash (iff pending) */
void conf_flush(void)
{
    vtss_flag_setbits(&control_flags, CONF_THREAD_FLAG_FLUSH);
}

/* Wait untill configuration saved and flush is cleared */
void conf_wait_flush(void)
{
    T_D("enter, set flush");
    vtss_flag_setbits(&control_flags, CONF_THREAD_FLAG_FLUSH);
    for (;;) {
        BOOL change_pending = FALSE;
        conf_sec_t sec;
        conf_section_t *section;
        vtss_flag_value_t flags;
        CONF_CRIT_ENTER();
        for (sec = CONF_SEC_LOCAL; sec < CONF_SEC_CNT; sec++) {
            section = &conf.section[sec];
            if (section->changed) {
                T_D("Change pending in section %s", section->name);
                change_pending = TRUE;
                break;
            }
        }
        flags = vtss_flag_peek(&control_flags);
        CONF_CRIT_EXIT();
        if (change_pending || (flags & CONF_THREAD_FLAG_FLUSH)) {
            T_D("Wait for flush - change_pending %d - flags 0x%x", change_pending, flags);
            VTSS_OS_MSLEEP(400);
        } else {
            T_I("Config flushed");
            break;
        }
    }
    T_D("exit");
}

static ulong conf_hdr_version(conf_sec_t sec)
{
    return (conf_sec_valid(sec) ?
            (sec == CONF_SEC_LOCAL ? CONF_HDR_VERSION_LOCAL : CONF_HDR_VERSION_GLOBAL) : 0);
}

/* Read section from Flash or message buffer */
static void conf_sec_read(conf_sec_t sec, uchar *ram_base, size_t size)
{
    conf_section_t *section;
    conf_hdr_t     hdr;
    conf_blk_hdr_t blk_hdr;
    conf_blk_t     *blk;
    BOOL           flash;
    u32            version;
    unsigned long  data_size, max_size;
    uchar          *p, *data;
    int            rc = Z_OK;
    vtss_flashaddr_t flash_base = 0;

    if (!conf_sec_valid(sec)) {
        return;
    }
    section = &conf.section[sec];
    T_D("enter, section: %s, ram_base: %p, size: " VPRIz, section->name, ram_base, size);

    /* Get Flash base address and size */
    if ((flash = (ram_base == NULL))) {
        flash_base = conf_flash_section_base(sec, &size);
        T_D("flash, using base: 0x%p, size: " VPRIz, (void *)flash_base, size);
        /* Read header */
        if (control_flash_read(flash_base, &hdr, sizeof(hdr)) != VTSS_FLASH_ERR_OK) {
            return;
        }
    } else {
        /* Copy header */
        memcpy(&hdr, ram_base, sizeof(hdr));
    }

    /* Check cookie */
    if (hdr.cookie != CONF_HDR_COOKIE_UNCOMP && hdr.cookie != CONF_HDR_COOKIE_COMP) {
        T_W("illegal cookie[%d] 0x" VPRIFlx("08"), sec, hdr.cookie);
        if (flash) {
            conf_sec_changed(section);    /* Force flash update */
        }
        return;
    }

    /* Check version */
    version = conf_hdr_version(sec);
    if (hdr.version != version) {
        T_W("illegal version[%d]: 0x" VPRIFlx("08")", expected: 0x%08x",
            sec, hdr.version, version);
        return;
    }

    /* Check length */
    max_size = CONF_SIZE_MAX;
    if (hdr.size > max_size) {
        T_W("illegal size: " VPRIlu", max: %lu", hdr.size, max_size);
        return;
    }

    /* Allocate data buffer */
    if ((VTSS_MALLOC_CAST(data, max_size)) == NULL) {
        T_E("malloc failed, size: %lu", max_size);
        return;
    }
    section->save_count = hdr.save_count;

    if (flash) {
        if ((VTSS_MALLOC_CAST(ram_base, hdr.size)) == NULL) {
            T_E("malloc failed, size: " VPRIlu, hdr.size);
            VTSS_FREE(data);
            return;
        }
        if (control_flash_read(flash_base, ram_base, hdr.size) != VTSS_FLASH_ERR_OK) {
            VTSS_FREE(ram_base);
            VTSS_FREE(data);
            return;
        }
    }

    /* Read data */
    section->crc = vtss_crc32(ram_base + sizeof(hdr), hdr.size - sizeof(hdr));
    p = (ram_base + sizeof(hdr));
    data_size = (hdr.size - sizeof(hdr));
    if (hdr.cookie == CONF_HDR_COOKIE_UNCOMP) {
        /* Uncompressed data */
        T_I("uncompressed data, %lu bytes", data_size);
        memcpy(data, p, data_size);
    } else {
        /* Compressed data */
        T_I("compressed data, %lu bytes", data_size);
        if (flash) {
            section->flash_size = hdr.size;
        } else {
            CONF_CRIT_EXIT();    /* Leave critical region while uncompressing */
        }
        rc = uncompress(data, &max_size, p, data_size);
        if (!flash) {
            CONF_CRIT_ENTER();
        }
        data_size = max_size;
    }

    /* If uncompress failed, we return here after unlocking flash */
    if (rc != Z_OK) {
        T_E("uncompress failed, rc: %d", rc);
        VTSS_FREE(data);
        if (flash) {
            VTSS_FREE(ram_base);
        }
        return;
    }

    /* Copy block data */
    p = data;
    while (p < (data + data_size)) {
        memcpy(&blk_hdr, p, sizeof(blk_hdr));
        if ((blk = conf_create_(sec, blk_hdr.id, blk_hdr.size)) != NULL) {
            memcpy(blk->data, p + sizeof(blk_hdr), blk_hdr.size);
            blk->crc = vtss_crc32((const unsigned char *)blk->data, blk->hdr.size);
        }
        p += (sizeof(blk_hdr) + blk_hdr.size);
    }
    VTSS_FREE(data);
    if (flash) {
        VTSS_FREE(ram_base);
    }

    T_D("exit");
}

static conf_msg_conf_set_req_t *conf_msg_build(conf_sec_t sec, ulong *msg_size)
{
    conf_section_t          *section;
    conf_hdr_t              hdr;
    conf_blk_t              *blk;
    uLong                   data_size, size;
    uchar                   *p, *tmp;
    conf_msg_conf_set_req_t *msg;
    int                     rc;

    /* Calculate data size */
    section = &conf.section[sec];
    data_size = 0;
    for (blk = section->blk_list; blk != NULL; blk = blk->next) {
        data_size += (sizeof(blk->hdr) + blk->hdr.size);
    }

    if (data_size > CONF_SIZE_MAX) { /* Warn about upcoming read problems */
        T_E("Configuration total: %lu bytes, read linit %d", data_size, CONF_SIZE_MAX);
    }

    /* Allocate message buffer */
    size = (sizeof(*msg) + sizeof(hdr) + data_size);
    /* Allow for buffer growth (can happen for small buffer sizes) */
#define COMPRESS_OVERHEAD 1024
    if ((VTSS_MALLOC_CAST(msg, size + COMPRESS_OVERHEAD)) == NULL) {
        T_E("malloc failed, size: %lu", size);
        return NULL;
    }

    if (data_size == 0) {
        /* No data blocks, save uncompressed */
        hdr.cookie = CONF_HDR_COOKIE_UNCOMP;
        size = 0;
    } else {
        /* Save compressed */
        hdr.cookie = CONF_HDR_COOKIE_COMP;

        /* Allocate temporary buffer for uncompressed data */
        if ((VTSS_MALLOC_CAST(tmp, data_size)) == NULL) {
            T_E("malloc failed, size: %lu", data_size);
            VTSS_FREE(msg);
            return NULL;
        }

        /* Copy blocks */
        p = tmp;
        for (blk = section->blk_list; blk != NULL; blk = blk->next) {
            size = (sizeof(blk->hdr) + blk->hdr.size);
            memcpy(p, &blk->hdr, size);
            p += size;
        }

        /* Compress data */
        size = data_size + COMPRESS_OVERHEAD; /* Output buffer length */
        CONF_CRIT_EXIT(); /* Leave critical region while compressing */
        rc = compress(&msg->data[sizeof(hdr)], &size, tmp, data_size);
        CONF_CRIT_ENTER();
        VTSS_FREE(tmp);
        if (rc != Z_OK) {
            T_E("compress failed, rc: %d", rc);
            VTSS_FREE(msg);
            return NULL;
        }
    }

    /* Fill out header */
    msg->msg_id = CONF_MSG_CONF_SET_REQ;
    hdr.size = (size + sizeof(hdr));
    hdr.save_count = section->save_count;
    hdr.version = conf_hdr_version(sec);
    memcpy(&msg->data[0], &hdr, sizeof(hdr));

    *msg_size = hdr.size;
    return msg;
}

static void conf_flash_save(conf_sec_t sec)
{
    conf_msg_conf_set_req_t *msg;
    uchar                   *p;
    vtss_flashaddr_t         base;
    size_t                  size;
    ulong                   msg_size = 0, hdr_len = sizeof(conf_hdr_t);

    T_I("enter, sec: %d", sec);

    /* Allocate and build message */
    if ((msg = conf_msg_build(sec, &msg_size)) != NULL) {
        /* Update section CRC */
        base = conf_flash_section_base(sec, &size);
        if (msg_size > size) {
            T_I("not saving, flash size exceeded, msg_size: " VPRIlu", size: " VPRIz, msg_size, size);
        } else {
            size = (msg_size - hdr_len);
            p = &msg->data[0];
            conf.section[sec].crc = vtss_crc32(p + hdr_len, size);
            if (conf.conf.flash_save) {
                /* Leave critical region and write to Flash (header is written last) */
                CONF_CRIT_EXIT();
                if (conf_flash_write(base + hdr_len, p + hdr_len, size, 1, size) == 0) {
                    conf_flash_write(base, p, hdr_len, 0, 0);
                }
                CONF_CRIT_ENTER();
            } else {
                /* Flash save is disabled (but section CRC has been updated) */
                T_I("no flash save");
            }
        }
        VTSS_FREE(msg);
    }

    T_I("exit, size: " VPRIlu, msg_size);
}

/****************************************************************************/
/*  Stack messages                                                          */
/****************************************************************************/

/* Receive message */
static BOOL conf_msg_rx(void *contxt, const void *const rx_msg, const size_t len,
                        const vtss_module_id_t modid, u32 id)
{
    conf_msg_conf_set_req_t *msg;
    conf_hdr_t              hdr;
    conf_sec_t              sec;
    conf_section_t          *section;
    ulong                   save_count;

    T_D("id: %d, len: %zu", id, len);

    T_D_HEX((const uchar *)rx_msg, 64);

    /* Check that we are a secondary switch */
    if (msg_switch_is_primary()) {
        T_W("Primary switch");
        return TRUE;
    }

    /* Check message ID */
    msg = (conf_msg_conf_set_req_t *)rx_msg;
    if (msg->msg_id != CONF_MSG_CONF_SET_REQ) {
        T_W("illegal msg_id: %d", msg->msg_id);
        return TRUE;
    }

    /* Check message length */
    memcpy(&hdr, &msg->data[0], sizeof(hdr));
    if ((sizeof(*msg) + hdr.size) != len) {
        T_W("length mismatch, len: %zu, hdr.size: " VPRIlu, len, hdr.size);
        return TRUE;
    }

    /* Compare with flash CRC to avoid unnecessary write operations */
    sec = CONF_SEC_GLOBAL;
    section = &conf.section[sec];
    if (section->crc == vtss_crc32(&msg->data[sizeof(hdr)], hdr.size - sizeof(hdr))) {
        T_D("section not changed");
        return TRUE;
    }

    /* Read blocks into global section and force update */
    CONF_CRIT_ENTER();
    save_count = section->save_count;
    conf_sec_read(sec, &msg->data[0], hdr.size);
    section->save_count = save_count;
    conf_sec_changed(section);
    CONF_CRIT_EXIT();

    return TRUE;
}

/* Register for messages */
static mesa_rc conf_stack_register(void)
{
    msg_rx_filter_t filter;

    memset(&filter, 0, sizeof(filter));
    filter.cb = conf_msg_rx;
    filter.modid = VTSS_MODULE_ID_CONF;
    return msg_rx_filter_register(&filter);
}

/* Release message buffer */
static void conf_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
    T_D("enter");

    CONF_CRIT_ENTER();
    if (conf.msg_tx_count == 0) {
        T_E("buffer already free");
    } else {
        conf.msg_tx_count--;
        if (conf.msg_tx_count == 0) {
            VTSS_FREE(msg);
        }
    }
    CONF_CRIT_EXIT();

    T_D("exit");
}

/* Copy global section to secondary switches */
static BOOL conf_stack_copy(void)
{
    vtss_isid_t             isid;
    ulong                   msg_size;
    conf_msg_conf_set_req_t *msg;

    T_I("enter");

    if (!conf.conf.stack_copy) {
        T_I("exit, no stack copy");
        return 1;
    }

    /* Only copy if primary switch */
    if (!msg_switch_is_primary()) {
        T_D("not primary switch");
        return 1;
    }

    /* Only copy if at least one secondary switch exists */
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (conf.isid_copy[isid - VTSS_ISID_START] &&
            msg_switch_exists(isid) && !msg_switch_is_local(isid)) {
            break;
        }
    }
    if (isid == VTSS_ISID_END) {
        T_D("no swcondary switches");
        return 1;
    }

    /* Only copy if no messages are currently being sent */
    if (conf.msg_tx_count) {
        T_D("buffer not ready");
        return 0;
    }

    /* Allocate and build message */
    if ((msg = conf_msg_build(CONF_SEC_GLOBAL, &msg_size)) == NULL) {
        return 1;
    }
    msg_size += sizeof(*msg);

    /* Send copy to all managed secondary switches */
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (conf.isid_copy[isid - VTSS_ISID_START] &&
            msg_switch_exists(isid) && !msg_switch_is_local(isid)) {
            T_I("Tx to isid %d, size: " VPRIlu, isid, msg_size);
            conf.msg_tx_count++;
            msg_tx_adv(NULL, conf_msg_tx_done, MSG_TX_OPT_DONT_FREE,
                       VTSS_MODULE_ID_CONF, isid, msg, msg_size);
        }
    }

    /* Free buffer if no secondary switches were found */
    if (!conf.msg_tx_count) {
        T_D("no secondary found");
        VTSS_FREE(msg);
    }

    T_I("exit");

    return 1;
}

/* Start application configuration */
static void conf_appl_start(void)
{
    conf_sec_t     sec;
    conf_section_t *section;

    T_D("enter");

    /* Read all sections from Flash */
    for (sec = CONF_SEC_LOCAL; sec < CONF_SEC_CNT; sec++) {
        section = &conf.section[sec];
        section->blk_list = NULL;
        section->name = (sec == CONF_SEC_LOCAL ? "Local" : "Global");
        conf_sec_read(sec, NULL, 0);
    }

    T_D("exit");
}

/****************************************************************************/
/*  Initialization                                                          */
/****************************************************************************/

static void conf_thread(vtss_addrword_t data)
{
    conf_sec_t              sec;
    conf_section_t          *section;
    int                     change_count;
    BOOL                    global_changed = 0;
    BOOL                    reset_locked = 0;
    vtss_isid_t             isid;
    ulong                   timer;

    /* Initialize stack message interface */
    conf.msg_tx_count = 0;
    conf_stack_register();

    for (;;) {
        BOOL do_flush;
        vtss_tick_count_t wakeup = vtss_current_time() + VTSS_OS_MSEC2TICK(100);
        vtss_flag_value_t flags;
        do_flush = 0;
        while ((flags = vtss_flag_timed_wait(&control_flags, 0xffff, VTSS_FLAG_WAITMODE_OR_CLR, wakeup))) {
            if (flags & CONF_THREAD_FLAG_COMMIT) {
                if (!reset_locked) {
                    T_I("locking reset");
                    control_system_flash_lock();
                    reset_locked = 1;
                }
            }
            if (flags & CONF_THREAD_FLAG_FLUSH) {
                T_I("Forced flush");
                do_flush = 1;
                break;          /* Fast forward */
            }
        }

        change_count = 0;
        CONF_CRIT_ENTER();

        /* Check if board configuration must be saved */
        if (conf.board_changed) {
            conf.board_changed = 0;
            conf_mgmt_board_set_(&conf.board);
        }

        for (sec = CONF_SEC_LOCAL; sec < CONF_SEC_CNT; sec++) {
            section = &conf.section[sec];
            if (!section->changed && !section->copy) { /* Skip unchanged sections */
                continue;
            }

            change_count++;
            if (do_flush || section->timer_started) {
                if (do_flush || VTSS_MTIMER_TIMEOUT(&section->mtimer)) {
                    section->timer_started = 0;
                    section->copy = 0;
                    if (sec == CONF_SEC_GLOBAL) {
                        global_changed = 1;
                    }
                    if (section->changed) {
                        T_I("section %s %s, saving to flash",
                            section->name, do_flush ? "flush" : "timeout");
                        section->save_count++;
                        if (!reset_locked) {
                            /* If reset not already locked, do it now before updating Flash */
                            T_I("locking reset");
                            control_system_flash_lock();
                            reset_locked = 1;
                        }
                        section->changed = 0;
                        conf_flash_save(sec);

                        /* If global section changed, copy to all switches */
                        if (sec == CONF_SEC_GLOBAL)
                            for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
                                conf.isid_copy[isid - VTSS_ISID_START] = 1;
                            }
                    }
                }
            } else {
                /* To protect the Flash, the timer grows with the number of save operations */
                timer = (1 << (section->save_count / 10000));
                VTSS_MTIMER_START(&section->mtimer, 1000 * timer);
                section->timer_started = 1;
                T_I("section %s changed, starting timer of " VPRIlu" seconds", section->name, timer);
            }
        } /* Section loop */

        /* Copy global section, if changed */
        if (global_changed && conf_stack_copy()) {
            global_changed = 0;
            for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
                conf.isid_copy[isid - VTSS_ISID_START] = 0;
            }
        }

        if (change_count == 0 && reset_locked && conf.msg_tx_count == 0) {
            T_I("unlocking reset");
            control_system_flash_unlock();
            reset_locked = 0;
        }
        CONF_CRIT_EXIT();
    } /* Forever loop */

}

extern "C" int conf_icli_cmd_register();

mesa_rc conf_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

    switch (data->cmd) {
    case INIT_CMD_EARLY_INIT:
        /* Update our own static variables with info about where the configuration is stored. */
        conf_flash_get_section_info();

        /* Initialize configuration */
        conf.conf.stack_copy = 1;
        conf.conf.flash_save = 1;
        conf.conf.change_detect = 1;

        critd_init(&conf.crit, "conf", VTSS_MODULE_ID_CONF, CRITD_TYPE_MUTEX);
        CONF_CRIT_ENTER();

        /* The conf critd's maximum lock time should be higher than what it takes
         * to update the firmware image to flash, and on some old flashes, this may
         * take up to a hundred seconds, so we increase this particular semaphore's
         * lock time to 5 minutes.
         */
        if (conf.crit.max_lock_time < 300) {
            /* Only change it if we wouldn't set the new lock time to something
               smaller than the default. */
            conf.crit.max_lock_time = 300;
        }

        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            conf.isid_copy[isid - VTSS_ISID_START] = 0;
        }

        /* Read in config data in order to serve configuration early */
        conf_board_start();

        // TBD Palle to check it the below statement is true
        // Only applicable for iCPU
        if (!misc_cpu_is_external()) {
            conf_appl_start();
        }

        vtss_flag_init(&control_flags);

        CONF_CRIT_EXIT();       /* NB: config data *is* ready, so unlock now */
        break;

    case INIT_CMD_INIT:
        conf_icli_cmd_register();
        break;

    case INIT_CMD_START:
        vtss_thread_create(VTSS_THREAD_PRIO_BELOW_NORMAL,
                           conf_thread,
                           0,
                           "Configuration",
                           nullptr,
                           0,
                           &conf.thread_handle,
                           &conf.thread_block);
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_I("ICFG_LOADING_POST");
        if (VTSS_ISID_LEGAL(isid) && !msg_switch_is_local(isid)) {
            /* Start configuration copy */
            CONF_CRIT_ENTER();
            conf.isid_copy[isid - VTSS_ISID_START] = 1;
            conf.section[CONF_SEC_GLOBAL].copy = 1;
            CONF_CRIT_EXIT();
        }

        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

