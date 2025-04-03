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

#include "ddmi_api.h"                          // For ourselves
#include "ddmi_expose.hxx"                     // For ddmi_notification_status_t
#include "port_api.h"                          // For port_sfp_transceiver_to_txt()
#include <vtss/appl/ddmi.h>                    // For ourselves
#include <vtss/appl/port.h>                    // For vtss_appl_port_status_t
#include <vtss/basics/expose/table-status.hxx> // For vtss::expose::TableStatus
#include <vtss/basics/memcmp-operator.hxx>     // For VTSS_BASICS_MEMCMP_OPERATOR

#if defined(VTSS_SW_OPTION_SYSLOG)
#include "syslog_api.h"
#endif

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_DDMI
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_DDMI

// Allow type++ on the following type
VTSS_ENUM_INC(vtss_appl_ddmi_monitor_type_t);

typedef struct {
    vtss_ifindex_t               ifindex;
    mesa_port_no_t               port_no;
    bool                         detected_done;
    uint8_t                      a2_type;
    vtss_appl_ddmi_port_status_t status;
} ddmi_port_state_t;

CapArray<ddmi_port_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> DDMI_port_state;

typedef struct {
    uint32_t int_part; // Negative if 'negative' is true
    uint32_t frac_part;

    // Reason for having this boolean is to be able to present negative
    // numbers where the int_part is 0.
    bool negative;
} ddmi_fraction_t;

#define ROUNDING_DIVISION(x, y) (((x) + ((y) / 2)) / (y))

static vtss_appl_ddmi_global_conf_t DDMI_global_conf;
static uint32_t                     DDMI_port_cnt;
static critd_t                      DDMI_crit;
static vtss_flag_t                  DDMI_status_flags;

static vtss_trace_reg_t DDMI_trace_reg = {
    VTSS_TRACE_MODULE_ID,
    "ddmi",
    "DDMI"
};

static vtss_trace_grp_t DDMI_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&DDMI_trace_reg, DDMI_trace_grps);

struct DdmiLockScope {
    DdmiLockScope(const char *file, int line)
        : file(file), line(line)
    {
        critd_enter(&DDMI_crit, file, line);
    }

    ~DdmiLockScope(void)
    {
        critd_exit(&DDMI_crit, file, line);
    }

private:
    const char *file;
    const int  line;
};

#define DDMI_LOCK_SCOPE() DdmiLockScope __ddmi_lock_guard__(__FILE__, __LINE__)

/* Thread variables */
static vtss_handle_t DDMI_thread_handle;
static vtss_thread_t DDMI_thread_block;

// ddmi_notification_status holds the per-interface, per DDMI monitor type
// status that we can get notifications on, that being SNMP traps or JSON
// notifications.
ddmi_notification_status_t ddmi_notification_status("ddmi_notification_status", VTSS_MODULE_ID_DDMI);

// Use memcmp() for values of ddmi_notification_status.
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_ddmi_monitor_status_t);

/******************************************************************************/
// vtss_appl_ddmi_notification_status_key_t::operator<
// Used for sorting the ddmi_notification_status map.
/******************************************************************************/
bool operator<(const vtss_appl_ddmi_notification_status_key_t &lhs, const vtss_appl_ddmi_notification_status_key_t &rhs)
{
    // First sort by ifindex
    if (lhs.ifindex != rhs.ifindex) {
        return lhs.ifindex < rhs.ifindex;
    }

    // Then by type
    return lhs.type < rhs.type;
}

/******************************************************************************/
// ddmi_fraction_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const ddmi_fraction_t &frac)
{
    o << "{int_part = "   << frac.int_part
      << ", frac_part = " << frac.frac_part
      << ", negative = "  << frac.negative
      << "}";

    return o;
}

/******************************************************************************/
// ddmi_fraction_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ddmi_fraction_t *frac)
{
    o << *frac;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// DDMI_ifindex_to_port()
/******************************************************************************/
static mesa_rc DDMI_ifindex_to_port(vtss_ifindex_t ifindex, mesa_port_no_t &port_no)
{
    vtss_ifindex_elm_t ife;

    // Check that we can decompose the ifindex and that it's a port.
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        port_no = VTSS_PORT_NO_NONE;
        return DDMI_ERROR_IFINDEX_NOT_PORT;
    }

    if (ife.ordinal >= DDMI_port_cnt) {
        return DDMI_ERROR_INVALID_PORT;
    }

    port_no = ife.ordinal;

    return VTSS_RC_OK;
}

#define DDMI_rxpwr2frac_internal DDMI_voltage2frac_internal
#define DDMI_txpwr2str           DDMI_voltage2str

/******************************************************************************/
// DDMI_ieee754_get()
/******************************************************************************/
static int DDMI_ieee754_get(uint8_t *input, int multiplier, uint32_t *result)
{
    int8_t   exponent;
    uint32_t mantissa = 0;
    int      i = 0;

    exponent = (input[0] << 1 | input[1] >> 7);
    mantissa = (input[1] & 0x7f) << 1 | input[2] >> 7;
    if (exponent == 0 && mantissa == 0) {
        *result = 0;
        return 0;
    } else if (exponent == 0) {
        /* un-normalize */
        return -1;
    } else if ((uint8_t)exponent == 255 && mantissa == 0) {
        /* infinite */
        return -2;
    } else if ((uint8_t)exponent == 255) {
        /* NaN */
        return -3;
    }

    exponent -= 127;

    *result = 1 * multiplier + mantissa * multiplier / 256;

    if (exponent >= 0) {
        for (i = 0; i < exponent; i++) {
            *result *= 2;
        }
    } else {
        for (i = 0; i > exponent; i--) {
            *result /= 2;
        }
    }

    return 0;
}

/******************************************************************************/
// DDMI_signed_frac_compute()
/******************************************************************************/
static void DDMI_signed_frac_compute(int32_t result, int32_t multiplier, int32_t units, ddmi_fraction_t &frac)
{
    frac.negative  = result < 0;
    frac.int_part  = abs(result / units);
    frac.frac_part = ROUNDING_DIVISION(multiplier * (abs(result) % units), units);
}

/******************************************************************************/
// DDMI_unsigned_frac_compute()
/******************************************************************************/
static void DDMI_unsigned_frac_compute(uint32_t result, uint32_t multiplier, uint32_t units, ddmi_fraction_t &frac)
{
    frac.negative  = false;
    frac.int_part  = result / units;
    frac.frac_part = ROUNDING_DIVISION(multiplier * (result % units), units);
}

/******************************************************************************/
// DDMI_temperature2frac_internal()
/******************************************************************************/
static void DDMI_temperature2frac_internal(uint8_t *input, ddmi_fraction_t &frac)
{
    int32_t result;

    // Computing in units of 1/256 Celsius
    result = (int8_t)input[0] * 256 + input[1];

    DDMI_signed_frac_compute(result, 1000, 256, frac);
}

/******************************************************************************/
// DDMI_temperature2frac_external()
/******************************************************************************/
static void DDMI_temperature2frac_external(uint8_t *input, uint8_t *slope, uint8_t *offset, ddmi_fraction_t &frac)
{
    int32_t  tmp_1, tmp_3, result;
    uint32_t tmp_2;

    // Computing in units of 1/256 Celsius.
    tmp_1  = (int8_t)input[0] * 256 + input[1];
    tmp_2  = slope[0] * 256 + slope[1];
    tmp_3  = (int8_t)offset[0] * 256 + offset[1];
    result = (tmp_1 * (int32_t)tmp_2) / 256 + tmp_3;

    DDMI_signed_frac_compute(result, 1000, 256, frac);
}

/******************************************************************************/
// DDMI_temperature2str()
/******************************************************************************/
static void DDMI_temperature2str(uint8_t a2_type, uint8_t *input, uint8_t *slope, uint8_t *offset, char *str, size_t str_size, ddmi_fraction_t &frac)
{
    if (a2_type & 0x20) {
        // Internally calibrated. The values can be used directly.
        DDMI_temperature2frac_internal(input, frac);
    } else  {
        // Externally calibrated, meaning that we need rom[56-95] to figure out
        // the real world units, since what we got in the remaining fields are
        // A/D values.
        DDMI_temperature2frac_external(input, slope, offset, frac);
    }

    str[str_size - 1] = '\0';
    (void)snprintf(str, str_size - 1, "%s%u.%03u", frac.negative ? "-" : "", frac.int_part, frac.frac_part);

    T_N("A2-type = 0x%x (%s), input = 0x%02x%02x, slope = 0x%02x%02x, offset = 0x%02x%02x => frac = %s => %s", a2_type, a2_type & 0x20 ? "Internal" : "External", input[0], input[1], slope[0], slope[1], offset[0], offset[1], frac, str);
}

/******************************************************************************/
// DDMI_voltage2frac_internal()
/******************************************************************************/
static void DDMI_voltage2frac_internal(uint8_t *input, ddmi_fraction_t &frac)
{
    uint32_t result;

    // Compute in units of 100 uV (0.1 mV).
    result = input[0] * 256 + input[1];

    DDMI_unsigned_frac_compute(result, 10000, 10000, frac);
}

/******************************************************************************/
// DDMI_voltage2frac_external()
/******************************************************************************/
static void DDMI_voltage2frac_external(uint8_t *input, uint8_t *slope, uint8_t *offset, ddmi_fraction_t &frac)
{
    int32_t  tmp_3, result;
    uint32_t tmp_1, tmp_2;

    tmp_1  = input[0] * 256 + input[1];
    tmp_2  = slope[0] * 256 + slope[1];
    tmp_3  = (int8_t)offset[0] * 256 + offset[1];
    result = (tmp_1 * tmp_2) / 256 + tmp_3;

    // Because tmp_3 can be negative, this may give a negative
    DDMI_signed_frac_compute(result, 10000, 10000, frac);
}

/******************************************************************************/
// DDMI_voltage2str()
/******************************************************************************/
static void DDMI_voltage2str(uint8_t a2_type, uint8_t *input, uint8_t *slope, uint8_t *offset, char *str, size_t str_size, ddmi_fraction_t &frac)
{
    if (a2_type & 0x20) {
        // Internally calibrated. The values can be used directly.
        DDMI_voltage2frac_internal(input, frac);
    } else  {
        // Externally calibrated, meaning that we need rom[56-95] to figure out
        // the real world units, since what we got in the remaining fields are
        // A/D values.
        DDMI_voltage2frac_external(input, slope, offset, frac);
    }

    str[str_size - 1] = '\0';
    (void)snprintf(str, str_size - 1, "%s%u.%04u", frac.negative ? "-" : "", frac.int_part, frac.frac_part);

    T_N("A2-type = 0x%x (%s), input = 0x%02x%02x, slope = 0x%02x%02x, offset = 0x%02x%02x => frac = %s => %s", a2_type, a2_type & 0x20 ? "Internal" : "External", input[0], input[1], slope[0], slope[1], offset[0], offset[1], frac, str);
}

/******************************************************************************/
// DDMI_txbias2frac_internal()
/******************************************************************************/
static void DDMI_txbias2frac_internal(uint8_t *input, ddmi_fraction_t &frac)
{
    uint32_t result;

    // Compute in units of 2 uA (0.002 mA);
    result = input[0] * 256 + input[1];

    DDMI_unsigned_frac_compute(result, 1000, 500, frac);
}

/******************************************************************************/
// DDMI_txbias2frac_external()
/******************************************************************************/
static void DDMI_txbias2frac_external(uint8_t *input, uint8_t *slope, uint8_t *offset, ddmi_fraction_t &frac)
{
    int32_t  tmp_3, result;
    uint32_t tmp_1, tmp_2;

    tmp_1  = input[0] * 256 + input[1];
    tmp_2  = slope[0] * 256 + slope[1];
    tmp_3  = (int8_t)offset[0] * 256 + offset[1];
    result = (tmp_1 * tmp_2) / 256 + tmp_3;

    // Because tmp_3 can be negative, this may give a negative.
    DDMI_signed_frac_compute(result, 1000, 500, frac);
}

/******************************************************************************/
// DDMI_txbias2str()
/******************************************************************************/
static void DDMI_txbias2str(uint8_t a2_type, uint8_t *input, uint8_t *slope, uint8_t *offset, char *str, size_t str_size, ddmi_fraction_t &frac)
{
    if (a2_type & 0x20) {
        // Internally calibrated. The values can be used directly.
        DDMI_txbias2frac_internal(input, frac);
    } else  {
        // Externally calibrated, meaning that we need rom[56-95] to figure out
        // the real world units, since what we got in the remaining fields are
        // A/D values.
        DDMI_txbias2frac_external(input, slope, offset, frac);
    }

    str[str_size - 1] = '\0';
    (void)snprintf(str, str_size - 1, "%s%u.%03u", frac.negative ? "-" : "", frac.int_part, frac.frac_part);

    T_N("A2-type = 0x%x (%s), input = 0x%02x%02x, slope = 0x%02x%02x, offset = 0x%02x%02x => frac = %s => %s", a2_type, a2_type & 0x20 ? "Internal" : "External", input[0], input[1], slope[0], slope[1], offset[0], offset[1], frac, str);
}

/******************************************************************************/
// DDMI_rxpwr2frac_external()
/******************************************************************************/
static void DDMI_rxpwr2frac_external(uint8_t *input, uint8_t *pwr, ddmi_fraction_t &frac)
{
    uint32_t tmp, pwr_4, pwr_3, pwr_2, pwr_1, pwr_0, result;

    tmp = (input[0] << 8) | input[1];

    (void)DDMI_ieee754_get(pwr +  0, 10000, &pwr_4);
    (void)DDMI_ieee754_get(pwr +  4, 10000, &pwr_3);
    (void)DDMI_ieee754_get(pwr +  8, 10000, &pwr_2);
    (void)DDMI_ieee754_get(pwr + 12, 10000, &pwr_1);
    (void)DDMI_ieee754_get(pwr + 16, 10000, &pwr_0);

    result = pwr_0                   +
             pwr_1 * tmp             +
             pwr_2 * tmp * tmp       +
             pwr_3 * tmp * tmp * tmp +
             pwr_4 * tmp * tmp * tmp * tmp;

    result = result / 10000;

    DDMI_unsigned_frac_compute(result, 10000, 10000, frac);
}

/******************************************************************************/
// DDMI_rxpwr2str()
/******************************************************************************/
static void DDMI_rxpwr2str(uint8_t a2_type, uint8_t *input, uint8_t *pwr, char *str, size_t str_size, ddmi_fraction_t &frac)
{
    if (a2_type & 0x20) {
        // Internally calibrated. The values can be used directly.
        DDMI_rxpwr2frac_internal(input, frac);
    } else  {
        // Externally calibrated, meaning that we need rom[56-95] to figure out
        // the real world units, since what we got in the remaining fields are
        // A/D values.
        DDMI_rxpwr2frac_external(input, pwr, frac);
    }

    str[str_size - 1] = '\0';
    (void)snprintf(str, str_size - 1, "%u.%04u", frac.int_part, frac.frac_part);

    T_N_HEX(pwr, 20);
    T_N("A2-type = 0x%x (%s), input = 0x%02x%02x => frac = %s => %s", a2_type, a2_type & 0x20 ? "Internal" : "External", input[0], input[1], frac, str);
}

/******************************************************************************/
// ddmi_fraction_t::operator<
/******************************************************************************/
static bool operator<(ddmi_fraction_t &lhs, ddmi_fraction_t &rhs)
{
    if (lhs.negative == rhs.negative) {
        // Both are either positive or negative, but the int_part is always
        // unsigned.
        if (lhs.int_part < rhs.int_part) {
            return !lhs.negative;
        }

        if (lhs.int_part > rhs.int_part) {
            return lhs.negative;
        }

        // lhs.int_part == rhs.int_part
        if (lhs.frac_part < rhs.frac_part) {
            return !lhs.negative;
        }

        if (lhs.frac_part > rhs.frac_part) {
            return lhs.negative;
        }

        return false;
    }

    return lhs.negative;
}

/******************************************************************************/
// ddmi_fraction_t::operator>
/******************************************************************************/
static bool operator>(ddmi_fraction_t &lhs, ddmi_fraction_t &rhs)
{
    return rhs < lhs;
}

/******************************************************************************/
// DDMI_state_get()
/******************************************************************************/
static vtss_appl_ddmi_monitor_state_t DDMI_state_get(ddmi_fraction_t val[])
{
    ddmi_fraction_t &current_val  = val[VTSS_APPL_DDMI_MONITOR_STATE_NONE];
    ddmi_fraction_t &lo_warn_val  = val[VTSS_APPL_DDMI_MONITOR_STATE_WARN_LO];
    ddmi_fraction_t &hi_warn_val  = val[VTSS_APPL_DDMI_MONITOR_STATE_WARN_HI];
    ddmi_fraction_t &lo_alarm_val = val[VTSS_APPL_DDMI_MONITOR_STATE_ALARM_LO];
    ddmi_fraction_t &hi_alarm_val = val[VTSS_APPL_DDMI_MONITOR_STATE_ALARM_HI];

    if (current_val < lo_alarm_val) {
        return VTSS_APPL_DDMI_MONITOR_STATE_ALARM_LO;
    }

    if (current_val > hi_alarm_val) {
        return VTSS_APPL_DDMI_MONITOR_STATE_ALARM_HI;
    }

    if (current_val < lo_warn_val) {
        return VTSS_APPL_DDMI_MONITOR_STATE_WARN_LO;
    }

    if (current_val > hi_warn_val) {
        return VTSS_APPL_DDMI_MONITOR_STATE_WARN_HI;
    }

    return VTSS_APPL_DDMI_MONITOR_STATE_NONE;
}

/******************************************************************************/
// DDMI_notification_status_update()
/******************************************************************************/
static void DDMI_notification_status_update(vtss_ifindex_t ifindex, vtss_appl_ddmi_monitor_type_t type, vtss_appl_ddmi_monitor_status_t *monitor_status)
{
    vtss_appl_ddmi_notification_status_key_t key;

    key.ifindex = ifindex;
    key.type    = type;

    if (monitor_status) {
        ddmi_notification_status.set(&key, monitor_status);
    } else {
        ddmi_notification_status.del(&key);
    }
}

/******************************************************************************/
// DDMI_read_a0()
/******************************************************************************/
static bool DDMI_read_a0(ddmi_port_state_t *port_state)
{
    vtss_appl_ddmi_port_status_t &status = port_state->status;
    vtss_appl_port_status_t      pstatus;
    mesa_rc                      rc;

    vtss_clear(status.vendor);
    vtss_clear(status.part_number);
    vtss_clear(status.serial_number);
    vtss_clear(status.revision);
    vtss_clear(status.date_code);
    status.sfp_type = MEBA_SFP_TRANSRECEIVER_NONE;
    port_state->a2_type = 0;

    // We only need one extra byte from the SFP, namely the one indicating
    // whether the SFP supports A2.
    // The remaining are taken from the port module's SFP data.
    if (meba_sfp_i2c_xfer(board_instance, port_state->port_no, false, 0x50, 92, &port_state->a2_type, 1, false) != VTSS_RC_OK) {
        return false;
    }

    // Use port module's SFP information
    if ((rc = vtss_appl_port_status_get(port_state->ifindex, &pstatus)) != VTSS_RC_OK) {
        T_E("vtss_appl_port_status_get(%s) failed: %s", port_state->ifindex, error_txt(rc));
        return false;
    }

    // It could be that we got here before the port module, so try again later.
    if (strlen(pstatus.sfp_info.vendor_name) == 0) {
        T_D_PORT(port_state->port_no, "Port module hasn't read SFP info yet");
        return false;
    }

    status.sfp_type = pstatus.sfp_info.transceiver;
    strncpy(status.vendor,        pstatus.sfp_info.vendor_name, sizeof(status.vendor)        - 1);
    strncpy(status.part_number,   pstatus.sfp_info.vendor_pn,   sizeof(status.part_number)   - 1);
    strncpy(status.serial_number, pstatus.sfp_info.vendor_sn,   sizeof(status.serial_number) - 1);
    strncpy(status.revision,      pstatus.sfp_info.vendor_rev,  sizeof(status.revision)      - 1);

    // Date code. The port module only has the raw date code format. We change
    // that into YYYY-MM-DDxx format
    status.date_code[0] = '2';
    status.date_code[1] = '0';
    memcpy(&status.date_code[2], &pstatus.sfp_info.date_code[0], 2);
    status.date_code[4] = '-';
    memcpy(&status.date_code[5], &pstatus.sfp_info.date_code[2], 2);
    status.date_code[7] = '-';
    memcpy(&status.date_code[8], &pstatus.sfp_info.date_code[4], 4);
    status.date_code[12] = '\0';

    T_I_PORT(port_state->port_no, "SFP vendor %s, P/N: %s, S/N %s, Date %s", status.vendor, status.part_number, status.serial_number, status.revision, status.date_code);

    if (port_state->a2_type & 0x40) {
        port_state->status.a2_supported = true;
    } else {
        port_state->status.a2_supported = false;
    }

    return true;
}

/******************************************************************************/
// DDMI_read_a2()
/******************************************************************************/
static bool DDMI_read_a2(ddmi_port_state_t *port_state)
{
    vtss_appl_ddmi_port_status_t    &status = port_state->status;
    vtss_appl_ddmi_monitor_status_t *monitor_status;
    vtss_appl_ddmi_monitor_state_t  new_state;
    ddmi_fraction_t                 frac[VTSS_APPL_DDMI_MONITOR_STATE_CNT];
    uint8_t                         rom[116 + 2];
    char                            buf[128], *p;

    if (meba_sfp_i2c_xfer(board_instance, port_state->port_no, false, 0x51, 0, rom, sizeof(rom), false) != VTSS_RC_OK) {
        vtss_clear(status.monitor_status);
        return false;
    }

    if ((port_state->a2_type & 0x30) == 0) {
        // Neither internal nor external calibration is supported.
        vtss_clear(status.monitor_status);
        return false;
    }

    // Temperature
    monitor_status = &status.monitor_status[VTSS_APPL_DDMI_MONITOR_TYPE_TEMPERATURE];
    DDMI_temperature2str(port_state->a2_type, &rom[96], &rom[84], &rom[86], monitor_status->current,  sizeof(monitor_status->current),  frac[VTSS_APPL_DDMI_MONITOR_STATE_NONE]);
    DDMI_temperature2str(port_state->a2_type, &rom[ 6], &rom[84], &rom[86], monitor_status->warn_lo,  sizeof(monitor_status->warn_lo),  frac[VTSS_APPL_DDMI_MONITOR_STATE_WARN_LO]);
    DDMI_temperature2str(port_state->a2_type, &rom[ 4], &rom[84], &rom[86], monitor_status->warn_hi,  sizeof(monitor_status->warn_hi),  frac[VTSS_APPL_DDMI_MONITOR_STATE_WARN_HI]);
    DDMI_temperature2str(port_state->a2_type, &rom[ 2], &rom[84], &rom[86], monitor_status->alarm_lo, sizeof(monitor_status->alarm_lo), frac[VTSS_APPL_DDMI_MONITOR_STATE_ALARM_LO]);
    DDMI_temperature2str(port_state->a2_type, &rom[ 0], &rom[84], &rom[86], monitor_status->alarm_hi, sizeof(monitor_status->alarm_hi), frac[VTSS_APPL_DDMI_MONITOR_STATE_ALARM_HI]);
    if ((new_state = DDMI_state_get(frac)) != monitor_status->state) {
        monitor_status->state = new_state;
        p = buf;
        p += snprintf(p, (buf + sizeof(buf) - p), "DDMI-TEMPERATURE_CHANGED: DoM temperature changed to %s on ", ddmi_monitor_state_to_txt(new_state));

#if defined(VTSS_SW_OPTION_SYSLOG)
        (void)snprintf(p, (buf + sizeof(buf) - p), "Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
        S_PORT_I(VTSS_ISID_LOCAL, port_state->port_no, "%s", buf);
#endif /* VTSS_SW_OPTION_SYSLOG */

        T_D("%s", buf);
        DDMI_notification_status_update(port_state->ifindex, VTSS_APPL_DDMI_MONITOR_TYPE_TEMPERATURE, monitor_status);
    }

    // Voltage
    monitor_status = &status.monitor_status[VTSS_APPL_DDMI_MONITOR_TYPE_VOLTAGE];
    DDMI_voltage2str(port_state->a2_type, &rom[98], &rom[88], &rom[90], monitor_status->current,  sizeof(monitor_status->current),  frac[VTSS_APPL_DDMI_MONITOR_STATE_NONE]);
    DDMI_voltage2str(port_state->a2_type, &rom[14], &rom[88], &rom[90], monitor_status->warn_lo,  sizeof(monitor_status->warn_lo),  frac[VTSS_APPL_DDMI_MONITOR_STATE_WARN_LO]);
    DDMI_voltage2str(port_state->a2_type, &rom[12], &rom[88], &rom[90], monitor_status->warn_hi,  sizeof(monitor_status->warn_hi),  frac[VTSS_APPL_DDMI_MONITOR_STATE_WARN_HI]);
    DDMI_voltage2str(port_state->a2_type, &rom[10], &rom[88], &rom[90], monitor_status->alarm_lo, sizeof(monitor_status->alarm_lo), frac[VTSS_APPL_DDMI_MONITOR_STATE_ALARM_LO]);
    DDMI_voltage2str(port_state->a2_type, &rom[ 8], &rom[88], &rom[90], monitor_status->alarm_hi, sizeof(monitor_status->alarm_hi), frac[VTSS_APPL_DDMI_MONITOR_STATE_ALARM_HI]);
    if ((new_state = DDMI_state_get(frac)) != monitor_status->state) {
        monitor_status->state = new_state;
        p = buf;
        p += snprintf(p, buf + sizeof(buf) - p, "DDMI-VOLTAGE_CHANGED: DoM voltage changed to %s on ", ddmi_monitor_state_to_txt(new_state));

#if defined(VTSS_SW_OPTION_SYSLOG)
        (void)snprintf(p, buf + sizeof(buf) - p, "Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
        S_PORT_I(VTSS_ISID_LOCAL, port_state->port_no, "%s", buf);
#endif /* VTSS_SW_OPTION_SYSLOG */

        T_D("%s", buf);
        DDMI_notification_status_update(port_state->ifindex, VTSS_APPL_DDMI_MONITOR_TYPE_VOLTAGE, monitor_status);
    }

    // Tx Bias
    monitor_status = &status.monitor_status[VTSS_APPL_DDMI_MONITOR_TYPE_TX_BIAS];
    DDMI_txbias2str(port_state->a2_type, &rom[100], &rom[76], &rom[78], monitor_status->current,  sizeof(monitor_status->current),  frac[VTSS_APPL_DDMI_MONITOR_STATE_NONE]);
    DDMI_txbias2str(port_state->a2_type, &rom[ 22], &rom[76], &rom[78], monitor_status->warn_lo,  sizeof(monitor_status->warn_lo),  frac[VTSS_APPL_DDMI_MONITOR_STATE_WARN_LO]);
    DDMI_txbias2str(port_state->a2_type, &rom[ 20], &rom[76], &rom[78], monitor_status->warn_hi,  sizeof(monitor_status->warn_hi),  frac[VTSS_APPL_DDMI_MONITOR_STATE_WARN_HI]);
    DDMI_txbias2str(port_state->a2_type, &rom[ 18], &rom[76], &rom[78], monitor_status->alarm_lo, sizeof(monitor_status->alarm_lo), frac[VTSS_APPL_DDMI_MONITOR_STATE_ALARM_LO]);
    DDMI_txbias2str(port_state->a2_type, &rom[ 16], &rom[76], &rom[78], monitor_status->alarm_hi, sizeof(monitor_status->alarm_hi), frac[VTSS_APPL_DDMI_MONITOR_STATE_ALARM_HI]);
    if ((new_state = DDMI_state_get(frac)) != monitor_status->state) {
        monitor_status->state = new_state;
        p = buf;
        p += snprintf(p, buf + sizeof(buf) - p, "DDMI-BIAS_CHANGED: DoM Bias changed to %s on ", ddmi_monitor_state_to_txt(new_state));
#if defined(VTSS_SW_OPTION_SYSLOG)
        (void)snprintf(p, buf + sizeof(buf) - p, "Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
        S_PORT_I(VTSS_ISID_LOCAL, port_state->port_no, "%s", buf);
#endif /* VTSS_SW_OPTION_SYSLOG */

        T_D("%s", buf);
        DDMI_notification_status_update(port_state->ifindex, VTSS_APPL_DDMI_MONITOR_TYPE_TX_BIAS, monitor_status);
    }

    // Tx Power
    monitor_status = &status.monitor_status[VTSS_APPL_DDMI_MONITOR_TYPE_TX_POWER];
    DDMI_txpwr2str(port_state->a2_type, &rom[102], &rom[80], &rom[82], monitor_status->current,  sizeof(monitor_status->current),  frac[VTSS_APPL_DDMI_MONITOR_STATE_NONE]);
    DDMI_txpwr2str(port_state->a2_type, &rom[ 30], &rom[80], &rom[82], monitor_status->warn_lo,  sizeof(monitor_status->warn_lo),  frac[VTSS_APPL_DDMI_MONITOR_STATE_WARN_LO]);
    DDMI_txpwr2str(port_state->a2_type, &rom[ 28], &rom[80], &rom[82], monitor_status->warn_hi,  sizeof(monitor_status->warn_hi),  frac[VTSS_APPL_DDMI_MONITOR_STATE_WARN_HI]);
    DDMI_txpwr2str(port_state->a2_type, &rom[ 26], &rom[80], &rom[82], monitor_status->alarm_lo, sizeof(monitor_status->alarm_lo), frac[VTSS_APPL_DDMI_MONITOR_STATE_ALARM_LO]);
    DDMI_txpwr2str(port_state->a2_type, &rom[ 24], &rom[80], &rom[82], monitor_status->alarm_hi, sizeof(monitor_status->alarm_hi), frac[VTSS_APPL_DDMI_MONITOR_STATE_ALARM_HI]);
    if ((new_state = DDMI_state_get(frac)) != monitor_status->state) {
        monitor_status->state = new_state;
        p = buf;
        p += snprintf(p, buf + sizeof(buf) - p, "DDMI-TX_POWER_CHANGED: DoM Tx Power changed to %s on ", ddmi_monitor_state_to_txt(new_state));

#if defined(VTSS_SW_OPTION_SYSLOG)
        (void)snprintf(p, buf + sizeof(buf) - p, "Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
        S_PORT_I(VTSS_ISID_LOCAL, port_state->port_no, "%s", buf);
#endif /* VTSS_SW_OPTION_SYSLOG */

        T_D("%s", buf);
        DDMI_notification_status_update(port_state->ifindex, VTSS_APPL_DDMI_MONITOR_TYPE_TX_POWER, monitor_status);
    }

    // Rx Power
    monitor_status = &status.monitor_status[VTSS_APPL_DDMI_MONITOR_TYPE_RX_POWER];
    DDMI_rxpwr2str(port_state->a2_type, &rom[104], &rom[56], monitor_status->current,  sizeof(monitor_status->current),  frac[VTSS_APPL_DDMI_MONITOR_STATE_NONE]);
    DDMI_rxpwr2str(port_state->a2_type, &rom[ 38], &rom[56], monitor_status->warn_lo,  sizeof(monitor_status->warn_lo),  frac[VTSS_APPL_DDMI_MONITOR_STATE_WARN_LO]);
    DDMI_rxpwr2str(port_state->a2_type, &rom[ 36], &rom[56], monitor_status->warn_hi,  sizeof(monitor_status->warn_hi),  frac[VTSS_APPL_DDMI_MONITOR_STATE_WARN_HI]);
    DDMI_rxpwr2str(port_state->a2_type, &rom[ 34], &rom[56], monitor_status->alarm_lo, sizeof(monitor_status->alarm_lo), frac[VTSS_APPL_DDMI_MONITOR_STATE_ALARM_LO]);
    DDMI_rxpwr2str(port_state->a2_type, &rom[ 32], &rom[56], monitor_status->alarm_hi, sizeof(monitor_status->alarm_hi), frac[VTSS_APPL_DDMI_MONITOR_STATE_ALARM_HI]);
    if ((new_state = DDMI_state_get(frac)) != monitor_status->state) {
        monitor_status->state = new_state;
        p = buf;
        p += snprintf(p, buf + sizeof(buf) - p, "DDMI-RX_POWER_CHANGED: DoM Rx Power changed to %s on ", ddmi_monitor_state_to_txt(new_state));

#if defined(VTSS_SW_OPTION_SYSLOG)
        (void)snprintf(p, buf + sizeof(buf) - p, "Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
        S_PORT_I(VTSS_ISID_LOCAL, port_state->port_no, "%s", buf);
#endif /* VTSS_SW_OPTION_SYSLOG */

        T_D("%s", buf);
        DDMI_notification_status_update(port_state->ifindex, VTSS_APPL_DDMI_MONITOR_TYPE_RX_POWER, monitor_status);
    }

    return true;
}

/******************************************************************************/
// DDMI_info_update()
/******************************************************************************/
static void DDMI_info_update(ddmi_port_state_t *port_state, bool sfp_status_old, bool sfp_status_new)
{
    vtss_appl_ddmi_monitor_status_t *monitor_status;
    vtss_appl_ddmi_monitor_type_t   type;
    bool                            changed = false;
    uint64_t                        start_ms, end_ms;
    char                            buf[128], *p;

    DDMI_LOCK_SCOPE();

    if (!port_state->status.a0_supported) {
        return;
    }

    if (sfp_status_old != sfp_status_new) {
        p = buf;
        p += snprintf(p, sizeof(buf), "DDMI-MODULE_INSERT_REMOVE: %s SFP module on ", sfp_status_new ? "Inserted" : "Removed");

#if defined(VTSS_SW_OPTION_SYSLOG)
        (void)snprintf(p, sizeof(buf) - (p - buf), "Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
        S_PORT_I(VTSS_ISID_LOCAL, port_state->port_no, "%s", buf);
#endif /* VTSS_SW_OPTION_SYSLOG */

        T_D("%s", buf);
        if (sfp_status_new) {
            port_state->status.sfp_detected = true;
        } else {
            port_state->status.sfp_detected = false;
            port_state->status.a2_supported = false;
            vtss_clear(port_state->status.monitor_status);

            for (type = (vtss_appl_ddmi_monitor_type_t)0; type < VTSS_APPL_DDMI_MONITOR_TYPE_CNT; type++) {
                DDMI_notification_status_update(port_state->ifindex, type, NULL);
            }
        }

        changed = true;
    }

    if ((changed || !port_state->detected_done) && port_state->status.sfp_detected) {
        if (DDMI_read_a0(port_state)) {
            T_D_PORT(port_state->port_no, "vendor name = %s", port_state->status.vendor);
            T_D_PORT(port_state->port_no, "vendor pn = %s",   port_state->status.part_number);
            T_D_PORT(port_state->port_no, "vendor sn = %s",   port_state->status.serial_number);
            T_D_PORT(port_state->port_no, "vendor rev = %s",  port_state->status.revision);
            T_D_PORT(port_state->port_no, "date code = %s",   port_state->status.date_code);
            T_D_PORT(port_state->port_no, "transceiver = %s", port_sfp_transceiver_to_txt(port_state->status.sfp_type));
            T_D_PORT(port_state->port_no, "A2 Type = 0x%x",   port_state->a2_type);
            port_state->detected_done = true;
        } else {
            return;
        }
    }

    if (!port_state->status.a2_supported) {
        return;
    }

    start_ms = vtss::uptime_milliseconds();
    if (DDMI_read_a2(port_state)) {
        end_ms = vtss::uptime_milliseconds();
        T_I_PORT(port_state->port_no, "DDMI_read_a2() took %u ms", end_ms - start_ms);

        for (type = (vtss_appl_ddmi_monitor_type_t)0; type < VTSS_APPL_DDMI_MONITOR_TYPE_CNT; type++) {
            monitor_status = &port_state->status.monitor_status[type];
            T_D_PORT(port_state->port_no, "%s:", ddmi_monitor_type_to_txt(type));
            T_D_PORT(port_state->port_no, "State: %s", ddmi_monitor_state_to_txt(monitor_status->state));
            T_D_PORT(port_state->port_no, "Current: %s", monitor_status->current);
            T_D_PORT(port_state->port_no, "Warn range:  %s - %s", monitor_status->warn_lo, monitor_status->warn_hi);
            T_D_PORT(port_state->port_no, "Alarm range: %s - %s", monitor_status->alarm_lo, monitor_status->alarm_hi);
        }
    }
}

/******************************************************************************/
// DDMI_thread()
/******************************************************************************/
static void DDMI_thread(vtss_addrword_t data)
{
    mesa_port_list_t  sfp_status_old, sfp_status_new;
    ddmi_port_state_t *port_state;
    mesa_port_no_t    port_no;
    bool              enabled;

    sfp_status_old.clear_all();

    while (1) {
        VTSS_OS_MSLEEP(2000);

        {
            DDMI_LOCK_SCOPE();
            enabled = DDMI_global_conf.admin_enable;
        }

        if (!enabled) {
            continue;
        }

        if (meba_sfp_insertion_status_get(board_instance, &sfp_status_new) != VTSS_RC_OK) {
            T_E("Could not perform a SFP module detect");
        }

        for (port_no = VTSS_PORT_NO_START; port_no < DDMI_port_cnt; port_no++) {
            port_state = &DDMI_port_state[port_no];
            DDMI_info_update(port_state, sfp_status_old[port_no], sfp_status_new[port_no]);
        }

        sfp_status_old = sfp_status_new;
    }
}

/******************************************************************************/
// ddmi_debug_monitor_state_set()
/******************************************************************************/
mesa_rc ddmi_debug_monitor_state_set(mesa_port_no_t port_no, vtss_appl_ddmi_monitor_type_t type, vtss_appl_ddmi_monitor_state_t state)
{
    ddmi_port_state_t               *port_state;
    vtss_appl_ddmi_monitor_status_t status;

    if (type >= VTSS_APPL_DDMI_MONITOR_TYPE_CNT) {
        return DDMI_ERROR_INVALID_ARGUMENT;
    }

    if (state > VTSS_APPL_DDMI_MONITOR_STATE_CNT) {
        return DDMI_ERROR_INVALID_ARGUMENT;
    }

    DDMI_LOCK_SCOPE();

    port_state = &DDMI_port_state[port_no];
    if (!port_state->status.a2_supported) {
        return VTSS_RC_ERROR;
    }

    port_state->status.monitor_status[type].state = state;

    vtss_clear(status);
    status.state = state;
    DDMI_notification_status_update(port_state->ifindex, type, &status);

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ddmi_global_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_ddmi_global_conf_default_get(vtss_appl_ddmi_global_conf_t *conf)
{
    if (!conf) {
        T_E("conf == nullptr");
        return DDMI_ERROR_INVALID_ARGUMENT;
    }

    conf->admin_enable = false;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ddmi_global_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_ddmi_global_conf_get(vtss_appl_ddmi_global_conf_t *conf)
{
    if (!conf) {
        T_E("conf == nullptr");
        return DDMI_ERROR_INVALID_ARGUMENT;
    }

    DDMI_LOCK_SCOPE();
    *conf = DDMI_global_conf;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ddmi_global_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_ddmi_global_conf_set(const vtss_appl_ddmi_global_conf_t *conf)
{
    if (!conf) {
        T_E("conf == nullptr");
        return DDMI_ERROR_INVALID_ARGUMENT;
    }

    DDMI_LOCK_SCOPE();
    DDMI_global_conf = *conf;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ddmi_port_status_get()
/******************************************************************************/
mesa_rc vtss_appl_ddmi_port_status_get(vtss_ifindex_t ifindex, vtss_appl_ddmi_port_status_t *status)
{
    mesa_port_no_t port_no;

    if (!status) {
        return DDMI_ERROR_INVALID_ARGUMENT;
    }

    VTSS_RC(DDMI_ifindex_to_port(ifindex, port_no));

    DDMI_LOCK_SCOPE();
    *status = DDMI_port_state[port_no].status;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ddmi_notification_status_get()
/******************************************************************************/
mesa_rc vtss_appl_ddmi_notification_status_get(const vtss_appl_ddmi_notification_status_key_t *key, vtss_appl_ddmi_monitor_status_t *notif_status)
{
    if (key == nullptr) {
        return DDMI_ERROR_INVALID_ARGUMENT;
    }

    // No need to lock scope, because the .get() function is guaranteed to be
    // atomic.
    return ddmi_notification_status.get(key, notif_status);
}

/******************************************************************************/
// vtss_appl_ddmi_notification_status_itr()
/******************************************************************************/
mesa_rc vtss_appl_ddmi_notification_status_itr(const vtss_appl_ddmi_notification_status_key_t *key_prev, vtss_appl_ddmi_notification_status_key_t *key_next)
{
    vtss_appl_ddmi_monitor_status_t monitor_status;

    if (key_next == nullptr) {
        return DDMI_ERROR_INVALID_ARGUMENT;
    }

    if (key_prev == nullptr) {
        return ddmi_notification_status.get_first(key_next, &monitor_status);
    }

    *key_next = *key_prev;
    return ddmi_notification_status.get_next(key_next, &monitor_status);
}

/******************************************************************************/
// ddmi_monitor_type_to_txt()
/******************************************************************************/
const char *ddmi_monitor_type_to_txt(vtss_appl_ddmi_monitor_type_t type)
{
    switch (type) {
    case VTSS_APPL_DDMI_MONITOR_TYPE_TEMPERATURE:
        return "Temperature";

    case VTSS_APPL_DDMI_MONITOR_TYPE_VOLTAGE:
        return "Voltage";

    case VTSS_APPL_DDMI_MONITOR_TYPE_TX_BIAS:
        return "TxBias";

    case VTSS_APPL_DDMI_MONITOR_TYPE_TX_POWER:
        return "TxPower";

    case VTSS_APPL_DDMI_MONITOR_TYPE_RX_POWER:
        return "RxPower";

    default:
        T_E("Unknown type (%d)", type);
        return "Unknown";
    }
}

/******************************************************************************/
// ddmi_monitor_state_to_txt()
/******************************************************************************/
const char *ddmi_monitor_state_to_txt(vtss_appl_ddmi_monitor_state_t state)
{
    switch (state) {
    case VTSS_APPL_DDMI_MONITOR_STATE_NONE:
        return "None";

    case VTSS_APPL_DDMI_MONITOR_STATE_WARN_LO:
        return "Low warning";

    case VTSS_APPL_DDMI_MONITOR_STATE_WARN_HI:
        return "High warning";

    case VTSS_APPL_DDMI_MONITOR_STATE_ALARM_LO:
        return "Low alarm";

    case VTSS_APPL_DDMI_MONITOR_STATE_ALARM_HI:
        return "High alarm";

    default:
        T_E("Unknown monitor state (%d)", state);
        return "Unknown";
    }
}

/******************************************************************************/
// ddmi_error_txt()
/******************************************************************************/
const char *ddmi_error_txt(mesa_rc rc)
{
    switch (rc) {
    case DDMI_ERROR_INVALID_ARGUMENT:
        return "Invalid argument";

    case DDMI_ERROR_IFINDEX_NOT_PORT:
        return "Interface is not a port interface";

    case DDMI_ERROR_INVALID_PORT:
        return "Port number is out of range";

    default:
        return "DDMI: Unknown error code";
    }
}

extern "C" {
    int  ddmi_icli_cmd_register(void);
    void ddmi_mib_init(void);
    void vtss_appl_ddmi_json_init(void);
}

mesa_rc ddmi_icfg_init(void);

/******************************************************************************/
// ddmi_init()
/******************************************************************************/
mesa_rc ddmi_init(vtss_init_data_t *data)
{
    mesa_port_no_t    port_no;
    ddmi_port_state_t *port_state;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        DDMI_port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

        critd_init(&DDMI_crit, "ddmi", VTSS_MODULE_ID_DDMI, CRITD_TYPE_MUTEX);
        vtss_flag_init(&DDMI_status_flags);

        for (port_no = 0; port_no < DDMI_port_cnt; port_no++) {
            port_state = &DDMI_port_state[port_no];

            vtss_clear(*port_state);
            port_state->port_no = port_no;

            if (vtss_ifindex_from_port(VTSS_ISID_LOCAL, port_no, &port_state->ifindex) != VTSS_RC_OK) {
                T_E("vtss_ifindex_from_port(%d) failed", port_no);
            }

            if ((port_custom_table[port_no].cap & (MEBA_PORT_CAP_SFP_DETECT | MEBA_PORT_CAP_DUAL_SFP_DETECT)) &&
                (port_custom_table[port_no].cap & MEBA_PORT_CAP_SFP_INACCESSIBLE) == 0) {
                port_state->status.a0_supported = true;
                port_state->detected_done = false;
            }
        }

#if defined(VTSS_SW_OPTION_ICFG)
        VTSS_RC(ddmi_icfg_init());
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register private mib */
        ddmi_mib_init();
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_ddmi_json_init();
#endif

        ddmi_icli_cmd_register();
        break;

    case INIT_CMD_START:
        // Create and start DDMI thread
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           DDMI_thread,
                           0,
                           "DDMI_main",
                           NULL,
                           0,
                           &DDMI_thread_handle,
                           &DDMI_thread_block);
        break;

    case INIT_CMD_CONF_DEF:
        if (data->isid == VTSS_ISID_GLOBAL) {
            DDMI_LOCK_SCOPE();
            vtss_appl_ddmi_global_conf_default_get(&DDMI_global_conf);
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE: {
        DDMI_LOCK_SCOPE();
        vtss_appl_ddmi_global_conf_default_get(&DDMI_global_conf);
        break;
    }

    default:
        break;
    }

    return VTSS_RC_OK;
}

