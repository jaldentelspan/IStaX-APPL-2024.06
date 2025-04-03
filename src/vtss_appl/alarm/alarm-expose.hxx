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

#ifndef __ALARM_EXPOSE_H__
#define __ALARM_EXPOSE_H__

#include <vtss/basics/stream.hxx>
#include "vtss/appl/alarm.h"
#include "vtss/basics/map.hxx"
#include "vtss/basics/expose/json/table-read-only-notification.hxx"
#include "alarm-node.hxx"

#include <vtss/basics/parser_impl.hxx>
#include "vtss/basics/expose/json/char-encode.hxx"
#include "vtss/basics/expose/json/specification/indent.hxx"
#include "vtss/basics/expose/json/specification/inventory.hxx"
#include "vtss/basics/expose/json/json-core-type.hxx"
#include "vtss/basics/expose/json/specification/param-descriptor.hxx"
#include "vtss/basics/expose/json/specification/type-descriptor.hxx"
#include "vtss/basics/expose/json/specification/type-descriptor-encoding.hxx"
#include "vtss/basics/expose/json/specification/type-descriptor-typedef.hxx"
#include "vtss/basics/expose/json/specification/type-descriptor-struct.hxx"
#include "vtss/basics/expose/json/specification/type-descriptor-enum.hxx"

extern "C" void vtss_appl_alarm_json_init();
extern "C" void alarm_mib_init();

using namespace vtss::expose;
using namespace vtss::notifications;

bool operator<(const vtss_appl_alarm_name_t &x, const vtss_appl_alarm_name_t &y);
bool operator!=(const vtss_appl_alarm_status_t &x,
                const vtss_appl_alarm_status_t &y);


vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_alarm_status_t &v);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_alarm_name_t &v);
vtss::ostream &operator<<(vtss::ostream &o,
                          const vtss_appl_alarm_expression_t &v);

namespace vtss {
namespace appl {
namespace alarm {

// This is okay, but I would prefere to use std::string internally, and only use
// these char[] types in the c-interface.
struct DottedName : public std::string {
    DottedName(std::string s) : std::string(s) {}
    DottedName(const char *s) : std::string(s) {}
};


bool operator<(const vtss::appl::alarm::DottedName &x,
               const vtss::appl::alarm::DottedName &y);

struct AlarmConfiguration {
    AlarmConfiguration(const vtss_appl_alarm_expression_t *p_e, Element *p_elem)
        : exp(*p_e), p_element(p_elem) {}
    AlarmConfiguration() : exp({}), p_element(nullptr) {}
    ~AlarmConfiguration() {
        if (p_element) delete p_element;
    }

    vtss_appl_alarm_expression_t exp;
    Element *p_element;
};

struct State {
    State(notifications::SubjectRunner *runner)
        : alarm_root(runner,
                     std::string("alarm")) {}
    vtss::Map<DottedName, AlarmConfiguration> the_alarms;
    appl::alarm::Node alarm_root;
};

extern TableStatus<expose::ParamKey<vtss_appl_alarm_name_t *>,
                   expose::ParamVal<vtss_appl_alarm_status_t *>> the_alarm_status;

mesa_rc vtss_alarm_init(void);
void alarm_name_any_init(void);

typedef struct {
    std::string name;
    std::string name_type;
    std::string enum_values;
} alarm_source_t;

struct JsonSpecSource {
    JsonSpecSource() {}

    bool getnext(vtss_appl_alarm_source_t *as, int index);
    bool create(const expose::json::specification::Inventory &i);
    Vector<alarm_source_t> alarm_sources;

  private:
    void parse(const expose::json::specification::Inventory &i,
               const std::string *semantic_name_type, std::string &name,
               bool firstlevel);
};
}  // namespace alarm
}  // namespace appl
}  // namespace vtss


#endif /* !defined(__ALARM_EXPOSE_H__) */
