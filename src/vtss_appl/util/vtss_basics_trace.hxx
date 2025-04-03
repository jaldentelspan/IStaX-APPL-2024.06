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

#ifndef __VTSS_APPL_UTIL_VTSS_BASICS_TRACE_HXX__
#define __VTSS_APPL_UTIL_VTSS_BASICS_TRACE_HXX__

#include "vtss_trace_lvl_api.h"
#include "vtss_trace_api.h"
#include "vtss/basics/trace_common.hxx"

namespace vtss {
namespace trace {

constexpr int severity_to_vtss_level(const severity::E l) {
    switch (l) {
        case severity::RACKET:
            return VTSS_TRACE_LVL_RACKET;

        case severity::NOISE:
            return VTSS_TRACE_LVL_NOISE;

        case severity::DEBUG:
            return VTSS_TRACE_LVL_DEBUG;

        case severity::INFO:
            return VTSS_TRACE_LVL_INFO;

        case severity::WARNING:
            return VTSS_TRACE_LVL_WARNING;

        case severity::ERROR:
        default:
            return VTSS_TRACE_LVL_ERROR;
    }
}

class MessageMetaData;
ostream& operator<<(ostream& o, const MessageMetaData &rhs);
class MessageMetaData {
  public:
    friend ostream& operator<<(ostream& o, const MessageMetaData &rhs);

    MessageMetaData(uint32_t line, const char *file, int module, int group,
                    severity::E level) : line_number_(line), file_name_(file),
                module_id_(module), group_idx_(group), level_(level) {
        trace_grp_ = vtss_trace_grp_get(module, group);
        trace_reg_ = vtss_trace_reg_get(module, group);
        webstax_level_ = severity_to_vtss_level(level);
    }

    MessageMetaData(uint32_t line, const char *file, severity::E level) :
                line_number_(line), file_name_(file), module_id_(0),
                group_idx_(0), level_(level), trace_grp_(0), trace_reg_(0) {
        webstax_level_ = severity_to_vtss_level(level);
    }

    uint32_t line_number() const { return line_number_; }
    const char *file_name() const { return file_name_; }
    const char *name() const {
        if (!trace_reg_)
            return 0;

        return trace_reg_->name;
    }
    const char *group_name() const {
        if (!trace_grp_)
            return 0;

        return trace_grp_->name;
    }
    int module_id() const { return module_id_; }
    int group_idx() const { return group_idx_; }
    severity::E level() const { return level_; }
    int webstax_level() const { return webstax_level_; }
    int walltime() const { return walltime_; }
    uint64_t useconds() const { return useconds_; }
    uint32_t thread_id() const { return thread_id_; }



    operator bool() const { return true; }

    bool ringbuf() const {
        if (!trace_grp_)
            return false;
#if defined(VTSS_TRACE_FLAGS_RINGBUF)
        return !!(trace_grp_->flags & VTSS_TRACE_FLAGS_RINGBUF);
#else
        return trace_grp_->ringbuf;
#endif
    }

    bool enable() const {
        // If the trace group does not exists not traces will be performed
        if (!trace_grp_)
            return false;

        // It should not be possible to suppress fatal errors
        if (level() == severity::FATAL)
            return true;

        // Take the global level into account
        if (webstax_level() < vtss_trace_global_lvl_get())
            return false;

        // TODO, pr. thread level

        // Look at the configured level
        if (webstax_level() < trace_grp_->lvl)
            return false;

        return true;
    }


    void sample() const;  // will initiliaze pid, tid and time

  private:
    uint32_t line_number_;
    const char *file_name_;
    int module_id_;
    int group_idx_;
    severity::E level_;

    mutable uint32_t thread_id_;

    mutable uint64_t useconds_;
    mutable int      walltime_;

    const vtss_trace_grp_t *trace_grp_;
    const vtss_trace_reg_t *trace_reg_;
    int webstax_level_;
};

struct GlobalSettings {
    GlobalSettings();

    bool enable(const MessageMetaData& meta) { return meta.enable(); }

    ostream* stream(const MessageMetaData& data);
    void commit(ostream* os, const MessageMetaData &meta);
};
extern GlobalSettings global_settings;

// A simple construction which uses RAII to push a trailing new line and call
// commit on the trace::global_settings object.
struct MessageTransaction {
    explicit MessageTransaction(const MessageMetaData &meta) :
                s_(global_settings.stream(meta)), meta_(meta) { }
    ostream *stream() { return s_; }
    ~MessageTransaction() { global_settings.commit(s_, meta_); }

  private:
    ostream *s_;
    const    MessageMetaData &meta_;
};

// Create a MessageTransaction object instance from a MessageMetaData. Read the
// macros below to see why this is needed.
inline MessageTransaction message_transaction(const MessageMetaData &meta) {
    return MessageTransaction(meta);
}

}  // namespace trace
}  // namespace vtss

#endif  // __VTSS_APPL_UTIL_VTSS_BASICS_TRACE_HXX__
