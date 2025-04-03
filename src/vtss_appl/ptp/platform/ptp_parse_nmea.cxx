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

#include "ptp_parse_nmea.hxx"
#include "vtss_ptp_api.h"                         // Note: Only included to enable trace. Can be excluded if trace not needed.
#include "ptp.h"                                  // Note: Only included to enable trace. Can be excluded if trace not needed.
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_PTP   // Note: Only included to enable trace. Can be excluded if trace not needed.
#include "vtss/basics/trace.hxx"                  // Note: Only included to enable trace. Can be excluded if trace not needed.

namespace vtss {

struct NmeaElement : public parser::ParserBase {
    bool operator()(const char *& b, const char * e) {
        if (b != e && *b == ',') {
            while (1) if (++b == e || *b == ',') return true;
        }
        else return false;
    }
};

struct NmeaNorS : public parser::ParserBase {
    bool operator()(const char *& b, const char * e) {
        const char *b_ = b;
        if (b != e && *b == ',') {
            if (++b == e || *b == ',') return true;
        }
        else return false;
        if (*b == 'N' || *b == 'S') {
            ++b;
            return true;
        }
        b = b_;
        return false;
    }
};

struct NmeaEorW : public parser::ParserBase {
    bool operator()(const char *& b, const char * e) {
        const char *b_ = b;
        if (b != e && *b == ',') {
            if (++b == e || *b == ',') return true;
        }
        else return false;
        if (*b == 'E' || *b == 'W') {
            ++b;
            return true;
        }
        b = b_;
        return false;
    }
};

struct NmeaSkip: public parser::ParserBase {
    bool operator()(const char *& b, const char * e) {
        const char *b_ = b;
        while (b != e) {
            if (*b++ == '*')
                return true;
        }
        b = b_;
        return false;
    }
};

bool NmeaAorV::operator()(const char *& b, const char * e) {
    status = ' ';
    
    const char *b_ = b;
    if (b != e && *b == ',') {
        if (++b == e || *b == ',') return true;
    }
    else return false;
    if (*b == 'A' || *b == 'V') {
        status = *b;
        ++b;
        return true;
    }
    b = b_;
    return false;
}

bool NmeaMsecs::operator()(const char *& b, const char * e) {
    parser::Lit dot(".");
    parser::Int<unsigned, 10, 1, 3> sub;

    value = 0;

    const char *_b = b;
    if (!dot(b, e)) {
        b = _b;
        return false;
    }

    _b = b;
    if (!sub(b, e)) {
        b = _b;
        return false;
    }
    value = sub.get();
    if (b - _b == 1) {
        value *= 100;
    }
    else if (b - _b == 2) {
        value *= 10;
    }

    return true;
}

bool NmeaUsecs::operator()(const char *& b, const char * e) {
    parser::Lit dot(".");
    parser::Int<unsigned, 10, 1, 7> sub;

    value = 0;

    const char *_b = b;
    if (!dot(b, e)) {
        b = _b;
        return false;
    }

    _b = b;
    if (!sub(b, e)) {
        b = _b;
        return false;
    }

    value = sub.get();
    switch(b - _b) {
        case 1: value *= 100000; break;
        case 2: value *= 10000;  break;
        case 3: value *= 1000;   break;
        case 4: value *= 100;    break;
        case 5: value *= 10;     break;
        case 7: value /= 10;     break;
    }

    return true;
}

bool NmeaWeek::operator()(const char *& b, const char * e) {
    parser::Lit comma(",");
    parser::Int<unsigned int, 10, 1, 4> week;

    valid = false;
    if (!Group(b, e, comma, week)) {
        // Could not parse a valid week number. Skip everything up until next comma or end of input whatever comes first.
        if (b != e && *b == ',') {
            while (1) if (++b == e || *b == ',') return true;
        }
        else {
            return false;
        }
    }
    valid = true;

    return true;
}

bool NmeaTime::operator()(const char *& b, const char * e) {
    parser::Int<unsigned char, 10, 2, 2> h;
    parser::Int<unsigned char, 10, 2, 2> m;
    parser::Int<unsigned char, 10, 2, 2> s;
    parser::Lit comma(",");
    NmeaMsecs ms;

    if (!Group(b, e, comma, h, m, s)) {
        return false;
    }

    hour = h.get();
    min = m.get();
    sec = s.get();

    const char *_b = b;
    if (!ms(b, e)) {
        b = _b;
        msecs = 0;
        return true;
    }
    msecs = ms.value;

    return true;
}

bool NmeaTimeOfWeek::operator()(const char *& b, const char * e) {
    parser::Int<unsigned int, 10, 1, 6> s;
    parser::Lit comma(",");
    NmeaUsecs us;

    valid = false;
    if (!Group(b, e, comma, s)) {
        // Could not parse a valid time of week. Skip everything up until next comma or end of input whatever comes first.
        if (b != e && *b == ',') {
            while (1) if (++b == e || *b == ',') return true;
        }
        else {
            return false;
        }
    }
    valid = true;
    sec = s.get();

    const char *_b = b;
    if (!us(b, e)) {
        b = _b;
        usecs = 0;
        return true;
    }
    usecs = us.value;

    return true;
}

bool NmeaDate::operator()(const char *& b, const char * e) {
    parser::Lit comma(",");
    parser::Int<unsigned char, 10, 2, 2> d;
    parser::Int<unsigned char, 10, 2, 2> m;
    parser::Int<unsigned char, 10, 2, 2> y;

    if (!Group(b, e, comma, d, m, y)) {
        return false;
    }

    day = d.get();
    month = m.get();
    year = y.get();

    return true;
}

bool NmeaDate2::operator()(const char *& b, const char * e) {
    parser::Lit comma(",");
    parser::Int<unsigned char, 10, 1, 2> d;
    parser::Int<unsigned char, 10, 1, 2> m;
    parser::Int<unsigned, 10, 1, 4> y;

    if (!Group(b, e, comma, d, comma, m, comma, y)) {
        return false;
    }

    day = d.get();
    month = m.get();
    year = y.get();

    return true;
}

bool NmeaBias::operator()(const char *& b, const char * e) {
    parser::Lit comma(",");
    parser::Int<int, 10, 1, 7> bias_;

    if (!Group(b, e, comma, bias_)) {
        return false;
    }

    nsecs = bias_.get();

    return true;
}

bool NmeaChecksum::operator()(const char *& b, const char * e) {
    parser::Int<unsigned char, 16, 2, 2> check;

    if (!check(b, e)) {
        return false;
    }
    
    value = check.get();

    return true;
}

bool NmeaAlarmTimeSrcType::operator()(const char *& b, const char * e) {
    parser::Int<unsigned char, 10, 2, 2> check;

    if (!check(b, e)) {
        return false;
    }

    type = check.get();

    return true;
}

bool NmeaAlarmTimeSrcStatus::operator()(const char *& b, const char * e) {
    parser::Int<unsigned char, 10, 2, 2> check;

    if (!check(b, e)) {
        return false;
    }

    status = check.get();

    return true;
}

bool NmeaAlarmMonitorStatus::operator()(const char *& b, const char * e) {
    parser::Int<unsigned short, 10, 1, 5> check;

    if (!check(b, e)) {
        return false;
    }

    alm_status = check.get();

    return true;
}

bool NmeaPolytParser::operator()(const char *& b, const char * e) {
    parser::Lit polyt("$POLYT");
    parser::Lit comma(",");
    NmeaSkip s;
    NmeaElement o;

    if (Group(b, e, polyt, time, date, utc_tow, gps_week, gps_tow, bias, s, chksum)) {
        return true;
    }

    return false;
}

bool NmeaZdaParser::operator()(const char *& b, const char * e) {
    parser::Lit gpzda("$GPZDA");
    parser::Lit comma(",");
    parser::Int<int, 10, 1, 3> offset_hours;
    parser::Int<int, 10, 1, 3> offset_minutes;
    NmeaSkip s;

    if (Group(b, e, gpzda, time, date, comma, offset_hours, comma, offset_minutes, s, chksum)) {
        return true;
    }

    return false;
}

bool NmeaRmcParser::operator()(const char *& b, const char * e) {
    parser::Lit gprmc("$GPRMC");
    parser::Lit gnrmc("$GNRMC");
    parser::Lit comma(",");
    NmeaNorS n_s;
    NmeaEorW e_w;
    NmeaSkip s;
    NmeaElement o;

    if (Group(b, e, gnrmc, time, a_v, o, n_s, o, e_w, o, o, date, s, chksum))
    {
        return true;
    }
    if (Group(b, e, gprmc, time, a_v, o, n_s, o, e_w, o, o, date, s, chksum)) {
        return true;
    }

    return false;
}

bool NmeaAlarmParser::operator()(const char *& b, const char * e) {
    parser::Lit alarms("$PMCPS");
    parser::Lit comma(",");
    NmeaSkip s;
    NmeaElement o;

    if (Group(b, e, alarms, comma, ts_type, comma, ts_status, comma, alarm, s, chksum)) {
        return true;
    }

    return false;
}
// TEST_CASE("asdf", "adsf") {
//     str in("$POLYT,172913.00,010170,,,,51,,,,,*50");
// 
//     NmeaParser p;
// 
//     const char *b = in.begin();
//     REQUIRE(p(b, in.end()));
// 
//     VTSS_TRACE(ERROR) << p.time.ts;
// }

}  // namespace vtss
