/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss_trace_io.h"
#include "main.h"
#include "vtss_module_id.h"
#include "vtss_os_wrapper.h"
#include <time.h>
#ifdef VTSS_SW_OPTION_SYSUTIL
#include <sysutil_api.h>
#endif
#ifdef VTSS_SW_OPTION_DAYLIGHT_SAVING
#include "daylight_saving_api.h"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_BASICS
#include "vtss/basics/predefs.hxx"
#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/trace_grps.hxx"

static vtss_trace_reg_t basics_trace_reg = {
    VTSS_MODULE_ID_BASICS, "basics", "basics"
};

#define DECLARE_GRP(STR)           \
    {                              \
        STR,                       \
        STR,                       \
        VTSS_TRACE_LVL_ERROR       \
    }

static vtss_trace_grp_t basics_trace_grps[] = {
    DECLARE_GRP("default"),
    DECLARE_GRP("snmp"),
    DECLARE_GRP("json"),
    DECLARE_GRP("http"),
    DECLARE_GRP("event"),
    DECLARE_GRP("dhcp"),
    DECLARE_GRP("process")
};
#undef DECLARE_GRP

VTSS_TRACE_REGISTER(&basics_trace_reg, basics_trace_grps);

namespace vtss {
namespace trace {

struct WebStaXTraceStream : public ostream {
    bool ok() const {
        return true;
    }

    bool push(char val) {
        trace_write_char(0, val);
        return true;
    }

    size_t write(const char *b, const char *e) {
        size_t length = e - b;
        trace_write_string_len(0, b, length);
        return length;
    }

    ~WebStaXTraceStream() { }
};

struct WebStaXTraceRingbufStream : public ostream {
    bool ok() const {
        return true;
    }

    bool push(char val) {
        trace_rb_write_char(0, val);
        return true;
    }

    size_t write(const char *b, const char *e) {
        size_t length = e - b;
        trace_rb_write_string_len(0, b, length);
        return length;
    }

    ~WebStaXTraceRingbufStream() { }
};


void MessageMetaData::sample() const {
    useconds_ = 0;
    if (trace_grp_->flags & VTSS_TRACE_FLAGS_USEC) {
        useconds_ = hal_time_get();
    }

    walltime_ = time(NULL);
#if defined(VTSS_SW_OPTION_SYSUTIL)
    /* Correct for timezone */
    walltime_ += (system_get_tz_off() * 60);
#endif
#ifdef VTSS_SW_OPTION_DAYLIGHT_SAVING
    /* Correct for DST */
    walltime_ += (time_dst_get_offset() * 60);
#endif

    thread_id_ = vtss_thread_id_get();
}

str trace_sub_path(const str& in) {
    unsigned match = 0;
    const char *i = in.end();

    while (i != in.begin()) {
        --i;

        if (*i == '/') match++;
        if (match == 3) break;
    }

    if (*i == '/' && i != in.end()) ++i;

    return str(i, in.end());
}

ostream& operator<<(ostream& o, const MessageMetaData &rhs) {
    o << rhs.level() << ' ';
    o << rhs.name();

    if (rhs.group_idx() > 0) {
        o << '/' << rhs.group_name();
    }
    o << ' ';

    if (rhs.trace_grp_->flags & VTSS_TRACE_FLAGS_USEC) {
        uint64_t u = rhs.useconds();
        o << as_time_us(u) << ' ';
    } else {
        int t = rhs.walltime();
        o << as_localtime_hr_min_sec(t) << ' ';
    }

    o << rhs.thread_id() << '/' << trace_sub_path(str(rhs.file_name()))
            << '#' << rhs.line_number() << ": ";

    return o;
}

GlobalSettings global_settings;

namespace severity {
    ostream& operator<<(ostream& o, const E &rhs) {
        switch (rhs) {
            case FATAL:
                o << "F"; break;
            case ERROR:
                o << "E"; break;
            case WARNING:
                o << "W"; break;
            case INFO:
                o << "I"; break;
            case DEBUG:
                o << "D"; break;
            case NOISE:
                o << "N"; break;
            case RACKET:
                o << "R"; break;
            default:
                o << "UNKNOWN"; break;
        }
        return o;
    }
}  // namespace severity

GlobalSettings::GlobalSettings() { }

ostream* GlobalSettings::stream(const MessageMetaData& data) {
    ostream *o;
    if (data.ringbuf()) {
        WebStaXTraceRingbufStream *buf =
                reinterpret_cast<WebStaXTraceRingbufStream *>(
                    VTSS_MALLOC(sizeof(WebStaXTraceRingbufStream)));
        VTSS_ASSERT(buf);
        new(buf) WebStaXTraceRingbufStream();
        o = buf;

    } else {
        WebStaXTraceStream *buf =
                (WebStaXTraceStream *)(
                    VTSS_MALLOC(sizeof(WebStaXTraceStream)));
        VTSS_ASSERT(buf);
        new(buf) WebStaXTraceStream();
        o = buf;
    }

    // Sample all meta-data needed, and then print it.
    data.sample();
    (*o) << data;

    // TODO
    // if (l == VTSS_TRACE_LVL_ERROR) {
    //    (*o) << "Error: ";
    // }
    //
    // if (l == VTSS_TRACE_LVL_WARNING) {
    //    (*o) << "Error: ";
    // }

    return o;
}

void GlobalSettings::commit(ostream* os, const MessageMetaData &meta) {
    os->push('\n');
    os->~ostream();
    VTSS_FREE(os);
    // TODO, log to flash
}

}  // namespace trace
}  // namespace vtss


