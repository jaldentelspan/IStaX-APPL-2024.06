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

#include <vtss/basics/map.hxx>
#include "misc_api.h"

#ifndef VTSS_REMOTE_FILE_TRANSFER_HXX
#define VTSS_REMOTE_FILE_TRANSFER_HXX

namespace vtss {

// PSCP command protocol types
#define VTSS_RFILE_TRNS_COMMAND_SCP         "scp"       // use "scp" protocol
#define VTSS_RFILE_TRNS_COMMAND_SFTP        "sftp"      // use "sftp" protocol

// File transfer operation
typedef enum {
    VTSS_RFILE_TRNS_OPER_GET,
    VTSS_RFILE_TRNS_OPER_PUT
} transfer_oper_t;

// File transfer error codes
enum {
    VTSS_RFILE_TRNS_OK = 0,             // All is OK
    VTSS_RFILE_TRNS_ERROR,              // Unspecified error
    VTSS_RFILE_TRNS_ARGS,               // Invalid arguments
    VTSS_RFILE_TRNS_NOPROT,             // Unsupported protocol
    VTSS_RFILE_TRNS_UNSUPP_METHOD,      // Unsupported method
    VTSS_RFILE_TRNS_INVAL_URL,          // Invalid URL
    VTSS_RFILE_TRNS_PROTERR,            // Protocol error
    VTSS_RFILE_TRNS_NO_HOST,            // Could not resolve hostname
    VTSS_RFILE_TRNS_CONN,               // Connection refused
    VTSS_RFILE_TRNS_CONN_TIMEOUT,       // Connection timed put
    VTSS_RFILE_TRNS_CONN_LOST,          // Connection lost
    VTSS_RFILE_TRNS_NOROUTE,            // No route to host
    VTSS_RFILE_TRNS_UNREACH,            // Network is unreachable
    VTSS_RFILE_TRNS_NOUSERPW,           // No username and/or password given
    VTSS_RFILE_TRNS_LOGIN,              // Login incorrect
    VTSS_RFILE_TRNS_NEEDUSERPW,         // Need to specify username and/pr password
    VTSS_RFILE_TRNS_EPERM,              // Bad file permissions
    VTSS_RFILE_TRNS_ENOTFOUND,          // File not found
    VTSS_RFILE_TRNS_INVALID_PATH,       // Invalid path
    VTSS_RFILE_TRNS_INVALID_FILENAME,   // Invalid filename
    VTSS_RFILE_TRNS_TIMEOUT,            // Operation timed out
    VTSS_RSFILE_TRNS_NO_HOSTKEY,        // SSH hostkey is not in cache
    VTSS_RSFILE_TRNS_CH_HOSTKEY,        // SSH hostkey is changed
    VTSS_RSFILE_TRNS_SERVER,            // Server problem (disk, memory)
    VTSS_RSFILE_TRNS_FTP_NOPASV,        // Cannot use FTP passive mode
    VTSS_RSFILE_TRNS_FTP_NOPORT,        // Cannot use FTP active mode
    VTSS_RSFILE_TRNS_SSH_ERR,           // An unspecified error occurred during the SSH session
    VTSS_RSFILE_TRNS_UPLOAD_FAILED,     // File upload failed
};

/**
 *
 */
struct remote_file_options_t
{
    bool ssh_save_host_keys = false;
    bool ftp_active = false;
};

/**
 * Get a file from a remote server. The file content will be put in the provided buffer.
 *
 * @param url_data          The decomposed URL
 * @param buffer            The data buffer to transfer
 * @param buflen            The size of the data buffer in octets
 * @param options           Protocol-specific transfer options
 * @param err [OUT]         Contains an error code if the request failed.
 *
 * @return                  true if the request succeeded and false if it failed.
 */
bool remote_file_get(const misc_url_parts_t *url_data,
                     char **buffer,
                     size_t *buflen,
                     remote_file_options_t &options,
                     int *err);

/*
 * Callback function type for writing received data to local storage
 */
typedef size_t (* remote_file_write_callback)(char *ptr, size_t size, size_t nmemb, void *callback_context);

/**
 * Get a file from a remote server. The file content will be passed to the provided callback in chunks.
 *
 * @param url_data          The decomposed URL
 * @param callback          The callback function which will receive the data
 * @param callback_context  The usage-specific context for the callback function
 * @param options           Protocol-specific transfer options
 * @param err [OUT]         Contains an error code if the request failed.
 *
 * @return                  true if the request succeeded and false if it failed.
 */
bool remote_file_get_chunked(const misc_url_parts_t *url_data,
                             remote_file_write_callback callback,
                             void *callback_context,
                             remote_file_options_t &options,
                             int *err);

/**
 * Put (send) a file to a remote server
 *
 * @param url_data          The decomposed URL
 * @param buffer            The data buffer to transfer
 * @param buflen            The size of the data buffer in octets
 * @param options           Protocol-specific transfer options
 * @param err [OUT]         Contains an error code if the request failed.
 *
 * @return                  true if the request succeeded and false if it failed.
 */
bool remote_file_put(const misc_url_parts_t *url_data,
                     const char *buffer,
                     size_t buflen,
                     remote_file_options_t &options,
                     int *err);

/**
 * Setup the environment for use with SSH-based transfer methods (SCP, SFTP)
 *
 * @return                      true if successful
 */
bool remote_file_ssh_environment_setup(int *err);

/*
 * Return a descriptive string for the VTSS_RFILE_TRNS_XXX error code given
 */
const char * remote_file_errstring_get(int rc);

/*
 * Delete cache of known host keys on system
 */
bool remote_file_known_host_keys_delete();

} // namespace vtss

#endif /* VTSS_REMOTE_FILE_TRANSFER_HXX */

