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

#include "vtss/basics/map.hxx"
#include <vtss/basics/memory.hxx>
#include "vtss/basics/synchronized.hxx"
#include "vtss/basics/expose/json/specification/walk.hxx"
#include "vtss/basics/json-rpc-server.hxx"
#include "alarm-node.hxx"
#include "alarm-expose.hxx"
#include "alarm-trace.h"
#include "alarm_api.hxx"
#include "subject.hxx"
#include "vtss/basics/expose/json/method-split.hxx"
#ifndef VTSS_BASICS_STANDALONE
#include "json_rpc_api.hxx"
#endif
#include "alarm-expression/any.hxx"
#include <vtss/basics/trace.hxx>

using namespace vtss;

// TODO, please use the SYNCHRONIZED pattern to ensure that locking is done
// correct


//    appl::alarm::Node alarm_root_node(&main_thread, "alarm");

namespace vtss {
extern vtss::expose::json::RootNode JSON_RPC_ROOT;

#ifdef VTSS_BASICS_STANDALONE
std::shared_ptr<expose::json::specification::Inventory> JSON_RPC_ROOT_INVENTORY;
std::shared_ptr<expose::json::specification::Inventory>
json_rpc_inventory_get(bool specific) {
    if (JSON_RPC_ROOT_INVENTORY) {
        VTSS_TRACE(DEBUG) << "Returning cached inventory";
        return JSON_RPC_ROOT_INVENTORY;
    }

    VTSS_TRACE(DEBUG) << "No cache, building new inventory";
    auto inv = make_unique<expose::json::specification::Inventory>();
    for (auto &i : JSON_RPC_ROOT.leafs) {
        walk(*inv.get(), &i, specific);
    }

    VTSS_TRACE(DEBUG) << "New inventory ready";
    JSON_RPC_ROOT_INVENTORY = vtss::move(inv);

    return JSON_RPC_ROOT_INVENTORY;
}
#endif

}

namespace vtss {
namespace appl {
namespace alarm {

// TODO Change to SNMP friendly operator
bool operator<(const vtss::appl::alarm::DottedName &x,
               const vtss::appl::alarm::DottedName &y) {
    int x_len = x.length();
    int y_len = y.length();
    return (x_len < y_len) ||
           ((x_len == y_len) &&
            (static_cast<std::string>(x) < static_cast<std::string>(y)));
}

SynchronizedSubjectRunner<State> alarm_state(&subject_locked_thread,
                                             &subject_locked_thread);
TableStatus<expose::ParamKey<vtss_appl_alarm_name_t *>,
            expose::ParamVal<vtss_appl_alarm_status_t *>
> the_alarm_status("the_alarm_status", VTSS_MODULE_ID_ALARM);


#ifndef VTSS_BASICS_STANDALONE
mesa_rc vtss_alarm_init(void) {
    alarm_state.init(__LINE__, "Alarm");
    vtss::appl::alarm::alarm_name_any_init();
    return VTSS_RC_OK;
}
#endif

// ---------------------------------------------------------------------------
// start AnyAlarmName

struct AnyAlarmName final : public appl::alarm::expression::Any {
    AnyAlarmName(vtss_appl_alarm_name_t x) : value(x) {};
    static appl::alarm::expression::AnyPtr construct_from(const appl::alarm::expression::AnyJsonPrimitive &a);
    static str operator_result(appl::alarm::expression::Token t);
    str name_of_type() const override { return NAME_OF_TYPE; };
    appl::alarm::expression::AnySharedPtr opr(appl::alarm::expression::Token opr, appl::alarm::expression::AnySharedPtr res, appl::alarm::expression::AnySharedPtr rhs) const override;

    static const str NAME_OF_TYPE;
    vtss_appl_alarm_name_t value;
};

const str AnyAlarmName::NAME_OF_TYPE = str("vtss_appl_alarm_name_t");

////////////////////////////////////////////////////////////////////////////////

using namespace appl::alarm::expression;

AnyPtr AnyAlarmName::construct_from(const AnyJsonPrimitive &a) {
    if (a.type() == AnyJsonPrimitive::STR) {
        if (a.as_str().size() < ALARM_NAME_SIZE) {
          const char *b = a.as_str().begin();
          vtss_appl_alarm_name_t v;
          strncpy(v.alarm_name,b, a.as_str().size());
          v.alarm_name[a.as_str().size()]=0;
          return make_unique<AnyAlarmName>(v);
        }
    }
    return AnyPtr(nullptr);
}

str AnyAlarmName::operator_result(Token t) {
    switch (t) {
    case Token::equal:
    case Token::not_equal:
        return AnyBool::NAME_OF_TYPE;

    default:
        return str();
    }
}

AnySharedPtr AnyAlarmName::opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const {
    if (!rhs) {
        VTSS_TRACE(ERROR) << "No 'rhs'";
        return nullptr;
    }

    if (name_of_type() != rhs->name_of_type()) {
        VTSS_TRACE(ERROR) << "Invalid type: " << name_of_type()
                       << " != " << rhs->name_of_type();
        return nullptr;
    }

    auto rhs_ = std::static_pointer_cast<AnyAlarmName>(rhs);

    bool b;
    switch (opr) {
    case Token::equal:
        b = (strncmp(value.alarm_name,rhs_->value.alarm_name,ALARM_NAME_SIZE) == 0);
        VTSS_TRACE(DEBUG) << value << " == " << rhs_->value << " -> " << b;
        break;

    case Token::not_equal:
        b = (strncmp(value.alarm_name,rhs_->value.alarm_name,ALARM_NAME_SIZE) != 0);
        VTSS_TRACE(DEBUG) << value << " != " << rhs_->value << " -> " << b;
        break;

    default:
        // Can not be reached
        return AnySharedPtr(nullptr);
    }

    return make_unique<AnyBool>(b);
}

void alarm_name_any_init() {
    vtss::appl::alarm::expression::any_add_type<vtss::appl::alarm::AnyAlarmName>();
}
// end AnyAlarmName
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// start alarm source

struct StringEncode {
    StringEncode(const std::string &_s) : s(_s) {}
    StringEncode() {}
    void setval(std::string &_s) { s = _s; }
    std::string getval() { return s; }
    std::string s;
};


ostream &operator<<(ostream &o, StringEncode s) {
    for (auto c : s.s) {
        char ctrl = expose::json::eschape_char(c);

        if (expose::json::is_non_eschape_char(c)) {
            o.push(c);

        } else if (ctrl) {
            o.push('\\');
            o.push(ctrl);
        }
    }
    return o;
}


void JsonSpecSource::parse(const expose::json::specification::Inventory &i,
                           const std::string *semantic_name_type,
                           std::string &name, bool firstlevel) {
    for (const auto &t : i.types) {
        alarm_source_t as;
        // walk through Map of pair with
        // const vtss::Pair<std::string, typedescriptor>
        if (t.first == *semantic_name_type) {
            switch (t.second->type_class) {
            case expose::json::specification::TypeClass::EncodingSpecification: {
                as.name = name;
                as.name_type = *semantic_name_type;
                as.enum_values = "";
                alarm_sources.insert(alarm_sources.end(), as);
                return;
            } break;

            case expose::json::specification::TypeClass::Typedef: {
                // "Typedef";
                as.name = name;
                as.name_type = *semantic_name_type;
                as.enum_values = "";
                alarm_sources.insert(alarm_sources.end(), as);
                return;
            } break;

            case expose::json::specification::TypeClass::Struct: {
                auto s =
                        static_cast<expose::json::specification::TypeDescriptorStruct *>(
                                t.second.get());
                for (const auto &e : s->elements) {
                    StringStream tmp;
                    std::string str_name;
                    if (firstlevel)
                        tmp << name << "@" << e.name.c_str();
                    else
                        tmp << name << "." << e.name.c_str();

                    str_name = tmp.cstring();
                    parse(i, &e.type, str_name, false);
                }
                return;
            } break;

            case expose::json::specification::TypeClass::Enum: {
                StringStream tmp;
                auto s =
                        static_cast<expose::json::specification::TypeDescriptorEnum *>(
                                t.second.get());
                bool first = true;
                for (const auto &e : s->elements) {
                    if (!first) tmp << ",";
                    first = false;
                    tmp << "\"" << StringEncode(e.name) << "\"";
                }
                as.name = name;
                as.name_type = *semantic_name_type;
                as.enum_values = tmp.cstring();
                alarm_sources.insert(alarm_sources.end(), as);
                return;
            } break;
            }  // end switch (t.second->type_class)
        }      // end if (t.first == r.semantic_name_type)
    }          // end for (const auto &t : i.types)
}



bool JsonSpecSource::create(const expose::json::specification::Inventory &i) {
    alarm_sources.clear();
    for (const auto &m : i.methods) {
        StringStream tmp;
        if (!m.has_notification) continue;
        tmp << m.method_name.substr(0, m.method_name.size() - 4);
        if (m.params.size() != 0) {
            bool first = true;
            tmp << "[";
            for (const auto &e : m.params) {
                if (!first) tmp << ",";
                first = false;
                tmp << StringEncode(e.semantic_name_type);
            }
            tmp << "]";
        }
        std::string name = tmp.cstring();
        // const auto &r = m.results.begin();
        for (const auto &r : m.results) {
            parse(i, &r.semantic_name_type, name, true);
        }
    }
    return true;
}


bool JsonSpecSource::getnext(vtss_appl_alarm_source_t *as, int index) {
    if ((size_t)index >= alarm_sources.size()) return false;

    strncpy(as->alarm_name, alarm_sources[index].name.c_str(),
            ALARM_NAME_SIZE - 1);
    as->alarm_name[ALARM_NAME_SIZE - 1] = 0;

    strncpy(as->type, alarm_sources[index].name_type.c_str(),
            ALARM_SOURCE_TYPE_SIZE - 1);
    as->type[ALARM_SOURCE_TYPE_SIZE - 1] = 0;

    strncpy(as->enum_values, alarm_sources[index].enum_values.c_str(),
            ALARM_SOURCE_ENUM_VALUES_SIZE - 1);
    as->enum_values[ALARM_SOURCE_ENUM_VALUES_SIZE - 1] = 0;

    return true;
}

JsonSpecSource jsonspec_source;
// end alarm source
// ---------------------------------------------------------------------------
}  // namespace alarm
}  // namespace appl
}  // namespace vtss

bool operator<(const vtss_appl_alarm_name_t &x, const vtss_appl_alarm_name_t &y) {
    int x_len = strlen(x.alarm_name);
    int y_len = strlen(y.alarm_name);
    return (x_len < y_len ||
            (x_len == y_len &&
             strncmp(x.alarm_name, y.alarm_name, ALARM_NAME_SIZE) < 0));
}

bool operator!=(const vtss_appl_alarm_status_t &x,
                const vtss_appl_alarm_status_t &y) {
    return (x.suppressed != y.suppressed) || (x.active != y.active);
}

using namespace vtss;
using namespace appl;
using namespace alarm;
using namespace notifications;

extern "C" {
/**
 * \brief Iterate through all alarms.
 * \param in [IN]   Pointer to current alarm index. Provide a null pointer
 *                  to get the first alarm.
 * \param out [OUT] Next alarm index (relative to the value provided in
 *                  'in').
 * \return Error code. VTSS_RC_OK means that the value in out is valid,
 *                     VTSS_RC_ERROR means that no "next" alarm index exists
 *                     and the end has been reached.
 */
mesa_rc vtss_appl_alarm_conf_itr(const vtss_appl_alarm_name_t *const in,
                                 vtss_appl_alarm_name_t *const out) {
    if (in == nullptr) {
        SYNCHRONIZED(alarm_state) {
            if (alarm_state.the_alarms.begin() == alarm_state.the_alarms.end()) {
                return VTSS_RC_ERROR;
            }
            strncpy(out->alarm_name,
                    alarm_state.the_alarms.begin()->first.c_str(),
                    ALARM_NAME_SIZE - 1);
            out->alarm_name[ALARM_NAME_SIZE - 1] =
                    0;  // just to make sure it is 0 terminated
        }
        return VTSS_RC_OK;
    }

    SYNCHRONIZED(alarm_state) {
        auto i = alarm_state.the_alarms.greater_than(in->alarm_name);
        if (i != alarm_state.the_alarms.end()) {
            strncpy(out->alarm_name, i->first.c_str(), ALARM_NAME_SIZE - 1);
            out->alarm_name[ALARM_NAME_SIZE - 1] =
                    0;  // just to make sure it is 0 terminated
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

/* Alarm functions ----------------------------------------------------- */

/**
 * \brief Get configuration for a specific alarm
 * \param nm [IN] Name of the alarm
 * \param conf [OUT] The expression defining the alarm
 * \return Error code.
 */
mesa_rc vtss_appl_alarm_conf_get(const vtss_appl_alarm_name_t *const nm,
                                 vtss_appl_alarm_expression_t *expr) {
    if (!nm) return VTSS_RC_ERROR;
    SYNCHRONIZED(alarm_state) {
        auto i = alarm_state.the_alarms.find(nm->alarm_name);
        if (i == alarm_state.the_alarms.end()) {
            return VTSS_RC_ERROR_ALARM_ENTRY_NOT_FOUND;
        }
        if (expr) *expr = i->second.exp;
    }
    return VTSS_RC_OK;
}

/**
 * \brief Set configuration for a specific alarm
 * \param nm [IN] Name of the alarm
 * \param conf [OUT] The expression defining the alarm
 * \return Error code.
 */
mesa_rc vtss_appl_alarm_conf_add(const vtss_appl_alarm_name_t *const nm,
                                 const vtss_appl_alarm_expression_t *const conf) {
    // TODO, the alarm is not created
    if (!nm || !conf) return VTSS_RC_ERROR;

    if (vtss_appl_alarm_conf_get(nm, nullptr) == VTSS_RC_OK) {
        DEFAULT(NOISE) << "The entry " << nm->alarm_name
                       << " is already created";
        return VTSS_RC_ERROR;
    }

    auto inv = json_rpc_inventory_get(true);
    vtss::Pair<vtss::MapIterator<vtss::Pair<const DottedName, AlarmConfiguration> >, bool> res;
    SYNCHRONIZED(alarm_state) {
        DEFAULT(DEBUG) << "Creating " << *nm << " : " << *conf;
        auto rc = alarm_state.alarm_root.make_alarm(
                &subject_locked_thread, "alarm", std::string(nm->alarm_name),
                std::string(conf->alarm_expression), *inv.get(), JSON_RPC_ROOT);
        if (VTSS_RC_OK != rc) {
            return rc;
        }
        DEFAULT(NOISE) << "Create " << nm->alarm_name;
        AlarmConfiguration c(conf, nullptr);
        res = alarm_state.the_alarms.emplace(nm->alarm_name, c);
    }
    return res.second ? VTSS_RC_OK : VTSS_RC_ERROR;
}

/**
 * \brief Delete configuration for a specific alarm
 * \param nm [IN] Name of the alarm
 * \return Error code.
 */
mesa_rc vtss_appl_alarm_conf_del(const vtss_appl_alarm_name_t *const nm) {
    SYNCHRONIZED(alarm_state) {
        auto i = alarm_state.the_alarms.find(nm->alarm_name);
        if (i == alarm_state.the_alarms.end()) {
            DEFAULT(NOISE) << "Did not find " << nm->alarm_name;
            return VTSS_RC_ERROR_ALARM_ENTRY_NOT_FOUND;
        }
        alarm_state.alarm_root.delete_alarm(nm->alarm_name);
        alarm_state.the_alarms.erase(i);
    }
    return VTSS_RC_OK;
}

/**
 * \brief Iterate through all alarm status.
 * \param in [IN]   Pointer to current alarm index. Provide a null pointer
 *                  to get the first alarm node or leaf.
 * \param out [OUT] Next alarm index (relative to the value provided in
 *                  'in').
 * \return Error code. VTSS_RC_OK means that the value in out is valid,
 *                     VTSS_RC_ERROR means that no "next" alarm index exists
 *                     and the end has been reached.
 */
mesa_rc vtss_appl_alarm_status_itr(const vtss_appl_alarm_name_t *const in,
                                   vtss_appl_alarm_name_t *const out) {
    vtss_appl_alarm_status_t s;
    if (in) {
        strncpy(out->alarm_name, in->alarm_name, ALARM_NAME_SIZE);
        return the_alarm_status.get_next(out, &s);
    } else {
        return the_alarm_status.get_first(out, &s);
    }
}
/**
 * \brief Get suppression configuration for a specific alarm node or leaf
 * \param nm [IN] Name of the alarm node or leaf
 * \param supp [OUT] The current configuration of suppression
 * \return Error code.
 */
mesa_rc vtss_appl_alarm_suppress_get(const vtss_appl_alarm_name_t *const nm,
                                     vtss_appl_alarm_suppression_t *const supp) {
    if (!nm) return VTSS_RC_ERROR;
    SYNCHRONIZED(alarm_state) {
        auto p_elem = alarm_state.alarm_root.lookup(nm->alarm_name);
        if (p_elem) {
            if (supp) supp->suppress = p_elem->suppressed;
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR_ALARM_ENTRY_NOT_FOUND;
}

/**
 * \brief Set suppression configuration for a specific alarm node or leaf
 * \param nm [IN] Name of the alarm node or leaf
 * \param supp [IN] The configuration of suppression
 * \return Error code.
 */
mesa_rc vtss_appl_alarm_suppress_set(
        const vtss_appl_alarm_name_t *const nm,
        const vtss_appl_alarm_suppression_t *const supp) {
    mesa_rc rc;
    if (!nm) return VTSS_RC_ERROR;
    if (!supp) return VTSS_RC_ERROR;

    SYNCHRONIZED(alarm_state) {
        rc = alarm_state.alarm_root.suppress(nm->alarm_name, supp->suppress);
    }
    return rc;
}


/**
 * \brief Get configuration for a specific alarm
 * \param nm [IN] Name of the alarm
 * \param conf [OUT] The expression defining the alarm
 * \return Error code.
 */
mesa_rc vtss_appl_alarm_status_get(const vtss_appl_alarm_name_t *const nm,
                                   vtss_appl_alarm_status_t *status) {
    if (!nm) return VTSS_RC_ERROR;
    SYNCHRONIZED(alarm_state) {
        auto p_elem = alarm_state.alarm_root.lookup(nm->alarm_name);
        if (p_elem) {
            if (status) {
                status->active = p_elem->get();
                status->suppressed = p_elem->suppressed;
                status->exposed_active = status->active && (!status->suppressed);
            }
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR_ALARM_ENTRY_NOT_FOUND;
}

/**
 * \brief Iterate through all alarm_sources.
 * \param out [OUT] nxt alarm source, where user shall provide a char[256] to
 * copy to.
 * \return Error code. VTSS_RC_OK means that the value in out is valid,
 *                     VTSS_RC_ERROR means that no "next" alarm source exists
 *                     and the end has been reached.
 */

mesa_rc vtss_appl_alarm_sources_itr(const int *const in, int *const out) {
    auto inv = json_rpc_inventory_get(true);
    if (jsonspec_source.create(*inv.get()) == false) return VTSS_RC_ERROR;
    if (in == (const int *)0) {
        if (jsonspec_source.alarm_sources.size() == 0) return VTSS_RC_ERROR;
        *out = 0;
        return VTSS_RC_OK;
    } else {
        if (*out < 0) return VTSS_RC_ERROR;
        if (jsonspec_source.alarm_sources.size() <= (size_t)(1 + *out))
            return VTSS_RC_ERROR;
        *out = 1 + *out;
        return VTSS_RC_OK;
    }
}


mesa_rc vtss_appl_alarm_sources_get(const int *const in,
                                    vtss_appl_alarm_source_t *as) {
    if (jsonspec_source.getnext(as, *in)) {
        return VTSS_RC_OK;
    } else
        return VTSS_RC_ERROR;
}
}
