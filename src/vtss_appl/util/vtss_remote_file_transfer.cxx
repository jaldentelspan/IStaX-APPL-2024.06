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

/*
 * This is a set of utility functions that enable single file transfer to
 * and from remote servers.
 *
 * Supported protocols are:
 * - HTTP
 * - FTP
 * - SFTP
 * - SCP
 * - TFTP
 */

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <vtss/basics/vector.hxx>
#include <vtss/basics/tuple.hxx>
#include <vtss/basics/utility.hxx>
#include "vtss/basics/notifications/process-cmd.hxx"
#include "main.h"
#include "vtss_remote_file_transfer.hxx"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_SYSTEM
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_SYSTEM

namespace vtss
{

#define VTSS_RFILE_TRNS_SSHDIR_VAL     "/switch/.ssh/"
#define VTSS_RFILE_TRNS_SSH_HOSTS_FILE "known_hosts"
#define VTSS_RFILE_TRNS_SSH_HOSTS_PATH VTSS_RFILE_TRNS_SSHDIR_VAL VTSS_RFILE_TRNS_SSH_HOSTS_FILE

/*
 * Transfer direction
 */
enum transfer_direction_t {
    TRANSFER_GET,
    TRANSFER_PUT
};

// Typedefs for map from curl error code to MESA error code
typedef vtss::Map<CURLcode, mesa_rc>    curlrc_to_rc_map_t;

// Create error result map for curl
static curlrc_to_rc_map_t create_curl_rc_to_rc_map()
{
    curlrc_to_rc_map_t map;

    map[CURLE_OK] = VTSS_RFILE_TRNS_OK;
    map[CURLE_UNSUPPORTED_PROTOCOL] = VTSS_RFILE_TRNS_NOPROT;
    map[CURLE_URL_MALFORMAT] = VTSS_RFILE_TRNS_INVAL_URL;
    map[CURLE_COULDNT_RESOLVE_HOST] = VTSS_RFILE_TRNS_NO_HOST;
    map[CURLE_COULDNT_CONNECT] = VTSS_RFILE_TRNS_CONN;
    map[CURLE_FTP_WEIRD_SERVER_REPLY] = VTSS_RFILE_TRNS_CONN;
    map[CURLE_REMOTE_ACCESS_DENIED] = VTSS_RSFILE_TRNS_UPLOAD_FAILED;
    map[CURLE_FTP_ACCEPT_FAILED] = VTSS_RFILE_TRNS_CONN_LOST;
    map[CURLE_FTP_WEIRD_PASS_REPLY] = VTSS_RFILE_TRNS_LOGIN;
    map[CURLE_FTP_ACCEPT_TIMEOUT] = VTSS_RFILE_TRNS_CONN_TIMEOUT;
    map[CURLE_FTP_WEIRD_PASV_REPLY] = VTSS_RSFILE_TRNS_FTP_NOPASV;
    map[CURLE_HTTP_RETURNED_ERROR] = VTSS_RFILE_TRNS_PROTERR;
    map[CURLE_WRITE_ERROR] = VTSS_RFILE_TRNS_INVALID_FILENAME;
    map[CURLE_UPLOAD_FAILED] = VTSS_RSFILE_TRNS_UPLOAD_FAILED;
    map[CURLE_READ_ERROR] = VTSS_RFILE_TRNS_INVALID_FILENAME;
    map[CURLE_OPERATION_TIMEDOUT] = VTSS_RFILE_TRNS_CONN_TIMEOUT;
    map[CURLE_FTP_PORT_FAILED] = VTSS_RSFILE_TRNS_FTP_NOPORT;
    map[CURLE_SSL_CONNECT_ERROR] = VTSS_RFILE_TRNS_CONN;
    map[CURLE_FILE_COULDNT_READ_FILE] = VTSS_RFILE_TRNS_INVALID_FILENAME;
    map[CURLE_PEER_FAILED_VERIFICATION] = VTSS_RSFILE_TRNS_NO_HOSTKEY;
    map[CURLE_GOT_NOTHING] = VTSS_RSFILE_TRNS_SERVER;
    map[CURLE_SEND_ERROR] = VTSS_RFILE_TRNS_PROTERR;
    map[CURLE_RECV_ERROR] = VTSS_RFILE_TRNS_PROTERR;
    map[CURLE_FILESIZE_EXCEEDED] = VTSS_RSFILE_TRNS_SERVER;
    map[CURLE_LOGIN_DENIED] = VTSS_RFILE_TRNS_LOGIN;
    map[CURLE_TFTP_NOTFOUND] = VTSS_RFILE_TRNS_ENOTFOUND;
    map[CURLE_TFTP_PERM] = VTSS_RFILE_TRNS_EPERM;
    map[CURLE_REMOTE_DISK_FULL] = VTSS_RSFILE_TRNS_SERVER;
    map[CURLE_TFTP_ILLEGAL] = VTSS_RFILE_TRNS_NOPROT;
    map[CURLE_REMOTE_FILE_EXISTS] = VTSS_RFILE_TRNS_EPERM;
    map[CURLE_REMOTE_FILE_NOT_FOUND] = VTSS_RFILE_TRNS_ENOTFOUND;
    map[CURLE_SSH] = VTSS_RSFILE_TRNS_SSH_ERR;

    return map;
}

// Typedefs for map from error message snippet to error code
typedef vtss::Map<const char *, mesa_rc>    errstr_to_rc_map_t;
typedef errstr_to_rc_map_t::const_iterator  errstr_to_rc_map_iter_t;

// Curl string error messages map
static errstr_to_rc_map_t create_curl_str_to_rc_map()
{
    errstr_to_rc_map_t map;
    map["No route to host"] = VTSS_RFILE_TRNS_NOROUTE;
    map["Access denied"] = VTSS_RFILE_TRNS_LOGIN;
    map["Network is unreachable"] = VTSS_RFILE_TRNS_UNREACH;
    map["Connection refused"] = VTSS_RFILE_TRNS_CONN;
    map["File not found"] = VTSS_RFILE_TRNS_ENOTFOUND;
    return map;
}

// Map from generalized error code to descriptive string
typedef vtss::Map<mesa_rc, std::string>     rc_to_errstr_map_t;
typedef rc_to_errstr_map_t::iterator        rc_to_errstr_map_iter_t;

static rc_to_errstr_map_t create_rc_to_str_map()
{
    rc_to_errstr_map_t map;

    map[VTSS_RFILE_TRNS_OK]                 = "Transfer OK";
    map[VTSS_RFILE_TRNS_ERROR]              = "Unspecified error";
    map[VTSS_RFILE_TRNS_ARGS]               = "Invalid arguments";
    map[VTSS_RFILE_TRNS_NOPROT]             = "Unsupported protocol";
    map[VTSS_RFILE_TRNS_UNSUPP_METHOD]      = "Unsupported operation";
    map[VTSS_RFILE_TRNS_INVAL_URL]          = "Invalid URL";
    map[VTSS_RFILE_TRNS_PROTERR]            = "Protocol error";
    map[VTSS_RFILE_TRNS_NO_HOST]            = "Could not resolve hostname";
    map[VTSS_RFILE_TRNS_CONN]               = "Connection refused";
    map[VTSS_RFILE_TRNS_CONN_TIMEOUT]       = "Connection timed out";
    map[VTSS_RFILE_TRNS_CONN_LOST]          = "Connection lost";
    map[VTSS_RFILE_TRNS_NOROUTE]            = "No route to host";
    map[VTSS_RFILE_TRNS_UNREACH]            = "Network is unreachable";
    map[VTSS_RFILE_TRNS_NOUSERPW]           = "No username and/or password specified";
    map[VTSS_RFILE_TRNS_LOGIN]              = "Login incorrect";
    map[VTSS_RFILE_TRNS_NEEDUSERPW]         = "Need to specify username and/or password";
    map[VTSS_RFILE_TRNS_EPERM]              = "Bad file permissions";
    map[VTSS_RFILE_TRNS_ENOTFOUND]          = "File not found";
    map[VTSS_RFILE_TRNS_INVALID_PATH]       = "Invalid path";
    map[VTSS_RFILE_TRNS_INVALID_FILENAME]   = "Invalid filename";
    map[VTSS_RFILE_TRNS_TIMEOUT]            = "Operation timed out";
    map[VTSS_RSFILE_TRNS_NO_HOSTKEY]        = "Host key not in cache";
    map[VTSS_RSFILE_TRNS_CH_HOSTKEY]        = "Host key has changed";
    map[VTSS_RSFILE_TRNS_SERVER]            = "Server problem (disk, memory)";
    map[VTSS_RSFILE_TRNS_FTP_NOPASV]        = "Using FTP passive mode failed";
    map[VTSS_RSFILE_TRNS_FTP_NOPORT]        = "Using FTP active mode failed";
    map[VTSS_RSFILE_TRNS_SSH_ERR]           = "SSH error occurred";
    map[VTSS_RSFILE_TRNS_UPLOAD_FAILED]     = "File upload failed, check permissions";

    return map;
}

static errstr_to_rc_map_t curl_str_to_rc_map = create_curl_str_to_rc_map();
static curlrc_to_rc_map_t curlrc_to_rc_map = create_curl_rc_to_rc_map();

// The error code to descriptive string map.
static rc_to_errstr_map_t rc_to_str_map = create_rc_to_str_map();

// Convert a CURL error code to a MESA error code
static mesa_rc decode_curl_error_code(CURLcode curl_res)
{
    auto iter = curlrc_to_rc_map.find(curl_res);
    return (iter != curlrc_to_rc_map.end() ? iter->second : VTSS_RFILE_TRNS_ERROR);
}

// Decode a CURL error message to obtain a MESA error code
static mesa_rc decode_curl_error_message(const char *errstring, CURLcode curl_res,
                                         curl_khmatch khmatch, const char *needle = "")
{
    const char *p;

    if (errstring != nullptr && strlen(errstring) > 0) {
        if ((p = strstr(errstring, needle)) == nullptr) {
            p = errstring;
        }

        T_N("Searching in '%s'", p);
        // found start of error message
        errstr_to_rc_map_iter_t iter;
        for (iter = curl_str_to_rc_map.begin(); iter != curl_str_to_rc_map.end(); iter++) {
            T_N("Searching for '%s'", get<0>(*iter));
            if (strstr(p, get<0>(*iter)) != nullptr) {
                return get<1>(*iter);
            }
        }
    }

    if (curl_res == CURLE_PEER_FAILED_VERIFICATION && khmatch != CURLKHMATCH_OK) {
        // Fine-tune SSH hostkey error message to indicate if the key has changed
        return khmatch == CURLKHMATCH_MISMATCH ? VTSS_RSFILE_TRNS_CH_HOSTKEY : VTSS_RSFILE_TRNS_NO_HOSTKEY;

    } else {
        // Default to decoding the (less precise) CURL error code
        return decode_curl_error_code(curl_res);
    }
}

// Get descriptive error string from error code
const char *remote_file_errstring_get(int rc)
{
    auto iter = rc_to_str_map.find(rc);
    return (iter != rc_to_str_map.end() ? iter->second : rc_to_str_map[VTSS_RFILE_TRNS_ERROR]).c_str();
}

bool remote_file_ssh_environment_setup(int *err)
{
    // Ensure that the .ssh directory has been setup
    T_D("Creating .ssh data directory at '%s'", VTSS_RFILE_TRNS_SSHDIR_VAL);
    if (mkdir(VTSS_RFILE_TRNS_SSHDIR_VAL, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
        if (errno != EEXIST) {
            T_W("Cannot create '%s' directory, %s (%d)", VTSS_RFILE_TRNS_SSHDIR_VAL,
                strerror(errno), errno);
            *err = VTSS_RFILE_TRNS_ERROR;
            return false;
        } else {
            T_D(".ssh directory already exist");
        }
    }

    *err = VTSS_RFILE_TRNS_OK;
    return true;
}

/*
 * Data for SSH hostkey callback handling
 */
struct ssh_callback_data_t {
    bool save_host_keys;        // IN: Should we save unknown or changed hostkeys?
    curl_khmatch khmatch;       // OUT: Was the hostkey unknown or changed?
};

static int curl_ssh_keycallback(CURL *handle,
                                const struct curl_khkey *knownkey,
                                const struct curl_khkey *foundkey,
                                enum curl_khmatch khmatch,
                                void *data)
{
    auto callback_data = (ssh_callback_data_t *)data;

    T_D("Enter, khmatch:%u, save_host_keys:%d", khmatch, callback_data->save_host_keys);

    switch (khmatch) {
    case CURLKHMATCH_OK:
        return CURLKHSTAT_FINE;
        break;
    case CURLKHMATCH_MISMATCH:
    case CURLKHMATCH_MISSING:
        if (callback_data->save_host_keys) {
            return CURLKHSTAT_FINE_ADD_TO_FILE;
        } else {
            callback_data->khmatch = khmatch;
            return CURLKHSTAT_REJECT;
        }
        break;
    default:
        break;
    }

    return CURLKHSTAT_REJECT;
}

#define CURL_API_CALL(cmd)            \
    T_I("Invoking \"" #cmd "\"");     \
    if ((cret = (cmd)) != CURLE_OK) { \
        T_D("CURL Error: %u", cret);  \
        goto curl_transfer_api_exit;  \
    }

/*
 * Transfer file using CURL "easy" API to/from a local file
 */
static bool curl_transfer_file(transfer_direction_t direction,
                               const misc_url_parts_t *url_data,
                               const char *local_path,
                               size_t localfilelen,
                               remote_file_write_callback write_callback,
                               void *callback_context,
                               remote_file_options_t &options,
                               int *err)
{
    static const int max_url_size = 2000;
    char full_url[max_url_size];

    CURLcode cret = CURLE_OK;
    bool success = true;
    char errbuf[CURL_ERROR_SIZE];
    ssh_callback_data_t ssh_callback_data;

    vtss_clear(errbuf);
    *err = VTSS_RFILE_TRNS_OK;

    if (!remote_file_ssh_environment_setup(err)) {
        return false;
    }

    if (url_data->protocol_id == MISC_URL_PROTOCOL_SFTP || url_data->protocol_id == MISC_URL_PROTOCOL_SCP) {
        // check if we need to fix the URL path since SSH-based CURL is treating relative paths in an inconvenient way
        if ((strlen(url_data->path) == 0) || (url_data->path[0] != '~' && url_data->path[0] != '/')) {
            // a relative path was specified, prepend with '~/' to access users home directory
            char fixed_path[sizeof(url_data->path)];
            fixed_path[0] = '\0';
            strcpy(fixed_path, "~/");
            strncat(fixed_path, url_data->path, sizeof(url_data->path) - strlen(fixed_path));
            memcpy((void *)url_data->path, fixed_path, sizeof(url_data->path));

            T_D("SSH transfer: Changed URL path to '%s'", url_data->path);
        }
    }

    misc_url_compose(full_url, max_url_size, url_data);

    FILE *hfile = nullptr;

    if (direction == TRANSFER_GET) {
        T_D("GET: %s", full_url);

        if (local_path != nullptr) {
            if ((hfile = fopen(local_path, "w")) == nullptr) {
                T_D("Could not open file for writing: %s", strerror(errno));
                return false;
            }
        }
    } else {
        T_D("PUT: %s", full_url);

        if ((hfile = fopen(local_path, "r")) == nullptr) {
            T_D("Could not open file for reading: %s", strerror(errno));
            return false;
        }
    }

    CURL *handle = curl_easy_init();
    if (handle == nullptr) {
        T_D("Unable to initialize CURL API call");
        if (hfile != nullptr) {
            fclose(hfile);
        }
        return false;
    }

    CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_PROTOCOLS_STR, "http,ftp,tftp,scp,sftp"));
    CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_URL, full_url));
    CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 30L));

    if (direction == TRANSFER_GET) {
        CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_UPLOAD, 0L));
        if (hfile != nullptr) {
            // write data to local file stream
            CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_WRITEDATA, hfile));
        }
        if (write_callback != nullptr) {
            // write data using callback
            CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_callback));
            CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_WRITEDATA, callback_context));
        }
    } else {
        CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_UPLOAD, 1L));
        CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_INFILESIZE, localfilelen));
        CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_READDATA, hfile));
    }

    // Set error buffer for a more expressive error message
    CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errbuf));

    // HTTP options
    CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L));

    // FTP options
    CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_FTP_RESPONSE_TIMEOUT, 30L));
    if (options.ftp_active) {
        CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_FTPPORT, "-"));
    }

    // SSH options
    CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD));
    CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_SSH_KNOWNHOSTS, VTSS_RFILE_TRNS_SSH_HOSTS_PATH));
    CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_SSH_KEYFUNCTION, curl_ssh_keycallback));

    ssh_callback_data.khmatch = CURLKHMATCH_OK;
    ssh_callback_data.save_host_keys = options.ssh_save_host_keys;
    CURL_API_CALL(curl_easy_setopt(handle, CURLOPT_SSH_KEYDATA, &ssh_callback_data));

    // Execute the request
    T_D("Execuing request");
    CURL_API_CALL(curl_easy_perform(handle));

curl_transfer_api_exit:
    if (cret != CURLE_OK) {
        success = false;

        *err = decode_curl_error_message(errbuf, cret, ssh_callback_data.khmatch);
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
        const char *errmess = curl_easy_strerror(cret);
#endif
        T_I("CURL %s command failed: %s (%d); %s (%d)", (direction == TRANSFER_GET ? "GET" : "PUT"),
            errmess, cret, errbuf, *err);
    }

    if (handle != nullptr) {
        curl_easy_cleanup(handle);
    }

    if (hfile != nullptr) {
        fclose(hfile);
    }

    return success;
}

static bool curl_api_put_file(const misc_url_parts_t *url_data,
                              const char *local_path,
                              size_t localfilelen,
                              remote_file_options_t &options,
                              int *err)
{
    return curl_transfer_file(TRANSFER_PUT, url_data, local_path, localfilelen, nullptr, nullptr, options, err);
}

static bool curl_api_get_file(const misc_url_parts_t *url_data,
                              const char *local_path,
                              remote_file_options_t &options,
                              int *err)
{
    return curl_transfer_file(TRANSFER_GET, url_data, local_path, 0, nullptr, nullptr, options, err);
}

static bool copy_file_content_to_buffer(const char *filepath,
                                        char **buffer,
                                        size_t *buflen)
{
    bool success = false;
    int fd;
    struct stat statb;
    char *buf;

    if ((fd = open(filepath, O_RDONLY)) < 0) {
        return false;
    }

    if (fstat(fd, &statb) == 0 && statb.st_size > 0) {
        buf = (char *)VTSS_MALLOC(statb.st_size);
        if (buf != nullptr) {
            if (read(fd, buf, statb.st_size) == statb.st_size) {
                T_D("Read %zd bytes from %s", statb.st_size, filepath);
                success = true;
            } else {
                T_W("Read from temp. file failed (%s): %s", filepath, strerror(errno));
                VTSS_FREE(buf);
                buf = nullptr;
            }
        } else {
            T_W("Unable to allocate space for file content");
        }

        if (success) {
            *buffer = buf;
            *buflen = statb.st_size;
        }
    }

    close(fd);

    return success;
}

static const char *create_temp_directory(char *dirtemplate)
{
    strcpy(dirtemplate, "/tmp/fdl_XXXXXX");
    return mkdtemp(dirtemplate);
}

static bool validate_url(const misc_url_parts_t *url_data, int *err)
{
    switch (url_data->protocol_id) {
    case MISC_URL_PROTOCOL_SCP:
    case MISC_URL_PROTOCOL_SFTP:
        if (strlen(url_data->user) == 0 || strlen(url_data->pwd) == 0) {
            *err = VTSS_RFILE_TRNS_NOUSERPW;
            return false;
        }
        break;
    }

    return true;
}

bool remote_file_get(const misc_url_parts_t *url_data,
                     char **buffer,
                     size_t *buflen,
                     remote_file_options_t &options,
                     int *err)
{
    bool success = false;
    char dirtemplate[PATH_MAX];
    const char *dirname = nullptr;
    const char *filename = "dlfile";
    char fullpath[PATH_MAX];

    if (!validate_url(url_data, err)) {
        return false;
    }

    if (buffer == nullptr) {
        *err = VTSS_RFILE_TRNS_ERROR;
        return false;
    }

    *buffer = nullptr;
    *buflen = 0;

    dirname = create_temp_directory(dirtemplate);
    if (dirname == nullptr) {
        T_W("Creating temporary directory failed, errno = %d", errno);
        return false;
    }

    T_D("Created temporary directory '%s'", dirname);
    sprintf(fullpath, "%s/%s", dirname, filename);

    *err = VTSS_RFILE_TRNS_OK;

    success = curl_api_get_file(url_data, fullpath, options, err);
    if (success) {
        // copy content of file into buffer
        success = copy_file_content_to_buffer(fullpath, buffer, buflen);
    }

    // delete temp file and directory
    unlink(fullpath);
    rmdir(dirname);

    return success;
}

bool remote_file_get_chunked(const misc_url_parts_t *url_data,
                             remote_file_write_callback callback,
                             void *callback_context,
                             remote_file_options_t &options,
                             int *err)
{
    if (callback == nullptr) {
        *err = VTSS_RFILE_TRNS_ARGS;
        return false;
    }

    if (!validate_url(url_data, err)) {
        return false;
    }

    return curl_transfer_file(TRANSFER_GET, url_data, nullptr, 0, callback, callback_context, options, err);
}

bool remote_file_put(const misc_url_parts_t *url_data,
                     const char *buffer,
                     size_t buflen,
                     remote_file_options_t &options,
                     int *err)
{
    bool success = false;
    char dirtemplate[PATH_MAX];
    const char *dirname = nullptr;
    const char *filename = "dlfile";
    char fullpath[PATH_MAX];
    int fd;

    if (!validate_url(url_data, err)) {
        return false;
    }

    // initialize to report generic error
    *err = VTSS_RFILE_TRNS_ERROR;

    if (buffer == nullptr || buflen == 0) {
        return false;
    }

    T_D("Transfering buffer of %u bytes", buflen);

    // external tool needs a temporary file for the transfer
    dirname = create_temp_directory(dirtemplate);
    if (dirname == nullptr) {
        T_W("Creating temporary directory failed, errno = %d", errno);
        return false;
    }

    T_D("Created temporary directory '%s'", dirname);
    sprintf(fullpath, "%s/%s", dirname, filename);

    if ((fd = open(fullpath, O_CREAT | O_TRUNC | O_RDWR)) < 0) {
        return false;
    }

    if (write(fd, buffer, buflen) == buflen) {
        T_D("Wrote %zd bytes tp %s", buflen, fullpath);
        success = true;
    } else {
        T_W("Write to temp. file failed (%s): %s", fullpath, strerror(errno));
        success = false;
    }

    close(fd);

    if (success) {
        if (chmod(fullpath, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1) {
            T_W("chmod temp. file failed (%s): %s", fullpath, strerror(errno));
            success = false;
        }
    }

    if (success) {
        success = curl_api_put_file( url_data, fullpath, buflen, options, err);
    }

    if (dirname != nullptr) {
        // delete temp file and directory
        unlink(fullpath);
        rmdir(dirname);
    }

    return success;
}

bool remote_file_known_host_keys_delete()
{
    int rc;
    std::stringstream cmdbuf;
    std::string outbuf;
    std::string errbuf;

    cmdbuf << "rm " << VTSS_RFILE_TRNS_SSH_HOSTS_PATH;
    T_D("Executing command '%s'", cmdbuf.str().c_str());
    rc = vtss::notifications::process_cmd(cmdbuf.str().c_str(), &outbuf, &errbuf);
    if (rc) {
        T_D("Delete command failed (%d)", rc);
        return false;
    }

    return true;
}

} // namespace vtss
