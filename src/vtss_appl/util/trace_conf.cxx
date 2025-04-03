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

#include "vtss_trace.h"
#include "vtss_trace_api.h"
#include "vtss/basics/map.hxx"
#include "vtss/basics/fd.hxx"
#include "vtss/basics/json/stream-parser.hxx"

#define TRACE_CONF_DEFAULT_LVL  VTSS_TRACE_LVL_ERROR
#define TRACE_CONF_DEFAULT_USEC false
#define TRACE_CONF_DEFAULT_RB   false

// Where the trace configuration is written to. It is *no longer* written into
// 'conf', but into this file. We maintain the binary layout for now.
#define TRACE_CONF_FILE_PATH VTSS_FS_FLASH_DIR "trace-conf"

/******************************************************************************/
// trace_conf_key_t
/******************************************************************************/
typedef struct {
    vtss_module_id_t module_id; // Module ID.        Mandatory.
    std::string      grp_name;  // Trace group name. Mandatory.
} trace_conf_key_t;

/******************************************************************************/
// trace_conf_val_t
/******************************************************************************/
typedef struct {
    int  lvl;  // Trace level.  Optional. Defaults to TRACE_CONF_DEFAULT_LVL
    bool usec; // Microseconds. Optional. Defaults to TRACE_CONF_DEFAULT_USEC
    bool rb;   // Ring buffer.  Optional. Defaults to TRACE_CONF_DEFAULT_RB
} trace_conf_val_t;

/******************************************************************************/
// trace_conf
// We need to have this object constructed before it's being used the first
// time (which is when the first module registers its trace settings by the use
// of the VTSS_TRACE_REGISTER() macro), so the value used as init_priority here
// must be lower than that used in the VTSS_TRACE_REGISTER() macro.
/******************************************************************************/
static vtss::Map<trace_conf_key_t, trace_conf_val_t> __attribute__((init_priority (500))) trace_conf;

/******************************************************************************/
// trace_conf_key_t::operator<
/******************************************************************************/
static bool operator<(const trace_conf_key_t &lhs, const trace_conf_key_t &rhs)
{
    // First sort by module_id
    if (lhs.module_id != rhs.module_id) {
        return lhs.module_id < rhs.module_id;
    }

    // Then by name
    return lhs.grp_name < rhs.grp_name;
}

/******************************************************************************/
// TraceConfStream class.
// Called back during parsing of trace-conf JSON file. Stores fields in
// trace_conf map.
// The trace-conf file and the corresponding callbacks look like:
//
// {                             => object_start (new level 1)
//   "trace_conf": [             => object_element_start("trace_conf"), array_start
//     {                         => object_start (new level 2)
//       "module_id": 1,         => object_element_start("module_id"), number_value(u == 1), object_element_end
//       "name":      "api_cil", => object_element_start("name"), string_value("api_cil"), object_element_end
//       "grps": [               => object_element_start("grps"), array_start
//         {                     => object_start(new level 3)
//           "name":  "default", => object_element_start("name"), string_value("default"), object_element_end
//           "lvl":   "error",   => object_element_start("lvl"), string_value("error"), object_element_end
//           "rb":    true,      => object_element_start("rb"), boolean(b == true), object_element_end
//           "usec":  false      => object_element_start("usec"), boolean(b == false), object_element_end
//         },                    => object_end(new level 2)
//         ...
//         {
//           "name":  "clock",
//           "lvl":   "error",
//           "rb":    true,
//           "usec":  false
//         }                     => object_end(new level 2)
//       ]                       => array_end
//     },                        => object_element_end, object_end(new level 1)
//     {                         => object_start(new level 2)
/******************************************************************************/
struct TraceConfStream : public vtss::json::StreamParserCallback {
    TraceConfStream() : module_id_seen(false), error(false)
    {
        reset();
    }

    void reset()
    {
        grp_name_seen = false;
        val.lvl       = TRACE_CONF_DEFAULT_LVL;
        val.usec      = TRACE_CONF_DEFAULT_USEC;
        val.rb        = TRACE_CONF_DEFAULT_RB;
    }

    Action array_start() override
    {
        // Must be overridden, because the default implementation returns SKIP.
        return ACCEPT;
    }

    void array_end() override
    {
        if (level == 2) {
            // We expect a new module ID to come
            module_id_seen = false;
        }
    }

    Action object_start() override
    {
        level++;
        return ACCEPT;
    }

    void object_end() override
    {
        level--;
        if (level == 2) {
            if (module_id_seen && grp_name_seen) {
                T_D("Setting [%d, %s] => [%d, %d, %d]", key.module_id, key.grp_name.c_str(), val.lvl, val.rb, val.usec);
                trace_conf.set(key, val);
            } else {
                error = true;
                T_I("Missing either of %d or %d", module_id_seen, grp_name_seen);
            }

            reset();
        }
    }

    Action object_element_start(const std::string &s) override
    {
        cur_key = s;
        return ACCEPT;
    }

    void boolean(bool b) override
    {
        if (level != 3) {
            return;
        }

        if (cur_key == "usec") {
            val.usec = b;
        } else if (cur_key == "rb") {
            val.rb = b;
        }
    }

    void number_value(uint32_t u) override
    {
        if (level == 2 && cur_key == "module_id") {
            key.module_id = u;
            module_id_seen = true;
        } else if (level == 3 && cur_key == "rb") {
            val.rb = u;
        }
    }

    void string_value(const std::string &&s) override
    {
        if (level != 3) {
            // No strings we are interested in for levels other than 3.
            return;
        }

        if (cur_key == "name") {
            key.grp_name = s;
            grp_name_seen = true;
        } else if (cur_key == "lvl") {
            if (vtss_trace_lvl_to_val(s.c_str(), &val.lvl) != MESA_RC_OK) {
                error = true;
                T_D("Unknown level (%s)", s.c_str());
            }
        }
    }

    bool module_id_seen;
    bool grp_name_seen;
    bool error;

    std::string cur_key;
    trace_conf_key_t key;
    trace_conf_val_t val;
    int level = 0;
};

/******************************************************************************/
// trace_conf_load()
/******************************************************************************/
mesa_rc trace_conf_load(void)
{
    trace_conf.clear();
    vtss::FixedBuffer buf = vtss::read_file_into_buf(TRACE_CONF_FILE_PATH);
    TraceConfStream t;
    vtss::json::StreamParser s(&t);
    s.process(buf.begin(), buf.end());

    if (t.error) {
        // User gotta know that he should enable debug trace in order to get
        // something more useful out.
        trace_conf.clear();
        return MESA_RC_ERROR;
    }

    return MESA_RC_OK;
}

/******************************************************************************/
// trace_conf_save()
/******************************************************************************/
mesa_rc trace_conf_save(vtss_trace_reg_t **trace_regs, size_t size)
{
    int     module_id, grp_idx;
    BOOL    first_module = TRUE;
    FILE    *fp;
    mesa_rc rc = MESA_RC_OK;

    if ((fp = fopen(TRACE_CONF_FILE_PATH, "w")) == NULL) {
        T_E("Unable to open %s for writing", TRACE_CONF_FILE_PATH);
        return MESA_RC_ERROR;
    }

#define TRACE_WR(...)                                           \
    do {                                                        \
        if (fprintf(fp, ##__VA_ARGS__) <= 0) {                  \
            T_E("Unable to write to %s", TRACE_CONF_FILE_PATH); \
            rc = MESA_RC_ERROR;                                 \
            goto do_exit;                                       \
        }                                                       \
    } while (0)

    TRACE_WR("{\n");

    // Comments are not supported in JSON, so we need to make objects instead.
    TRACE_WR("  \"comment\": \"Optional keys per group:\",\n");
    TRACE_WR("  \"comment\":   \"'lvl'  (defaults to '%s')\",\n", vtss_trace_lvl_to_str(TRACE_CONF_DEFAULT_LVL));
    TRACE_WR("  \"comment\":   \"'usec' (defaults to %s)\",\n",   TRACE_CONF_DEFAULT_USEC ? "true" : "false");
    TRACE_WR("  \"comment\":   \"'rb'   (defaults to %s)\",\n",   TRACE_CONF_DEFAULT_RB   ? "true" : "false");
    TRACE_WR("  \"trace_conf\": [\n");

    for (module_id = 0; module_id < size; module_id++) {
        vtss_trace_reg_t *trace_reg = trace_regs[module_id];

        if (trace_reg == NULL) {
            continue;
        }

        TRACE_WR("%s    {\n",                      first_module ? "" : ",\n");
        TRACE_WR("      \"module_id\": %d,\n",     module_id);
        TRACE_WR("      \"name\":      \"%s\",\n", vtss_module_names[module_id]);
        TRACE_WR("      \"grps\": [\n");
        first_module = FALSE;

        for (grp_idx = 0; grp_idx < trace_reg->grp_cnt; grp_idx++) {
            vtss_trace_grp_t *grp = &trace_reg->grps[grp_idx];

            TRACE_WR("        {\n");
            TRACE_WR("          \"name\": \"%s\"", grp->name);

            if (grp->lvl != TRACE_CONF_DEFAULT_LVL) {
                TRACE_WR(",\n          \"lvl\":  \"%s\"", vtss_trace_lvl_to_str(grp->lvl));
            }

            if (HAS_FLAGS(grp, VTSS_TRACE_FLAGS_USEC) != TRACE_CONF_DEFAULT_USEC) {
                TRACE_WR(",\n          \"usec\": %s", TRACE_CONF_DEFAULT_USEC ? "false" : "true");
            }

            if (HAS_FLAGS(grp, VTSS_TRACE_FLAGS_RINGBUF) != TRACE_CONF_DEFAULT_RB) {
                TRACE_WR(",\n          \"rb\":   %s", TRACE_CONF_DEFAULT_RB ? "false" : "true");
            }

            TRACE_WR("\n        }%s\n", grp_idx == trace_reg->grp_cnt - 1 ? "" : ",");
        }

        TRACE_WR("      ]\n");
        TRACE_WR("    }");
    }

    TRACE_WR("\n  ]\n}\n");

#undef TRACE_WR

do_exit:
    if (fp) {
        (void)fclose(fp);
    }

    return rc;
}

/******************************************************************************/
// trace_conf_erase()
/******************************************************************************/
mesa_rc trace_conf_erase(void)
{
    (void)unlink(TRACE_CONF_FILE_PATH);
    trace_conf.clear();
    return VTSS_RC_OK;
}

/******************************************************************************/
// trace_conf_apply()
/******************************************************************************/
mesa_rc trace_conf_apply(vtss_trace_reg_t **trace_regs, size_t size, vtss_module_id_t module_id)
{
    int module_id_start, module_id_end;

    if (module_id == -1) {
        // Iterate over all modules
        module_id_start = 0;
        module_id_end   = size - 1;
    } else {
        module_id_start = module_id;
        module_id_end   = module_id;
    }

    for (module_id = module_id_start; module_id <= module_id_end; module_id++) {
        vtss_trace_reg_t *trace_reg = trace_regs[module_id];
        int grp_idx;

        if (trace_reg == NULL) {
            continue;
        }

        for (grp_idx = 0; grp_idx < trace_reg->grp_cnt; grp_idx++) {
            vtss_trace_grp_t *grp = &trace_reg->grps[grp_idx];
            trace_conf_key_t key;

            key.module_id = module_id;
            key.grp_name  = grp->name;

            auto itr = trace_conf.find(key);

            if (itr != trace_conf.end()) {
                u32 old_flags;

                if (grp->lvl != itr->second.lvl) {
                    T_I("Changing %s.%s.lvl from %s to %s", vtss_module_names[module_id], grp->name, grp->lvl, itr->second.lvl);
                }

                grp->lvl = itr->second.lvl;

                old_flags = grp->flags;

                if (itr->second.usec) {
                    grp->flags |= VTSS_TRACE_FLAGS_USEC;
                } else {
                    grp->flags &= ~VTSS_TRACE_FLAGS_USEC;
                }

                if (old_flags != grp->flags) {
                    T_I("Changing %s.%s.usec from %d to %d", vtss_module_names[module_id], grp->name, !itr->second.usec, itr->second.usec);
                }

                old_flags = grp->flags;

                if (itr->second.rb) {
                    grp->flags |= VTSS_TRACE_FLAGS_RINGBUF;
                } else {
                    grp->flags &= ~VTSS_TRACE_FLAGS_RINGBUF;
                }

                if (old_flags != grp->flags) {
                    T_I("Changing %s.%s.rb from %d to %d", vtss_module_names[module_id], grp->name, !itr->second.rb, itr->second.rb);
                }
            }
        }
    }

    return MESA_RC_OK;
}

