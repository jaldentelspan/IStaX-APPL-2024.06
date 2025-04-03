/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <mutex>

#include "vtss/basics/trace_details_standalone.hxx"

namespace vtss {
namespace trace {

void MessageMetaData::sample() const {
    timespec tp;
    int res = clock_gettime(CLOCK_REALTIME, &tp);

    if (res != 0) memset(&tp, 0, sizeof(tp));

    time_ = tp.tv_sec;
    time_ns_ = tp.tv_nsec;
    pid_ = getpid();
    tid_ = (uint32_t)pthread_self();
}

GlobalSettings global_settings;

// static const char *severity_vector = "FEWIDNR";
// 30 Black
// 31 Red
// 32 Green
// 33 Yellow
// 34 Blue
// 35 Magenta
// 36 Cyan
// 37 White

// 0 Reset all attributes
// 1 Bright
// 2 Dim
// 4 Underscore
// 5 Blink
// 7 Reverse
// 8 Hidden
//
// Foreground Colors:
//
// 30 Black
// 31 Red
// 32 Green
// 33 Yellow
// 34 Blue
// 35 Magenta
// 36 Cyan
// 37 White
//
// Background Colors:
//
// 40 Black
// 41 Red
// 42 Green
// 43 Yellow
// 44 Blue
// 45 Magenta
// 46 Cyan
// 47 White

#define VTSS_TERM_ATTR_BRIGHT      "\033[1m"
#define VTSS_TERM_ATTR_DIM         "\033[2m"
#define VTSS_TERM_ATTR_UNDERSCORE  "\033[4m"
#define VTSS_TERM_ATTR_BLINK       "\033[5m"
#define VTSS_TERM_ATTR_REVERSE     "\033[7m"
#define VTSS_TERM_ATTR_HIDDEN      "\033[8m"

#define VTSS_TERM_FG_BLACK    "\033[30m"
#define VTSS_TERM_FG_RED      "\033[31m"
#define VTSS_TERM_FG_GREEN    "\033[32m"
#define VTSS_TERM_FG_YELLOW   "\033[33m"
#define VTSS_TERM_FG_BLUE     "\033[34m"
#define VTSS_TERM_FG_MAGENTA  "\033[35m"
#define VTSS_TERM_FG_CYAN     "\033[36m"
#define VTSS_TERM_FG_WHITE    "\033[37m"

#define VTSS_TERM_BG_BLACK    "\033[40m"
#define VTSS_TERM_BG_RED      "\033[41m"
#define VTSS_TERM_BG_GREEN    "\033[42m"
#define VTSS_TERM_BG_YELLOW   "\033[43m"
#define VTSS_TERM_BG_BLUE     "\033[44m"
#define VTSS_TERM_BG_MAGENTA  "\033[45m"
#define VTSS_TERM_BG_CYAN     "\033[46m"
#define VTSS_TERM_BG_WHITE    "\033[47m"

#define VTSS_TERM_DEFAULT     "\033[0m"

namespace severity {
    ostream& operator<<(ostream& o, const E &rhs) {
        switch (rhs) {
            case FATAL:
                o << VTSS_TERM_FG_BLACK VTSS_TERM_BG_RED VTSS_TERM_ATTR_BLINK
                    "F" VTSS_TERM_DEFAULT; break;
            case ERROR:
                o << VTSS_TERM_FG_BLACK VTSS_TERM_BG_RED VTSS_TERM_ATTR_BRIGHT
                    "E" VTSS_TERM_DEFAULT; break;
            case WARNING:
                o << VTSS_TERM_BG_YELLOW VTSS_TERM_FG_BLACK "W"
                    VTSS_TERM_DEFAULT; break;
            case INFO:
                o << VTSS_TERM_BG_GREEN VTSS_TERM_FG_BLACK "I"
                    VTSS_TERM_DEFAULT; break;
            case DEBUG:
                o << VTSS_TERM_BG_CYAN VTSS_TERM_FG_BLACK "D"
                    VTSS_TERM_DEFAULT; break;
            case NOISE:
                o << VTSS_TERM_BG_BLACK VTSS_TERM_BG_CYAN "N"
                    VTSS_TERM_DEFAULT; break;
            case RACKET:
                o << VTSS_TERM_BG_BLACK VTSS_TERM_FG_MAGENTA "R"
                    VTSS_TERM_DEFAULT; break;
            default:
                o << VTSS_TERM_FG_BLACK VTSS_TERM_BG_RED "UNKNOWN"
                    VTSS_TERM_DEFAULT; break;
        }
        return o;
    }
}  // namespace severity

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
    // W trace 08:37:51 main:38: Warning: T_W Test: debug level is now 8
    o << rhs.level();

    {  // Format time stamp
        char buf[80];
        struct tm time_values;
        time_t timepoint = rhs.time();
        localtime_r(&timepoint, &time_values);
        strftime(buf, sizeof(buf), "%m-%d %H:%M:%S", &time_values);
        uint32_t us = rhs.time_ns() / 1000;
        o << " " << buf << ":" << us;
    }

    {  // Location
        o << " " << trace_sub_path(str(rhs.file_name())) <<
            ":" << rhs.line_number();
    }

    return o;
}

static std::mutex default_trace_stream_mutex;

GlobalSettings::GlobalSettings() {
    const char *val = ::getenv("VTSS_TRACE_LEVEL");
    if (val) {
        if (strcmp("FATAL", val) == 0) {
            default_severity_level = severity::FATAL;
            return;
        }

        if (strcmp("ERROR", val) == 0) {
            default_severity_level = severity::ERROR;
            return;
        }

        if (strcmp("WARNING", val) == 0) {
            default_severity_level = severity::WARNING;
            return;
        }

        if (strcmp("INFO", val) == 0) {
            default_severity_level = severity::INFO;
            return;
        }

        if (strcmp("DEBUG", val) == 0) {
            default_severity_level = severity::DEBUG;
            return;
        }

        if (strcmp("NOISE", val) == 0) {
            default_severity_level = severity::NOISE;
            return;
        }

        if (strcmp("RACKET", val) == 0) {
            default_severity_level = severity::RACKET;
            return;
        }
    }
}

ostream* GlobalSettings::stream(const MessageMetaData& data) {
    data.sample();
    if (sink) return sink->stream(data);

    default_trace_stream_mutex.lock();
    stdout_.fd(0);
    stdout_ << data << ": ";
    return &stdout_;
}

void GlobalSettings::commit(ostream* os, const MessageMetaData &meta) {
    if (sink) return sink->commit(os, meta);

    stdout_.push('\n');
    default_trace_stream_mutex.unlock();
    // TODO(anielsen) fix raise if the sink is changed while we are logging!
}




}  // namespace trace
}  // namespace vtss


