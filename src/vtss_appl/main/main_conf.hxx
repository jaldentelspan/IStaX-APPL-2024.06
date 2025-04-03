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

#include <string>
#include <vtss/basics/api_types.h>
#include <vtss/basics/map.hxx>

#ifndef _VTSS_APPL_MAIN_CONF_H_
#define _VTSS_APPL_MAIN_CONF_H_

namespace vtss {
namespace appl {
namespace main {

struct ModuleConfValue {
    enum E {
        BOOL,
        STRING,
        I32,
        U32
    };

    E type;
    std::string s;
    union {
        bool b;
        i32  i;
        u32  u;
    } v;
};

struct ModuleConf {
    ModuleConf() = default;
    ModuleConf(ModuleConf &&) = default;
    ModuleConf(const ModuleConf &) = delete;
    ModuleConf &operator=(ModuleConf &&) = default;

    bool bool_get(const char *name, bool def) const;
    const std::string& str_get(const char *name, const std::string &def) const;
    i32 i32_get(const char *name, i32 def) const;
    u32 u32_get(const char *name, u32 def) const;

    Map<std::string, ModuleConfValue> data;
};

const ModuleConf &module_conf_get(const char *mname);
bool module_enabled(const char *mname);

} // namespace main
} // namespace appl
} // namespace vtss

#endif /* _VTSS_APPL_MAIN_CONF_H_ */
