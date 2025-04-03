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

#include <sstream>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <zlib.h>

#include <vtss/basics/fd.hxx>
#include <vtss/basics/map.hxx>
#include <vtss/basics/trace_basics.hxx>
#include <vtss/basics/time.hxx>
#include <vtss/basics/notifications/process.hxx>
#include <vtss/basics/notifications/timer.hxx>
#include <vtss/basics/notifications/subject-runner.hxx>

#include "download.hxx"
#include "subject.hxx"
#include "vtss_os_wrapper.h"
#include "vtss_tftp_api.h"
#include "firmware_api.h"
#include "conf_api.h"
#include "vtss_usb.h"
#include "vtss_mtd_api.hxx"
#include "vtss_remote_file_transfer.hxx"
#include "firmware_ubi.hxx"
#include "vtss_uboot.hxx"

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
#include "eth_link_oam_api.h"   /* For ETH Link OAM module   */
#endif

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#else
/* Define dummy syslog macros */
#define S_I(fmt, ...)
#define S_W(fmt, ...)
#define S_E(fmt, ...)
#endif


#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_FIRMWARE
/* Upgrade scenarios to look out for:
   Redboot - NOR/NAND (mfi image)
   Redboot - NOR-only (mfi image)
   Uboot (MIBS) NOR/NAND (mfi image)
   Uboot (MIBS) NOR-only (mfi image)
   UBoot (Arm) NAND (ubifs image)
   UBoot (Arm) NOR (itb image)
   UBoot (Arm) eMMC (ext4.gz image)
*/

void FirmwareDownload::reset()
{
    T_D("%s begin", __FUNCTION__);
    maxsize_ = 0;
    length_ = 0;
    total_chunks_ = 0;
    curr_chunk_ = 0;
    errbuf_.clear();
    fname_.clear();

    if (map_) {
        munmap(map_, mapsize_);
        map_ = nullptr;
    }
    if (fd_ >= 0) {
        close(fd_);
        if (unlink(fsfile)) {
            T_W("Unlink %s: %s", fsfile, strerror(errno));
        } else {
            T_I("Unlinked %s", fsfile);
        }
        fd_ = -1;
    }
    write_error_ = false;
    cli_ = nullptr;
}

void FirmwareDownload::init(void *mem, size_t maxsize)
{
    T_D("%s begin", __FUNCTION__);
    reset();
    maxsize_ = maxsize;
    mem_ = (uint8_t *) mem;
    if (mem == nullptr) {
        if (firmware_is_nor_only()) {
            strncpy(fsfile, VTSS_FS_RUN_DIR DLD_FWFN, sizeof(fsfile));
            fd_ = mkstemp(fsfile);
        } else {
            int flags, res;
            (void)mkdir(DLD_DIR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            strncpy(fsfile, DLD_FILE, sizeof(fsfile));
            fd_ = mkstemp(fsfile);
            // Disabling compression at filesystem layer
            res = ioctl(fd_, FS_IOC_GETFLAGS, &flags);
            if (res != 0) {
                T_I("Failed get FS_IOC_GETFLAGS %s", strerror(errno));
            } else {
                flags &= ~FS_COMPR_FL;
                res = ioctl(fd_, FS_IOC_SETFLAGS, &flags);
                if (res != 0) {
                    T_I("Failed get disable FS compression for stage2 file %s", strerror(errno));
                }
            }
        }
        T_D("file = %s", fsfile);
    }
}

mesa_rc FirmwareDownload::check()
{
    T_D("%s begin", __FUNCTION__);
    mesa_rc rc = MESA_RC_ERROR;
    const unsigned char *buffer = map();
    if (buffer) {
        rc = firmware_check(buffer, length_);
    }
    return rc;
}

ssize_t FirmwareDownload::write(const void *p, size_t dlen)
{
    ssize_t written;
    // Handle overrun
    if (write_error_ || (maxsize_ && dlen + length_ > maxsize_)) {
        write_error_ = true;
        return -1;
    }
    if (mem_) {
        memcpy(mem_ + length_, p, dlen);
        written = dlen;
        T_N("memory write offset %zd: %zd bytes", length_, dlen);
    } else {
        written = ::write(fd_, p, dlen);
        T_N("file write offset %zd: %zd/%zd bytes", length_, dlen, written);
        if (written != dlen) {
            write_error_ = true;
        }
    }
    if (written > 0) {
        length_ += written;
    }
    return written;
}

const unsigned char *FirmwareDownload::map()
{
    if (mem_) {
        return mem_;
    } else {
        if (map_ == NULL) {
            size_t len = length();
            if (len) {
                mapsize_ = (((len-1)/getpagesize())+1) * getpagesize(); // Round up
                map_ = mmap(NULL, mapsize_, PROT_READ, MAP_PRIVATE, fd_, 0);
            }
        }
        return (const unsigned char *) map_;
    }
}

namespace vtss {
namespace notifications {
enum class DownloadState { stopped, running, completed, timeout, writerror, readerror };
struct DownloadHandler : public EventHandler {
    DownloadHandler(SubjectRunner &sr, FirmwareDownload *dld, std::string name) :
            EventHandler(&sr),
            p(&sr, name),
            dld_(dld),
            e_(this),
            e_out(this),
            e_err(this),
            timeout_(this)
    {
    }

    // Callback method for "normal" events
    void execute(Event *e) {
        auto st = p.status(e_);
        // When the process is running, then we can attach the file descriptors
        T_I("State %d, exited = %d", (int) st.state(), st.exited());
        if (st.state() == ProcessState::running) {
            state_ = DownloadState::running;
            sr->timer_add(timeout_, maxwait_);
            e_out.assign(p.fd_out_release());
            sr->event_fd_add(e_out, EventFd::READ);
            e_err.assign(p.fd_err_release());
            sr->event_fd_add(e_err, EventFd::READ);
        }

        if (st.exited()) {
            if (state_ == DownloadState::running) {
                state_ = DownloadState::completed;
            }
            // Return value
            return_code = st.exit_value();
            sr->return_when_out_of_work();
        }
    }

    // Callback method for timer events
    void execute(Timer *e) {
        if (e == &timeout_) {
            T_W("Timeout waiting for data exceeded");
            p.stop_and_wait(false); // Force stop, can't use any data
            state_ = DownloadState::timeout;
        }
    }

    // Callback method for events on one of the file-descriptors
    void execute(EventFd *e) {
        ssize_t res = ::read(e->raw(), b_, sizeof(b_));
        T_N("Read %zd bytes on fd %d", res, e->raw());
        if (res > 0) {
            sr->event_fd_add(*e, EventFd::READ);    // Call again
            if (e == &e_out) {
                if (dld_->write(b_, res) != res) { // Append data, check for overrun
                    sr->event_fd_del(*e);         // Don't listen to this fd anymore
                    T_W("Data write error at %zd, kill process %s", dld_->length(), p.name().c_str());
                    p.stop_and_wait(false); // Force stop, can't use any data
                    state_ = DownloadState::writerror;
                } else {
                    // Restart timer
                    sr->timer_del(timeout_);
                    sr->timer_add(timeout_, maxwait_);
                }
            } else if (e == &e_err) {
                dld_->stderr_add((char*)b_, res);    // Append error message
            }
        } else {
            T_I("Read return %zd, fd %d, state %d", res, e->raw(), (int) state_);
            if (e == &e_out) {
                // Kill data receive timer
                T_D("Stop data timer");
                sr->timer_del(timeout_);
            }
            if (res < 0 && state_ == DownloadState::running) {
                state_ = DownloadState::readerror;
            }
            // Close the file descriptor
            e->close();
        }
    }
    DownloadState state() { return state_; }

    Process p;
    FirmwareDownload *dld_;
    int return_code = -1;
    Event e_;
    EventFd e_out;
    EventFd e_err;
    Timer timeout_;
    const nanoseconds maxwait_ = seconds(5);
    DownloadState state_ = DownloadState::stopped;
    unsigned char b_[32*1024];    // 32k read buffer
};

}  // namespace notifications
}  // namespace vtss


static int download(std::string cmd, vtss::Vector<std::string> &arguments, FirmwareDownload *dld, vtss::notifications::DownloadState *state)
{
    vtss::notifications::SubjectRunner sr("downloader", VTSS_TRACE_MODULE_ID, true);
    vtss::notifications::DownloadHandler handler(sr, dld, cmd);
    T_D("%s begin", __FUNCTION__);

    handler.p.executable = cmd;
    handler.p.arguments = arguments;
    handler.p.status(handler.e_);
    handler.p.fd_out_capture = true;
    handler.p.fd_err_capture = true;
    handler.p.run();
    sr.run();

    *state = handler.state();

    T_D("%s done: ret %d, state %d", cmd.c_str(), handler.return_code, (int) handler.state());

    if (handler.state() == vtss::notifications::DownloadState::completed) {
        return handler.return_code;
    }

    const char *reason;
    switch (handler.state()) {
        case vtss::notifications::DownloadState::readerror:
            reason = "Read error";
            break;
        case vtss::notifications::DownloadState::writerror:
            reason = "Write overflow";
            break;
        case vtss::notifications::DownloadState::timeout:
            reason = "Timeout";
            break;
        default:
            reason = "Unknown error";
    }
    dld->stderr_add(reason, strlen(reason));

    // return non-zero always
    return handler.return_code ? handler.return_code : -1;
}

mesa_rc FirmwareDownload::tftp_get(const char *file, const char *server, int *err)
{
    vtss::Vector<std::string> args = {"-g", "-l", "-", "-r", file, server};
    vtss::notifications::DownloadState dlstate;

    T_D("tftp get %s %s", file, server);

    int ret = ::download("/usr/bin/tftp", args, this, &dlstate);

    if (dlstate == vtss::notifications::DownloadState::writerror) {
        *err = VTSS_TFTP_TOOLARGE;
        return MESA_RC_ERROR;
    }

    if (ret) {
        T_D("TFTP: command failed: %d", ret);
        *err = vtss_tftp_err(errbuf_.c_str());
        return MESA_RC_ERROR;
    }

    return MESA_RC_OK;
}

mesa_rc FirmwareDownload::usb_file_get(const char *file)
{
    char buf[128];
    ssize_t len;
    int usb_file;
    char fullpath[128];

    snprintf(fullpath, sizeof(fullpath), "%s/%s", USB_DEVICE_DIR, file);
    usb_file = ::open(fullpath, O_RDONLY);

    T_D("Reading file %s (from USB) into temp file %s", fullpath, fsfile);
    if (usb_file >= 0 && fd_ >= 0) {
        while ((len = ::read(usb_file, buf, sizeof(buf)))) {
            ::write(fd_, buf, len);
            length_ += len;
        }
        ::close(usb_file);
        return MESA_RC_OK;
    } else {
        return MESA_RC_ERROR;
    }
}

void FirmwareDownload::seal()
{
    if (fd_ >= 0) {
        close(fd_);
        fd_ = open(fsfile, O_RDONLY);
        assert(fd_ >= 0);
        T_D("Sealed, fd = %d, file = %s", fd_, fname_.c_str());
    }
}

void FirmwareDownload::set_filename(const char *url)
{
    const char *fname = strrchr(url, '/');
    if (!fname) {
        fname = url;
    } else {
        fname += 1;
        if (!*fname) {
            fname = "<unknown>";
        }
    }
    fname_.assign(fname);
}

void FirmwareDownload::set_total_chunks(uint32_t total_chunks)
{
    total_chunks_ = total_chunks;
    curr_chunk_ = 0;
}

uint32_t FirmwareDownload::get_next_exp_chunk()
{
    if (curr_chunk_ < total_chunks_) {
        return curr_chunk_ + 1;
    }

    return 0;
}

bool FirmwareDownload::set_chunk_received(uint32_t chunk_number)
{
    if (chunk_number < 1 || total_chunks_ < chunk_number) {
        return false;
    }

    curr_chunk_ = chunk_number;
    return true;
}

bool FirmwareDownload::has_chunk(uint32_t chunk_number)
{
    if (chunk_number < 1 || total_chunks_ < chunk_number) {
        return false;
    }

    return curr_chunk_ >= chunk_number;
}


/*
 * Append a filename TLV for NOR-only systems as this information is otherwise
 * not present (on a NAND system it is saved in a sideband TLV)
 */
mesa_rc FirmwareDownload::append_filename_tlv()
{
    if (!firmware_is_nor_only()) {
        // We only need to do this for a NOR-only system
        return MESA_RC_OK;
    }

    T_I("Appending TLV for filename %s", filename());
    mscc_firmware_vimage_stage2_tlv_t *s2tlv = nullptr;
    uint32_t tlvlen = 0;
    s2tlv = mscc_vimage_stage2_filename_tlv_create(filename(), &tlvlen);
    if (s2tlv != nullptr && tlvlen > 0) {
        write(s2tlv, tlvlen);
        mscc_vimage_stage2_filename_tlv_delete(s2tlv);
    }

    return MESA_RC_OK;
}

/*
 * File download callback - write received bytes
 */
size_t FirmwareDownload::write_callback(char *ptr, size_t size, size_t nmemb, void *callback_context)
{
    ssize_t total_size;
    ssize_t total_written;

    if (callback_context == nullptr) {
        return 0;
    }

    // get our object instance context
    auto fw = (FirmwareDownload *)callback_context;

    total_size = size * nmemb;
    T_N("Got %u * %u bytes = %u bytes", nmemb, size, total_size);
    
    if (total_size == 0) {
        return 0;
    }

    // write data
    total_written = fw->write(ptr, total_size);

    if (total_written != total_size) {
        T_D("Write mismatch, got %u bytes, wrote %u bytes", total_size, total_written);
    }

    return total_written;
}

mesa_rc FirmwareDownload::download(const char *url, vtss::remote_file_options_t &transfer_options)
{
    T_D("%s begin", __FUNCTION__);
    mesa_rc rc = MESA_RC_ERROR;
    misc_url_parts_t url_parts;
    int err = 0;

    // Save filename
    set_filename(url);
    if (cli_) {
        cli_io_printf(cli_, "Downloading...\n");
    } else {
        T_I("Downloading...\n");
    }
    // Parse url and download
    misc_url_parts_init(&url_parts,
                        MISC_URL_PROTOCOL_TFTP |
                        MISC_URL_PROTOCOL_FTP | 
                        MISC_URL_PROTOCOL_HTTP | 
                        MISC_URL_PROTOCOL_SFTP | 
                        MISC_URL_PROTOCOL_SCP |
                        MISC_URL_PROTOCOL_USB);

    if (!misc_url_decompose(url, &url_parts)) {
        if (cli_) {
            cli_io_printf(cli_, "Invalid URL or protocol\n");
        }
        return MESA_RC_ERROR;
    }

    if(url_parts.protocol_id == MISC_URL_PROTOCOL_TFTP) {
        // If we are downloading firmware with tftp, we chose to use the "Old" method 
        // tftp_get because it is much faster than using remote_file_get_chunked.
        int err;
        rc = tftp_get(url_parts.path, url_parts.host, &err);
    } else if (url_parts.protocol_id == MISC_URL_PROTOCOL_USB) {
        rc = usb_file_get(url_parts.path);
    } else {
        if (vtss::remote_file_get_chunked(&url_parts, write_callback, this, transfer_options, &err)) {
            rc = MESA_RC_OK;
        } else {
            rc = MESA_RC_ERROR;
            errbuf_ = vtss::remote_file_errstring_get(err);
        }
    }

    // CLI or trace log
    if (rc == MESA_RC_OK) {
        // Append a TLV for the filename if necessary
        append_filename_tlv();

        // Seal data if in a file
        seal();

        if (cli_) {
            cli_io_printf(cli_, "Got %zd bytes\n", length_);
        } else {
            T_I("Got %zd bytes\n", length_);
        }
    } else {
        if (cli_) {
            cli_io_printf(cli_, "Download failed: %s\n", last_error());
        } else {
            T_I("Download failed: %s", last_error());
        }
    }
    return rc;
}

mesa_rc FirmwareDownload::firmware_update_stage2(const mscc_firmware_vimage_t *fw, const char *stage2_file)
{
    T_D("%s begin", __FUNCTION__);
    mesa_rc rc = VTSS_RC_OK;
    size_t stage2_size;
    const unsigned char *stage2_ptr;

    if (length_ < fw->imglen) {
        T_D("Invalid image %zu %u", length_, fw->imglen);
        return FIRMWARE_ERROR_INVALID;
    }

    stage2_ptr = ((const unsigned char *)fw) + fw->imglen;
    stage2_size = length_ - fw->imglen;

    if (!stage2_size) {
        return FIRMWARE_ERROR_NO_STAGE2;
    }

    // Update bootloader, if includedd
    rc = firmware_update_stage2_bootloader(cli_, stage2_ptr, stage2_size);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    (void)mkdir(FIRWARE_STAGE2, 0755);
    unlink(stage2_file);    // Be sure file does not exist prior to link
    if (fw->version == 1) {
        int fd = open(stage2_file, O_WRONLY|O_CREAT|O_TRUNC|O_EXCL, S_IRUSR|S_IWUSR);
        if (fd >= 0) {
            // Disabling compression at filesystem layer
            int flags, res;
            ssize_t written;
            res = ioctl(fd, FS_IOC_GETFLAGS, &flags);
            if (res != 0) {
                T_I("Failed get FS_IOC_GETFLAGS %s", strerror(errno));
            } else {
                flags &= ~FS_COMPR_FL;
                res = ioctl(fd, FS_IOC_SETFLAGS, &flags);
                if (res != 0) {
                    T_I("Failed get disable FS compression for stage2 file %s", strerror(errno));
                }
            }
            T_I("Writing stage2 file %s", stage2_file);
            written = ::write(fd, stage2_ptr, stage2_size);
            if (written != stage2_size) {
                T_E("Write error %s: Wrote %zd, expected %zd", strerror(errno), written, stage2_size);
                rc = FIRMWARE_ERROR_FLASH_PROGRAM;
            } else {
                T_I("Wrote %zd bytes for stage2 in %s", written, stage2_file);
                rc = VTSS_RC_OK;
            }
            close(fd);
        } else {
            T_E("%s: open(%s)", strerror(errno), stage2_file);
            return FIRMWARE_ERROR_FLASH_PROGRAM;
        }
    } else {
        T_I("Linking stage2 download %s to image file %s", fsfile, stage2_file);
        if (link(fsfile, stage2_file) != 0) {
            T_E("%s: link(%s,%s)", strerror(errno), fsfile, stage2_file);
            rc = FIRMWARE_ERROR_FLASH_PROGRAM;
        }
    }

    return rc;
}

// Update firmware image.  Depending on the rootfs_data type, the
// image is either stored in entirety (NOR only) - or split with stage
// 1+2 in NAND (where already downloaded to) - and stage1 copied to NOR.
mesa_rc FirmwareDownload::update_stage1_and_stage2(const char *mtd_name)
{
    T_D("%s begin", __FUNCTION__);
    int rc = VTSS_RC_OK;
    if (firmware_is_nor_only()) {
        T_D("NOR rootfs detected - updating %s directly", mtd_name);
        rc = firmware_flash_mtd_if_needed(cli_, mtd_name, map(), length_);
    } else {
        T_D("NOR/NAND rootfs detected - updating %s", mtd_name);
        int fno;
        char stage2_file[sizeof(STAGE2_FILE)+1];
        const char *s2_basename;

        (void)mkdir(FIRWARE_STAGE2, 0755);
        strncpy(stage2_file, STAGE2_FILE, sizeof(stage2_file));
        fno = mkstemp(stage2_file);
        if (fno < 0) {
            T_E("%s: %s", stage2_file, strerror(errno));
            return VTSS_RC_ERROR;
        }
        close(fno);    // Only using the filename, not the fd

        // Assumed caller has called 'firmware_check'
        mscc_firmware_vimage_t *fw = (mscc_firmware_vimage_t *) map();

        // Write stage2 part to nand
        rc = firmware_update_stage2(fw, stage2_file);
        if (rc == FIRMWARE_ERROR_NO_STAGE2) {
            s2_basename = NULL;
        } else if (rc == VTSS_RC_OK) {
            s2_basename = stage2_file + strlen(STAGE2_DIR);
        } else {
            goto EXIT;
        }

        // Write stage1 to NOR flash
        rc = firmware_update_stage1(cli_, map(), fw->imglen, mtd_name, filename(), s2_basename);

  EXIT:
        // Regardless of success, clean-up the stage2 components (simply delete
        // everything that has no pointers), for the MTD partitions applicable
        firmware_stage2_cleanup();
    }

    return rc;
}

mesa_rc FirmwareDownload::load_nor(const char *mtdname, const char *type)
{
    T_D("%s begin", __FUNCTION__);
    cli_io_printf(cli_, "Writing %s image\n", type);
    mesa_rc rc = update_stage1_and_stage2(mtdname);
    if (rc == MESA_RC_OK) {
        cli_io_printf(cli_, "  Done\n");
    } else {
        cli_io_printf(cli_, "  Failed: %s\n", error_txt(rc));
    }
    return rc;
}

mesa_rc FirmwareDownload::load_raw(const char *mtdname) {
    return firmware_flash_mtd(cli_, mtdname, map(), length_);
}

/* ZLIB_SIZE is the block size used by the zlib routines. */
#define ZLIB_SIZE 0x4000

mesa_rc FirmwareDownload::update_mmc() {
    z_stream z_str = {0};
    int zlib_rc;
    const unsigned char *zipped_map = map();

    T_D("%s begin", __FUNCTION__);
    if (length_ < 4) {
        T_D("Invalid length: %d", length_);
        return FIRMWARE_ERROR_INVALID;
    }

    // This must be a '.ext4.gz' file. Do some rough format validation
    if (zipped_map[0] != 0x1f || zipped_map[1] != 0x8b ||
        zipped_map[2] != 0x08 || zipped_map[3] != 0x00) {
        // Wrong magic
        T_D("Magic invalid, found: %02x %02x %02x %02x", zipped_map[0], zipped_map[1], zipped_map[2], zipped_map[3]);
        return FIRMWARE_ERROR_INVALID;
    }

    unsigned char zipped[ZLIB_SIZE];
    size_t map_size = length_;
    unsigned char unzipped[ZLIB_SIZE];
    int total_write = 0;
    int written = 0;

    // For explanation of constants, see https://zlib.net/manual.html
    zlib_rc = inflateInit2(&z_str, 15 + 32);
    if (zlib_rc < 0) {
        T_D("zlib failed: %d", zlib_rc);
        return VTSS_RC_ERROR;
    }
    char devname[256];
    sprintf(devname, "/dev/%s", firmware_fis_to_update());
    FILE *fd_to_update = ::fopen(devname, "r"); // Check that file exists
    if (!fd_to_update) {
        T_E("No such device: %s", devname);
        return VTSS_RC_ERROR;
    }
    ::fclose(fd_to_update);
    fd_to_update = ::fopen(devname, "w");
    if (!fd_to_update) {
        T_E("Failed to open: %s", devname);
        return VTSS_RC_ERROR;
    }
    while (map_size > 0) {
        if (map_size > ZLIB_SIZE) {
            memcpy(zipped, zipped_map, ZLIB_SIZE);
            z_str.avail_in = ZLIB_SIZE;
            z_str.next_in = zipped;
            map_size -= ZLIB_SIZE;
        } else {
            memcpy(zipped, zipped_map, map_size);
            z_str.avail_in = map_size;
            z_str.next_in = zipped;
            map_size = 0;
        }
        zipped_map += z_str.avail_in;
        z_str.avail_out = 0;
        while (z_str.avail_out == 0) {
	    unsigned result_size;
            z_str.avail_out = ZLIB_SIZE;
	    z_str.next_out = unzipped;
	    zlib_rc = inflate(&z_str, Z_NO_FLUSH);
	    switch (zlib_rc) {
                case Z_OK:
                case Z_STREAM_END:
                case Z_BUF_ERROR:
                    break;

                default:
                    inflateEnd(&z_str);
                    ::fclose(fd_to_update);
                    T_E("Failed to uncompress to: %s", devname);
                    return VTSS_RC_ERROR;
	    }
	    result_size = ZLIB_SIZE - z_str.avail_out;
            written = ::fwrite(unzipped, 1, result_size, fd_to_update);
            if (written != result_size) {
                T_E("Trying to write %d bytes, wrote %d bytes", result_size, written);
            }
            total_write += result_size;
        }
    }
    ::fclose(fd_to_update);
    inflateEnd (&z_str);
    vtss_uboot_set_env(firmware_fis_to_update(), filename());
    return VTSS_RC_OK;
}

mesa_rc FirmwareDownload::update_nand() {
    const char *bak = firmware_fis_to_update();
    T_D("%s begin", __FUNCTION__);

    if (!bak) {
        return FIRMWARE_ERROR_FLASH_PROGRAM;
    }
    Firmware_ubi ubi(bak, -1, 0, 2048);
    const unsigned char *ubi_file = map();
    if (length_ < 4) {
        return FIRMWARE_ERROR_INVALID;
    }
    // This must be a '.ubifs' file. Do some rough format validation
    if (ubi_file[0] != 0x31 || ubi_file[1] != 0x18 || ubi_file[2] != 0x10 || ubi_file[3] != 0x06) {
        // Wrong magic
        T_D("Magic invalid, found: %02x %02x %02x %02x", ubi_file[0], ubi_file[1], ubi_file[2], ubi_file[3]);
        return FIRMWARE_ERROR_INVALID;
    }

    VTSS_RC(ubi.ubiformat());
    VTSS_RC(ubi.ubiattach());
    VTSS_RC(ubi.ubimkvol(0, "rootfs"));
    VTSS_RC(ubi.ubiupdatevol(map(), length_));
    vtss_uboot_set_env(firmware_fis_to_update(), filename());
    return VTSS_RC_OK;
}

mesa_rc FirmwareDownload::update_nor_only()
{
    vtss_mtd_t mtd;
    const char *bak = firmware_fis_to_update();
    size_t written = 0;
    mesa_rc rc;
    
    T_D("%s begin", __FUNCTION__);
    if (!bak) {
        return FIRMWARE_ERROR_FLASH_PROGRAM;
    }
    const unsigned char *itb_file = map();
    if (length_ < 8) {
        return FIRMWARE_ERROR_INVALID;
    }

    // This must be an '.itb' file. Do some rough format validation
    if (itb_file[0] != 0xd0 || itb_file[1] != 0x0d ||
        itb_file[2] != 0xfe || itb_file[3] != 0xed) {
        // Wrong magic
        T_D("Magic invalid, found: %02x %02x %02x %02x", itb_file[0], itb_file[1], itb_file[2], itb_file[3]);
        return FIRMWARE_ERROR_INVALID;
    }

    uint32_t exp_length = (itb_file[4]*0x1000000) + (itb_file[5]*0x10000)
                           + (itb_file[6]*0x100) + (itb_file[7]);
    if ( exp_length > length_) {
        // Wrong size
        T_D("Wrong size: got %d bytes, expected %d bytes", length_, exp_length);
        return FIRMWARE_ERROR_INVALID;
    }

    VTSS_RC(vtss_mtd_open(&mtd, bak));
    rc = vtss_mtd_erase(&mtd, length_);
    if (rc != VTSS_RC_OK) {
        vtss_mtd_close(&mtd);
        return rc;
    }

    written = ::write(mtd.fd, map(), length_);
    vtss_mtd_close(&mtd);
    vtss_uboot_set_env(mtd.dev, filename());
    return (written == length_) ? VTSS_RC_OK : VTSS_RC_ERROR;
}

mesa_rc FirmwareDownload::update(mesa_restart_t restart) {
    mesa_rc rc;
    const char *fis_name = firmware_fis_to_update();

    T_D("Update fis: %s (type %s), new file: %s", fis_name, firmware_fis_layout(), filename());
    S_I("Upgraded to firmware %s (%s:%s)", filename(), firmware_fis_layout(), fis_name);

    /* LED state: Firmware update */
    led_front_led_state(LED_FRONT_LED_FLASHING_BOARD, TRUE);
    cli_io_printf(cli_, "Starting flash update - do not power off device!\n");

    control_system_flash_lock();
    if (firmware_is_mfi_based()) {
        rc = update_stage1_and_stage2(fis_name);
        if (rc == VTSS_RC_OK) {
            if (strcmp(fis_name, "linux.bk") == 0) {
                firmware_setstate(cli_, FWS_SWAPPING, fis_name);
                if ((firmware_swap_images()) == VTSS_RC_OK) {
                    firmware_setstate(cli_, FWS_SWAP_DONE, fis_name);
                } else {
                    firmware_setstate(cli_, FWS_SWAP_FAILED, fis_name);
                }
            }
        }
    } else if (firmware_is_nand_only()) {
        if (((rc = update_nand()) == VTSS_RC_OK) &&
            ((rc = firmware_swap_images()) == VTSS_RC_OK)) {
            firmware_setstate(cli_, FWS_SWAP_DONE, fis_name);
        } else {
            firmware_setstate(cli_, FWS_SWAP_FAILED, fis_name);
        }
    } else if (firmware_is_mmc()) {
        if (((rc = update_mmc()) == VTSS_RC_OK) &&
            ((rc = firmware_swap_images()) == VTSS_RC_OK)) {
            firmware_setstate(cli_, FWS_SWAP_DONE, fis_name);
        } else {
            firmware_setstate(cli_, FWS_SWAP_FAILED, fis_name);
        }
    } else if (firmware_is_nor_only()) {
        // nor only
        if (((rc = update_nor_only()) == VTSS_RC_OK) &&
            ((rc = firmware_swap_images()) == VTSS_RC_OK)) {
            firmware_setstate(cli_, FWS_SWAP_DONE, fis_name);
        } else {
            firmware_setstate(cli_, FWS_SWAP_FAILED, fis_name);
        }
    } else {
        rc = FIRMWARE_ERROR_CURRENT_UNKNOWN;
    }

    /* Back to prev cli system LED state */
    led_front_led_state_clear(LED_FRONT_LED_FLASHING_BOARD);

    if (rc == VTSS_RC_OK) {
        reset();    // Loose any file associated
        firmware_setstate(cli_, FWS_REBOOT, fis_name);
        control_system_flash_unlock();
        conf_wait_flush();
        control_system_reset_sync(restart);
        /* NORETURN */
    } else {
        T_D("firmware upload status: %s", error_txt(rc));
        firmware_setstate(NULL, FWS_DONE, NULL);
    }

    /* Unlock reboot/config lock */
    control_system_flash_unlock();

    return rc;
}

FwManager manager;

FwManager::FwManager() : m_("fw", VTSS_MODULE_ID_FIRMWARE) {}

FwWrap FwManager::get() {
    vtss::lock_guard<vtss::Critd> fwlock(__FILE__, __LINE__, m_);

    if (status != FREE) {
        return FwWrap(nullptr);
    }
    
    status = IN_USE;
    session_id_ = SESSION_ID_NONE;
    return FwWrap(&obj);
}

bool FwManager::start_session_timer(uint32_t sess_id, uint32_t timeout_secs)
{
    if (status == FREE || session_id_ != sess_id || timeout_secs == 0) {
        return false;
    }

    session_timer.set_repeat(false);
    session_timer.set_period(vtss::seconds(timeout_secs));
    session_timer.callback = session_timer_callback;
    session_timer.modid = VTSS_MODULE_ID_FIRMWARE;
    session_timer.user_data = this;

    if (vtss_timer_start(&session_timer) != VTSS_RC_OK) {
        T_D("vtss_timer_start() failed");
        return false;
    }

    return true;
}

void FwManager::session_timer_callback(struct vtss::Timer *timer)
{
    if (timer == nullptr || timer->user_data == nullptr) {
        return;
    }

    // This is a static member function so we need to get the manager instance from the timer
    auto manager = (FwManager*)timer->user_data;
    if (manager->status == IN_USE || manager->session_id_ != SESSION_ID_NONE) {
        T_D("Cancelling ongoing async upgrade session with ID %u", manager->session_id_);
        firmware_update_async_abort(manager->session_id_);
    }
}

FwWrap FwManager::get_for_session(uint32_t sess_id, uint32_t timeout_secs)
{
    vtss::lock_guard<vtss::Critd> fwlock(__FILE__, __LINE__, m_);

    if (status == IN_USE_ASYNC || (status == IN_USE && session_id_ != sess_id)) {
        return FwWrap(nullptr);
    }

    vtss_timer_cancel(&session_timer);

    status = IN_USE;
    session_id_ = sess_id;

    return FwWrap(&obj);
}

bool FwManager::clear_session_id(uint32_t sess_id)
{
    vtss::lock_guard<vtss::Critd> fwlock(__FILE__, __LINE__, m_);

    if (session_id_ != SESSION_ID_NONE && session_id_ != sess_id) {
        return false;
    }

    session_id_ = SESSION_ID_NONE;
    return true;
}

FwWrap FwManager::get_async() {
    vtss::unique_lock<vtss::Critd> fwlock(__FILE__, __LINE__, m_);
    while (status != IN_USE_ASYNC) {
        cv_.wait(fwlock);
    }
    return FwWrap(&obj);
}

void FwManager::store_async(FwWrap &&rhs) {
    vtss::lock_guard<vtss::Critd> fwlock(__FILE__, __LINE__, m_);
    assert(status == IN_USE);
    assert(rhs.data == &obj);
    status = IN_USE_ASYNC;
    cv_.notify_one();
    // DO NOT RESET
    rhs.data = nullptr;
}
