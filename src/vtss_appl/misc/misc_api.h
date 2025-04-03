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

#ifndef _VTSS_APPL_MISC_API_H_
#define _VTSS_APPL_MISC_API_H_

#include <time.h>
#include <sys/types.h>
#include "main_types.h"    /* For mesa_rc                        */
#include "vtss/appl/defines.h" /* For VTSS_APPL_RFC3339_TIME_STR_LEN */
#include "main.h"              /* For vtss_isid_t                    */
#include <limits.h>            /* For PATH_MAX                       */
#ifdef VTSS_SW_OPTION_IP
#include "ip_api.h"
#endif /* VTSS_SW_OPTION_IP */

#ifdef __cplusplus
extern "C" {
#endif

/* Get chip number */
mesa_rc misc_chip_no_get(mesa_chip_no_t *chip_no);

/* Set chip number */
mesa_rc misc_chip_no_set(mesa_chip_no_t chip_no);

/* Read switch chip register */
mesa_rc misc_debug_reg_read(vtss_isid_t isid, mesa_chip_no_t chip_no, ulong addr, uint32_t *value);

/* Write switch chip register */
mesa_rc misc_debug_reg_write(vtss_isid_t isid, mesa_chip_no_t chip_no, ulong addr, uint32_t value);

/* Read PHY register */
mesa_rc misc_debug_phy_read(vtss_isid_t isid,
                            mesa_port_no_t port_no, uint reg, uint page, ushort *value, BOOL mmd_access, ushort devad);

/* Write PHY register */
mesa_rc misc_debug_phy_write(vtss_isid_t isid,
                             mesa_port_no_t port_no, uint reg, uint page, ushort value, BOOL mmd_access, ushort devad);

/* Suspend/resume */
mesa_rc misc_suspend_resume(vtss_isid_t isid, BOOL resume);

/* Convert Hex text string to byte array
 *
 * Return -1 when error occurred
 */
int misc_hexstr_to_array(uchar *array, size_t array_max_size, const char *hex_str);

/* strip leading path from file */
const char *misc_filename(const char *fn);

mesa_rc mgmt_txt2list_bf(char *buf, BOOL *list, ulong min, ulong max, BOOL def, BOOL bf);

/* MAC address text string */
char *misc_mac_txt(const uchar *mac, char *buf);

/* MAC to string. Deprecated. Use misc_mac_txt(), which is thread-safe */
const char *misc_mac2str(const uchar *mac);

/* Create an instantiated MAC address based on base MAC and instance number */
void misc_instantiate_mac(uchar *mac, const uchar *base, int instance);

/* IPv4 address text string */
char *misc_ipv4_txt(mesa_ipv4_t ip, char *buf);

/* Convert prefix length into IPv4 address's netmask. */
BOOL misc_prefix2ipv4 (u32 prefix_len, mesa_ipv4_t *const ipv4);

/* IPv6 address text string. Buf must be at least 50 chars long */
char *misc_ipv6_txt(const mesa_ipv6_t *ipv6, char *buf);

/* IPv4/v6 address text string. Buf must be at least 50 chars long */
const char *misc_ip_txt(mesa_ip_addr_t *ip, char *buf);

/* IPv4/v6 address/mask text string. Give about 128 bytes. */
char *misc_ipaddr_txt(char *buf, size_t bufsize, mesa_ip_addr_t *addr, mesa_prefix_size_t prefix_size);

/* IP address text string - network order */
const char *misc_ntoa(ulong ip);

/* IP address text string - host order */
const char *misc_htoa(ulong ip);

/* Software version text string */
const char *misc_software_version_txt(void);

/* Product name text string */
const char *misc_product_name(void);

/* Software codebase revision string */
const char *misc_software_code_revision_txt(void);

/* Software date text string */
const char *misc_software_date_txt(void);

/* Chip ID and revision as string */
const char *misc_chip_id_txt(void);

/* Board name */
const char *misc_board_name(void);

/* Time to string */
const char *misc_time2str(time_t time);

/* time_t to 999d 99:99:99 format */
const char *misc_time_txt(time_t time_val);

/* Convert UTC time (Pacific Standard Time) to local time */
time_t misc_utctime2localtime(time_t time);

/* Like misc_time2str() but doesn't use a thread unsafe internal buffer
 * to build the string up.
 * #input_str must be at least VTSS_APPL_RFC3339_TIME_STR_LEN (including
 * terminating '\0' character) bytes long.
 * Returns input_str on success, NULL on error.
 */
const char *misc_time2str_r(time_t time, char *input_str);

/*
 * Like misc_time2str_r(), but this function allows you to print only the date
 * or only the time. If invoked with FALSE in both print_date and print_time, you
 * will get a dash.
 * Longest length is VTSS_APPL_RFC3339_TIME_STR_LEN.
 */
const char *misc_time2date_time_str_r(time_t time, char *buf, BOOL print_date, BOOL print_time);

/* Seconds to interval (string) */
const char *misc_time2interval(time_t time);

/* engine ID to string */
const char *misc_engineid2str(char *engineid_txt, const uchar *engineid, ulong engineid_len);

/* OID to string.
  'oid_mask' is a list with 8 bits of hex octets.
  'oid_mask_len' is the total mask len in octets.
  For example:
  oid = {.1.3.6.1.2.1.2.2.1.1.1},
  oid_len = 11
  oid_mask_len = 2;
  oid_mask = {0xFF, 0xA0}
  ---> The output is .1.3.6.1.2.1.2.2.1.*.1.

  Note1: The value of 'oid_len' should be not great than 128.
  Note2: The default output will exclude the character '*' when oid_mask = NULL
 */
const char *misc_oid2str(const ulong *oid, ulong oid_len, const uchar *oid_mask, ulong oid_mask_len);

/**
  * \brief Check the specfic OID string is valid and parse the prefix node.
  *
  * \param oid_str [IN]: Pointer to the OID string.
  * \param prefix [OUT]: The result of parsing the prefix node, if prefix node[0] = 0 indicates that
  * the OID string is valid but it is numeric OID, otherwise the OID string has a prefix node.
  *
  * \return: TRUE if the operation success, FALSE indicates the oid_stri is invalid.
  **/
BOOL misc_parse_oid_prefix(const char *oid_str, char *prefix);

/* OID to string for general case. */
const char *misc_oid2txt(const ulong *oid, ulong oid_len);

const char *misc_raw2txt(const u_char *raw, ulong oid_len, BOOL hex_format);
/* OUI address text string */
char *misc_oui_addr_txt(const uchar *oui, char *buf);

/*  Checks if a string only contains numbers */
mesa_rc misc_str_chk_numbers_only(const char *str);

/* Zero terminated strncpy */
void
misc_strncpyz(char *dst, const char *src, size_t maxlen);

mesa_rc misc_str_is_ipv4(const char *str);
mesa_rc misc_str_is_ipv6(const char *str);
mesa_rc misc_str_is_hostname(const char *hostname);
mesa_rc misc_str_is_domainname(const char *domainname);

/* Check the string is alphanumeric. (excluding empty string)
 * A valid alphanumeric is a text string drawn from alphabet (A-Za-z),
 *    digits (0-9).
 */
mesa_rc misc_str_is_alphanumeric(const char *str);

/* Check the string is alphanumeric or have special characters
 * exclude empty characters
 */
mesa_rc misc_str_is_alphanumeric_or_spec_char(const char *str);

/* Check the string is a hex string.
 * A valid hex string is a text string drawn from alphabet (A-Fa-f), digits (0-9).
 * The length must be even since one byte hex value is presented as two octets string.
 */
mesa_rc misc_str_is_hex(const char *str);

vtss_inst_t misc_phy_inst_get(void);
mesa_rc misc_phy_inst_set(vtss_inst_t instance);

/* Chiptype functions */
uint32_t misc_chip2family(u32 chip_id);
const u16 misc_chiptype(void);

/* Software Identification functions */
typedef enum {
    VTSS_SOFTWARE_TYPE_UNKNOWN = 0, /* ??? */
    VTSS_SOFTWARE_TYPE_DEFAULT = 1, /* Default */
    VTSS_SOFTWARE_TYPE_WEBSTAX = 2, /* WebStaX/WebSparX */
    VTSS_SOFTWARE_TYPE_SMBSTAX = 3, /* SMBStaX */
    VTSS_SOFTWARE_TYPE_ISTAX   = 5, /* IStaX */
    /* ADD NEW TYPES IMMEDIATELY ABOVE THIS LINE - DONT REMOVE LINES EVER! */
} vtss_software_type_t;

const vtss_software_type_t misc_softwaretype(void);

/* Utility function for runtime check of CPU type (internal/external) */
bool misc_cpu_is_external(void);

// Utility function that runtime returns true if we are running on a BeagleBone
// Black board, false otherwise.
bool misc_is_bbb(void);

/* Initialize module */
mesa_rc misc_init(vtss_init_data_t *data);

/* ================================================================= *
 *  Conversion between internal and user port numbers.
 *  These functions are intended to be used only in places that
 *  interact with the user, i.e. Web, CLI, and SNMP.
 * ================================================================= */
typedef mesa_port_no_t vtss_uport_no_t; /* User port is of same type as normal, internal port. */

/****************************************************************************/
// Convert from internal to user port number.
/****************************************************************************/
static inline vtss_uport_no_t iport2uport(mesa_port_no_t iport) {
  return iport + 1 - VTSS_PORT_NO_START;
}

/****************************************************************************/
// Convert from user to internal port number.
/****************************************************************************/
static inline mesa_port_no_t uport2iport(vtss_uport_no_t uport) {
  return (uport == 0) ? VTSS_PORT_NO_NONE : (uport - 1 + VTSS_PORT_NO_START);
}

/* ================================================================= *
 *  Conversion between internal and user priority.
 *  These functions are intended to be used only in places that
 *  interact with the user, i.e. Web, CLI, and SNMP.
 * ================================================================= */

/****************************************************************************/
// Convert from internal to user priority.
/****************************************************************************/
static inline mesa_prio_t iprio2uprio(mesa_prio_t iprio) {
  return iprio - VTSS_PRIO_START;
}

/****************************************************************************/
// Convert from user to internal priority.
/****************************************************************************/
static inline mesa_prio_t uprio2iprio(mesa_prio_t uprio) {
  return uprio + VTSS_PRIO_START;
}

/* ================================================================= *
 *  Conversion between internal and user policy.
 *  These functions are intended to be used only in places that
 *  interact with the user, i.e. Web, CLI, and SNMP.
 * ================================================================= */

/****************************************************************************/
// Convert from internal to user policer.
/****************************************************************************/
static inline mesa_acl_policer_no_t ipolicer2upolicer(mesa_acl_policer_no_t ipolicer) {
  return (ipolicer + 1);
}

/****************************************************************************/
// Convert from user to internal policer.
/****************************************************************************/
static inline mesa_acl_policer_no_t upolicer2ipolicer(mesa_acl_policer_no_t upolicer) {
  return (upolicer - 1);
}

/* ================================================================= *
 *  Conversion between internal and user policer.
 *  These functions are intended to be used only in places that
 *  interact with the user, i.e. Web, CLI, and SNMP.
 * ================================================================= */

/**
 * \brief For containing I2C device adapter information.
 */
typedef struct {
    int i2c_bus;  // The adapter number (bus) of the i2c device.
    int i2c_addr; // The address of the i2c device.
} vtss_i2c_adp_t;

/**
 * \brief Find an I2C device
 *
 * \param driver      [IN]    (Driver) Name of device
 * \param i2c_adp     [OUT]   The adapter number (bus) of the i2c device. May be NULL.
 * \param i2c_addr    [OUT]   The address of the i2c device. May be NULL.
 *
 * \return "TRUE" if device was found, in which case the i2c_adp
 * and/or i2c_addr params will be updated if non-zero.
 */
BOOL vtss_i2c_dev_find(const char *driver, int *i2c_adp, int *i2c_addr);

/**
 * \brief Find an I2C device, open device
 *
 * \param driver      [IN]    (Driver) Name of device
 * \param i2c_adp     [OUT]   The adapter number (bus) of the i2c device. May be NULL.
 * \param i2c_addr    [OUT]   The address of the i2c device. May be NULL.
 *
 * \return As vtss_i2c_dev_find(), but returns file descriptor
 * (non-negative) if device found.
 */
int vtss_i2c_dev_open(const char *driver, int *i2c_bus, int *i2c_addr);

/**
 * \brief Open an I2C device
 *
 * \param adapter_nr [IN]    Adapter number of the device
 * \param i2c_addr   [IN]    The address of the i2c device.
 *
 * \return Returns file descriptor (non-negative) for the I2C device.
 */
int vtss_i2c_adapter_open(int adapter_nr, int i2c_addr);

/**
 * \brief Close device returned by vtss_i2c_dev_open() / vtss_i2c_adapter_open()
 *
 * \param i2c_dev    [IN]    File descriptor of device
 */
static inline void vtss_i2c_dev_close(int i2c_dev)
{
    if (i2c_dev >= 0) {
        close(i2c_dev);
    }
}

/**
 * \brief Read from i2c controller
 *
 * \param file        [IN]    i2c device fd
 * \param data        [OUT]   Pointer to where to put the read data.
 * \param size        [IN]    The number of bytes to read.
 *
 * \return Return code.
 */
mesa_rc vtss_i2c_dev_rd(int file,
                        u8 *data,
                        const u8 size);

/**
 * \brief Write to i2c controller
 *
 * \param file        [IN]    i2c device fd
 * \param data        [IN]    Data to be written.
 * \param size        [IN]    The number of bytes to write.
 *
 * \return Return code.
 */
mesa_rc vtss_i2c_dev_wr(int file,
                        const u8 *data,
                        const u8 size);

/**
 * \brief Do a write and read in one step
 *
 * \param file        [IN]    i2c device fd
 * \param wr_data        [IN]  Data to be written.
 * \param wr_size        [IN]  The number of bytes to write.
 * \param rd_data        [IN]  Pointer to where to put the read data.
 * \param rd_size        [IN]  The number of bytes to read.
 * \param max_wait_time  [IN]  The maximum time to wait for the i2c controller to perform the wrtie (in ms).
 *
 * \return Return code.
 */
mesa_rc vtss_i2c_dev_wr_rd(int                            file,
                           const u8                       i2c_addr,
                           u8                             *wr_data,
                           const u8                       wr_size,
                           u8                             *rd_data,
                           const u8                       rd_size);

#define MISC_URL_PROTOCOL_TFTP          (1 << 0)
#define MISC_URL_PROTOCOL_FTP           (1 << 1)
#define MISC_URL_PROTOCOL_HTTP          (1 << 2)
#define MISC_URL_PROTOCOL_HTTPS         (1 << 3)
#define MISC_URL_PROTOCOL_FILE          (1 << 4)
#define MISC_URL_PROTOCOL_FLASH         (1 << 5)
#define MISC_URL_PROTOCOL_SFTP          (1 << 6)
#define MISC_URL_PROTOCOL_SCP           (1 << 7)
#define MISC_URL_PROTOCOL_USB           (1 << 8)

#define MISC_URL_PROTOCOL_NAME_TFTP     "tftp"
#define MISC_URL_PROTOCOL_NAME_FTP      "ftp"
#define MISC_URL_PROTOCOL_NAME_HTTP     "http"
#define MISC_URL_PROTOCOL_NAME_HTTPS    "https"
#define MISC_URL_PROTOCOL_NAME_FILE     "file"
#define MISC_URL_PROTOCOL_NAME_FLASH    "flash"
#define MISC_URL_PROTOCOL_NAME_SFTP     "sftp"
#define MISC_URL_PROTOCOL_NAME_SCP      "scp"
#define MISC_URL_PROTOCOL_NAME_USB      "usb"

/** \brief Structure for the elements that make up a URL of the form:
 *
 *      <protocol>://[<username>[:<password>]@]<host>[:<port>][/<path>]
 *
 * or
 *
 *      file://[/<path>]/<file>
 *
 * or
 *
 *      flash:path
 *
 * e.g.
 *
 *      tftp://1.2.3.4:5678/path/to/myfile.dat
 *
 * or
 *
 *      flash:startup-config
 *      flash:/startup-config
 */
typedef struct {
    u32     proto_support;
    char    protocol[32];
    u32     protocol_id;
    char    user[64];
    char    pwd[64];
    char    host[64];
    u16     port;               /* Port number, or zero if none given */
    char    path[256];
    char    file[64];
    BOOL    unchecked_filename; /* Set TRUE to bypass the file name checking */

    // Return true if the URL protocol is FLASH
    const bool is_flash() const {
        return protocol_id == MISC_URL_PROTOCOL_FLASH;
    }
    const bool is_usb() const {
        return protocol_id == MISC_URL_PROTOCOL_USB;
    }

} misc_url_parts_t;

/** \brief Initialize #misc_url_parts_t to defaults (empty strings, port zero)
 *
 * \param parts [IN/OUT] The initialized struct
 */
void misc_url_parts_init(misc_url_parts_t *parts, u32 proto_support);

/** \brief Decompose URL string by scanning #url.
 *
 * \param url   [IN]  Pointer to string
 * \param parts [OUT] The decomposed URL
 *
 * \return TRUE = decomposition is valid; FALSE = error in URL
 */
BOOL misc_url_decompose(const char *url, misc_url_parts_t *parts);

/** \brief Compose URL into pre-allocated buffer of given max length.
 *
 * \param url      [IN/OUT] Destination buffer
 * \param max_len  [IN]     Max length of buffer, including terminating \0
 * \param parts    [IN]     URL parts
 *
 * \return TRUE = composition is valid; FALSE = result too long or #parts invalid
 */
BOOL misc_url_compose(char *url, int max_len, const misc_url_parts_t *parts);


/** \brief Convert string to lowercase characters only
 *
 * \param url      [IN/OUT] string to convert
 */
char *str_tolower(char *str);

/* Print thread status */
void misc_thread_status_print(int (*print_function)(const char *fmt, ...), BOOL backtrace, BOOL running_thread_only);
void misc_thread_details_print(int (*print_function)(const char *fmt, ...), int tid);
void misc_thread_context_switches_print(int (*print_function)(const char *fmt, ...));

mesa_rc misc_thread_context_switches_get(int tid, u64 *context_switches);

#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
// Stepping-stone functions to allow misc.icli to be compiled with a
// C and not a C++ compiler.
mesa_rc misc_thread_load_monitor_start(void);
mesa_rc misc_thread_load_monitor_stop(void);
#endif /* defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR) */

// This structure defines the entries that you get from cat'ing
// /proc/<pid>/task/<tid>/stat.
// Since these are printed by the kernel, we use the official types rather than
// our own u32, u64 ones.
// Source of info: http://man7.org/linux/man-pages/man5/proc.5.html
typedef struct {
    int                pid;                   // Process ID
    char               comm[PATH_MAX];        // Filename of executable
    char               state;                 // Process state (R, S, D, Z, T, t, W, X, x, K, W, P)
    int                ppid;                  // PID of the parent process
    int                pgrp;                  // Process group ID
    int                session;               // Session ID
    int                tty_nr;                // Controlling terminal of the process
    int                tpgid;                 // ID of the foreground process of the controlling terminal
    unsigned           flags;                 // Kernel flags word (see PF_* defines in the Linux kernel)
    unsigned long      minflt;                // Number of minor faults not requiring loading of a memory page from disk
    unsigned long      cminflt;               // Number of minor faults that this process' waited-for children have made
    unsigned long      majflt;                // Number of major faults requiring loading of a memory page from disk
    unsigned long      cmajflt;               // Number of major faults that this process' waited-for children have made
    unsigned long      utime;                 // Amount of time process has been scheduled in user mode in clock ticks (sysconf(_SC_CLK_TCK)
    unsigned long      stime;                 // Amount of time process has been scheduled in kernel mode in clock ticks
    long               cutime;                // Amount of time that this process' waited-for children have been scheduled in user mode
    long               cstime;                // Amount of time that this process' waited-for children have been scheduled in kernel mode
    long               priority;              // Negated scheduling priority minus one for processes running a real-time scheduling policy, otherwise raw nice value
    long               nice;                  // Nice value
    long               num_threads;           // Number of threads in this process
    long               itrealvalue;           // Time in jiffies before a SIGALRM is sent to the process due to an interval timer (obsolete since 2.6.17)
    unsigned long long start_time;            // The time the process started after boot in clock ticks
    unsigned long      vsize;                 // Virtual memory size in bytes
    long               rss;                   // Number of pages the process has in real memory
    unsigned long      rsslim;                // Current soft limit in bytes on #rss
    unsigned long      startcode;             // Address above which program text can run
    unsigned long      endcode;               // Address below which program text can run
    unsigned long      startstack;            // Address of the start (bottom) of the stack
    unsigned long      kstkesp;               // Current value of ESP (stack pointer) as found in the kernel stack page
    unsigned long      kstkeip;               // Current value of EIP (program counter) as found in the kernel stack page
    unsigned long      signal;                // Bitmap of pending signals (obsolete; use /proc/[pid]/status instead)
    unsigned long      blocked;               // Bitmap of blocked signals (obsolete; use /proc/[pid]/status instead)
    unsigned long      sigignore;             // Bitmap of ignores signals (obsolete; use /proc/[pid]/status instead)
    unsigned long      sigcatch;              // Bitmap of caught signals  (obsolete; use /proc/[pid]/status instead)
    unsigned long      wchan;                 // Address of a location in the kernel where the process is sleeping (see symbolic name in /proc/[pid]/wchan
    unsigned long      nswap;                 // Number of pages swapped (not maintained)
    unsigned long      cnswap;                // Cumulative nswap for child processes (not maintained)
    int                exit_signal;           // Signal to be sent to parent when process dies
    int                processor;             // CPU number last executed on
    unsigned           rt_priority;           // Real-time scheduling priority ([1; 99]) for processes scheduled under a real-time policy, 0 otherwise
    unsigned           policy;                // Scheduling policy (see SCHED_* defined in the Linux kernel)
    unsigned long long delayacct_blkio_ticks; // Aggregated block I/O delays, measured in clock ticks
    unsigned long      guest_time;            // Time spent running a virtual CPU for a guest operating system, measured in clock ticks
    long               cguest_time;           // Guest time of the process' children, measured in clock ticks
    unsigned long      start_data;            // Address above which program initialized and uninitialized (BSS) data are placed
    unsigned long      end_data;              // Address below which program initialized and uninitialized (BSS) data are placed
    unsigned long      start_brk;             // Address above which program heap can be expanded with brk(2)
    unsigned long      arg_start;             // Address above which program command-line arguments (argv) are placed
    unsigned long      arg_end;               // Address below which program command-line arguments (argv) are placed
    unsigned long      env_start;             // Address above which program environment is placed
    unsigned long      env_end;               // Address below which program environment is placed
    int                exit_code;             // The thread's exit status in the form reported by waitpid(2)
} vtss_proc_thread_stat_t;

// This structure defines the entries that you get from cat'ing
// /proc/<tid>/status or /proc/<pid>/task/<tid>/status.
// Since the values of the fields are not of a particular type (some consists of
// many ints, and some are octal, while others are hex or unsigned long longs),
// we read the values into a char-array, and let it be up to the user of the
// TLVs to parse the values.
// The kernel prints this file in .../fs/proc/array.c#proc_pid_status().
typedef struct {
    struct {
        char key[40];
        char value[40];
    } tlv[150];
} vtss_proc_thread_status_t;

// This structure defines the entries that you get from cat'ing /proc/stat.
// Since these are printed by the kernel, we use the official types rather than
// our own u32, u64 ones.
// Source of info: http://man7.org/linux/man-pages/man5/proc.5.html and kernel
// source code (.../fs/proc/stat.c)
typedef struct {
    unsigned long long user;       // Time spent in user mode
    unsigned long long nice;       // Time spent in user mode with low priority (nice)
    unsigned long long system;     // Time spent in system mode
    unsigned long long idle;       // Time spent in the idle task. This value should be USER_HZ times the second entry in the /proc/uptime pseudo-file
    unsigned long long iowait;     // Time waiting for I/O to complete (since Linux 2.5.41)
    unsigned long long irq;        // Time servicing interrupts (since Linux 2.6.0-test4)
    unsigned long long softirq;    // Time servicing softirqs (since Linux 2.6.0-test4)
    unsigned long long steal;      // Stolen time, which is the time spent in other operating systems when running in a virtualized environment (since Linux 2.6.11)
    unsigned long long guest;      // Time spent running a virtual CPU for guest operating systems under the control of the Linux kernel (since Linux 2.6.24)
    unsigned long long guest_nice; // Time spend running a niced guest (virtual CPU for guest operating systems under the control of the Linux kernel (since Linux 2.6.33)
} vtss_proc_stat_t;

// This structure defines the entries that you get from cat'ing /proc/vmstat.
// Since these are printed by the kernel, we use the official types rather than
// our own u32, u64 ones.
// Source of info: http://man7.org/linux/man-pages/man5/proc.5.html and kernel
// source code (.../mm/vmstat.c)
// Since the lines are VERY different from kernel version to kernel version,
// it is not spelled out, but rather an array of entries with name and value.
typedef struct {
    struct {
        char name[40];
        unsigned long value;
    } tlv[150];

    u32 max_name_wid;
} vtss_proc_vmstat_t;

mesa_rc misc_proc_thread_stat_parse(int tid, vtss_proc_thread_stat_t *stat);
mesa_rc misc_proc_thread_status_parse(int tid, vtss_proc_thread_status_t *status);
mesa_rc misc_proc_stat_parse(vtss_proc_stat_t *stat);
mesa_rc misc_proc_vmstat_parse(vtss_proc_vmstat_t *vmstat);
void    misc_code_version_print(int (*print_function)(const char *fmt, ...));

// Debug method of changing port state
void    misc_debug_port_state(mesa_port_no_t port_no, BOOL state);

/****************************************************************************/
// Error Return Codes (mesa_rc)
/****************************************************************************/
enum {
    MISC_ERROR_INV_PARAM = MODULE_ERROR_START(VTSS_MODULE_ID_MISC), // NULL parameter passed to one of the mirror_XXX functions, where a non-NULL was expected.
    VTSS_RC_MISC_I2C_REBOOT_IN_PROGRESS                         // Signaling that the result of the I2C access is invalid due to the system is rebooting.
};

mesa_rc misc_find_spidev(char *spi_file, size_t max_size, const char *id);

// Make a memory dump of in_buf buffer of size in_sz into out_buf of size out_sz
// The output looks like:
// 0x7f52ffb0c0/0000: 03 04 21 00 00 00 00 00 04 00 00 00 ff 00 00 00 ..!.............
//
// The address (in the example "0x7f52ffb0c0/") can be avoided if print_address
// is set to false
//
// The printable chars (in the example " ..!.............") can be avoided if
// print_printable is set to false.
//
// Returns out_buf.
char *misc_mem_print(const uint8_t *in_buf, size_t in_sz, char *out_buf, size_t out_sz, bool print_address = false, bool print_printable = true);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_APPL_MISC_API_H_ */

