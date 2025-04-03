/*

 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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


#include <microchip/ethernet/switch/api.h>
#include "vtss/basics/preprocessor.h"
#include "vtss/basics/expose/json.hxx"
#include "vtss/basics/expose/json/parse-and-compare.hxx"

#define PP_SERIALIZE_JSON_ENUM_EXPORTER_CASE(X)     \
    case PP_TUPLE_N(0, X) :                         \
        serialize(ar, vtss::str(PP_TUPLE_N(1, X))); \
        break;

#define PP_SERIALIZE_JSON_ENUM_EXPORTER(N, ...)                 \
    void serialize(::vtss::expose::json::Exporter &ar, const N &b) {    \
        switch (b) {                                            \
            PP_FOR_EACH(PP_SERIALIZE_JSON_ENUM_EXPORTER_CASE,   \
                        __VA_ARGS__) default : ar.flag_error(); \
            break;                                              \
        }                                                       \
    }

#define PP_SERIALIZE_JSON_ENUM_LOADER_CASE(X)                               \
    if (parse_and_compare(ar.pos_, ar.end_, vtss::str(PP_TUPLE_N(1, X)))) { \
        b = PP_TUPLE_N(0, X);                                               \
        return;                                                             \
    }

#define PP_SERIALIZE_JSON_ENUM_LOADER(N, ...)           \
    void serialize(::vtss::expose::json::Loader &ar, N &b) {    \
        using ::vtss::expose::json::parse_and_compare;          \
        PP_FOR_EACH(PP_SERIALIZE_JSON_ENUM_LOADER_CASE, \
                    __VA_ARGS__) ar.flag_error();       \
    }

#define PP_SERIALIZE_JSON_ENUM(N, ...)              \
    PP_SERIALIZE_JSON_ENUM_EXPORTER(N, __VA_ARGS__) \
            PP_SERIALIZE_JSON_ENUM_LOADER(N, __VA_ARGS__)

#define PP_SERIALIZE_JSON_ENUM_DECLARE(N)      \
    PP_SERIALIZE_JSON_ENUM_EXPORTER_DECLARE(N) \
            PP_SERIALIZE_JSON_ENUM_LOADER_DECLARE(N)

#define DECL_SERIALIZE_REF(X)                             \
    void serialize(::vtss::expose::json::Exporter &e, const X &); \
    void serialize(::vtss::expose::json::Loader &e, X &);

#define DECL_SERIALIZE_TAG(X)                     \
    void serialize(::vtss::expose::json::Exporter &e, X); \
    void serialize(::vtss::expose::json::Loader &e, X);

PP_SERIALIZE_JSON_ENUM(mesa_clock_selection_mode_t,
    (MESA_CLOCK_SELECTION_MODE_DISABLED, "DISABLED"),
    (MESA_CLOCK_SELECTION_MODE_MANUEL, "MANUEL"),
    (MESA_CLOCK_SELECTION_MODE_AUTOMATIC_NONREVERTIVE, "AUTOMATIC_NONREVERTIVE"),
    (MESA_CLOCK_SELECTION_MODE_AUTOMATIC_REVERTIVE, "AUTOMATIC_REVERTIVE"),
    (MESA_CLOCK_SELECTION_MODE_FORCED_HOLDOVER, "FORCED_HOLDOVER"),
    (MESA_CLOCK_SELECTION_MODE_FORCED_FREE_RUN, "FORCED_FREE_RUN"),
    (MESA_CLOCK_SELECTION_MODE_FORCED_DCO, "FORCED_DCO")
);

template<class A>
void serialize(A &archiver, mesa_clock_selection_conf_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.mode, vtss::tag::Name("mode"));
    map.add_leaf(s.clock_input, vtss::tag::Name("clock_input"));
}

PP_SERIALIZE_JSON_ENUM(mesa_clock_operation_mode_t,
                       (MESA_CLOCK_OPERATION_MODE_DISABLED, "DISABLED"),
                       (MESA_CLOCK_OPERATION_MODE_ENABLED, "ENABLED")
                      );

template<class A>
void serialize(A &archiver, mesa_clock_dpll_conf_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.mode, vtss::tag::Name("mode"));
    map.add_leaf(s.holdoff, vtss::tag::Name("holdoff"));
    map.add_leaf(s.holdover, vtss::tag::Name("holdover"));
    map.add_leaf(s.wtr, vtss::tag::Name("wtr"));
}

template<class A>
void serialize(A &archiver, mesa_clock_psl_conf_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.limit_ppb, vtss::tag::Name("limit_ppb"));
    map.add_leaf(s.phase_build_out_ena, vtss::tag::Name("phase_build_out_ena"));
    map.add_leaf(s.ho_based, vtss::tag::Name("ho_based"));
}

template<class A>
void serialize(A &archiver, mesa_clock_ho_stack_conf_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.ho_post_filtering_bw, vtss::tag::Name("ho_post_filtering_bw"));
    map.add_leaf(s.ho_qual_time_conf, vtss::tag::Name("ho_qual_time_conf"));
}

template<class A>
void serialize(A &archiver, mesa_clock_ho_stack_content_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.stack_value[0], vtss::tag::Name("stack_value0"));
    map.add_leaf(s.stack_value[1], vtss::tag::Name("stack_value1"));
    map.add_leaf(s.stack_value[2], vtss::tag::Name("stack_value2"));
    map.add_leaf(s.stack_value[3], vtss::tag::Name("stack_value3"));
    map.add_leaf(s.stack_value[4], vtss::tag::Name("stack_value4"));
    map.add_leaf(s.stack_value[5], vtss::tag::Name("stack_value5"));
    map.add_leaf(s.stack_value[6], vtss::tag::Name("stack_value6"));
    map.add_leaf(s.stack_value[7], vtss::tag::Name("stack_value7"));
    map.add_leaf(s.stack_value[8], vtss::tag::Name("stack_value8"));
    map.add_leaf(s.stack_value[9], vtss::tag::Name("stack_value9"));
    map.add_leaf(s.stack_value[10], vtss::tag::Name("stack_value10"));
    map.add_leaf(s.stack_value[11], vtss::tag::Name("stack_value11"));
    map.add_leaf(s.ho_sel, vtss::tag::Name("ho_sel"));
    map.add_leaf(s.ho_min_fill_lvl, vtss::tag::Name("ho_min_fill_lvl"));
    map.add_leaf(s.ho_filled, vtss::tag::Name("ho_filled"));
}

template<class A>
void serialize(A &archiver, mesa_clock_priority_selector_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.priority, vtss::tag::Name("priority"));
    map.add_leaf(s.enable, vtss::tag::Name("enable"));
}

template<class A>
void serialize(A &archiver, mesa_clock_ratio_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.num, vtss::tag::Name("num"));
    map.add_leaf(s.den, vtss::tag::Name("den"));
}

PP_SERIALIZE_JSON_ENUM(mesa_clock_input_type_t,
                       (MESA_CLOCK_INPUT_TYPE_DPLL, "DPLL"),
                       (MESA_CLOCK_INPUT_TYPE_IN, "IN"),
                       (MESA_CLOCK_INPUT_TYPE_PURE_DCO, "PURE_DCO")
                      );

template<class A>
void serialize(A &archiver, mesa_clock_input_selector_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.input_type, vtss::tag::Name("input_type"));
    map.add_leaf(s.input_inst, vtss::tag::Name("input_inst"));
}


template<class A>
void serialize(A &archiver, mesa_clock_input_alarm_ena_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.los, vtss::tag::Name("los"));
    map.add_leaf(s.pfm, vtss::tag::Name("pfm"));
    map.add_leaf(s.cfm, vtss::tag::Name("cfm"));
    map.add_leaf(s.scm, vtss::tag::Name("scm"));
    map.add_leaf(s.gst, vtss::tag::Name("gst"));
    map.add_leaf(s.lol, vtss::tag::Name("lol"));
}

template<class A>
void serialize(A &archiver, mesa_clock_input_conf_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.los_active_high, vtss::tag::Name("los_active_high"));
    map.add_leaf(s.alarm_ena, vtss::tag::Name("alarm_ena"));
}

template<class A>
void serialize(A &archiver, mesa_clock_cfm_conf_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.cfm_set_ppb, vtss::tag::Name("cfm_set_ppb"));
    map.add_leaf(s.cfm_clr_ppb, vtss::tag::Name("cfm_clr_ppb"));
}

template<class A>
void serialize(A &archiver, mesa_clock_pfm_conf_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.pfm_set_ppb, vtss::tag::Name("pfm_set_ppb"));
    map.add_leaf(s.pfm_clr_ppb, vtss::tag::Name("pfm_clr_ppb"));
}

template<class A>
void serialize(A &archiver, mesa_clock_gst_conf_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.disqualification_time_us, vtss::tag::Name("disqualification_time_us"));
    map.add_leaf(s.qualification_time_us, vtss::tag::Name("qualification_time_us"));
    map.add_leaf(s.los, vtss::tag::Name("los"));
    map.add_leaf(s.pfm, vtss::tag::Name("pfm"));
    map.add_leaf(s.cfm, vtss::tag::Name("cfm"));
    map.add_leaf(s.scm, vtss::tag::Name("scm"));
    map.add_leaf(s.lol, vtss::tag::Name("lol"));
}

PP_SERIALIZE_JSON_ENUM(mesa_clock_selector_state_t,
                       (MESA_CLOCK_SELECTOR_STATE_LOCKED, "LOCKED"),
                       (MESA_CLOCK_SELECTOR_STATE_HOLDOVER, "HOLDOVER"),
                       (MESA_CLOCK_SELECTOR_STATE_FREERUN, "FREERUN"),
                       (MESA_CLOCK_SELECTOR_STATE_DCO, "DCO"),
                       (MESA_CLOCK_SELECTOR_STATE_REF_FAILED, "REF_FAILED"),
                       (MESA_CLOCK_SELECTOR_STATE_ACQUIRING, "ACQUIRING")
                      );


template<class A>
void serialize(A &archiver, mesa_clock_dpll_state_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.pll_freq_lock, vtss::tag::Name("pll_freq_lock"));
    map.add_leaf(s.pll_phase_lock, vtss::tag::Name("pll_phase_lock"));
    map.add_leaf(s.pll_losx, vtss::tag::Name("pll_losx"));
    map.add_leaf(s.pll_lol, vtss::tag::Name("pll_lol"));
    map.add_leaf(s.pll_dig_hold_vld, vtss::tag::Name("pll_dig_hold_vld"));
}

template<class A>
void serialize(A &archiver, mesa_clock_input_state_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.los, vtss::tag::Name("los"));
    map.add_leaf(s.pfm, vtss::tag::Name("pfm"));
    map.add_leaf(s.cfm, vtss::tag::Name("cfm"));
    map.add_leaf(s.scm, vtss::tag::Name("scm"));
    map.add_leaf(s.lol, vtss::tag::Name("lol"));
}

PP_SERIALIZE_JSON_ENUM(mesa_synce_divider_t,
                       (MESA_SYNCE_DIVIDER_1, "DIVIDER_1"),
                       (MESA_SYNCE_DIVIDER_4, "DIVIDER_4"),
                       (MESA_SYNCE_DIVIDER_5, "DIVIDER_5")
                      );

template<class A>
void serialize(A &archiver, mesa_synce_clock_out_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.divider, vtss::tag::Name("divider"));
    map.add_leaf(s.enable, vtss::tag::Name("enable"));
}

PP_SERIALIZE_JSON_ENUM(mesa_synce_clock_in_type_t,
                       (MESA_SYNCE_CLOCK_INTERFACE,     "INTERFACE"),
                       (MESA_SYNCE_CLOCK_STATION_CLK,   "STATION_CLK"),
                       (MESA_SYNCE_CLOCK_DIFF,          "DIFF")
                      );

template<class A>
void serialize(A &archiver, mesa_synce_clock_in_t &s) {
    typename A::Map map = archiver.as_map();

    map.add_leaf(s.port_no, vtss::tag::Name("port_no"));
    map.add_leaf(s.squelsh, vtss::tag::Name("squelsh"));
    map.add_leaf(s.enable, vtss::tag::Name("enable"));
    map.add_leaf(s.clk_in, vtss::tag::Name("clk_in"));  // FIXME: This leaf should only be present/defined on ServalT
}


namespace vtss {

static expose::json::NamespaceNode ns("api_clock");

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u32>,
    json::PARAM_AUTO<     u32 *const>
> export_rd(&ns, "rd", &mesa_clock_rd);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u32>,
    json::PARAM_AUTO<     u32>
> export_wr(&ns, "wr", &mesa_clock_wr);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u32>,
    json::PARAM_AUTO<     u32>,
    json::PARAM_AUTO<     u32>
> export_wrm(&ns, "wrm", &mesa_clock_wrm);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     const mesa_clock_global_enable_t>
> export_global_enable_set(&ns, "global_enable_set", &mesa_clock_global_enable_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
         json::PARAM_AUTO<     mesa_clock_global_enable_t *const>
> export_global_enable_get(&ns, "global_enable_get", &mesa_clock_global_enable_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>
> export_global_sw_reset(&ns, "global_sw_reset", &mesa_clock_global_sw_reset);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     const mesa_clock_selection_conf_t *const>
> export_selection_mode_set(&ns, "selection_mode_set", &mesa_clock_selection_mode_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     mesa_clock_selection_conf_t *const>
> export_selection_mode_get(&ns, "selection_mode_get", &mesa_clock_selection_mode_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     const mesa_clock_dpll_conf_t *const>
> export_operation_conf_set(&ns, "operation_conf_set", &mesa_clock_operation_conf_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_conf_t *const>
> export_operation_conf_get(&ns, "operation_conf_get", &mesa_clock_operation_conf_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     const mesa_clock_ho_stack_conf_t *const>
> export_operation_ho_stack_conf_set(&ns, "ho_stack_conf_set", &mesa_clock_ho_stack_conf_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     mesa_clock_ho_stack_conf_t *const>
> export_operation_ho_stack_conf_get(&ns, "ho_stack_conf_get", &mesa_clock_ho_stack_conf_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     mesa_clock_ho_stack_content_t *const>
> export_operation_ho_stack_content_get(&ns, "ho_stack_content_get", &mesa_clock_ho_stack_content_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     i64>
> export_dco_frequency_offset_set(&ns, "dco_frequency_offset_set", &mesa_clock_dco_frequency_offset_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     i64 *const>
> export_dco_frequency_offset_get(&ns, "dco_frequency_offset_get", &mesa_clock_dco_frequency_offset_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     u32>
> export_output_filter_bw_set(&ns, "output_filter_bw_set", &mesa_clock_output_filter_bw_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     u32 *const>
> export_output_filter_bw_get(&ns, "output_filter_bw_get", &mesa_clock_output_filter_bw_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>
> export_output_filter_lock_fast_set(&ns, "output_filter_lock_fast_set", &mesa_clock_output_filter_lock_fast_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     BOOL *const>
> export_output_filter_lock_fast_get(&ns, "output_filter_lock_fast_get", &mesa_clock_output_filter_lock_fast_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     const mesa_clock_psl_conf_t *const>
> export_output_psl_conf_set(&ns, "output_psl_conf_set", &mesa_clock_output_psl_conf_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     mesa_clock_psl_conf_t *const>
> export_output_psl_conf_get(&ns, "output_psl_conf_get", &mesa_clock_output_psl_conf_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     i64>
> export_adj_frequency_set(&ns, "adj_frequency_set", &mesa_clock_adj_frequency_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     i64 *const>
> export_adj_frequency_get(&ns, "adj_frequency_get", &mesa_clock_adj_frequency_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     i32>
> export_adj_phase_set(&ns, "adj_phase_set", &mesa_clock_adj_phase_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     BOOL *const>
> export_adj_phase_get(&ns, "adj_phase_get", &mesa_clock_adj_phase_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     const mesa_clock_priority_selector_t *const>
> export_priority_set(&ns, "priority_set", &mesa_clock_priority_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     mesa_clock_priority_selector_t *const>
> export_priority_get(&ns, "priority_get", &mesa_clock_priority_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     u32>,
    json::PARAM_AUTO<     BOOL>
> export_input_frequency_set(&ns, "input_frequency_set", &mesa_clock_input_frequency_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     u32 *const>,
    json::PARAM_AUTO<     BOOL *const>
> export_input_frequency_get(&ns, "input_frequency_get", &mesa_clock_input_frequency_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     u32>,
    json::PARAM_AUTO<     const mesa_clock_ratio_t *const>,
    json::PARAM_AUTO<     BOOL>
> export_input_frequency_ratio_set(&ns, "input_frequency_ratio_set", &mesa_clock_input_frequency_ratio_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     u32 *const>,
    json::PARAM_AUTO<     mesa_clock_ratio_t *const>,
    json::PARAM_AUTO<     BOOL *const>
> export_input_frequency_ratio_get(&ns, "input_frequency_ratio_get", &mesa_clock_input_frequency_ratio_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     u32>,
    json::PARAM_AUTO<     u32>
> export_output_frequency_set(&ns, "output_frequency_set", &mesa_clock_output_frequency_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     u32 *const>,
    json::PARAM_AUTO<     u32 *const>
> export_output_frequency_get(&ns, "output_frequency_get", &mesa_clock_output_frequency_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     u32>,
    json::PARAM_AUTO<     u32>,
    json::PARAM_AUTO<     const mesa_clock_ratio_t *const>
> export_output_frequency_ratio_set(&ns, "output_frequency_ratio_set", &mesa_clock_output_frequency_ratio_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     u32 *const>,
    json::PARAM_AUTO<     u32 *const>,
    json::PARAM_AUTO<     mesa_clock_ratio_t *const>
> export_output_frequency_ratio_get(&ns, "output_frequency_ratio_get", &mesa_clock_output_frequency_ratio_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     u16>
> export_output_level_set(&ns, "output_level_set", &mesa_clock_output_level_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     u16 *const>
> export_output_level_get(&ns, "output_level_get", &mesa_clock_output_level_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     const mesa_clock_input_selector_t *const>
> export_output_selector_set(&ns, "output_selector_set", &mesa_clock_output_selector_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     mesa_clock_input_selector_t *const>
> export_output_selector_get(&ns, "output_selector_get", &mesa_clock_output_selector_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     const mesa_clock_input_conf_t   *const >
> export_input_alarm_conf_set(&ns, "input_alarm_conf_set", &mesa_clock_input_alarm_conf_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     mesa_clock_input_conf_t   *const >
> export_input_alarm_conf_get(&ns, "input_alarm_conf_get", &mesa_clock_input_alarm_conf_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     const mesa_clock_cfm_conf_t   *const >
> export_input_cfm_conf_set(&ns, "input_cfm_conf_set", &mesa_clock_input_cfm_conf_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     mesa_clock_cfm_conf_t   *const >
> export_input_cfm_conf_get(&ns, "input_cfm_conf_get", &mesa_clock_input_cfm_conf_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     const mesa_clock_pfm_conf_t   *const >
> export_input_pfm_conf_set(&ns, "input_pfm_conf_set", &mesa_clock_input_pfm_conf_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     mesa_clock_pfm_conf_t   *const >
> export_input_pfm_conf_get(&ns, "input_pfm_conf_get", &mesa_clock_input_pfm_conf_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     const mesa_clock_gst_conf_t   *const >
> export_input_gst_conf_set(&ns, "input_gst_conf_set", &mesa_clock_input_gst_conf_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     mesa_clock_gst_conf_t   *const >
> export_input_gst_conf_get(&ns, "input_gst_conf_get", &mesa_clock_input_gst_conf_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     mesa_clock_selector_state_t   *const>,
    json::PARAM_AUTO<     u8                            *const>
> export_selector_state_get(&ns, "selector_state_get", &mesa_clock_selector_state_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_state_t       *const>
> export_dpll_state_get(&ns, "dpll_state_get", &mesa_clock_dpll_state_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     i64 *const>
> export_ho_stack_frequency_offset_get(&ns, "ho_stack_frequency_offset_get", &mesa_clock_ho_stack_frequency_offset_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     mesa_clock_input_state_t      *const>
> export_input_state_get(&ns, "input_state_get", &mesa_clock_input_state_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     u32   *const>
> export_input_event_poll(&ns, "input_event_poll", &mesa_clock_input_event_poll);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     u8>,
    json::PARAM_AUTO<     u32>,
    json::PARAM_AUTO<     BOOL>
> export_input_event_enable(&ns, "input_event_enable", &mesa_clock_input_event_enable);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     u32   *const>
> export_dpll_event_poll(&ns, "dpll_event_poll", &mesa_clock_dpll_event_poll);


static json::FunctionExporter<
    json::PARAM_INST_PTR< const mesa_inst_t>,
    json::PARAM_AUTO<     mesa_clock_dpll_inst_t>,
    json::PARAM_AUTO<     u32>,
    json::PARAM_AUTO<     BOOL>
> export_dpll_event_enable(&ns, "dpll_event_enable", &mesa_clock_dpll_event_enable);



static json::FunctionExporter<
json::PARAM_INST_PTR< const mesa_inst_t>,
json::PARAM_AUTO<     mesa_synce_clk_port_t>,
json::PARAM_AUTO<     const mesa_synce_clock_out_t *const>
> export_clock_out_set(&ns, "clock_out_set", &mesa_synce_clock_out_set);

static json::FunctionExporter<
json::PARAM_INST_PTR< const mesa_inst_t>,
json::PARAM_AUTO<     mesa_synce_clk_port_t>,
json::PARAM_AUTO<     mesa_synce_clock_out_t *const>
> export_clock_out_get(&ns, "clock_out_get", &mesa_synce_clock_out_get);

static json::FunctionExporter<
json::PARAM_INST_PTR< const mesa_inst_t>,
json::PARAM_AUTO<     mesa_synce_clk_port_t>,
json::PARAM_AUTO<     const mesa_synce_clock_in_t *const>
> export_clock_in_set(&ns, "clock_in_set", &mesa_synce_clock_in_set);

static json::FunctionExporter<
json::PARAM_INST_PTR< const mesa_inst_t>,
json::PARAM_AUTO<     mesa_synce_clk_port_t>,
json::PARAM_AUTO<     mesa_synce_clock_in_t *const>
> export_clock_in_get(&ns, "clock_in_get", &mesa_synce_clock_in_get);


}

extern "C" {
    void vtss_synce_json_init() {
        vtss::json_node_add(&vtss::ns);
    }
}
