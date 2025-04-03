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

#include <sys/mount.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <linux/loop.h>
#include <mtd/mtd-user.h>
#include <dirent.h>
#include <mntent.h>

#include "main.h"
#include "firmware.h"
#include "msg_api.h"
#include "led_api.h"
#include "conf_api.h"
#include "vtss_mtd_api.hxx"
#include "vtss_uboot.hxx"
#include "firmware_ubi.hxx"
#include "version.h"
#include "lock.hxx"

#include "firmware_mfi_info.hxx"
#include "standalone_api.h"

#include <vtss/basics/algorithm.hxx>
#include <vtss/basics/parse_group.hxx>
#include <vtss/basics/parser_impl.hxx>
#include <vtss/basics/stream.hxx>
#include <vtss/basics/string.hxx>
#include <vtss/basics/vector.hxx>

// NB: Linux is over-committing, so a large number is OK....
size_t firmware_max_download = (40 * 1024 * 1024);

/* Thread variables */
static vtss_handle_t firmware_thread_handle;
static vtss_thread_t firmware_thread_block;

/* firmware upload thread variables */
#define FIRMWARE_UPLOAD_THREAD_FLAG_UPLOAD          VTSS_BIT(1)    // upload Image

static vtss_handle_t g_firmware_upload_thread_handle;
static vtss_thread_t g_firmware_upload_thread_block;

vtss_appl_firmware_control_image_upload_t g_image_upload;
static vtss_appl_firmware_status_image_t         g_image_status[VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE + 1];
static vtss_flag_t                               g_firmware_upload_thread_flag;

/* "simple" lock functions */
#define SIMPLE_LOCK()   vtss_global_lock(__FILE__, __LINE__)
#define SIMPLE_UNLOCK() vtss_global_unlock(__FILE__, __LINE__)

extern mesa_rc check_bootloader(cli_iolayer_t *io, const u8 *buffer, size_t length);
extern mesa_rc firmware_icli_load_image(cli_iolayer_t *io,
                                     const char *mtd_name,
                                     const char *url,
                                     mesa_rc (*checker)(cli_iolayer_t *, const u8 *, size_t),
                                     vtss::remote_file_options_t &transfer_options);
extern critd_t mepa_crit;

const char *const fw_fis_linux = "linux";
const char *const fw_fis_linux_bk = "linux.bk";
const char *const fw_fis_boot0 = "Boot0";
const char *const fw_fis_boot1 = "Boot1";
char fw_fis_mmc1[128]; // Will be filled out when partition Boot0 is found in mmc
char fw_fis_mmc2[128]; // Will be filled out when partition Boot1 is found in mmc

const char *const fw_nor = "nor";
const char *const fw_nand = "nand";
const char *const fw_emmc = "emmc";
const char *const fw_nor_nand = "nor_nand";

const char *fw_fis_active = nullptr;
const char *fw_fis_primary = nullptr;
const char *fw_fis_backup = nullptr;

const char *fw_fis_layout = nullptr;

static vtss::Lock version_information_available;

static mesa_rc vtss_appl_firmware_status_image_entry_init(u32 image_id,
                                                          vtss_appl_firmware_status_image_t *const image_entry);

size_t firmware_section_size(const char *name)
{
    vtss_mtd_t mtd;

    if (vtss_mtd_open(&mtd, name) == VTSS_RC_OK) {
        vtss_mtd_close(&mtd);
        return mtd.info.size;
    }
    return 0;
}


/******************************* For handling the FIS directory **************************************/

/*
 * Close and free after updating the fis table
 */
mesa_rc vtss_fis_close(vtss_fis_mtd_t *fis_mtd)
{
    /* free memory space */
    VTSS_ASSERT(fis_mtd->fis_data);
    VTSS_FREE(fis_mtd->fis_data);
    /* close file */
    return vtss_mtd_close(&(fis_mtd->mtd));
}

/*
 * Acquire the fis descriptor of the fis table in the flash
 * input : "FIS_directory" or "Redundant_FIS"
 * output:  fis descriptor
 */
mesa_rc vtss_fis_open(vtss_fis_mtd_t *fis_mtd, const char *name)
{
    if( vtss_mtd_open(&(fis_mtd->mtd), name) != VTSS_RC_OK ) {
        T_D("Not found: %s\n", name);
        goto done;
    }
    T_D("%s size=0x%08x, erasesize=0x%08x, fd=%d", name, fis_mtd->mtd.info.size, fis_mtd->mtd.info.erasesize, fis_mtd->mtd.fd);

    fis_mtd->name = name;
    fis_mtd->fis_size = fis_mtd->mtd.info.size;

    if ((fis_mtd->fis_data = (u8*) VTSS_MALLOC(fis_mtd->fis_size)) == NULL) {
        T_D("malloc failed! [%s]", strerror(errno));
        goto done;
    }
    memset(fis_mtd->fis_data, 0, fis_mtd->fis_size);
    if (pread(fis_mtd->mtd.fd, fis_mtd->fis_data, fis_mtd->fis_size, 0) == -1) {
        T_D("pread from %s failed! [%s]", name, strerror(errno));
        goto done;
    }

    // Enumeration limits
    fis_mtd->fis_start = (vtss_fis_desc_t *) fis_mtd->fis_data;
    fis_mtd->fis_end   = (vtss_fis_desc_t *) (fis_mtd->fis_data + fis_mtd->fis_size);
    return VTSS_RC_OK;

done:
    vtss_mtd_close(&fis_mtd->mtd);
    return VTSS_RC_ERROR;
}

static mesa_rc firmware_fis_read(vtss_fis_mtd_t *fis_dir,
                                 vtss_fis_mtd_t *fis_bak,
                                 vtss_fis_mtd_t **act_fis,
                                 vtss_fis_mtd_t **alt_fis)
{
    *act_fis = NULL;  /* active fis table */
    *alt_fis = NULL;  /* alternative fis table */

    memset(fis_dir, 0x0, sizeof(vtss_fis_mtd_t));
    memset(fis_bak, 0x0, sizeof(vtss_fis_mtd_t));
    if ((vtss_fis_open(fis_dir, "FIS_directory")) != VTSS_RC_OK) {
        T_E("open FIS_directory failed! [%s]\n", strerror(errno));
        goto done;
    }

    if ((vtss_fis_open(fis_bak, "Redundant_FIS")) != VTSS_RC_OK) {
        T_E("open Redundant_FIS failed! [%s]\n", strerror(errno));
        goto done;
    }

    if (!fis_dir->fis_start || !fis_bak->fis_start) {
        T_E("descriptor pointer is NULL!\n");
        goto done;
    }

    if (fis_dir->fis_start->u.valid_info.valid_flag[0] != VTSS_REDBOOT_RFIS_VALID &&
        fis_bak->fis_start->u.valid_info.valid_flag[0] != VTSS_REDBOOT_RFIS_VALID) {
        T_E("Neither flash directories are valid, unable to update\n");
        goto done;
    }

    /* Detect which fis table we are currently running based on version_count */
    if (fis_dir->fis_start->u.valid_info.valid_flag[0] == VTSS_REDBOOT_RFIS_VALID &&
        fis_bak->fis_start->u.valid_info.valid_flag[0] == VTSS_REDBOOT_RFIS_VALID) {
        if (fis_dir->fis_start->u.valid_info.version_count > fis_bak->fis_start->u.valid_info.version_count) {
            /*  "FIS_directory" is currently active */
            T_D("Two good FIS - Primary is good (%d, %d)", fis_dir->fis_start->u.valid_info.version_count, fis_bak->fis_start->u.valid_info.version_count);
            *act_fis = fis_dir;
            *alt_fis = fis_bak;
        } else {
            /* "Redundant_FIS" is currently active */
            T_D("Two good FIS - Redundant is good (%d, %d)", fis_dir->fis_start->u.valid_info.version_count, fis_bak->fis_start->u.valid_info.version_count);
            *act_fis = fis_bak;
            *alt_fis = fis_dir;
        }
    } else {
        // Both Fis are bot valid, but one is
        if (fis_dir->fis_start->u.valid_info.valid_flag[0] == VTSS_REDBOOT_RFIS_VALID) {
            /*  "FIS_directory" is currently active */
            T_D("One good FIS - primary");
            *act_fis = fis_dir;
            *alt_fis = fis_bak;
        } else {
            /* "Redundant_FIS" is currently active */
            T_D("One good FIS - redundant");
            *act_fis = fis_bak;
            *alt_fis = fis_dir;
        }
    }

    return VTSS_RC_OK;

done:
    /* cleanup */
    vtss_fis_close(fis_dir);
    vtss_fis_close(fis_bak);
    return VTSS_RC_ERROR;
}

static mesa_rc firmware_fis_update(const vtss_fis_mtd_t *act_fis,
                                   const vtss_fis_mtd_t *alt_fis)
{
    mesa_rc rc = VTSS_RC_ERROR;    // Assume the worst

    /* Activating the alternative fis table by increasing version counter */
    act_fis->fis_start->u.valid_info.version_count += 0x1;

    /* MTD device: erase before writing */
    T_D("Erasing alt fis table %s...", alt_fis->name);
    if ( (vtss_mtd_erase(&alt_fis->mtd, alt_fis->fis_size)) == VTSS_RC_OK ) {
        T_D(" done!");
        T_D("Programming alt fis table...");
        act_fis->fis_start->u.valid_info.valid_flag[0] =
                act_fis->fis_start->u.valid_info.valid_flag[1] =
                VTSS_REDBOOT_RFIS_IN_PROGRESS;
        if ( (vtss_mtd_program(&alt_fis->mtd, (const u8*)act_fis->fis_start, act_fis->fis_size)) == VTSS_RC_OK ) {
            off_t flagpos = offsetof(vtss_fis_desc_t, u.valid_info.valid_flag[0]);
            u8 flags[2] = { VTSS_REDBOOT_RFIS_VALID, VTSS_REDBOOT_RFIS_VALID };
            T_D("Wrote in-progress FIS");
            // Now update valid bits - essentially clearing bits atomically (UNERASED)
            if ( (vtss_mtd_program_pos(&alt_fis->mtd, flags, sizeof(flags), flagpos)) == VTSS_RC_OK ) {
                T_D("Wrote FIS valid flags");
                rc = VTSS_RC_OK;
            } else {
                T_D("Failure writing FIS valid flags");
            }
        } else {
            T_D("Failure writing FIS valid flags");
        }
    } else {
        T_D("Failure erasing FIS");
    }
    return rc;
}

#define FIS_ENTRY_INVALID(f) ((f)->u.name[0] == 0 || (f)->u.name[0] == '\xff')

vtss_fis_desc_t *firmware_fis_find(const vtss_fis_mtd_t *fis, const char *name)
{
    vtss_fis_desc_t *desc;
    for (desc = fis->fis_start; desc < fis->fis_end; desc++)  {
        /* Analyze the contents of the active fis table */
        if (FIS_ENTRY_INVALID(desc)) {
            T_N("%p: Bad entry %d", desc, desc->u.name[0]);
            continue;
        }
        T_N("%p: %s Looking for '%s'", desc, desc->u.name, name);
        if (strcmp(desc->u.name, name) == 0) {
            T_D("Found %s at offset %p", name, desc);
            return desc;
        }
    }
    T_D("Did not find '%s'", name);
    return NULL;
}

vtss_fis_desc_t *firmware_fis_insert(vtss_fis_desc_t *start, vtss_fis_desc_t *end)
{
    if ((start < end)) {
        if (FIS_ENTRY_INVALID(&end[-1])) {    // Assume last is invalid
            vtss_fis_desc_t *desc;
            desc = &start[1];    // Inserted after start
            memmove(desc+1, desc, ((char*)end) - ((char*)(desc+1)));
            return desc;
        }
    }
    return NULL;
}

void firmware_fis_list(const char *what, const vtss_fis_mtd_t *fis)
{
    vtss_fis_desc_t *desc;
    T_D("%s: %s - Entries %p-%p", what, fis->name, fis->fis_start, fis->fis_end);
    for (desc = fis->fis_start; desc < fis->fis_end; desc++)  {
        /* Analyze the contents of the active fis table */
        if (FIS_ENTRY_INVALID(desc)) {
            T_N("%p: %s", desc, "(invalid)");
            continue;
        }
        T_D("%p: %-16s: base 0x%08x, length %u", desc, desc->u.name, desc->flash_base, desc->size);
    }
}

mesa_rc firmware_fis_split(const char *primary, size_t primary_size,
                           const char *backup, size_t backup_size)
{
    vtss_fis_mtd_t fis_dir, fis_bak;  /* Two fis tables */
    vtss_fis_mtd_t *act_fis, *alt_fis;  /* active, backup fis table */
    mesa_rc rc;

    if ((rc = firmware_fis_read(&fis_dir, &fis_bak, &act_fis, &alt_fis)) == VTSS_RC_OK) {
        vtss_fis_desc_t *linux;
        T_D("Active: %s, Alt: %s", act_fis->name,  alt_fis->name);
        firmware_fis_list("Active", act_fis);
        if ((linux = firmware_fis_find(act_fis, "linux"))) {
            vtss_fis_desc_t *newent;
            if ((newent = firmware_fis_find(act_fis, "linux.bk")) != NULL) {
                rc = VTSS_RC_ERROR;
                T_E("Already have a %s FIS entry", newent->u.name);
            } else {
                newent = firmware_fis_insert(linux, act_fis->fis_end);
                T_D("Found linux FIS at %p, insert new @ %p", linux, newent);
                if (newent) {
                    linux->size = primary_size;
                    *newent = *linux;
                    strcpy(newent->u.name, "linux.bk");
                    newent->flash_base += primary_size;
                    linux->size = backup_size;
                    T_D("Split into %d, %d", primary_size, backup_size);
                    firmware_fis_list("Updated FIS", act_fis);
                    rc = firmware_fis_update(act_fis, alt_fis);
                }
            }
        } else {
            rc = VTSS_RC_ERROR;
            T_E("Unable to find linux FIS in %s", act_fis->name);
        }
        vtss_fis_close(&fis_dir);
        vtss_fis_close(&fis_bak);
    }

    return rc;
}

static mesa_rc firmware_redboot_resize(const char *name, size_t new_size)
{
    vtss_fis_mtd_t fis_dir, fis_bak;  /* Two fis tables */
    vtss_fis_mtd_t *act_fis, *alt_fis;  /* active, backup fis table */
    mesa_rc rc;
    if ((rc = firmware_fis_read(&fis_dir, &fis_bak, &act_fis, &alt_fis)) == VTSS_RC_OK) {
        vtss_fis_desc_t *desc;
        T_D("Active: %s, Alt: %s", act_fis->name, alt_fis->name);
        firmware_fis_list("Active", act_fis);
        if ((desc = firmware_fis_find(act_fis, name))) {
            desc->data_length = desc->size = new_size;
            firmware_fis_list("Updated", act_fis);
            rc = firmware_fis_update(act_fis, alt_fis);
        } else {
            rc = VTSS_RC_ERROR;
            T_W("Unable to find '%s' FIS in %s", name, act_fis->name);
        }
        vtss_fis_close(&fis_dir);
        vtss_fis_close(&fis_bak);
    }
    return rc;
}

bool has_part_name(FILE *fp, const char *partname)
{
    char buf[128], lookfor[128];

    snprintf(lookfor, sizeof(lookfor), "PARTNAME=%s", partname);
    while (fgets(buf, sizeof(buf), fp)) {
        buf[strlen(buf)-1] = '\0'; /* Chomp */
        if (strcmp(buf, lookfor) == 0)
            return true;
    }
    return false;
}

const char *get_mmc_device(const char *partname)
{
    static char buf[128];
    char fn[128];
    FILE *fp;
    int i;

    for (i = 0; i < 10; i++) {
        snprintf(fn , sizeof(fn), "/sys/block/mmcblk0/mmcblk0p%d/uevent", i);
        if ((fp = fopen(fn, "r"))) {
            if (has_part_name(fp, partname)) {
                snprintf(buf, sizeof(buf), "/dev/mmcblk0p%d", i);
                fclose(fp);
                return buf;
            }
            fclose(fp);
        }
    }
    return NULL;
}

struct MtdPart {
    MtdPart(std::string n, unsigned long s) : name{n}, size{s} {}
    std::string bus;
    std::string name;
    unsigned long size;
};

namespace uboot_mtd_parser {

struct MtdResTag : public vtss::parser::ParserBase {
    typedef const char val;

    bool operator()(const char *& b, const char *e) {
        if (*b == 'k' || *b == 'm') {
            _c = *b;
            ++b;
            return true;
        }
        return false;
    }

    val get() const { return _c; }

    char _c = '\0';
};

struct MtdNameTag : public vtss::parser::ParserBase {
    typedef const std::string val;

    bool operator()(const char *& b, const char *e) {
        const char *i = b;
        while (*i != ')') {
            ++i;
        }

        _s = std::string(b, i);
        b = i;

        return true;
    }

    val get() const { return _s; }

    std::string _s;
};

struct MtdTag : public vtss::parser::ParserBase {
    MtdTag() : _open("("), _close(")") {}

    bool operator()(const char *& b, const char *e) {
        if (!vtss::parser::Group(b, e, _val))
            return false;
        vtss::parser::Group(b, e, _tag);
        vtss::parser::Group(b, e, _open);
        vtss::parser::Group(b, e, _name);
        vtss::parser::Group(b, e, _close);

        return true;
    }

    MtdNameTag::val get_name() const { return _name.get(); }
    uint32_t get_val() const {
        auto res = _val.get();
        if (_tag.get() == 'k')
            res *= 0x00000400;
        if (_tag.get() == 'm')
            res *= 0x00100000;
        return res;
    }

    vtss::parser::IntUnsignedBase10<uint32_t, 1, 8> _val;
    MtdResTag _tag;
    vtss::parser::Lit _open;
    MtdNameTag _name;
    vtss::parser::Lit _close;
};

struct IsComma {
    bool operator()(char c) { return c == ','; }
};

static vtss::Vector<MtdPart> deserialize(std::string& mtdparts)
{
    vtss::Vector<MtdPart> res;
    std::string tmp(mtdparts);

    // remove first('mtdparts=spi_flash:') part which is not used
    tmp = tmp.substr(tmp.find(":") + 1);
    const char *b = &*tmp.begin();
    const char *e = tmp.c_str() + tmp.size();

    MtdTag tag;
    vtss::parser::OneOrMore<IsComma> comma;
    while (vtss::parser::Group(b, e, tag)) {
        res.emplace_back(tag.get_name(), tag.get_val());
        vtss::parser::Group(b, e, comma);
    }
    return res;
}

static std::string serialize(vtss::Vector<MtdPart>&& res)
{
    vtss::StringStream stream;
    stream << "mtdparts=spi_flash:";
    for (auto &m : res) {
        stream << m.size << "(" << m.name << "),";
    }
    // pop the last ,
    return std::string(stream.begin(), stream.end()-1);
}

}

static mesa_rc firmware_uboot_resize(const char *name, size_t new_size)
{
    T_E("PALLE: Please review this code");
    char buf[4096];
    memset(buf, '\0', 4096);
    auto out = vtss_uboot_get_env("mtdparts");
    if (!out.valid()) {
        return VTSS_RC_ERROR;
    }
    // if (vtss_uboot_get_env("mtdparts", buf, 4096) != VTSS_RC_OK)
    //     return VTSS_RC_ERROR;

    // std::string out(buf, 4096);
    vtss::Vector<MtdPart> res = uboot_mtd_parser::deserialize(out.get());
    auto it = vtss::find_if(res.begin(), res.end(), [name](auto mtd) {
                                if (strncmp(name, mtd.name.data(),
                                            vtss::max(strlen(name), mtd.name.size())) == 0)
                                    return true;
                                return false;
                            });
    if (it != res.end()) {
            it->size = new_size;
    }
    out = uboot_mtd_parser::serialize(vtss::move(res));
    if (vtss_uboot_set_env("mtdparts", out.get().data()) != VTSS_RC_OK)
        return VTSS_RC_ERROR;

    return VTSS_RC_OK;
}

mesa_rc firmware_fis_resize(const char *name, size_t new_size)
{
    if (conf_is_uboot())
        return firmware_uboot_resize(name, new_size);
    else
        return firmware_redboot_resize(name, new_size);
}

static mesa_rc firmware_swap_redboot(void)
{
    vtss_fis_mtd_t fis_dir, fis_bak;  /* Two fis tables */
    vtss_fis_mtd_t *act_fis, *alt_fis;  /* active, backup fis table */
    vtss_fis_desc_t *act_desc, *alt_desc;  /* active, alternative desc in active fis table */
    mesa_rc rc;
    if ((rc = firmware_fis_read(&fis_dir, &fis_bak, &act_fis, &alt_fis)) == VTSS_RC_OK) {
        act_desc = firmware_fis_find(act_fis, "linux");
        alt_desc = firmware_fis_find(act_fis, "linux.bk");
        /* only swap if both "linux" & "linux.bk" are available */
        if (act_desc && alt_desc) {
            /* swap name */
            vtss_fis_desc_t tmp;
            tmp = *act_desc;              // Save active
            act_desc->u = alt_desc->u;    // Act = Alt
            alt_desc->u = tmp.u;          // Alt = Act
            firmware_fis_list("Updated FIS", act_fis);
            rc = firmware_fis_update(act_fis, alt_fis);
        } else {
            T_D("Unable to swap firmware slots, did not find %s", act_desc == NULL ? "linux" : "linux.bk");
            rc = VTSS_RC_ERROR;
        }
        /* cleanup */
        vtss_fis_close(&fis_dir);
        vtss_fis_close(&fis_bak);
    }
    return rc;
}

static void swap_partitions(std::string &mtdparts)
{
    constexpr size_t linux_len = 7;
    constexpr size_t linux_bk_len = 10;
    size_t p_linux = mtdparts.find("(linux)");
    size_t p_linux_bk = mtdparts.find("(linux.bk)");
    if (p_linux < p_linux_bk) {
        mtdparts = mtdparts.replace(p_linux, linux_len, "(linux.bk)");
        p_linux_bk = mtdparts.find("(linux.bk)", p_linux + linux_len);
        mtdparts = mtdparts.replace(p_linux_bk, linux_bk_len, "(linux)");
    } else {
        mtdparts = mtdparts.replace(p_linux_bk, linux_bk_len, "(linux)");
        p_linux = mtdparts.find("(linux)", p_linux_bk + linux_bk_len);
        mtdparts = mtdparts.replace(p_linux, linux_len, "(linux.bk)");
    }
}


static mesa_rc firmware_swap_uboot_mmc(void)
{
    // The number to write to uboot environment is the last character in the device name
    // e.g mmcblk0p5 shall become 5 in uboot environment
    VTSS_RC(vtss_uboot_set_env("mmc_bak", fw_fis_primary + strlen(fw_fis_primary) - 1));
    return vtss_uboot_set_env("mmc_cur", fw_fis_backup + strlen(fw_fis_backup) - 1);
}

static mesa_rc firmware_swap_uboot_nand(void)
{
    auto nand_cur = vtss_uboot_get_env("nand_cur");
    if (!nand_cur.valid()) {
        return VTSS_RC_ERROR;
    }
    auto nand_bak = vtss_uboot_get_env("nand_bak");
    if (!nand_bak.valid()) {
        return VTSS_RC_ERROR;
    }

    if (vtss_uboot_set_env("nand_cur", nand_bak.get().c_str()) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }
    if (vtss_uboot_set_env("nand_bak", nand_cur.get().c_str()) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

static mesa_rc firmware_swap_uboot_nor(void)
{
    auto mtd_parts = vtss_uboot_get_env("mtdparts");
    if (!mtd_parts.valid()) {
         return VTSS_RC_ERROR;
    }

    swap_partitions(mtd_parts.get());

    if (vtss_uboot_set_env("mtdparts", mtd_parts.get().c_str()) != VTSS_RC_OK) {
         return VTSS_RC_ERROR;
    }

     return VTSS_RC_OK;
}

static mesa_rc firmware_swap_uboot(void)
{
    if (firmware_is_mmc()) {
        return firmware_swap_uboot_mmc();
    } else if (firmware_is_nand_only()) {
        return firmware_swap_uboot_nand();
    } else if (firmware_is_nor_only()) {
        return firmware_swap_uboot_nor();
    } else if (firmware_is_nor_nand()) {
        return firmware_swap_uboot_nor();
    } else {
        T_E("Don't know how to swap");
        return VTSS_RC_ERROR;
    }
}

mesa_rc firmware_swap_images(void)
{
    mesa_rc rc;
    mepa_lock_t lock;
    lock.function=__FUNCTION__;
    // Prevent API from running while swapping.
    // When SPI bus is busy, some temperature reading may fail, causing writing to
    // syslog which can cause a semaphore deadlock with swap
    mepa_callout_lock(&lock);
    if (conf_is_uboot()) {
        rc = firmware_swap_uboot();
    } else {
        rc = firmware_swap_redboot();
    }
    mepa_callout_unlock(&lock);
    return rc;
}

mesa_rc get_mount_point(const char *fsname, char *mt_point, int mt_point_size)
{
  struct mntent *ent;
  FILE *mounts;

  mounts = setmntent("/proc/mounts", "r");
  if (!mounts) {
      return VTSS_RC_ERROR;
  }
  while (NULL != (ent = getmntent(mounts))) {
      T_D("%s mounted on %s", ent->mnt_fsname, ent->mnt_dir);
      if (strcmp(ent->mnt_fsname, fsname) == 0) {
          strncpy(mt_point, ent->mnt_dir, mt_point_size);
          endmntent(mounts);
          return VTSS_RC_OK;
      }
  }
  endmntent(mounts);
  return VTSS_RC_ERROR;
}

mesa_rc get_mount_point_source(const char *mt_point, char *fsname, int fsname_size)
{
  struct mntent *ent;
  FILE *mounts;

  mounts = setmntent("/proc/mounts", "r");
  if (!mounts) {
      return VTSS_RC_ERROR;
  }
  while (NULL != (ent = getmntent(mounts))) {
      T_N("%s mounted on %s", ent->mnt_fsname, ent->mnt_dir);
      if (strcmp(ent->mnt_dir, mt_point) == 0) {
          strncpy(fsname, ent->mnt_fsname, fsname_size);
          endmntent(mounts);
          return VTSS_RC_OK;
      }
  }
  endmntent(mounts);
  return VTSS_RC_ERROR;
}

/* Read the contents from linux kernel cmdline */
const char* firmware_cmdline_get()
{
    static char str[1024] = "";
    if (str[0] == 0) {
        FILE* fp;
        if ( (fp = fopen ("/proc/cmdline", "r")) == NULL) {
            T_D("fopen /proc/cmdline FAILED! [%s]\n", strerror(errno));
            return nullptr;
        }
        if ( (fgets(str, sizeof(str), fp)) == NULL) {
            T_D("fgets FAILED! [%s]\n", strerror(errno));
            fclose(fp);
            return nullptr;
        }
        /* cleanup */
        fclose(fp);
    }
    return str;
}

const char *firmware_fis_prim_emmc(void)
{
    auto emcc_cur = vtss_uboot_get_env("mmc_cur");
    if (emcc_cur.valid() && emcc_cur.get().c_str()[0] == fw_fis_mmc1[strlen(fw_fis_mmc1)-1]) {
        return fw_fis_mmc1;
    } else if (emcc_cur.valid() && emcc_cur.get().c_str()[0] == fw_fis_mmc2[strlen(fw_fis_mmc2)-1]) {
        return fw_fis_mmc2;
    }
    return nullptr;
}

const char *firmware_fis_bak_emmc(void)
{
    auto emcc_cur = vtss_uboot_get_env("mmc_cur");
    if (emcc_cur.valid() && emcc_cur.get().c_str()[0] == fw_fis_mmc1[strlen(fw_fis_mmc1)-1]) {
        return fw_fis_mmc2;
    } else if (emcc_cur.valid() && emcc_cur.get().c_str()[0] == fw_fis_mmc2[strlen(fw_fis_mmc2)-1]) {
        return fw_fis_mmc1;
    }
    return nullptr;
}

const char *firmware_fis_prim_nand(void)
{
    auto nand_cur = vtss_uboot_get_env("nand_cur");
    if (nand_cur.valid() && nand_cur.get().c_str()[0] == '0') {
        return fw_fis_boot0;
    } else if (nand_cur.valid() && nand_cur.get().c_str()[0] == '1') {
        return fw_fis_boot1;
    }
    return nullptr;
}

const char *firmware_fis_bak_nand(void)
{
    auto nand_cur = vtss_uboot_get_env("nand_cur");
    if (nand_cur.valid() && nand_cur.get().c_str()[0] == '0') {
        return fw_fis_boot1;
    } else if (nand_cur.valid() && nand_cur.get().c_str()[0] == '1') {
        return fw_fis_boot0;
    }
    return nullptr;
}

void firmware_determine_memory_type()
{
    const char *str = firmware_cmdline_get();
    char needle[256];

    // Determine whether application is running from nor, nand, emmc, nor-nand
    T_D("firmware_determine_memory_type from cmdline %s", str);
    if (!str) {
        T_E("Could not get cmd line");
        return;
    }
    const char *mmcblk = get_mmc_device("Boot0");
    if (mmcblk && strstr(mmcblk, "/dev/")) {
        strncpy(fw_fis_mmc1, mmcblk+sizeof("/dev/")-1, sizeof(fw_fis_mmc1));
    }
    mmcblk = get_mmc_device("Boot1");
    if (mmcblk && strstr(mmcblk, "/dev/")) {
        strncpy(fw_fis_mmc2, mmcblk+sizeof("/dev/")-1, sizeof(fw_fis_mmc2));
    }

    sprintf(needle, "root_next=/dev/%s", fw_fis_mmc1);
    if ( strstr(str, needle) ) {
        // Software running from emmc
        T_D("Running from mmc, active image %s", fw_fis_mmc1);
        fw_fis_active = fw_fis_mmc1;
        fw_fis_primary = firmware_fis_prim_emmc();
        fw_fis_backup = firmware_fis_bak_emmc();
        fw_fis_layout = fw_emmc;
        return;
    }

    sprintf(needle, "root_next=/dev/%s", fw_fis_mmc2);
    if ( strstr(str, needle) ) {
        // Software running from emmc
        T_D("Running from mmc, active image %s", fw_fis_mmc2);
        fw_fis_active = fw_fis_mmc2;
        fw_fis_primary = firmware_fis_prim_emmc();
        fw_fis_backup = firmware_fis_bak_emmc();
        fw_fis_layout = fw_emmc;
        return;
    }

    if ( strstr(str, "(linux.bk)") && strstr(str, "fis_act=linux.bk") ) {
        // Software running from nor
        T_D("Running from nor (or nor_nand), active image %s", fw_fis_linux_bk);
        fw_fis_active = fw_fis_linux_bk;
        fw_fis_primary = fw_fis_linux;
        fw_fis_backup = fw_fis_linux_bk;
        if (vtss_mtd_rootfs_is_nor()) {
            fw_fis_layout = fw_nor;
        } else {
            fw_fis_layout = fw_nor_nand;
        }
        return;
    }

    if ( strstr(str, "(linux)") && strstr(str, "fis_act=linux") ) {
        // Software running from nor
        T_D("Running from nor (or nor_nand), active image %s", fw_fis_linux);
        fw_fis_active = fw_fis_linux;
        fw_fis_primary = fw_fis_linux;
        if ( strstr(str, "(linux.bk)") ) {
            fw_fis_backup = fw_fis_linux_bk;
        } else {
            fw_fis_backup = nullptr;
        }
        if (vtss_mtd_rootfs_is_nor()) {
            fw_fis_layout = fw_nor;
        } else {
            fw_fis_layout = fw_nor_nand;
        }
        return;
    }

    if ( strstr(str, "(Boot0)") && strstr(str, "ubi.mtd=Boot0") ) {
        // Software running from nand
        T_D("Running from nand, active image %s", fw_fis_boot0);
        fw_fis_active = fw_fis_boot0;
        fw_fis_primary = firmware_fis_prim_nand();
        fw_fis_backup = firmware_fis_bak_nand();
        fw_fis_layout = fw_nand;
        return;
    }

    if ( strstr(str, "(Boot1)") && strstr(str, "ubi.mtd=Boot1") ) {
        // Software running from nand
        T_D("Running from nand, active image %s", fw_fis_boot1);
        fw_fis_active = fw_fis_boot1;
        fw_fis_primary = firmware_fis_prim_nand();
        fw_fis_backup = firmware_fis_bak_nand();
        fw_fis_layout = fw_nand;
        return;
    }

    if ( strstr(str, "m(rootfs_data)") ) {
        // Software is running from ram, probably loaded over tftp.
        // A rootfs_data partition of fixed size is mounted which only happens when
        // running from NOR, so look in NOR for version of stored images
        T_D("Running from nor, primary image %s", fw_fis_linux);
        fw_fis_active = nullptr; // None of the installed images are active
        fw_fis_primary = fw_fis_linux;
        if ( strstr(str, "(linux.bk)") ) {
            fw_fis_backup = fw_fis_linux_bk;
        } else {
            fw_fis_backup = fw_fis_linux;
        }
        fw_fis_layout = fw_nor;
        return;
    }

    if (strstr(str, "-(rootfs_data)")) {
        // Software is running from ram, probably loaded over tftp.
        // A rootfs_data partition of variable size is mounted which only happens when
        // running from NAND, so look in NAND for version of stored images
        fw_fis_active = nullptr; // None of the installed images are active
        fw_fis_primary = firmware_fis_prim_nand();
        fw_fis_backup = firmware_fis_bak_nand();
        fw_fis_layout = fw_nand;
        T_D("Running from nand, primary image %s", fw_fis_primary);
        return;
    }

    if (strstr(str, "image=mfi")) {
        // Software is running from ram, probably loaded over tftp.
        // A rootfs_data partition of variable size is mounted which only happens when
        // running from NAND, so look in NAND for version of stored images
        fw_fis_active = nullptr; // None of the installed images are active
        fw_fis_primary = fw_fis_linux;
        if ( strstr(str, "(linux.bk)") ) {
            fw_fis_backup = fw_fis_linux_bk;
        } else {
            fw_fis_backup = fw_fis_linux;
        }
        if (vtss_mtd_rootfs_is_nor()) {
            fw_fis_layout = fw_nor;
        } else {
            fw_fis_layout = fw_nor_nand;
        }
        T_D("Running from %s, primary image %s", fw_fis_layout, fw_fis_primary);
        return;
    }

    char switch_mnt[1024];

    if (VTSS_RC_OK != get_mount_point_source("/switch", switch_mnt, sizeof(switch_mnt))) {
        T_D("No /switch, no idea where to look for installed images");
        // No /switch, no idea where to look for installed images
        return;
    }

    if ( strstr(switch_mnt, "/dev/mmcblk") ) {
        fw_fis_active = nullptr;
        fw_fis_primary = firmware_fis_prim_emmc();
        fw_fis_backup = firmware_fis_bak_emmc();
        fw_fis_layout = fw_emmc;
        T_D("Running from %s, primary image %s", fw_fis_layout, fw_fis_primary);
        return;
    }
    T_D("No idea where to look for installed images");

}

const char *firmware_fis_layout()
{
    return fw_fis_layout;
}

bool firmware_is_mfi_based()
{
    const char *str = firmware_cmdline_get();
    if ( str && strstr(str, "image=mfi") ) {
        return true;
    }
    return false;
}

bool firmware_is_nand_only()
{
    return fw_fis_layout == fw_nand;
}

bool firmware_is_nor_only()
{
    return fw_fis_layout == fw_nor;
}

bool firmware_is_mmc()
{
    return fw_fis_layout == fw_emmc;
}

bool firmware_is_nor_nand()
{
    return fw_fis_layout == fw_nor_nand;
}

/* Find out active fis section */
const char *firmware_fis_act(void)
{
    return fw_fis_active;
}

const char *firmware_fis_prim(void)
{
    return fw_fis_primary;
}

const char *firmware_fis_bak(void)
{
    return fw_fis_backup;
}

/* Find out alternate fis section, if applicable */
BOOL firmware_has_alt(void)
{
    return fw_fis_backup != nullptr && fw_fis_backup != fw_fis_primary;
}

/* Find out which fis section should be upgraded */
const char *firmware_fis_to_update(void)
{
    if (!firmware_fis_bak() || firmware_fis_act() == firmware_fis_bak()) {
        return firmware_fis_prim();
    }
    return firmware_fis_bak();
}

static bool firmware_is_board_compatible(const char *machine)
{
    vtss_mtd_t act;
    bool ret = true;    // Assume compatible by default
    if (vtss_mtd_open(&act, firmware_fis_act()) == VTSS_RC_OK) {
        mscc_firmware_vimage_t fw;
        if (mscc_firmware_fw_vimage_get(&act, &fw) == VTSS_RC_OK) {
            ret = (strncmp(machine, fw.machine, sizeof(fw.machine)) == 0);
            T_I("%s platform: Expect %s, have %s", ret ? "Compatible" : "Incompatible", fw.machine, machine);
        }
        vtss_mtd_close(&act);
    }
    return ret;
}

mesa_rc firmware_check_mfi(const unsigned char *buffer, size_t length)
{
    const char *msg;

    VTSS_ASSERT(buffer != NULL);
    {
        const mscc_firmware_vimage_t *fw = (const mscc_firmware_vimage_t *) buffer;
        mscc_firmware_vimage_tlv_t tlv;
        const u8 *tlvdata;

        if (fw->imglen > length) {
            T_D("Image too small: %u > %zu", fw->imglen, length);
            return FIRMWARE_ERROR_INVALID;
        }

        if (mscc_vimage_hdrcheck(fw, &msg) != 0) {
            T_D("Invalid image: %s", msg);
            return FIRMWARE_ERROR_EXPECTING_MFI_IMAGE;
        }

        // Check the machine name matches the current machine name
        if (!firmware_is_board_compatible(fw->machine)) {
            return FIRMWARE_ERROR_INCOMPATIBLE_TARGET;
        }

        // Authenticate
        if ((tlvdata = mscc_vimage_find_tlv(fw, &tlv, MSCC_STAGE1_TLV_SIGNATURE))) {
            if (!mscc_vimage_validate(fw, tlvdata, tlv.data_len)) {
                T_D("Validation fails");
                return FIRMWARE_ERROR_AUTHENTICATION;
            }
        } else {
            T_D("Invalid image: Signature missing");
            return FIRMWARE_ERROR_SIGNATURE;
        }

        // Ensure we have at least a kernel TLV
        if ((tlvdata = mscc_vimage_find_tlv(fw, &tlv, MSCC_STAGE1_TLV_KERNEL)) == NULL) {
            T_D("No kernel in image?");
            return FIRMWARE_ERROR_NO_CODE;
        }

        // Do we have MFI stage2 as well?
        if (length > fw->imglen) {
            // Yes - so check it
            u32 s2len = length - fw->imglen;
            const u8 *s2ptr = buffer + fw->imglen;
            const mscc_firmware_vimage_stage2_tlv_t *s2tlv;
            T_D("Stage2 at %p, length %d", s2ptr, s2len);
            while(s2len > sizeof(*s2tlv)) {
                s2tlv = (const mscc_firmware_vimage_stage2_tlv_t*) s2ptr;
                T_D("Stage2 TLV at %p, type %u", s2tlv, s2tlv->type);
                if (!mscc_vimage_stage2_check_tlv(s2tlv, s2len, true)) {
                    T_D("Stage2 TLV error at %p, offset %08zx", s2ptr, s2ptr - buffer);
                    return FIRMWARE_ERROR_INVALID;
                }
                s2ptr += s2tlv->tlv_len;
                s2len -= s2tlv->tlv_len;
            }
        }
    }

    return VTSS_RC_OK;
}

mesa_rc firmware_check_nand_only(const unsigned char *buffer, size_t length)
{
    if (length < 4) {
        T_D("Exit %s: status: %d", __FUNCTION__, g_image_upload.upload_status);
        return FIRMWARE_ERROR_INVALID;
    }
    // This must be a '.ubifs' file. Do some rough format validation
    if (buffer[0] != 0x31 || buffer[1] != 0x18 || buffer[2] != 0x10 || buffer[3] != 0x06) {
        // Wrong magic
        T_D("Magic invalid, found: %02x %02x %02x %02x", buffer[0], buffer[1], buffer[2], buffer[3]);
        return FIRMWARE_ERROR_EXPECTING_UBIFS_IMAGE;
    }
    return MESA_RC_OK;
}

mesa_rc firmware_check_mmc(const unsigned char *buffer, size_t length)
{
    if (length < 4) {
        T_D("Exit %s: status: %d", __FUNCTION__, g_image_upload.upload_status);
        return FIRMWARE_ERROR_INVALID;
    }

    // This must be a '.ext4.gz' file. Do some rough format validation
    if (buffer[0] != 0x1f || buffer[1] != 0x8b ||
        buffer[2] != 0x08 || buffer[3] != 0x00) {
        // Wrong magic
        T_D("Magic invalid, found: %02x %02x %02x %02x", buffer[0], buffer[1], buffer[2], buffer[3]);
        return FIRMWARE_ERROR_EXPECTING_EXT4_GZ_IMAGE;
    }
    return MESA_RC_OK;
}

mesa_rc firmware_check_nor_only(const unsigned char *buffer, size_t length)
{
    if (length < 8) {
        T_D("Exit %s: status: %d", __FUNCTION__, g_image_upload.upload_status);
        return FIRMWARE_ERROR_INVALID;
    }

    // This must be an '.itb' file. Do some rough format validation
    if (buffer[0] != 0xd0 || buffer[1] != 0x0d ||
        buffer[2] != 0xfe || buffer[3] != 0xed) {
        // Wrong magic
        T_D("Magic invalid, found: %02x %02x %02x %02x", buffer[0], buffer[1], buffer[2], buffer[3]);
        return FIRMWARE_ERROR_EXPECTING_ITB_IMAGE;
    }

    uint32_t exp_length = (buffer[4]*0x1000000) + (buffer[5]*0x10000)
                           + (buffer[6]*0x100) + (buffer[7]);
    if ( exp_length > length) {
        // Wrong size
        T_D("Wrong size: got %d bytes, expected %d bytes", length, exp_length);
        return FIRMWARE_ERROR_INVALID;
    }
    return MESA_RC_OK;
}

mesa_rc firmware_check(const unsigned char *buffer, size_t length)
{
    T_D("Enter %s", __FUNCTION__);

    if (firmware_is_mfi_based()) {
        return firmware_check_mfi(buffer,length);
    } else if (firmware_is_nand_only()) {
        return firmware_check_nand_only(buffer,length);
    } else if (firmware_is_mmc()) {
        return firmware_check_mmc(buffer,length);
    } else if (firmware_is_nor_only()) {
        return firmware_check_nor_only(buffer,length);
    } else {
        return FIRMWARE_ERROR_CURRENT_UNKNOWN;
    }
}

// Return TRUE if mtd is already programmed - FALSE if update is needed
BOOL firmware_mtd_checksame(cli_iolayer_t *io,
                            vtss_mtd_t *mtd,
                            const char *name,
                            const u8 *buffer,
                            size_t length)
{
    size_t off;
    ssize_t nread;
    u8 readbuf[8*1024];
    cli_io_printf(io, "Checking old image %s... ", name);
    lseek(mtd->fd, 0, SEEK_SET);
    for (off = 0; off < length; off += nread) {
        size_t rreq = MIN(length - off, sizeof(readbuf));
        nread = read(mtd->fd, readbuf, rreq);
        if(nread == 0) {
            break;    // eof
        } else if(nread > 0) {
            if(memcmp(buffer+off, readbuf, nread) != 0) {
                cli_io_printf(io, "needs update\n");
                return FALSE;    // Difference found
            }
        } else {
            cli_io_printf(io, "read error at offset 0x%08zx\n", off);
            return FALSE;    // Bail out - pretend different
        }
    }
    cli_io_printf(io, "already updated\n");
    return TRUE;    // Read all, all the same
}

void firmware_setstate(cli_iolayer_t *io, fw_state_t state, const char *name)
{
    const char *msg;
    switch(state) {
        case FWS_BUSY:
            msg = "Error: Update already in progress";
            if (io)
                cli_io_printf(io, "%s\n", msg);
            else
                firmware_status_set(msg);
            break;
        case FWS_ERASING:
            msg = "Erasing flash...";
            if (io)
                cli_io_printf(io, "%s", msg);
            else
                firmware_status_set(msg);
            break;
        case FWS_ERASED:
            if (io)
                cli_io_printf(io, "done\n");
            else
                firmware_status_set("Erased flash");
            break;
        case FWS_PROGRAMMING:
            msg = "Programming flash...";
            if (io)
                cli_io_printf(io, "%s", msg);
            else
                firmware_status_set(msg);
            break;
        case FWS_PROGRAMMED:
            if (io)
                cli_io_printf(io, "done\n");
            else
                firmware_status_set("Programmed flash");
            break;
        case FWS_SWAPPING:
            msg = "Swapping images...";
            if (io)
                cli_io_printf(io, "%s", msg);
            else
                firmware_status_set(msg);
            break;
        case FWS_SWAP_DONE:
            if (io)
                cli_io_printf(io, "done\n");
            else
                firmware_status_set("Swapped swapped active and backup image");
            break;
        case FWS_SWAP_FAILED:
            if (io)
                cli_io_printf(io, "failed!\n");
            else
                firmware_status_set("Error: Swapping images failed");
            break;
        case FWS_PROGRAM_FAILED:
            if (io)
                cli_io_printf(io, "failed!\n");
            else
                firmware_status_set("Error: Programming flash failed");
            break;
        case FWS_ERASE_FAILED:
            if (io)
                cli_io_printf(io, "failed!\n");
            else
                firmware_status_set("Error: Erasing flash failed");
            break;
        case FWS_SAME_IMAGE:
            msg = "Error: Flash already updated";
            if (io)
                cli_io_printf(io, "%s\n", msg);
            else
                firmware_status_set(msg);
            break;
        case FWS_INVALID_IMAGE:
            msg = "Error: Invalid image";
            if (io)
                cli_io_printf(io, "%s\n", msg);
            else
                firmware_status_set(msg);
            break;
        case FWS_UNKNOWN_MTD:
            msg = "Error: Firmware flash device not found";
            if (io)
                cli_io_printf(io, "%s: %s\n", name, msg);
            else
                firmware_status_set(msg);
            break;
        case FWS_REBOOT:
            msg = "Restarting, please wait...";
            if (io)
                cli_io_printf(io, "%s", msg);
            else
                firmware_status_set(msg);
            break;
        case FWS_DONE:
            if (io)
                cli_io_printf(io, "Firmware update completed. Restart system to activate new firmware.\n");
            else
                firmware_status_set(NULL);
            break;
    }
}

void firmware_stage2_cleanup()
{
    DIR *dp;
    struct dirent *dirp;
    mesa_rc rc;
    const char *primary = "linux", *backup = "linux.bk";

    char slot_a[128] = {};
    char slot_b[128] = {};

    bool slot_a_valid = false;
    bool slot_b_valid = false;

    T_D("firmware_stage2_erase");

    // Figure out what files to keep
    rc = firmware_image_stage2_name_get(primary, slot_a, sizeof(slot_a));
    if (rc == VTSS_RC_OK) {
        T_D("Got stage2 filename for '%s': %s", primary, slot_a);
        slot_a_valid = true;
    } else {
        T_D("No stage2 filename for '%s'", primary);
    }

    rc = firmware_image_stage2_name_get(backup, slot_b, sizeof(slot_b));
    if (rc == VTSS_RC_OK) {
        T_D("Got stage2 filename for '%s': %s", backup, slot_b);
        slot_b_valid = true;
    } else {
        backup = "split_linux_hi";    // Special case for split bootstrap
        rc = firmware_image_stage2_name_get(backup, slot_b, sizeof(slot_b));
        if (rc == VTSS_RC_OK) {
            T_D("Got stage2 filename for '%s': %s", backup, slot_b);
            slot_b_valid = true;
        } else {
            T_D("No stage2 filename for '%s'", backup);
        }
    }

    // Now, delete everything in /switch/stage2/ except for 'slot_a' and
    // 'slot_b'

    // Open directory to iterate over the files
    dp = opendir(FIRWARE_STAGE2);
    if (!dp) {
        if (errno != ENOENT) {
            T_D("Failed to open %s - error %s", FIRWARE_STAGE2, strerror(errno));
        }
        goto EXIT;
    }

    // Iterate
    while ((dirp = readdir(dp)) != nullptr) {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0) {
            continue;
        }

        // Do not delete the stage2 file used by primary
        if (slot_a_valid && (strcmp(dirp->d_name, slot_a) == 0)) {
            T_D("Do not delete slot_a: %s", slot_a);
            continue;
        }

        // Do not delete the stage2 file used by backup
        if (slot_b_valid && (strcmp(dirp->d_name, slot_b) == 0)) {
            T_D("Do not delete slot_b: %s", slot_b);
            continue;
        }

        // Nobody is using this, delete it!
        if (unlinkat(dirfd(dp), dirp->d_name, 0) == -1) {
            T_W("Failed to erase the stage2 image: %s - error %s", dirp->d_name,
                strerror(errno));
        } else {
            T_D("Stage2 image %s was erased successfully", dirp->d_name);
        }
    }

EXIT:
    closedir(dp);
}

#if 0
// Check same image against the current active fis
static mesa_rc firmware_update_checksame(cli_iolayer_t *io,
                                         const unsigned char *buffer,
                                         size_t length)
{
    mesa_rc rc = VTSS_RC_OK;
    vtss_mtd_t mtd_act;   // mtd device that is currently running

    // open
    if ((rc = vtss_mtd_open(&mtd_act,firmware_fis_act())) != VTSS_RC_OK) {
        firmware_setstate(io, FWS_UNKNOWN_MTD, firmware_fis_act());
        return VTSS_RC_ERROR;
    }

    if (firmware_mtd_checksame(io, &mtd_act, firmware_fis_act(), buffer, length)) {
        firmware_setstate(io, FWS_SAME_IMAGE, firmware_fis_act());
        rc = FIRMWARE_ERROR_SAME;
    }

    vtss_mtd_close(&mtd_act);  // cleanup
    return rc;
}
#endif

mesa_rc firmware_update_stage1(cli_iolayer_t *io,
                               const unsigned char *buffer,
                               size_t length,
                               const char *mtd_name,
                               const char *sb_file_name,
                               const char *sb_stage2_name)
{
    mesa_rc rc = VTSS_RC_OK;
    vtss_mtd_t mtd;       // mtd device that is going to be upgraded
    mscc_firmware_sideband_t *sb;

    T_D("Update %s length %zu", mtd_name, length);

    // TODO - This will become a problem. You may have two different images with
    // identical stage1 (different stage 2).
    // I suggest that we simply drop this feature.
    //
    // checksame first
#if 0    // Until fixed for stage1/stage2
    if ((rc = firmware_update_checksame(io, buffer, length)) != VTSS_RC_OK) {
        return rc;
    }
#endif

    // Pre-allocate sideband data blob (include NULL termination)
    // We always have a filename, optionally stage2 name
    VTSS_ASSERT(sb_file_name);
    sb = mscc_vimage_sideband_create();
    if (sb) {
        sb = mscc_vimage_sideband_add_tlv(sb, (u8 *) sb_file_name, strlen(sb_file_name)+1, MSCC_FIRMWARE_SIDEBAND_FILENAME);
    }
    if (sb && sb_stage2_name) {
        T_D("Adding sb_stage2_name: %s", sb_stage2_name);
        sb = mscc_vimage_sideband_add_tlv(sb, (u8 *) sb_stage2_name, strlen(sb_stage2_name)+1, MSCC_FIRMWARE_SIDEBAND_STAGE2_FILENAME);
    }
    if (!sb) {
        T_E("Failed to allocate sideband data");
        return FIRMWARE_ERROR_MALLOC;
    }

    if ((rc = vtss_mtd_open(&mtd, mtd_name)) != VTSS_RC_OK) {
        firmware_setstate(io, FWS_UNKNOWN_MTD, mtd_name);
        goto EXIT;
    }

    firmware_setstate(io, FWS_ERASING, mtd_name);
    if ((rc = vtss_mtd_erase(&mtd, length)) != VTSS_RC_OK) {
        firmware_setstate(io, FWS_ERASE_FAILED, mtd_name);
        T_D("FAILED: vtss_mtd_erase");
        goto EXIT;
    }

    firmware_setstate(io, FWS_ERASED, mtd_name);
    firmware_setstate(io, FWS_PROGRAMMING, mtd_name);
    if ((rc = vtss_mtd_program(&mtd, buffer, length)) != VTSS_RC_OK) {
        firmware_setstate(io, FWS_PROGRAM_FAILED, mtd_name);
        T_D("FAILED: vtss_mtd_program");
        goto EXIT;
    }

    // Add sideband data
    mscc_vimage_sideband_update_crc(sb);
    if ((rc = mscc_vimage_sideband_write(&mtd, sb, MSCC_FIRMWARE_ALIGN(length, mtd.info.erasesize))) != VTSS_RC_OK) {
        firmware_setstate(io, FWS_PROGRAM_FAILED, mtd_name);
        T_D("FAILED: mscc_vimage_sideband_write");
        goto EXIT;
    }

    T_D("Stage1: All done");
    firmware_setstate(io, FWS_PROGRAMMED, mtd_name);

EXIT:
    if (sb) {
        VTSS_FREE(sb);
    }
    vtss_mtd_close(&mtd);
    return rc;
}

mesa_rc firmware_check_bootloader_simg(const u8 *buffer,
                                       size_t length,
                                       u32 *type)
{
    mesa_rc rc = FIRMWARE_ERROR_INVALID;
    simage_t simg;
    if (simage_parse(&simg, buffer, length) == VTSS_RC_OK) {
        u32 arch = 0, chip = 0, imgtype = 0;
        if (!simage_get_tlv_dword(&simg, SIMAGE_TLV_ARCH, &arch) ||
            arch != MY_CPU_ARC) {
            T_D("Invalid or missing architecture: %d", arch);
            rc = FIRMWARE_ERROR_WRONG_ARCH;
        } else if (!simage_get_tlv_dword(&simg, SIMAGE_TLV_CHIP, &chip) ||
                   misc_chip2family(chip) != fast_cap(MESA_CAP_MISC_CHIP_FAMILY)) {
            T_D("Firmware image chip_family (%d) does not match system's (%d)",
                misc_chip2family(chip), fast_cap(MESA_CAP_MISC_CHIP_FAMILY));
            rc = FIRMWARE_ERROR_WRONG_ARCH;
        } else if (!simage_get_tlv_dword(&simg, SIMAGE_TLV_IMGTYPE, &imgtype)) {
            T_D("Image type TLV not present");
        } else {
            *type = imgtype;
            rc = VTSS_RC_OK;
        }
    } else {
        T_D("Bad Image, invalid format");
    }
    return rc;
}

static
mesa_rc firmware_check_bootloader(const u8 *buffer,
                                  size_t length,
                                  const char **mtd_name)
{
    u32 imgtype;
    mesa_rc rc = firmware_check_bootloader_simg(buffer, length, &imgtype);
    if (rc == VTSS_RC_OK) {
        if (imgtype == SIMAGE_IMGTYPE_BOOT_LOADER) {
            *mtd_name = "RedBoot";
        } else {
            rc = FIRMWARE_ERROR_WRONG_ARCH;
        }
    }
    return rc;
}

mesa_rc firmware_flash_mtd(cli_iolayer_t *io,
                           const char *mtd_name,
                           const unsigned char *buffer,
                           size_t length)
{
    vtss_mtd_t mtd;
    mesa_rc rc;
    if ((rc = vtss_mtd_open(&mtd, mtd_name)) != VTSS_RC_OK) {
        cli_io_printf(io, "Unable to open '%s' for Flash image update\n", mtd_name);
        return rc;
    }

    if (mtd.info.size < length) {
        vtss_mtd_close(&mtd);
        T_D("Image too big, image size: %d, mtd size: %d", length, mtd.info.size);
        return FIRMWARE_ERROR_SIZE;
    }

    if (!firmware_mtd_checksame(io, &mtd, mtd_name, buffer, length)) {
        cli_io_printf(io, "Erasing '%s'...", mtd_name);
        if (vtss_mtd_erase(&mtd, length) == VTSS_RC_OK) {
            cli_io_printf(io, " done!\n");
            cli_io_printf(io, "Programming '%s'...", mtd_name);
            if (vtss_mtd_program(&mtd, buffer, length) == VTSS_RC_OK) {
                cli_io_printf(io, " done!\n");
            } else {
                cli_io_printf(io, " failed!\n");
                rc = FIRMWARE_ERROR_FLASH_PROGRAM;
            }
        } else {
            cli_io_printf(io, " failed!\n");
            rc = FIRMWARE_ERROR_FLASH_ERASE;
        }
    } else {
        rc = FIRMWARE_ERROR_SAME;
    }

    vtss_mtd_close(&mtd);
    return rc;
}

mesa_rc firmware_flash_mtd_if_needed(cli_iolayer_t *io,
                                     const char *mtd_name,
                                     const unsigned char *buffer,
                                     size_t length)
{
    mesa_rc rc = firmware_flash_mtd(io, mtd_name, buffer, length);
    // Same is also OK;
    return rc == FIRMWARE_ERROR_SAME ? VTSS_RC_OK : rc;
}

mesa_rc firmware_update_stage2_bootloader(cli_iolayer_t *io,
                                          const unsigned char *s2ptr,
                                          size_t s2len)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
    const unsigned char *origin = s2ptr;
#endif
    const mscc_firmware_vimage_stage2_tlv_t *s2tlv, *s2bootloader = NULL;
    mesa_rc rc = VTSS_RC_OK;

    T_D("Stage2 at %p, length %zd", s2ptr, s2len);

    // Find *last* bootloader TLV, if any
    while(s2len > sizeof(*s2tlv)) {
        s2tlv = (const mscc_firmware_vimage_stage2_tlv_t*) s2ptr;
        T_D("Stage2 TLV at %p", s2tlv);
        // This may have been checked before, but...
        if (!mscc_vimage_stage2_check_tlv(s2tlv, s2len, true)) {
            T_D("Stage2 TLV error at %p, offset %08zx", s2ptr, s2ptr - origin);
            return FIRMWARE_ERROR_INVALID;
        } else {
            mscc_firmware_vimage_stage2_tlv_t tlvcopy;
            // TLV may be unaligned
            memcpy(&tlvcopy, s2tlv, sizeof(tlvcopy));
            // See if its a bootloader
            if (tlvcopy.type == MSCC_STAGE2_TLV_BOOTLOADER) {
                T_D("Stage2 bootloader TLV at %p, offset %08zx", s2ptr, s2ptr - origin);
                s2bootloader = s2tlv;
            }
        }
        s2ptr += s2tlv->tlv_len;
        s2len -= s2tlv->tlv_len;
    }

    if (s2bootloader) {
        mscc_firmware_vimage_stage2_tlv_t tlvcopy;
        const unsigned char *bootdata = s2bootloader->value;
        const char *mtd_name;
        // TLV may be unaligned
        memcpy(&tlvcopy, s2bootloader, sizeof(tlvcopy));
        T_D("Stage2 bootloader update, len = %d", tlvcopy.data_len);
        rc = firmware_check_bootloader(bootdata, tlvcopy.data_len, &mtd_name);
        if (rc == VTSS_RC_OK) {
            rc = firmware_flash_mtd_if_needed(io, mtd_name, bootdata, tlvcopy.data_len);
        }
    }

    return rc;
}


mesa_rc firmware_update_async_start(uint32_t session_id, uint32_t total_chunks,
                                    cli_iolayer_t *io, const char *filename)
{
    T_D("Start flash update, filename: %s", filename);
    mesa_rc rc = VTSS_RC_OK;

    auto fw = manager.get_for_session(session_id, 0);
    if (fw.ok()) {
        fw->init(nullptr, firmware_max_download);
        fw->attach_cli(io);
        fw->set_filename(filename);
        fw->set_total_chunks(total_chunks);
    } else {
        rc = FIRMWARE_ERROR_BUSY;
    }
    return rc;
}

mesa_rc firmware_update_async_write(uint32_t session_id, uint32_t chunk_number,
                                    const unsigned char *buffer,
                                    size_t length, uint32_t timeout_secs)
{
    T_D("+Write %u bytes", length);
    mesa_rc rc = VTSS_RC_OK;
    ssize_t written;
    uint32_t next_exp_chunk;

    auto fw = manager.get_for_session(session_id, timeout_secs);
    if (fw.ok()) {
        // get next expected chunk number
        next_exp_chunk = fw->get_next_exp_chunk();
        if (next_exp_chunk > chunk_number) {
            // We already have this chunk. This is OK as it may have been retransmitted due to network problems.
            return VTSS_RC_OK;
        } else if (next_exp_chunk < chunk_number) {
            // We apparently missed one or more chunks before this one - fail download!
            return FIRMWARE_ERROR_CHUNK_OOO;
        }

        // we got the expected chunk - so write it
        written = fw->write(buffer, length);
        if (written != length) {
            return FIRMWARE_ERROR_SIZE;
        }
        // set the current chunk number as valid
        fw->set_chunk_received(chunk_number);
        
        // re-start session timer
        manager.start_session_timer(session_id, timeout_secs);

    } else {
        rc = FIRMWARE_ERROR_BUSY;
    }

    T_D("-Write %u bytes", length);
    return rc;
}

mesa_rc firmware_update_async_get_last_chunk(uint32_t session_id,
                                             uint32_t &chunk_number)
{
    T_D("Commit flash update");
    mesa_rc rc = VTSS_RC_OK;

    auto fw = manager.get_for_session(session_id, 0);
    if (fw.ok()) {
        chunk_number = fw->get_last_received_chunk_number();
    } else {
        chunk_number = 0;
        rc = FIRMWARE_ERROR_BUSY;
    }
    return rc;
}

mesa_rc firmware_update_async_commit(uint32_t session_id, mesa_restart_t restart)
{
    T_D("Commit flash update");
    mesa_rc rc = VTSS_RC_OK;

    auto fw = manager.get_for_session(session_id, 0);
    if (fw.ok()) {
        // check that we have received all expected chunks
        if (fw->get_last_received_chunk_number() != fw->get_total_chunks()) {
            return FIRMWARE_ERROR_INVALID;
        }

        // append a filename TLV if needed
        fw->append_filename_tlv();

        // this will release the FW instance when 'fw' goes out of scope
        manager.clear_session_id(session_id);
      
        if (fw->check() == MESA_RC_OK) {
            manager.store_async(vtss::move(fw));
            rc = FIRMWARE_ERROR_IN_PROGRESS;
        } else {
            rc = FIRMWARE_ERROR_INVALID;
        }

    } else {
        rc = FIRMWARE_ERROR_BUSY;
    }

    return rc;
}

mesa_rc firmware_update_async_abort(uint32_t session_id)
{
    mesa_rc rc = VTSS_RC_OK;

    auto fw = manager.get_for_session(session_id, 0);
    if (fw.ok()) {
        // When we clear the session ID the FirmwareDownload instance will be
        // released when the 'fw' instance goes out of scope.
        manager.clear_session_id(session_id);
    } else {
        rc = FIRMWARE_ERROR_BUSY;
    }

    return rc;
}

/*
 * API: Asynchronous firmware update.
 * Returns VTSS_RC_OK if update is started, any other FIRMWARE_ERROR_xxx code otherwise
 */
mesa_rc firmware_update_async(cli_iolayer_t *io,
                              const unsigned char *buffer,
                              char *buf2free,
                              size_t length,
                              const char *filename,
                              mesa_restart_t restart)
{
    mesa_rc rc;
    auto fw = manager.get();
    if (fw.ok()) {
        fw->init(nullptr, firmware_max_download);
        fw->attach_cli(io);
        fw->write(buffer, length);
        fw->set_filename(filename);
        fw->append_filename_tlv();
        // We are done using the received buffer. Free it now before we mmap the saved data.
        if (buf2free) {
            VTSS_FREE(buf2free);
        }
        if (fw->check() == MESA_RC_OK) {
            manager.store_async(vtss::move(fw));
            rc = FIRMWARE_ERROR_IN_PROGRESS;
        } else {
            rc = FIRMWARE_ERROR_INVALID;
        }
    } else {
        rc = FIRMWARE_ERROR_BUSY;

        if (buf2free) {
            VTSS_FREE(buf2free);
        }
    }
    return rc;
}

static void firmware_thread(vtss_addrword_t data)
{
    // Wait until first INIT_DONE event.
    msg_wait(MSG_WAIT_UNTIL_INIT_DONE, VTSS_MODULE_ID_FIRMWARE);

    firmware_determine_memory_type();

    T_I("fis_act = %s", firmware_fis_act());
    T_I("fis_prim = %s", firmware_fis_prim());
    T_I("fis_bak = %s", firmware_fis_bak());
    T_I("firmware_fis_to_update = %s", firmware_fis_to_update());
    T_I("fw_fis_layout = %s\n", firmware_fis_layout());

    for(;;) {
        T_D("main loop");
        FwWrap fw = manager.get_async();
        T_D("got image");
        if (fw.ok()) {
            T_D("Firmware update: Image is %zu length filename %s", fw->length(), fw->filename());
            fw->update();
            // Only returns if error occurred
            fw->reset();
        }
    }
}

static void _upload_status_get(mesa_rc rc, BOOL b_firmware)
{
    SIMPLE_LOCK();

    switch(rc) {
        case VTSS_RC_OK:
            if ( b_firmware ) {
                g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_IN_PROGRESS;
            } else {
                g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_SUCCESS;
            }
            break;

        case FIRMWARE_ERROR_IN_PROGRESS:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_IN_PROGRESS;
            break;

        case FIRMWARE_ERROR_IP:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_IP;
            break;

        case FIRMWARE_ERROR_TFTP:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_TFTP;
            break;

        case FIRMWARE_ERROR_BUSY:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_BUSY;
            break;

        case FIRMWARE_ERROR_MALLOC:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_MEMORY_INSUFFICIENT;
            break;

        case FIRMWARE_ERROR_INVALID:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_INVALID_IMAGE;
            break;

        case FIRMWARE_ERROR_FLASH_PROGRAM:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_WRITE_FLASH;
            break;

        case FIRMWARE_ERROR_SAME:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_SAME_IMAGE_EXISTED;
            break;

        case FIRMWARE_ERROR_CURRENT_UNKNOWN:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_UNKNOWN_IMAGE;
            break;

        case FIRMWARE_ERROR_CURRENT_NOT_FOUND:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_FLASH_IMAGE_NOT_FOUND;
            break;

        case FIRMWARE_ERROR_UPDATE_NOT_FOUND:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_FLASH_ENTRY_NOT_FOUND;
            break;

        case FIRMWARE_ERROR_CRC:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_CRC;
            break;

        case FIRMWARE_ERROR_SIZE:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_IMAGE_SIZE;
            break;

        case FIRMWARE_ERROR_FLASH_ERASE:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_ERASE_FLASH;
            break;

        case FIRMWARE_ERROR_INCOMPATIBLE_TARGET:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_INCOMPATIBLE_TARGET;
            break;

        case FIRMWARE_ERROR_PROTOCOL:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_PROTOCOL;
            break;

        case FIRMWARE_ERROR_NO_USERPW:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_NO_USERPW;
            break;

        case FIRMWARE_ERROR_CHUNK_OOO:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_CHUNK_OOO;
            break;

        default:
            g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_NONE;
            break;
    }
    SIMPLE_UNLOCK();
}

static void _firmware_control_image_upload(void)
{
    mesa_rc rc = VTSS_RC_ERROR;
    vtss::remote_file_options_t transfer_options;

    transfer_options.ftp_active = g_image_upload.ftp_active;
    transfer_options.ssh_save_host_keys = g_image_upload.ssh_save_host_keys;

    switch( g_image_upload.type ) {
        case VTSS_APPL_FIRMWARE_UPLOAD_IMAGE_TYPE_BOOTLOADER:
            rc = firmware_icli_load_image(cli_get_io_handle(), "RedBoot", g_image_upload.url, check_bootloader, transfer_options);
            break;

        case VTSS_APPL_FIRMWARE_UPLOAD_IMAGE_TYPE_FIRMWARE:
            {
                auto fw = manager.get();
                if (fw.ok()) {
                    fw->init(nullptr, firmware_max_download);
                    if ((rc = fw->download(g_image_upload.url, transfer_options)) == MESA_RC_OK) {
                        if (fw->check() != MESA_RC_OK) {
                            rc = FIRMWARE_ERROR_INVALID;
                        } else {
                            manager.store_async(vtss::move(fw));
                            rc = FIRMWARE_ERROR_IN_PROGRESS;
                        }
                    }
                } else {
                    rc = FIRMWARE_ERROR_BUSY;
                }
            }
            break;

        default:
            /* should not get there as vtss_appl_firmware_control_image_upload_set() has checked parameters */
            T_D("invalid g_image_upload.type (%u)", g_image_upload.type);
            return;
    }

    /* get upload status */
    _upload_status_get(rc, g_image_upload.type == VTSS_APPL_FIRMWARE_UPLOAD_IMAGE_TYPE_FIRMWARE);
}

static void _firmware_upload_thread(vtss_addrword_t data)
{
    vtss_flag_value_t flag;

    msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_FIRMWARE);
    
    /* read image status to initialize g_image_status[] */
    if (vtss_appl_firmware_status_image_entry_init(VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_BOOTLOADER,
                                                  &g_image_status[VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_BOOTLOADER]) != VTSS_RC_OK) {
        //T_E("_firmware_status_image_entry_get( VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_BOOTLOADER )\n");
    }
    if (firmware_fis_prim() &&
        vtss_appl_firmware_status_image_entry_init(VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ACTIVE_FIRMWARE,
                                                  &g_image_status[VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ACTIVE_FIRMWARE]) != VTSS_RC_OK) {
        //T_E("_firmware_status_image_entry_get( VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ACTIVE_FIRMWARE )\n");
    }
    if (firmware_fis_bak() &&
        vtss_appl_firmware_status_image_entry_init(VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE,
                                                  &g_image_status[VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE]) != VTSS_RC_OK) {
        //T_E("_firmware_status_image_entry_get( VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE )\n");
    }
    version_information_available.lock(false);

    /* process upload event */
    while (1) {
        while (msg_switch_is_primary()) {
            flag = vtss_flag_wait(&g_firmware_upload_thread_flag,
                                  FIRMWARE_UPLOAD_THREAD_FLAG_UPLOAD,
                                  VTSS_FLAG_WAITMODE_OR_CLR);
            if (flag & FIRMWARE_UPLOAD_THREAD_FLAG_UPLOAD) {
                _firmware_control_image_upload();
            }
        }

        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_FIRMWARE);
    }
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

static void clean_dir(const char * dirname)
{
    DIR *dp;
    struct dirent *dirp;

    // Open directory to iterate over the files
    dp = opendir(dirname);
    if (!dp) {
        return;
    }

    // Iterate
    while ((dirp = readdir(dp)) != nullptr) {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0) {
            continue;
        }
        // Remove all else
        (void) unlinkat(dirfd(dp), dirp->d_name, 0);
    }

    closedir(dp);
}


void
firmware_init_os(vtss_init_data_t *data)
{
    switch (data->cmd) {
        case INIT_CMD_INIT:
            clean_dir(DLD_DIR);
            vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                               firmware_thread,
                               0,
                               "FIRMWARE",
                               nullptr,
                               0,
                               &firmware_thread_handle,
                               &firmware_thread_block);

            vtss_flag_init(&g_firmware_upload_thread_flag);  /* No initialization needed */
            vtss_flag_maskbits(&g_firmware_upload_thread_flag, 0);

            memset(&g_image_upload, 0, sizeof(g_image_upload));
            memset(&g_image_status, 0, sizeof(g_image_status));
            vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                               _firmware_upload_thread,
                               0,
                               "Firmware Upload",
                               nullptr,
                               0,
                               &g_firmware_upload_thread_handle,
                               &g_firmware_upload_thread_block);
            break;
        default:
            break;
    }
}


/*
==============================================================================

    Public APIs in vtss_appl\include\vtss\appl\firmware.h

==============================================================================
*/
static const char *bin_findstr(const char *haystack, const char *haystack_end,
                               const char *needle, size_t nlen)
{
    /* memchr used here to scan memory for a special character */
    while(haystack && haystack < haystack_end &&
          (haystack = (const char *)memchr(haystack, needle[0], haystack_end -haystack))) {
        if (strncmp(haystack, needle, nlen) == 0)
            return haystack;
        haystack++;
    }
    return NULL;
}

static const char *firmware_bin_findstr(const char *haystack,
                                        size_t sz_buffer,
                                        const char *needle)
{
    return bin_findstr(haystack, haystack + sz_buffer, needle, strlen(needle));
}

static mesa_rc
_firmware_bootloader_status_get(vtss_mtd_t *mtd, vtss_appl_firmware_status_image_t  *image_entry)
{
    mesa_rc rc = FIRMWARE_ERROR_INVALID;
    image_entry->version[0] = image_entry->built_date[0] = image_entry->code_revision[0] = '\0';
    if (mtd->info.size) {
        char *blob = NULL;

        VTSS_MALLOC_CAST(blob, mtd->info.size);
        if (blob != NULL) {
            if (read(mtd->fd, blob, mtd->info.size) != -1) {
                const char *idstr = "Non-certified release, ", *hit;
                char *split;

                if ((hit = firmware_bin_findstr(blob, mtd->info.size, idstr)) != NULL) {
                    hit += strlen(idstr);  /* Adcance past marker */
                    strncpy(image_entry->version, hit, sizeof(image_entry->version));
                    image_entry->version[VTSS_APPL_FIRMWARE_VERSION_LEN] = 0;

                    if ((split = strstr(image_entry->version, (idstr = " - built "))) != NULL) {
                        *split = '\0';  /* Split version string */
                        split += strlen(idstr);  /* Advance past marker */
                        strncpy(image_entry->built_date, split, sizeof(image_entry->built_date));
                        image_entry->built_date[VTSS_APPL_FIRMWARE_DATE_LEN] = 0;
                        if ((split = strchr(image_entry->built_date, '\n'))) {
                            *split = '\0';  /* Nuke newline */
                        }
                        rc = VTSS_RC_OK;
                    } else {
                        T_E("Split failed?");
                    }
                } else {
                    T_E("RedBoot special idstr [%s] not found!\n", idstr);
                }
            } else {
                T_E("Read RedBoot failed [%s]!\n", strerror(errno));
            }
            VTSS_FREE(blob);
        } else {
            T_E("blob malloc failed!\n");
            rc = FIRMWARE_ERROR_MALLOC;
        }
    } else {
        T_E("mtd:%s size is %d!\n", mtd->dev, mtd->info.size);
    }
    vtss_mtd_close(mtd);
    return rc;
}

static mesa_rc
_firmware_bootloader_status_get(vtss_appl_firmware_status_image_t *image_entry)
{
    char buf[4096];
    memset(buf, '\0', 4096);
    image_entry->version[0] = image_entry->built_date[0] = image_entry->code_revision[0] = '\0';
    auto vers = vtss_uboot_get_env("ver");
    if (!vers.valid()) {
        return VTSS_RC_ERROR;
    }

    const char *split_version = strstr(vers.get().c_str(), " ");
    if (split_version == NULL || ++split_version == NULL)
        return VTSS_RC_ERROR;

    const char *split_date = strstr(split_version, " ");
    if (split_date == NULL || ++split_date == NULL)
        return VTSS_RC_ERROR;

    strncpy(image_entry->version, split_version, split_date - split_version);
    strncpy(image_entry->built_date, split_date, sizeof(image_entry->built_date));
    return VTSS_RC_OK;
}


mesa_rc _firmware_parse_metadata(vtss_appl_firmware_status_image_t *imageEntry, u8 *blob, size_t length)
{
    const char *delim = "\n";
    char *ptr;
    blob[length] = '\0';
    char *saveptr; // Local strtok_r() context
    for (ptr = strtok_r((char*)blob, delim, &saveptr); ptr != NULL; ptr = strtok_r(NULL, delim, &saveptr)) {
        char *data = strchr(ptr, ':');
        if (data) {
            *data++ = '\0';
            while(*data == ' ')
                data++;
            T_D("%s: metadata [%s,%s]", imageEntry->name, ptr, data);
            if        (strcmp(ptr, "Version") == 0) {
                strncpy(imageEntry->version, data, sizeof(imageEntry->version));
            } else if (strcmp(ptr, "Date") == 0) {
                strncpy(imageEntry->built_date, data, sizeof(imageEntry->built_date));
            } else if (strcmp(ptr, "Revision") == 0) {
                strncpy(imageEntry->code_revision, data, sizeof(imageEntry->code_revision));
            }
        }
    }
    return VTSS_RC_OK;
}

static
void _firmware_get_version_from_itb_dev(char* buf,
                                        uint32_t buf_size,
                                        vtss_appl_firmware_status_image_t *image_entry)
{
    char *match = (char *)memmem(buf, buf_size, "FIT", 3); // The description starts with the word "FIT"
    uint32_t size = buf_size-(match-buf);
    image_entry->version[0] = 0;
    image_entry->code_revision[0] = 0;
    image_entry->built_date[0] = 0;
    /* First, cut up string in small strings */
    T_D("Description: %s", match);
    char *itr = match;
    while (itr) {
        itr = strchr(itr, '\n');
        if (itr) {
            *itr=0;
            itr++;
        }
    }

    while (match && (!image_entry->version[0] || !image_entry->code_revision[0] || !image_entry->built_date[0])) {
        match = (char*)memmem(match, size, MAGIC_ID, strlen(MAGIC_ID));
        if (match) {
            if (strlen(match) > strlen(MAGIC_ID_VERS) &&
                strncmp(match, MAGIC_ID_VERS, strlen(MAGIC_ID_VERS)) == 0) {
                // Found version info
                T_D("Found Version: %s", match);
                strncpy(image_entry->version, match+sizeof(MAGIC_ID_VERS)-1, sizeof(image_entry->version));
            } else if (strlen(match) > strlen(MAGIC_ID_REV) &&
                       strncmp(match, MAGIC_ID_REV, strlen(MAGIC_ID_REV)) == 0) {
                // Found revision info
                T_D("Found Revision: %s", match);
                strncpy(image_entry->code_revision, match+sizeof(MAGIC_ID_REV)-1, sizeof(image_entry->code_revision));
            } else if (strlen(match) > strlen(MAGIC_ID_DATE) &&
                       strncmp(match, MAGIC_ID_DATE, strlen(MAGIC_ID_DATE)) == 0) {
                // Found built date info
                T_D("Found Date: %s", match);
                strncpy(image_entry->built_date, match+sizeof(MAGIC_ID_DATE)-1, sizeof(image_entry->built_date));
            }
            match = match + strlen(MAGIC_ID);
            size = buf_size - (match - buf);
        }
    }
}

mesa_rc
_firmware_vimage_status_get(vtss_mtd_t                        *mtd,
                            vtss_appl_firmware_status_image_t *imageEntry)
{
    mesa_rc rc = FIRMWARE_ERROR_INVALID;
    imageEntry->version[0] = imageEntry->built_date[0] = imageEntry->code_revision[0] = '\0';
    size_t hdrlen = sizeof(mscc_firmware_vimage_t)+1000;
    u8 *hdrbuf = (u8*) VTSS_MALLOC(hdrlen);
    if (mtd->info.size && hdrbuf) {
        if (read(mtd->fd, hdrbuf, hdrlen) == hdrlen) {
            // New style image
            mscc_firmware_vimage_t *fw = (mscc_firmware_vimage_t *) hdrbuf;
            const char *errmsg;
            if (mscc_vimage_hdrcheck(fw, &errmsg) == 0) {
                mscc_firmware_vimage_tlv_t tlv;
                off_t toff;
                if ((toff = mscc_vimage_mtd_find_tlv(fw, mtd, &tlv, MSCC_STAGE1_TLV_METADATA)) != 0) {
                    // NB: Reusing hdrbuf/fw
                    hdrbuf = (u8 *) VTSS_REALLOC(hdrbuf, tlv.data_len + 1);
                    if (hdrbuf &&
                        pread(mtd->fd, hdrbuf, tlv.data_len, toff) == tlv.data_len) {
                        hdrbuf[tlv.data_len] = '\0';
                        rc = _firmware_parse_metadata(imageEntry, hdrbuf, tlv.data_len);
                    }
                }
            } else if (hdrbuf[0] == 0xd0 && hdrbuf[1] == 0x0d &&
                       hdrbuf[2] == 0xfe && hdrbuf[3] == 0xed) {
                T_D("Found itb dev for: %s", mtd->name);
                _firmware_get_version_from_itb_dev((char*)hdrbuf, hdrlen, imageEntry);
            } else {
                T_D("Invalid unified image: %s", errmsg);
            }
        } else {
            T_E("Read \"%s\" image header failed [%s]!\n", imageEntry->name, strerror(errno));
        }
    }
    if (hdrbuf) {
        VTSS_FREE(hdrbuf);
    }
    vtss_mtd_close(mtd);
    return rc;
}

static mesa_rc _firmware_get_version_from_tmp_mount(
    const char *tmp_mnt_point,
    const char *filepath,
    vtss_appl_firmware_status_image_t *image_entry)
{
    char switch_appl_file[256];
    char *the_switch_appl_file;
    int fd;
    struct stat st = {0};

    sprintf(switch_appl_file, "%s%s", tmp_mnt_point, filepath);

    if ((fd = open(switch_appl_file, O_RDONLY)) < 0) {
        T_D("Could not open %s for reading.\n", switch_appl_file );
        return VTSS_RC_ERROR;
    }
    if (fstat(fd, &st) < 0) {
        T_E("Could not determine size of %s.\n", switch_appl_file );
    } else {
        if ((the_switch_appl_file = (char *)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
            T_E("Could not map %s.\n", switch_appl_file );
        } else {
            const char *match = the_switch_appl_file;
            int size = st.st_size;
            while (match && (!image_entry->version[0] || !image_entry->code_revision[0] || !image_entry->built_date[0])) {
                match = (char*)memmem(match, size, MAGIC_ID, strlen(MAGIC_ID));
                if (match) {
                    if (strlen(match) > strlen(MAGIC_ID_VERS) &&
                        strncmp(match, MAGIC_ID_VERS, strlen(MAGIC_ID_VERS)) == 0) {
                        // Found version info
                        strncpy(image_entry->version, match+sizeof(MAGIC_ID_VERS)-1, sizeof(image_entry->version));
                    } else if (strlen(match) > strlen(MAGIC_ID_REV) &&
                               strncmp(match, MAGIC_ID_REV, strlen(MAGIC_ID_REV)) == 0) {
                        // Found revision info
                        strncpy(image_entry->code_revision, match+sizeof(MAGIC_ID_REV)-1, sizeof(image_entry->code_revision));
                    } else if (strlen(match) > strlen(MAGIC_ID_DATE) &&
                               strncmp(match, MAGIC_ID_DATE, strlen(MAGIC_ID_DATE)) == 0) {
                        // Found built date info
                        strncpy(image_entry->built_date, match+sizeof(MAGIC_ID_DATE)-1, sizeof(image_entry->built_date));
                    }
                    match = match + strlen(MAGIC_ID);
                    size = st.st_size - (match - the_switch_appl_file);
                }
            }

            (void) munmap(the_switch_appl_file, st.st_size);
        }
    }
    if (close(fd)!=0) {
        T_E("close file failed");
    }
    return VTSS_RC_OK;
}

static mesa_rc _firmware_ubi_status_get(const char *name,
                                        vtss_appl_firmware_status_image_t *image_entry)
{

    Firmware_ubi ubi(name, -1, 0, 2048);
    struct stat st = {0};
    char tmp_mnt_point[64];
    if (name == firmware_fis_act()) {
        image_entry->type = (firmware_fis_prim() == name) ?
                VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ACTIVE_FIRMWARE :
                VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE;
        strncpy(image_entry->name, name, sizeof(image_entry->name));
        strncpy(image_entry->version, misc_software_version_txt(), sizeof(image_entry->version));
        strncpy(image_entry->code_revision, misc_software_code_revision_txt(), sizeof(image_entry->code_revision));
        strncpy(image_entry->built_date, misc_software_date_txt(), sizeof(image_entry->built_date));
        return VTSS_RC_OK;
    }

    sprintf(tmp_mnt_point, "/tmp/%s", name);
    if (stat(tmp_mnt_point, &st) == -1) {
        (void)mkdir(tmp_mnt_point, 0777);
    }

    if (ubi.ubiattach() != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    if (ubi.ubimount("rootfs", tmp_mnt_point) != VTSS_RC_OK) {
        ubi.ubidetach();
        return VTSS_RC_ERROR;
    }
    strcpy(image_entry->name, name);
    (void)_firmware_get_version_from_tmp_mount(tmp_mnt_point, "/usr/bin/switch_app", image_entry);

    if (umount(tmp_mnt_point)!=0) {
        T_E("umount failed");
    }
    if (ubi.ubidetach() != VTSS_RC_OK) {
        T_E("detach failed");
    }
    return VTSS_RC_OK;
}

static mesa_rc _firmware_mmc_status_get(const char *name,
                                        vtss_appl_firmware_status_image_t *image_entry)
{
    if (name == firmware_fis_act()) {
        image_entry->type = (firmware_fis_prim() == name) ?
                VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ACTIVE_FIRMWARE :
                VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE;
        strncpy(image_entry->name, name, sizeof(image_entry->name));
        strncpy(image_entry->version, misc_software_version_txt(), sizeof(image_entry->version));
        strncpy(image_entry->code_revision, misc_software_code_revision_txt(), sizeof(image_entry->code_revision));
        strncpy(image_entry->built_date, misc_software_date_txt(), sizeof(image_entry->built_date));
        return VTSS_RC_OK;
    }

    struct stat st = {0};
    char device_name[256];
    char tmp_mnt_point[256];

    sprintf(tmp_mnt_point, "/tmp/%s", name);
    if (stat(tmp_mnt_point, &st) == -1) {
        (void)mkdir(tmp_mnt_point, 0777);
    }
    sprintf(device_name, "/dev/%s", name);
    if (mount(device_name, tmp_mnt_point, "ext4", MS_RDONLY, nullptr) != 0) {
        T_D("Could not mount %s on %s", device_name, tmp_mnt_point); // Need not be an error, happens if only one partition contains something meaningful
        return VTSS_RC_ERROR;
    }

    strcpy(image_entry->name, name);
    (void)_firmware_get_version_from_tmp_mount(tmp_mnt_point, "/usr/bin/switch_app", image_entry);
    
    if (umount2(tmp_mnt_point, MNT_DETACH) !=0 ) {
        T_E("umount failed: %s", strerror(errno));
    }
    return VTSS_RC_OK;
}

/**
 * \brief Get firmware image entry
 *
 * To read status of each firmware image.
 *
 * \param image_id    [IN]  (key) Image ID
 * \param image_entry [OUT] The current status of the image
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_status_image_entry_get(u32 image_id,
                                                  vtss_appl_firmware_status_image_t *const image_entry)
{
    version_information_available.wait();

    /* check parameter */
    if (image_id > VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE) {
        T_D("Invalid image_id( %u )", image_id);
        return VTSS_RC_ERROR;
    }
    if (g_image_status[image_id].type == image_id) {
        memcpy(image_entry, &g_image_status[image_id], sizeof(*image_entry));
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

static mesa_rc vtss_appl_firmware_status_image_entry_init(u32 image_id,
                                                          vtss_appl_firmware_status_image_t *const image_entry)
{
    mesa_rc rc;
    vtss_mtd_t mtd;
    const char *fis_name;

    /* check parameter */
    if (image_id > VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE) {
        T_D("Invalid image_id( %u )", image_id);
        return VTSS_RC_ERROR;
    }
    memset(image_entry, 0, sizeof(vtss_appl_firmware_status_image_t));
    image_entry->type = static_cast<vtss_appl_firmware_status_image_type_t>(image_id);
    switch ( image_id ) {
        case VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_BOOTLOADER:
            if (conf_is_uboot()) {
                strcpy(image_entry->name, "UBoot");
                return _firmware_bootloader_status_get(image_entry);
            } else {
                if ( (rc = vtss_mtd_open(&mtd, (fis_name = "RedBoot"))) != VTSS_RC_OK &&
                     (rc = vtss_mtd_open(&mtd, (fis_name = "Redboot"))) != VTSS_RC_OK &&
                     (rc = vtss_mtd_open(&mtd, (fis_name = "redboot"))) != VTSS_RC_OK) {
                    T_D("%s (%u) cannot be located", fis_name, image_id);
                    return FIRMWARE_ERROR_IMAGE_NOT_FOUND;
                }
                strcpy(image_entry->name, "RedBoot");
                return _firmware_bootloader_status_get(&mtd, image_entry);
            }
            break;
        case VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ACTIVE_FIRMWARE:
        case VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE:
            fis_name = (image_id == VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ACTIVE_FIRMWARE) ?
                    firmware_fis_prim() : firmware_fis_bak();
            if (fis_name == fw_fis_linux || fis_name == fw_fis_linux_bk) {
                // SW is in nor-only or split between nor/nand
                if ( (rc = vtss_mtd_open(&mtd, fis_name)) != VTSS_RC_OK) {
                    T_D("%s (%u) cannot be located", fis_name, image_id);
                    return FIRMWARE_ERROR_IMAGE_NOT_FOUND;
                }
                strcpy(image_entry->name, fis_name);
                return _firmware_vimage_status_get(&mtd, image_entry);
            } else if (firmware_is_nand_only()) {
                // SW is nand only
                return _firmware_ubi_status_get(fis_name, image_entry);
            } else if (firmware_is_mmc()) {
                return _firmware_mmc_status_get(fis_name, image_entry);
            }

            return VTSS_RC_ERROR;
        default:
            T_D("ImageId (%u) not available", image_id);
            return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get firmware image entry
 *
 * To read firmware status of each switch.
 *
 * \param switch_id    [IN]  (key) Image number, starts from 1 and 1 is always the boot loader
 * \param imageEntry [OUT] The current status of the image
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_status_switch_entry_get(
    u32                                 switch_id,
    vtss_appl_firmware_status_switch_t  *const switch_entry
)
{
    vtss_usid_t         usid;
    msg_switch_info_t   info;

    usid = (vtss_usid_t)switch_id;

    if (usid < VTSS_USID_START || usid >= VTSS_USID_END) {
        T_D("usid out of range");
        return VTSS_RC_ERROR;
    }
    vtss_isid_t isid = topo_usid2isid(usid);
    if (msg_switch_info_get(isid, &info) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    if (switch_entry == NULL) {
        T_D("switch_entry == NULL");
        return VTSS_RC_ERROR;
    }

    memset(switch_entry, 0, sizeof(vtss_appl_firmware_status_switch_t));

    snprintf(switch_entry->chip_id, VTSS_APPL_FIRMWARE_CHIP_ID_LEN,       "%s", misc_chip_id_txt());
    snprintf(switch_entry->board_type, VTSS_APPL_FIRMWARE_BOARD_TYPE_LEN, "%s", misc_board_name());
    switch_entry->port_cnt = info.info.port_cnt;
    snprintf(switch_entry->product, VTSS_APPL_FIRMWARE_PRODUCT_LEN, "%s", info.product_name);
    snprintf(switch_entry->version, VTSS_APPL_FIRMWARE_VERSION_LEN, "%s", misc_software_version_txt());
    snprintf(switch_entry->built_date, VTSS_APPL_FIRMWARE_DATE_LEN, "%s", misc_software_date_txt());

    return VTSS_RC_OK;
}

/**
 * \brief Get firmware status to image upload
 *
 * To get the status of current upload operation.
 *
 * \param status [OUT] The status of current upload operation.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_status_image_upload_get(
    vtss_appl_firmware_status_image_upload_t    *const status
)
{
    const char *firmware_status = firmware_status_get();

    /* check parameter */
    if (status == NULL) {
        T_D("status == NULL");
        return VTSS_RC_ERROR;
    }

    T_D("Trying to get image_upload status!");
    SIMPLE_LOCK();
    if (!g_image_upload.upload) {
        if (!strcmp(firmware_status, "idle")) {
            status->status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_NONE;
        } else if (!strncmp(firmware_status, "Error", 5)) {
            status->status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_UNKNOWN_IMAGE;
        } else {
            status->status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_IN_PROGRESS;
        }
    } else {
        status->status = g_image_upload.upload_status;
    }
    SIMPLE_UNLOCK();

    return VTSS_RC_OK;
}

/**
 * \brief Get global parameters of firmware control
 *
 * \param globals [OUT] The global parameters.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_control_globals_get(
    vtss_appl_firmware_control_globals_t            *const globals
)
{
    /* check parameter */
    if ( globals == NULL ) {
        T_D("globals == NULL");
        return VTSS_RC_ERROR;
    }

    globals->swap_firmware = FALSE;
    return VTSS_RC_OK;
}

/**
 * \brief Set global parameters of firmware control
 *
 * \param globals [IN] The global parameters.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_control_globals_set(
    const vtss_appl_firmware_control_globals_t      *const globals
)
{



    mesa_restart_t restart_type = MESA_RESTART_COOL;


    T_D("Trying to set control_globals!");
    /* check parameter */
    if ( globals == NULL ) {
        T_D("globals == NULL");
        return VTSS_RC_ERROR;
    }

    if ( globals->swap_firmware ) {
        /* works only when active image is "linux" &&
         * alternative image "linux.bk" is available
         */
        if (strcmp("linux", firmware_fis_act()) == 0) {
            T_D("Active image is: %s", "linux");
            if (firmware_has_alt()) {
                if (firmware_swap_images() == VTSS_RC_OK) {
                    T_D("Swap firmware ok!");
                    /* Now restart */
                    if (vtss_switch_standalone()) {
                        (void) control_system_reset(TRUE, VTSS_USID_ALL, restart_type);
                    } else {
                        (void) control_system_reset(FALSE, VTSS_USID_ALL, restart_type);
                    }
                    return VTSS_RC_OK;
                } else {
                    T_D("Swap firmware failed!");
                }
            } else {
                T_D("%s not available", "linux.bk");
            }
        } else {
            T_D("Active image is %s, expect %s", firmware_fis_act(), "linux");
        }
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

/**
 * \brief Get firmware image upload status
 *
 * To get the configuration and status of current upload operation.
 *
 * \param image [OUT] The configuration and status of current upload operation.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_control_image_upload_get(
    vtss_appl_firmware_control_image_upload_t           *const image
)
{
    /* check parameter */
    if (image == NULL) {
        T_D("image == NULL\n");
        return VTSS_RC_ERROR;
    }

    SIMPLE_LOCK();

    g_image_upload.upload = FALSE;
    memcpy( image, &g_image_upload, sizeof(vtss_appl_firmware_control_image_upload_t) );

    SIMPLE_UNLOCK();

    return VTSS_RC_OK;
}

/**
 * \brief Set firmware image upload
 *
 * To set the configuration to upload image.
 *
 * \param image [IN] The configuration to upload image.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_control_image_upload_set(const vtss_appl_firmware_control_image_upload_t *const image)
{
    /* check parameter */
    if (image == NULL) {
        T_D("image == NULL");
        return VTSS_RC_ERROR;
    }

    if (image->upload != TRUE && image->upload != FALSE) {
        T_D("invalid image->upload (%u)", image->upload);
        return VTSS_RC_ERROR;
    }

    if (image->type > VTSS_APPL_FIRMWARE_UPLOAD_IMAGE_TYPE_FIRMWARE) {
        T_D("invalid image->type (%u)", image->type);
        return VTSS_RC_ERROR;
    }

    SIMPLE_LOCK();

    /* in uploading */
    if (g_image_upload.upload_status == VTSS_APPL_FIRMWARE_UPLOAD_STATUS_IN_PROGRESS) {
        SIMPLE_UNLOCK();
        return VTSS_RC_ERROR;
    }

    memcpy(&g_image_upload, image, sizeof(g_image_upload));
    g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_NONE;

    if (image->upload == FALSE) {
        SIMPLE_UNLOCK();
        return VTSS_RC_OK;
    }

    T_D("Start upload of: %s", g_image_upload.url);
    /* start uploading */
    g_image_upload.upload_status = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_IN_PROGRESS;

    SIMPLE_UNLOCK();

    vtss_flag_maskbits(&g_firmware_upload_thread_flag, 0);
    vtss_flag_setbits(&g_firmware_upload_thread_flag, FIRMWARE_UPLOAD_THREAD_FLAG_UPLOAD);

    return VTSS_RC_OK;
}
