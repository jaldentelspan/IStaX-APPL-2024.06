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

#ifndef __VTSS_BASICS_MEASURE_SCOPE_HXX__
#define __VTSS_BASICS_MEASURE_SCOPE_HXX__

#include <vtss/basics/time.hxx>

namespace vtss {

struct MeasureInfo {
    LinuxClock::duration m_total_time;
    size_t m_nr_calls;
};

class MeasureContainer {
  public:
    void append_time(LinuxClock::duration d, size_t id);
    MeasureInfo get_id(size_t id);
    void clear();

  private:
    MeasureInfo m_measurements[100];
};

MeasureContainer &get_measure_container();

class MeasureScope {
  public:
    MeasureScope(size_t id, MeasureContainer &container);
    ~MeasureScope();

  private:
    size_t m_id;
    LinuxClock::time_point m_time;
    MeasureContainer &m_container;
};
}

#endif