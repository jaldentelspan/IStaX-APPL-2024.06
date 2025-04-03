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

#include "main_conf.hxx"
#include "vtss/basics/fd.hxx"
#include "vtss/basics/json/stream-parser.hxx"

namespace vtss {
namespace appl {
namespace main {

Map<std::string, ModuleConf> conf;

bool ModuleConf::bool_get(const char *name, bool def) const
{
    auto itr = data.find(name);
    return (itr != data.end() && itr->second.type == ModuleConfValue::BOOL ? itr->second.v.b : def);
}

const std::string& ModuleConf::str_get(const char *name, const std::string &def) const
{
    auto itr = data.find(name);
    return (itr != data.end() && itr->second.type == ModuleConfValue::STRING ? itr->second.s : def);
}

i32 ModuleConf::i32_get(const char *name, i32 def) const
{
    auto itr = data.find(name);
    return (itr != data.end() && itr->second.type == ModuleConfValue::I32 ? itr->second.v.i : def);
}

u32 ModuleConf::u32_get(const char *name, u32 def) const
{
    auto itr = data.find(name);
    return (itr != data.end() && itr->second.type == ModuleConfValue::U32 ? itr->second.v.u : def);
}

struct ModuleConfStream : public json::StreamParserCallback {
    Action object_start() override {
        level++;
        return ACCEPT;
    }

    void object_end() override {
        level--;
        if (level == 1) {
            conf.set(mname, vtss::move(m));
        }
    }

    Action object_element_start(const std::string &s) override {
        if (level == 1) {
            mname = s;
            return ACCEPT;
        } else if (level == 2) {
            name = s;
            return ACCEPT;
        } else {
            return SKIP;
        }
    }

    void boolean(bool b) override {
        ModuleConfValue val;

        val.type = ModuleConfValue::BOOL;
        val.v.b = b;
        m.data.set(name, val);
    }

    void number_value(uint32_t u) override {
        ModuleConfValue val;

        val.type = ModuleConfValue::U32;
        val.v.u = u;
        m.data.set(name, val);
    }

    void number_value(int32_t i) override {
        ModuleConfValue val;

        val.type = ModuleConfValue::I32;
        val.v.u = i;
        m.data.set(name, val);
    }

    void string_value(const std::string &&s) override {
        ModuleConfValue val;

        val.type = ModuleConfValue::STRING;
        val.s = s;
        m.data.set(name, val);
    }

    std::string mname;
    std::string name;
    int level = 0;
    ModuleConf m;
};

#ifndef MAIN_CONF_FILE
#define MAIN_CONF_FILE "/etc/switch.conf"
#endif /* MAIN_CONF_FILE */ 

static ModuleConf mz;

const ModuleConf &module_conf_get(const char *mname)
{
    static bool read_file = true;
    
    if (read_file) {
        ModuleConfStream t;
        json::StreamParser s(&t);
        auto buf = read_file_into_buf(MAIN_CONF_FILE);

        s.process(buf.begin(), buf.end());
        read_file = false;
    }
    
#if 0
    for (auto i = conf.begin(); i != conf.end(); i++) {
        printf("mname: %s\n", i->first.c_str());
        auto m = &i->second;
        for (auto j = m->data.begin(); j != m->data.end(); j++) {
            printf("  name: %s\n", j->first.c_str());
        }
    }
#endif

    auto itr = conf.find(mname);
    return (itr == conf.end() ? mz : itr->second);
}

bool module_enabled(const char *mname)
{
    auto &c = vtss::appl::main::module_conf_get(mname);

    return c.bool_get("enable", true);
}

} // namespace main
} // namespace appl
} // namespace vtss
