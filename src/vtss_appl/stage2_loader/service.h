/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __SERVICE_HXX__
#define __SERVICE_HXX__

#include <stdio.h>
#include "utlist.h"
#include "main.h"

// ****************************************************************************
enum {
    // Name of service. Must appear once.
    SERVICE_NAME,

    // Command to invoke. must appear once.
    SERVICE_CMD,

    // Command type. Allow values:
    //  "service"   A long-lived process that must be restarted if it dies
    //  "oneshot"   A command that is expected to return
    // Optional, if not provided then defaults to "service"
    SERVICE_TYPE,

    // Control what happens if or when a process terminates.
    // The semantics differ between long-lived and one-shot processes.
    // If service type is "service", termination of a process, whether
    // intentionally with any return value or through a signal, is considered an
    // error, and either of the following settings takes effect.
    // If service type is "oneshot", termination of a process is considered an
    // error if the process terminated through a signal or with a non-zero exit
    // value. In these cases, the following will take effect. In other words,
    // a one-shot process will just terminate and signal readiness *only* if
    // it returns normally with an exit value of 0.
    // Valid values are:
    //  "respawn"   Restart the process
    //  "reboot"    Reboot the system
    //  "ignore"    Do nothing
    // Optional, if not provided, it defaults to "respawn"
    SERVICE_ON_ERROR,

    // Specify environment variables to set for the given service. Environments
    // must use the syntax: key=val
    // To specify multiple environment, repeat this statement.
    SERVICE_ENV,

    // A list of services, this service depends on.
    SERVICE_DEPEND,

    SERVICE_READY_FILE,

    // Service profle
    SERVICE_PROFILE,
    // ****************
    SERVICE_CNT
};

enum ServiceType {
    SERVICE,
    ONESHOT,

    // Add new above
    _SERVICE_TYPE_INVALID = -1
};

enum OnError {
  RESPAWN,
  REBOOT,
  IGNORE,

  // Add new above
  _ON_ERROR_INVALID = -1
};

enum AttrAppearence {
    ATTR_REQUIRED,  // Must apear once
    ATTR_OPTIONAL,  // Zero or one
    ATTR_DEFAULT,   // Allow once, if not defined use default
    ATTR_SEQUENCE   // zero or more
};

struct Attr;

struct AttrConf;

typedef struct Service2 {
    struct Service2 *next, *prev;

    struct Attr *attr[SERVICE_CNT];

    int running;

    int ready;

    pid_t pid;
} Service_t;

extern int g_is_ubifs;

// TODO, implement this first
// Read the entire file into a buffer
char *attr_read_file_into_buf(const char *filename, ssize_t *size_of_buf);

// TODO, implement this first
// Return a line from the file buffer. It will search for the first occurence of
// '\n' or '\r' and replace that char with a '\0', and return how many chars was
// consumed.
// When it reach the end of buffer, then it will return 0.
// Should be used like this:
// void itr_lines(char *buf, size_t len) {
//     while (len) {
//         size_t l = attr_get_line(buf, len);
//
//         // Use buf as a "line"
//
//         buf += l;
//         len -= l;
//     }
// }
size_t attr_get_line(char *buf, size_t len);

// Takes a line, and split it into a key/value pair. The line is expected to be
// formated as "KEY = VALUE".
//
// The function is suppose to implement the equivalent of the following regex:
//   /\s*(\w+)\s*=\s*(.*?)\s*$/
//
// On success update the '**key' and '**val' pointers and return 1;
// On error, let 'key' and 'val' point to NULL, and return 0;
int attr_split_into_key_val(char *line, char **key, char **val);

// Construct a new attribute using the configuration set. The method will scan
// through the list of AttrConf_t and find the matching key. If found, the the
// 'create' function pointer to construct the attribute.
// On error, return 'NULL'
// On success, return a pointer to the newly allocated Attr.
struct Attr *attr_construct(const struct AttrConf *conf, int conf_size, const char *key,
                            const char *val);

typedef struct Attr *(*AttrCreate)(const char *data);

typedef void (*AttrFree)(struct Attr *data);

typedef struct AttrConf {
    const char *key_name;
    enum AttrAppearence appearence;
    AttrCreate create;
    AttrFree free;
    const char *default_value;
} AttrConf_t;

typedef struct Attr {
    struct Attr *next, *prev;
    int attr_type;
}Attr_t;

// ****************************************************************************
typedef struct AttrDepend {
    struct Attr attr;

    const char *name;
    Service_t *service;
}AttrDepend_t;

struct Attr *attr_depend_create(const char *data);

void attr_depend_free(struct Attr *data);

// ****************************************************************************
typedef struct AttrName {
    struct Attr attr;

    const char *name;
} AttrName_t;

struct Attr *attr_name_create(const char *data);

void attr_name_free(struct Attr *data);

// ****************************************************************************
typedef struct AttrCmd {
    struct Attr attr;

    const char *cmd;
} AttrCmd_t;

struct Attr *attr_cmd_create(const char *data);

void attr_cmd_free(struct Attr *data);

// ****************************************************************************
typedef struct AttrReadyFile {
    struct Attr attr;

    const char *file;
} AttrReadyFile_t;

struct Attr *attr_ready_file_create(const char *data);

void attr_ready_file_free(struct Attr *data);

// ****************************************************************************
typedef struct AttrType {
    struct Attr attr;

    enum ServiceType type;
} AttrType_t;

struct Attr *attr_type_create(const char *data);

// ****************************************************************************
typedef struct {
    struct Attr attr;

    enum OnError on_error;
} AttrOnError_t;

Attr_t *attr_on_error_create(const char *data);

// ****************************************************************************
typedef struct AttrEnv {
    struct Attr attr;

    const char *key;  // FOO
    const char *val;  // bar
    char *res;  // FOO=bar
} AttrEnv_t;

struct Attr *attr_env_create(const char *data);

void attr_env_free(struct Attr *data);

// ****************************************************************************
typedef struct AttrProfile
{
    struct Attr attr;

    const char *profile;
} AttrProfile_t;

struct Attr *attr_profile_create(const char *data);

void attr_profile_free(struct Attr *data);

// ****************************************************************************
static const AttrConf_t attr_conf[SERVICE_CNT] = {
    [SERVICE_NAME] = {
        .key_name   = "name",
        .appearence = ATTR_REQUIRED,
        .create     = attr_name_create,
        .free       = attr_name_free
    },

    [SERVICE_CMD] = {
        .key_name   = "cmd",
        .appearence = ATTR_REQUIRED,
        .create     = attr_cmd_create,
        .free       = attr_cmd_free
    },

    [SERVICE_TYPE] = {
        .key_name   = "type",
        .appearence = ATTR_DEFAULT,
        .create     = attr_type_create,
        .free       = NULL,
        .default_value = "service"
    },

    [SERVICE_ON_ERROR] = {
        .key_name   = "on_error",
        .appearence = ATTR_DEFAULT,
        .create     = attr_on_error_create,
        .free       = NULL,
        .default_value = "respawn"
    },

    [SERVICE_ENV] = {
        .key_name   = "env",
        .appearence = ATTR_SEQUENCE,
        .create     = attr_env_create,
        .free       = attr_env_free
    },

    [SERVICE_DEPEND] = {
        .key_name   = "depend",
        .appearence = ATTR_SEQUENCE,
        .create     = attr_depend_create,
        .free       = attr_depend_free
    },

    [SERVICE_READY_FILE] = {
        .key_name   = "ready_file",
        .appearence = ATTR_SEQUENCE,
        .create     = attr_ready_file_create,
        .free       = attr_ready_file_free
    },

    [SERVICE_PROFILE] = {
        .key_name   = "serviced_profile",
        .appearence = ATTR_DEFAULT,
        .create     = attr_profile_create,
        .free       = attr_profile_free,
        .default_value = "webstax"
    },
};

// ****************************************************************************
int config_parse(
    char *data, size_t data_len, const AttrConf_t *conf, int conf_size,
                  struct Attr **result);

int config_parse_file(FILE *file, const AttrConf_t *conf, int conf_size,
                       struct Attr **result);

// Find service by name. If not found, then return NULL
// TODO: changed arguments
Service_t *service_by_name(Service_t *head, const char *name);

// Find the service by pid. If not found, then return NULL
Service_t *service_by_pid(Service_t *head, pid_t pid);

// Start service 's'. This will call fork and exec
void service_start(Service_t *s, int *forked, int use_non_blocking_console);

// Check if a given service is running (pid exists)
void service_running(Service_t *s);

// TODO, should return if it is ready
// Check if service is ready. This is not always the same as running. If a
// service has specified ready file or other facilities to check for readiness,
// the ready flag is delayed from the running flag.
int service_ready(Service_t *s);

// TODO, should return if it is ready
// Check if all dependencies of a given service is ready.
int service_all_dependency_ready(Service_t *head, Service_t *s);

// Update the status of a service based on the status returned by wait
void service_update_wait_status(Service_t *head, pid_t pid, int status);

// Call wait in a loop waitpid(-1, x, WNOHANG), until it start to fail
void service_wait_all_pending(Service_t *head);

// Call wait and let it block
void service_wait_blocking(Service_t *head, unsigned int milisec);

// Setup selfpipe
void selfpipe_setup();

void service_spawn(int use_non_blocking_console);

int service_validate(const AttrConf_t *conf, Service_t *service);

#endif  // __SERVICE_HXX__
