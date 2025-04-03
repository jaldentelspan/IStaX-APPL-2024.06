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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#include "main.h"
#include "misc_api.h"
#include "critd_api.h"
#include "msg_api.h"
#include "vtss_api_if_api.h"
#include "misc.h"
#include "version.h"
#include "port_api.h" // For port_is_phy()
#include "vtss_os_wrapper.h"
#include "vtss_usb.h"
#include "board_if.h" /* For vtss_api_chipid() */

#include <microchip/ethernet/board/api.h>

#if defined(VTSS_SW_OPTION_SYSUTIL)
#include "sysutil_api.h"
#endif /* defined(VTSS_SW_OPTION_SYSUTIL) */

#if defined(VTSS_SW_OPTION_DAYLIGHT_SAVING)
#include "daylight_saving_api.h"
#endif /* defined(VTSS_SW_OPTION_DAYLIGHT_SAVING) */

#include <fcntl.h>
#include <dirent.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>  /* I2C support */
#include "backtrace.hxx"

#if defined(VTSS_SW_OPTION_PHY)
#include "phy_api.h"
#endif /* defined(VTSS_SW_OPTION_PHY) */

#include "mgmt_api.h" // For mgmt_txt2ipv4
#include "icli_api.h"

#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
#include "thread_load_monitor_api.hxx"
#endif /* defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR) */

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_MISC

static misc_global_t misc;
extern void vtss_ifindex_init(void);
static const vtss_software_type_t software_type = (vtss_software_type_t)VTSS_SW_ID;
void misc_any_init();

static vtss_trace_reg_t trace_reg =
{
    VTSS_TRACE_MODULE_ID, "misc", "Miscellaneous Control"
};

static vtss_trace_grp_t trace_grps[] =
{
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_USB] = {
        "usb",
        "USB storage control",
        VTSS_TRACE_LVL_ERROR
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/****************************************************************************/
/*  Stack functions                                                         */
/****************************************************************************/

/* Allocate message buffer */
static misc_msg_t *misc_msg_alloc(misc_msg_buf_t *buf, misc_msg_id_t msg_id)
{
    buf->sem = &misc.sem;
    buf->msg = &misc.msg;
    vtss_sem_wait(buf->sem);
    buf->msg->msg_id = msg_id;
    return buf->msg;
}

static void misc_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
    vtss_sem_post((vtss_sem_t *)contxt);
}

static void misc_msg_tx(misc_msg_buf_t *buf, vtss_isid_t isid, size_t len)
{
    msg_tx_adv(buf->sem, misc_msg_tx_done, MSG_TX_OPT_DONT_FREE,
               VTSS_MODULE_ID_MISC, isid, buf->msg, len +
               MSG_TX_DATA_HDR_LEN(misc_msg_t, data));
}

static BOOL misc_msg_rx(void *contxt, const void * const rx_msg, const size_t len,
                        const vtss_module_id_t modid, const uint32_t isid)
{
    misc_msg_t     *msg = (misc_msg_t *)rx_msg;
    misc_msg_buf_t buf;

    switch (msg->msg_id) {
    case MISC_MSG_ID_REG_READ_REQ:
    {
        mesa_chip_no_t chip_no = msg->data.reg.chip_no;
        ulong          addr = msg->data.reg.addr;

        T_D("REG_READ_REQ, isid: %d, chip_no: %u, addr: 0x" VPRIFlx("08"), isid, chip_no, addr);
        msg = misc_msg_alloc(&buf, MISC_MSG_ID_REG_READ_REP);
        msg->data.reg.addr = addr;
        mesa_reg_read(NULL, chip_no, addr, &msg->data.reg.value);
        misc_msg_tx(&buf, isid, sizeof(msg->data.reg));
        break;
    }
    case MISC_MSG_ID_REG_WRITE_REQ:
    {
        mesa_chip_no_t chip_no = msg->data.reg.chip_no;
        ulong          addr = msg->data.reg.addr;
        ulong          value = msg->data.reg.value;

        T_D("REG_WRITE_REQ, isid: %d, chip_no: %u, addr: 0x" VPRIFlx("08")", value: 0x" VPRIFlx("08"),
            isid, chip_no, addr, value);
        mesa_reg_write(NULL, chip_no, addr, value);
        break;
    }
    case MISC_MSG_ID_REG_READ_REP:
    {
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
        mesa_chip_no_t chip_no = msg->data.reg.chip_no;
        ulong          addr = msg->data.reg.addr;
#endif
        ulong          value = msg->data.reg.value;

        T_D("REG_READ_REP, isid: %d, chip_no: %u, addr: 0x" VPRIFlx("08")", value: 0x" VPRIFlx("08"),
            isid, chip_no, addr, value);
        misc.value = value;
        vtss_flag_setbits(&misc.flags, MISC_FLAG_READ_DONE);
        break;
    }
    case MISC_MSG_ID_PHY_READ_REQ:
    {
        mesa_port_no_t port_no = msg->data.phy.port_no;
        uint           addr = msg->data.phy.addr;

        T_D("PHY_READ_REQ, isid: %d, port_no: %u, addr: 0x%08x", isid, port_no, addr);
        msg = misc_msg_alloc(&buf, MISC_MSG_ID_PHY_READ_REP);
        msg->data.phy.port_no = port_no;
        msg->data.phy.addr = addr;

        if (msg->data.phy.mmd_access) {
            if (port_is_phy(port_no)) {
                vtss_phy_mmd_read(PHY_INST, port_no, msg->data.phy.devad, addr, &msg->data.phy.value) ;
            } else if (fast_cap(MESA_CAP_PORT_10G)) {
                mesa_port_mmd_read(NULL, port_no, msg->data.phy.devad, addr, &msg->data.phy.value) ;
            }
        } else {
            vtss_phy_read(PHY_INST, port_no, addr, &msg->data.phy.value);
        }
        misc_msg_tx(&buf, isid, sizeof(msg->data.phy));
        break;
    }
    case MISC_MSG_ID_PHY_WRITE_REQ:
    {
        mesa_port_no_t port_no = msg->data.phy.port_no;
        uint           addr = msg->data.phy.addr;
        ushort         value = msg->data.phy.value;

        T_D("PHY_WRITE_REQ, isid: %d, port_no: %u, addr: 0x%08x, value: %04x",
            isid, port_no, addr, value);
        vtss_phy_write(PHY_INST, port_no, addr, value);
        break;
    }
    case MISC_MSG_ID_PHY_READ_REP:
    {
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
        mesa_port_no_t port_no = msg->data.phy.port_no;
        uint           addr = msg->data.phy.addr;
        ushort         value = msg->data.phy.value;
#endif

        T_D("PHY_READ_REP, isid: %d, port_no: %u, addr: 0x%08x, value: %04x",
            isid, port_no, addr, value);
        misc.value = msg->data.phy.value;
        vtss_flag_setbits(&misc.flags, MISC_FLAG_READ_DONE);
        break;
    }
    case MISC_MSG_ID_SUSPEND_RESUME:
    {
        vtss_init_data_t data = { .cmd = INIT_CMD_SUSPEND_RESUME };
        data.resume = msg->data.suspend_resume.resume;
        data.isid = isid;
        T_D("SUSPEND_RESUME, isid: %d, resume: %d", data.isid, data.resume);
        init_modules(&data);
        break;
    }
    default:
        T_W("unknown message ID: %d", msg->msg_id);
        break;
    }
    return TRUE;
}

/* Determine if ISID must be accessed locally */
static BOOL misc_stack_local(vtss_isid_t isid)
{
    return (isid == VTSS_ISID_LOCAL || !msg_switch_is_primary() || msg_switch_is_local(isid));
}

/* Determine if ISID is ready and read operation idle */
static BOOL misc_stack_ready(vtss_isid_t isid, BOOL write)
{
    return (msg_switch_exists(isid) &&
            (write || vtss_flag_poll(&misc.flags, MISC_FLAG_READ_IDLE,
                                     VTSS_FLAG_WAITMODE_OR_CLR)));
}

/* Wait for read response */
static void misc_stack_read(uint32_t *value)
{
    vtss_tick_count_t time;

    /* Wait for DONE event */
    time = vtss_current_time() + VTSS_OS_MSEC2TICK(5000);
    vtss_flag_timed_wait(&misc.flags, MISC_FLAG_READ_DONE, VTSS_FLAG_WAITMODE_OR_CLR, time);
    *value = misc.value;
    vtss_flag_setbits(&misc.flags, MISC_FLAG_READ_IDLE);
}

/* Read/write register from ISID */
static mesa_rc misc_stack_reg(vtss_isid_t isid, mesa_chip_no_t chip_no, ulong addr, uint32_t *value, BOOL write)
{
    mesa_rc          rc = VTSS_RC_OK;
    misc_msg_buf_t   buf;
    misc_msg_t       *msg;

    if (misc_stack_local(isid)) {
        /* Local access */
        if (write) {
            rc = mesa_reg_write(NULL, chip_no, addr, *value);
        } else {
            rc = mesa_reg_read(NULL, chip_no, addr, value);
        }
    } else if (misc_stack_ready(isid, write)) {
        /* Stack access */
        msg = misc_msg_alloc(&buf, write ? MISC_MSG_ID_REG_WRITE_REQ :
                             MISC_MSG_ID_REG_READ_REQ);
        msg->data.reg.chip_no = chip_no;
        msg->data.reg.addr = addr;
        msg->data.reg.value = *value;
        misc_msg_tx(&buf, isid, sizeof(msg->data.reg));
        if (write) {
            T_D("REG_WRITE_REQ, isid: %d, chip_no: %u, addr: 0x" VPRIFlx("08")", value: 0x%08x",
                isid, chip_no, addr, *value);
        } else {
            T_D("REG_READ_REQ, isid: %d, chip_no: %u, addr: 0x" VPRIFlx("08"), isid, chip_no, addr);
            misc_stack_read(value);
        }
    }
    return rc;
}

/* Read/write PHY register from ISID */
static mesa_rc misc_stack_phy(vtss_isid_t isid, mesa_port_no_t port_no,
                              uint reg, uint page, ushort *value, BOOL write, BOOL mmd_access, ushort devad)
{
    mesa_rc          rc = VTSS_RC_OK;
    misc_msg_buf_t   buf;
    misc_msg_t       *msg;
    uint             addr;
    uint32_t         val;

    addr = ((page<<5) | reg);
    if (misc_stack_local(isid)) {
        /* Local access */
        if (write) {
            if (mmd_access) {
                if (port_is_phy(port_no)) {
                    rc = vtss_phy_mmd_write(PHY_INST, port_no, devad, addr, *value);
                } else if (fast_cap(MESA_CAP_PORT_10G)) {
                    rc = mesa_port_mmd_write(NULL, port_no, devad, reg, *value);
                }
            } else {
                rc = vtss_phy_write(PHY_INST, port_no, addr, *value);
            }
        } else {
            if (mmd_access) {
                if (port_is_phy(port_no)) {
                    rc = vtss_phy_mmd_read(PHY_INST, port_no, devad, addr, value);
                } else if (fast_cap(MESA_CAP_PORT_10G)) {
                    rc = mesa_port_mmd_read(NULL, port_no, devad, reg, value);
                }
            } else {
                rc = vtss_phy_read(PHY_INST, port_no, addr, value);
            }
        }
    } else if (misc_stack_ready(isid, write)) {
        /* Stack access */
        msg = misc_msg_alloc(&buf, write ? MISC_MSG_ID_PHY_WRITE_REQ :
                             MISC_MSG_ID_PHY_READ_REQ);
        msg->data.phy.port_no = port_no;
        msg->data.phy.addr = port_is_phy(port_no) ? addr : reg;
        msg->data.phy.value = *value;
        msg->data.phy.mmd_access = mmd_access;
        msg->data.phy.devad = devad;
        misc_msg_tx(&buf, isid, sizeof(msg->data.phy));
        if (write) {
            T_D("PHY_WRITE_REQ, isid: %d, port_no: %u, addr: 0x%08x, value: 0x%04x",
                isid, port_no, addr, *value);
        } else {
            T_D("PHY_READ_REQ, isid: %d, port_no: %u, addr: 0x%08x", isid, port_no, addr);
            misc_stack_read(&val);
            *value = val;
        }
    }
    return rc;
}

static mesa_rc misc_stack_register(void)
{
    msg_rx_filter_t filter;

    memset(&filter, 0, sizeof(filter));
    filter.cb = misc_msg_rx;
    filter.modid = VTSS_MODULE_ID_MISC;
    return msg_rx_filter_register(&filter);
}

/****************************************************************************/
/*  Debug register and PHY access                                           */
/****************************************************************************/

/* Get chip number */
mesa_rc misc_chip_no_get(mesa_chip_no_t *chip_no)
{
    *chip_no = misc.chip_no;
    return VTSS_RC_OK;
}

/* Set chip number */
mesa_rc misc_chip_no_set(mesa_chip_no_t chip_no)
{
    if (chip_no == VTSS_CHIP_NO_ALL || chip_no < vtss_api_if_chip_count())
        misc.chip_no = chip_no;
    return VTSS_RC_OK;
}

/* Read switch chip register */
mesa_rc misc_debug_reg_read(vtss_isid_t isid, mesa_chip_no_t chip_no, ulong addr, uint32_t *value)
{
    *value = 0xffffffff;
    return misc_stack_reg(isid, chip_no, addr, value, 0);
}

/* Write switch chip register */
mesa_rc misc_debug_reg_write(vtss_isid_t isid, mesa_chip_no_t chip_no, ulong addr, uint32_t value)
{
    return misc_stack_reg(isid, chip_no, addr, &value, 1);
}

/* Read PHY register */
mesa_rc misc_debug_phy_read(vtss_isid_t isid,
                            mesa_port_no_t port_no, uint reg, uint page, ushort *value, BOOL mmd_access, ushort devad)
{
    *value = 0xffff;
    return misc_stack_phy(isid, port_no, reg, page, value, 0, mmd_access, devad);
}

/* Write PHY register */
mesa_rc misc_debug_phy_write(vtss_isid_t isid,
                             mesa_port_no_t port_no, uint reg, uint page, ushort value, BOOL mmd_access, ushort devad)
{
    return misc_stack_phy(isid, port_no, reg, page, &value, 1, mmd_access, devad);
}

/* Suspend/resume */
mesa_rc misc_suspend_resume(vtss_isid_t isid, BOOL resume)
{
    if (misc_stack_local(isid)) {
        vtss_init_data_t data = { .cmd = INIT_CMD_SUSPEND_RESUME };
        /* Local switch */
        data.resume = resume;
        data.isid = isid;
        init_modules(&data);
    } else {
        misc_msg_buf_t   buf;
        misc_msg_t       *msg;
        msg = misc_msg_alloc(&buf, MISC_MSG_ID_SUSPEND_RESUME);
        msg->data.suspend_resume.resume = resume;
        misc_msg_tx(&buf, isid, sizeof(msg->data.suspend_resume));
        if (!resume) /* Wait for suspend */
            VTSS_OS_MSLEEP(1000);
    }
    return VTSS_RC_OK;
}

vtss_inst_t misc_phy_inst_get(void)
{
    return PHY_INST;
}

mesa_rc misc_phy_inst_set(vtss_inst_t inst)
{
    misc.phy_inst = inst;
    return VTSS_RC_OK;
}

void misc_debug_port_state(mesa_port_no_t port_no, BOOL state)
{
    vtss_appl_port_status_t port_status;
    meba_port_admin_state_t admin_state = {};
    vtss_ifindex_t          ifindex;
    mepa_conf_t             phy_conf;
    mesa_rc                 rc;

    if ((rc = vtss_ifindex_from_port(VTSS_ISID_LOCAL, port_no, &ifindex)) != VTSS_RC_OK) {
        T_E("Unable to convert port_no %d to ifindex", port_no);
        return;
    }

    if ((rc = vtss_appl_port_status_get(ifindex, &port_status)) != VTSS_RC_OK) {
        T_E("vtss_appl_port_status_get(%s) failed: %s", ifindex, error_txt(rc));
        return;
    }

    // Figure out whether it's a PHY or SFP port or - in case of dual media -
    // which one is currently active.
    if (!port_status.fiber) {
        if (meba_phy_conf_get(board_instance, port_no, &phy_conf) != VTSS_RC_OK) {
            T_E("vtss_phy_conf_get failed, port_no: %u", port_no);
        }

        if (state == FALSE) {
            u16 reg0, reg4, reg9;
            // Remove all aneg capabilities and then restart aneg.
            // This should mean that a link down interrupt is genarated and the
            // link stays down.
            meba_phy_clause22_read(board_instance,  port_no, 4, &reg4);
            reg4 = reg4 & 0xf01f;
            meba_phy_clause22_write(board_instance,  port_no, 4, reg4);
            meba_phy_clause22_read(board_instance,  port_no, 9, &reg9);
            reg9 = reg9 & 0xfcff;
            meba_phy_clause22_write(board_instance,  port_no, 9, reg9);
            meba_phy_clause22_read(board_instance,  port_no, 0, &reg0);
            reg0 |= (1<<9);
            meba_phy_clause22_write(board_instance,  port_no,  0, reg0);
        } else {
            if (meba_phy_conf_set(board_instance, port_no, &phy_conf) != VTSS_RC_OK) {
                T_E("vtss_phy_conf_set failed, port_no: %u", port_no);
            }
        }
    } else {
        admin_state.enable = state;

        // meba_port_admin_state_set() calls mesa_sgpio_conf_get()/set(), and
        // these two calls must be invoked without interference.
        VTSS_APPL_API_LOCK_SCOPE();
        (void)meba_port_admin_state_set(board_instance, port_no, &admin_state);
    }
}

static char MISC_chip_id_as_txt[20];
static void MISC_chip_id_cache(void)
{
    mesa_target_type_t part_number;
    mesa_chip_id_t     chip_id;
    mesa_rc            rc;

    if ((rc = mesa_chip_id_get(NULL, &chip_id)) != VTSS_RC_OK) {
        T_E("Unable to obtain chip ID (rc = %s)", error_txt(rc));
    }

    part_number = vtss_api_chipid();

    // if partnumber is 7xxx or 47xxx it is a VSC,
    // if partnumber is 9xxx it is a LAN
    MISC_chip_id_as_txt[sizeof(MISC_chip_id_as_txt) - 1] = '\0';
    snprintf(MISC_chip_id_as_txt, sizeof(MISC_chip_id_as_txt) - 1, "%s%04x Rev. %c",
             (part_number >= 0x9000 && part_number < 0x10000) ? "LAN" : "VSC", part_number, chip_id.revision + 'A');
}

static char MISC_board_name_as_txt[sizeof(meba_board_props_t::name) + 1];
static void MISC_board_name_cache(void)
{
    MISC_board_name_as_txt[sizeof(MISC_board_name_as_txt) - 1] = '\0';
    snprintf(MISC_board_name_as_txt, sizeof(MISC_board_name_as_txt) - 1, "%s", vtss_board_name());
}

/****************************************************************************/
/*  String conversions                                                      */
/****************************************************************************/

/* Convert Hex text string to byte array
 *
 * Return -1 when error occurred
 */
int misc_hexstr_to_array(uchar *array, size_t array_max_size, const char *hex_str)
{
    int     i;
    size_t  hex_size = strlen(hex_str) / 2;

    if (strlen(hex_str) % 2 /* not even length */ ||
        hex_size > array_max_size /* the array size is too small */) {
        return -1;
    }

    for (i = 0; i < hex_size && i < array_max_size; ++i) {
        if (sscanf(hex_str + 2 * i, "%2hhx", &array[i]) != 1) {
            return -1;
        }
    }

    return i;
}

/* strip leading path from file */
const char *misc_filename(const char *fn)
{
    if(!fn)
        return NULL;
    int i, start;
    for (start = 0, i = strlen(fn); i > 0; i--) {
        if (fn[i-1] == '/') {
            start = i;
            break;
        }
    }
    return fn+start;
}

/* MAC address text string */
char *misc_mac_txt(const uchar *mac, char *buf)
{
    sprintf(buf, "%02x-%02x-%02x-%02x-%02x-%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}

/* MAC to string */
const char *misc_mac2str(const uchar *mac)
{
    static char buf[6*3];
    return misc_mac_txt(mac, buf);
}

/* Create an instantiated MAC address based on base MAC and instance number */
void misc_instantiate_mac(uchar *mac, const uchar *base, int instance)
{
    int i;
    ulong x;
    for (i = 0; i < 6; i++)
        mac[i] = base[i];
    x = (((mac[3]<<16) | (mac[4]<<8) | mac[5]) + instance);
    mac[3] = ((x>>16) & 0xff);
    mac[4] = ((x>>8) & 0xff);
    mac[5] = (x & 0xff);
}

/* IPv4 address text string */
char *misc_ipv4_txt(mesa_ipv4_t ipv4, char *buf)
{
    sprintf(buf, "%d.%d.%d.%d", (ipv4 >> 24) & 0xff, (ipv4 >> 16) & 0xff, (ipv4 >> 8) & 0xff, ipv4 & 0xff);
    return buf;
}


/* Convert prefix length into IPv4 address's netmask. */
BOOL
misc_prefix2ipv4 (u32 prefix_len, mesa_ipv4_t *const ipv4)
{
    static const u8 maskbit[] = {0x00, 0x80, 0xc0, 0xe0, 0xf0,
                                 0xf8, 0xfc, 0xfe, 0xff};
    u8 *ptr;
    i32 bit;
    u32 offset;

    if (prefix_len > 32) {
        return FALSE;
    }

    memset (ipv4, 0, sizeof (*ipv4));
    ptr = (u8 *) ipv4 + sizeof(mesa_ipv4_t) - 1;

    offset = prefix_len / 8;
    bit = (i32)(prefix_len % 8);

    while (offset--) {
        *(ptr--) = 0xff;
    }

    if (bit) {
        *ptr = maskbit[bit];
    }

    return TRUE;
}

/* IPv6 address text string */
char *misc_ipv6_txt(const mesa_ipv6_t *ipv6, char *buf)
{
    memset(buf, 0, 40 * sizeof(char));
    inet_ntop(AF_INET6, (char*)&ipv6->addr, buf, 40);
    return buf;
}

/* IPv4 or IPv6 to string */
const char *misc_ip_txt(mesa_ip_addr_t *ip_addr, char *buf)
{
    switch (ip_addr->type) {
    case MESA_IP_TYPE_IPV4:
        return misc_ipv4_txt(ip_addr->addr.ipv4, buf);

    case MESA_IP_TYPE_IPV6:
        return misc_ipv6_txt(&ip_addr->addr.ipv6, buf);

    default:
        break;
    }

    return "<none>";
}

/* IPv4/v6 network text string. Give about 64 bytes. */
char *misc_ipaddr_txt(char *buf, size_t bufsize, mesa_ip_addr_t *addr, mesa_prefix_size_t prefix_size)
{
    char temp[64];
    switch (addr->type) {
    case MESA_IP_TYPE_IPV4:
        misc_ipv4_txt(addr->addr.ipv4, temp);
        snprintf(buf, bufsize, "%s/%d", temp, prefix_size);
        break;
    case MESA_IP_TYPE_IPV6:
        misc_ipv6_txt(&addr->addr.ipv6, temp);
        snprintf(buf, bufsize, "%s/%d", temp, prefix_size);
        break;
    default:
        strncpy(buf, "<unknown>", bufsize);
        break;
    }
    return buf;
}

/* IP address text string - network order */
const char *misc_ntoa(ulong ip)
{
    struct in_addr ina;
    ina.s_addr = ip;
    return inet_ntoa(ina);
}

/* IP address text string - host order */
const char *misc_htoa(ulong ip)
{
    return misc_ntoa(htonl(ip));
}

/* Time to Interval (string) */
const char *misc_time2interval(time_t time)
{
    static char buf[16];
    int days, hrs, mins;

#define SECS_DAY  (60 * 60 * 24)
#define SECS_HOUR (60 * 60 * 1)
#define SECS_MIN  (60)

    if (time >= 0) {
        days = time / SECS_DAY;
        time %= SECS_DAY;
        hrs = time / SECS_HOUR;
        time %= SECS_HOUR;
        mins = time / SECS_MIN;
        time %= SECS_MIN;

        snprintf(buf, sizeof(buf),
                 "%dd %02d:%02d:" VPRIFld("02"),
                 days, hrs, mins, time);

        return buf;
    }
    return "Never";
}

/* Time to string
   Whereas RFC3339 makes allowances for multiple syntaxes,
   here lists an example that we used now.

   2003-08-24T05:14:15-07:00

   This represents 24 August 2003 at 05:14:15.  The timestamp indicates that its
   local time is -7 hours from UTC.  This timestamp might be created in
   the US Pacific time zone during daylight savings time.
*/
static const char *MISC_time2str_r(time_t time, char *buf, BOOL print_date, BOOL print_time)
{
    struct tm *timeinfo_p;
    int       tz_off;
    char      *p = buf;
    struct tm timeinfo;

    if (time >= 0 && (print_date || print_time)) {
#if defined(VTSS_SW_OPTION_SYSUTIL)
        tz_off = system_get_tz_off();
        time += (tz_off * 60); /* Adjust for TZ minutes => seconds*/
#else
        tz_off = 0;
#endif /* defined(VTSS_SW_OPTION_SYSUTIL) */

#if defined(VTSS_SW_OPTION_DAYLIGHT_SAVING)
        u32 dst_offset = time_dst_get_offset();
        time += (dst_offset * 60); /* Correct for DST */
        // also add DST offset to timezone offset
        tz_off += dst_offset;
#endif /* defined(VTSS_SW_OPTION_DAYLIGHT_SAVING) */

        timeinfo_p = localtime_r(&time, &timeinfo);
        if (print_date) {
            p += sprintf(p, "%04d-%02d-%02d%s",
                         timeinfo_p->tm_year+1900,
                         timeinfo_p->tm_mon+1,
                         timeinfo_p->tm_mday,
                         print_time ? "T" : "");
        }

        if (print_time) {
            sprintf(p, "%02d:%02d:%02d%c%02d:%02d",
                    timeinfo_p->tm_hour,
                    timeinfo_p->tm_min,
                    timeinfo_p->tm_sec,
                    tz_off >= 0 ? '+' : '-',
                    tz_off >= 0 ? tz_off / 60 : (~tz_off + 1) / 60,
                    tz_off >= 0 ? tz_off % 60 : (~tz_off + 1) % 60);
        }
    } else {
        strcpy(p, "-");
    }

    return buf;
}

const char *misc_time2str_r(time_t time, char *buf)
{
    return MISC_time2str_r(time, buf, TRUE, TRUE);
}

const char *misc_time2date_time_str_r(time_t time, char *buf, BOOL print_date, BOOL print_time)
{
    return MISC_time2str_r(time, buf, print_date, print_time);
}

const char *misc_time2str(time_t time)
{
    static char buf[VTSS_APPL_RFC3339_TIME_STR_LEN];
    return misc_time2str_r(time, buf);
}

const char *misc_time_txt(time_t time_val)
{
    const char *s;

    s = misc_time2interval(time_val);
    while (*s == ' ') {
        s++;
    }
    if (!strncmp(s, "0d ", 3)) {
        s += 3;
    }

    return s;
}

time_t misc_utctime2localtime(time_t time)
{
    time_t local_time = time;

#if defined(VTSS_SW_OPTION_SYSUTIL)
    local_time += (system_get_tz_off() * 60); /* Adjust for TZ minutes => seconds */
#if defined(VTSS_SW_OPTION_DAYLIGHT_SAVING)
    local_time += (time_dst_get_offset() * 60); /* Correct for DST */
#endif /* VTSS_SW_OPTION_DAYLIGHT_SAVING */
#endif /* VTSS_SW_OPTION_SYSUTIL */

    return local_time;
}

/* engine ID to string */
const char *misc_engineid2str(char *engineid_txt, const uchar *engineid, ulong engineid_len)
{
    int i;
    char *p_engineid_txt = engineid_txt;

    if (!engineid_txt || !engineid || engineid_len > 64)
        return "fail";

    for (i = 0; i <engineid_len; i++) {
        p_engineid_txt += sprintf(p_engineid_txt,"%02x",engineid[i]);
    }

    return engineid_txt;
}

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
const char *misc_oid2str(const ulong *oid, ulong oid_len, const uchar *oid_mask, ulong oid_mask_len)
{
    static char buf[128 * 2 + 1];
    size_t buf_max_len = sizeof(buf);
    int i, j;
    int mask = 0x80, maskpos = 0;

    if (!oid || oid_len > 128)
        return "fail";

    memset(buf, 0x0, sizeof(buf));
    for (i = 0, j = 0; i < oid_len; i++) {
        if (oid_mask == NULL || (oid_mask[maskpos] & mask) != 0 || i >= oid_mask_len*8) {
            if (snprintf(buf + j, buf_max_len - j, "." VPRIlu, (ulong)((uint32_t*)oid)[i]) >= (buf_max_len - j)) {
                break;
            }
        } else {
            if (snprintf(buf + j, buf_max_len - j, ".*") >= (buf_max_len - j)) {
                break;
            }
        }
        j = strlen(buf);

        if (mask == 1) {
            mask = 0x80;
            maskpos++;
        } else {
            mask >>= 1;
        }
    }

    return buf;
}

/**
  * \brief Check the specfic OID string is valid and parse the prefix node.
  *
  * \param oid_str [IN]: Pointer to the OID string.
  * \param prefix [OUT]: The result of parsing the prefix node, if prefix node[0] = 0 indicates that
  * the OID string is valid but it is numeric OID, otherwise the OID string has a prefix node.
  *
  * \return: TRUE if the operation success, FALSE indicates the oid_stri is invalid.
  **/
BOOL misc_parse_oid_prefix(const char *oid_str, char *prefix)
{
    size_t      len;
    i32         i = 0, num = 0;
    char const  *value_char = oid_str;
    char        buf[256] = {0}, *cp = buf;
    BOOL        is_prefix_str = FALSE, dot_flag = FALSE;

    if ( !oid_str || (len = strlen(oid_str)) > 255) {
        return FALSE;
    }

    //check if OID format .x.x.x
    if (value_char[0] != '.' || value_char[len - 1] == '.') {
        return FALSE;
    }

    if (isalpha(value_char[1])) {
        is_prefix_str = TRUE;
    }

    for (i = 0; i < len; i++) {
        if (value_char[i] != '.' && value_char[i] != '*' &&
            ((is_prefix_str == TRUE && 0 == isalnum(value_char[i])) ||
            ((is_prefix_str == FALSE && 0 == isdigit(value_char[i]))))) {
            return FALSE;
        }

        if (value_char[i] == '*') {
            if (i == 0 || value_char[i - 1] != '.' || value_char[i + 1] != '.') {
                return FALSE;
            }
        }

        if (value_char[i] == '.') {
            if (dot_flag) { //double dot
                return FALSE;
            }
            dot_flag = TRUE;
            num++;
            if (num > 1) {
                is_prefix_str = FALSE;
            }
            if (num > 128) {
                return FALSE;
            }
        } else {
            if (is_prefix_str) {
               *cp++ = value_char[i];
            }
            dot_flag = FALSE;
        }
    }
    *cp = 0;

    if (buf[0] && prefix) {
        strncpy(prefix, buf, strlen(buf));
        prefix[strlen(buf)] = 0;
    } else if (prefix) {
        prefix[0] = 0;
    }
    return TRUE;
}

/* OID to string for general case. */
const char *misc_oid2txt(const ulong *oid, ulong oid_len)
{
    static char buf[128 * 2 + 1];
    char *ptr = buf, *ptr_end = ptr + sizeof(buf)/sizeof(buf[0]);
    int i;

    buf[0] = '\0';
    if (!oid) {
        return buf;
    }

    for (i = 0; i < oid_len; i++) {
        ptr += snprintf(ptr, ptr_end - ptr, ".%lu", (ulong)((uint32_t*)oid)[i]);
        if (ptr >= ptr_end) {
            break;
        }
    }

    return buf;
}

/* raw to string for general case. */
const char *misc_raw2txt(const u_char *raw, ulong oid_len, BOOL hex_format)
{
    static char buf[128 * 2 + 1];
    char *ptr = buf, *ptr_end = ptr + sizeof(buf)/sizeof(buf[0]);
    int i;

    buf[0] = '\0';
    if (!raw) {
        return buf;
    }

    for (i = 0; i < oid_len; i++) {
        if (hex_format) {
            ptr += snprintf(ptr, ptr_end - ptr, ".%02X", raw[i]);
        } else {
            ptr += snprintf(ptr, ptr_end - ptr, ".%d", raw[i]);
        }
        if (ptr >= ptr_end) {
            break;
        }
    }

    return buf;
}

/* OUI address text string */
char *misc_oui_addr_txt(const uchar *oui, char *buf)
{
    sprintf(buf, "%02x-%02x-%02x", oui[0], oui[1], oui[2]);
    return buf;
}

/* Zero terminated strncpy */
void
misc_strncpyz(char *dst, const char *src, size_t maxlen)
{
    if (maxlen > 0) {
        (void) strncpy(dst, src, maxlen);
        dst[maxlen - 1] = '\0'; /* Explicit null terminated/truncated */
    }
}

/****************************************************************************/
/*  String check                                                            */
/****************************************************************************/

// Checks if a string only contains numbers
//
// In : str - String to check;
//
// Return : VTSS_RC_OK if string only contained numbers else VTSS_INVALID_PARAMETER
//
mesa_rc misc_str_chk_numbers_only(const char *str) {
    // Loop through all characters in str, and check that cmd only contains numbers
    u16 i;
    char char_txt;
    for (i = 0; i < strlen(str); i++) {
        char_txt = *(str+i);
        if (isdigit(char_txt) == 0) {
            return VTSS_INVALID_PARAMETER; // The character was not a number. return error indication.
        }
    }
    return VTSS_RC_OK;
}


// Checks if a string is a valid IPv4 of the format xxx.yyy.zzz.qqq
//
// In : str - String to check;
//
// Return : VTSS_RC_OK if string only a valid IPv4 format else VTSS_INVALID_PARAMETER
//
mesa_rc misc_str_is_ipv4(const char *str) {
    mesa_ipv4_t temp1;
    return mgmt_txt2ipv4(str, &temp1, NULL, 0);
}

#if defined(VTSS_SW_OPTION_IP)
mesa_rc misc_str_is_ipv6(const char *str) {
    mesa_ipv6_t temp1;
    return mgmt_txt2ipv6(str, &temp1);
}
#endif /* defined(VTSS_SW_OPTION_IP) */

/*
 * The original specification of hostnames in RFC 952, mandated that
 * labels could not start with a digit or with a hyphen, and must not
 * end with a hyphen. However, a subsequent specification (RFC 1123)
 * permitted hostname labels to start with digits.
 * We refer to RFC 1123 here.
 *
 * A valid hostname is a string drawn from the alphabet (A-Za-z),
 * digits (0-9), dot (.), hyphen (-). Spaces are not allowed, the first
 * character must be an alphanumeric character, and the first and last
 * characters must not be a dot or a hyphen.
 *
 * Return TRUE: invalid hostname, FALSE: valid hostname */
static BOOL MISC_invalid_hostname(const char *hostname)
{
#if defined(VTSS_SW_OPTION_DNS)
    const char * label;
    const char * label_end = NULL;

    label = hostname;
    if (strstr(label, "..")) {
        return TRUE; // String cannot including '..'
    }
    while (*label) { // Loop through each label
        if (!isalnum(*label)) {
            T_D("First label char ('%c') invalid in %s", *label, hostname);
            return TRUE; // First char must be alphanumeric
        }
        label_end = strchr(label, (int)'.') - 1;
        if (label_end == (char *)-1) {
            label_end = strchr(label, (int)'\0') - 1;
        }
        // label_end now points to the last char in the label (the one before '.' or null)
        while (label != label_end) { // Loop through this label up to, but not including the last char
            if (!isalnum(*label) && (*label != '-')) {
                T_D("Middle label char ('%c') invalid in %s", *label, hostname);
                return TRUE; // Char must be alphanumeric or '-'
            }
            label++;
        }
        if (!isalnum(*label)) {
            T_D("Last label char ('%c') invalid in %s", *label, hostname);
            return TRUE; // Last char must be alphanumeric
        }
        label++; // Move label onto the '.' or the null

        if (*label == '.') {
            label++; // Move label past the '.' to the next label or null
        }
    }
    label_end++; // Move label_end onto the last '.' or the null
    if (*label_end == '.') {
        T_D("Last char ('%c') invalid in %s", *label_end, hostname);
        return TRUE; // Last char must not be '.'
    }

    return FALSE; // It is a valid hostname
#else
    // Only allow host names if we have DNS
    return TRUE;
#endif /* VTSS_SW_OPTION_DNS */
}

// Checks if a string is a valid hostname (if DNS is part of the build) or IPv4
// A valid hostname is defined in RFC 1123 and 952
// A trailing '.' is not allowed
//
// The code is inspired from vtss_valid_hostname(), but with (hopefully) fewer bugs
//
// In : str - String to check;
//
// Return : VTSS_RC_OK if string only a valid host name else VTSS_INVALID_PARAMETER
//
mesa_rc misc_str_is_hostname(const char *hostname)
{
    uint  a, b, c, d;
    char  garbage[2]; // Reserve space for null terminator
    mesa_ipv4_t temp;

    if (!hostname) {
        return VTSS_INVALID_PARAMETER;
    }
    if (*hostname == '\0') {
        return VTSS_INVALID_PARAMETER;
    }

    // Check for dotted-decimal syntax. See RFC1123, section 2.1
    if (sscanf(hostname, "%u.%u.%u.%u%1s", &a, &b, &c, &d, garbage) == 4) {
        return mgmt_txt2ipv4_ext(hostname, &temp, NULL, FALSE, FALSE, FALSE, FALSE);// Check to allow a valid hostname
    }

    if (MISC_invalid_hostname(hostname)) {
        T_D("It is an invalid hostname");
        return VTSS_INVALID_PARAMETER;
    }

    return VTSS_RC_OK;
}

/*
 * A valid name consist of a sequence of domain labels separated by ".",
 * each domain label starting and ending with an alphanumeric character
 * and possibly also containing "-" characters.
 * The length of a domain label must be 63 characters or less.
 *
 * Refer to RFC 1035 2.3.1. Preferred name syntax
 * ---------------------------------------------
 *   <domain> ::= <subdomain>
 *   <subdomain> ::= <label> | <subdomain> "." <label>
 *   <label> ::= <letter> [ [ <ldh-str> ] <let-dig> ]
 *   <ldh-str> ::= <let-dig-hyp> | <let-dig-hyp> <ldh-str>
 *   <let-dig-hyp> ::= <let-dig> | "-"
 *   <let-dig> ::= <letter> | <digit>
 *   <letter> ::= any one of the 52 alphabetic characters A through Z in upper case and a through z in lower case
 *   <digit> ::= any one of the ten digits 0 through 9
 *
 * The labels must follow the rules for ARPANET host names.  They must
 * start with a letter, end with a letter or digit, and have as interior
 * characters only letters, digits, and hyphen.  There are also some
 * restrictions on the length.  Labels must be 63 characters or less.
 *
 * Refer to RFC 1123 2.1  Host Names and Numbers
 * ---------------------------------------------
 * The syntax of a legal Internet host name was specified in RFC-952
 * [DNS:4].  One aspect of host name syntax is hereby changed: the
 * restriction on the first character is relaxed to allow either a
 * letter or a digit.  Host software MUST support this more liberal
 * syntax.
 *
 * Refer to RFC 2396 3.2.2. Server-based Naming Authority
 * ------------------------------------------------------
 * hostname      = *( domainlabel "." ) toplabel [ "." ]
 * domainlabel   = alphanum | alphanum *( alphanum | "-" ) alphanum
 * toplabel      = alpha | alpha *( alphanum | "-" ) alphanum
 *
 * Hostnames take the form described in Section 3 of [RFC1034] and
 * Section 2.1 of [RFC1123]: a sequence of domain labels separated by
 * ".", each domain label starting and ending with an alphanumeric
 * character and possibly also containing "-" characters.
 *
 */
mesa_rc misc_str_is_domainname(const char *domainname)
{
    char        *subdomain, input_str[VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN + 1];
    const char  *delim = ".";

    if (!domainname) {
        T_E("NULL point");
        return VTSS_INVALID_PARAMETER;
    }
    if (*domainname == '\0' || strlen(domainname) > VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN) {
        T_D("Invalid string length");
        return VTSS_INVALID_PARAMETER;
    }
    if (domainname[strlen(domainname) - 1] == '.') {
        T_D("Last character must not dot(.)");
        return VTSS_INVALID_PARAMETER;
    }

    // Loop through each subdomain
    strcpy(input_str, domainname);
    char *saveptr; // Local strtok_r() context
    for (subdomain = strtok_r(input_str, delim, &saveptr); subdomain != NULL; subdomain = (char *)strtok_r(NULL, delim, &saveptr)) {
        T_D("subdomain = %s", subdomain);
        if (strlen(subdomain) < 1 || strlen(subdomain) > 63) {
            T_D("Invalid string length of subdomain: %s", subdomain);
            return VTSS_INVALID_PARAMETER;
        }

        if (!isalnum(domainname[0])) {
            T_D("Invalid first character, must start with a letter or digit");
            return VTSS_INVALID_PARAMETER;
        }

        if (!isalnum(subdomain[strlen(subdomain) - 1])) {
            T_D("Invalid last character, must end with a letter or digit");
            return VTSS_INVALID_PARAMETER;
        }

        if (MISC_invalid_hostname(subdomain)) {
            return VTSS_INVALID_PARAMETER;
        }
    }

    return VTSS_RC_OK;
}

/* Check the string is alphanumeric. (excluding empty string)
 * A valid alphanumeric is a text string drawn from alphabet (A-Za-z),
 *    digits (0-9).
 */
mesa_rc misc_str_is_alphanumeric(const char *str)
{
    const char    *c;
    size_t  len;

    if (str == NULL || !(len = strlen(str))) {
        return VTSS_INVALID_PARAMETER;
    }

    // Check each character
    for (c = str; c < (str + len); ++c) {
        if ((*c >= 'a' && *c <= 'z') ||
            (*c >= 'A' && *c <= 'Z') ||
            (*c >= '0' && *c <= '9')) {
            continue;
        }
        return VTSS_INVALID_PARAMETER;
    }

    return VTSS_RC_OK;
}

/* Check the string is alphanumeric or have special characters
 * exclude empty characters
 */
mesa_rc misc_str_is_alphanumeric_or_spec_char(const char *str)
{
    const char *c;
    size_t     len;

    if (str == NULL || !(len = strlen(str))) {
        return VTSS_INVALID_PARAMETER;
    }

    // Check each character
    for (c = str; c < (str + len); ++c) {
        if ((*c >= 'a' && *c <= 'z') ||
            (*c >= 'A' && *c <= 'Z') ||
            (*c >= '0' && *c <= '9') ||
            (*c >= 33 && *c <= 48) ||
            (*c >= 58 && *c <= 65) ||
            (*c >= 91 && *c <= 97) ||
            (*c >= 123 && *c <= 127)) {
            continue;
        }
        return VTSS_INVALID_PARAMETER;
    }

    return VTSS_RC_OK;
}

/* Check the string is a hex string.
 * A valid hex string is a text string drawn from alphabet (A-Fa-f), digits (0-9).
 * The length must be even since one byte hex value is presented as two octets string.
 */
mesa_rc misc_str_is_hex(const char *str)
{
    const char    *c;
    size_t  len;

    if (str == NULL || !(len = strlen(str)) || (len % 2)) {
        return VTSS_INVALID_PARAMETER;
    }

    // Check each character
    for (c = str; c < (str + len); ++c) {
        if ((*c >= 'a' && *c <= 'f') ||
            (*c >= 'A' && *c <= 'F') ||
            (*c >= '0' && *c <= '9')) {
            continue;
        }
        return VTSS_INVALID_PARAMETER;
    }

    return VTSS_RC_OK;
}

/****************************************************************************/
/*  Chiptype functions                                                      */
/****************************************************************************/
uint32_t misc_chip2family(u32 chip_id)
{
    switch(chip_id) {
    case MESA_TARGET_CARACAL_LITE:
    case MESA_TARGET_SPARX_III_10:
    case MESA_TARGET_SPARX_III_18:
    case MESA_TARGET_SPARX_III_24:
    case MESA_TARGET_SPARX_III_26:
    case MESA_TARGET_CARACAL_1:
    case MESA_TARGET_CARACAL_2:
        return MESA_CHIP_FAMILY_CARACAL;
    case MESA_TARGET_7513:
    case MESA_TARGET_7514:
        return MESA_CHIP_FAMILY_OCELOT;
    case MESA_TARGET_SERVAL_2:
    case MESA_TARGET_SPARX_IV_52:
    case MESA_TARGET_SPARX_IV_44:
    case MESA_TARGET_SPARX_IV_80:
    case MESA_TARGET_SPARX_IV_90:
    case MESA_TARGET_LYNX_2:
    case MESA_TARGET_JAGUAR_2:
        return MESA_CHIP_FAMILY_JAGUAR2;
    case MESA_TARGET_SERVAL_T:
    case MESA_TARGET_SERVAL_TP:
    case MESA_TARGET_SERVAL_TE:
    case MESA_TARGET_SERVAL_TEP:
    case MESA_TARGET_SERVAL_2_LITE:
    case MESA_TARGET_SPARX_IV_34:
    case MESA_TARGET_SERVAL_TE10:
        return MESA_CHIP_FAMILY_SERVALT;
    case MESA_TARGET_7546:
    case MESA_TARGET_7546TSN:
    case MESA_TARGET_7549:
    case MESA_TARGET_7549TSN:
    case MESA_TARGET_7552:
    case MESA_TARGET_7552TSN:
    case MESA_TARGET_7556:
    case MESA_TARGET_7556TSN:
    case MESA_TARGET_7558:
    case MESA_TARGET_7558TSN:
        return MESA_CHIP_FAMILY_SPARX5;
    case MESA_TARGET_LAN9694:
    case MESA_TARGET_LAN9691VAO:
    case MESA_TARGET_LAN9694TSN:
    case MESA_TARGET_LAN9694RED:
    case MESA_TARGET_LAN9696:
    case MESA_TARGET_LAN9692VAO:
    case MESA_TARGET_LAN9696TSN:
    case MESA_TARGET_LAN9696RED:
    case MESA_TARGET_LAN9698:
    case MESA_TARGET_LAN9693VAO:
    case MESA_TARGET_LAN9698TSN:
    case MESA_TARGET_LAN9698RED:
        return MESA_CHIP_FAMILY_LAN969X;
    default:
        return MESA_CHIP_FAMILY_UNKNOWN;
    }
}

const u16 misc_chiptype(void)
{
    return (u16)vtss_api_chipid();
}

const vtss_software_type_t misc_softwaretype(void)
{
    return (vtss_software_type_t) software_type;
}

bool misc_cpu_is_external(void)
{
    static bool is_external;
    static bool is_external_defined = false;
    FILE       *fp;

    if (!is_external_defined) {
        if ((fp = fopen("/sys/firmware/devicetree/base/model", "r"))) {
            char model[128];
            const char *m = fgets(model, sizeof(model), fp);
            fclose(fp);
            if (m && strstr(model, "BeagleBone")) {
                is_external = true;
            } else if (m && strstr(model, "LS1046")) {
                is_external = true;
            } else {
                is_external = false;
            }
        } else {
            is_external = false;
        }
    }
    is_external_defined = true;
    return is_external;
}

// Returns true if we are running on a BeagleBone Black board.
bool misc_is_bbb(void)
{
    static bool is_bbb;
    static bool is_bbb_initialized;
    FILE       *fp;

    if (!is_bbb_initialized) {
        if ((fp = fopen("/sys/firmware/devicetree/base/model", "r"))) {
            char model[128];
            const char *m = fgets(model, sizeof(model), fp);
            fclose(fp);
            if (m && strstr(model, "BeagleBone")) {
                is_bbb = true;
            }
        }

        is_bbb_initialized = true;
    }

    return is_bbb;
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
extern "C" int misc_icli_cmd_register();

mesa_rc misc_init(vtss_init_data_t *data)
{
#ifdef VTSS_SW_OPTION_ALARM
        misc_any_init();
#endif
    switch (data->cmd) {
    case INIT_CMD_INIT:
        // At this point in time, the API is initialized, so we can safely
        // obtain the chip ID and convert it to plain text, making it available
        // by calls to misc_chip_id_txt().
        // Trace errors cause a call to misc_chip_id_txt(), so if that function
        // cached the chip ID upon first invocation and the trace error occurred
        // inside the API, the API's mutex would be attempted taken twice,
        // causing assertion errors.
        // Callers of misc_chip_id_txt() before the chip ID is cached will
        // receive an empty string, which is the best we can do.
        MISC_chip_id_cache();

        // We can also get the board name
        MISC_board_name_cache();

        misc.chip_no = VTSS_CHIP_NO_ALL;
        misc.phy_inst = PHY_INST;

        /* Initialize ISID and message buffer */
        vtss_sem_init(&misc.sem, 1);
        vtss_flag_init(&misc.flags);
        vtss_flag_setbits(&misc.flags, MISC_FLAG_READ_IDLE);
        vtss_flag_maskbits(&misc.flags, ~MISC_FLAG_PORT_FLAP);

        /* ifindex information initial */
        vtss_ifindex_init();

#if !defined(VTSS_BUILD_CONFIG_BRINGUP)
        misc_icli_cmd_register();
#endif
        (void)usb_init();

        T_D("INIT");
        break;
    case INIT_CMD_START:
        T_D("START");
        misc_stack_register();
        break;
    default:
        break;
    }
    return VTSS_RC_OK;
}



/****************************************************************************/
/*  I2C functions                                                */
/****************************************************************************/

static BOOL find_i2c_bus_dev(const char *driver, const char *bus,
                             int *i2c_dev, int *i2c_addr)
{
    DIR *dir;
    struct dirent *dent;
    char fn[PATH_MAX + 400], devname[128];
    FILE *fp;
    BOOL found = false;

    if (!(dir = opendir (bus))) {
        printf("Skip %s\n", bus);
        return false;
    }

    while((dent = readdir(dir)) != NULL) {
        if (dent->d_name[0] != '.') {
            snprintf(fn, sizeof(fn), "%s/%s/name", bus, dent->d_name);
            if ((fp = fopen(fn, "r"))) {
                const char *rrd = fgets(devname, sizeof(devname), fp);
                fclose(fp);
                if (rrd) {
                    size_t slen = strlen(devname);
                    // Chomp
                    if (slen && devname[slen-1] == '\n') {
                        devname[slen-1] = '\0';
                    }
                    if (strcmp(devname, driver) == 0) {
                        T_N("Hit %s for %s", fn, driver);
                        // Have "ddd-xxxx"
                        if(sscanf(dent->d_name, "%u-%x", i2c_dev, i2c_addr) == 2) {
                            T_D("Found device %s at %d:0x%04x", driver, *i2c_dev, *i2c_addr);
                            found = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    closedir(dir);

    return found;
}

BOOL vtss_find_i2c_dev(const char *driver,
                       int *i2c_dev, int *i2c_addr)
{
    const char *top = "/sys/class/i2c-adapter";
    DIR *dir;
    struct dirent *dent;
    char fn[PATH_MAX];
    BOOL found = false;

    if (!(dir = opendir (top))) {
        perror(top);
        return false;
    }

    while((dent = readdir(dir)) != NULL) {
        if (dent->d_name[0] != '.') {
            snprintf(fn, sizeof(fn), "%s/%s", top, dent->d_name);
            if ((found = find_i2c_bus_dev(driver, fn, i2c_dev, i2c_addr))) {
                found = (*i2c_dev > 0 || *i2c_addr > 0);
                break;
            }
        }
    }

    closedir(dir);

    return found;
}

/**
 * Open i2c adapter from user space, return the file descriptor for further i2c read/write.
 * @param[in] adapter_nr zero by default
 * @param[in] i2c_addr i2c slave address
 * @return fileno
 */
int vtss_i2c_adapter_open(int adapter_nr, int i2c_addr)
{
    char filename[20];  /* 20 char should be enough for holding the file name */
    int file;

    snprintf(filename, sizeof(filename), "/dev/i2c-%d", adapter_nr);
    if ((file = open(filename, O_RDWR)) >= 0) {
        T_I("Opened(%s)", filename);
        if (ioctl(file, I2C_SLAVE, i2c_addr) < 0) {
            T_I("cannot specify i2c slave at 0x%02x! [%s]\n", i2c_addr, strerror(errno));
        }
    } else {
        T_I("cannot open /dev/i2c-%d! [%s]\n", adapter_nr, strerror(errno));
    }
    return file;
}

int vtss_i2c_dev_open(const char *driver, int *i2c_bus, int *i2c_addr)
{
    int _i2c_bus, _i2c_addr, f = -1;
    T_I("dev: %s", driver);
    if (vtss_find_i2c_dev(driver, &_i2c_bus, &_i2c_addr)) {
        T_I("bus: %d, addr: %d", _i2c_bus, _i2c_addr);
        f = vtss_i2c_adapter_open(_i2c_bus, _i2c_addr);
        T_I("f: %d", f);
        if (f >= 0) {
            if (i2c_bus)
                *i2c_bus = _i2c_bus;
            if (i2c_addr)
                *i2c_addr = _i2c_addr;
        }
    }
    return f;
}

mesa_rc vtss_i2c_dev_rd(int file,
                        u8 *data,
                        const u8 size)
{
    int rc_cnt = 0;
    mesa_rc rc = VTSS_RC_ERROR;
    if ((rc_cnt = read(file, data, size)) != size) {
        /* Not necessarily error, sfp ports might not be inserted! */
        T_I("read failed! read %d bytes, expect only %d bytes! [%s]\n",  rc_cnt, size, strerror(errno));
    } else {
        rc = VTSS_RC_OK;
    }
    T_D("fd %d, size %d, Read %d byte(s):", file, size, rc_cnt);
    T_D_HEX(data, rc_cnt);
    return rc;
}

mesa_rc vtss_i2c_dev_wr(int file,
                        const u8          *data,
                        const u8          size)
{
    int rc_cnt;
    mesa_rc rc = VTSS_RC_ERROR;

    if ((rc_cnt = write(file, data, size)) != size) {
        T_I("write failed! tried to write %d byte(s) [%s]\n", size, strerror(errno));
    } else {
        rc = VTSS_RC_OK;
    }
    T_D("fd %d, size %d, Wrote %d byte(s):", file, size, rc_cnt);
    T_D_HEX(data, rc_cnt);
    return rc;
}

mesa_rc vtss_i2c_dev_wr_rd(int file,
                           const u8          i2c_addr,
                           u8                *wr_data,
                           const u8          wr_size,
                           u8                *rd_data,
                           const u8          rd_size)
{
    mesa_rc rc = VTSS_RC_ERROR;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];

    // Write portion
    messages[0].addr  = i2c_addr;
    messages[0].flags = 0;
    messages[0].len   = wr_size;
    messages[0].buf   = wr_data;

    // Read portion
    messages[1].addr  = i2c_addr;
    messages[1].flags = I2C_M_RD /* | I2C_M_NOSTART*/;
    messages[1].len   = rd_size;
    messages[1].buf   = rd_data;

    /* Transfer the i2c packets to the kernel and verify it worked */
    packets.msgs  = messages;
    packets.nmsgs = ARRSZ(messages);
    if(ioctl(file, I2C_RDWR, &packets) < 0) {
        T_I("I2C transfer failed!: %s", strerror(errno));
    } else {
        rc = VTSS_RC_OK;
    }
    T_D("I2C(file %d:0x%x): Write %d - Read %d ret %d", file, i2c_addr, wr_size, rd_size, rc);
    return rc;
}

//-----------------------------------------------------------------------------
// URL utilities
//-----------------------------------------------------------------------------

#define MAX_PORT (0xffff)

/*
 * Check if there is any unsafe character in URL username/password string
 *
 * If the following special characters: space !"#$%&'()*+,/:;<=>?@[\]^`{|}~
 * need to be contained in the input url string, they should have percent-encoded.
 *
 * Return TRUE when success, return FALSE otherwise.
 */
static BOOL misc_url_user_pwd_safe(const char *user_or_pwd, size_t len)
{
    const char *c;
    size_t process_idx;

    for (process_idx = 0, c = user_or_pwd; process_idx < len; process_idx++, c++) {
        if ((*c >= '0' && *c <= '9') || (*c >= 'A' && *c <= 'Z') || (*c >= 'a' && *c <= 'z')) {
            continue;
        }

        switch (*c) {
        case '-':
        case '_':
        case '.':
            break;
        case '%':
            // Check if the two next chars are hexadecimal.
            if (process_idx >= (len - 2) ||
                !isxdigit(*(c+1)) ||
                !isxdigit(*(c+2))) {
                // Not match format [%2F]
                return FALSE;
            }
            break;
        default:
            return FALSE;
        }
    }

    return TRUE;
}

static int misc_url_user_pwd_encode(const char *from, char *to)
{
    const char *from_ptr = from;
    char       *to_ptr   = to;
    int        len       = 0;
    size_t     from_len  = strlen(from), i;

    for (i = 0; i < from_len; i++, from_ptr++) {
        if (misc_url_user_pwd_safe(from_ptr, 1)) {
            *to_ptr++ = *from_ptr;
            len++;
        } else {
            sprintf(to_ptr, "%%%02X", *from_ptr);
            to_ptr += 3;
            len    += 3;
        }
    }

    *to_ptr = '\0';

    return len;
}

static int misc_a16toi(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    return toupper(ch) - 'A' + 10;
}

/*
 * If the following special characters: space !"#$%&'()*+,/:;<=>?@[\]^`{|}~
 * need to be contained in the input url string, they should have percent-encoded.
 *
 * The special characters are encoded by a character triplet consisting
 * of the character "%" followed by the two hexadecimal digits (from
 * "0123456789ABCDEF") which forming the hexadecimal value of the octet.
 * For example, '!' is encoded to '%21', '@' is encoded to '%40'.
 *
 * The reserved characters is based on RFC 3986 and here is the mapping list.
 * space  !   "   #   $   %   &   '   (   )   *   +   ,   /   :   ;   <   =   >   ?   @   [   \   ]   ^   `   {   |   }   ~
 * %20    %21 %22 %23 %24 %25 %26 %27 %28 %29 %2A %2B %2C %2F %3A %3B %3C %3D %3E %3F %40 %5B %5C %5D %5E %60 %7B %7C %7D %7E
 *
 * Return TRUE when success, return FALSE otherwise.
 */
static BOOL misc_url_user_pwd_decode(const char *from, char *to, unsigned int from_len, unsigned int to_len)
{
    unsigned int from_i = 0, to_i = 0;
    char ch;

    if (to_len == 0) {
        return FALSE;
    }

    while (from_i < from_len) {
        if (from[from_i] == '%') {
            // Check if there are chars enough left in @from to complete the conversion
            if (from_i + 2 >= from_len) {
                // There aren't.
                return FALSE;
            }

            // Check if the two next chars are hexadecimal.
            if (!isxdigit(from[from_i + 1])  || !isxdigit(from[from_i + 2])) {
                return FALSE;
            }

            ch = 16 * misc_a16toi(from[from_i + 1])  + misc_a16toi(from[from_i + 2]);

            from_i += 3;
        } else {
            ch = from[from_i++];
        }

        to[to_i++] = ch;
        if (to_i == to_len) {
            // Not room for trailing '\0'
            return FALSE;
        }
    }

    to[to_i] = '\0';

    return TRUE;
}

void misc_url_parts_init(misc_url_parts_t *parts, u32 proto_support)
{
    memset(parts, 0, sizeof(*parts));
    parts->proto_support = proto_support;
}

#define _URL_PROTOCOL_SUPPORT_CHECK(protocol_str, protocol_bit) \
    if (strcmp(parts->protocol, protocol_str) == 0) { \
        if ((parts->proto_support & protocol_bit) == 0) { \
            return FALSE; \
        } \
        parts->protocol_id = protocol_bit; \
    }

/* URL(Uniform Resource Locator) is a specific character string that constitutes
 * a reference to a resource. A URL is technically a type of uniform resource
 * identifier (URI), it is specified in RFC 3986.
 *
 * To meet the existing APIs(TFTP/ICLI/libfetch), we implement a simple URL syntax validation as below.
 *
 * URL syntax format:
 * <protocol>://[<username>[:<password>]@]<host>[:<port>][/<path>]
 * file://[/<path>]/<file>
 *
 * <protocol>: The scheme of URI. The input string allows the lowercase letters only and its maximum length is 31.
 * <username>: (Optional) User information. The maximum length is 63.
 * <password>: (Optional) User information. The maximum length is 63.
 * <host>: It can be a domain name or an IPv4 address. The maximum length is 63.
 * <port>: (Optional) port number.
 * <path>: If the path is presented, it must separated by forward slash(/). The maximum length is 255.
 *
 * If the following special characters: space !"#$%&'()*+,/:;<=>?@[\]^`{|}~
 * need to be contained in the input url string, they should have percent-encoded.
 */
BOOL misc_url_decompose(const char *url, misc_url_parts_t *parts)
{
    const char  *p;
    u32         n;
    i32         i;
    i32         len;
    BOOL        b_hostip6 = FALSE;
    char encoded_user[(sizeof(parts->user) - 1) * 3 + 1], encoded_pwd[(sizeof(parts->pwd) - 1) * 3 + 1];

    // Protocol

    p = url;
    n = 0;
    while (*p  &&  *p != ':'  &&  n < sizeof(parts->protocol) - 1) {
        parts->protocol[n++] = *p++;
    }
    if ((*p != ':')  ||  (n == 0)) {
        return FALSE;
    }
    parts->protocol_id = 0;
    parts->protocol[n] = 0;
    p++;

    // check if protocol is supported or not
    _URL_PROTOCOL_SUPPORT_CHECK(MISC_URL_PROTOCOL_NAME_TFTP,  MISC_URL_PROTOCOL_TFTP);
    _URL_PROTOCOL_SUPPORT_CHECK(MISC_URL_PROTOCOL_NAME_FTP,   MISC_URL_PROTOCOL_FTP);
    _URL_PROTOCOL_SUPPORT_CHECK(MISC_URL_PROTOCOL_NAME_HTTP,  MISC_URL_PROTOCOL_HTTP);
    _URL_PROTOCOL_SUPPORT_CHECK(MISC_URL_PROTOCOL_NAME_HTTPS, MISC_URL_PROTOCOL_HTTPS);
    _URL_PROTOCOL_SUPPORT_CHECK(MISC_URL_PROTOCOL_NAME_FILE,  MISC_URL_PROTOCOL_FILE);
    _URL_PROTOCOL_SUPPORT_CHECK(MISC_URL_PROTOCOL_NAME_FLASH, MISC_URL_PROTOCOL_FLASH);
    _URL_PROTOCOL_SUPPORT_CHECK(MISC_URL_PROTOCOL_NAME_SFTP,  MISC_URL_PROTOCOL_SFTP);
    _URL_PROTOCOL_SUPPORT_CHECK(MISC_URL_PROTOCOL_NAME_SCP,   MISC_URL_PROTOCOL_SCP);
    _URL_PROTOCOL_SUPPORT_CHECK(MISC_URL_PROTOCOL_NAME_USB,   MISC_URL_PROTOCOL_USB);

    if (parts->protocol_id == 0) {
        // We did not find any supported protocol
        return FALSE;
    }

    // For flash: protocol, only decompose protocol and path

    if (parts->protocol_id == MISC_URL_PROTOCOL_FLASH || parts->protocol_id == MISC_URL_PROTOCOL_USB) {
        if (icli_file_name_check((char *)p, NULL) != ICLI_RC_OK) {
            return FALSE;
        }

        icli_str_cpy(parts->path, p);
        icli_str_cpy(parts->file, p);
        return TRUE;
    }

    // Not flash:, decompose all

    // Double-slash

    if ((*p != '/')  ||  (*(p + 1) != '/')) {
        return FALSE;
    }
    p += 2;

    // Optional user information
    if (strpbrk(url, "@")) { // Check if the commercial at sign(@) existing
        // User name
        n = 0;
        while (*p  &&  *p != '@' &&  *p != ':' && n < sizeof(encoded_user) - 1) {
            encoded_user[n++] = *p++;
        }
        encoded_user[n] = '\0';
        if ((n == 0)  ||  (n == sizeof(encoded_user) - 1) ||
            !misc_url_user_pwd_safe(encoded_user, n) ||
            (misc_url_user_pwd_decode(encoded_user, parts->user, strlen(encoded_user), sizeof(parts->user)) == FALSE)) {
            return FALSE;
        }

        // Password
        if (*p == ':') {
            p ++; // pass the ":" character
            n = 0;
            while (*p  &&  *p != '@' && n < sizeof(encoded_pwd) - 1) {
                encoded_pwd[n++] = *p++;
            }
            encoded_pwd[n] = '\0';
            if ((n == 0)  ||  (n == sizeof(encoded_pwd) - 1) ||
                !misc_url_user_pwd_safe(encoded_pwd, n) ||
                (misc_url_user_pwd_decode(encoded_pwd, parts->pwd, strlen(encoded_pwd), sizeof(parts->pwd)) == FALSE)) {
                return FALSE;
            }
        }
        p ++; // pass the "@" character
    }

    // Host

    n = 0;
    if (*p == '[') { //rfc3986 URI ,IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
        b_hostip6 = TRUE;
        ++p; //pass '[' character
    }
    while (*p  &&  *p != '/'  &&  ((!b_hostip6 && *p != ':') || b_hostip6)  &&  n < sizeof(parts->host) - 1) {
        if (b_hostip6 && *p == ']') {
            ++p; //pass ']' character
            break;
        }
        parts->host[n++] = *p++;
    }
    parts->host[n] = '\0';

    if (n == sizeof(parts->host) - 1) {
        return FALSE;
    }

    if (n == 0 && strcmp(parts->protocol, "file")) {
        return FALSE;
    }

    // Optional port

    if (n && *p == ':') {
        int port = 0;
        ++p;
        if (!(*p >= '0'  &&  *p <= '9')) {
            return FALSE;
        }
        while (*p >= '0'  &&  *p <= '9') {
            port = 10 * port + (*p - '0');
            if (port > MAX_PORT) {
                return FALSE;
            }
            ++p;
        }
        parts->port = (u16)port;
    }

    // Check for single path-divider slash
    if (*p != '/') {
        return FALSE;
    }

    /*
     * Ignore the single slash '/' separating the address/port part and the path
     * which according to RFC 1738, section 3.1 is NOT part of the path.
     */
    p++;

    // Path
    n = 0;
    while (*p  &&  n < sizeof(parts->path) - 1) {
        parts->path[n++] = *p++;
    }
    parts->path[n] = '\0';
    parts->file[0] = '\0';

    if (*p || n == 0) {
        // there's no path or file name which is OK since it is optional
        return TRUE;
    }

    // File

    if (parts->unchecked_filename) {
        // Bypass the file name checking
        return TRUE;
    }

    len = (i32)n;
    for (i = len - 1; i >= 0 && parts->path[i] != '/'; --i);
    if (i == len - 1) {
        return FALSE;
    }

    for (++i, n = 0; i < len && n < sizeof(parts->file) - 1; ++i, ++n) {
        parts->file[n] = parts->path[i];
    }

    if (icli_file_name_check(parts->file, NULL) != ICLI_RC_OK) {
        return FALSE;
    }

    return TRUE;
}

// Shortest possible URL (but meaningless in our world) is length 7: a://b/c
BOOL misc_url_compose(char *url, int max_len, const misc_url_parts_t *parts)
{
    int n;
    char encoded_user[(sizeof(parts->user) - 1) * 3 + 1], encoded_pwd[(sizeof(parts->pwd) - 1) * 3 + 1];

    if (!url || max_len < 8 || !parts || !(parts->protocol[0]) || !(parts->path[0])) {
        return FALSE;
    }

    if (strcmp(parts->protocol, "flash") == 0) {
        n = snprintf(url, max_len, "%s:%s", parts->protocol, parts->path);
    } else if (strcmp(parts->protocol, "file") == 0) {
        n = snprintf(url, max_len, "%s:///%s", parts->protocol, parts->path);
    } else {
        if (!(parts->host[0])) {
            return FALSE;
        }

        // Encode username/password
        encoded_user[0] = encoded_pwd[0] = '\0';
        if ((strlen(parts->user) && (misc_url_user_pwd_encode(parts->user, encoded_user) == FALSE)) ||
            (strlen(parts->pwd) && (misc_url_user_pwd_encode(parts->pwd, encoded_pwd) == FALSE))) {
            return FALSE;
        }

        /* URL syntax format:
           <protocol>://[<username>[:<password>]@]<host>[:<port>][/<path>] */
        if (parts->port) {
            n = snprintf(url, max_len, "%s://%s%s%s%s%s:%u%s%s",
                         parts->protocol,
                         encoded_user,
                         strlen(encoded_pwd) ? ":" : "",
                         strlen(encoded_pwd) ? encoded_pwd : "",
                         strlen(encoded_user) ? "@" : "",
                         parts->host,
                         parts->port,
                         parts->path[0] != '/' ? "/" : "",
                         parts->path);
        } else {
            n = snprintf(url, max_len, "%s://%s%s%s%s%s%s%s",
                         parts->protocol,
                         encoded_user,
                         strlen(encoded_pwd) ? ":" : "",
                         strlen(encoded_pwd) ? encoded_pwd : "",
                         strlen(encoded_user) ? "@" : "",
                         parts->host,
                         parts->path[0] != '/' ? "/" : "",
                         parts->path);
        }
    }

    return (n > 0)  &&  (n < max_len - 1);   // Less-than because n doesn't count trailing '\0'; -1 because of eCos silliness regarding snprintf return value
}

// See misc_api.h
char *str_tolower(char *str)
{
    int i;
    for (i = 0; i < strlen(str); i++) {
        str[i] = tolower(str[i]);
    }
    return str;
} /* str_tolower */

const char *misc_chip_id_txt(void)
{
    return MISC_chip_id_as_txt;
}

const char *misc_board_name(void)
{
    return MISC_board_name_as_txt;
}

/****************************************************************************/
/* Thread debug functions                                                   */
/****************************************************************************/

#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
static void misc_threadload2buf(bool threadload_started, char *threadload_buf, u16 threadload, int id)
{
    if (threadload_started) {
        mgmt_long2str_float(threadload_buf, threadload, 2);
        strcat(threadload_buf, "%");
    } else {
        strcpy(threadload_buf, "N/A");
    }
}
#endif /* defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR) */

void misc_code_version_print(int (*print_function)(const char *fmt, ...))
{
    const char *code_rev = misc_software_code_revision_txt();
    (void)print_function("Version      : %s\n", misc_software_version_txt());
    (void)print_function("Chip ID      : %s\n", misc_chip_id_txt());
    (void)print_function("Board        : %s\n", misc_board_name());
    (void)print_function("Build Date   : %s\n", misc_software_date_txt());
    if (strlen(code_rev)) {
        (void)print_function("Code Revision: %s\n", code_rev);
    }
}

/******************************************************************************/
// misc_proc_thread_stat_parse()
/******************************************************************************/
mesa_rc misc_proc_thread_stat_parse(int tid, vtss_proc_thread_stat_t *stat)
{
    FILE *input;
    char filename[sizeof("/proc/%d/task/%d/stat") + 24];
    int  cnt;

    sprintf(filename, "/proc/%d/task/%d/stat", getpid(), tid);
    if ((input = fopen(filename, "r")) == NULL) {
        T_E("%% Unable to open %s\n", filename);
        return VTSS_RC_ERROR;
    }

    memset(stat, 0, sizeof(*stat));

    cnt = fscanf(input,
                 "%d %s %c %d %d "        // pid, comm, state, ppid, pgrp
                 "%d %d %d %u %lu "       // session, tty_nr, tpgid, flags, minflt
                 "%lu %lu %lu %lu %lu "   // cminflt, majflt, cmajflt, utime, stime
                 "%ld %ld %ld %ld %ld "   // cutime, cstime, priority, nice, num_threads
                 "%ld %llu %lu %ld %lu "  // itrealvalue, start_time, vsize, rss, rsslim
                 "%lu %lu %lu %lu %lu "   // startcode, endcode, startstack, kstkesp, kstkeip
                 "%lu %lu %lu %lu %lu "   // signal, blocked, sigignore, sigcatch, wchan
                 "%lu %lu %d %d %u "      // nswap, cnswap, exit_signal, processor, rt_priority
                 "%u %llu %lu %ld %lu "   // policy, delayacct_blkio_ticks, guest_time, cguest_time, start_data
                 "%lu %lu %lu %lu %lu "   // end_data, start_brk, arg_start, arg_end, env_start
                 "%lu %d",                // env_end, exit_code
                 &stat->pid,         &stat->comm[0],               &stat->state,       &stat->ppid,        &stat->pgrp,
                 &stat->session,     &stat->tty_nr,                &stat->tpgid,       &stat->flags,       &stat->minflt,
                 &stat->cminflt,     &stat->majflt,                &stat->cmajflt,     &stat->utime,       &stat->stime,
                 &stat->cutime,      &stat->cstime,                &stat->priority,    &stat->nice,        &stat->num_threads,
                 &stat->itrealvalue, &stat->start_time,            &stat->vsize,       &stat->rss,         &stat->rsslim,
                 &stat->startcode,   &stat->endcode,               &stat->startstack,  &stat->kstkesp,     &stat->kstkeip,
                 &stat->signal,      &stat->blocked,               &stat->sigignore,   &stat->sigcatch,    &stat->wchan,
                 &stat->nswap,       &stat->cnswap,                &stat->exit_signal, &stat->processor,   &stat->rt_priority,
                 &stat->policy,      &stat->delayacct_blkio_ticks, &stat->guest_time,  &stat->cguest_time, &stat->start_data,
                 &stat->end_data,    &stat->start_brk,             &stat->arg_start,   &stat->arg_end,     &stat->env_start,
                 &stat->env_end,     &stat->exit_code);

    (void)fclose(input);

    if (cnt != 52) {
        T_E("Expected parsing of 52 fields. Actually parsed %d", cnt);
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// misc_proc_thread_status_parse()
/******************************************************************************/
mesa_rc misc_proc_thread_status_parse(int tid, vtss_proc_thread_status_t *status)
{
    FILE *input;
    char filename[sizeof("/proc/%d/task/%d/stat") + 24], line[100];
    int  cnt = 0;

    sprintf(filename, "/proc/%d/task/%d/status", getpid(), tid);
    if ((input = fopen(filename, "r")) == NULL) {
        T_E("%% Unable to open %s", filename);
        return VTSS_RC_ERROR;
    }

    // Make sure to NULL terminate everything.
    memset(status, 0, sizeof(*status));

    while (fgets(line, sizeof(line), input)) {
        size_t len = 0, max_len = sizeof(status->tlv[cnt].key) - 1;
        char   *dst = status->tlv[cnt].key;
        bool   in_key = true;
        char   *p = line;

        if (cnt >= ARRSZ(status->tlv)) {
            T_E("More lines in %s than we can cope with (%zu)", filename, ARRSZ(status->tlv));
            break;
        }

        while (*p != '\0') {
            if (isspace(*p)) {
                // Skip spaces, tabs, and newlines
            } else if (in_key && *p == ':') {
                in_key = false;
                dst = status->tlv[cnt].value;
                max_len = sizeof(status->tlv[cnt].value) - 1;
            } else if (len >= max_len) {
                T_E("Reserved %zu bytes for %s-portion, which is not enough for line = %s", max_len, in_key ? "key" : "value", line);
                break;
            } else {
                *(dst++) = *p;
                len++;
            }

            p++;
        }

        cnt++;
    }

    (void)fclose(input);
    return VTSS_RC_OK;
}

/******************************************************************************/
// misc_thread_context_switches_get()
/******************************************************************************/
mesa_rc misc_thread_context_switches_get(int tid, u64 *context_switches)
{
    vtss_proc_thread_status_t status;
    int                       i, j;

    struct {
        const char *match;
        bool       seen;
        u64        value;
    } m[2] = {{"voluntary_ctxt_switches", false, 0}, {"nonvoluntary_ctxt_switches", false, 0}};

    if (misc_proc_thread_status_parse(tid, &status) != VTSS_RC_OK) {
        T_E("Unable to get thread status for TID = %d", tid);
        return VTSS_RC_ERROR;
    }

    *context_switches = 0;

    for (i = 0; i < ARRSZ(status.tlv); i++) {
        for (j = 0; j < ARRSZ(m); j++) {
            if (m[j].seen) {
                continue;
            }

            if (strcmp(status.tlv[i].key, m[j].match) == 0) {
                m[j].seen = true;
                if (sscanf(status.tlv[i].value, VPRI64u, &m[j].value) != 1) {
                    T_E("Unable to parse %s as a 64-bit unsigned int", status.tlv[i].value);
                    return VTSS_RC_ERROR;
                }
            }
        }
    }

    for (j = 0; j < ARRSZ(m); j++) {
        if (!m[j].seen) {
            T_E("Didn't find \"%s\" key in thread status proc file for TID = %d", m[j].match, tid);
            return VTSS_RC_ERROR;
        }

        *context_switches += m[j].value;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// misc_proc_stat_parse()
/******************************************************************************/
mesa_rc misc_proc_stat_parse(vtss_proc_stat_t *stat)
{
    FILE       *input;
    const char *filename = "/proc/stat";
    int        cnt;

    if ((input = fopen(filename, "r")) == NULL) {
        T_E("%% Unable to open %s\n", filename);
        return VTSS_RC_ERROR;
    }

    memset(stat, 0, sizeof(*stat));

    cnt = fscanf(input,
                 "cpu  %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                 &stat->user, &stat->nice,    &stat->system, &stat->idle,  &stat->iowait,
                 &stat->irq,  &stat->softirq, &stat->steal,  &stat->guest, &stat->guest_nice);

    (void)fclose(input);

    if (cnt != 10) {
        T_E("Expected parsing of 10 fields. Actually parsed %d", cnt);
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// misc_proc_vmstat_parse()
/******************************************************************************/
mesa_rc misc_proc_vmstat_parse(vtss_proc_vmstat_t *vmstat)
{
    FILE *input;
    const char *filename = "/proc/vmstat";
    int  cnt = 0;
    char line[100], format[32];

    if ((input = fopen(filename, "r")) == NULL) {
        T_E("%% Unable to open %s", filename);
        return VTSS_RC_ERROR;
    }

    memset(vmstat, 0, sizeof(*vmstat));
    snprintf(format, sizeof(format), "%%%zus %%lu\n", sizeof(vmstat->tlv[0].name) - 1);

    while (fgets(line, sizeof(line), input)) {
        size_t name_len;

        if (cnt >= ARRSZ(vmstat->tlv)) {
            T_E("More lines in %s than we can cope with (%zu)", filename, ARRSZ(vmstat->tlv));
            break;
        }

        if (sscanf(line, format, vmstat->tlv[cnt].name, &vmstat->tlv[cnt].value) != 2) {
            T_E("Unable to parse line %s", line);
            continue;
        }

        if ((name_len = strlen(vmstat->tlv[cnt].name)) > vmstat->max_name_wid) {
             vmstat->max_name_wid = name_len;
        }

        cnt++;
    }

    (void)fclose(input);
    return VTSS_RC_OK;
}

/******************************************************************************/
// MISC_thread_details_print()
/******************************************************************************/
static void MISC_thread_details_print(int (*pr)(const char *fmt, ...), vtss_thread_info_t *info)
{
    vtss_proc_thread_stat_t stat;

    if (misc_proc_thread_stat_parse(info->tid, &stat) != VTSS_RC_OK) {
        pr("%% Unable to parse thread info for thread ID %d\n", info->tid);
        return;
    }

#define MY_PR(_fld_, _fmt_) pr("%-22s " _fmt_ "\n", vtss_xstr(_fld_) ":", stat._fld_)
    pr("Process info for thread %d (%s)\n", info->tid, info->name);
    MY_PR(pid,                   "%d");
    MY_PR(comm,                  "%s");
    MY_PR(state,                 "%c");
    MY_PR(ppid,                  "%d");
    MY_PR(pgrp,                  "%d");
    MY_PR(session,               "%d");
    MY_PR(tty_nr,                "%d");
    MY_PR(tpgid,                 "%d");
    MY_PR(flags,                 "0x%08x");
    MY_PR(minflt,                "%lu");
    MY_PR(cminflt,               "%lu");
    MY_PR(majflt,                "%lu");
    MY_PR(cmajflt,               "%lu");
    MY_PR(utime,                 "%lu");
    MY_PR(stime,                 "%lu");
    MY_PR(cutime,                "%ld");
    MY_PR(cstime,                "%ld");
    MY_PR(priority,              "%ld");
    MY_PR(nice,                  "%ld");
    MY_PR(num_threads,           "%ld");
    MY_PR(itrealvalue,           "%ld");
    MY_PR(start_time,            "%llu");
    MY_PR(vsize,                 "%lu");
    MY_PR(rss,                   "%ld");
    MY_PR(rsslim,                "%lu");
    MY_PR(startcode,             "0x%08lx");
    MY_PR(endcode,               "0x%08lx");
    MY_PR(startstack,            "0x%08lx");
    MY_PR(kstkesp,               "0x%08lx");
    MY_PR(kstkeip,               "0x%08lx");
    MY_PR(signal,                "0x%08lx");
    MY_PR(blocked,               "0x%08lx");
    MY_PR(sigignore,             "0x%08lx");
    MY_PR(sigcatch,              "0x%08lx");
    MY_PR(wchan,                 "0x%08lx");
    MY_PR(nswap,                 "%lu");
    MY_PR(cnswap,                "%lu");
    MY_PR(exit_signal,           "%d");
    MY_PR(processor,             "%d");
    MY_PR(rt_priority,           "%u");
    MY_PR(policy,                "%u");
    MY_PR(delayacct_blkio_ticks, "%llu");
    MY_PR(guest_time,            "%lu");
    MY_PR(cguest_time,           "%ld");
    MY_PR(start_data,            "0x%08lx");
    MY_PR(end_data,              "0x%08lx");
    MY_PR(start_brk,             "0x%08lx");
    MY_PR(arg_start,             "0x%08lx");
    MY_PR(arg_end,               "0x%08lx");
    MY_PR(env_start,             "0x%08lx");
    MY_PR(env_end,               "0x%08lx");
    MY_PR(exit_code,             "%d");
    pr("\n");
#undef MY_PR
}

void misc_thread_details_print(int (*pr)(const char *fmt, ...), int tid)
{
    vtss_handle_t      thread_handle;
    vtss_thread_info_t info;

    if (tid <= 0) {
        // Print details for all threads that we know of.
        thread_handle = 0;
        while ((thread_handle = vtss_thread_get_next(thread_handle)) != 0) {
            if (!vtss_thread_info_get(thread_handle, &info)) {
                continue;
            }

            MISC_thread_details_print(pr, &info);
        }
    } else {
        if ((thread_handle = vtss_thread_handle_from_id(tid)) == 0 || !vtss_thread_info_get(thread_handle, &info)) {
            // We don't know anything about this thread, but attempt to print it anyway.
            memset(&info, 0, sizeof(info));
            info.tid = tid;
            info.name = "<unknown thread>";
        }

        MISC_thread_details_print(pr, &info);
    }
}

void misc_thread_status_print(int (*print_function)(const char *fmt, ...), BOOL backtrace, BOOL running_thread_only)
{
    vtss_handle_t           thread_handle      = 0;
    vtss_handle_t           self_thread_handle = vtss_thread_self();
    vtss_thread_info_t      info;
    vtss_proc_thread_stat_t stat;
#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
    vtss::Map<int, thread_load_monitor_load_t> load;
    char                    threadload_1sec_buf[10];
    char                    threadload_10sec_buf[10];
    bool                    threadload_started;
    mesa_rc                 rc;

    if ((rc = thread_load_monitor_load_get(load, threadload_started)) != VTSS_RC_OK) {
        (void)print_function("%% Unable to get thread load. Error = %s\n", error_txt(rc));
    }
#endif /* defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR) */

    if (backtrace) {
        // Nice to also get some version and revision output should this function be invoked due to an exception.
        misc_code_version_print(print_function);
    }

    (void)print_function("Sched policy = %d\n", sched_getscheduler(0 /* current process */));
    (void)print_function("Ticks/sec    = %ld\n", sysconf(_SC_CLK_TCK));

#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
    (void)print_function("ID  Name                             State Prio         OS Val 1sec Load 10sec Load utime      stime\n");
    (void)print_function("--- -------------------------------- ----- ------------ ------ --------- ---------- ---------- ----------\n");
#else
    (void)print_function("ID  Name                             State Prio         OS Val utime      stime\n");
    (void)print_function("--- -------------------------------- ----- ------------ ------ ---------- ----------\n");
#endif /* (!)defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR) */

#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
    {
        // Print the Idle (thread ID -1) and <other> loads (thread ID 0).
        auto itr_idle = load.find(-1);
        misc_threadload2buf(threadload_started, threadload_1sec_buf,  itr_idle == load.end() ? 0 : itr_idle->second.one_sec_load, 0);
        misc_threadload2buf(threadload_started, threadload_10sec_buf, itr_idle == load.end() ? 0 : itr_idle->second.ten_sec_load, 0);
        (void)print_function("  - <Idle task>                      -     -                 - %9s %10s          -          -\n", threadload_1sec_buf, threadload_10sec_buf);
        auto itr_other = load.find(0);
        misc_threadload2buf(threadload_started, threadload_1sec_buf,  itr_other == load.end() ? 0 : itr_other->second.one_sec_load, 0);
        misc_threadload2buf(threadload_started, threadload_10sec_buf, itr_other == load.end() ? 0 : itr_other->second.ten_sec_load, 0);
        (void)print_function("  - <Other processes/interrupts>     -     -                 - %9s %10s          -          -\n", threadload_1sec_buf, threadload_10sec_buf);
    }
#endif /* defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR) */

    while ((thread_handle = vtss_thread_get_next(thread_handle)) != 0) {
        BOOL self_thread = thread_handle == self_thread_handle;

        if (running_thread_only && !self_thread) {
            continue;
        }

        (void)vtss_thread_info_get(thread_handle, &info);
        if (misc_proc_thread_stat_parse(info.tid, &stat) != VTSS_RC_OK) {
            (void)print_function("Error: Unable to read state from /proc/ for TID = %d (%s)\n", info.tid, info.name);
            continue;
        }

#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
        {
            auto itr = load.find(info.tid);
            misc_threadload2buf(threadload_started, threadload_1sec_buf,  itr == load.end() ? 0 : itr->second.one_sec_load, info.tid);
            misc_threadload2buf(threadload_started, threadload_10sec_buf, itr == load.end() ? 0 : itr->second.ten_sec_load, info.tid);
        }
#endif /* defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR) */

        (void)print_function("%3d %-32s %-5c %-12s %6d"
#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
                             " %9s %10s"
#endif /* defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR) */
                             " %10lu %10lu%s\n",
                             info.tid,
                             info.name,
                             stat.state,
                             vtss_thread_prio_to_txt(info.prio),
                             info.os_prio,
#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
                             threadload_1sec_buf,
                             threadload_10sec_buf,
#endif /* defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR) */
                             stat.utime,
                             stat.stime,
                             self_thread ? "*" : "");

        if (backtrace) {
            vtss_backtrace(print_function, info.tid);
        }
    }
}

// Unfortunately, we have to redefine this struct in case thread load monitor
// is not included in the build.
typedef struct {
    u64 one_sec;
    u64 ten_sec;
    u64 total;
} misc_context_switches_t;

static void MISC_thread_context_switches_print(int (*print_function)(const char *fmt, ...), vtss_thread_info_t *info, misc_context_switches_t *sample)
{
    char tid_buf[10];
    char name_buf[50];

    if (info) {
        snprintf(tid_buf,  sizeof(tid_buf)  - 1, "%3d",   info->tid);
        snprintf(name_buf, sizeof(name_buf) - 1, "%-32s", info->name);
    } else {
        sprintf(tid_buf,  "%3s",   "");
        sprintf(name_buf, "%-32s", "Total");
    }

    tid_buf [sizeof(tid_buf)  - 1] = '\0';
    name_buf[sizeof(name_buf) - 1] = '\0';
    (void)print_function("%s %s ",
                         tid_buf,
                         name_buf);

    if (sample) {
#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
        char one_sec_buf[21], ten_sec_buf[21];
        if (sample->one_sec == -1) {
            sprintf(one_sec_buf, "%10s", "N/A");
        } else {
            sprintf(one_sec_buf, VPRI64Fu("10"), sample->one_sec);
        }

        if (sample->ten_sec == -1) {
            sprintf(ten_sec_buf, "%10s", "N/A");
        } else {
            sprintf(ten_sec_buf, VPRI64Fu("10"), sample->ten_sec);
        }


        (void)print_function("%s %s " VPRI64Fu("10"),
                             one_sec_buf,
                             ten_sec_buf,
                             sample->total);
#else
        (void)print_function(VPRI64Fu("10"), sample->total);
#endif
    }

    (void)print_function("\n");
}

void misc_thread_context_switches_print(int (*print_function)(const char *fmt, ...))
{
#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
    vtss::Map<int, thread_load_monitor_context_switches_t> context_switches;
    vtss::Map<int, thread_load_monitor_context_switches_t>::iterator itr;
    mesa_rc                 rc;
#endif
    misc_context_switches_t sample;
    vtss_handle_t           thread_handle = 0;
    bool                    threadload_started;
    u64                     total = 0;

#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
    if ((rc = thread_load_monitor_context_switches_get(context_switches, threadload_started)) != VTSS_RC_OK) {
        (void)print_function("%% Unable to get threads' number of context switches. Error = %s\n", error_txt(rc));
    }
#else
    threadload_started = false;
#endif

    sample.one_sec = (u64)-1;
    sample.ten_sec = (u64)-1;

#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
    (void)print_function("ID  Name                             Last 1sec  Last 10sec Total\n");
    (void)print_function("--- -------------------------------- ---------- ---------- ----------\n");
#else
    (void)print_function("ID  Name                             Total\n");
    (void)print_function("--- -------------------------------- ----------\n");
#endif

    while ((thread_handle = vtss_thread_get_next(thread_handle)) != 0) {
        vtss_thread_info_t info;

        (void)vtss_thread_info_get(thread_handle, &info);

        if (threadload_started) {
#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
            itr = context_switches.find(info.tid);
            if (itr != context_switches.end()) {
                sample.one_sec = itr->second.one_sec;
                sample.ten_sec = itr->second.ten_sec;
                sample.total   = itr->second.total;
            }

            MISC_thread_context_switches_print(print_function, &info, itr == context_switches.end() ? nullptr : &sample);
#endif
        } else {
            // Gotta get them ourselves. We cannot print last 1sec and 10sec
            // context switches, but that's OK as long as we get the total.
            if (misc_thread_context_switches_get(info.tid, &sample.total) != VTSS_RC_OK) {
                // Error already printed.
                continue;
            }

            MISC_thread_context_switches_print(print_function, &info, &sample);
            total += sample.total;
        }
    }

    // Also print the total
    if (threadload_started) {
#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
        itr = context_switches.find(0);
        if (itr != context_switches.end()) {
            sample.one_sec = itr->second.one_sec;
            sample.ten_sec = itr->second.ten_sec;
            sample.total   = itr->second.total;
        }

        MISC_thread_context_switches_print(print_function, NULL, itr == context_switches.end() ? nullptr : &sample);
#endif
    } else {
        sample.total = total;
        MISC_thread_context_switches_print(print_function, NULL, &sample);
    }
}

#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
// Stepping-stone functions to allow compiling
// misc.icli with C instead of C++ compiler.
mesa_rc misc_thread_load_monitor_start(void)
{
    return thread_load_monitor_start();
}
#endif /* defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR) */

#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
mesa_rc misc_thread_load_monitor_stop(void)
{
    return thread_load_monitor_stop();
}
#endif /* defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR) */

mesa_rc misc_find_spidev(char *spi_file, size_t max_size, const char *id)
{
    return meba_synce_spi_if_find_spidev(board_instance, id, spi_file, max_size);
}

/******************************************************************************/
// misc_mem_print()
/******************************************************************************/
char *misc_mem_print(const uint8_t *in_buf, size_t in_sz, char *out_buf, size_t out_sz, bool print_address, bool print_printable)
{
    int  i = 0, j, out_cnt = 0;

#define P(_fmt_, ...) out_cnt += snprintf(out_buf + out_cnt, MAX(out_sz - out_cnt, 0), _fmt_, ##__VA_ARGS__)

    while (i < in_sz) {
        if (print_address) {
            P("%p/", in_buf + i);
        }

        P("%04x:", i);

        j = 0;
        while (j + i < in_sz && j < 16) {
            P("%c%02x", j == 8 ? '-' : ' ', in_buf[i + j]);
            j++;
        }

        if (print_printable) {
            while (j++ < 16) {
                P("   ");
            }

            j = 0;
            P(" ");
            while (j + i < in_sz && j < 16) {
                P("%c", isprint(in_buf[i + j]) ? in_buf[i + j] : '.');
                j++;
            }
        }

        P("\n");
        i += 16;
    }

#undef P

    return out_buf;
}

