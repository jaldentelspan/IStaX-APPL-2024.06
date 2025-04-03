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

/* Implement the serial 1pps interface */

#include "main.h"
#include "vtss_os_wrapper.h"
#include "ptp_1pps_serial.h"
#include "vtss_ptp_api.h"
#include "ptp.h"
#include "ptp_api.h"
#include "ptp_parse_nmea.hxx"
#include "vtss/basics/string.hxx"
#include "vtss/appl/ptp.h"

static vtss_thread_t    thread_data;
static vtss_handle_t    thread_handle;
static vtss_io_handle_t ser1;

static const char *ptp_1pps_convert_time_2_message(const mesa_timestamp_t *t, const ptp_rs422_protocol_t proto)
{
    static char buf[64];
    struct tm timeinfo;

    if(t->seconds > 0) {
        time_t my_time = t->seconds;
        struct tm *timeinfo_p = localtime_r(&my_time, &timeinfo);
        int len;

        switch(proto) {
           case VTSS_PTP_RS422_PROTOCOL_SER_ZDA:
               len = snprintf(buf, sizeof(buf)-1, "$GPZDA,%02d%02d%02d.000,%02d,%02d,%04d,00,00*",
                              timeinfo_p->tm_hour,
                              timeinfo_p->tm_min,
                              timeinfo_p->tm_sec,
                              timeinfo_p->tm_mday,
                              timeinfo_p->tm_mon+1,
                              timeinfo_p->tm_year+1900);
               break;
           case VTSS_PTP_RS422_PROTOCOL_SER_RMC:
               // $GPRMC,123733.022,A,5543.147827,N,01226.113680,E,0.027,303.72,140815,,,A*52
               len = snprintf(buf, sizeof(buf)-1, "$GPRMC,%02d%02d%02d,A,,,,,,,%02d%02d%02d,,,*",
                              timeinfo_p->tm_hour,
                              timeinfo_p->tm_min,
                              timeinfo_p->tm_sec,
                              timeinfo_p->tm_mday,
                              timeinfo_p->tm_mon+1,
                              timeinfo_p->tm_year);
               break;
           default:
               if (timeinfo_p->tm_year >= 100) timeinfo_p->tm_year -= 100;
               len = snprintf(buf, sizeof(buf)-1, "$POLYT,%02d%02d%02d.00,%02d%02d%02d,,,,%d,,,,,*",
                              timeinfo_p->tm_hour,
                              timeinfo_p->tm_min,
                              timeinfo_p->tm_sec,
                              timeinfo_p->tm_mday,
                              timeinfo_p->tm_mon+1,
                              timeinfo_p->tm_year,
                              t->nanoseconds);
        }
        u8 checksum = 0;
        for (int i = 1; buf[i] != '*'; i++) {
            checksum ^= (unsigned char)buf[i];
        }
        (void)sprintf(buf + len, "%02x\r\n", checksum);

        return buf;
    }
    return "-";
}

static int ptp_1pps_convert_message_2_time(const char *s, mesa_timestamp_t *t)
{
    vtss_ptp_rs422_conf_t conf;

    vtss_ext_clock_rs422_conf_get(&conf);
    t->sec_msb = 0;
    t->seconds = 0;
    t->nanoseconds = 0;
    t->nanosecondsfrac = 0;

    if (s != NULL) {
        vtss::str _str(s);
        const char *b = _str.begin();
        struct tm timeinfo;
        i32 nanoseconds;
        u8 rx_sum;
        timeinfo.tm_isdst = 0; // assuming standard time
        if (conf.proto == VTSS_PTP_RS422_PROTOCOL_SER_ZDA) {
            vtss::NmeaZdaParser nmea_zda_parser;
            if (nmea_zda_parser(b, _str.end())) {
                vtss_appl_ptp_clock_timeproperties_ds_t timeproperties_ds;
                int utc_tai_ofs = 0;
                if (ptp_get_virtual_port_time_property(conf.instance, &timeproperties_ds) == VTSS_RC_OK) {
                    if (timeproperties_ds.currentUtcOffsetValid)
                        utc_tai_ofs = timeproperties_ds.currentUtcOffset;
                }

                timeinfo.tm_hour = nmea_zda_parser.time.hour;
                timeinfo.tm_min =  nmea_zda_parser.time.min;
                timeinfo.tm_sec =  nmea_zda_parser.time.sec + utc_tai_ofs;  // NOTE: we assume the received time  matches the time of the prev PPS. Also need to add UTC offset as time reported by GPS is UTC and not TAI.
                                                                                //       The wrapping from 59 to 60 will be corrected by the call to mktime below.
                timeinfo.tm_mday =  nmea_zda_parser.date.day;
                timeinfo.tm_mon = nmea_zda_parser.date.month - 1;
                timeinfo.tm_year = nmea_zda_parser.date.year - 1900;

                nanoseconds = 0;  // NOTE: The PPS pulse is output exactly at the second so we shall ignore the milliseconds output in the ZDA message

                rx_sum = nmea_zda_parser.chksum.value;
                T_DG(VTSS_TRACE_GRP_PTP_SER, "zda message utc offset %d", utc_tai_ofs);
            } else {
                T_NG(VTSS_TRACE_GRP_PTP_SER, "Error encountered while parsing message from GPS. Was probably not an NMEA ZDA message.");
                return 0;
            }
        }
        else if (conf.proto == VTSS_PTP_RS422_PROTOCOL_SER_RMC) {
            vtss::NmeaRmcParser nmea_rmc_parser;
            if (nmea_rmc_parser(b, _str.end())) {  // Status != 'A' means GPS data is not valid. In that case do not return anything.
                if (nmea_rmc_parser.a_v.status != 'A') {
                    T_DG(VTSS_TRACE_GRP_PTP_SER, "GPS status is not 'A'. Therefore data is not valid.");
                    return 0;
                }

                vtss_appl_ptp_clock_timeproperties_ds_t timeproperties_ds;
                int utc_tai_ofs = 0;
                if (ptp_get_virtual_port_time_property(conf.instance, &timeproperties_ds) == VTSS_RC_OK) {
                    if (timeproperties_ds.currentUtcOffsetValid)
                        utc_tai_ofs = timeproperties_ds.currentUtcOffset;
                }

                timeinfo.tm_hour = nmea_rmc_parser.time.hour;
                timeinfo.tm_min =  nmea_rmc_parser.time.min;
                timeinfo.tm_sec =  nmea_rmc_parser.time.sec + utc_tai_ofs;  // NOTE: assume the received time  matches the time of the prev PPS. Also need to add UTC offset as time reported by GPS is UTC and not TAI.
                                                                                //       The wrapping from 59 to 60 will be corrected by the call to mktime below.
                timeinfo.tm_mday =  nmea_rmc_parser.date.day;
                timeinfo.tm_mon = nmea_rmc_parser.date.month - 1;
                timeinfo.tm_year = nmea_rmc_parser.date.year;
                if (timeinfo.tm_year < 70) timeinfo.tm_year += 100;

                nanoseconds = 0;  // NOTE: The PPS pulse is output exactly at the second so we shall ignore the milliseconds output in the RMC message

                rx_sum = nmea_rmc_parser.chksum.value;
                T_DG(VTSS_TRACE_GRP_PTP_SER, "rmc message utc offset %d", utc_tai_ofs);
            } else {
                T_NG(VTSS_TRACE_GRP_PTP_SER, "Error encountered while parsing message from GPS. Was probably not an NMEA RMC message.");
                return 0;
            }
        }
        else {
            vtss::NmeaPolytParser nmea_polyt_parser;  // FIXME: Parser should determine whether message comes from another switch (some fields will be undefined) or from GPS (all fields defined).
            if (nmea_polyt_parser(b, _str.end())) {   //        When message comes from a switch, the nanoseconds shall be used. When message comes from GPS, only the seconds shall be used.
                timeinfo.tm_hour = nmea_polyt_parser.time.hour;
                timeinfo.tm_min =  nmea_polyt_parser.time.min;
                timeinfo.tm_sec =  nmea_polyt_parser.time.sec;  // NOTE: assume the received time  matches the time of the prev PPS. The wrapping from 59 to 60 will be corrected by the call to mktime below.

                timeinfo.tm_mday =  nmea_polyt_parser.date.day;
                timeinfo.tm_mon = nmea_polyt_parser.date.month - 1;
                timeinfo.tm_year = nmea_polyt_parser.date.year;
                if (timeinfo.tm_year < 70) timeinfo.tm_year += 100;

                if (nmea_polyt_parser.utc_tow.valid && nmea_polyt_parser.gps_tow.valid) {
                    int utc_gps_ofs = nmea_polyt_parser.gps_tow.sec - nmea_polyt_parser.utc_tow.sec;
                    T_NG(VTSS_TRACE_GRP_PTP_SER, "GPS time is %d seconds ahead of UTC this means PTP is %d seconds ahead of UTC.", utc_gps_ofs, (utc_gps_ofs + 19));
                    timeinfo.tm_sec += (utc_gps_ofs + 19);
                }

                nanoseconds = nmea_polyt_parser.time.msecs * 1000000L - nmea_polyt_parser.bias.nsecs;
                if (nanoseconds < 0) {
                    nanoseconds += 1000000000L;
                    timeinfo.tm_sec -= 1;
                    mktime(&timeinfo);
                }
                else if (nanoseconds >= 1000000000L) {
                    nanoseconds -= 1000000000L;
                    timeinfo.tm_sec += 1;
                    mktime(&timeinfo);
                }
                //nanoseconds = 0;  // NOTE: The PPS pulse is output exactly at the second so we shall ignore the milliseconds output in the Polyt message

                rx_sum = nmea_polyt_parser.chksum.value;
            } else {
                T_NG(VTSS_TRACE_GRP_PTP_SER, "Error encountered while parsing message from GPS. Was probably not a POLYT message.");
                return 0;
            }
        }

        u8 checksum = 0;
        for (int i = 1; s[i] != '*'; i ++) {
            checksum ^= (unsigned char)s[i];
        }
        if (checksum != rx_sum) {
            T_IG(VTSS_TRACE_GRP_PTP_SER,"Checksum error, checksum: %x, rx_sum: %x", checksum, rx_sum);
            return 0;
        }

        t->seconds = mktime(&timeinfo);
        t->nanoseconds = nanoseconds;

        T_DG(VTSS_TRACE_GRP_PTP_SER, "Successfully parsed serial message %s", s);

        return 1;
    }
    else return 0;
}

void ptp_1pps_msg_send(const mesa_timestamp_t *t, const ptp_rs422_protocol_t proto)
{
    const char *buf = ptp_1pps_convert_time_2_message(t, proto);
    size_t len = strlen(buf);

    T_NG(VTSS_TRACE_GRP_PTP_SER, "Serial Message %s", buf);
    if (vtss_io_write(ser1, buf, &len)  != ENOERR) {
        T_WG(VTSS_TRACE_GRP_PTP_SER,"Error writing to ser1");
    }
}

void ptp_rs422_alarm_send(mesa_bool_t alarm)
{
    char buf[64] = {};
    size_t len;

    // $GMCPS,{type of time source},{status of time source},{Alarm status monitor}*checksum
    len = snprintf(buf, sizeof(buf)-1, "$PMCPS,%02d,%02d,%05d*",
                   0x2,   //PTP source
                   0x0,   //Position Unknown
                   alarm ? 0x200 : 0);

    u8 checksum = 0;
    for (int i = 1; buf[i] != '*'; i++) {
        checksum ^= (unsigned char)buf[i];
    }
    snprintf(buf + len, sizeof(buf)-1-len, "%02x\r\n", checksum);
    len = strlen(buf);
    T_DG(VTSS_TRACE_GRP_PTP_SER, "Sending alarm %s", buf);
    if (vtss_io_write(ser1, buf, &len)  != ENOERR) {
        T_WG(VTSS_TRACE_GRP_PTP_SER,"Error writing to ser1");
    }
}

// returns true if decoding is succesfull else false.
static bool ptp_1pps_convert_message_to_alarm_status(const char *s, mesa_bool_t *alarm)
{
    vtss::NmeaAlarmParser nmea_alarm_parser;
    vtss::str _str(s);
    const char *b = _str.begin();
    bool ret = false;
    if (nmea_alarm_parser(b, _str.end())) {
        T_DG(VTSS_TRACE_GRP_PTP_SER, "type %d src-status %d alarm %d", nmea_alarm_parser.ts_type.type, nmea_alarm_parser.ts_status.status, nmea_alarm_parser.alarm.alm_status);
        *alarm = nmea_alarm_parser.alarm.alm_status ? TRUE : FALSE;
        ret = true;
        // Validate cheksum
        u8 checksum = 0;
        for (int i = 1; s[i] != '*'; i ++) {
            checksum ^= (unsigned char)s[i];
        }
        if (checksum != nmea_alarm_parser.chksum.value) {
            T_IG(VTSS_TRACE_GRP_PTP_SER,"Checksum error, expected checksum: %x, pkt chk_sum: %x", checksum, nmea_alarm_parser.chksum.value);
        }
    }

    return ret;
}
//---------------------------------------------------------------------------
// Serial input main function.
static void ptp_1pps_serial_thread( void )
{
    u8 in_buffer[1];
    char rx_buffer[256];
    size_t len = 1;
    unsigned int rx_idx = 0;
    mesa_timestamp_t rx_t;
    char s[50] = {};
    mesa_bool_t alarm;
    meba_ptp_rs422_conf_t rs422_conf;
    const char *rs422_serial_port = VTSS_OS_RS422;

    if (meba_ptp_rs422_conf_get(board_instance, &rs422_conf) == VTSS_RC_OK &&
        rs422_conf.serial_port) {
        rs422_serial_port = rs422_conf.serial_port;
    }

    T_IG(VTSS_TRACE_GRP_PTP_SER,"starting serial thread %s", rs422_serial_port);
    if (vtss_io_lookup(rs422_serial_port, &ser1) != ENOERR) {
        T_WG(VTSS_TRACE_GRP_PTP_SER,"Error opening %s", rs422_serial_port);
        return;
    }

    memset(rx_buffer, 0, sizeof(rx_buffer));
    while (1) {
        len = 1;
        if (vtss_io_read(ser1, in_buffer, &len) != ENOERR) {
            T_IG(VTSS_TRACE_GRP_PTP_SER,"Error reading from %s", rs422_serial_port);
        } else {
            if(len) {
                if (in_buffer[0] == '$') {
                    rx_idx = 0;
                    memset(rx_buffer, 0, sizeof(rx_buffer));
                    rx_buffer[rx_idx++] = in_buffer[0];
                } else if (in_buffer[0] == 0x0a) {
                    // call out to handler
                    T_NG(VTSS_TRACE_GRP_PTP_SER,"Received: %s", rx_buffer);

                    if (strstr(rx_buffer, "$PMCPS")) {
                        if (ptp_1pps_convert_message_to_alarm_status(rx_buffer, &alarm)) {
                            ptp_virtual_port_alarm_rx(alarm);
                        }
                        memset(rx_buffer, 0, sizeof(rx_buffer));
                    } else if (ptp_1pps_convert_message_2_time(rx_buffer, &rx_t)) {
                        T_IG(VTSS_TRACE_GRP_PTP_SER,"time received %s", TimeStampToString (&rx_t, s));
                        ptp_virtual_port_timestamp_rx(&rx_t);
                    }
                } else {
                    rx_buffer[rx_idx++] = in_buffer[0];
                    if (rx_idx >= sizeof(rx_buffer) - 1) {
                        T_IG(VTSS_TRACE_GRP_PTP_SER,"RS422 rx buffer overflow");
                        rx_idx = 0;
                    }
                }
            }
        }
    }
}

mesa_rc ptp_1pps_set_baudrate(const vtss_serial_info_t *serial_info)
{
    mesa_rc rc = VTSS_RC_OK;
    size_t length;
    int err_code;
    if ((err_code = vtss_io_set_config(ser1, VTSS_IO_SET_CONFIG_SERIAL_INFO, serial_info, &length)) != ENOERR) {
        T_WG(VTSS_TRACE_GRP_PTP_SER,"Error setting configuration of RS422 port. Error code was %d", err_code);
        rc = VTSS_RC_ERROR;
    }
    return rc;
}

mesa_rc ptp_1pps_get_baudrate(vtss_serial_info_t *serial_info)
{
    mesa_rc rc = VTSS_RC_OK;
    size_t length;
    int err_code;
    if ((err_code = vtss_io_get_config(ser1, VTSS_IO_GET_CONFIG_SERIAL_INFO, serial_info, &length)) != ENOERR) {
        T_WG(VTSS_TRACE_GRP_PTP_SER,"Error getting configuration of RS422 port. Error code was %d", err_code);
        rc = VTSS_RC_ERROR;
    }
    return rc;
}

void ptp_1pps_get_default_baudrate(vtss_serial_info_t *serial_info)
{
    serial_info->baud = VTSS_SERIAL_BAUD_115200;
    serial_info->parity = VTSS_SERIAL_PARITY_NONE;
    serial_info->word_length = VTSS_SERIAL_WORD_LENGTH_8;
    serial_info->stop = VTSS_SERIAL_STOP_1;
    serial_info->flags = 0;
}

mesa_rc ptp_1pps_serial_init(void)
{
    vtss_thread_create(VTSS_THREAD_PRIO_BELOW_NORMAL,                 // Priority
                      (vtss_thread_entry_f *)ptp_1pps_serial_thread,  // entry
                      0,                                              //
                      "PTP-serial",                                   // Name
                      nullptr,                                        // Stack
                      0,                                              // Size
                      &thread_handle,                                 // Handle
                      &thread_data                                    // Thread data structure
                     );
    return VTSS_RC_OK;
}
