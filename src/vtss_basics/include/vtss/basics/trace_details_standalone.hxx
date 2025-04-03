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

#ifndef __VTSS_BASICS_TRACE_DETAILS_STANDALONE_HXX__
#define __VTSS_BASICS_TRACE_DETAILS_STANDALONE_HXX__

#include "vtss/basics/trace_common.hxx"

namespace vtss {
namespace trace {

// A simple data container to hold all the meta data which belongs to a log
// message
class MessageMetaData {
  public:
    MessageMetaData(uint32_t line, const char *file, int module, int group,
                    severity::E level) : line_number_(line), file_name_(file),
                module_id_(module), group_idx_(group), level_(level) { }

    MessageMetaData(uint32_t line, const char *file, severity::E level) :
                line_number_(line), file_name_(file), module_id_(0),
                group_idx_(0), level_(level) { }

    uint32_t line_number() const { return line_number_; }
    const char *file_name() const { return file_name_; }
    int module_id() const { return module_id_; }
    int group_idx() const { return group_idx_; }
    severity::E level() const { return level_; }

    uint32_t pid() const { return pid_; }
    uint32_t tid() const { return tid_; }
    uint64_t time() const { return time_; }
    uint32_t time_ns() const { return time_ns_; }

    operator bool() const { return true; }

    void sample() const;  // will initiliaze pid, tid and time

  private:
    uint32_t line_number_;
    const char *file_name_;
    int module_id_;
    int group_idx_;
    severity::E level_;

    mutable uint32_t pid_;
    mutable uint32_t tid_;
    mutable uint64_t time_;
    mutable uint32_t time_ns_;
};
ostream& operator<<(ostream& o, const MessageMetaData &rhs);

// Filtering object. Before a message is being formated the filtering will be
// performed.
struct Filter {
    virtual bool enable(const MessageMetaData& meta) = 0;
};

// A log message may be delivered to one or more places. The sink controles
// where it is delivered.
struct Sink {
    virtual ostream* stream(const MessageMetaData& data) = 0;
    virtual void commit(ostream*, const MessageMetaData &meta) = 0;
};

// Kind of a trace hub which connects the sources and the sinks... Only one
// instance of this should exists.
// The object implements a default policy which is active until "someone"
// installs an alternative policy.
struct GlobalSettings {
    GlobalSettings();

    bool enable(const MessageMetaData& meta) {
        if (filter)
            return filter->enable(meta);
        return meta.level() <= default_severity_level;
    }

    ostream* stream(const MessageMetaData& data);
    void commit(ostream* os, const MessageMetaData &meta);

    Filter   *filter = 0;
    Sink     *sink   = 0;

  private:
    severity::E default_severity_level = severity::WARNING;
    fdstream stdout_;
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

#endif  // __VTSS_BASICS_TRACE_DETAILS_STANDALONE_HXX__
