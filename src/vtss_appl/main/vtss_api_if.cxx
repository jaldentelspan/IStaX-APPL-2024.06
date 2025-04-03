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

#include <endian.h>
#include <asm/byteorder.h>
#include <sys/mman.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>  /* I2C support */
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <string.h>
#include <linux/spi/spidev.h> /* For SPI_IOC_WR_MODE and friends */

#include "main.h"
#include "main_conf.hxx"
#include "board_if.h"
#include "vtss_api_if_api.h"
#include "critd_api.h"
#include "conf_api.h"
#include "misc_api.h"
#include "vtss_os_wrapper.h"
#include "board_subjects.hxx"
#include "port_api.h"
#include "l2proto_api.h"
#ifdef VTSS_SW_OPTION_DHCP_HELPER
#include "dhcp_helper_api.h"
#endif
#if defined(VTSS_SW_OPTION_VLAN)
#include "vlan_api.h"
#endif
#if defined(VTSS_SW_OPTION_AFI)
#include "afi_api.h"
#endif
#if defined(VTSS_SW_OPTION_QOS)
#include "qos_api.h"
#endif
#if defined(VTSS_SW_OPTION_DOT1X)
#include "dot1x_api.h"
#endif
#ifdef VTSS_SW_OPTION_PSEC
#include "psec_api.h"
#endif
#ifdef VTSS_SW_OPTION_AGGR
#include "aggr_api.h"
#endif

#if defined(VTSS_SW_OPTION_LLDP)
#include "lldp_api.h"
#endif
#ifdef VTSS_SW_OPTION_LLDP_MED
#include "lldpmed_shared.h"
#endif

#ifdef VTSS_SW_OPTION_PTP
#include "ptp_constants.h"
#endif

#if defined(VTSS_SW_OPTION_ZLS30387)
#include "zl_3038x_api_pdv_api.h"
#endif

#if defined(VTSS_PERSONALITY_STACKABLE)
#include "topo_api.h"
#endif
#if defined(VTSS_SW_OPTION_CE_MAX)
#include "ce_max_api.h"
#endif /* VTSS_SW_OPTION_CE_MAX */

#if defined(MSCC_BRSDK)
#include "packet_api.h" /* For packet_ufdma_trace_update() */
#endif /* defined(MSCC_BRSDK) */

#if (__BYTE_ORDER == __BIG_ENDIAN)
#define PCIE_HOST_CVT(x) __builtin_bswap32((x))  /* PCIe is LE - we're BE, so swap */
#define CPU_HTONL(x)     (x)                     /* We're network order already */
#else
#define PCIE_HOST_CVT(x) (x)                     /* We're LE already */
#define CPU_HTONL(x)     __builtin_bswap32(x)    /* LE to network order */
#endif

#ifdef __cplusplus
VTSS_ENUM_INC(mesa_trace_group_t);
#endif /* #ifdef __cplusplus */

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_API_AI /* Can't distinguish between AIL and CIL */

/* API semaphore */
static critd_t vtss_api_crit;

// Mepa mutex used for re-organised PHY Api
static critd_t mepa_crit;

/* Mutex for get-modify-set API operations */
critd_t vtss_appl_api_crit;

volatile uint32_t *base_mem;

#define MAX_REG_MAPS        2
static struct reg_map {
    // uio mapping
    int addr, offset, size;
    // Register (word) ranges
    int start, end;
    // mapped memory
    volatile u32 *ptr;
} regmap[MAX_REG_MAPS];

// Variables for using SPI for register access
static char spi_reg_io_dev[512];           // Device name
static int  spi_reg_io_pad;                // Padding
static int  spi_reg_io_freq;               // Frequency
static bool spi_reg_io;                    // True if using SPI for register access.
static int  spi_fd;                        // File descriptor to use for R/W of registers
#define SPI_BYTE_CNT     7                 // Number of bytes to transmit or receive
#define SPI_PADDING_MAX 15                 // Maximum number of optional padding bytes
#define TO_SPI(_a_)     (_a_ & 0x007FFFFF) // 23 bit SPI address

/* Board information and instance data */
static meba_board_interface_t board_info;
meba_inst_t board_instance;

#include "main_trace.h" /* For MAIN_TRACE_GRP_BOARD */

#undef VTSS_TRACE_MODULE_ID
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_API_AI

static vtss_trace_reg_t trace_reg_ail =
{
    VTSS_MODULE_ID_API_AI, "api_ail", "VTSS API - Application Interface Layer"
};

static vtss_trace_reg_t trace_reg_cil =
{
    VTSS_MODULE_ID_API_CI, "api_cil", "VTSS API - Chip Interface Layer"
};

/* Default trace level */
#define VTSS_API_DEFAULT_TRACE_LVL VTSS_TRACE_LVL_ERROR
static vtss_trace_grp_t trace_grps_ail[] =
{
    /* MESA_TRACE_GROUP_DEFAULT */ {
        "default",
        "Default",
        VTSS_API_DEFAULT_TRACE_LVL
    },
    /* MESA_TRACE_GROUP_PORT */ {
        "port",
        "Port",
        VTSS_API_DEFAULT_TRACE_LVL
    },
    /* MESA_TRACE_GROUP_PHY */ {
        "phy",
        "PHY",
        VTSS_API_DEFAULT_TRACE_LVL
    },
    /* MESA_TRACE_GROUP_PACKET */ {
        "packet",
        "Packet",
        VTSS_API_DEFAULT_TRACE_LVL
    },
    /* MESA_TRACE_GROUP_AFI */ {
        "afi",
        "AFI",
        VTSS_API_DEFAULT_TRACE_LVL
    },
    /* MESA_TRACE_GROUP_QOS */ {
        "qos",
        "QoS",
        VTSS_API_DEFAULT_TRACE_LVL
    },
    /* MESA_TRACE_GROUP_L2 */ {
        "l2",
        "Layer 2",
        VTSS_API_DEFAULT_TRACE_LVL
    },
    /* MESA_TRACE_GROUP_L3 */ {
        "l3",
        "Layer 3",
        VTSS_API_DEFAULT_TRACE_LVL
    },
    /* MESA_TRACE_GROUP_SECURITY */ {
        "security",
        "Security",
        VTSS_API_DEFAULT_TRACE_LVL
    },
    /* MESA_TRACE_GROUP_EVC */ {
        "evc",
        "Obsolete",
        VTSS_API_DEFAULT_TRACE_LVL
    },
    /* MESA_TRACE_GROUP_FDMA_NORMAL */ {
        "fdma",
        "Obsolete",
        VTSS_API_DEFAULT_TRACE_LVL
    },
    /* MESA_TRACE_GROUP_FDMA_IRQ */ {
        "fdma_irq",
        "Obsolete",
        VTSS_API_DEFAULT_TRACE_LVL,
        VTSS_TRACE_FLAGS_USEC|VTSS_TRACE_FLAGS_RINGBUF
    },

    /* MESA_TRACE_GROUP_REG_CHECK */ {
        "reg_check",
        "Obsolete",
        VTSS_API_DEFAULT_TRACE_LVL,
        VTSS_TRACE_FLAGS_USEC|VTSS_TRACE_FLAGS_RINGBUF
    },

    /* MESA_TRACE_GROUP_MPLS */ {
        "mpls",
        "Obsolete",
        VTSS_API_DEFAULT_TRACE_LVL
    },

    /* MESA_TRACE_GROUP_HW_PROT */ {
        "hwprot",
        "Obsolete",
        VTSS_API_DEFAULT_TRACE_LVL
    },

    /* MESA_TRACE_GROUP_HQOS */ {
        "hqos",
        "Obsolete",
        VTSS_API_DEFAULT_TRACE_LVL
    },

    /* MESA_TRACE_GROUP_MACSEC */ {
        "macsec",
        "MACsec (IEEE 802.1AE)",
        VTSS_API_DEFAULT_TRACE_LVL
    },

    /* MESA_TRACE_GROUP_VCAP */ {
        "vcap",
        "VCAP",
        VTSS_API_DEFAULT_TRACE_LVL
    },

    /* MESA_TRACE_GROUP_OAM */ {
        "oam",
        "OAM",
        VTSS_API_DEFAULT_TRACE_LVL
    },

    /* MESA_TRACE_GROUP_MRP */ {
        "mrp",
        "Media Redundancy Protocol",
        VTSS_API_DEFAULT_TRACE_LVL
    },

    /* MESA_TRACE_GROUP_TS */ {
        "ts",
        "Timestamp",
        VTSS_API_DEFAULT_TRACE_LVL
    },

    /* MESA_TRACE_GROUP_CLOCK */ {
        "clock",
        "SyncE clock api",
        VTSS_API_DEFAULT_TRACE_LVL
    },

    /* MESA_TRACE_GROUP_EMUL */ {
        "emul",
        "Emulation",
        VTSS_API_DEFAULT_TRACE_LVL
    },
};

VTSS_TRACE_REGISTER(&trace_reg_ail, trace_grps_ail);

// The CIL trace initialization is deferred to vtss_api_if_init().
static vtss_trace_grp_t trace_grps_cil[ARRSZ(trace_grps_ail)];

/* Using function name is more informative than file name in this case */
#define API_CRIT_ENTER(function) critd_enter(&vtss_api_crit, function, 0)
#define API_CRIT_EXIT(function)  critd_exit( &vtss_api_crit, function, 0)

/* Globally exposed variable! */
CapArray<meba_port_entry_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_custom_table;

/* ================================================================= *
 *  API lock/unlock
 * ================================================================= */

void mesa_callout_lock(const mesa_api_lock_t *const lock)
{
    API_CRIT_ENTER(lock->function);
}

void mesa_callout_unlock(const mesa_api_lock_t *const lock)
{
    API_CRIT_EXIT(lock->function);
}

/* ================================================================= *
 *  Trace
 * ================================================================= */

/* Convert API trace level to application trace level */
static int api2appl_level(mesa_trace_level_t level)
{
    int lvl;

    switch (level) {
    case MESA_TRACE_LEVEL_NONE:
        lvl = VTSS_TRACE_LVL_NONE;
        break;
    case MESA_TRACE_LEVEL_ERROR:
        lvl = VTSS_TRACE_LVL_ERROR;
        break;
    case MESA_TRACE_LEVEL_INFO:
        lvl = VTSS_TRACE_LVL_INFO;
        break;
    case MESA_TRACE_LEVEL_DEBUG:
        lvl = VTSS_TRACE_LVL_DEBUG;
        break;
    case MESA_TRACE_LEVEL_NOISE:
        lvl = VTSS_TRACE_LVL_NOISE;
        break;
    default:
        lvl = VTSS_TRACE_LVL_RACKET; /* Should never happen */
        break;
    }
    return lvl;
}

/* Convert application trace level to API trace level */
static mesa_trace_level_t appl2api_level(int lvl)
{
    mesa_trace_level_t level;

    switch (lvl) {
    case VTSS_TRACE_LVL_ERROR:
    case VTSS_TRACE_LVL_WARNING:
        level = MESA_TRACE_LEVEL_ERROR;
        break;
    case VTSS_TRACE_LVL_INFO:
        level = MESA_TRACE_LEVEL_INFO;
        break;
    case VTSS_TRACE_LVL_DEBUG:
        level = MESA_TRACE_LEVEL_DEBUG;
        break;
    case VTSS_TRACE_LVL_NOISE:
    case VTSS_TRACE_LVL_RACKET:
        level = MESA_TRACE_LEVEL_NOISE;
        break;
    case VTSS_TRACE_LVL_NONE:
        level = MESA_TRACE_LEVEL_NONE;
        break;
    default:
        level = MESA_TRACE_LEVEL_ERROR; /* Should never happen */
        break;
    }
    return level;
}

static int api2appl_module_id(const mesa_trace_layer_t layer)
{
    return (layer == MESA_TRACE_LAYER_AIL ? trace_reg_ail.module_id : trace_reg_cil.module_id);
}

/* Trace callout function */
void mesa_callout_trace_printf(const mesa_trace_layer_t layer,
                               const mesa_trace_group_t group,
                               const mesa_trace_level_t level,
                               const char *file,
                               const int line,
                               const char *function,
                               const char *format,
                               va_list args)
{
    int     module_id = api2appl_module_id(layer);
    int     lvl = api2appl_level(level);
    vtss_trace_vprintf(module_id, group, lvl, function, line, format, args);
}

/* Trace hex-dump callout function */
void mesa_callout_trace_hex_dump(const mesa_trace_layer_t layer,
                                 const mesa_trace_group_t group,
                                 const mesa_trace_level_t level,
                                 const char               *file,
                                 const int                line,
                                 const char               *function,
                                 const unsigned char      *byte_p,
                                 const int                byte_cnt)
{
    int module_id = api2appl_module_id(layer);
    int lvl = api2appl_level(level);

    /* Map API trace to WebStaX trace */
    vtss_trace_hex_dump(module_id, group, lvl, function, line, byte_p, byte_cnt);
}

/* Called when module trace levels have been changed */
static void vtss_api_trace_update_do(BOOL call_ufdma)
{
    int               grp;
    mesa_trace_conf_t conf;
    int               global_lvl = vtss_trace_global_lvl_get();

    for (grp = 0; grp < MESA_TRACE_GROUP_COUNT; grp++) {
        /* Map WebStaX trace level to API trace level */
        conf.level[MESA_TRACE_LAYER_AIL] = global_lvl > trace_grps_ail[grp].lvl ? appl2api_level(global_lvl) : appl2api_level(trace_grps_ail[grp].lvl);
        conf.level[MESA_TRACE_LAYER_CIL] = global_lvl > trace_grps_cil[grp].lvl ? appl2api_level(global_lvl) : appl2api_level(trace_grps_cil[grp].lvl);
        mesa_trace_conf_set((mesa_trace_group_t)grp, &conf);
    }

#if defined(MSCC_BRSDK)
    // Only applicable for iCPU
    if (call_ufdma && (!misc_cpu_is_external())) {
        // The uFDMA piggy-backs on the normal trace system, but is
        // completely decoupled from the API trace, because it is
        // a standalone component. Just call it to update itself.
        packet_ufdma_trace_update();
    }
#endif /* defined(MSCC_BRSDK) */
}

void vtss_api_trace_update(void)
{
    vtss_api_trace_update_do(TRUE);
}

/* ================================================================= *
 *  I2C
 * ================================================================= */

#define I2C_PORT2DEV(p) (100 + p)

/**
 * \brief Function for doing i2c reads from the switch i2c controller
 *
 * \param port_no [IN] Port number
 * \param i2c_addr [IN] I2C device address
 * \param addr [IN]   Register address
 * \param data [OUT]  Pointer the register(s) data value.
 * \param cnt [IN]    Number of registers to read
 *
 * \return Return code.
 **/
mesa_rc i2c_read(const mesa_port_no_t port_no, const u8 i2c_addr, const u8 addr, u8 *const data, const u8 cnt)
{
    int file;
    mesa_rc rc = VTSS_RC_ERROR;
    if ((file = vtss_i2c_adapter_open(I2C_PORT2DEV(port_no), i2c_addr)) >= 0) {
        struct i2c_rdwr_ioctl_data packets;
        struct i2c_msg messages[2];

        // Write portion
        messages[0].addr  = i2c_addr;
        messages[0].flags = 0;
        messages[0].len   = 1;
        *data = addr;    // (Re-)Use the read buffer for the address write
        messages[0].buf   = data;

        // Read portion
        messages[1].addr  = i2c_addr;
        messages[1].flags = I2C_M_RD /* | I2C_M_NOSTART*/;
        messages[1].len   = cnt;
        messages[1].buf   = data;

        /* Transfer the i2c packets to the kernel and verify it worked */
        packets.msgs  = messages;
        packets.nmsgs = ARRSZ(messages);
        if(ioctl(file, I2C_RDWR, &packets) < 0) {
            T_I("I2C transfer failed: %s, port_no: %u, i2c_addr: %u, addr: %u, cnt: %u", strerror(errno), port_no, i2c_addr, addr, cnt);
        } else {
            rc = VTSS_RC_OK;
        }
        close(file);
    }

    T_D("i2c read port %d, addr 0x%x, %d bytes - RC %d", port_no, i2c_addr, cnt, rc);
    return rc;
}

/**
 * \brief Function for doing i2c reads from the switch i2c controller
 *
 * \param port_no [IN] Port number
 * \param i2c_addr [IN] I2C device address
 * \param data [OUT]  Pointer the register(s) data value.
 * \param cnt [IN]    Number of registers to read
 *
 * \return Return code.
 **/
mesa_rc i2c_write(const mesa_port_no_t port_no, const u8 i2c_addr, u8 *const data, const u8 cnt)
{
    int file;
    mesa_rc rc = VTSS_RC_ERROR;
    if ((file = vtss_i2c_adapter_open(I2C_PORT2DEV(port_no), i2c_addr)) >= 0) {
        struct i2c_rdwr_ioctl_data packets;
        struct i2c_msg messages[1];

        // Write portion
        messages[0].addr  = i2c_addr;
        messages[0].flags = 0;
        messages[0].len   = cnt;
        messages[0].buf   = data;

        /* Transfer the i2c packets to the kernel and verify it worked */
        packets.msgs  = messages;
        packets.nmsgs = ARRSZ(messages);
        if(ioctl(file, I2C_RDWR, &packets) < 0) {
            T_I("I2C transfer failed!: %s", strerror(errno));
        } else {
            rc = VTSS_RC_OK;
        }
        close(file);
    }
    T_D("i2c write port %d, addr 0x%x, %d bytes - RC %d", port_no, i2c_addr, cnt, rc);
    return rc;
}

static mesa_rc board_conf_get(const char *tag, char *buf, size_t bufsize, size_t *buflen)
{
    mesa_rc rc = MESA_RC_ERROR;
    std::string fdt_file("/proc/device-tree/meba/");
    int fd;
    ssize_t n;

    fdt_file += std::string(tag);
    if (access(fdt_file.c_str(), R_OK) == 0 &&
        (fd = open(fdt_file.c_str(), O_RDONLY)) >= 0) {
        if ((n = read(fd, buf, bufsize)) < 0)
            n = 0;
        buf[n] = '\0';
        close(fd);
        if (strcmp(tag, "pcb") == 0) {
            // The pcb tag is supposed to return a number. On pcb134 and pcb135 a little magic is needed
            if (strstr(buf, "134")) {
                strcpy(buf, "134");
                n = strlen(buf);
            } else if (strstr(buf, "135")) {
                strcpy(buf, "135");
                n = strlen(buf);
            }
        }
        T_D("Read DT tag '%s' as '%s' (len %zd)", tag, buf, n);
        if (buflen) {
            *buflen = n;
        }
        rc = MESA_RC_OK;
    } else {
        auto &c = vtss::appl::main::module_conf_get("meba");
        auto str = c.str_get(tag, "");

        if (str.length()) {
            strncpy(buf, str.c_str(), bufsize);
            buf[bufsize-1] = '\0';    // Ensure terminated
            n = strlen(buf);
            T_D("Read JSON tag '%s' as '%s'", tag, buf);
            if (buflen) {
                *buflen = n;
            }
            rc = MESA_RC_OK;
        }
    }

    if (rc != MESA_RC_OK) {
        T_I("Unable to read tag '%s'", tag);
    }

    return rc;
}

static void board_debug(meba_trace_level_t level,
                        const char *location,
                        uint32_t line_no,
                        const char *fmt,
                        ...)
{
    int m = VTSS_MODULE_ID_MAIN, g = MAIN_TRACE_GRP_BOARD;
    if (TRACE_IS_ENABLED(m, g, level)) {
        va_list ap;
        va_start(ap, fmt);
        vtss_trace_vprintf(m, g, level, location, line_no, fmt, ap);
        va_end(ap);
    }
}

// Mepa synchronisation routines
void mepa_callout_lock(const mepa_lock_t *const lock)
{
    critd_enter(&mepa_crit, lock->function, 0);
}
void mepa_callout_unlock(const mepa_lock_t *const lock)
{
    critd_exit(&mepa_crit, lock->function, 0);
}

// phy driver debug callback
static void phy_driver_trace(const mepa_trace_data_t *data, va_list args)
{
    int m = VTSS_MODULE_ID_API_CI, g;

    switch (data->group) {
    case MEPA_TRACE_GRP_TS:
        g = MESA_TRACE_GROUP_TS;
        break;
    case MEPA_TRACE_GRP_MACSEC:
        g = MESA_TRACE_GROUP_MACSEC;
        break;
    case MEPA_TRACE_GRP_GEN:
    default:
        g = MESA_TRACE_GROUP_PHY;
        break;
    }
    if (TRACE_IS_ENABLED(m, g, data->level)) {
        vtss_trace_vprintf(m, g, data->level, data->location, data->line,
                           data->format, args);
    }
}

/* ================================================================= *
 *  API register access
 * ================================================================= */
//namespace vtss {

class board;

/******************************************************************************/
// spi_reg_io_init()
/******************************************************************************/
static void spi_reg_io_init(void)
{
    int mode = 0;

    if (spi_reg_io_pad > SPI_PADDING_MAX) {
        T_E("Invalid SPI padding %d (valid range is [0; %u]", spi_reg_io_pad, SPI_PADDING_MAX);
        exit(1);
    }

    if ((spi_fd = open(spi_reg_io_dev, O_RDWR)) < 0) {
        T_E("SPI: open(%s) failed: %s", spi_reg_io_dev, strerror(errno));
        exit(1);
    }

    // TODO: Delete this once it has been fixed in the DTS
    // Set SPI in write mode
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) {
        T_E("SPI: ioctl(%s, SPI_IOC_WR_MODE) failed: %s", spi_reg_io_dev, strerror(errno));
        close(spi_fd);
        exit(1);
    }

    // TODO: Delete this once it has been fixed in the DTS
    // Set SPI in read mode
    if (ioctl(spi_fd, SPI_IOC_RD_MODE, &mode) < 0) {
        T_E("SPI: ioctl(%s, SPI_IOC_RD_MODE) failed: %s", spi_reg_io_dev, strerror(errno));
        close(spi_fd);
        exit(1);
    }

    T_I("SPI: %s opened successfully", spi_reg_io_dev);
}

/******************************************************************************/
// slurp_to_int()
/******************************************************************************/
static bool slurp_to_int(const char *dir, const char *file, int *val)
{
    char fn[PATH_MAX];
    FILE *fp;
    bool ret = false;

    snprintf(fn, sizeof(fn), "%s/%s", dir, file);
    if ((fp = fopen(fn, "r"))) {
        if (fscanf(fp, "%i", val)) {
            ret = true;
        }

        fclose(fp);
    }

    return ret;
}

/******************************************************************************/
// uio_find_dev()
// Walk through /sys/class/uio to find a suitable UIO driver
/******************************************************************************/
static bool uio_find_dev(char *iodev, size_t bufsz, int *devno)
{
    const char *top = "/sys/class/uio";
    DIR        *dir;
    struct     dirent *dent;
    char       fn[PATH_MAX], devname[128];
    FILE       *fp;
    bool       found = false;

    if ((dir = opendir(top)) == nullptr) {
        perror(top);
        T_E("UIO: opendir(%s) failed: %s", top, strerror(errno));
        exit (1);
    }

    while ((dent = readdir(dir)) != nullptr) {
        if (dent->d_name[0] == '.') {
            continue;
        }

        snprintf(fn, sizeof(fn), "%s/%s/name", top, dent->d_name);

        if ((fp = fopen(fn, "r")) == nullptr) {
            T_E("UIO: fopen(%s) failed: %s", fn, strerror(errno));
            continue;
        }

        const char *rrd = fgets(devname, sizeof(devname), fp);
        fclose(fp);

        if (rrd == nullptr) {
            T_E("UIO: fgets(%s) failed: %s", fn, strerror(errno));
            continue;
        }

        T_D("UIO: %s -> %s", fn, devname);
        if (!strstr(devname, "mscc_switch")     &&
            !strstr(devname, "vcoreiii_switch")) {
            continue;
        }

        snprintf(iodev, bufsz, "/dev/%s", dent->d_name);
        (void)sscanf(dent->d_name, "uio%d", devno);

        T_I("UIO: Found driver = %s => device = %s on uio%d", devname, iodev, devno);
        found = true;
        break;
    }

    closedir(dir);
    return found;
}

/******************************************************************************/
// uio_region_props_get()
/******************************************************************************/
static bool uio_region_props_get(int devno, int idx, struct reg_map *map)
{
    char       dirname[PATH_MAX], regoff[32];
    const char *meba = "/proc/device-tree/meba";

    snprintf(dirname, sizeof(dirname), "/sys/class/uio/uio%d/maps/map%d", devno, idx);
    snprintf(regoff,  sizeof(regoff),  "regoff%d", idx);

    if (slurp_to_int(dirname, "size", &map->size) &&
        slurp_to_int(dirname, "addr", &map->addr)) {
        if (!slurp_to_int(meba, regoff, &map->offset)) {
            map->offset = 0;
        }

        map->start = map->offset / 4;
        map->end   = map->start + map->size / 4;
        T_I("Got UIO map%d addr = 0x%08x size = 0x%08x offset = 0x%08x", idx, map->addr, map->size, map->offset);
        return true;
    }

    return false;
}

/******************************************************************************/
// uio_reg_io_init()
/******************************************************************************/
static void uio_reg_io_init(int &dev_fd, char *uio_path, size_t uio_path_sz)
{
    char   iodev[NAME_MAX + 64];
    int    devno, i;
    size_t sz;
    bool   map_found = false;

    if (!uio_find_dev(iodev, sizeof(iodev), &devno)) {
        T_E("Unable to find a suitable UIO device for register I/O");
        exit(1);
    }

    // Construct /sys uio path (for interrupt control)
    snprintf(uio_path, uio_path_sz, "/sys/class/uio/%s", iodev + strlen("/dev/"));

    // Open the UIO device file
    dev_fd = open(iodev, O_RDWR);
    if (dev_fd < 1) {
        T_E("open(%s) failed: %s", iodev, strerror(errno));
        exit(1);
    }

    for (i = 0; i < MAX_REG_MAPS; ++i) {
        // Get region properties
        if (!uio_region_props_get(devno, i, &regmap[i])) {
            T_W("%s: No UIO map%d", iodev, i);
            continue;
        }

        sz = regmap[i].size;

        // mmap the UIO device
        if ((regmap[i].ptr = static_cast<volatile u32*>(mmap(nullptr, sz, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, i * getpagesize()))) == MAP_FAILED) {
            T_E("UIO: mmap(%s:map%d) failed: %s", iodev, i, strerror(errno));
            exit(1);
        }

        T_D("Mapped register memory @ %p", regmap[i].ptr);
        map_found = true;
    }

    if (!map_found) {
        T_E("Unable to find maps for device = %s", iodev);
        exit(1);
    }

    base_mem = regmap[0].ptr;
}

class board_factory
{
  public:
    board_factory() {}
    static board *the_board();
  private:
    static board *the_board_ptr;
}; // class board_factory

board *board_factory::the_board_ptr=nullptr;

class board
{
public:
    board() {}

    board(bool use_spi)
    {
        if (use_spi) {
            // Using SPI for register access
            spi_reg_io_init();
            dev_fd      = 0;
            uio_path[0] = '\0';
            reg_read    = spi_reg_read;
            reg_write   = spi_reg_write;
        } else {
            // Using UIO for register access
            uio_reg_io_init(dev_fd, uio_path, sizeof(uio_path));
            reg_read  = uio_reg_read;
            reg_write = uio_reg_write;
        }
    }

    static volatile u32 *addr_to_ptr(u32 addr)
    {
        int i;
        for (i = 0; i < MAX_REG_MAPS; i++) {
            if (addr >= regmap[i].start && addr < regmap[i].end) {
                addr -= regmap[i].start;
                return regmap[i].ptr + addr;
            }
        }
        T_E("Unable to map address 0x%08 to a register area", addr);
        return NULL;
    }

    int irq_fd() {return dev_fd;}

    char uio_path[NAME_MAX + 200];

    mesa_reg_read_t  reg_read;
    mesa_reg_write_t reg_write;

  private:
    int dev_fd;

    static mesa_rc uio_reg_read(const mesa_chip_no_t chip_no, const uint32_t addr, uint32_t *const value)
    {
        volatile uint32_t *ptr = addr_to_ptr(addr);
        *value = PCIE_HOST_CVT(*ptr);
        return VTSS_RC_OK;
    }

    static mesa_rc uio_reg_write(const mesa_chip_no_t chip_no, const uint32_t addr, const uint32_t value)
    {
        volatile uint32_t *ptr = addr_to_ptr(addr);
        *ptr = PCIE_HOST_CVT(value);
        return VTSS_RC_OK;
    }

    static mesa_rc spi_reg_read(const mesa_chip_no_t chip_no, const uint32_t addr, uint32_t *const value)
    {
        uint8_t                 tx[SPI_BYTE_CNT + SPI_PADDING_MAX];
        uint8_t                 rx[sizeof(tx)] = {};
        uint32_t                siaddr = TO_SPI(addr), rxword;
        struct spi_ioc_transfer tr = {};

        memset(tx, 0xff, sizeof(tx));
        tx[0] = (uint8_t)(siaddr >> 16);
        tx[1] = (uint8_t)(siaddr >>  8);
        tx[2] = (uint8_t)(siaddr >>  0);

        tr.tx_buf        = (uint64_t)tx;
        tr.rx_buf        = (uint64_t)rx;
        tr.len           = (uint32_t)SPI_BYTE_CNT + spi_reg_io_pad;
        tr.speed_hz      = (uint32_t)spi_reg_io_freq;
        tr.bits_per_word = 8;

        if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
            T_E("spi_read: %s", strerror(errno));
            return VTSS_RC_ERROR;
        }

        rxword = (rx[3 + spi_reg_io_pad] << 24) | (rx[4 + spi_reg_io_pad] << 16) | (rx[5 + spi_reg_io_pad] << 8) | (rx[6 + spi_reg_io_pad] << 0);
        *value = rxword;

        T_N("Read(0x%06x) = 0x%08x", siaddr, rxword);

        return VTSS_RC_OK;
    }

    static mesa_rc spi_reg_write(const mesa_chip_no_t chip_no, const uint32_t addr, const uint32_t value)
    {
        uint8_t                 tx[SPI_BYTE_CNT] = {};
        uint8_t                 rx[sizeof(tx)];
        uint32_t                siaddr = TO_SPI(addr);
        struct spi_ioc_transfer tr = {};

        tx[0] = (uint8_t)(siaddr >> 16) | 0x80;
        tx[1] = (uint8_t)(siaddr >>  8);
        tx[2] = (uint8_t)(siaddr >>  0);
        tx[3] = (uint8_t)(value  >> 24);
        tx[4] = (uint8_t)(value  >> 16);
        tx[5] = (uint8_t)(value  >>  8);
        tx[6] = (uint8_t)(value  >>  0);

        T_N("Write(0x%06x) = 0x%08x", siaddr, value);

        tr.tx_buf        = (uint64_t)tx;
        tr.rx_buf        = (uint64_t)rx;
        tr.len           = sizeof(tx);
        tr.speed_hz      = spi_reg_io_freq;
        tr.bits_per_word = 8;

        if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
            T_E("spi_write: %s", strerror(errno));
            return VTSS_RC_ERROR;
        }

        return VTSS_RC_OK;
    }
};

class board_serval_ref : public board
{
  public:
    board_serval_ref() : board(spi_reg_io)
    {
    }
};

board *board_factory::the_board()
{
    if (!the_board_ptr) {
        the_board_ptr=new board_serval_ref();
    }
    return the_board_ptr;
}

#ifdef VTSS_SW_OPTION_DEBUG
static u64 reg_reads[2];
static u64 reg_writes[2];
#endif


static mesa_rc reg_read(const mesa_chip_no_t chip_no,
                        const u32            addr,
                        u32                  *const value)
{
    int rc=board_factory::the_board()->reg_read(chip_no, addr, value);

#ifdef VTSS_SW_OPTION_DEBUG
    vtss_global_lock(__FILE__, __LINE__);
    reg_reads[chip_no]++;
    vtss_global_unlock(__FILE__, __LINE__);
#endif
    return rc;
}

static mesa_rc reg_write(const mesa_chip_no_t chip_no,
                         const u32            addr,
                         const u32            value)
{
    int rc=board_factory::the_board()->reg_write(chip_no,addr,value);
#ifdef VTSS_SW_OPTION_DEBUG
    vtss_global_lock(__FILE__, __LINE__);
    reg_writes[chip_no]++;
    vtss_global_unlock(__FILE__, __LINE__);
#endif
    return rc;
}

static mesa_rc clock_read_write(const u32 addr, u32 *value, BOOL read)
{
    if (read) {
        return reg_read(0, addr, value);
    } else {
        return reg_write(0, addr, *value);
    }
}
static mesa_rc clock_read(const u32 addr, u32 *const value)
{
    mesa_rc rc;
    rc = clock_read_write(addr, value, TRUE);
    return rc;
}
static mesa_rc clock_write(const u32 addr, const u32 value)
{
    u32 val = value;
    return clock_read_write(addr, &val, FALSE);
}

static uint32_t loop_port_up_inj_get()
{
    static bool     initialized;
    static uint32_t loop_port_up_inj;

    if (!initialized) {
        char param[20];
        loop_port_up_inj = MESA_PORT_NO_NONE;
        if (board_info.conf_get("loop_port_up_inj", param, sizeof(param), 0) == MESA_RC_OK) {
            uint32_t port;
            char     *end;
            port = strtoul(param, &end, 0);
            if (*end == '\0' || port != 0 || port <= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
                loop_port_up_inj = port;
            }
        }

        initialized = true;
    }

    return loop_port_up_inj;
}

// Wrapper function for meba_gpio_func_info_get()
// which is implemented in MEBA
static mesa_rc gpio_func_info_get(const mesa_inst_t inst, mesa_gpio_func_t gpio_func,  mesa_gpio_func_info_t *info)
{
    if (board_instance->api.meba_gpio_func_info_get != NULL) {
        return board_instance->api.meba_gpio_func_info_get(board_instance, gpio_func, info);
    } else {
        return MESA_RC_ERROR;
    }
}

// Wrapper function for meba_serdes_tap_get()
// which is implemented in MEBA
static mesa_rc serdes_tap_get(const mesa_inst_t inst, mesa_port_no_t port_no,
                              mesa_port_speed_t speed, mesa_port_serdes_tap_enum_t tap, uint32_t *const value)
{
    if (board_instance->api.meba_serdes_tap_get != NULL) {
        return board_instance->api.meba_serdes_tap_get(board_instance, port_no, speed, tap, value);
    } else {
        return MESA_RC_NOT_IMPLEMENTED;
    }
}

/* ================================================================= *
 *  API initialization
 * ================================================================= */

uint32_t VTSS_APPL_CACHE_MEBA_CAP_BOARD_PORT_COUNT = 0;
uint32_t VTSS_APPL_CACHE_MEBA_CAP_BOARD_PORT_MAP_COUNT = 0;

static uint32_t vtss_appl_capability_(const void *_inst_unused_, int cap)
{
    uint32_t c = 0;

    T_D("cap = %d", cap);

    switch (cap) {
    case VTSS_APPL_CAP_ISID_CNT:
        c = VTSS_ISID_CNT;
        break;
    case VTSS_APPL_CAP_ISID_END:
        c = VTSS_ISID_END;
        break;
    case VTSS_APPL_CAP_PORT_USER_CNT:
        c = PORT_USER_CNT;
        break;
    case VTSS_APPL_CAP_L2_PORT_CNT:
        c = L2_MAX_PORTS_;
        break;
    case VTSS_APPL_CAP_L2_LLAG_CNT:
        c = L2_MAX_LLAGS_;
        break;
    case VTSS_APPL_CAP_L2_GLAG_CNT:
        c = L2_MAX_GLAGS_;
        break;
    case VTSS_APPL_CAP_L2_POAG_CNT:
        c = L2_MAX_POAGS_;
        break;
    case VTSS_APPL_CAP_MSTI_CNT:
        c = N_L2_MSTI_MAX;
        break;
    case VTSS_APPL_CAP_PACKET_RX_PORT_CNT:
        c = (fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) + 1);
        break;
    case VTSS_APPL_CAP_PACKET_RX_PRIO_CNT:
        c = (MESA_PRIO_CNT + 1);
        break;
    case VTSS_APPL_CAP_PACKET_TX_PORT_CNT:
        c = (fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) + 2);
        break;
    case VTSS_APPL_CAP_VLAN_VID_CNT:
        c = MESA_VIDS;
        break;
    case VTSS_APPL_CAP_VLAN_COUNTERS:
#if defined(VTSS_SW_OPTION_CFM)
        c = 0;
#else
        c = fast_cap(MESA_CAP_L2_VLAN_COUNTERS);
#endif
        break;
    case VTSS_APPL_CAP_VLAN_FLOODING:
        c = fast_cap(MESA_CAP_L2_FRER);
        break;
    case VTSS_APPL_CAP_VLAN_USER_INT_CNT:
#if defined(VTSS_SW_OPTION_VLAN)
        c = vlan_user_int_cnt();
#endif
        break;
#if defined(VTSS_SW_OPTION_AFI)
    case VTSS_APPL_CAP_AFI_SINGLE_CNT:
        c = AFI_SINGLE_CNT;
        break;
    case VTSS_APPL_CAP_AFI_MULTI_CNT:
        c = AFI_MULTI_CNT;
        break;
    case VTSS_APPL_CAP_AFI_MULTI_LEN_MAX:
        c = AFI_MULTI_LEN_MAX;
        break;
#endif
    case VTSS_APPL_CAP_MSTP_PORT_CONF_CNT:
        // MSTP: One config per port and one common config for all aggregations
        c = (fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) + 1);
        break;
    case VTSS_APPL_CAP_MSTP_MSTI_CNT:
        c = N_L2_MSTI_MAX;
        break;
#if defined(VTSS_SW_OPTION_QOS)
    case VTSS_APPL_CAP_QOS_PORT_QUEUE_CNT:
        c = VTSS_APPL_QOS_PORT_QUEUE_CNT;
        break;
    case VTSS_APPL_CAP_QOS_WRED_DPL_CNT:
        c = (fast_cap(MESA_CAP_QOS_DPL_CNT) - 1);
        break;
    case VTSS_APPL_CAP_QOS_WRED_GRP_CNT:
        c = fast_cap(MESA_CAP_QOS_WRED_GROUP_CNT);
        break;
#endif
#if defined(VTSS_SW_OPTION_DOT1X)
    case VTSS_APPL_CAP_NAS_STATE_MACHINE_CNT:
        c = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
#if defined(NAS_USES_PSEC) && defined(NAS_MULTI_CLIENT)
        if (c < PSEC_MAC_ADDR_ENTRY_CNT) {
            c = PSEC_MAC_ADDR_ENTRY_CNT;
        }
#endif
        break;
#endif
    case VTSS_APPL_CAP_ACL_ACE_CNT:
        switch (fast_cap(MESA_CAP_MISC_CHIP_FAMILY)) {
        case MESA_CHIP_FAMILY_CARACAL:
            c = 256;
            break;
        case MESA_CHIP_FAMILY_OCELOT:
            c = 128;
            break;
        case MESA_CHIP_FAMILY_SERVALT:
            c = 128; // One VCAP_SUPER block
            break;
        case MESA_CHIP_FAMILY_JAGUAR2:
            c = 512; // One VCAP_SUPER block
            break;
        case MESA_CHIP_FAMILY_SPARX5:
        case MESA_CHIP_FAMILY_LAN969X:
            c = 512; // One VCAP_SUPER block
            break;
        default:
            c = 128;
            break;
        }
        break;
    case VTSS_APPL_CAP_MAX_ACL_RULES_PR_PTP_CLOCK:
        if (fast_cap(MESA_CAP_TS_DELAY_REQ_AUTO_RESP)) {  // in auto resp mode we need a rule pr. interface
            c = 7 + 2 * fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
        } else {
            c = 12;
        }
        break;

    case VTSS_APPL_CAP_LLDP_REMOTE_ENTRY_CNT:
#if defined(VTSS_SW_OPTION_LLDP)
        c = LLDP_REMOTE_ENTRIES;
#endif
        break;

    case VTSS_APPL_CAP_LLDPMED_POLICIES_CNT:
#ifdef VTSS_SW_OPTION_LLDP_MED
        c = LLDPMED_POLICIES_CNT;
#endif
        break;

    case VTSS_APPL_CAP_IP_INTERFACE_CNT:
        c = fast_cap(MESA_CAP_L3_RLEG_CNT);
        if (!c) {
            c = 8;
        }
        break;

    case VTSS_APPL_CAP_IP_ROUTE_CNT:
        c = fast_cap(VTSS_APPL_CAP_IP_INTERFACE_CNT);
        if (c < 32) {
            c = 32;
        }
        break;

    case VTSS_APPL_CAP_DHCP_HELPER_USER_CNT:
#ifdef VTSS_SW_OPTION_DHCP_HELPER
        c = DHCP_HELPER_USER_CNT;
#endif
        break;

    case VTSS_APPL_CAP_AGGR_MGMT_GLAG_END:
#ifdef VTSS_SW_OPTION_AGGR
        c = AGGR_MGMT_GLAG_END_;
#endif
        break;

    case VTSS_APPL_CAP_AGGR_MGMT_GROUPS:
#ifdef VTSS_SW_OPTION_AGGR
         c = AGGR_LLAG_CNT_ + AGGR_GLAG_CNT + 1;
#endif
        break;

    case VTSS_APPL_CAP_LACP_MAX_PORTS_IN_AGGR:
#ifdef VTSS_SW_OPTION_LACP
         c = AGGR_MGMT_LAG_PORTS_MAX_;
#endif
        break;

    case VTSS_APPL_CAP_VOICE_VLAN_LLDP_TELEPHONY_MAC_ENTRIES_CNT:
#ifdef VTSS_SW_OPTION_VOICE_VLAN
         c = 4 * fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
#endif
        break;

    case VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT:
#define SYNCE_STATION_CLOCK_CNT 1
        c = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) + SYNCE_STATION_CLOCK_CNT;
        break;

    case VTSS_APPL_CAP_PTP_CLOCK_CNT:
#ifdef VTSS_SW_OPTION_PTP
        c = PTP_CLOCK_INSTANCES;
#endif
        break;

    case VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_AND_PTP_CNT:
#ifdef VTSS_SW_OPTION_PTP
        c = PTP_CLOCK_INSTANCES + fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) + SYNCE_STATION_CLOCK_CNT;
#else
        c = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) + SYNCE_STATION_CLOCK_CNT;
#endif
        break;

    case VTSS_APPL_CAP_SYNCE_NOMINATED_CNT:
        switch (fast_cap(MESA_CAP_MISC_CHIP_FAMILY)) {
            case MESA_CHIP_FAMILY_SERVALT:
                if (fast_cap(MESA_CAP_CLOCK)) {
                    c = 2;
                } else {
                    c = 3;
                }
                break;
            default:
                c = 3;
                break;
        }
        break;
    case VTSS_APPL_CAP_SYNCE_SELECTED_CNT:
        c = fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT) + 1;
        break;

    case VTSS_APPL_CAP_PACKET_RX_MTU:
        return    fast_cap(MESA_CAP_PORT_FRAME_LENGTH_MAX);
        break;

    case VTSS_APPL_CAP_AFI:
        return fast_cap(MESA_CAP_AFI_V1) || fast_cap(MESA_CAP_AFI_V2);

    case VTSS_APPL_CAP_ZARLINK_SERVO_TYPE:
#if defined(VTSS_SW_OPTION_ZLS30387)
        c = zl_3038x_module_type();
#endif
        break;

    case VTSS_APPL_CAP_LOOP_PORT_UP_INJ:
        c = loop_port_up_inj_get();
        break;

    case VTSS_APPL_CAP_PORT_CNT_PTP_PHYS_AND_VIRT:
        c = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) + fast_cap(MESA_CAP_TS_IO_CNT);
        break;

    default:
        if (board_instance) {
            c = meba_capability(board_instance, cap);
        } else {
            VTSS_ASSERT(0);
        }
        break;
    }

    return c;
}

const char *VTSS_F;
const char *VTSS_C;
int VTSS_L;

static bool mesa_inst_created;
uint32_t vtss_appl_capability(const void *_inst_unused_, int cap)
{
    // Capability allocation
    //
    // MESA:     0..9999
    // APPL: 10000..19999
    // MEBA: 50000..59999

    // We cannot be sure that trace has been initialized, so use printf() and
    // VTSS_ASSERT() in the following
    if (cap <= 9999) {
        // MESA
        if (!mesa_inst_created) {
            printf("%s:%d %s: Cannot invoke mesa_capability(%d = 0x%08x) before mesa_inst_create() has been invoked\n", VTSS_F, VTSS_L, VTSS_C, cap, cap);
            VTSS_ASSERT(0);
        }

        return mesa_capability(nullptr, cap);
    } else if (cap >= 10000 && cap < 20000) {
        // APPL
        return vtss_appl_capability_(nullptr, cap);
    } else if (cap >= 50000 && cap < 60000) {
        // MEBA
        if (!board_instance) {
            printf("%s:%d %s: Cannot invoke meba_capability(%d = 0x%08x) before meba_initialize() has been invoked\n", VTSS_F, VTSS_L, VTSS_C, cap, cap);
            VTSS_ASSERT(0);
        }

        return meba_capability(board_instance, cap);
    }

    printf("%s:%d %s: Unknown capability %d = 0x%08x\n", VTSS_F, VTSS_L, VTSS_C, cap, cap);
    VTSS_ASSERT(0);
    return 0xffffffff;
}

static void api_init(void)
{
    mesa_rc            rc;
    mesa_inst_create_t create;
    mesa_init_conf_t   conf;
    mesa_port_no_t     port_no;
    CapArray<mesa_port_map_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_map;

    // Initialize SPI slave before doing anything else in the API
    if (spi_reg_io) {
        mesa_spi_slave_init_t s;
        s.reg_write = reg_write;
        s.reg_read  = reg_read;
        s.endian    = MESA_SPI_ENDIAN_BIG;
        s.bit_order = MESA_SPI_BIT_ORDER_MSB_FIRST;
        s.padding   = spi_reg_io_pad;
        if ((rc = mesa_spi_slave_init(&s)) != MESA_RC_OK) {
            T_E("mesa_spi_slave_init() failed: %s", error_txt(rc));
            exit(1);
        }
    }

    /* Initialize Board API */
    board_info.reg_read = reg_read;
    board_info.reg_write = reg_write;

    board_info.i2c_read = i2c_read;
    board_info.i2c_write = i2c_write;
    board_info.conf_get = board_conf_get;
    board_info.debug = board_debug;
    board_info.trace = phy_driver_trace;
    board_info.lock_enter = mepa_callout_lock;
    board_info.lock_exit = mepa_callout_unlock;
    board_instance = meba_initialize(sizeof(board_info), &board_info);
    VTSS_ASSERT(board_instance != NULL);

    VTSS_APPL_CACHE_MEBA_CAP_BOARD_PORT_COUNT     = meba_capability(board_instance, MEBA_CAP_BOARD_PORT_COUNT);
    VTSS_APPL_CACHE_MEBA_CAP_BOARD_PORT_MAP_COUNT = meba_capability(board_instance, MEBA_CAP_BOARD_PORT_MAP_COUNT);

    mesa_inst_get(board_instance->props.target, &create);
    mesa_inst_create(&create, NULL);
    mesa_inst_created = true;

    (void) mesa_init_conf_get(NULL, &conf);
    conf.reg_read = board_info.reg_read;
    conf.reg_write = board_info.reg_write;
    conf.spi_bus = spi_reg_io;

    if (fast_cap(MESA_CAP_CLOCK)) {
        conf.clock_read  = clock_read;
        conf.clock_write = clock_write;
    }

    if (mesa_capability(NULL, MESA_CAP_INIT_CORE_CLOCK)) {
        conf.core_clock.ref_freq = board_instance->props.ref_freq;
    }
    // Skip switch core reset in the API, as this is done
    // by the Linux kernel.
    conf.skip_switch_reset = TRUE;





    conf.mux_mode = board_instance->props.mux_mode;

    conf.using_ufdma = !spi_reg_io;

    conf.loopback_bw_mbps = 0;
    if (fast_cap(MESA_CAP_PORT_QS_CONF)) {
        mesa_qs_conf_t *qs_conf;
        ulong          size;

        if ((qs_conf = (mesa_qs_conf_t *)conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_QUEUE_CONF, &size)) != NULL) {
            conf.qs_conf = *qs_conf; // The stored queue system configuration
        }
    }

    // VLAN counter support, appl capability function called directly, since it is not registered yet
    conf.vlan_counters_disable = (vtss_appl_capability(NULL, VTSS_APPL_CAP_VLAN_COUNTERS) ? FALSE : TRUE);
    conf.psfp_counters_enable = true;

    conf.gpio_func_info_get = gpio_func_info_get;
    conf.serdes_tap_get = serdes_tap_get;

    rc = mesa_init_conf_set(NULL, &conf);
    VTSS_ASSERT(rc == MESA_RC_OK);

    /* Open up API only after initialization */
    API_CRIT_EXIT(__FUNCTION__);

    // Do a board init before the port map is established in case of any changes
    meba_reset(board_instance, MEBA_BOARD_INITIALIZE);

    /* Setup port map for board */
    if (meba_capability(board_instance, MEBA_CAP_BOARD_PORT_MAP_COUNT) > fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        T_E("Consistency issue: fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) %d, board has %d ports",
            fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT), meba_capability(board_instance, MEBA_CAP_BOARD_PORT_MAP_COUNT));
    }

    for (port_no = 0; port_no < port_custom_table.size(); port_no++) {
        if (meba_port_entry_get(board_instance, port_no, &port_custom_table[port_no]) == MESA_RC_OK) {
            port_map[port_no] = port_custom_table[port_no].map;
        } else {
            memset(&port_map[port_no], 0, sizeof(port_map[0]));
            port_map[port_no].chip_port = -1;    // Unused entry
        }
    }

    rc = mesa_port_map_set(NULL, fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT), port_map.data());
    VTSS_ASSERT(rc == MESA_RC_OK);
}

static void api_start(void)
{
    // Start off any background activities on the board if interrupts are
    // supported.
    if (board_factory::the_board()->irq_fd() > 0) {
        vtss::board_subjects_start(board_factory::the_board()->irq_fd(), board_factory::the_board()->uio_path);
    }
}

u32 vtss_api_if_chip_count(void)
{
    return 1;    // Legacy
}

void vtss_api_if_spi_reg_io_set(const char *spi_dev, int spi_pad, int spi_freq)
{
    strncpy(spi_reg_io_dev, spi_dev, sizeof(spi_reg_io_dev));
    spi_reg_io_pad  = spi_pad;
    spi_reg_io_freq = spi_freq;
    spi_reg_io      = true;
}

#if defined(VTSS_SW_OPTION_PORT_LOOP_TEST)
/* Port loopback test                                                                                 */
/* -----------------                                                                                  */
/* This is a standalone function which uses the API to perform a port loopback test.                  */
/* This function should only be used  before the main application is started.                         */
/* Each switch port is enabled and the phy is reset and set in loopback.                              */
/* The CPU transmits 1 frame to each port and verifies that it is received again to the CPU buffer.   */
/* After the test completes the MAC address and counters are cleared.                                 */
static void port_loop_test(void)
{
    mesa_port_conf_t        conf;
    vtss_phy_reset_conf_t   phy_reset;
    mesa_port_no_t          port_no;
    BOOL                    testpassed=1;
    mesa_mac_table_entry_t  entry;
    u8                      frame[100];
    mesa_packet_rx_header_t header;

    /* Initilize */
    memset(frame,0,sizeof(frame));
    memset(&phy_reset,0,sizeof(vtss_phy_reset_conf_t));
    memset(&conf, 0, sizeof(mesa_port_conf_t));
    memset(&entry, 0, sizeof(mesa_mac_table_entry_t));
    frame[5] = 0xFF;  /* Test frame DMAC: 00-00-00-00-00-00-FF */
    frame[11] = 0x1;  /* Test frame SMAC: 00-00-00-00-00-00-01 */
    conf.if_type = MESA_PORT_INTERFACE_SGMII;
    conf.speed = MESA_SPEED_1G;
    conf.fdx = 1;
    conf.max_frame_length = 1518;
    entry.vid_mac.vid = 1;
    entry.locked = TRUE;
    entry.vid_mac.mac.addr[5] = 0xff;
    entry.copy_to_cpu = 1;
    (void) mesa_mac_table_add(NULL, &entry);

    /* Board and Phy init */
    port_custom_init();
    port_custom_reset();
    for(port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
        if (port_is_phy(port_no)) {
            vtss_phy_post_reset(NULL, port_no);
            break;
        }
    }
    /* Reset and enable switch ports and phys. Enable phy loopback. */
    T_D("Port Loop Test Start:");
    for(port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
        if (port_is_phy(port_no)) {
            conf.if_type = MESA_PORT_INTERFACE_SGMII;
            (void)vtss_phy_reset_get(NULL, port_no, &phy_reset);
            phy_reset.mac_if = port_custom_table[port_no].mac_if;
            (void)vtss_phy_reset(NULL, port_no, &phy_reset);
            (void)vtss_phy_write(NULL, port_no, 0, 0x4040);
        } else {
            conf.if_type = MESA_PORT_INTERFACE_LOOPBACK;
        }
        (void)mesa_port_conf_set(NULL, port_no, &conf);
    }
    /* Wait while the Phys syncs up with the SGMII interface */
    VTSS_MSLEEP(2000);

    /* Enable frame forwarding and send one frame towards each port from the CPU.  Verify that the frame is received again. */
    for(port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
        (void)mesa_port_state_set(NULL, port_no, 1);
        (void)mesa_packet_tx_frame_port(NULL, port_no, frame, 64);
        VTSS_MSLEEP(1); /* Wait until the frame is received again */
        if (mesa_packet_rx_frame_get(NULL, 0, &header, frame, 100) != VTSS_RC_OK) {
            T_E("Port %lu failed self test \n",port_no);
            testpassed = 0;
        }
        (void)mesa_port_state_set(NULL, port_no, 0);
        if (port_is_phy(port_no)) {
            (void)vtss_phy_write(NULL, port_no, 0, 0x8000);
        }
        (void)mesa_port_counters_clear(NULL, port_no);
    }
    /* Clean up */
   (void)mesa_mac_table_del(NULL, &entry.vid_mac);
   (void)mesa_mac_table_flush(NULL);
    if (testpassed) {
        T_D("...Passed\n");
    }
}
#endif /* VTSS_SW_OPTION_PORT_LOOP_TEST */

mesa_rc vtss_api_if_init(vtss_init_data_t *data)
{
    int i;

    switch (data->cmd) {
    case INIT_CMD_EARLY_INIT:
        /* Copy ail groups to cil */
        for (i = 0; i < ARRSZ(trace_grps_cil); i++) {
            trace_grps_cil[i] = trace_grps_ail[i];
            trace_grps_cil[i].flags &= ~VTSS_TRACE_FLAGS_INIT;
        }

        // Register CIL trace groups. The AIL groups are registered statically
        // already.
        vtss_trace_reg_init(&trace_reg_cil, trace_grps_cil, ARRSZ(trace_grps_cil));
        vtss_trace_register(&trace_reg_cil);

        /* Update API trace levels to initialization settings */
        vtss_api_trace_update_do(FALSE);

        /* Create API semaphore (initially locked) */
        critd_init_legacy(&vtss_api_crit, "mesa", VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);

        // Create API mutex for MEPA
        critd_init(&mepa_crit, "mepa", VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);

        // Create application API mutex (initially unlocked)
        // This mutex is used if two (or more) API calls are required to be
        // consistent (e.g. XXX_conf_get() followed by an XXX_conf_set()) and
        // needed to be called by more than one module.
        critd_init(&vtss_appl_api_crit, "vtss_appl_api", VTSS_MODULE_ID_API_AI, CRITD_TYPE_MUTEX);

        // Initialize API. This makes e.g. board_instance available for modules
        // to use in INIT_CMD_INIT.
        api_init();
        break;

    case INIT_CMD_INIT:
#if defined(VTSS_SW_OPTION_PORT_LOOP_TEST)
        /* Perform a port loop test */
        port_loop_test();
#endif /*VTSS_SW_OPTION_PORT_LOOP_TEST*/
        break;
    case INIT_CMD_START:
        api_start();

        /* Update API trace levels to settings loaded from Flash by trace module */
        vtss_api_trace_update();
        break;
    default:
        break;
    }

    return VTSS_RC_OK;
}
