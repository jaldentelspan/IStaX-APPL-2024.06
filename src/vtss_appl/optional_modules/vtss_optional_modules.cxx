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

#include <dlfcn.h>
#include "main_conf.hxx"
#include "vtss_optional_modules.hxx"
#include <vtss/basics/trace.hxx>

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_OPTIONAL_MODULES
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_OPTIONAL_MODULES
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_OPTIONAL_MODULES

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "optmod", "Optional Modules"
};

static vtss_trace_grp_t trace_grps[] = {
    {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

static VtssOptionalModule *MODULE_HEAD;

#if 0
bool vtss_has_module(const char *path) {
    bool present = access(path, X_OK) == 0;
    VTSS_TRACE(DEBUG) << "Module: " << path << " present: " << present;
    return present;
}

static void insert_after(VtssOptionalModule *prev, VtssOptionalModule *m) {
    if (prev) {
        // Insert in chain
        m->module_next = prev->module_next;
        prev->module_next = m;
    } else {
        // Insert at head
        m->module_next = MODULE_HEAD; // module_head may or may not be NULL
        MODULE_HEAD = m;
    }
}

static void insert(VtssOptionalModule *m) {
    VtssOptionalModule *i = MODULE_HEAD;
    VtssOptionalModule *prev = 0;

    while (i) {
        if (m->init_priority < i->init_priority) {
            insert_after(prev, m);
            return;
        }

        prev = i;
        i = i->module_next;
    }

    insert_after(prev, m);
}

static void load_single(const char *path, void **ref) {
    VTSS_TRACE(DEBUG) << "Loading module: " << path;
    if (!vtss_has_module(path)) {
        VTSS_TRACE(DEBUG) << "Disabling: " << path << " not an executable";
        return;
    }

    void *h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        VTSS_TRACE(ERROR) << "Failed to load: " << path << " error: "
                << dlerror();
        return;
    }

    auto create_ptr = (void *(*)()) dlsym(h, "create");

    if (create_ptr) {
        auto *res = (VtssOptionalModule *)create_ptr();
        *ref = res->module_api;
        insert(res);
        VTSS_TRACE(DEBUG) << "Module fully loaded: " << path;
    } else {
        VTSS_TRACE(ERROR) << "Faild to resolve create symbol in : " << path
                          << " error: " << dlerror();
        return;
    }
}
#endif

static void load_all() {
}

static void init_all(vtss_init_data_t *data) {
    VtssOptionalModule *i = MODULE_HEAD;

    while (i) {
        VTSS_TRACE(DEBUG) << "Init cmd: " << data->cmd << " Module: "
                          << i->name;
        i->init(data);
        i = i->module_next;
    }
}

mesa_rc vtss_optional_modules_init(vtss_init_data_t *data)
{
    if (data->cmd == INIT_CMD_INIT) {
        load_all();
    }

    switch (data->cmd) {
    case INIT_CMD_INIT:
    case INIT_CMD_START:
    case INIT_CMD_CONF_DEF:
    case INIT_CMD_ICFG_LOADING_PRE:
    case INIT_CMD_ICFG_LOADING_POST:
    case INIT_CMD_SUSPEND_RESUME:
    case INIT_CMD_WARMSTART_QUERY:
        init_all(data);
        break;

    default:
        break;
    }

    return MESA_RC_OK;
}

