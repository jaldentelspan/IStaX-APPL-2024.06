/*
 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_APPL_PTP_PARSE_NMEA_HXX_
#define _VTSS_APPL_PTP_PARSE_NMEA_HXX_

#include "vtss/basics/parser_impl.hxx"

namespace vtss {

struct NmeaAorV : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    char status = ' ';
};

struct NmeaTime : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    unsigned hour;
    unsigned min;
    unsigned sec;
    unsigned msecs;
};

struct NmeaTimeOfWeek : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    bool valid = false;
    unsigned sec;
    unsigned usecs;
};

struct NmeaMsecs : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    unsigned value;
};

struct NmeaUsecs : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    unsigned value;
};

struct NmeaWeek : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    bool valid = false;
    unsigned week;
};

struct NmeaDate : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    unsigned day;
    unsigned month;
    unsigned year;
};

struct NmeaDate2 : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    unsigned day;
    unsigned month;
    unsigned year;
};

struct NmeaBias : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    int nsecs;
};

struct NmeaChecksum : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);
    
    u8 value;
};

struct NmeaPolytParser : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    NmeaTime time;
    NmeaDate date;
    NmeaTimeOfWeek utc_tow;
    NmeaWeek gps_week;
    NmeaTimeOfWeek gps_tow;
    NmeaBias bias;

    NmeaChecksum chksum;
};

struct NmeaZdaParser : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    NmeaTime time;
    NmeaDate2 date;
    NmeaChecksum chksum;
};

struct NmeaRmcParser : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    NmeaTime time;
    NmeaDate date;
    NmeaAorV a_v;
    NmeaChecksum chksum;
};

struct NmeaAlarmTimeSrcType : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    uint8_t type;
};

struct NmeaAlarmTimeSrcStatus : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    uint8_t status;
};

struct NmeaAlarmMonitorStatus : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    uint16_t alm_status;
};

struct NmeaAlarmParser : public parser::ParserBase {
    bool operator()(const char *& b, const char * e);

    NmeaAlarmTimeSrcType   ts_type;
    NmeaAlarmTimeSrcStatus ts_status;
    NmeaAlarmMonitorStatus alarm;
    NmeaChecksum           chksum;
};

}  // namespace vtss

#endif // _VTSS_APPL_PTP_PARSE_NMEA_HXX_
