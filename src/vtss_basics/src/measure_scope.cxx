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

#include "vtss/basics/measure_scope.hxx"
#include "vtss/basics/mutex.hxx"
#include "vtss/basics/notifications/event.hxx"

namespace vtss {

//Critd measure_mutex("measure_container", 100);

void MeasureContainer::append_time(LinuxClock::duration d, size_t id) {
    //    ScopeLock<Critd> measure_lock(&measure_mutex, __FILE__, __LINE__);
    m_measurements[id].m_total_time += d;
    ++m_measurements[id].m_nr_calls;
}

MeasureInfo MeasureContainer::get_id(size_t id) {
    //    ScopeLock<Critd> measure_lock(&measure_mutex, __FILE__, __LINE__);
    MeasureInfo result = m_measurements[id];
    m_measurements[id] = MeasureInfo{LinuxClock::duration{}, 0};
    return result;
}

void MeasureContainer::clear() {
    //    ScopeLock<Critd> measure_lock(&measure_mutex, __FILE__, __LINE__);
    for (int i = 0; i < 100; ++i) {
        m_measurements[i] = MeasureInfo{LinuxClock::duration{}, 0};
    }
}

MeasureContainer &get_measure_container() {
    static MeasureContainer container;
    return container;
}

MeasureScope::MeasureScope(size_t id, MeasureContainer &container)
    : m_id{id}, m_time{LinuxClock::now()}, m_container{container} {}

MeasureScope::~MeasureScope() {
    auto end = LinuxClock::now();
    LinuxClock::duration d = end - m_time;
    m_container.append_time(d, m_id);
}
}
