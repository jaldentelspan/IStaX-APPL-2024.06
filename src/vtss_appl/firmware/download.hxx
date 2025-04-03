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

#ifndef _FIRMWARE_DOWNLOAD_H_
#define _FIRMWARE_DOWNLOAD_H_

#include <string>

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <microchip/ethernet/switch/api.h>
#include <vtss/basics/mutex.hxx>
#include "vtss_remote_file_transfer.hxx"

#include "vtss_timer_api.h"
#include "cli_io_api.h"
#include "led_api.h"
#include "firmware.h"    // XXX

#ifdef __cplusplus
extern "C" {
#endif

#define DLD_FWFN    "fwXXXXXX"
#define DLD_DIR     VTSS_FS_FLASH_DIR "dld/"
#define DLD_FILE    DLD_DIR DLD_FWFN
#define STAGE2_DIR  VTSS_FS_FLASH_DIR "stage2/"
#define STAGE2_FILE STAGE2_DIR DLD_FWFN

struct FirmwareDownload {
    ~FirmwareDownload() { reset(); }
    FirmwareDownload() { }

    void reset();
    void init(void *mem, size_t maxsize = 0);
    void attach_cli(cli_iolayer_t *io) { cli_ = io; }

    const size_t maxsize() { return maxsize_; }
    size_t length() { return length_; }
    bool write_error() { return write_error_; }
    void stderr_add(const char *p, ssize_t len) { errbuf_.append(p, len); }
    const char *last_error() { return errbuf_.c_str(); }
    void set_filename(const char *name);

    /*
     * Return the total expected chunks for this download
     */
    uint32_t get_total_chunks() { return total_chunks_; }

    /*
     * Set the total expected chunks for this download
     */
    void set_total_chunks(uint32_t total_chunks);
    
    /*
     * Return the last received chunk number
     */
    uint32_t get_last_received_chunk_number() { return curr_chunk_; }

    /*
     * Get the next expected chunk number (= last + 1)
     */
    uint32_t get_next_exp_chunk();

    /*
     * Set the chunk number as received
     */
    bool set_chunk_received(uint32_t chunk_number);

    /*
     * Check if we have received the indicated chunk nmber
     */
    bool has_chunk(uint32_t chunk_number);

    const char * filename() { return fname_.c_str(); }

    ssize_t write(const void *p, size_t);
    const unsigned char *map();
    mesa_rc download(const char *url, vtss::remote_file_options_t &transfer_options);
    mesa_rc tftp_get(const char *file, const char *server, int *err);
    mesa_rc usb_file_get(const char *file);

    mesa_rc check();
    mesa_rc update(mesa_restart_t restart = MESA_RESTART_COOL);
    mesa_rc load_nor(const char *mtdname, const char *type);
    mesa_rc load_raw(const char *mtdname);
    mesa_rc update_nand();
    mesa_rc update_mmc();
    mesa_rc update_nor_only();
    mesa_rc append_filename_tlv();

  private:
    static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *callback_context);
    void seal();    // Seal downloaded data
    mesa_rc firmware_update_stage2(const mscc_firmware_vimage_t *fw, const char *stage2_file);
    mesa_rc update_stage1_and_stage2(const char *mtd_name);
    uint8_t *mem_ = nullptr;
    size_t maxsize_ = 0;
    size_t length_ = 0;
    size_t mapsize_ = 0;
    char fsfile[128];
    std::string errbuf_, fname_;
    int fd_ = -1;
    void *map_ = NULL;
    bool write_error_ = false;
    cli_iolayer_t *cli_ = nullptr;
    uint32_t total_chunks_ = 0;
    uint32_t curr_chunk_ = 0;
};

struct FwWrap;

struct FwManager {
    static const uint32_t SESSION_ID_NONE = 0;

    friend struct FwWrap;

    enum Status { FREE, IN_USE, IN_USE_ASYNC };

    /*
     * Default ctor
     */
    FwManager();

    /*
     * This call is used to lock the FirmwareDownload instance for a "current scope"
     * invocation. The call checks if the FirmwareDownload instance is FREE, sets
     * the status to IN_USE, and returns the instance wrapped in a FwWrap instance.
     *
     * The FirmwareDownload instance is automatically released and set back to FREE
     * when the returned FwWrap instance goes out of scope in the calling code.
     * This is typically used when a local CLI user starts a firmware upgrade.
     *
     * If the FirmwareDownload instance is not FREE a FwWrap instance wrapping a
     * null pointer is returned.
     */
    FwWrap get();

    /*
     * This call is used to lock the FirmwareDownload instance for a "recurring call"
     * invocation with the given session ID. The call checks if the FirmwareDownload
     * instance is either FREE or locked by a prior call with the same session ID.
     * If it is FREE the status is set to IN_USE and the session ID is recorded.
     * If the instane is IN_USE it is checked if the session ID matches the original
     * caller who locked it. The instance is then returned wrapped in a FwWrap instance.
     *
     * This is typically used when the firmware upgrade is started by a remote
     * web client.
     *
     * The FirmwareDownload instance is *not* automatically released and set back
     * to FREE when the returned FwWrap instance goes out of scope in the calling
     * code. Instead the caller will have to manually ensure this by calling the
     * clear_session_id() function with the locking session ID.
     *
     * If the FirmwareDownload instance is not FREE and the session ID provided
     * does not match the session ID used the locking the instance a FwWrap instance
     * wrapping a null pointer is returned.
     *
     * If the caller does not re-call after expiry of timeout_secs the instance
     * is automatically released and the upgrade is aborted.
     */
    FwWrap get_for_session(uint32_t sess_id, uint32_t timeout_secs);


    /*
     * This call starts a session timer to detect if the web client disappears
     * while uploading a chunked image.
     *
     * Return true if the instance was in fact locked by the session ID provided
     * and false if not.
     */
    bool start_session_timer(uint32_t sess_id, uint32_t timeout_secs);

    /*
     * Clear the registered session ID for the FirmwareDownload instance locked by
     * a prior call to get_for_session(). This will not release the instance as this
     * will automatically be performed when the currently active FwWrap instance
     * afterwards goes out of scope in the calling code.
     *
     * Return true if the instance was in fact locked by the session ID provided
     * and false if not.
     */
    bool clear_session_id(uint32_t sess_id);

    /*
     * This call is used by the firmware thread to wait for the completion of an
     * async invocation from the web, JSON or SNMP interfaces. It shall not be
     * used by other threads.
     */
    FwWrap get_async();

    /*
     * This call is used by the web, JSON or SNMP interfaces to signal to the
     * firmware thread that it should complete the upgrade operation. It will
     * thus make the pending get_async() function call return.
     */
    void   store_async(FwWrap &&fw);

  private:
    void give_it_back() {
        vtss::lock_guard<vtss::Critd> fwlock(__FILE__, __LINE__, m_);
        assert(status != FREE);

        if (session_id_ != SESSION_ID_NONE) {
            return;
        }

        obj.reset();
        status = FREE;
    }

    static void session_timer_callback(struct vtss::Timer *timer);

    uint32_t session_id_ = SESSION_ID_NONE;
    vtss::Timer session_timer;
    vtss::Critd m_;
    vtss::condition_variable<vtss::Critd> cv_;
    Status status = FREE;
    FirmwareDownload obj;
};

extern FwManager manager;

struct FwWrap {
    friend struct FwManager;

    FwWrap() : data(nullptr) {}
    FwWrap(const FwWrap &rhs) = delete;
    FwWrap(FwWrap &&rhs) : data(rhs.data) {
        rhs.data = nullptr;
    }

    void assign(FwWrap &&rhs) {
        release();
        data = rhs.data;
        rhs.data = nullptr;
    }

    void release() {
        if (data) {
            manager.give_it_back();
        }
    }

    bool ok() { return data; }

    FirmwareDownload *operator->() {
        return data;
    }

    ~FwWrap() {
        release();
    }

  private:
    FwWrap(FirmwareDownload *d) : data(d) {}
    FirmwareDownload *data;
};

#ifdef __cplusplus
}
#endif

#endif // _FIRMWARE_DOWNLOAD_H_

