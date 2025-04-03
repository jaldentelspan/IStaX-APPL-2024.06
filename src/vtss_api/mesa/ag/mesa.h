#include "vtss_api.h"
#include <microchip/ethernet/switch/api.h>

#ifndef __MESA_H__
#define __MESA_H__
#ifdef __cplusplus
extern "C" {
#endif
inline static mesa_rc mesa_conv_vtss_inst_t_to_mesa_inst_t(const vtss_inst_t *in, mesa_inst_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_inst_t *tmp = (const mesa_inst_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_inst_t_to_vtss_inst_t(const mesa_inst_t *in, vtss_inst_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_inst_t *tmp = (const vtss_inst_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V1)
mesa_rc mesa_conv_vtss_afi_frm_dscr_t_to_mesa_afi_frm_dscr_t(const vtss_afi_frm_dscr_t *in, mesa_afi_frm_dscr_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_AFI_V1)
#endif  // defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V1)
mesa_rc mesa_conv_mesa_afi_frm_dscr_t_to_vtss_afi_frm_dscr_t(const mesa_afi_frm_dscr_t *in, vtss_afi_frm_dscr_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_AFI_V1)
#endif  // defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
inline static mesa_rc mesa_conv_vtss_afi_fast_inj_alloc_cfg_t_to_mesa_afi_fast_inj_alloc_cfg_t(const vtss_afi_fast_inj_alloc_cfg_t *in, mesa_afi_fast_inj_alloc_cfg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_afi_fast_inj_alloc_cfg_t *tmp = (const mesa_afi_fast_inj_alloc_cfg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_AFI_V2)
#endif  // defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
inline static mesa_rc mesa_conv_mesa_afi_fast_inj_alloc_cfg_t_to_vtss_afi_fast_inj_alloc_cfg_t(const mesa_afi_fast_inj_alloc_cfg_t *in, vtss_afi_fast_inj_alloc_cfg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_afi_fast_inj_alloc_cfg_t *tmp = (const vtss_afi_fast_inj_alloc_cfg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_AFI_V2)
#endif  // defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
inline static mesa_rc mesa_conv_vtss_afi_fast_inj_frm_cfg_t_to_mesa_afi_fast_inj_frm_cfg_t(const vtss_afi_fast_inj_frm_cfg_t *in, mesa_afi_fast_inj_frm_cfg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_afi_fast_inj_frm_cfg_t *tmp = (const mesa_afi_fast_inj_frm_cfg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_AFI_V2)
#endif  // defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
inline static mesa_rc mesa_conv_mesa_afi_fast_inj_frm_cfg_t_to_vtss_afi_fast_inj_frm_cfg_t(const mesa_afi_fast_inj_frm_cfg_t *in, vtss_afi_fast_inj_frm_cfg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_afi_fast_inj_frm_cfg_t *tmp = (const vtss_afi_fast_inj_frm_cfg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_AFI_V2)
#endif  // defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
mesa_rc mesa_conv_vtss_afi_fast_inj_start_cfg_t_to_mesa_afi_fast_inj_start_cfg_t(const vtss_afi_fast_inj_start_cfg_t *in, mesa_afi_fast_inj_start_cfg_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_AFI_V2)
#endif  // defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
mesa_rc mesa_conv_mesa_afi_fast_inj_start_cfg_t_to_vtss_afi_fast_inj_start_cfg_t(const mesa_afi_fast_inj_start_cfg_t *in, vtss_afi_fast_inj_start_cfg_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_AFI_V2)
#endif  // defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
inline static mesa_rc mesa_conv_vtss_afi_slow_inj_alloc_cfg_t_to_mesa_afi_slow_inj_alloc_cfg_t(const vtss_afi_slow_inj_alloc_cfg_t *in, mesa_afi_slow_inj_alloc_cfg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_afi_slow_inj_alloc_cfg_t *tmp = (const mesa_afi_slow_inj_alloc_cfg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_AFI_V2)
#endif  // defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
inline static mesa_rc mesa_conv_mesa_afi_slow_inj_alloc_cfg_t_to_vtss_afi_slow_inj_alloc_cfg_t(const mesa_afi_slow_inj_alloc_cfg_t *in, vtss_afi_slow_inj_alloc_cfg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_afi_slow_inj_alloc_cfg_t *tmp = (const vtss_afi_slow_inj_alloc_cfg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_AFI_V2)
#endif  // defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
inline static mesa_rc mesa_conv_vtss_afi_slow_inj_start_cfg_t_to_mesa_afi_slow_inj_start_cfg_t(const vtss_afi_slow_inj_start_cfg_t *in, mesa_afi_slow_inj_start_cfg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_afi_slow_inj_start_cfg_t *tmp = (const mesa_afi_slow_inj_start_cfg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_AFI_V2)
#endif  // defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
inline static mesa_rc mesa_conv_mesa_afi_slow_inj_start_cfg_t_to_vtss_afi_slow_inj_start_cfg_t(const mesa_afi_slow_inj_start_cfg_t *in, vtss_afi_slow_inj_start_cfg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_afi_slow_inj_start_cfg_t *tmp = (const vtss_afi_slow_inj_start_cfg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_AFI_V2)
#endif  // defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_selection_mode_t_to_mesa_clock_selection_mode_t(const vtss_clock_selection_mode_t *in, mesa_clock_selection_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_selection_mode_t *tmp = (const mesa_clock_selection_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_selection_mode_t_to_vtss_clock_selection_mode_t(const mesa_clock_selection_mode_t *in, vtss_clock_selection_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_selection_mode_t *tmp = (const vtss_clock_selection_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_selection_conf_t_to_mesa_clock_selection_conf_t(const vtss_clock_selection_conf_t *in, mesa_clock_selection_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_selection_conf_t *tmp = (const mesa_clock_selection_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_selection_conf_t_to_vtss_clock_selection_conf_t(const mesa_clock_selection_conf_t *in, vtss_clock_selection_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_selection_conf_t *tmp = (const vtss_clock_selection_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_selector_state_t_to_mesa_clock_selector_state_t(const vtss_clock_selector_state_t *in, mesa_clock_selector_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_selector_state_t *tmp = (const mesa_clock_selector_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_selector_state_t_to_vtss_clock_selector_state_t(const mesa_clock_selector_state_t *in, vtss_clock_selector_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_selector_state_t *tmp = (const vtss_clock_selector_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_operation_mode_t_to_mesa_clock_operation_mode_t(const vtss_clock_operation_mode_t *in, mesa_clock_operation_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_operation_mode_t *tmp = (const mesa_clock_operation_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_operation_mode_t_to_vtss_clock_operation_mode_t(const mesa_clock_operation_mode_t *in, vtss_clock_operation_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_operation_mode_t *tmp = (const vtss_clock_operation_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_psl_conf_t_to_mesa_clock_psl_conf_t(const vtss_clock_psl_conf_t *in, mesa_clock_psl_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_psl_conf_t *tmp = (const mesa_clock_psl_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_psl_conf_t_to_vtss_clock_psl_conf_t(const mesa_clock_psl_conf_t *in, vtss_clock_psl_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_psl_conf_t *tmp = (const vtss_clock_psl_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_dpll_conf_t_to_mesa_clock_dpll_conf_t(const vtss_clock_dpll_conf_t *in, mesa_clock_dpll_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_dpll_conf_t *tmp = (const mesa_clock_dpll_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_dpll_conf_t_to_vtss_clock_dpll_conf_t(const mesa_clock_dpll_conf_t *in, vtss_clock_dpll_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_dpll_conf_t *tmp = (const vtss_clock_dpll_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_ho_stack_conf_t_to_mesa_clock_ho_stack_conf_t(const vtss_clock_ho_stack_conf_t *in, mesa_clock_ho_stack_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_ho_stack_conf_t *tmp = (const mesa_clock_ho_stack_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_ho_stack_conf_t_to_vtss_clock_ho_stack_conf_t(const mesa_clock_ho_stack_conf_t *in, vtss_clock_ho_stack_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_ho_stack_conf_t *tmp = (const vtss_clock_ho_stack_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_ho_stack_content_t_to_mesa_clock_ho_stack_content_t(const vtss_clock_ho_stack_content_t *in, mesa_clock_ho_stack_content_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_ho_stack_content_t *tmp = (const mesa_clock_ho_stack_content_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_ho_stack_content_t_to_vtss_clock_ho_stack_content_t(const mesa_clock_ho_stack_content_t *in, vtss_clock_ho_stack_content_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_ho_stack_content_t *tmp = (const vtss_clock_ho_stack_content_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_priority_selector_t_to_mesa_clock_priority_selector_t(const vtss_clock_priority_selector_t *in, mesa_clock_priority_selector_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_priority_selector_t *tmp = (const mesa_clock_priority_selector_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_priority_selector_t_to_vtss_clock_priority_selector_t(const mesa_clock_priority_selector_t *in, vtss_clock_priority_selector_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_priority_selector_t *tmp = (const vtss_clock_priority_selector_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_ratio_t_to_mesa_clock_ratio_t(const vtss_clock_ratio_t *in, mesa_clock_ratio_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_ratio_t *tmp = (const mesa_clock_ratio_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_ratio_t_to_vtss_clock_ratio_t(const mesa_clock_ratio_t *in, vtss_clock_ratio_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_ratio_t *tmp = (const vtss_clock_ratio_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_input_type_t_to_mesa_clock_input_type_t(const vtss_clock_input_type_t *in, mesa_clock_input_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_input_type_t *tmp = (const mesa_clock_input_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_input_type_t_to_vtss_clock_input_type_t(const mesa_clock_input_type_t *in, vtss_clock_input_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_input_type_t *tmp = (const vtss_clock_input_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_input_selector_t_to_mesa_clock_input_selector_t(const vtss_clock_input_selector_t *in, mesa_clock_input_selector_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_input_selector_t *tmp = (const mesa_clock_input_selector_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_input_selector_t_to_vtss_clock_input_selector_t(const mesa_clock_input_selector_t *in, vtss_clock_input_selector_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_input_selector_t *tmp = (const vtss_clock_input_selector_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_dpll_state_t_to_mesa_clock_dpll_state_t(const vtss_clock_dpll_state_t *in, mesa_clock_dpll_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_dpll_state_t *tmp = (const mesa_clock_dpll_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_dpll_state_t_to_vtss_clock_dpll_state_t(const mesa_clock_dpll_state_t *in, vtss_clock_dpll_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_dpll_state_t *tmp = (const vtss_clock_dpll_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_input_state_t_to_mesa_clock_input_state_t(const vtss_clock_input_state_t *in, mesa_clock_input_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_input_state_t *tmp = (const mesa_clock_input_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_input_state_t_to_vtss_clock_input_state_t(const mesa_clock_input_state_t *in, vtss_clock_input_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_input_state_t *tmp = (const vtss_clock_input_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_input_alarm_ena_t_to_mesa_clock_input_alarm_ena_t(const vtss_clock_input_alarm_ena_t *in, mesa_clock_input_alarm_ena_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_input_alarm_ena_t *tmp = (const mesa_clock_input_alarm_ena_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_input_alarm_ena_t_to_vtss_clock_input_alarm_ena_t(const mesa_clock_input_alarm_ena_t *in, vtss_clock_input_alarm_ena_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_input_alarm_ena_t *tmp = (const vtss_clock_input_alarm_ena_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_input_conf_t_to_mesa_clock_input_conf_t(const vtss_clock_input_conf_t *in, mesa_clock_input_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_input_conf_t *tmp = (const mesa_clock_input_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_input_conf_t_to_vtss_clock_input_conf_t(const mesa_clock_input_conf_t *in, vtss_clock_input_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_input_conf_t *tmp = (const vtss_clock_input_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_cfm_conf_t_to_mesa_clock_cfm_conf_t(const vtss_clock_cfm_conf_t *in, mesa_clock_cfm_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_cfm_conf_t *tmp = (const mesa_clock_cfm_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_cfm_conf_t_to_vtss_clock_cfm_conf_t(const mesa_clock_cfm_conf_t *in, vtss_clock_cfm_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_cfm_conf_t *tmp = (const vtss_clock_cfm_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_pfm_conf_t_to_mesa_clock_pfm_conf_t(const vtss_clock_pfm_conf_t *in, mesa_clock_pfm_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_pfm_conf_t *tmp = (const mesa_clock_pfm_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_pfm_conf_t_to_vtss_clock_pfm_conf_t(const mesa_clock_pfm_conf_t *in, vtss_clock_pfm_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_pfm_conf_t *tmp = (const vtss_clock_pfm_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_vtss_clock_gst_conf_t_to_mesa_clock_gst_conf_t(const vtss_clock_gst_conf_t *in, mesa_clock_gst_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_clock_gst_conf_t *tmp = (const mesa_clock_gst_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
inline static mesa_rc mesa_conv_mesa_clock_gst_conf_t_to_vtss_clock_gst_conf_t(const mesa_clock_gst_conf_t *in, vtss_clock_gst_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_clock_gst_conf_t *tmp = (const vtss_clock_gst_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_conv_vtss_target_type_t_to_mesa_target_type_t(const vtss_target_type_t *in, mesa_target_type_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_target_type_t_to_vtss_target_type_t(const mesa_target_type_t *in, vtss_target_type_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_vtss_inst_create_t_to_mesa_inst_create_t(const vtss_inst_create_t *in, mesa_inst_create_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_inst_create_t_to_vtss_inst_create_t(const mesa_inst_create_t *in, vtss_inst_create_t *out); /* ag.rb:1595 */
inline static mesa_rc mesa_conv_vtss_spi_read_write_t_to_mesa_spi_read_write_t(const vtss_spi_read_write_t *in, mesa_spi_read_write_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_spi_read_write_t *tmp = (const mesa_spi_read_write_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_spi_read_write_t_to_vtss_spi_read_write_t(const mesa_spi_read_write_t *in, vtss_spi_read_write_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_spi_read_write_t *tmp = (const vtss_spi_read_write_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_spi_32bit_read_write_t_to_mesa_spi_32bit_read_write_t(const vtss_spi_32bit_read_write_t *in, mesa_spi_32bit_read_write_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_spi_32bit_read_write_t *tmp = (const mesa_spi_32bit_read_write_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_spi_32bit_read_write_t_to_vtss_spi_32bit_read_write_t(const mesa_spi_32bit_read_write_t *in, vtss_spi_32bit_read_write_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_spi_32bit_read_write_t *tmp = (const vtss_spi_32bit_read_write_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_spi_64bit_read_write_t_to_mesa_spi_64bit_read_write_t(const vtss_spi_64bit_read_write_t *in, mesa_spi_64bit_read_write_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_spi_64bit_read_write_t *tmp = (const mesa_spi_64bit_read_write_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_spi_64bit_read_write_t_to_vtss_spi_64bit_read_write_t(const mesa_spi_64bit_read_write_t *in, vtss_spi_64bit_read_write_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_spi_64bit_read_write_t *tmp = (const vtss_spi_64bit_read_write_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_miim_read_t_to_mesa_miim_read_t(const vtss_miim_read_t *in, mesa_miim_read_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_miim_read_t *tmp = (const mesa_miim_read_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_miim_read_t_to_vtss_miim_read_t(const mesa_miim_read_t *in, vtss_miim_read_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_miim_read_t *tmp = (const vtss_miim_read_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_miim_write_t_to_mesa_miim_write_t(const vtss_miim_write_t *in, mesa_miim_write_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_miim_write_t *tmp = (const mesa_miim_write_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_miim_write_t_to_vtss_miim_write_t(const mesa_miim_write_t *in, vtss_miim_write_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_miim_write_t *tmp = (const vtss_miim_write_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_mmd_read_t_to_mesa_mmd_read_t(const vtss_mmd_read_t *in, mesa_mmd_read_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mmd_read_t *tmp = (const mesa_mmd_read_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_mmd_read_t_to_vtss_mmd_read_t(const mesa_mmd_read_t *in, vtss_mmd_read_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mmd_read_t *tmp = (const vtss_mmd_read_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_mmd_read_inc_t_to_mesa_mmd_read_inc_t(const vtss_mmd_read_inc_t *in, mesa_mmd_read_inc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mmd_read_inc_t *tmp = (const mesa_mmd_read_inc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_mmd_read_inc_t_to_vtss_mmd_read_inc_t(const mesa_mmd_read_inc_t *in, vtss_mmd_read_inc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mmd_read_inc_t *tmp = (const vtss_mmd_read_inc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_mmd_write_t_to_mesa_mmd_write_t(const vtss_mmd_write_t *in, mesa_mmd_write_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mmd_write_t *tmp = (const mesa_mmd_write_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_mmd_write_t_to_vtss_mmd_write_t(const mesa_mmd_write_t *in, vtss_mmd_write_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mmd_write_t *tmp = (const vtss_mmd_write_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#if defined(VTSS_FEATURE_PORT_MUX)
mesa_rc mesa_conv_vtss_port_mux_mode_t_to_mesa_port_mux_mode_t(const vtss_port_mux_mode_t *in, mesa_port_mux_mode_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_MUX)
#if defined(VTSS_FEATURE_PORT_MUX)
mesa_rc mesa_conv_mesa_port_mux_mode_t_to_vtss_port_mux_mode_t(const mesa_port_mux_mode_t *in, vtss_port_mux_mode_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_MUX)
#if defined(VTSS_FEATURE_SERDES_MACRO_SETTINGS)
inline static mesa_rc mesa_conv_vtss_serdes_fields_t_to_mesa_serdes_fields_t(const vtss_serdes_fields_t *in, mesa_serdes_fields_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_serdes_fields_t *tmp = (const mesa_serdes_fields_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SERDES_MACRO_SETTINGS)
#if defined(VTSS_FEATURE_SERDES_MACRO_SETTINGS)
inline static mesa_rc mesa_conv_mesa_serdes_fields_t_to_vtss_serdes_fields_t(const mesa_serdes_fields_t *in, vtss_serdes_fields_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_serdes_fields_t *tmp = (const vtss_serdes_fields_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SERDES_MACRO_SETTINGS)
#if defined(VTSS_FEATURE_SERDES_MACRO_SETTINGS)
inline static mesa_rc mesa_conv_vtss_serdes_macro_conf_t_to_mesa_serdes_macro_conf_t(const vtss_serdes_macro_conf_t *in, mesa_serdes_macro_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_serdes_macro_conf_t *tmp = (const mesa_serdes_macro_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SERDES_MACRO_SETTINGS)
#if defined(VTSS_FEATURE_SERDES_MACRO_SETTINGS)
inline static mesa_rc mesa_conv_mesa_serdes_macro_conf_t_to_vtss_serdes_macro_conf_t(const mesa_serdes_macro_conf_t *in, vtss_serdes_macro_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_serdes_macro_conf_t *tmp = (const vtss_serdes_macro_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SERDES_MACRO_SETTINGS)
#if defined(VTSS_FEATURE_WARM_START)
inline static mesa_rc mesa_conv_vtss_restart_info_src_t_to_mesa_restart_info_src_t(const vtss_restart_info_src_t *in, mesa_restart_info_src_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_restart_info_src_t *tmp = (const mesa_restart_info_src_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_WARM_START)
#if defined(VTSS_FEATURE_WARM_START)
inline static mesa_rc mesa_conv_mesa_restart_info_src_t_to_vtss_restart_info_src_t(const mesa_restart_info_src_t *in, vtss_restart_info_src_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_restart_info_src_t *tmp = (const vtss_restart_info_src_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_WARM_START)
#if defined(VTSS_ARCH_JAGUAR_2_CE)
inline static mesa_rc mesa_conv_vtss_qs_mode_t_to_mesa_qs_mode_t(const vtss_qs_mode_t *in, mesa_qs_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qs_mode_t *tmp = (const mesa_qs_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2_CE)
#if defined(VTSS_ARCH_JAGUAR_2_CE)
inline static mesa_rc mesa_conv_mesa_qs_mode_t_to_vtss_qs_mode_t(const mesa_qs_mode_t *in, vtss_qs_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qs_mode_t *tmp = (const vtss_qs_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2_CE)
#if defined(VTSS_ARCH_JAGUAR_2_CE)
inline static mesa_rc mesa_conv_vtss_qs_conf_t_to_mesa_qs_conf_t(const vtss_qs_conf_t *in, mesa_qs_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qs_conf_t *tmp = (const mesa_qs_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2_CE)
#if defined(VTSS_ARCH_JAGUAR_2_CE)
inline static mesa_rc mesa_conv_mesa_qs_conf_t_to_vtss_qs_conf_t(const mesa_qs_conf_t *in, vtss_qs_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qs_conf_t *tmp = (const vtss_qs_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2_CE)
#if defined(VTSS_FEATURE_CORE_CLOCK)
mesa_rc mesa_conv_vtss_core_clock_freq_t_to_mesa_core_clock_freq_t(const vtss_core_clock_freq_t *in, mesa_core_clock_freq_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_CORE_CLOCK)
#if defined(VTSS_FEATURE_CORE_CLOCK)
mesa_rc mesa_conv_mesa_core_clock_freq_t_to_vtss_core_clock_freq_t(const mesa_core_clock_freq_t *in, vtss_core_clock_freq_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_CORE_CLOCK)
#if defined(VTSS_FEATURE_CORE_CLOCK)
inline static mesa_rc mesa_conv_vtss_core_ref_clk_t_to_mesa_core_ref_clk_t(const vtss_core_ref_clk_t *in, mesa_core_ref_clk_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_core_ref_clk_t *tmp = (const mesa_core_ref_clk_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CORE_CLOCK)
#if defined(VTSS_FEATURE_CORE_CLOCK)
inline static mesa_rc mesa_conv_mesa_core_ref_clk_t_to_vtss_core_ref_clk_t(const mesa_core_ref_clk_t *in, vtss_core_ref_clk_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_core_ref_clk_t *tmp = (const vtss_core_ref_clk_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CORE_CLOCK)
#if defined(VTSS_FEATURE_CORE_CLOCK)
mesa_rc mesa_conv_vtss_core_clock_conf_t_to_mesa_core_clock_conf_t(const vtss_core_clock_conf_t *in, mesa_core_clock_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_CORE_CLOCK)
#if defined(VTSS_FEATURE_CORE_CLOCK)
mesa_rc mesa_conv_mesa_core_clock_conf_t_to_vtss_core_clock_conf_t(const mesa_core_clock_conf_t *in, vtss_core_clock_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_CORE_CLOCK)
mesa_rc mesa_conv_vtss_init_conf_t_to_mesa_init_conf_t(const vtss_init_conf_t *in, mesa_init_conf_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_init_conf_t_to_vtss_init_conf_t(const mesa_init_conf_t *in, vtss_init_conf_t *out); /* ag.rb:1595 */
inline static mesa_rc mesa_conv_vtss_spi_endian_t_to_mesa_spi_endian_t(const vtss_spi_endian_t *in, mesa_spi_endian_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_spi_endian_t *tmp = (const mesa_spi_endian_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_spi_endian_t_to_vtss_spi_endian_t(const mesa_spi_endian_t *in, vtss_spi_endian_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_spi_endian_t *tmp = (const vtss_spi_endian_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_spi_bit_order_t_to_mesa_spi_bit_order_t(const vtss_spi_bit_order_t *in, mesa_spi_bit_order_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_spi_bit_order_t *tmp = (const mesa_spi_bit_order_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_spi_bit_order_t_to_vtss_spi_bit_order_t(const mesa_spi_bit_order_t *in, vtss_spi_bit_order_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_spi_bit_order_t *tmp = (const vtss_spi_bit_order_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
mesa_rc mesa_conv_vtss_spi_slave_init_t_to_mesa_spi_slave_init_t(const vtss_spi_slave_init_t *in, mesa_spi_slave_init_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_spi_slave_init_t_to_vtss_spi_slave_init_t(const mesa_spi_slave_init_t *in, vtss_spi_slave_init_t *out); /* ag.rb:1595 */
#if defined(VTSS_FEATURE_WARM_START)
mesa_rc mesa_conv_vtss_restart_t_to_mesa_restart_t(const vtss_restart_t *in, mesa_restart_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_WARM_START)
#if defined(VTSS_FEATURE_WARM_START)
mesa_rc mesa_conv_mesa_restart_t_to_vtss_restart_t(const mesa_restart_t *in, vtss_restart_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_WARM_START)
#if defined(VTSS_FEATURE_WARM_START)
mesa_rc mesa_conv_vtss_restart_status_t_to_mesa_restart_status_t(const vtss_restart_status_t *in, mesa_restart_status_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_WARM_START)
#if defined(VTSS_FEATURE_WARM_START)
mesa_rc mesa_conv_mesa_restart_status_t_to_vtss_restart_status_t(const mesa_restart_status_t *in, vtss_restart_status_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_WARM_START)
#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_conv_vtss_mac_table_entry_t_to_mesa_mac_table_entry_t(const vtss_mac_table_entry_t *in, mesa_mac_table_entry_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_conv_mesa_mac_table_entry_t_to_vtss_mac_table_entry_t(const mesa_mac_table_entry_t *in, vtss_mac_table_entry_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_vtss_mac_table_status_t_to_mesa_mac_table_status_t(const vtss_mac_table_status_t *in, mesa_mac_table_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mac_table_status_t *tmp = (const mesa_mac_table_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_mesa_mac_table_status_t_to_vtss_mac_table_status_t(const mesa_mac_table_status_t *in, vtss_mac_table_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mac_table_status_t *tmp = (const vtss_mac_table_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_vtss_learn_mode_t_to_mesa_learn_mode_t(const vtss_learn_mode_t *in, mesa_learn_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_learn_mode_t *tmp = (const mesa_learn_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_mesa_learn_mode_t_to_vtss_learn_mode_t(const mesa_learn_mode_t *in, vtss_learn_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_learn_mode_t *tmp = (const vtss_learn_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_vtss_stp_state_t_to_mesa_stp_state_t(const vtss_stp_state_t *in, mesa_stp_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_stp_state_t *tmp = (const mesa_stp_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_mesa_stp_state_t_to_vtss_stp_state_t(const mesa_stp_state_t *in, vtss_stp_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_stp_state_t *tmp = (const vtss_stp_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_vtss_vlan_conf_t_to_mesa_vlan_conf_t(const vtss_vlan_conf_t *in, mesa_vlan_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vlan_conf_t *tmp = (const mesa_vlan_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_mesa_vlan_conf_t_to_vtss_vlan_conf_t(const mesa_vlan_conf_t *in, vtss_vlan_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vlan_conf_t *tmp = (const vtss_vlan_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_vtss_vlan_port_type_t_to_mesa_vlan_port_type_t(const vtss_vlan_port_type_t *in, mesa_vlan_port_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vlan_port_type_t *tmp = (const mesa_vlan_port_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_mesa_vlan_port_type_t_to_vtss_vlan_port_type_t(const mesa_vlan_port_type_t *in, vtss_vlan_port_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vlan_port_type_t *tmp = (const vtss_vlan_port_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_conv_vtss_vlan_port_conf_t_to_mesa_vlan_port_conf_t(const vtss_vlan_port_conf_t *in, mesa_vlan_port_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_conv_mesa_vlan_port_conf_t_to_vtss_vlan_port_conf_t(const mesa_vlan_port_conf_t *in, vtss_vlan_port_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_conv_vtss_vlan_vid_conf_t_to_mesa_vlan_vid_conf_t(const vtss_vlan_vid_conf_t *in, mesa_vlan_vid_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_conv_mesa_vlan_vid_conf_t_to_vtss_vlan_vid_conf_t(const mesa_vlan_vid_conf_t *in, vtss_vlan_vid_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_vtss_vlan_tx_tag_t_to_mesa_vlan_tx_tag_t(const vtss_vlan_tx_tag_t *in, mesa_vlan_tx_tag_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vlan_tx_tag_t *tmp = (const mesa_vlan_tx_tag_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_mesa_vlan_tx_tag_t_to_vtss_vlan_tx_tag_t(const mesa_vlan_tx_tag_t *in, vtss_vlan_tx_tag_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vlan_tx_tag_t *tmp = (const vtss_vlan_tx_tag_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VLAN_COUNTERS)
inline static mesa_rc mesa_conv_vtss_vlan_counter_types_t_to_mesa_vlan_counter_types_t(const vtss_vlan_counter_types_t *in, mesa_vlan_counter_types_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vlan_counter_types_t *tmp = (const mesa_vlan_counter_types_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VLAN_COUNTERS)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VLAN_COUNTERS)
inline static mesa_rc mesa_conv_mesa_vlan_counter_types_t_to_vtss_vlan_counter_types_t(const mesa_vlan_counter_types_t *in, vtss_vlan_counter_types_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vlan_counter_types_t *tmp = (const vtss_vlan_counter_types_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VLAN_COUNTERS)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VLAN_COUNTERS)
inline static mesa_rc mesa_conv_vtss_vlan_counters_t_to_mesa_vlan_counters_t(const vtss_vlan_counters_t *in, mesa_vlan_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vlan_counters_t *tmp = (const mesa_vlan_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VLAN_COUNTERS)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VLAN_COUNTERS)
inline static mesa_rc mesa_conv_mesa_vlan_counters_t_to_vtss_vlan_counters_t(const mesa_vlan_counters_t *in, vtss_vlan_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vlan_counters_t *tmp = (const vtss_vlan_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VLAN_COUNTERS)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_vtss_vcl_port_conf_t_to_mesa_vcl_port_conf_t(const vtss_vcl_port_conf_t *in, mesa_vcl_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcl_port_conf_t *tmp = (const mesa_vcl_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_mesa_vcl_port_conf_t_to_vtss_vcl_port_conf_t(const mesa_vcl_port_conf_t *in, vtss_vcl_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcl_port_conf_t *tmp = (const vtss_vcl_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_vtss_vce_type_t_to_mesa_vce_type_t(const vtss_vce_type_t *in, mesa_vce_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vce_type_t *tmp = (const mesa_vce_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_mesa_vce_type_t_to_vtss_vce_type_t(const mesa_vce_type_t *in, vtss_vce_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vce_type_t *tmp = (const vtss_vce_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_vce_mac_t_to_mesa_vce_mac_t(const vtss_vce_mac_t *in, mesa_vce_mac_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_mesa_vce_mac_t_to_vtss_vce_mac_t(const mesa_vce_mac_t *in, vtss_vce_mac_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_vtss_vce_tag_t_to_mesa_vce_tag_t(const vtss_vce_tag_t *in, mesa_vce_tag_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vce_tag_t *tmp = (const mesa_vce_tag_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_mesa_vce_tag_t_to_vtss_vce_tag_t(const mesa_vce_tag_t *in, vtss_vce_tag_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vce_tag_t *tmp = (const vtss_vce_tag_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_vtss_vce_frame_etype_t_to_mesa_vce_frame_etype_t(const vtss_vce_frame_etype_t *in, mesa_vce_frame_etype_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vce_frame_etype_t *tmp = (const mesa_vce_frame_etype_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_mesa_vce_frame_etype_t_to_vtss_vce_frame_etype_t(const mesa_vce_frame_etype_t *in, vtss_vce_frame_etype_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vce_frame_etype_t *tmp = (const vtss_vce_frame_etype_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_vtss_vce_frame_llc_t_to_mesa_vce_frame_llc_t(const vtss_vce_frame_llc_t *in, mesa_vce_frame_llc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vce_frame_llc_t *tmp = (const mesa_vce_frame_llc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_mesa_vce_frame_llc_t_to_vtss_vce_frame_llc_t(const mesa_vce_frame_llc_t *in, vtss_vce_frame_llc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vce_frame_llc_t *tmp = (const vtss_vce_frame_llc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_vtss_vce_frame_snap_t_to_mesa_vce_frame_snap_t(const vtss_vce_frame_snap_t *in, mesa_vce_frame_snap_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vce_frame_snap_t *tmp = (const mesa_vce_frame_snap_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_mesa_vce_frame_snap_t_to_vtss_vce_frame_snap_t(const mesa_vce_frame_snap_t *in, vtss_vce_frame_snap_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vce_frame_snap_t *tmp = (const vtss_vce_frame_snap_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_vce_frame_ipv4_t_to_mesa_vce_frame_ipv4_t(const vtss_vce_frame_ipv4_t *in, mesa_vce_frame_ipv4_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_mesa_vce_frame_ipv4_t_to_vtss_vce_frame_ipv4_t(const mesa_vce_frame_ipv4_t *in, vtss_vce_frame_ipv4_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_vce_frame_ipv6_t_to_mesa_vce_frame_ipv6_t(const vtss_vce_frame_ipv6_t *in, mesa_vce_frame_ipv6_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_mesa_vce_frame_ipv6_t_to_vtss_vce_frame_ipv6_t(const mesa_vce_frame_ipv6_t *in, vtss_vce_frame_ipv6_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_vce_key_t_to_mesa_vce_key_t(const vtss_vce_key_t *in, mesa_vce_key_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_mesa_vce_key_t_to_vtss_vce_key_t(const mesa_vce_key_t *in, vtss_vce_key_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_vtss_imap_sel_t_to_mesa_imap_sel_t(const vtss_imap_sel_t *in, mesa_imap_sel_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_imap_sel_t *tmp = (const mesa_imap_sel_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_mesa_imap_sel_t_to_vtss_imap_sel_t(const mesa_imap_sel_t *in, vtss_imap_sel_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_imap_sel_t *tmp = (const vtss_imap_sel_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_vtss_oam_detect_t_to_mesa_oam_detect_t(const vtss_oam_detect_t *in, mesa_oam_detect_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_oam_detect_t *tmp = (const mesa_oam_detect_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_mesa_oam_detect_t_to_vtss_oam_detect_t(const mesa_oam_detect_t *in, vtss_oam_detect_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_oam_detect_t *tmp = (const vtss_oam_detect_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_vce_action_t_to_mesa_vce_action_t(const vtss_vce_action_t *in, mesa_vce_action_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_mesa_vce_action_t_to_vtss_vce_action_t(const mesa_vce_action_t *in, vtss_vce_action_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_vce_t_to_mesa_vce_t(const vtss_vce_t *in, mesa_vce_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_mesa_vce_t_to_vtss_vce_t(const mesa_vce_t *in, vtss_vce_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XSTAT)
mesa_rc mesa_conv_vtss_ingress_counters_t_to_mesa_ingress_counters_t(const vtss_ingress_counters_t *in, mesa_ingress_counters_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_XSTAT)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XSTAT)
mesa_rc mesa_conv_mesa_ingress_counters_t_to_vtss_ingress_counters_t(const mesa_ingress_counters_t *in, vtss_ingress_counters_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_XSTAT)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XDLB)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_vtss_dlb_policer_status_t_to_mesa_dlb_policer_status_t(const vtss_dlb_policer_status_t *in, mesa_dlb_policer_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_dlb_policer_status_t *tmp = (const mesa_dlb_policer_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#endif  // defined(VTSS_FEATURE_XDLB)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XDLB)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_mesa_dlb_policer_status_t_to_vtss_dlb_policer_status_t(const mesa_dlb_policer_status_t *in, vtss_dlb_policer_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_dlb_policer_status_t *tmp = (const vtss_dlb_policer_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#endif  // defined(VTSS_FEATURE_XDLB)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XFLOW)
mesa_rc mesa_conv_vtss_iflow_conf_t_to_mesa_iflow_conf_t(const vtss_iflow_conf_t *in, mesa_iflow_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_XFLOW)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XFLOW)
mesa_rc mesa_conv_mesa_iflow_conf_t_to_vtss_iflow_conf_t(const mesa_iflow_conf_t *in, vtss_iflow_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_XFLOW)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_tce_key_t_to_mesa_tce_key_t(const vtss_tce_key_t *in, mesa_tce_key_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_mesa_tce_key_t_to_vtss_tce_key_t(const mesa_tce_key_t *in, vtss_tce_key_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_vtss_tpid_sel_t_to_mesa_tpid_sel_t(const vtss_tpid_sel_t *in, mesa_tpid_sel_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_tpid_sel_t *tmp = (const mesa_tpid_sel_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_mesa_tpid_sel_t_to_vtss_tpid_sel_t(const mesa_tpid_sel_t *in, vtss_tpid_sel_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_tpid_sel_t *tmp = (const vtss_tpid_sel_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_vtss_pcp_sel_t_to_mesa_pcp_sel_t(const vtss_pcp_sel_t *in, mesa_pcp_sel_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_pcp_sel_t *tmp = (const mesa_pcp_sel_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_mesa_pcp_sel_t_to_vtss_pcp_sel_t(const mesa_pcp_sel_t *in, vtss_pcp_sel_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_pcp_sel_t *tmp = (const vtss_pcp_sel_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_vtss_dei_sel_t_to_mesa_dei_sel_t(const vtss_dei_sel_t *in, mesa_dei_sel_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_dei_sel_t *tmp = (const mesa_dei_sel_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_mesa_dei_sel_t_to_vtss_dei_sel_t(const mesa_dei_sel_t *in, vtss_dei_sel_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_dei_sel_t *tmp = (const vtss_dei_sel_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_tce_tag_t_to_mesa_tce_tag_t(const vtss_tce_tag_t *in, mesa_tce_tag_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_mesa_tce_tag_t_to_vtss_tce_tag_t(const mesa_tce_tag_t *in, vtss_tce_tag_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
#if defined(VTSS_FEATURE_FRER)
inline static mesa_rc mesa_conv_vtss_rtag_sel_t_to_mesa_rtag_sel_t(const vtss_rtag_sel_t *in, mesa_rtag_sel_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rtag_sel_t *tmp = (const mesa_rtag_sel_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FRER)
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
#if defined(VTSS_FEATURE_FRER)
inline static mesa_rc mesa_conv_mesa_rtag_sel_t_to_vtss_rtag_sel_t(const mesa_rtag_sel_t *in, vtss_rtag_sel_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rtag_sel_t *tmp = (const vtss_rtag_sel_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FRER)
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
#if defined(VTSS_FEATURE_FRER)
inline static mesa_rc mesa_conv_vtss_tce_rtag_t_to_mesa_tce_rtag_t(const vtss_tce_rtag_t *in, mesa_tce_rtag_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_tce_rtag_t *tmp = (const mesa_tce_rtag_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FRER)
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
#if defined(VTSS_FEATURE_FRER)
inline static mesa_rc mesa_conv_mesa_tce_rtag_t_to_vtss_tce_rtag_t(const mesa_tce_rtag_t *in, vtss_tce_rtag_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_tce_rtag_t *tmp = (const vtss_tce_rtag_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FRER)
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_tce_action_t_to_mesa_tce_action_t(const vtss_tce_action_t *in, mesa_tce_action_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_mesa_tce_action_t_to_vtss_tce_action_t(const mesa_tce_action_t *in, vtss_tce_action_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_tce_t_to_mesa_tce_t(const vtss_tce_t *in, mesa_tce_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_mesa_tce_t_to_vtss_tce_t(const mesa_tce_t *in, vtss_tce_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XSTAT)
inline static mesa_rc mesa_conv_vtss_egress_counters_t_to_mesa_egress_counters_t(const vtss_egress_counters_t *in, mesa_egress_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_egress_counters_t *tmp = (const mesa_egress_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_XSTAT)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XSTAT)
inline static mesa_rc mesa_conv_mesa_egress_counters_t_to_vtss_egress_counters_t(const mesa_egress_counters_t *in, vtss_egress_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_egress_counters_t *tmp = (const vtss_egress_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_XSTAT)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XFLOW)
mesa_rc mesa_conv_vtss_eflow_conf_t_to_mesa_eflow_conf_t(const vtss_eflow_conf_t *in, mesa_eflow_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_XFLOW)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XFLOW)
mesa_rc mesa_conv_mesa_eflow_conf_t_to_vtss_eflow_conf_t(const mesa_eflow_conf_t *in, vtss_eflow_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_XFLOW)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_vlan_trans_port2grp_conf_t_to_mesa_vlan_trans_port2grp_conf_t(const vtss_vlan_trans_port2grp_conf_t *in, mesa_vlan_trans_port2grp_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_mesa_vlan_trans_port2grp_conf_t_to_vtss_vlan_trans_port2grp_conf_t(const mesa_vlan_trans_port2grp_conf_t *in, vtss_vlan_trans_port2grp_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_vtss_vlan_trans_dir_t_to_mesa_vlan_trans_dir_t(const vtss_vlan_trans_dir_t *in, mesa_vlan_trans_dir_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vlan_trans_dir_t *tmp = (const mesa_vlan_trans_dir_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_mesa_vlan_trans_dir_t_to_vtss_vlan_trans_dir_t(const mesa_vlan_trans_dir_t *in, vtss_vlan_trans_dir_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vlan_trans_dir_t *tmp = (const vtss_vlan_trans_dir_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_vtss_vlan_trans_grp2vlan_conf_t_to_mesa_vlan_trans_grp2vlan_conf_t(const vtss_vlan_trans_grp2vlan_conf_t *in, mesa_vlan_trans_grp2vlan_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vlan_trans_grp2vlan_conf_t *tmp = (const mesa_vlan_trans_grp2vlan_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
inline static mesa_rc mesa_conv_mesa_vlan_trans_grp2vlan_conf_t_to_vtss_vlan_trans_grp2vlan_conf_t(const mesa_vlan_trans_grp2vlan_conf_t *in, vtss_vlan_trans_grp2vlan_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vlan_trans_grp2vlan_conf_t *tmp = (const vtss_vlan_trans_grp2vlan_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
inline static mesa_rc mesa_conv_vtss_vcap_port_conf_t_to_mesa_vcap_port_conf_t(const vtss_vcap_port_conf_t *in, mesa_vcap_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcap_port_conf_t *tmp = (const mesa_vcap_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
inline static mesa_rc mesa_conv_mesa_vcap_port_conf_t_to_vtss_vcap_port_conf_t(const mesa_vcap_port_conf_t *in, vtss_vcap_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcap_port_conf_t *tmp = (const vtss_vcap_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_vtss_dgroup_port_conf_t_to_mesa_dgroup_port_conf_t(const vtss_dgroup_port_conf_t *in, mesa_dgroup_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_dgroup_port_conf_t *tmp = (const mesa_dgroup_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_mesa_dgroup_port_conf_t_to_vtss_dgroup_port_conf_t(const mesa_dgroup_port_conf_t *in, vtss_dgroup_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_dgroup_port_conf_t *tmp = (const vtss_dgroup_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_vtss_sflow_type_t_to_mesa_sflow_type_t(const vtss_sflow_type_t *in, mesa_sflow_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_sflow_type_t *tmp = (const mesa_sflow_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_sflow_type_t_to_vtss_sflow_type_t(const mesa_sflow_type_t *in, vtss_sflow_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_sflow_type_t *tmp = (const vtss_sflow_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_vtss_sflow_port_conf_t_to_mesa_sflow_port_conf_t(const vtss_sflow_port_conf_t *in, mesa_sflow_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_sflow_port_conf_t *tmp = (const mesa_sflow_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_mesa_sflow_port_conf_t_to_vtss_sflow_port_conf_t(const mesa_sflow_port_conf_t *in, vtss_sflow_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_sflow_port_conf_t *tmp = (const vtss_sflow_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_vtss_aggr_mode_t_to_mesa_aggr_mode_t(const vtss_aggr_mode_t *in, mesa_aggr_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_aggr_mode_t *tmp = (const mesa_aggr_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_aggr_mode_t_to_vtss_aggr_mode_t(const mesa_aggr_mode_t *in, vtss_aggr_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_aggr_mode_t *tmp = (const vtss_aggr_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
inline static mesa_rc mesa_conv_vtss_mirror_tag_t_to_mesa_mirror_tag_t(const vtss_mirror_tag_t *in, mesa_mirror_tag_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mirror_tag_t *tmp = (const mesa_mirror_tag_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
inline static mesa_rc mesa_conv_mesa_mirror_tag_t_to_vtss_mirror_tag_t(const mesa_mirror_tag_t *in, vtss_mirror_tag_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mirror_tag_t *tmp = (const vtss_mirror_tag_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_conv_vtss_mirror_conf_t_to_mesa_mirror_conf_t(const vtss_mirror_conf_t *in, mesa_mirror_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_conv_mesa_mirror_conf_t_to_vtss_mirror_conf_t(const mesa_mirror_conf_t *in, vtss_mirror_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_vtss_eps_port_type_t_to_mesa_eps_port_type_t(const vtss_eps_port_type_t *in, mesa_eps_port_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_eps_port_type_t *tmp = (const mesa_eps_port_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_mesa_eps_port_type_t_to_vtss_eps_port_type_t(const mesa_eps_port_type_t *in, vtss_eps_port_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_eps_port_type_t *tmp = (const vtss_eps_port_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_vtss_eps_port_conf_t_to_mesa_eps_port_conf_t(const vtss_eps_port_conf_t *in, mesa_eps_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_eps_port_conf_t *tmp = (const mesa_eps_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_mesa_eps_port_conf_t_to_vtss_eps_port_conf_t(const mesa_eps_port_conf_t *in, vtss_eps_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_eps_port_conf_t *tmp = (const vtss_eps_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_vtss_eps_selector_t_to_mesa_eps_selector_t(const vtss_eps_selector_t *in, mesa_eps_selector_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_eps_selector_t *tmp = (const mesa_eps_selector_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_mesa_eps_selector_t_to_vtss_eps_selector_t(const mesa_eps_selector_t *in, vtss_eps_selector_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_eps_selector_t *tmp = (const vtss_eps_selector_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_L2_ERPS)
inline static mesa_rc mesa_conv_vtss_erps_state_t_to_mesa_erps_state_t(const vtss_erps_state_t *in, mesa_erps_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_erps_state_t *tmp = (const mesa_erps_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_L2_ERPS)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_L2_ERPS)
inline static mesa_rc mesa_conv_mesa_erps_state_t_to_vtss_erps_state_t(const mesa_erps_state_t *in, vtss_erps_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_erps_state_t *tmp = (const vtss_erps_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_L2_ERPS)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_RCL)
mesa_rc mesa_conv_vtss_rcl_vid_conf_t_to_mesa_rcl_vid_conf_t(const vtss_rcl_vid_conf_t *in, mesa_rcl_vid_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_RCL)
#if defined(VTSS_FEATURE_RCL)
mesa_rc mesa_conv_mesa_rcl_vid_conf_t_to_vtss_rcl_vid_conf_t(const mesa_rcl_vid_conf_t *in, vtss_rcl_vid_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_RCL)
#if defined(VTSS_FEATURE_RCL)
inline static mesa_rc mesa_conv_vtss_rcl_etype_t_to_mesa_rcl_etype_t(const vtss_rcl_etype_t *in, mesa_rcl_etype_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rcl_etype_t *tmp = (const mesa_rcl_etype_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_RCL)
#if defined(VTSS_FEATURE_RCL)
inline static mesa_rc mesa_conv_mesa_rcl_etype_t_to_vtss_rcl_etype_t(const mesa_rcl_etype_t *in, vtss_rcl_etype_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rcl_etype_t *tmp = (const vtss_rcl_etype_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_RCL)
#if defined(VTSS_FEATURE_RCL)
inline static mesa_rc mesa_conv_vtss_rce_key_t_to_mesa_rce_key_t(const vtss_rce_key_t *in, mesa_rce_key_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rce_key_t *tmp = (const mesa_rce_key_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_RCL)
#if defined(VTSS_FEATURE_RCL)
inline static mesa_rc mesa_conv_mesa_rce_key_t_to_vtss_rce_key_t(const mesa_rce_key_t *in, vtss_rce_key_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rce_key_t *tmp = (const vtss_rce_key_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_RCL)
#if defined(VTSS_FEATURE_RCL)
mesa_rc mesa_conv_vtss_rce_action_t_to_mesa_rce_action_t(const vtss_rce_action_t *in, mesa_rce_action_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_RCL)
#if defined(VTSS_FEATURE_RCL)
mesa_rc mesa_conv_mesa_rce_action_t_to_vtss_rce_action_t(const mesa_rce_action_t *in, vtss_rce_action_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_RCL)
#if defined(VTSS_FEATURE_RCL)
mesa_rc mesa_conv_vtss_rce_t_to_mesa_rce_t(const vtss_rce_t *in, mesa_rce_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_RCL)
#if defined(VTSS_FEATURE_RCL)
mesa_rc mesa_conv_mesa_rce_t_to_vtss_rce_t(const mesa_rce_t *in, vtss_rce_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_RCL)
#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_conv_vtss_rb_cap_t_to_mesa_rb_cap_t(const vtss_rb_cap_t *in, mesa_rb_cap_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_conv_mesa_rb_cap_t_to_vtss_rb_cap_t(const mesa_rb_cap_t *in, vtss_rb_cap_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_vtss_rb_mode_t_to_mesa_rb_mode_t(const vtss_rb_mode_t *in, mesa_rb_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rb_mode_t *tmp = (const mesa_rb_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_mesa_rb_mode_t_to_vtss_rb_mode_t(const mesa_rb_mode_t *in, vtss_rb_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rb_mode_t *tmp = (const vtss_rb_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_vtss_rb_sv_t_to_mesa_rb_sv_t(const vtss_rb_sv_t *in, mesa_rb_sv_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rb_sv_t *tmp = (const mesa_rb_sv_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_mesa_rb_sv_t_to_vtss_rb_sv_t(const mesa_rb_sv_t *in, vtss_rb_sv_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rb_sv_t *tmp = (const vtss_rb_sv_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_vtss_rb_conf_t_to_mesa_rb_conf_t(const vtss_rb_conf_t *in, mesa_rb_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rb_conf_t *tmp = (const mesa_rb_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_mesa_rb_conf_t_to_vtss_rb_conf_t(const mesa_rb_conf_t *in, vtss_rb_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rb_conf_t *tmp = (const vtss_rb_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_vtss_rb_port_counters_t_to_mesa_rb_port_counters_t(const vtss_rb_port_counters_t *in, mesa_rb_port_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rb_port_counters_t *tmp = (const mesa_rb_port_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_mesa_rb_port_counters_t_to_vtss_rb_port_counters_t(const mesa_rb_port_counters_t *in, vtss_rb_port_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rb_port_counters_t *tmp = (const vtss_rb_port_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_vtss_rb_counters_t_to_mesa_rb_counters_t(const vtss_rb_counters_t *in, mesa_rb_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rb_counters_t *tmp = (const mesa_rb_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_mesa_rb_counters_t_to_vtss_rb_counters_t(const mesa_rb_counters_t *in, vtss_rb_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rb_counters_t *tmp = (const vtss_rb_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_vtss_rb_node_type_t_to_mesa_rb_node_type_t(const vtss_rb_node_type_t *in, mesa_rb_node_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rb_node_type_t *tmp = (const mesa_rb_node_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_mesa_rb_node_type_t_to_vtss_rb_node_type_t(const mesa_rb_node_type_t *in, vtss_rb_node_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rb_node_type_t *tmp = (const vtss_rb_node_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_vtss_rb_node_conf_t_to_mesa_rb_node_conf_t(const vtss_rb_node_conf_t *in, mesa_rb_node_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rb_node_conf_t *tmp = (const mesa_rb_node_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_mesa_rb_node_conf_t_to_vtss_rb_node_conf_t(const mesa_rb_node_conf_t *in, vtss_rb_node_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rb_node_conf_t *tmp = (const vtss_rb_node_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_vtss_rb_clear_t_to_mesa_rb_clear_t(const vtss_rb_clear_t *in, mesa_rb_clear_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rb_clear_t *tmp = (const mesa_rb_clear_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_mesa_rb_clear_t_to_vtss_rb_clear_t(const mesa_rb_clear_t *in, vtss_rb_clear_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rb_clear_t *tmp = (const vtss_rb_clear_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_vtss_rb_node_counters_t_to_mesa_rb_node_counters_t(const vtss_rb_node_counters_t *in, mesa_rb_node_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rb_node_counters_t *tmp = (const mesa_rb_node_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_mesa_rb_node_counters_t_to_vtss_rb_node_counters_t(const mesa_rb_node_counters_t *in, vtss_rb_node_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rb_node_counters_t *tmp = (const vtss_rb_node_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_vtss_rb_node_port_t_to_mesa_rb_node_port_t(const vtss_rb_node_port_t *in, mesa_rb_node_port_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rb_node_port_t *tmp = (const mesa_rb_node_port_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_mesa_rb_node_port_t_to_vtss_rb_node_port_t(const mesa_rb_node_port_t *in, vtss_rb_node_port_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rb_node_port_t *tmp = (const vtss_rb_node_port_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_vtss_rb_node_t_to_mesa_rb_node_t(const vtss_rb_node_t *in, mesa_rb_node_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rb_node_t *tmp = (const mesa_rb_node_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_mesa_rb_node_t_to_vtss_rb_node_t(const mesa_rb_node_t *in, vtss_rb_node_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rb_node_t *tmp = (const vtss_rb_node_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_vtss_rb_proxy_node_type_t_to_mesa_rb_proxy_node_type_t(const vtss_rb_proxy_node_type_t *in, mesa_rb_proxy_node_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rb_proxy_node_type_t *tmp = (const mesa_rb_proxy_node_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_mesa_rb_proxy_node_type_t_to_vtss_rb_proxy_node_type_t(const mesa_rb_proxy_node_type_t *in, vtss_rb_proxy_node_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rb_proxy_node_type_t *tmp = (const vtss_rb_proxy_node_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_vtss_rb_proxy_node_conf_t_to_mesa_rb_proxy_node_conf_t(const vtss_rb_proxy_node_conf_t *in, mesa_rb_proxy_node_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rb_proxy_node_conf_t *tmp = (const mesa_rb_proxy_node_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_mesa_rb_proxy_node_conf_t_to_vtss_rb_proxy_node_conf_t(const mesa_rb_proxy_node_conf_t *in, vtss_rb_proxy_node_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rb_proxy_node_conf_t *tmp = (const vtss_rb_proxy_node_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_vtss_rb_proxy_node_counters_t_to_mesa_rb_proxy_node_counters_t(const vtss_rb_proxy_node_counters_t *in, mesa_rb_proxy_node_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rb_proxy_node_counters_t *tmp = (const mesa_rb_proxy_node_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_mesa_rb_proxy_node_counters_t_to_vtss_rb_proxy_node_counters_t(const mesa_rb_proxy_node_counters_t *in, vtss_rb_proxy_node_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rb_proxy_node_counters_t *tmp = (const vtss_rb_proxy_node_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_vtss_rb_proxy_node_t_to_mesa_rb_proxy_node_t(const vtss_rb_proxy_node_t *in, mesa_rb_proxy_node_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rb_proxy_node_t *tmp = (const mesa_rb_proxy_node_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_REDBOX)
inline static mesa_rc mesa_conv_mesa_rb_proxy_node_t_to_vtss_rb_proxy_node_t(const mesa_rb_proxy_node_t *in, vtss_rb_proxy_node_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rb_proxy_node_t *tmp = (const vtss_rb_proxy_node_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)
#if defined(VTSS_FEATURE_LAYER3)
inline static mesa_rc mesa_conv_vtss_l3_rleg_common_mode_t_to_mesa_l3_rleg_common_mode_t(const vtss_l3_rleg_common_mode_t *in, mesa_l3_rleg_common_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_l3_rleg_common_mode_t *tmp = (const mesa_l3_rleg_common_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER3)
#if defined(VTSS_FEATURE_LAYER3)
inline static mesa_rc mesa_conv_mesa_l3_rleg_common_mode_t_to_vtss_l3_rleg_common_mode_t(const mesa_l3_rleg_common_mode_t *in, vtss_l3_rleg_common_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_l3_rleg_common_mode_t *tmp = (const vtss_l3_rleg_common_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER3)
#if defined(VTSS_FEATURE_LAYER3)
inline static mesa_rc mesa_conv_vtss_l3_common_conf_t_to_mesa_l3_common_conf_t(const vtss_l3_common_conf_t *in, mesa_l3_common_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_l3_common_conf_t *tmp = (const mesa_l3_common_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER3)
#if defined(VTSS_FEATURE_LAYER3)
inline static mesa_rc mesa_conv_mesa_l3_common_conf_t_to_vtss_l3_common_conf_t(const mesa_l3_common_conf_t *in, vtss_l3_common_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_l3_common_conf_t *tmp = (const vtss_l3_common_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER3)
#if defined(VTSS_FEATURE_LAYER3)
inline static mesa_rc mesa_conv_vtss_l3_rleg_conf_t_to_mesa_l3_rleg_conf_t(const vtss_l3_rleg_conf_t *in, mesa_l3_rleg_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_l3_rleg_conf_t *tmp = (const mesa_l3_rleg_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER3)
#if defined(VTSS_FEATURE_LAYER3)
inline static mesa_rc mesa_conv_mesa_l3_rleg_conf_t_to_vtss_l3_rleg_conf_t(const mesa_l3_rleg_conf_t *in, vtss_l3_rleg_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_l3_rleg_conf_t *tmp = (const vtss_l3_rleg_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER3)
#if defined(VTSS_FEATURE_LAYER3)
inline static mesa_rc mesa_conv_vtss_l3_neighbour_type_t_to_mesa_l3_neighbour_type_t(const vtss_l3_neighbour_type_t *in, mesa_l3_neighbour_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_l3_neighbour_type_t *tmp = (const mesa_l3_neighbour_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER3)
#if defined(VTSS_FEATURE_LAYER3)
inline static mesa_rc mesa_conv_mesa_l3_neighbour_type_t_to_vtss_l3_neighbour_type_t(const mesa_l3_neighbour_type_t *in, vtss_l3_neighbour_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_l3_neighbour_type_t *tmp = (const vtss_l3_neighbour_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER3)
#if defined(VTSS_FEATURE_LAYER3)
inline static mesa_rc mesa_conv_vtss_l3_neighbour_t_to_mesa_l3_neighbour_t(const vtss_l3_neighbour_t *in, mesa_l3_neighbour_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_l3_neighbour_t *tmp = (const mesa_l3_neighbour_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER3)
#if defined(VTSS_FEATURE_LAYER3)
inline static mesa_rc mesa_conv_mesa_l3_neighbour_t_to_vtss_l3_neighbour_t(const mesa_l3_neighbour_t *in, vtss_l3_neighbour_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_l3_neighbour_t *tmp = (const vtss_l3_neighbour_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER3)
inline static mesa_rc mesa_conv_vtss_trace_layer_t_to_mesa_trace_layer_t(const vtss_trace_layer_t *in, mesa_trace_layer_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_trace_layer_t *tmp = (const mesa_trace_layer_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_trace_layer_t_to_vtss_trace_layer_t(const mesa_trace_layer_t *in, vtss_trace_layer_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_trace_layer_t *tmp = (const vtss_trace_layer_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_trace_group_t_to_mesa_trace_group_t(const vtss_trace_group_t *in, mesa_trace_group_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_trace_group_t *tmp = (const mesa_trace_group_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_trace_group_t_to_vtss_trace_group_t(const mesa_trace_group_t *in, vtss_trace_group_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_trace_group_t *tmp = (const vtss_trace_group_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_trace_level_t_to_mesa_trace_level_t(const vtss_trace_level_t *in, mesa_trace_level_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_trace_level_t *tmp = (const mesa_trace_level_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_trace_level_t_to_vtss_trace_level_t(const mesa_trace_level_t *in, vtss_trace_level_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_trace_level_t *tmp = (const vtss_trace_level_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_trace_conf_t_to_mesa_trace_conf_t(const vtss_trace_conf_t *in, mesa_trace_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_trace_conf_t *tmp = (const mesa_trace_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_trace_conf_t_to_vtss_trace_conf_t(const mesa_trace_conf_t *in, vtss_trace_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_trace_conf_t *tmp = (const vtss_trace_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#if VTSS_OPT_DEBUG_PRINT
inline static mesa_rc mesa_conv_vtss_debug_layer_t_to_mesa_debug_layer_t(const vtss_debug_layer_t *in, mesa_debug_layer_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_debug_layer_t *tmp = (const mesa_debug_layer_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // VTSS_OPT_DEBUG_PRINT
#if VTSS_OPT_DEBUG_PRINT
inline static mesa_rc mesa_conv_mesa_debug_layer_t_to_vtss_debug_layer_t(const mesa_debug_layer_t *in, vtss_debug_layer_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_debug_layer_t *tmp = (const vtss_debug_layer_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // VTSS_OPT_DEBUG_PRINT
#if VTSS_OPT_DEBUG_PRINT
inline static mesa_rc mesa_conv_vtss_debug_group_t_to_mesa_debug_group_t(const vtss_debug_group_t *in, mesa_debug_group_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_debug_group_t *tmp = (const mesa_debug_group_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // VTSS_OPT_DEBUG_PRINT
#if VTSS_OPT_DEBUG_PRINT
inline static mesa_rc mesa_conv_mesa_debug_group_t_to_vtss_debug_group_t(const mesa_debug_group_t *in, vtss_debug_group_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_debug_group_t *tmp = (const vtss_debug_group_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // VTSS_OPT_DEBUG_PRINT
#if VTSS_OPT_DEBUG_PRINT
mesa_rc mesa_conv_vtss_debug_info_t_to_mesa_debug_info_t(const vtss_debug_info_t *in, mesa_debug_info_t *out); /* ag.rb:1595 */
#endif  // VTSS_OPT_DEBUG_PRINT
#if VTSS_OPT_DEBUG_PRINT
mesa_rc mesa_conv_mesa_debug_info_t_to_vtss_debug_info_t(const mesa_debug_info_t *in, vtss_debug_info_t *out); /* ag.rb:1595 */
#endif  // VTSS_OPT_DEBUG_PRINT
inline static mesa_rc mesa_conv_vtss_api_lock_t_to_mesa_api_lock_t(const vtss_api_lock_t *in, mesa_api_lock_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_api_lock_t *tmp = (const mesa_api_lock_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_api_lock_t_to_vtss_api_lock_t(const mesa_api_lock_t *in, vtss_api_lock_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_api_lock_t *tmp = (const vtss_api_lock_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_debug_lock_t_to_mesa_debug_lock_t(const vtss_debug_lock_t *in, mesa_debug_lock_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_debug_lock_t *tmp = (const mesa_debug_lock_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_debug_lock_t_to_vtss_debug_lock_t(const mesa_debug_lock_t *in, vtss_debug_lock_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_debug_lock_t *tmp = (const vtss_debug_lock_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#if defined(VTSS_FEATURE_MISC)
inline static mesa_rc mesa_conv_vtss_chip_id_t_to_mesa_chip_id_t(const vtss_chip_id_t *in, mesa_chip_id_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_chip_id_t *tmp = (const mesa_chip_id_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MISC)
#if defined(VTSS_FEATURE_MISC)
inline static mesa_rc mesa_conv_mesa_chip_id_t_to_vtss_chip_id_t(const mesa_chip_id_t *in, vtss_chip_id_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_chip_id_t *tmp = (const vtss_chip_id_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MISC)
#if defined(VTSS_FEATURE_MISC)
inline static mesa_rc mesa_conv_vtss_ptp_event_type_t_to_mesa_ptp_event_type_t(const vtss_ptp_event_type_t *in, mesa_ptp_event_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ptp_event_type_t *tmp = (const mesa_ptp_event_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MISC)
#if defined(VTSS_FEATURE_MISC)
inline static mesa_rc mesa_conv_mesa_ptp_event_type_t_to_vtss_ptp_event_type_t(const mesa_ptp_event_type_t *in, vtss_ptp_event_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ptp_event_type_t *tmp = (const vtss_ptp_event_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MISC)
#if defined(VTSS_FEATURE_MISC)
inline static mesa_rc mesa_conv_vtss_dev_all_event_type_t_to_mesa_dev_all_event_type_t(const vtss_dev_all_event_type_t *in, mesa_dev_all_event_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_dev_all_event_type_t *tmp = (const mesa_dev_all_event_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MISC)
#if defined(VTSS_FEATURE_MISC)
inline static mesa_rc mesa_conv_mesa_dev_all_event_type_t_to_vtss_dev_all_event_type_t(const mesa_dev_all_event_type_t *in, vtss_dev_all_event_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_dev_all_event_type_t *tmp = (const vtss_dev_all_event_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MISC)
#if defined(VTSS_FEATURE_MISC)
inline static mesa_rc mesa_conv_vtss_dev_all_event_poll_t_to_mesa_dev_all_event_poll_t(const vtss_dev_all_event_poll_t *in, mesa_dev_all_event_poll_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_dev_all_event_poll_t *tmp = (const mesa_dev_all_event_poll_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MISC)
#if defined(VTSS_FEATURE_MISC)
inline static mesa_rc mesa_conv_mesa_dev_all_event_poll_t_to_vtss_dev_all_event_poll_t(const mesa_dev_all_event_poll_t *in, vtss_dev_all_event_poll_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_dev_all_event_poll_t *tmp = (const vtss_dev_all_event_poll_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MISC)
#if defined(VTSS_GPIOS)
inline static mesa_rc mesa_conv_vtss_gpio_mode_t_to_mesa_gpio_mode_t(const vtss_gpio_mode_t *in, mesa_gpio_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_gpio_mode_t *tmp = (const mesa_gpio_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_GPIOS)
#if defined(VTSS_GPIOS)
inline static mesa_rc mesa_conv_mesa_gpio_mode_t_to_vtss_gpio_mode_t(const mesa_gpio_mode_t *in, vtss_gpio_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_gpio_mode_t *tmp = (const vtss_gpio_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_GPIOS)
#if defined(VTSS_GPIOS)
inline static mesa_rc mesa_conv_vtss_gpio_func_t_to_mesa_gpio_func_t(const vtss_gpio_func_t *in, mesa_gpio_func_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_gpio_func_t *tmp = (const mesa_gpio_func_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_GPIOS)
#if defined(VTSS_GPIOS)
inline static mesa_rc mesa_conv_mesa_gpio_func_t_to_vtss_gpio_func_t(const mesa_gpio_func_t *in, vtss_gpio_func_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_gpio_func_t *tmp = (const vtss_gpio_func_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_GPIOS)
#if defined(VTSS_GPIOS)
inline static mesa_rc mesa_conv_vtss_gpio_func_alt_t_to_mesa_gpio_func_alt_t(const vtss_gpio_func_alt_t *in, mesa_gpio_func_alt_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_gpio_func_alt_t *tmp = (const mesa_gpio_func_alt_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_GPIOS)
#if defined(VTSS_GPIOS)
inline static mesa_rc mesa_conv_mesa_gpio_func_alt_t_to_vtss_gpio_func_alt_t(const mesa_gpio_func_alt_t *in, vtss_gpio_func_alt_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_gpio_func_alt_t *tmp = (const vtss_gpio_func_alt_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_GPIOS)
#if defined(VTSS_GPIOS)
inline static mesa_rc mesa_conv_vtss_gpio_func_info_t_to_mesa_gpio_func_info_t(const vtss_gpio_func_info_t *in, mesa_gpio_func_info_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_gpio_func_info_t *tmp = (const mesa_gpio_func_info_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_GPIOS)
#if defined(VTSS_GPIOS)
inline static mesa_rc mesa_conv_mesa_gpio_func_info_t_to_vtss_gpio_func_info_t(const mesa_gpio_func_info_t *in, vtss_gpio_func_info_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_gpio_func_info_t *tmp = (const vtss_gpio_func_info_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_GPIOS)
#if defined(VTSS_GPIOS)
inline static mesa_rc mesa_conv_vtss_gpio_func_info_get_t_to_mesa_gpio_func_info_get_t(const vtss_gpio_func_info_get_t *in, mesa_gpio_func_info_get_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_gpio_func_info_get_t *tmp = (const mesa_gpio_func_info_get_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_GPIOS)
#if defined(VTSS_GPIOS)
inline static mesa_rc mesa_conv_mesa_gpio_func_info_get_t_to_vtss_gpio_func_info_get_t(const mesa_gpio_func_info_get_t *in, vtss_gpio_func_info_get_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_gpio_func_info_get_t *tmp = (const vtss_gpio_func_info_get_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_GPIOS)
#if defined(VTSS_FEATURE_SERIAL_GPIO)
inline static mesa_rc mesa_conv_vtss_sgpio_mode_t_to_mesa_sgpio_mode_t(const vtss_sgpio_mode_t *in, mesa_sgpio_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_sgpio_mode_t *tmp = (const mesa_sgpio_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO)
#if defined(VTSS_FEATURE_SERIAL_GPIO)
inline static mesa_rc mesa_conv_mesa_sgpio_mode_t_to_vtss_sgpio_mode_t(const mesa_sgpio_mode_t *in, vtss_sgpio_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_sgpio_mode_t *tmp = (const vtss_sgpio_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO)
#if defined(VTSS_FEATURE_SERIAL_GPIO)
inline static mesa_rc mesa_conv_vtss_sgpio_bmode_t_to_mesa_sgpio_bmode_t(const vtss_sgpio_bmode_t *in, mesa_sgpio_bmode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_sgpio_bmode_t *tmp = (const mesa_sgpio_bmode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO)
#if defined(VTSS_FEATURE_SERIAL_GPIO)
inline static mesa_rc mesa_conv_mesa_sgpio_bmode_t_to_vtss_sgpio_bmode_t(const mesa_sgpio_bmode_t *in, vtss_sgpio_bmode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_sgpio_bmode_t *tmp = (const vtss_sgpio_bmode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO)
#if defined(VTSS_FEATURE_SERIAL_GPIO)
inline static mesa_rc mesa_conv_vtss_sgpio_port_conf_t_to_mesa_sgpio_port_conf_t(const vtss_sgpio_port_conf_t *in, mesa_sgpio_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_sgpio_port_conf_t *tmp = (const mesa_sgpio_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO)
#if defined(VTSS_FEATURE_SERIAL_GPIO)
inline static mesa_rc mesa_conv_mesa_sgpio_port_conf_t_to_vtss_sgpio_port_conf_t(const mesa_sgpio_port_conf_t *in, vtss_sgpio_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_sgpio_port_conf_t *tmp = (const vtss_sgpio_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO)
#if defined(VTSS_FEATURE_SERIAL_GPIO)
inline static mesa_rc mesa_conv_vtss_sgpio_conf_t_to_mesa_sgpio_conf_t(const vtss_sgpio_conf_t *in, mesa_sgpio_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_sgpio_conf_t *tmp = (const mesa_sgpio_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO)
#if defined(VTSS_FEATURE_SERIAL_GPIO)
inline static mesa_rc mesa_conv_mesa_sgpio_conf_t_to_vtss_sgpio_conf_t(const mesa_sgpio_conf_t *in, vtss_sgpio_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_sgpio_conf_t *tmp = (const vtss_sgpio_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO)
#if defined(VTSS_FEATURE_SERIAL_GPIO)
inline static mesa_rc mesa_conv_vtss_sgpio_port_data_t_to_mesa_sgpio_port_data_t(const vtss_sgpio_port_data_t *in, mesa_sgpio_port_data_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_sgpio_port_data_t *tmp = (const mesa_sgpio_port_data_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO)
#if defined(VTSS_FEATURE_SERIAL_GPIO)
inline static mesa_rc mesa_conv_mesa_sgpio_port_data_t_to_vtss_sgpio_port_data_t(const mesa_sgpio_port_data_t *in, vtss_sgpio_port_data_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_sgpio_port_data_t *tmp = (const vtss_sgpio_port_data_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO)
#if defined(VTSS_FEATURE_INTERRUPTS)
mesa_rc mesa_conv_vtss_intr_t_to_mesa_intr_t(const vtss_intr_t *in, mesa_intr_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_INTERRUPTS)
#if defined(VTSS_FEATURE_INTERRUPTS)
mesa_rc mesa_conv_mesa_intr_t_to_vtss_intr_t(const mesa_intr_t *in, vtss_intr_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_INTERRUPTS)
#ifdef VTSS_FEATURE_IRQ_CONTROL
mesa_rc mesa_conv_vtss_irq_t_to_mesa_irq_t(const vtss_irq_t *in, mesa_irq_t *out); /* ag.rb:1595 */
#endif  // VTSS_FEATURE_IRQ_CONTROL
#ifdef VTSS_FEATURE_IRQ_CONTROL
mesa_rc mesa_conv_mesa_irq_t_to_vtss_irq_t(const mesa_irq_t *in, vtss_irq_t *out); /* ag.rb:1595 */
#endif  // VTSS_FEATURE_IRQ_CONTROL
#ifdef VTSS_FEATURE_IRQ_CONTROL
inline static mesa_rc mesa_conv_vtss_irq_conf_t_to_mesa_irq_conf_t(const vtss_irq_conf_t *in, mesa_irq_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_irq_conf_t *tmp = (const mesa_irq_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // VTSS_FEATURE_IRQ_CONTROL
#ifdef VTSS_FEATURE_IRQ_CONTROL
inline static mesa_rc mesa_conv_mesa_irq_conf_t_to_vtss_irq_conf_t(const mesa_irq_conf_t *in, vtss_irq_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_irq_conf_t *tmp = (const vtss_irq_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // VTSS_FEATURE_IRQ_CONTROL
#ifdef VTSS_FEATURE_IRQ_CONTROL
inline static mesa_rc mesa_conv_vtss_irq_status_t_to_mesa_irq_status_t(const vtss_irq_status_t *in, mesa_irq_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_irq_status_t *tmp = (const mesa_irq_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // VTSS_FEATURE_IRQ_CONTROL
#ifdef VTSS_FEATURE_IRQ_CONTROL
inline static mesa_rc mesa_conv_mesa_irq_status_t_to_vtss_irq_status_t(const mesa_irq_status_t *in, vtss_irq_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_irq_status_t *tmp = (const vtss_irq_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // VTSS_FEATURE_IRQ_CONTROL
inline static mesa_rc mesa_conv_vtss_os_timestamp_t_to_mesa_os_timestamp_t(const vtss_os_timestamp_t *in, mesa_os_timestamp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_os_timestamp_t *tmp = (const mesa_os_timestamp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_os_timestamp_t_to_vtss_os_timestamp_t(const mesa_os_timestamp_t *in, vtss_os_timestamp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_os_timestamp_t *tmp = (const vtss_os_timestamp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#if defined(VTSS_FEATURE_FAN)
inline static mesa_rc mesa_conv_vtss_fan_pwd_freq_t_to_mesa_fan_pwd_freq_t(const vtss_fan_pwd_freq_t *in, mesa_fan_pwd_freq_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_fan_pwd_freq_t *tmp = (const mesa_fan_pwd_freq_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FAN)
#if defined(VTSS_FEATURE_FAN)
inline static mesa_rc mesa_conv_mesa_fan_pwd_freq_t_to_vtss_fan_pwd_freq_t(const mesa_fan_pwd_freq_t *in, vtss_fan_pwd_freq_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_fan_pwd_freq_t *tmp = (const vtss_fan_pwd_freq_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FAN)
#if defined(VTSS_FEATURE_FAN)
inline static mesa_rc mesa_conv_vtss_fan_type_t_to_mesa_fan_type_t(const vtss_fan_type_t *in, mesa_fan_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_fan_type_t *tmp = (const mesa_fan_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FAN)
#if defined(VTSS_FEATURE_FAN)
inline static mesa_rc mesa_conv_mesa_fan_type_t_to_vtss_fan_type_t(const mesa_fan_type_t *in, vtss_fan_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_fan_type_t *tmp = (const vtss_fan_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FAN)
#if defined(VTSS_FEATURE_FAN)
inline static mesa_rc mesa_conv_vtss_fan_conf_t_to_mesa_fan_conf_t(const vtss_fan_conf_t *in, mesa_fan_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_fan_conf_t *tmp = (const mesa_fan_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FAN)
#if defined(VTSS_FEATURE_FAN)
inline static mesa_rc mesa_conv_mesa_fan_conf_t_to_vtss_fan_conf_t(const mesa_fan_conf_t *in, vtss_fan_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_fan_conf_t *tmp = (const vtss_fan_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FAN)
#ifdef VTSS_FEATURE_EEE
inline static mesa_rc mesa_conv_vtss_eee_port_conf_t_to_mesa_eee_port_conf_t(const vtss_eee_port_conf_t *in, mesa_eee_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_eee_port_conf_t *tmp = (const mesa_eee_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // VTSS_FEATURE_EEE
#ifdef VTSS_FEATURE_EEE
inline static mesa_rc mesa_conv_mesa_eee_port_conf_t_to_vtss_eee_port_conf_t(const mesa_eee_port_conf_t *in, vtss_eee_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_eee_port_conf_t *tmp = (const vtss_eee_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // VTSS_FEATURE_EEE
mesa_rc mesa_conv_vtss_symreg_reg_t_to_mesa_symreg_reg_t(const vtss_symreg_reg_t *in, mesa_symreg_reg_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_symreg_reg_t_to_vtss_symreg_reg_t(const mesa_symreg_reg_t *in, vtss_symreg_reg_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_vtss_symreg_reggrp_t_to_mesa_symreg_reggrp_t(const vtss_symreg_reggrp_t *in, mesa_symreg_reggrp_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_symreg_reggrp_t_to_vtss_symreg_reggrp_t(const mesa_symreg_reggrp_t *in, vtss_symreg_reggrp_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_vtss_symreg_target_t_to_mesa_symreg_target_t(const vtss_symreg_target_t *in, mesa_symreg_target_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_symreg_target_t_to_vtss_symreg_target_t(const mesa_symreg_target_t *in, vtss_symreg_target_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_vtss_symreg_data_t_to_mesa_symreg_data_t(const vtss_symreg_data_t *in, mesa_symreg_data_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_symreg_data_t_to_vtss_symreg_data_t(const mesa_symreg_data_t *in, vtss_symreg_data_t *out); /* ag.rb:1595 */
inline static mesa_rc mesa_conv_vtss_vscope_scan_t_to_mesa_vscope_scan_t(const vtss_vscope_scan_t *in, mesa_vscope_scan_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vscope_scan_t *tmp = (const mesa_vscope_scan_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vscope_scan_t_to_vtss_vscope_scan_t(const mesa_vscope_scan_t *in, vtss_vscope_scan_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vscope_scan_t *tmp = (const vtss_vscope_scan_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vscope_conf_t_to_mesa_vscope_conf_t(const vtss_vscope_conf_t *in, mesa_vscope_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vscope_conf_t *tmp = (const mesa_vscope_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vscope_conf_t_to_vtss_vscope_conf_t(const mesa_vscope_conf_t *in, vtss_vscope_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vscope_conf_t *tmp = (const vtss_vscope_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#if defined(VTSS_FEATURE_VSCOPE)
inline static mesa_rc mesa_conv_vtss_vscope_ib_storage_t_to_mesa_vscope_ib_storage_t(const vtss_vscope_ib_storage_t *in, mesa_vscope_ib_storage_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vscope_ib_storage_t *tmp = (const mesa_vscope_ib_storage_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VSCOPE)
#if defined(VTSS_FEATURE_VSCOPE)
inline static mesa_rc mesa_conv_mesa_vscope_ib_storage_t_to_vtss_vscope_ib_storage_t(const mesa_vscope_ib_storage_t *in, vtss_vscope_ib_storage_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vscope_ib_storage_t *tmp = (const vtss_vscope_ib_storage_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VSCOPE)
#if defined(VTSS_FEATURE_VSCOPE)
inline static mesa_rc mesa_conv_vtss_vscope_scan_conf_t_to_mesa_vscope_scan_conf_t(const vtss_vscope_scan_conf_t *in, mesa_vscope_scan_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vscope_scan_conf_t *tmp = (const mesa_vscope_scan_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VSCOPE)
#if defined(VTSS_FEATURE_VSCOPE)
inline static mesa_rc mesa_conv_mesa_vscope_scan_conf_t_to_vtss_vscope_scan_conf_t(const mesa_vscope_scan_conf_t *in, vtss_vscope_scan_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vscope_scan_conf_t *tmp = (const vtss_vscope_scan_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VSCOPE)
#if defined(VTSS_FEATURE_VSCOPE)
inline static mesa_rc mesa_conv_vtss_vscope_scan_status_t_to_mesa_vscope_scan_status_t(const vtss_vscope_scan_status_t *in, mesa_vscope_scan_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vscope_scan_status_t *tmp = (const mesa_vscope_scan_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VSCOPE)
#if defined(VTSS_FEATURE_VSCOPE)
inline static mesa_rc mesa_conv_mesa_vscope_scan_status_t_to_vtss_vscope_scan_status_t(const mesa_vscope_scan_status_t *in, vtss_vscope_scan_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vscope_scan_status_t *tmp = (const vtss_vscope_scan_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VSCOPE)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_vtss_mrp_ring_role_t_to_mesa_mrp_ring_role_t(const vtss_mrp_ring_role_t *in, mesa_mrp_ring_role_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mrp_ring_role_t *tmp = (const mesa_mrp_ring_role_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_mesa_mrp_ring_role_t_to_vtss_mrp_ring_role_t(const mesa_mrp_ring_role_t *in, vtss_mrp_ring_role_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mrp_ring_role_t *tmp = (const vtss_mrp_ring_role_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_vtss_mrp_conf_t_to_mesa_mrp_conf_t(const vtss_mrp_conf_t *in, mesa_mrp_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mrp_conf_t *tmp = (const mesa_mrp_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_mesa_mrp_conf_t_to_vtss_mrp_conf_t(const mesa_mrp_conf_t *in, vtss_mrp_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mrp_conf_t *tmp = (const vtss_mrp_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_vtss_mrp_voe_idx_t_to_mesa_mrp_voe_idx_t(const vtss_mrp_voe_idx_t *in, mesa_mrp_voe_idx_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mrp_voe_idx_t *tmp = (const mesa_mrp_voe_idx_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_mesa_mrp_voe_idx_t_to_vtss_mrp_voe_idx_t(const mesa_mrp_voe_idx_t *in, vtss_mrp_voe_idx_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mrp_voe_idx_t *tmp = (const vtss_mrp_voe_idx_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_vtss_mrp_ring_state_t_to_mesa_mrp_ring_state_t(const vtss_mrp_ring_state_t *in, mesa_mrp_ring_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mrp_ring_state_t *tmp = (const mesa_mrp_ring_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_mesa_mrp_ring_state_t_to_vtss_mrp_ring_state_t(const mesa_mrp_ring_state_t *in, vtss_mrp_ring_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mrp_ring_state_t *tmp = (const vtss_mrp_ring_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_vtss_mrp_best_mrm_t_to_mesa_mrp_best_mrm_t(const vtss_mrp_best_mrm_t *in, mesa_mrp_best_mrm_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mrp_best_mrm_t *tmp = (const mesa_mrp_best_mrm_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_mesa_mrp_best_mrm_t_to_vtss_mrp_best_mrm_t(const mesa_mrp_best_mrm_t *in, vtss_mrp_best_mrm_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mrp_best_mrm_t *tmp = (const vtss_mrp_best_mrm_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_vtss_mrp_port_state_t_to_mesa_mrp_port_state_t(const vtss_mrp_port_state_t *in, mesa_mrp_port_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mrp_port_state_t *tmp = (const mesa_mrp_port_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_mesa_mrp_port_state_t_to_vtss_mrp_port_state_t(const mesa_mrp_port_state_t *in, vtss_mrp_port_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mrp_port_state_t *tmp = (const vtss_mrp_port_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_vtss_mrp_tst_loc_conf_t_to_mesa_mrp_tst_loc_conf_t(const vtss_mrp_tst_loc_conf_t *in, mesa_mrp_tst_loc_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mrp_tst_loc_conf_t *tmp = (const mesa_mrp_tst_loc_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_mesa_mrp_tst_loc_conf_t_to_vtss_mrp_tst_loc_conf_t(const mesa_mrp_tst_loc_conf_t *in, vtss_mrp_tst_loc_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mrp_tst_loc_conf_t *tmp = (const vtss_mrp_tst_loc_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_vtss_mrp_tst_copy_conf_t_to_mesa_mrp_tst_copy_conf_t(const vtss_mrp_tst_copy_conf_t *in, mesa_mrp_tst_copy_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mrp_tst_copy_conf_t *tmp = (const mesa_mrp_tst_copy_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_mesa_mrp_tst_copy_conf_t_to_vtss_mrp_tst_copy_conf_t(const mesa_mrp_tst_copy_conf_t *in, vtss_mrp_tst_copy_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mrp_tst_copy_conf_t *tmp = (const vtss_mrp_tst_copy_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_vtss_mrp_port_status_t_to_mesa_mrp_port_status_t(const vtss_mrp_port_status_t *in, mesa_mrp_port_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mrp_port_status_t *tmp = (const mesa_mrp_port_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_mesa_mrp_port_status_t_to_vtss_mrp_port_status_t(const mesa_mrp_port_status_t *in, vtss_mrp_port_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mrp_port_status_t *tmp = (const vtss_mrp_port_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_vtss_mrp_status_t_to_mesa_mrp_status_t(const vtss_mrp_status_t *in, mesa_mrp_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mrp_status_t *tmp = (const mesa_mrp_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_mesa_mrp_status_t_to_vtss_mrp_status_t(const mesa_mrp_status_t *in, vtss_mrp_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mrp_status_t *tmp = (const vtss_mrp_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_vtss_mrp_port_counters_t_to_mesa_mrp_port_counters_t(const vtss_mrp_port_counters_t *in, mesa_mrp_port_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mrp_port_counters_t *tmp = (const mesa_mrp_port_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_mesa_mrp_port_counters_t_to_vtss_mrp_port_counters_t(const mesa_mrp_port_counters_t *in, vtss_mrp_port_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mrp_port_counters_t *tmp = (const vtss_mrp_port_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_vtss_mrp_counters_t_to_mesa_mrp_counters_t(const vtss_mrp_counters_t *in, mesa_mrp_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mrp_counters_t *tmp = (const mesa_mrp_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_mesa_mrp_counters_t_to_vtss_mrp_counters_t(const mesa_mrp_counters_t *in, vtss_mrp_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mrp_counters_t *tmp = (const vtss_mrp_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_vtss_mrp_event_t_to_mesa_mrp_event_t(const vtss_mrp_event_t *in, mesa_mrp_event_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mrp_event_t *tmp = (const mesa_mrp_event_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_MRP)
inline static mesa_rc mesa_conv_mesa_mrp_event_t_to_vtss_mrp_event_t(const mesa_mrp_event_t *in, vtss_mrp_event_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mrp_event_t *tmp = (const vtss_mrp_event_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_MRP)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_npi_conf_t_to_mesa_npi_conf_t(const vtss_npi_conf_t *in, mesa_npi_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_npi_conf_t *tmp = (const mesa_npi_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_npi_conf_t_to_vtss_npi_conf_t(const mesa_npi_conf_t *in, vtss_npi_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_npi_conf_t *tmp = (const vtss_npi_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_rx_queue_npi_conf_t_to_mesa_packet_rx_queue_npi_conf_t(const vtss_packet_rx_queue_npi_conf_t *in, mesa_packet_rx_queue_npi_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_rx_queue_npi_conf_t *tmp = (const mesa_packet_rx_queue_npi_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_rx_queue_npi_conf_t_to_vtss_packet_rx_queue_npi_conf_t(const mesa_packet_rx_queue_npi_conf_t *in, vtss_packet_rx_queue_npi_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_rx_queue_npi_conf_t *tmp = (const vtss_packet_rx_queue_npi_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_conv_vtss_packet_rx_queue_conf_t_to_mesa_packet_rx_queue_conf_t(const vtss_packet_rx_queue_conf_t *in, mesa_packet_rx_queue_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_conv_mesa_packet_rx_queue_conf_t_to_vtss_packet_rx_queue_conf_t(const mesa_packet_rx_queue_conf_t *in, vtss_packet_rx_queue_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_rx_reg_t_to_mesa_packet_rx_reg_t(const vtss_packet_rx_reg_t *in, mesa_packet_rx_reg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_rx_reg_t *tmp = (const mesa_packet_rx_reg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_rx_reg_t_to_vtss_packet_rx_reg_t(const mesa_packet_rx_reg_t *in, vtss_packet_rx_reg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_rx_reg_t *tmp = (const vtss_packet_rx_reg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_conv_vtss_packet_rx_queue_map_t_to_mesa_packet_rx_queue_map_t(const vtss_packet_rx_queue_map_t *in, mesa_packet_rx_queue_map_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_conv_mesa_packet_rx_queue_map_t_to_vtss_packet_rx_queue_map_t(const mesa_packet_rx_queue_map_t *in, vtss_packet_rx_queue_map_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_conv_vtss_packet_rx_conf_t_to_mesa_packet_rx_conf_t(const vtss_packet_rx_conf_t *in, mesa_packet_rx_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_conv_mesa_packet_rx_conf_t_to_vtss_packet_rx_conf_t(const mesa_packet_rx_conf_t *in, vtss_packet_rx_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_frame_info_t_to_mesa_packet_frame_info_t(const vtss_packet_frame_info_t *in, mesa_packet_frame_info_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_frame_info_t *tmp = (const mesa_packet_frame_info_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_frame_info_t_to_vtss_packet_frame_info_t(const mesa_packet_frame_info_t *in, vtss_packet_frame_info_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_frame_info_t *tmp = (const vtss_packet_frame_info_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_filter_t_to_mesa_packet_filter_t(const vtss_packet_filter_t *in, mesa_packet_filter_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_filter_t *tmp = (const mesa_packet_filter_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_filter_t_to_vtss_packet_filter_t(const mesa_packet_filter_t *in, vtss_packet_filter_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_filter_t *tmp = (const vtss_packet_filter_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_port_info_t_to_mesa_packet_port_info_t(const vtss_packet_port_info_t *in, mesa_packet_port_info_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_port_info_t *tmp = (const mesa_packet_port_info_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_port_info_t_to_vtss_packet_port_info_t(const mesa_packet_port_info_t *in, vtss_packet_port_info_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_port_info_t *tmp = (const vtss_packet_port_info_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_port_filter_t_to_mesa_packet_port_filter_t(const vtss_packet_port_filter_t *in, mesa_packet_port_filter_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_port_filter_t *tmp = (const mesa_packet_port_filter_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_port_filter_t_to_vtss_packet_port_filter_t(const mesa_packet_port_filter_t *in, vtss_packet_port_filter_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_port_filter_t *tmp = (const vtss_packet_port_filter_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_vlan_filter_t_to_mesa_packet_vlan_filter_t(const vtss_packet_vlan_filter_t *in, mesa_packet_vlan_filter_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_vlan_filter_t *tmp = (const mesa_packet_vlan_filter_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_vlan_filter_t_to_vtss_packet_vlan_filter_t(const mesa_packet_vlan_filter_t *in, vtss_packet_vlan_filter_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_vlan_filter_t *tmp = (const vtss_packet_vlan_filter_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_vlan_status_t_to_mesa_packet_vlan_status_t(const vtss_packet_vlan_status_t *in, mesa_packet_vlan_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_vlan_status_t *tmp = (const mesa_packet_vlan_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_vlan_status_t_to_vtss_packet_vlan_status_t(const mesa_packet_vlan_status_t *in, vtss_packet_vlan_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_vlan_status_t *tmp = (const vtss_packet_vlan_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_oam_type_t_to_mesa_packet_oam_type_t(const vtss_packet_oam_type_t *in, mesa_packet_oam_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_oam_type_t *tmp = (const mesa_packet_oam_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_oam_type_t_to_vtss_packet_oam_type_t(const mesa_packet_oam_type_t *in, vtss_packet_oam_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_oam_type_t *tmp = (const vtss_packet_oam_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_ptp_action_t_to_mesa_packet_ptp_action_t(const vtss_packet_ptp_action_t *in, mesa_packet_ptp_action_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_ptp_action_t *tmp = (const mesa_packet_ptp_action_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_ptp_action_t_to_vtss_packet_ptp_action_t(const mesa_packet_ptp_action_t *in, vtss_packet_ptp_action_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_ptp_action_t *tmp = (const vtss_packet_ptp_action_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_encap_type_t_to_mesa_packet_encap_type_t(const vtss_packet_encap_type_t *in, mesa_packet_encap_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_encap_type_t *tmp = (const mesa_packet_encap_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_encap_type_t_to_vtss_packet_encap_type_t(const mesa_packet_encap_type_t *in, vtss_packet_encap_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_encap_type_t *tmp = (const vtss_packet_encap_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_inj_encap_t_to_mesa_packet_inj_encap_t(const vtss_packet_inj_encap_t *in, mesa_packet_inj_encap_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_inj_encap_t *tmp = (const mesa_packet_inj_encap_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_inj_encap_t_to_vtss_packet_inj_encap_t(const mesa_packet_inj_encap_t *in, vtss_packet_inj_encap_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_inj_encap_t *tmp = (const vtss_packet_inj_encap_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_tag_type_t_to_mesa_tag_type_t(const vtss_tag_type_t *in, mesa_tag_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_tag_type_t *tmp = (const mesa_tag_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_tag_type_t_to_vtss_tag_type_t(const mesa_tag_type_t *in, vtss_tag_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_tag_type_t *tmp = (const vtss_tag_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_rx_meta_t_to_mesa_packet_rx_meta_t(const vtss_packet_rx_meta_t *in, mesa_packet_rx_meta_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_rx_meta_t *tmp = (const mesa_packet_rx_meta_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_rx_meta_t_to_vtss_packet_rx_meta_t(const mesa_packet_rx_meta_t *in, vtss_packet_rx_meta_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_rx_meta_t *tmp = (const vtss_packet_rx_meta_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_rx_hints_t_to_mesa_packet_rx_hints_t(const vtss_packet_rx_hints_t *in, mesa_packet_rx_hints_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_rx_hints_t *tmp = (const mesa_packet_rx_hints_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_rx_hints_t_to_vtss_packet_rx_hints_t(const mesa_packet_rx_hints_t *in, vtss_packet_rx_hints_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_rx_hints_t *tmp = (const vtss_packet_rx_hints_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_rx_info_t_to_mesa_packet_rx_info_t(const vtss_packet_rx_info_t *in, mesa_packet_rx_info_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_rx_info_t *tmp = (const mesa_packet_rx_info_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_rx_info_t_to_vtss_packet_rx_info_t(const mesa_packet_rx_info_t *in, vtss_packet_rx_info_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_rx_info_t *tmp = (const vtss_packet_rx_info_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET_PIPELINE_PT)
inline static mesa_rc mesa_conv_vtss_packet_pipeline_pt_t_to_mesa_packet_pipeline_pt_t(const vtss_packet_pipeline_pt_t *in, mesa_packet_pipeline_pt_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_pipeline_pt_t *tmp = (const mesa_packet_pipeline_pt_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET_PIPELINE_PT)
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET_PIPELINE_PT)
inline static mesa_rc mesa_conv_mesa_packet_pipeline_pt_t_to_vtss_packet_pipeline_pt_t(const mesa_packet_pipeline_pt_t *in, vtss_packet_pipeline_pt_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_pipeline_pt_t *tmp = (const vtss_packet_pipeline_pt_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET_PIPELINE_PT)
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_rb_fwd_t_to_mesa_packet_rb_fwd_t(const vtss_packet_rb_fwd_t *in, mesa_packet_rb_fwd_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_rb_fwd_t *tmp = (const mesa_packet_rb_fwd_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_rb_fwd_t_to_vtss_packet_rb_fwd_t(const mesa_packet_rb_fwd_t *in, vtss_packet_rb_fwd_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_rb_fwd_t *tmp = (const vtss_packet_rb_fwd_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_conv_vtss_packet_tx_info_t_to_mesa_packet_tx_info_t(const vtss_packet_tx_info_t *in, mesa_packet_tx_info_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_conv_mesa_packet_tx_info_t_to_vtss_packet_tx_info_t(const mesa_packet_tx_info_t *in, vtss_packet_tx_info_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_ptp_message_type_t_to_mesa_packet_ptp_message_type_t(const vtss_packet_ptp_message_type_t *in, mesa_packet_ptp_message_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_ptp_message_type_t *tmp = (const mesa_packet_ptp_message_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_ptp_message_type_t_to_vtss_packet_ptp_message_type_t(const mesa_packet_ptp_message_type_t *in, vtss_packet_ptp_message_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_ptp_message_type_t *tmp = (const vtss_packet_ptp_message_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_internal_tc_mode_t_to_mesa_packet_internal_tc_mode_t(const vtss_packet_internal_tc_mode_t *in, mesa_packet_internal_tc_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_internal_tc_mode_t *tmp = (const mesa_packet_internal_tc_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_internal_tc_mode_t_to_vtss_packet_internal_tc_mode_t(const mesa_packet_internal_tc_mode_t *in, vtss_packet_internal_tc_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_internal_tc_mode_t *tmp = (const vtss_packet_internal_tc_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_ptp_delay_comp_t_to_mesa_packet_ptp_delay_comp_t(const vtss_packet_ptp_delay_comp_t *in, mesa_packet_ptp_delay_comp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_ptp_delay_comp_t *tmp = (const mesa_packet_ptp_delay_comp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_ptp_delay_comp_t_to_vtss_packet_ptp_delay_comp_t(const mesa_packet_ptp_delay_comp_t *in, vtss_packet_ptp_delay_comp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_ptp_delay_comp_t *tmp = (const vtss_packet_ptp_delay_comp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_vtss_packet_timestamp_props_t_to_mesa_packet_timestamp_props_t(const vtss_packet_timestamp_props_t *in, mesa_packet_timestamp_props_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_timestamp_props_t *tmp = (const mesa_packet_timestamp_props_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
inline static mesa_rc mesa_conv_mesa_packet_timestamp_props_t_to_vtss_packet_timestamp_props_t(const mesa_packet_timestamp_props_t *in, vtss_packet_timestamp_props_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_timestamp_props_t *tmp = (const vtss_packet_timestamp_props_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_vtss_internal_bw_t_to_mesa_internal_bw_t(const vtss_internal_bw_t *in, mesa_internal_bw_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_internal_bw_t *tmp = (const mesa_internal_bw_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_mesa_internal_bw_t_to_vtss_internal_bw_t(const mesa_internal_bw_t *in, vtss_internal_bw_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_internal_bw_t *tmp = (const vtss_internal_bw_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_vtss_sd_sgpio_action_t_to_mesa_sd_sgpio_action_t(const vtss_sd_sgpio_action_t *in, mesa_sd_sgpio_action_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_sd_sgpio_action_t *tmp = (const mesa_sd_sgpio_action_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_mesa_sd_sgpio_action_t_to_vtss_sd_sgpio_action_t(const mesa_sd_sgpio_action_t *in, vtss_sd_sgpio_action_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_sd_sgpio_action_t *tmp = (const vtss_sd_sgpio_action_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_vtss_port_sgpio_map_t_to_mesa_port_sgpio_map_t(const vtss_port_sgpio_map_t *in, mesa_port_sgpio_map_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_mesa_port_sgpio_map_t_to_vtss_port_sgpio_map_t(const mesa_port_sgpio_map_t *in, vtss_port_sgpio_map_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_vtss_port_map_t_to_mesa_port_map_t(const vtss_port_map_t *in, mesa_port_map_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_mesa_port_map_t_to_vtss_port_map_t(const mesa_port_map_t *in, vtss_port_map_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_vtss_port_clause_37_remote_fault_t_to_mesa_port_clause_37_remote_fault_t(const vtss_port_clause_37_remote_fault_t *in, mesa_port_clause_37_remote_fault_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_mesa_port_clause_37_remote_fault_t_to_vtss_port_clause_37_remote_fault_t(const mesa_port_clause_37_remote_fault_t *in, vtss_port_clause_37_remote_fault_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_vtss_port_clause_37_adv_t_to_mesa_port_clause_37_adv_t(const vtss_port_clause_37_adv_t *in, mesa_port_clause_37_adv_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_mesa_port_clause_37_adv_t_to_vtss_port_clause_37_adv_t(const mesa_port_clause_37_adv_t *in, vtss_port_clause_37_adv_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_vtss_port_sgmii_aneg_t_to_mesa_port_sgmii_aneg_t(const vtss_port_sgmii_aneg_t *in, mesa_port_sgmii_aneg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_sgmii_aneg_t *tmp = (const mesa_port_sgmii_aneg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_mesa_port_sgmii_aneg_t_to_vtss_port_sgmii_aneg_t(const mesa_port_sgmii_aneg_t *in, vtss_port_sgmii_aneg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_sgmii_aneg_t *tmp = (const vtss_port_sgmii_aneg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_vtss_port_clause_37_control_t_to_mesa_port_clause_37_control_t(const vtss_port_clause_37_control_t *in, mesa_port_clause_37_control_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_mesa_port_clause_37_control_t_to_vtss_port_clause_37_control_t(const mesa_port_clause_37_control_t *in, vtss_port_clause_37_control_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_vtss_port_flow_control_conf_t_to_mesa_port_flow_control_conf_t(const vtss_port_flow_control_conf_t *in, mesa_port_flow_control_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_mesa_port_flow_control_conf_t_to_vtss_port_flow_control_conf_t(const mesa_port_flow_control_conf_t *in, vtss_port_flow_control_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_vtss_port_frame_gaps_t_to_mesa_port_frame_gaps_t(const vtss_port_frame_gaps_t *in, mesa_port_frame_gaps_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_frame_gaps_t *tmp = (const mesa_port_frame_gaps_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_mesa_port_frame_gaps_t_to_vtss_port_frame_gaps_t(const mesa_port_frame_gaps_t *in, vtss_port_frame_gaps_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_frame_gaps_t *tmp = (const vtss_port_frame_gaps_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_vtss_port_max_tags_t_to_mesa_port_max_tags_t(const vtss_port_max_tags_t *in, mesa_port_max_tags_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_max_tags_t *tmp = (const mesa_port_max_tags_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_mesa_port_max_tags_t_to_vtss_port_max_tags_t(const mesa_port_max_tags_t *in, vtss_port_max_tags_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_max_tags_t *tmp = (const vtss_port_max_tags_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_vtss_port_loop_t_to_mesa_port_loop_t(const vtss_port_loop_t *in, mesa_port_loop_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_loop_t *tmp = (const mesa_port_loop_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_mesa_port_loop_t_to_vtss_port_loop_t(const mesa_port_loop_t *in, vtss_port_loop_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_loop_t *tmp = (const vtss_port_loop_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_vtss_port_serdes_conf_t_to_mesa_port_serdes_conf_t(const vtss_port_serdes_conf_t *in, mesa_port_serdes_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_serdes_conf_t *tmp = (const mesa_port_serdes_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_mesa_port_serdes_conf_t_to_vtss_port_serdes_conf_t(const mesa_port_serdes_conf_t *in, vtss_port_serdes_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_serdes_conf_t *tmp = (const vtss_port_serdes_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_PCS_CONF)
inline static mesa_rc mesa_conv_vtss_port_pcs_conf_t_to_mesa_port_pcs_conf_t(const vtss_port_pcs_conf_t *in, mesa_port_pcs_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_pcs_conf_t *tmp = (const mesa_port_pcs_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_PCS_CONF)
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_PCS_CONF)
inline static mesa_rc mesa_conv_mesa_port_pcs_conf_t_to_vtss_port_pcs_conf_t(const mesa_port_pcs_conf_t *in, vtss_port_pcs_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_pcs_conf_t *tmp = (const vtss_port_pcs_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_PCS_CONF)
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_vtss_port_conf_t_to_mesa_port_conf_t(const vtss_port_conf_t *in, mesa_port_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_mesa_port_conf_t_to_vtss_port_conf_t(const mesa_port_conf_t *in, vtss_port_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_vtss_port_bulk_t_to_mesa_port_bulk_t(const vtss_port_bulk_t *in, mesa_port_bulk_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_bulk_t *tmp = (const mesa_port_bulk_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_mesa_port_bulk_t_to_vtss_port_bulk_t(const mesa_port_bulk_t *in, vtss_port_bulk_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_bulk_t *tmp = (const vtss_port_bulk_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_vtss_port_rmon_counters_t_to_mesa_port_rmon_counters_t(const vtss_port_rmon_counters_t *in, mesa_port_rmon_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_rmon_counters_t *tmp = (const mesa_port_rmon_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_port_rmon_counters_t_to_vtss_port_rmon_counters_t(const mesa_port_rmon_counters_t *in, vtss_port_rmon_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_rmon_counters_t *tmp = (const vtss_port_rmon_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_port_if_group_counters_t_to_mesa_port_if_group_counters_t(const vtss_port_if_group_counters_t *in, mesa_port_if_group_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_if_group_counters_t *tmp = (const mesa_port_if_group_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_port_if_group_counters_t_to_vtss_port_if_group_counters_t(const mesa_port_if_group_counters_t *in, vtss_port_if_group_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_if_group_counters_t *tmp = (const vtss_port_if_group_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
mesa_rc mesa_conv_vtss_port_ethernet_like_counters_t_to_mesa_port_ethernet_like_counters_t(const vtss_port_ethernet_like_counters_t *in, mesa_port_ethernet_like_counters_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_port_ethernet_like_counters_t_to_vtss_port_ethernet_like_counters_t(const mesa_port_ethernet_like_counters_t *in, vtss_port_ethernet_like_counters_t *out); /* ag.rb:1595 */
inline static mesa_rc mesa_conv_vtss_port_bridge_counters_t_to_mesa_port_bridge_counters_t(const vtss_port_bridge_counters_t *in, mesa_port_bridge_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_bridge_counters_t *tmp = (const mesa_port_bridge_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_port_bridge_counters_t_to_vtss_port_bridge_counters_t(const mesa_port_bridge_counters_t *in, vtss_port_bridge_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_bridge_counters_t *tmp = (const vtss_port_bridge_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
mesa_rc mesa_conv_vtss_port_dot3br_counters_t_to_mesa_port_dot3br_counters_t(const vtss_port_dot3br_counters_t *in, mesa_port_dot3br_counters_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_port_dot3br_counters_t_to_vtss_port_dot3br_counters_t(const mesa_port_dot3br_counters_t *in, vtss_port_dot3br_counters_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_vtss_port_counters_t_to_mesa_port_counters_t(const vtss_port_counters_t *in, mesa_port_counters_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_port_counters_t_to_vtss_port_counters_t(const mesa_port_counters_t *in, vtss_port_counters_t *out); /* ag.rb:1595 */
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_vtss_basic_counters_t_to_mesa_basic_counters_t(const vtss_basic_counters_t *in, mesa_basic_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_basic_counters_t *tmp = (const mesa_basic_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_mesa_basic_counters_t_to_vtss_basic_counters_t(const mesa_basic_counters_t *in, vtss_basic_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_basic_counters_t *tmp = (const vtss_basic_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_vtss_port_forward_t_to_mesa_port_forward_t(const vtss_port_forward_t *in, mesa_port_forward_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_forward_t *tmp = (const mesa_port_forward_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
inline static mesa_rc mesa_conv_mesa_port_forward_t_to_vtss_port_forward_t(const mesa_port_forward_t *in, vtss_port_forward_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_forward_t *tmp = (const vtss_port_forward_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_IFH)
mesa_rc mesa_conv_vtss_port_ifh_t_to_mesa_port_ifh_t(const vtss_port_ifh_t *in, mesa_port_ifh_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_IFH)
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_IFH)
mesa_rc mesa_conv_mesa_port_ifh_t_to_vtss_port_ifh_t(const mesa_port_ifh_t *in, vtss_port_ifh_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PORT_IFH)
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_port_kr_status_aneg_t_to_mesa_port_kr_status_aneg_t(const vtss_port_kr_status_aneg_t *in, mesa_port_kr_status_aneg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_kr_status_aneg_t *tmp = (const mesa_port_kr_status_aneg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_mesa_port_kr_status_aneg_t_to_vtss_port_kr_status_aneg_t(const mesa_port_kr_status_aneg_t *in, vtss_port_kr_status_aneg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_kr_status_aneg_t *tmp = (const vtss_port_kr_status_aneg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_port_kr_status_train_t_to_mesa_port_kr_status_train_t(const vtss_port_kr_status_train_t *in, mesa_port_kr_status_train_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_kr_status_train_t *tmp = (const mesa_port_kr_status_train_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_mesa_port_kr_status_train_t_to_vtss_port_kr_status_train_t(const mesa_port_kr_status_train_t *in, vtss_port_kr_status_train_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_kr_status_train_t *tmp = (const vtss_port_kr_status_train_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_port_kr_status_fec_t_to_mesa_port_kr_status_fec_t(const vtss_port_kr_status_fec_t *in, mesa_port_kr_status_fec_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_kr_status_fec_t *tmp = (const mesa_port_kr_status_fec_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_mesa_port_kr_status_fec_t_to_vtss_port_kr_status_fec_t(const mesa_port_kr_status_fec_t *in, vtss_port_kr_status_fec_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_kr_status_fec_t *tmp = (const vtss_port_kr_status_fec_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_port_kr_status_t_to_mesa_port_kr_status_t(const vtss_port_kr_status_t *in, mesa_port_kr_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_kr_status_t *tmp = (const mesa_port_kr_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_mesa_port_kr_status_t_to_vtss_port_kr_status_t(const mesa_port_kr_status_t *in, vtss_port_kr_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_kr_status_t *tmp = (const vtss_port_kr_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_port_kr_aneg_t_to_mesa_port_kr_aneg_t(const vtss_port_kr_aneg_t *in, mesa_port_kr_aneg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_kr_aneg_t *tmp = (const mesa_port_kr_aneg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_mesa_port_kr_aneg_t_to_vtss_port_kr_aneg_t(const mesa_port_kr_aneg_t *in, vtss_port_kr_aneg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_kr_aneg_t *tmp = (const vtss_port_kr_aneg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_port_kr_train_t_to_mesa_port_kr_train_t(const vtss_port_kr_train_t *in, mesa_port_kr_train_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_kr_train_t *tmp = (const mesa_port_kr_train_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_mesa_port_kr_train_t_to_vtss_port_kr_train_t(const mesa_port_kr_train_t *in, vtss_port_kr_train_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_kr_train_t *tmp = (const vtss_port_kr_train_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_port_kr_conf_t_to_mesa_port_kr_conf_t(const vtss_port_kr_conf_t *in, mesa_port_kr_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_kr_conf_t *tmp = (const mesa_port_kr_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_mesa_port_kr_conf_t_to_vtss_port_kr_conf_t(const mesa_port_kr_conf_t *in, vtss_port_kr_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_kr_conf_t *tmp = (const vtss_port_kr_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_train_state_t_to_mesa_train_state_t(const vtss_train_state_t *in, mesa_train_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_train_state_t *tmp = (const mesa_train_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_mesa_train_state_t_to_vtss_train_state_t(const mesa_train_state_t *in, vtss_train_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_train_state_t *tmp = (const vtss_train_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_ber_stage_t_to_mesa_ber_stage_t(const vtss_ber_stage_t *in, mesa_ber_stage_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ber_stage_t *tmp = (const mesa_ber_stage_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_mesa_ber_stage_t_to_vtss_ber_stage_t(const mesa_ber_stage_t *in, vtss_ber_stage_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ber_stage_t *tmp = (const vtss_ber_stage_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_kr_tap_t_to_mesa_kr_tap_t(const vtss_kr_tap_t *in, mesa_kr_tap_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_kr_tap_t *tmp = (const mesa_kr_tap_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_mesa_kr_tap_t_to_vtss_kr_tap_t(const mesa_kr_tap_t *in, vtss_kr_tap_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_kr_tap_t *tmp = (const vtss_kr_tap_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_kr_status_results_t_to_mesa_kr_status_results_t(const vtss_kr_status_results_t *in, mesa_kr_status_results_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_kr_status_results_t *tmp = (const mesa_kr_status_results_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_mesa_kr_status_results_t_to_vtss_kr_status_results_t(const mesa_kr_status_results_t *in, vtss_kr_status_results_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_kr_status_results_t *tmp = (const vtss_kr_status_results_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_port_ctle_t_to_mesa_port_ctle_t(const vtss_port_ctle_t *in, mesa_port_ctle_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_ctle_t *tmp = (const mesa_port_ctle_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_port_ctle_t_to_vtss_port_ctle_t(const mesa_port_ctle_t *in, vtss_port_ctle_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_ctle_t *tmp = (const vtss_port_ctle_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#if defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_port_kr_state_t_to_mesa_port_kr_state_t(const vtss_port_kr_state_t *in, mesa_port_kr_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_kr_state_t *tmp = (const mesa_port_kr_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_mesa_port_kr_state_t_to_vtss_port_kr_state_t(const mesa_port_kr_state_t *in, vtss_port_kr_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_kr_state_t *tmp = (const vtss_port_kr_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_port_kr_fec_t_to_mesa_port_kr_fec_t(const vtss_port_kr_fec_t *in, mesa_port_kr_fec_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_kr_fec_t *tmp = (const mesa_port_kr_fec_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_mesa_port_kr_fec_t_to_vtss_port_kr_fec_t(const mesa_port_kr_fec_t *in, vtss_port_kr_fec_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_kr_fec_t *tmp = (const vtss_port_kr_fec_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_port_kr_eye_dim_t_to_mesa_port_kr_eye_dim_t(const vtss_port_kr_eye_dim_t *in, mesa_port_kr_eye_dim_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_kr_eye_dim_t *tmp = (const mesa_port_kr_eye_dim_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ)
#if defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_mesa_port_kr_eye_dim_t_to_vtss_port_kr_eye_dim_t(const mesa_port_kr_eye_dim_t *in, vtss_port_kr_eye_dim_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_kr_eye_dim_t *tmp = (const vtss_port_kr_eye_dim_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ)
inline static mesa_rc mesa_conv_vtss_port_lb_t_to_mesa_port_lb_t(const vtss_port_lb_t *in, mesa_port_lb_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_lb_t *tmp = (const mesa_port_lb_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_port_lb_t_to_vtss_port_lb_t(const mesa_port_lb_t *in, vtss_port_lb_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_lb_t *tmp = (const vtss_port_lb_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_port_test_conf_t_to_mesa_port_test_conf_t(const vtss_port_test_conf_t *in, mesa_port_test_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_test_conf_t *tmp = (const mesa_port_test_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_port_test_conf_t_to_vtss_port_test_conf_t(const mesa_port_test_conf_t *in, vtss_port_test_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_test_conf_t *tmp = (const vtss_port_test_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_port_serdes_tap_enum_t_to_mesa_port_serdes_tap_enum_t(const vtss_port_serdes_tap_enum_t *in, mesa_port_serdes_tap_enum_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_serdes_tap_enum_t *tmp = (const mesa_port_serdes_tap_enum_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_port_serdes_tap_enum_t_to_vtss_port_serdes_tap_enum_t(const mesa_port_serdes_tap_enum_t *in, vtss_port_serdes_tap_enum_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_serdes_tap_enum_t *tmp = (const vtss_port_serdes_tap_enum_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_port_serdes_tap_get_t_to_mesa_port_serdes_tap_get_t(const vtss_port_serdes_tap_get_t *in, mesa_port_serdes_tap_get_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_serdes_tap_get_t *tmp = (const mesa_port_serdes_tap_get_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_port_serdes_tap_get_t_to_vtss_port_serdes_tap_get_t(const mesa_port_serdes_tap_get_t *in, vtss_port_serdes_tap_get_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_serdes_tap_get_t *tmp = (const vtss_port_serdes_tap_get_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_serdes_debug_type_t_to_mesa_serdes_debug_type_t(const vtss_serdes_debug_type_t *in, mesa_serdes_debug_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_serdes_debug_type_t *tmp = (const mesa_serdes_debug_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_serdes_debug_type_t_to_vtss_serdes_debug_type_t(const mesa_serdes_debug_type_t *in, vtss_serdes_debug_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_serdes_debug_type_t *tmp = (const vtss_serdes_debug_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_port_serdes_debug_t_to_mesa_port_serdes_debug_t(const vtss_port_serdes_debug_t *in, mesa_port_serdes_debug_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_serdes_debug_t *tmp = (const mesa_port_serdes_debug_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_port_serdes_debug_t_to_vtss_port_serdes_debug_t(const mesa_port_serdes_debug_t *in, vtss_port_serdes_debug_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_serdes_debug_t *tmp = (const vtss_port_serdes_debug_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_WRED_V2) || defined(VTSS_FEATURE_QOS_WRED_V3)
inline static mesa_rc mesa_conv_vtss_wred_max_t_to_mesa_wred_max_t(const vtss_wred_max_t *in, mesa_wred_max_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_wred_max_t *tmp = (const mesa_wred_max_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_WRED_V2) || defined(VTSS_FEATURE_QOS_WRED_V3)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_WRED_V2) || defined(VTSS_FEATURE_QOS_WRED_V3)
inline static mesa_rc mesa_conv_mesa_wred_max_t_to_vtss_wred_max_t(const mesa_wred_max_t *in, vtss_wred_max_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_wred_max_t *tmp = (const vtss_wred_max_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_WRED_V2) || defined(VTSS_FEATURE_QOS_WRED_V3)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_WRED_V2) || defined(VTSS_FEATURE_QOS_WRED_V3)
inline static mesa_rc mesa_conv_vtss_wred_conf_t_to_mesa_wred_conf_t(const vtss_wred_conf_t *in, mesa_wred_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_wred_conf_t *tmp = (const mesa_wred_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_WRED_V2) || defined(VTSS_FEATURE_QOS_WRED_V3)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_WRED_V2) || defined(VTSS_FEATURE_QOS_WRED_V3)
inline static mesa_rc mesa_conv_mesa_wred_conf_t_to_vtss_wred_conf_t(const mesa_wred_conf_t *in, vtss_wred_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_wred_conf_t *tmp = (const vtss_wred_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_WRED_V2) || defined(VTSS_FEATURE_QOS_WRED_V3)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv_vtss_qos_conf_t_to_mesa_qos_conf_t(const vtss_qos_conf_t *in, mesa_qos_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv_mesa_qos_conf_t_to_vtss_qos_conf_t(const mesa_qos_conf_t *in, vtss_qos_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
inline static mesa_rc mesa_conv_vtss_qos_status_t_to_mesa_qos_status_t(const vtss_qos_status_t *in, mesa_qos_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_status_t *tmp = (const mesa_qos_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
inline static mesa_rc mesa_conv_mesa_qos_status_t_to_vtss_qos_status_t(const mesa_qos_status_t *in, vtss_qos_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_status_t *tmp = (const vtss_qos_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
inline static mesa_rc mesa_conv_vtss_policer_t_to_mesa_policer_t(const vtss_policer_t *in, mesa_policer_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_policer_t *tmp = (const mesa_policer_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
inline static mesa_rc mesa_conv_mesa_policer_t_to_vtss_policer_t(const mesa_policer_t *in, vtss_policer_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_policer_t *tmp = (const vtss_policer_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
inline static mesa_rc mesa_conv_vtss_dscp_mode_t_to_mesa_dscp_mode_t(const vtss_dscp_mode_t *in, mesa_dscp_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_dscp_mode_t *tmp = (const mesa_dscp_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
inline static mesa_rc mesa_conv_mesa_dscp_mode_t_to_vtss_dscp_mode_t(const mesa_dscp_mode_t *in, vtss_dscp_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_dscp_mode_t *tmp = (const vtss_dscp_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv_vtss_dscp_emode_t_to_mesa_dscp_emode_t(const vtss_dscp_emode_t *in, mesa_dscp_emode_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv_mesa_dscp_emode_t_to_vtss_dscp_emode_t(const mesa_dscp_emode_t *in, vtss_dscp_emode_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv_vtss_tag_remark_mode_t_to_mesa_tag_remark_mode_t(const vtss_tag_remark_mode_t *in, mesa_tag_remark_mode_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv_mesa_tag_remark_mode_t_to_vtss_tag_remark_mode_t(const mesa_tag_remark_mode_t *in, vtss_tag_remark_mode_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_RT)
mesa_rc mesa_conv_vtss_shaper_mode_t_to_mesa_shaper_mode_t(const vtss_shaper_mode_t *in, mesa_shaper_mode_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_RT)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_RT)
mesa_rc mesa_conv_mesa_shaper_mode_t_to_vtss_shaper_mode_t(const mesa_shaper_mode_t *in, vtss_shaper_mode_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_RT)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv_vtss_shaper_t_to_mesa_shaper_t(const vtss_shaper_t *in, mesa_shaper_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv_mesa_shaper_t_to_vtss_shaper_t(const mesa_shaper_t *in, vtss_shaper_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv_vtss_qos_port_conf_t_to_mesa_qos_port_conf_t(const vtss_qos_port_conf_t *in, mesa_qos_port_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv_mesa_qos_port_conf_t_to_vtss_qos_port_conf_t(const mesa_qos_port_conf_t *in, vtss_qos_port_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
inline static mesa_rc mesa_conv_vtss_qce_type_t_to_mesa_qce_type_t(const vtss_qce_type_t *in, mesa_qce_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qce_type_t *tmp = (const mesa_qce_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
inline static mesa_rc mesa_conv_mesa_qce_type_t_to_vtss_qce_type_t(const mesa_qce_type_t *in, vtss_qce_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qce_type_t *tmp = (const vtss_qce_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_vtss_qce_mac_t_to_mesa_qce_mac_t(const vtss_qce_mac_t *in, mesa_qce_mac_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_mesa_qce_mac_t_to_vtss_qce_mac_t(const mesa_qce_mac_t *in, vtss_qce_mac_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
inline static mesa_rc mesa_conv_vtss_qce_tag_t_to_mesa_qce_tag_t(const vtss_qce_tag_t *in, mesa_qce_tag_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qce_tag_t *tmp = (const mesa_qce_tag_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
inline static mesa_rc mesa_conv_mesa_qce_tag_t_to_vtss_qce_tag_t(const mesa_qce_tag_t *in, vtss_qce_tag_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qce_tag_t *tmp = (const vtss_qce_tag_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
inline static mesa_rc mesa_conv_vtss_qce_frame_etype_t_to_mesa_qce_frame_etype_t(const vtss_qce_frame_etype_t *in, mesa_qce_frame_etype_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qce_frame_etype_t *tmp = (const mesa_qce_frame_etype_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
inline static mesa_rc mesa_conv_mesa_qce_frame_etype_t_to_vtss_qce_frame_etype_t(const mesa_qce_frame_etype_t *in, vtss_qce_frame_etype_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qce_frame_etype_t *tmp = (const vtss_qce_frame_etype_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
inline static mesa_rc mesa_conv_vtss_qce_frame_llc_t_to_mesa_qce_frame_llc_t(const vtss_qce_frame_llc_t *in, mesa_qce_frame_llc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qce_frame_llc_t *tmp = (const mesa_qce_frame_llc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
inline static mesa_rc mesa_conv_mesa_qce_frame_llc_t_to_vtss_qce_frame_llc_t(const mesa_qce_frame_llc_t *in, vtss_qce_frame_llc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qce_frame_llc_t *tmp = (const vtss_qce_frame_llc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
inline static mesa_rc mesa_conv_vtss_qce_frame_snap_t_to_mesa_qce_frame_snap_t(const vtss_qce_frame_snap_t *in, mesa_qce_frame_snap_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qce_frame_snap_t *tmp = (const mesa_qce_frame_snap_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
inline static mesa_rc mesa_conv_mesa_qce_frame_snap_t_to_vtss_qce_frame_snap_t(const mesa_qce_frame_snap_t *in, vtss_qce_frame_snap_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qce_frame_snap_t *tmp = (const vtss_qce_frame_snap_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_vtss_qce_frame_ipv4_t_to_mesa_qce_frame_ipv4_t(const vtss_qce_frame_ipv4_t *in, mesa_qce_frame_ipv4_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_mesa_qce_frame_ipv4_t_to_vtss_qce_frame_ipv4_t(const mesa_qce_frame_ipv4_t *in, vtss_qce_frame_ipv4_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_vtss_qce_frame_ipv6_t_to_mesa_qce_frame_ipv6_t(const vtss_qce_frame_ipv6_t *in, mesa_qce_frame_ipv6_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_mesa_qce_frame_ipv6_t_to_vtss_qce_frame_ipv6_t(const mesa_qce_frame_ipv6_t *in, vtss_qce_frame_ipv6_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_vtss_qce_key_t_to_mesa_qce_key_t(const vtss_qce_key_t *in, mesa_qce_key_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_mesa_qce_key_t_to_vtss_qce_key_t(const mesa_qce_key_t *in, vtss_qce_key_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_vtss_qce_action_t_to_mesa_qce_action_t(const vtss_qce_action_t *in, mesa_qce_action_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_mesa_qce_action_t_to_vtss_qce_action_t(const mesa_qce_action_t *in, vtss_qce_action_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_vtss_qce_t_to_mesa_qce_t(const vtss_qce_t *in, mesa_qce_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_mesa_qce_t_to_vtss_qce_t(const mesa_qce_t *in, vtss_qce_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_vtss_qos_ingress_map_key_t_to_mesa_qos_ingress_map_key_t(const vtss_qos_ingress_map_key_t *in, mesa_qos_ingress_map_key_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_ingress_map_key_t *tmp = (const mesa_qos_ingress_map_key_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_mesa_qos_ingress_map_key_t_to_vtss_qos_ingress_map_key_t(const mesa_qos_ingress_map_key_t *in, vtss_qos_ingress_map_key_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_ingress_map_key_t *tmp = (const vtss_qos_ingress_map_key_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_vtss_qos_ingress_map_action_t_to_mesa_qos_ingress_map_action_t(const vtss_qos_ingress_map_action_t *in, mesa_qos_ingress_map_action_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_ingress_map_action_t *tmp = (const mesa_qos_ingress_map_action_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_mesa_qos_ingress_map_action_t_to_vtss_qos_ingress_map_action_t(const mesa_qos_ingress_map_action_t *in, vtss_qos_ingress_map_action_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_ingress_map_action_t *tmp = (const vtss_qos_ingress_map_action_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_vtss_qos_ingress_map_values_t_to_mesa_qos_ingress_map_values_t(const vtss_qos_ingress_map_values_t *in, mesa_qos_ingress_map_values_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_ingress_map_values_t *tmp = (const mesa_qos_ingress_map_values_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_mesa_qos_ingress_map_values_t_to_vtss_qos_ingress_map_values_t(const mesa_qos_ingress_map_values_t *in, vtss_qos_ingress_map_values_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_ingress_map_values_t *tmp = (const vtss_qos_ingress_map_values_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_vtss_qos_ingress_map_dpd_t_to_mesa_qos_ingress_map_dpd_t(const vtss_qos_ingress_map_dpd_t *in, mesa_qos_ingress_map_dpd_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_ingress_map_dpd_t *tmp = (const mesa_qos_ingress_map_dpd_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_mesa_qos_ingress_map_dpd_t_to_vtss_qos_ingress_map_dpd_t(const mesa_qos_ingress_map_dpd_t *in, vtss_qos_ingress_map_dpd_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_ingress_map_dpd_t *tmp = (const vtss_qos_ingress_map_dpd_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_vtss_qos_ingress_map_maps_t_to_mesa_qos_ingress_map_maps_t(const vtss_qos_ingress_map_maps_t *in, mesa_qos_ingress_map_maps_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_ingress_map_maps_t *tmp = (const mesa_qos_ingress_map_maps_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_mesa_qos_ingress_map_maps_t_to_vtss_qos_ingress_map_maps_t(const mesa_qos_ingress_map_maps_t *in, vtss_qos_ingress_map_maps_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_ingress_map_maps_t *tmp = (const vtss_qos_ingress_map_maps_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_vtss_qos_ingress_map_t_to_mesa_qos_ingress_map_t(const vtss_qos_ingress_map_t *in, mesa_qos_ingress_map_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_ingress_map_t *tmp = (const mesa_qos_ingress_map_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_mesa_qos_ingress_map_t_to_vtss_qos_ingress_map_t(const mesa_qos_ingress_map_t *in, vtss_qos_ingress_map_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_ingress_map_t *tmp = (const vtss_qos_ingress_map_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_vtss_qos_egress_map_key_t_to_mesa_qos_egress_map_key_t(const vtss_qos_egress_map_key_t *in, mesa_qos_egress_map_key_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_egress_map_key_t *tmp = (const mesa_qos_egress_map_key_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_mesa_qos_egress_map_key_t_to_vtss_qos_egress_map_key_t(const mesa_qos_egress_map_key_t *in, vtss_qos_egress_map_key_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_egress_map_key_t *tmp = (const vtss_qos_egress_map_key_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_vtss_qos_egress_map_action_t_to_mesa_qos_egress_map_action_t(const vtss_qos_egress_map_action_t *in, mesa_qos_egress_map_action_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_egress_map_action_t *tmp = (const mesa_qos_egress_map_action_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_mesa_qos_egress_map_action_t_to_vtss_qos_egress_map_action_t(const mesa_qos_egress_map_action_t *in, vtss_qos_egress_map_action_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_egress_map_action_t *tmp = (const vtss_qos_egress_map_action_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_vtss_qos_egress_map_values_t_to_mesa_qos_egress_map_values_t(const vtss_qos_egress_map_values_t *in, mesa_qos_egress_map_values_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_egress_map_values_t *tmp = (const mesa_qos_egress_map_values_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_mesa_qos_egress_map_values_t_to_vtss_qos_egress_map_values_t(const mesa_qos_egress_map_values_t *in, vtss_qos_egress_map_values_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_egress_map_values_t *tmp = (const vtss_qos_egress_map_values_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_vtss_qos_egress_map_maps_t_to_mesa_qos_egress_map_maps_t(const vtss_qos_egress_map_maps_t *in, mesa_qos_egress_map_maps_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_egress_map_maps_t *tmp = (const mesa_qos_egress_map_maps_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_mesa_qos_egress_map_maps_t_to_vtss_qos_egress_map_maps_t(const mesa_qos_egress_map_maps_t *in, vtss_qos_egress_map_maps_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_egress_map_maps_t *tmp = (const vtss_qos_egress_map_maps_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_vtss_qos_egress_map_t_to_mesa_qos_egress_map_t(const vtss_qos_egress_map_t *in, mesa_qos_egress_map_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_egress_map_t *tmp = (const mesa_qos_egress_map_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
inline static mesa_rc mesa_conv_mesa_qos_egress_map_t_to_vtss_qos_egress_map_t(const mesa_qos_egress_map_t *in, vtss_qos_egress_map_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_egress_map_t *tmp = (const vtss_qos_egress_map_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) || defined(VTSS_FEATURE_QOS_EGRESS_MAP)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
mesa_rc mesa_conv_vtss_dlb_policer_conf_t_to_mesa_dlb_policer_conf_t(const vtss_dlb_policer_conf_t *in, mesa_dlb_policer_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS_POLICER_DLB)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
mesa_rc mesa_conv_mesa_dlb_policer_conf_t_to_vtss_dlb_policer_conf_t(const mesa_dlb_policer_conf_t *in, vtss_dlb_policer_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS_POLICER_DLB)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_vtss_auth_state_t_to_mesa_auth_state_t(const vtss_auth_state_t *in, mesa_auth_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_auth_state_t *tmp = (const mesa_auth_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
inline static mesa_rc mesa_conv_mesa_auth_state_t_to_vtss_auth_state_t(const mesa_auth_state_t *in, vtss_auth_state_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_auth_state_t *tmp = (const vtss_auth_state_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_acl_policer_conf_t_to_mesa_acl_policer_conf_t(const vtss_acl_policer_conf_t *in, mesa_acl_policer_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_acl_policer_conf_t *tmp = (const mesa_acl_policer_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_mesa_acl_policer_conf_t_to_vtss_acl_policer_conf_t(const mesa_acl_policer_conf_t *in, vtss_acl_policer_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_acl_policer_conf_t *tmp = (const vtss_acl_policer_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_acl_port_action_t_to_mesa_acl_port_action_t(const vtss_acl_port_action_t *in, mesa_acl_port_action_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_acl_port_action_t *tmp = (const mesa_acl_port_action_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_mesa_acl_port_action_t_to_vtss_acl_port_action_t(const mesa_acl_port_action_t *in, vtss_acl_port_action_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_acl_port_action_t *tmp = (const vtss_acl_port_action_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_vtss_acl_ptp_action_t_to_mesa_acl_ptp_action_t(const vtss_acl_ptp_action_t *in, mesa_acl_ptp_action_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_mesa_acl_ptp_action_t_to_vtss_acl_ptp_action_t(const mesa_acl_ptp_action_t *in, vtss_acl_ptp_action_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
inline static mesa_rc mesa_conv_vtss_acl_sip_conf_t_to_mesa_acl_sip_conf_t(const vtss_acl_sip_conf_t *in, mesa_acl_sip_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_acl_sip_conf_t *tmp = (const mesa_acl_sip_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
inline static mesa_rc mesa_conv_mesa_acl_sip_conf_t_to_vtss_acl_sip_conf_t(const mesa_acl_sip_conf_t *in, vtss_acl_sip_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_acl_sip_conf_t *tmp = (const vtss_acl_sip_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
inline static mesa_rc mesa_conv_vtss_acl_ptp_rsp_t_to_mesa_acl_ptp_rsp_t(const vtss_acl_ptp_rsp_t *in, mesa_acl_ptp_rsp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_acl_ptp_rsp_t *tmp = (const mesa_acl_ptp_rsp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
inline static mesa_rc mesa_conv_mesa_acl_ptp_rsp_t_to_vtss_acl_ptp_rsp_t(const mesa_acl_ptp_rsp_t *in, vtss_acl_ptp_rsp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_acl_ptp_rsp_t *tmp = (const vtss_acl_ptp_rsp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
inline static mesa_rc mesa_conv_vtss_acl_ptp_action_conf_t_to_mesa_acl_ptp_action_conf_t(const vtss_acl_ptp_action_conf_t *in, mesa_acl_ptp_action_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_acl_ptp_action_conf_t *tmp = (const mesa_acl_ptp_action_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
inline static mesa_rc mesa_conv_mesa_acl_ptp_action_conf_t_to_vtss_acl_ptp_action_conf_t(const mesa_acl_ptp_action_conf_t *in, vtss_acl_ptp_action_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_acl_ptp_action_conf_t *tmp = (const vtss_acl_ptp_action_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
inline static mesa_rc mesa_conv_vtss_acl_addr_update_t_to_mesa_acl_addr_update_t(const vtss_acl_addr_update_t *in, mesa_acl_addr_update_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_acl_addr_update_t *tmp = (const mesa_acl_addr_update_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
inline static mesa_rc mesa_conv_mesa_acl_addr_update_t_to_vtss_acl_addr_update_t(const mesa_acl_addr_update_t *in, vtss_acl_addr_update_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_acl_addr_update_t *tmp = (const vtss_acl_addr_update_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
inline static mesa_rc mesa_conv_vtss_acl_addr_action_t_to_mesa_acl_addr_action_t(const vtss_acl_addr_action_t *in, mesa_acl_addr_action_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_acl_addr_action_t *tmp = (const mesa_acl_addr_action_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
inline static mesa_rc mesa_conv_mesa_acl_addr_action_t_to_vtss_acl_addr_action_t(const mesa_acl_addr_action_t *in, vtss_acl_addr_action_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_acl_addr_action_t *tmp = (const vtss_acl_addr_action_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_vtss_acl_action_t_to_mesa_acl_action_t(const vtss_acl_action_t *in, mesa_acl_action_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_mesa_acl_action_t_to_vtss_acl_action_t(const mesa_acl_action_t *in, vtss_acl_action_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_acl_key_t_to_mesa_acl_key_t(const vtss_acl_key_t *in, mesa_acl_key_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_acl_key_t *tmp = (const mesa_acl_key_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_mesa_acl_key_t_to_vtss_acl_key_t(const mesa_acl_key_t *in, vtss_acl_key_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_acl_key_t *tmp = (const vtss_acl_key_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_acl_frame_key_t_to_mesa_acl_frame_key_t(const vtss_acl_frame_key_t *in, mesa_acl_frame_key_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_acl_frame_key_t *tmp = (const mesa_acl_frame_key_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_mesa_acl_frame_key_t_to_vtss_acl_frame_key_t(const mesa_acl_frame_key_t *in, vtss_acl_frame_key_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_acl_frame_key_t *tmp = (const vtss_acl_frame_key_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_vtss_acl_port_conf_t_to_mesa_acl_port_conf_t(const vtss_acl_port_conf_t *in, mesa_acl_port_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_mesa_acl_port_conf_t_to_vtss_acl_port_conf_t(const mesa_acl_port_conf_t *in, vtss_acl_port_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_ace_type_t_to_mesa_ace_type_t(const vtss_ace_type_t *in, mesa_ace_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ace_type_t *tmp = (const mesa_ace_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_mesa_ace_type_t_to_vtss_ace_type_t(const mesa_ace_type_t *in, vtss_ace_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ace_type_t *tmp = (const vtss_ace_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_ace_bit_t_to_mesa_ace_bit_t(const vtss_ace_bit_t *in, mesa_ace_bit_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_mesa_ace_bit_t_to_vtss_ace_bit_t(const mesa_ace_bit_t *in, vtss_ace_bit_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_vcap_u8_t_to_mesa_vcap_u8_t(const vtss_vcap_u8_t *in, mesa_vcap_u8_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcap_u8_t *tmp = (const mesa_vcap_u8_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vcap_u8_t_to_vtss_vcap_u8_t(const mesa_vcap_u8_t *in, vtss_vcap_u8_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcap_u8_t *tmp = (const vtss_vcap_u8_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vcap_u16_t_to_mesa_vcap_u16_t(const vtss_vcap_u16_t *in, mesa_vcap_u16_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcap_u16_t *tmp = (const mesa_vcap_u16_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vcap_u16_t_to_vtss_vcap_u16_t(const mesa_vcap_u16_t *in, vtss_vcap_u16_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcap_u16_t *tmp = (const vtss_vcap_u16_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vcap_u32_t_to_mesa_vcap_u32_t(const vtss_vcap_u32_t *in, mesa_vcap_u32_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcap_u32_t *tmp = (const mesa_vcap_u32_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vcap_u32_t_to_vtss_vcap_u32_t(const mesa_vcap_u32_t *in, vtss_vcap_u32_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcap_u32_t *tmp = (const vtss_vcap_u32_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vcap_u40_t_to_mesa_vcap_u40_t(const vtss_vcap_u40_t *in, mesa_vcap_u40_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcap_u40_t *tmp = (const mesa_vcap_u40_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vcap_u40_t_to_vtss_vcap_u40_t(const mesa_vcap_u40_t *in, vtss_vcap_u40_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcap_u40_t *tmp = (const vtss_vcap_u40_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vcap_u48_t_to_mesa_vcap_u48_t(const vtss_vcap_u48_t *in, mesa_vcap_u48_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcap_u48_t *tmp = (const mesa_vcap_u48_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vcap_u48_t_to_vtss_vcap_u48_t(const mesa_vcap_u48_t *in, vtss_vcap_u48_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcap_u48_t *tmp = (const vtss_vcap_u48_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vcap_u128_t_to_mesa_vcap_u128_t(const vtss_vcap_u128_t *in, mesa_vcap_u128_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcap_u128_t *tmp = (const mesa_vcap_u128_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vcap_u128_t_to_vtss_vcap_u128_t(const mesa_vcap_u128_t *in, vtss_vcap_u128_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcap_u128_t *tmp = (const vtss_vcap_u128_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vcap_vid_t_to_mesa_vcap_vid_t(const vtss_vcap_vid_t *in, mesa_vcap_vid_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcap_vid_t *tmp = (const mesa_vcap_vid_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vcap_vid_t_to_vtss_vcap_vid_t(const mesa_vcap_vid_t *in, vtss_vcap_vid_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcap_vid_t *tmp = (const vtss_vcap_vid_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vcap_ip_t_to_mesa_vcap_ip_t(const vtss_vcap_ip_t *in, mesa_vcap_ip_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcap_ip_t *tmp = (const mesa_vcap_ip_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vcap_ip_t_to_vtss_vcap_ip_t(const mesa_vcap_ip_t *in, vtss_vcap_ip_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcap_ip_t *tmp = (const vtss_vcap_ip_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vcap_udp_tcp_t_to_mesa_vcap_udp_tcp_t(const vtss_vcap_udp_tcp_t *in, mesa_vcap_udp_tcp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcap_udp_tcp_t *tmp = (const mesa_vcap_udp_tcp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vcap_udp_tcp_t_to_vtss_vcap_udp_tcp_t(const mesa_vcap_udp_tcp_t *in, vtss_vcap_udp_tcp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcap_udp_tcp_t *tmp = (const vtss_vcap_udp_tcp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_ace_ptp_t_to_mesa_ace_ptp_t(const vtss_ace_ptp_t *in, mesa_ace_ptp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ace_ptp_t *tmp = (const mesa_ace_ptp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_mesa_ace_ptp_t_to_vtss_ace_ptp_t(const mesa_ace_ptp_t *in, vtss_ace_ptp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ace_ptp_t *tmp = (const vtss_ace_ptp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_ace_sip_smac_t_to_mesa_ace_sip_smac_t(const vtss_ace_sip_smac_t *in, mesa_ace_sip_smac_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ace_sip_smac_t *tmp = (const mesa_ace_sip_smac_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_mesa_ace_sip_smac_t_to_vtss_ace_sip_smac_t(const mesa_ace_sip_smac_t *in, vtss_ace_sip_smac_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ace_sip_smac_t *tmp = (const vtss_ace_sip_smac_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_ace_vlan_t_to_mesa_ace_vlan_t(const vtss_ace_vlan_t *in, mesa_ace_vlan_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ace_vlan_t *tmp = (const mesa_ace_vlan_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_mesa_ace_vlan_t_to_vtss_ace_vlan_t(const mesa_ace_vlan_t *in, vtss_ace_vlan_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ace_vlan_t *tmp = (const vtss_ace_vlan_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_ace_frame_any_t_to_mesa_ace_frame_any_t(const vtss_ace_frame_any_t *in, mesa_ace_frame_any_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ace_frame_any_t *tmp = (const mesa_ace_frame_any_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_mesa_ace_frame_any_t_to_vtss_ace_frame_any_t(const mesa_ace_frame_any_t *in, vtss_ace_frame_any_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ace_frame_any_t *tmp = (const vtss_ace_frame_any_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_ace_frame_etype_t_to_mesa_ace_frame_etype_t(const vtss_ace_frame_etype_t *in, mesa_ace_frame_etype_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ace_frame_etype_t *tmp = (const mesa_ace_frame_etype_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_mesa_ace_frame_etype_t_to_vtss_ace_frame_etype_t(const mesa_ace_frame_etype_t *in, vtss_ace_frame_etype_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ace_frame_etype_t *tmp = (const vtss_ace_frame_etype_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_ace_frame_llc_t_to_mesa_ace_frame_llc_t(const vtss_ace_frame_llc_t *in, mesa_ace_frame_llc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ace_frame_llc_t *tmp = (const mesa_ace_frame_llc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_mesa_ace_frame_llc_t_to_vtss_ace_frame_llc_t(const mesa_ace_frame_llc_t *in, vtss_ace_frame_llc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ace_frame_llc_t *tmp = (const vtss_ace_frame_llc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_ace_frame_snap_t_to_mesa_ace_frame_snap_t(const vtss_ace_frame_snap_t *in, mesa_ace_frame_snap_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ace_frame_snap_t *tmp = (const mesa_ace_frame_snap_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_mesa_ace_frame_snap_t_to_vtss_ace_frame_snap_t(const mesa_ace_frame_snap_t *in, vtss_ace_frame_snap_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ace_frame_snap_t *tmp = (const vtss_ace_frame_snap_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_ace_frame_arp_t_to_mesa_ace_frame_arp_t(const vtss_ace_frame_arp_t *in, mesa_ace_frame_arp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ace_frame_arp_t *tmp = (const mesa_ace_frame_arp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_mesa_ace_frame_arp_t_to_vtss_ace_frame_arp_t(const mesa_ace_frame_arp_t *in, vtss_ace_frame_arp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ace_frame_arp_t *tmp = (const vtss_ace_frame_arp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_vtss_ace_frame_ipv4_t_to_mesa_ace_frame_ipv4_t(const vtss_ace_frame_ipv4_t *in, mesa_ace_frame_ipv4_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_mesa_ace_frame_ipv4_t_to_vtss_ace_frame_ipv4_t(const mesa_ace_frame_ipv4_t *in, vtss_ace_frame_ipv4_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_vtss_ace_frame_ipv6_t_to_mesa_ace_frame_ipv6_t(const vtss_ace_frame_ipv6_t *in, mesa_ace_frame_ipv6_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_mesa_ace_frame_ipv6_t_to_vtss_ace_frame_ipv6_t(const mesa_ace_frame_ipv6_t *in, vtss_ace_frame_ipv6_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_vtss_ace_t_to_mesa_ace_t(const vtss_ace_t *in, mesa_ace_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_mesa_ace_t_to_vtss_ace_t(const mesa_ace_t *in, vtss_ace_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
#if defined(VTSS_ARCH_LUTON26)
inline static mesa_rc mesa_conv_vtss_ace_status_t_to_mesa_ace_status_t(const vtss_ace_status_t *in, mesa_ace_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ace_status_t *tmp = (const mesa_ace_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_LUTON26)
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
#if defined(VTSS_ARCH_LUTON26)
inline static mesa_rc mesa_conv_mesa_ace_status_t_to_vtss_ace_status_t(const mesa_ace_status_t *in, vtss_ace_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ace_status_t *tmp = (const vtss_ace_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_LUTON26)
#endif  // defined(VTSS_FEATURE_ACL)
inline static mesa_rc mesa_conv_vtss_hacl_type_t_to_mesa_hacl_type_t(const vtss_hacl_type_t *in, mesa_hacl_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_hacl_type_t *tmp = (const mesa_hacl_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_hacl_type_t_to_vtss_hacl_type_t(const mesa_hacl_type_t *in, vtss_hacl_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_hacl_type_t *tmp = (const vtss_hacl_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_rleg_list_t_to_mesa_rleg_list_t(const vtss_rleg_list_t *in, mesa_rleg_list_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_rleg_list_t *tmp = (const mesa_rleg_list_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_rleg_list_t_to_vtss_rleg_list_t(const mesa_rleg_list_t *in, vtss_rleg_list_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_rleg_list_t *tmp = (const vtss_rleg_list_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#if defined(VTSS_FEATURE_HACL)
inline static mesa_rc mesa_conv_vtss_hace_frame_etype_t_to_mesa_hace_frame_etype_t(const vtss_hace_frame_etype_t *in, mesa_hace_frame_etype_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_hace_frame_etype_t *tmp = (const mesa_hace_frame_etype_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
inline static mesa_rc mesa_conv_mesa_hace_frame_etype_t_to_vtss_hace_frame_etype_t(const mesa_hace_frame_etype_t *in, vtss_hace_frame_etype_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_hace_frame_etype_t *tmp = (const vtss_hace_frame_etype_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
inline static mesa_rc mesa_conv_vtss_hace_frame_llc_t_to_mesa_hace_frame_llc_t(const vtss_hace_frame_llc_t *in, mesa_hace_frame_llc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_hace_frame_llc_t *tmp = (const mesa_hace_frame_llc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
inline static mesa_rc mesa_conv_mesa_hace_frame_llc_t_to_vtss_hace_frame_llc_t(const mesa_hace_frame_llc_t *in, vtss_hace_frame_llc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_hace_frame_llc_t *tmp = (const vtss_hace_frame_llc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
inline static mesa_rc mesa_conv_vtss_hace_frame_snap_t_to_mesa_hace_frame_snap_t(const vtss_hace_frame_snap_t *in, mesa_hace_frame_snap_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_hace_frame_snap_t *tmp = (const mesa_hace_frame_snap_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
inline static mesa_rc mesa_conv_mesa_hace_frame_snap_t_to_vtss_hace_frame_snap_t(const mesa_hace_frame_snap_t *in, vtss_hace_frame_snap_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_hace_frame_snap_t *tmp = (const vtss_hace_frame_snap_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
inline static mesa_rc mesa_conv_vtss_hace_frame_arp_t_to_mesa_hace_frame_arp_t(const vtss_hace_frame_arp_t *in, mesa_hace_frame_arp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_hace_frame_arp_t *tmp = (const mesa_hace_frame_arp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
inline static mesa_rc mesa_conv_mesa_hace_frame_arp_t_to_vtss_hace_frame_arp_t(const mesa_hace_frame_arp_t *in, vtss_hace_frame_arp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_hace_frame_arp_t *tmp = (const vtss_hace_frame_arp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
inline static mesa_rc mesa_conv_vtss_hace_frame_ipv4_t_to_mesa_hace_frame_ipv4_t(const vtss_hace_frame_ipv4_t *in, mesa_hace_frame_ipv4_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_hace_frame_ipv4_t *tmp = (const mesa_hace_frame_ipv4_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
inline static mesa_rc mesa_conv_mesa_hace_frame_ipv4_t_to_vtss_hace_frame_ipv4_t(const mesa_hace_frame_ipv4_t *in, vtss_hace_frame_ipv4_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_hace_frame_ipv4_t *tmp = (const vtss_hace_frame_ipv4_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
inline static mesa_rc mesa_conv_vtss_hace_frame_ipv6_t_to_mesa_hace_frame_ipv6_t(const vtss_hace_frame_ipv6_t *in, mesa_hace_frame_ipv6_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_hace_frame_ipv6_t *tmp = (const mesa_hace_frame_ipv6_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
inline static mesa_rc mesa_conv_mesa_hace_frame_ipv6_t_to_vtss_hace_frame_ipv6_t(const mesa_hace_frame_ipv6_t *in, vtss_hace_frame_ipv6_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_hace_frame_ipv6_t *tmp = (const vtss_hace_frame_ipv6_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
mesa_rc mesa_conv_vtss_hace_key_t_to_mesa_hace_key_t(const vtss_hace_key_t *in, mesa_hace_key_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
mesa_rc mesa_conv_mesa_hace_key_t_to_vtss_hace_key_t(const mesa_hace_key_t *in, vtss_hace_key_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
mesa_rc mesa_conv_vtss_hacl_action_t_to_mesa_hacl_action_t(const vtss_hacl_action_t *in, mesa_hacl_action_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
mesa_rc mesa_conv_mesa_hacl_action_t_to_vtss_hacl_action_t(const mesa_hacl_action_t *in, vtss_hacl_action_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
mesa_rc mesa_conv_vtss_hace_t_to_mesa_hace_t(const vtss_hace_t *in, mesa_hace_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_HACL)
mesa_rc mesa_conv_mesa_hace_t_to_vtss_hace_t(const mesa_hace_t *in, vtss_hace_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_HACL)
#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_conv_vtss_synce_divider_t_to_mesa_synce_divider_t(const vtss_synce_divider_t *in, mesa_synce_divider_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_SYNCE)
#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_conv_mesa_synce_divider_t_to_vtss_synce_divider_t(const mesa_synce_divider_t *in, vtss_synce_divider_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_SYNCE)
#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_conv_vtss_synce_clock_out_t_to_mesa_synce_clock_out_t(const vtss_synce_clock_out_t *in, mesa_synce_clock_out_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_SYNCE)
#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_conv_mesa_synce_clock_out_t_to_vtss_synce_clock_out_t(const mesa_synce_clock_out_t *in, vtss_synce_clock_out_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_SYNCE)
#if defined(VTSS_FEATURE_SYNCE)
#if defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
mesa_rc mesa_conv_vtss_synce_clock_in_type_t_to_mesa_synce_clock_in_type_t(const vtss_synce_clock_in_type_t *in, mesa_synce_clock_in_type_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#endif  // defined(VTSS_FEATURE_SYNCE)
#if defined(VTSS_FEATURE_SYNCE)
#if defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
mesa_rc mesa_conv_mesa_synce_clock_in_type_t_to_vtss_synce_clock_in_type_t(const mesa_synce_clock_in_type_t *in, vtss_synce_clock_in_type_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#endif  // defined(VTSS_FEATURE_SYNCE)
#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_conv_vtss_synce_clock_in_t_to_mesa_synce_clock_in_t(const vtss_synce_clock_in_t *in, mesa_synce_clock_in_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_SYNCE)
#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_conv_mesa_synce_clock_in_t_to_vtss_synce_clock_in_t(const mesa_synce_clock_in_t *in, vtss_synce_clock_in_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_SYNCE)
#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_conv_vtss_synce_station_clock_out_t_to_mesa_synce_station_clock_out_t(const vtss_synce_station_clock_out_t *in, mesa_synce_station_clock_out_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_SYNCE)
#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_conv_mesa_synce_station_clock_out_t_to_vtss_synce_station_clock_out_t(const mesa_synce_station_clock_out_t *in, vtss_synce_station_clock_out_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_SYNCE)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_vtss_ts_conf_t_to_mesa_ts_conf_t(const vtss_ts_conf_t *in, mesa_ts_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ts_conf_t *tmp = (const mesa_ts_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_mesa_ts_conf_t_to_vtss_ts_conf_t(const mesa_ts_conf_t *in, vtss_ts_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ts_conf_t *tmp = (const vtss_ts_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X) /* TBD_henrikb */
inline static mesa_rc mesa_conv_vtss_ts_alt_clock_mode_t_to_mesa_ts_alt_clock_mode_t(const vtss_ts_alt_clock_mode_t *in, mesa_ts_alt_clock_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ts_alt_clock_mode_t *tmp = (const mesa_ts_alt_clock_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X) /* TBD_henrikb */
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X) /* TBD_henrikb */
inline static mesa_rc mesa_conv_mesa_ts_alt_clock_mode_t_to_vtss_ts_alt_clock_mode_t(const mesa_ts_alt_clock_mode_t *in, vtss_ts_alt_clock_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ts_alt_clock_mode_t *tmp = (const vtss_ts_alt_clock_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X) /* TBD_henrikb */
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_vtss_ts_ext_clock_one_pps_mode_t_to_mesa_ts_ext_clock_one_pps_mode_t(const vtss_ts_ext_clock_one_pps_mode_t *in, mesa_ts_ext_clock_one_pps_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ts_ext_clock_one_pps_mode_t *tmp = (const mesa_ts_ext_clock_one_pps_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_mesa_ts_ext_clock_one_pps_mode_t_to_vtss_ts_ext_clock_one_pps_mode_t(const mesa_ts_ext_clock_one_pps_mode_t *in, vtss_ts_ext_clock_one_pps_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ts_ext_clock_one_pps_mode_t *tmp = (const vtss_ts_ext_clock_one_pps_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_vtss_ts_ext_clock_mode_t_to_mesa_ts_ext_clock_mode_t(const vtss_ts_ext_clock_mode_t *in, mesa_ts_ext_clock_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ts_ext_clock_mode_t *tmp = (const mesa_ts_ext_clock_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_mesa_ts_ext_clock_mode_t_to_vtss_ts_ext_clock_mode_t(const mesa_ts_ext_clock_mode_t *in, vtss_ts_ext_clock_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ts_ext_clock_mode_t *tmp = (const vtss_ts_ext_clock_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT)
inline static mesa_rc mesa_conv_vtss_ts_ext_io_pin_cfg_t_to_mesa_ts_ext_io_pin_cfg_t(const vtss_ts_ext_io_pin_cfg_t *in, mesa_ts_ext_io_pin_cfg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ts_ext_io_pin_cfg_t *tmp = (const mesa_ts_ext_io_pin_cfg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT)
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT)
inline static mesa_rc mesa_conv_mesa_ts_ext_io_pin_cfg_t_to_vtss_ts_ext_io_pin_cfg_t(const mesa_ts_ext_io_pin_cfg_t *in, vtss_ts_ext_io_pin_cfg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ts_ext_io_pin_cfg_t *tmp = (const vtss_ts_ext_io_pin_cfg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT)
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT)
inline static mesa_rc mesa_conv_vtss_ts_ext_io_mode_t_to_mesa_ts_ext_io_mode_t(const vtss_ts_ext_io_mode_t *in, mesa_ts_ext_io_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ts_ext_io_mode_t *tmp = (const mesa_ts_ext_io_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT)
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT)
inline static mesa_rc mesa_conv_mesa_ts_ext_io_mode_t_to_vtss_ts_ext_io_mode_t(const mesa_ts_ext_io_mode_t *in, vtss_ts_ext_io_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ts_ext_io_mode_t *tmp = (const vtss_ts_ext_io_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT)
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_vtss_ts_mode_t_to_mesa_ts_mode_t(const vtss_ts_mode_t *in, mesa_ts_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ts_mode_t *tmp = (const mesa_ts_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_mesa_ts_mode_t_to_vtss_ts_mode_t(const mesa_ts_mode_t *in, vtss_ts_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ts_mode_t *tmp = (const vtss_ts_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_conv_vtss_ts_operation_mode_t_to_mesa_ts_operation_mode_t(const vtss_ts_operation_mode_t *in, mesa_ts_operation_mode_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_conv_mesa_ts_operation_mode_t_to_vtss_ts_operation_mode_t(const mesa_ts_operation_mode_t *in, vtss_ts_operation_mode_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_vtss_ts_internal_fmt_t_to_mesa_ts_internal_fmt_t(const vtss_ts_internal_fmt_t *in, mesa_ts_internal_fmt_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ts_internal_fmt_t *tmp = (const mesa_ts_internal_fmt_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_mesa_ts_internal_fmt_t_to_vtss_ts_internal_fmt_t(const mesa_ts_internal_fmt_t *in, vtss_ts_internal_fmt_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ts_internal_fmt_t *tmp = (const vtss_ts_internal_fmt_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_vtss_ts_internal_mode_t_to_mesa_ts_internal_mode_t(const vtss_ts_internal_mode_t *in, mesa_ts_internal_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ts_internal_mode_t *tmp = (const mesa_ts_internal_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_mesa_ts_internal_mode_t_to_vtss_ts_internal_mode_t(const mesa_ts_internal_mode_t *in, vtss_ts_internal_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ts_internal_mode_t *tmp = (const vtss_ts_internal_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_vtss_ts_id_t_to_mesa_ts_id_t(const vtss_ts_id_t *in, mesa_ts_id_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ts_id_t *tmp = (const mesa_ts_id_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_mesa_ts_id_t_to_vtss_ts_id_t(const mesa_ts_id_t *in, vtss_ts_id_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ts_id_t *tmp = (const vtss_ts_id_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_vtss_ts_timestamp_t_to_mesa_ts_timestamp_t(const vtss_ts_timestamp_t *in, mesa_ts_timestamp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ts_timestamp_t *tmp = (const mesa_ts_timestamp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_mesa_ts_timestamp_t_to_vtss_ts_timestamp_t(const mesa_ts_timestamp_t *in, vtss_ts_timestamp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ts_timestamp_t *tmp = (const vtss_ts_timestamp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_vtss_ts_timestamp_alloc_cb_t_to_mesa_ts_timestamp_alloc_cb_t(const vtss_ts_timestamp_alloc_cb_t *in, mesa_ts_timestamp_alloc_cb_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ts_timestamp_alloc_cb_t *tmp = (const mesa_ts_timestamp_alloc_cb_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
inline static mesa_rc mesa_conv_mesa_ts_timestamp_alloc_cb_t_to_vtss_ts_timestamp_alloc_cb_t(const mesa_ts_timestamp_alloc_cb_t *in, vtss_ts_timestamp_alloc_cb_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ts_timestamp_alloc_cb_t *tmp = (const vtss_ts_timestamp_alloc_cb_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_conv_vtss_ts_timestamp_alloc_t_to_mesa_ts_timestamp_alloc_t(const vtss_ts_timestamp_alloc_t *in, mesa_ts_timestamp_alloc_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_conv_mesa_ts_timestamp_alloc_t_to_vtss_ts_timestamp_alloc_t(const mesa_ts_timestamp_alloc_t *in, vtss_ts_timestamp_alloc_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined (VTSS_FEATURE_DELAY_REQ_AUTO_RESP)
inline static mesa_rc mesa_conv_vtss_ts_autoresp_dom_cfg_t_to_mesa_ts_autoresp_dom_cfg_t(const vtss_ts_autoresp_dom_cfg_t *in, mesa_ts_autoresp_dom_cfg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ts_autoresp_dom_cfg_t *tmp = (const mesa_ts_autoresp_dom_cfg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined (VTSS_FEATURE_DELAY_REQ_AUTO_RESP)
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined (VTSS_FEATURE_DELAY_REQ_AUTO_RESP)
inline static mesa_rc mesa_conv_mesa_ts_autoresp_dom_cfg_t_to_vtss_ts_autoresp_dom_cfg_t(const mesa_ts_autoresp_dom_cfg_t *in, vtss_ts_autoresp_dom_cfg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ts_autoresp_dom_cfg_t *tmp = (const vtss_ts_autoresp_dom_cfg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined (VTSS_FEATURE_DELAY_REQ_AUTO_RESP)
#endif  // defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_FEATURE_FRER)
inline static mesa_rc mesa_conv_vtss_frer_recovery_alg_t_to_mesa_frer_recovery_alg_t(const vtss_frer_recovery_alg_t *in, mesa_frer_recovery_alg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_frer_recovery_alg_t *tmp = (const mesa_frer_recovery_alg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FRER)
#if defined(VTSS_FEATURE_FRER)
inline static mesa_rc mesa_conv_mesa_frer_recovery_alg_t_to_vtss_frer_recovery_alg_t(const mesa_frer_recovery_alg_t *in, vtss_frer_recovery_alg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_frer_recovery_alg_t *tmp = (const vtss_frer_recovery_alg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FRER)
#if defined(VTSS_FEATURE_FRER)
inline static mesa_rc mesa_conv_vtss_frer_stream_conf_t_to_mesa_frer_stream_conf_t(const vtss_frer_stream_conf_t *in, mesa_frer_stream_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_frer_stream_conf_t *tmp = (const mesa_frer_stream_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FRER)
#if defined(VTSS_FEATURE_FRER)
inline static mesa_rc mesa_conv_mesa_frer_stream_conf_t_to_vtss_frer_stream_conf_t(const mesa_frer_stream_conf_t *in, vtss_frer_stream_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_frer_stream_conf_t *tmp = (const vtss_frer_stream_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FRER)
#if defined(VTSS_FEATURE_FRER)
inline static mesa_rc mesa_conv_vtss_frer_counters_t_to_mesa_frer_counters_t(const vtss_frer_counters_t *in, mesa_frer_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_frer_counters_t *tmp = (const mesa_frer_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FRER)
#if defined(VTSS_FEATURE_FRER)
inline static mesa_rc mesa_conv_mesa_frer_counters_t_to_vtss_frer_counters_t(const mesa_frer_counters_t *in, vtss_frer_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_frer_counters_t *tmp = (const vtss_frer_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FRER)
#if defined(VTSS_FEATURE_FRER)
mesa_rc mesa_conv_vtss_frer_iflow_conf_t_to_mesa_frer_iflow_conf_t(const vtss_frer_iflow_conf_t *in, mesa_frer_iflow_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_FRER)
#if defined(VTSS_FEATURE_FRER)
mesa_rc mesa_conv_mesa_frer_iflow_conf_t_to_vtss_frer_iflow_conf_t(const mesa_frer_iflow_conf_t *in, vtss_frer_iflow_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_FRER)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_vtss_psfp_gce_t_to_mesa_psfp_gce_t(const vtss_psfp_gce_t *in, mesa_psfp_gce_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_psfp_gce_t *tmp = (const mesa_psfp_gce_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_mesa_psfp_gce_t_to_vtss_psfp_gce_t(const mesa_psfp_gce_t *in, vtss_psfp_gce_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_psfp_gce_t *tmp = (const vtss_psfp_gce_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_vtss_psfp_gcl_conf_t_to_mesa_psfp_gcl_conf_t(const vtss_psfp_gcl_conf_t *in, mesa_psfp_gcl_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_psfp_gcl_conf_t *tmp = (const mesa_psfp_gcl_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_mesa_psfp_gcl_conf_t_to_vtss_psfp_gcl_conf_t(const mesa_psfp_gcl_conf_t *in, vtss_psfp_gcl_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_psfp_gcl_conf_t *tmp = (const vtss_psfp_gcl_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_vtss_psfp_gate_conf_t_to_mesa_psfp_gate_conf_t(const vtss_psfp_gate_conf_t *in, mesa_psfp_gate_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_psfp_gate_conf_t *tmp = (const mesa_psfp_gate_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_mesa_psfp_gate_conf_t_to_vtss_psfp_gate_conf_t(const mesa_psfp_gate_conf_t *in, vtss_psfp_gate_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_psfp_gate_conf_t *tmp = (const vtss_psfp_gate_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_vtss_psfp_gate_status_t_to_mesa_psfp_gate_status_t(const vtss_psfp_gate_status_t *in, mesa_psfp_gate_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_psfp_gate_status_t *tmp = (const mesa_psfp_gate_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_mesa_psfp_gate_status_t_to_vtss_psfp_gate_status_t(const mesa_psfp_gate_status_t *in, vtss_psfp_gate_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_psfp_gate_status_t *tmp = (const vtss_psfp_gate_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_vtss_psfp_filter_conf_t_to_mesa_psfp_filter_conf_t(const vtss_psfp_filter_conf_t *in, mesa_psfp_filter_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_psfp_filter_conf_t *tmp = (const mesa_psfp_filter_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_mesa_psfp_filter_conf_t_to_vtss_psfp_filter_conf_t(const mesa_psfp_filter_conf_t *in, vtss_psfp_filter_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_psfp_filter_conf_t *tmp = (const vtss_psfp_filter_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_vtss_psfp_filter_status_t_to_mesa_psfp_filter_status_t(const vtss_psfp_filter_status_t *in, mesa_psfp_filter_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_psfp_filter_status_t *tmp = (const mesa_psfp_filter_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_mesa_psfp_filter_status_t_to_vtss_psfp_filter_status_t(const mesa_psfp_filter_status_t *in, vtss_psfp_filter_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_psfp_filter_status_t *tmp = (const vtss_psfp_filter_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_vtss_psfp_iflow_conf_t_to_mesa_psfp_iflow_conf_t(const vtss_psfp_iflow_conf_t *in, mesa_psfp_iflow_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_psfp_iflow_conf_t *tmp = (const mesa_psfp_iflow_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
inline static mesa_rc mesa_conv_mesa_psfp_iflow_conf_t_to_vtss_psfp_iflow_conf_t(const mesa_psfp_iflow_conf_t *in, vtss_psfp_iflow_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_psfp_iflow_conf_t *tmp = (const vtss_psfp_iflow_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_QOS_TAS)
inline static mesa_rc mesa_conv_vtss_qos_tas_conf_t_to_mesa_qos_tas_conf_t(const vtss_qos_tas_conf_t *in, mesa_qos_tas_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_tas_conf_t *tmp = (const mesa_qos_tas_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_TAS)
#if defined(VTSS_FEATURE_QOS_TAS)
inline static mesa_rc mesa_conv_mesa_qos_tas_conf_t_to_vtss_qos_tas_conf_t(const mesa_qos_tas_conf_t *in, vtss_qos_tas_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_tas_conf_t *tmp = (const vtss_qos_tas_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_TAS)
#if defined(VTSS_FEATURE_QOS_TAS)
inline static mesa_rc mesa_conv_vtss_qos_tas_gco_t_to_mesa_qos_tas_gco_t(const vtss_qos_tas_gco_t *in, mesa_qos_tas_gco_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_tas_gco_t *tmp = (const mesa_qos_tas_gco_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_TAS)
#if defined(VTSS_FEATURE_QOS_TAS)
inline static mesa_rc mesa_conv_mesa_qos_tas_gco_t_to_vtss_qos_tas_gco_t(const mesa_qos_tas_gco_t *in, vtss_qos_tas_gco_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_tas_gco_t *tmp = (const vtss_qos_tas_gco_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_TAS)
#if defined(VTSS_FEATURE_QOS_TAS)
inline static mesa_rc mesa_conv_vtss_qos_tas_gce_t_to_mesa_qos_tas_gce_t(const vtss_qos_tas_gce_t *in, mesa_qos_tas_gce_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_tas_gce_t *tmp = (const mesa_qos_tas_gce_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_TAS)
#if defined(VTSS_FEATURE_QOS_TAS)
inline static mesa_rc mesa_conv_mesa_qos_tas_gce_t_to_vtss_qos_tas_gce_t(const mesa_qos_tas_gce_t *in, vtss_qos_tas_gce_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_tas_gce_t *tmp = (const vtss_qos_tas_gce_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_TAS)
#if defined(VTSS_FEATURE_QOS_TAS)
mesa_rc mesa_conv_vtss_qos_tas_port_conf_t_to_mesa_qos_tas_port_conf_t(const vtss_qos_tas_port_conf_t *in, mesa_qos_tas_port_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS_TAS)
#if defined(VTSS_FEATURE_QOS_TAS)
mesa_rc mesa_conv_mesa_qos_tas_port_conf_t_to_vtss_qos_tas_port_conf_t(const mesa_qos_tas_port_conf_t *in, vtss_qos_tas_port_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS_TAS)
#if defined(VTSS_FEATURE_QOS_TAS)
inline static mesa_rc mesa_conv_vtss_qos_tas_port_status_t_to_mesa_qos_tas_port_status_t(const vtss_qos_tas_port_status_t *in, mesa_qos_tas_port_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_tas_port_status_t *tmp = (const mesa_qos_tas_port_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_TAS)
#if defined(VTSS_FEATURE_QOS_TAS)
inline static mesa_rc mesa_conv_mesa_qos_tas_port_status_t_to_vtss_qos_tas_port_status_t(const mesa_qos_tas_port_status_t *in, vtss_qos_tas_port_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_tas_port_status_t *tmp = (const vtss_qos_tas_port_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_TAS)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
inline static mesa_rc mesa_conv_vtss_qos_fp_port_conf_t_to_mesa_qos_fp_port_conf_t(const vtss_qos_fp_port_conf_t *in, mesa_qos_fp_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_fp_port_conf_t *tmp = (const mesa_qos_fp_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
inline static mesa_rc mesa_conv_mesa_qos_fp_port_conf_t_to_vtss_qos_fp_port_conf_t(const mesa_qos_fp_port_conf_t *in, vtss_qos_fp_port_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_fp_port_conf_t *tmp = (const vtss_qos_fp_port_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
inline static mesa_rc mesa_conv_vtss_mm_status_verify_t_to_mesa_mm_status_verify_t(const vtss_mm_status_verify_t *in, mesa_mm_status_verify_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mm_status_verify_t *tmp = (const mesa_mm_status_verify_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
inline static mesa_rc mesa_conv_mesa_mm_status_verify_t_to_vtss_mm_status_verify_t(const mesa_mm_status_verify_t *in, vtss_mm_status_verify_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mm_status_verify_t *tmp = (const vtss_mm_status_verify_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
inline static mesa_rc mesa_conv_vtss_qos_fp_port_status_t_to_mesa_qos_fp_port_status_t(const vtss_qos_fp_port_status_t *in, mesa_qos_fp_port_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_qos_fp_port_status_t *tmp = (const mesa_qos_fp_port_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
inline static mesa_rc mesa_conv_mesa_qos_fp_port_status_t_to_vtss_qos_fp_port_status_t(const mesa_qos_fp_port_status_t *in, vtss_qos_fp_port_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_qos_fp_port_status_t *tmp = (const vtss_qos_fp_port_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
inline static mesa_rc mesa_conv_vtss_storm_policer_mode_t_to_mesa_storm_policer_mode_t(const vtss_storm_policer_mode_t *in, mesa_storm_policer_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_storm_policer_mode_t *tmp = (const mesa_storm_policer_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_storm_policer_mode_t_to_vtss_storm_policer_mode_t(const mesa_storm_policer_mode_t *in, vtss_storm_policer_mode_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_storm_policer_mode_t *tmp = (const vtss_storm_policer_mode_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_policer_type_t_to_mesa_policer_type_t(const vtss_policer_type_t *in, mesa_policer_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_policer_type_t *tmp = (const mesa_policer_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_policer_type_t_to_vtss_policer_type_t(const mesa_policer_type_t *in, vtss_policer_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_policer_type_t *tmp = (const vtss_policer_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_opt_bool_t_to_mesa_opt_bool_t(const vtss_opt_bool_t *in, mesa_opt_bool_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_opt_bool_t *tmp = (const mesa_opt_bool_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_opt_bool_t_to_vtss_opt_bool_t(const mesa_opt_bool_t *in, vtss_opt_bool_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_opt_bool_t *tmp = (const vtss_opt_bool_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_opt_prio_t_to_mesa_opt_prio_t(const vtss_opt_prio_t *in, mesa_opt_prio_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_opt_prio_t *tmp = (const mesa_opt_prio_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_opt_prio_t_to_vtss_opt_prio_t(const mesa_opt_prio_t *in, vtss_opt_prio_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_opt_prio_t *tmp = (const vtss_opt_prio_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vlan_frame_t_to_mesa_vlan_frame_t(const vtss_vlan_frame_t *in, mesa_vlan_frame_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vlan_frame_t *tmp = (const mesa_vlan_frame_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vlan_frame_t_to_vtss_vlan_frame_t(const mesa_vlan_frame_t *in, vtss_vlan_frame_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vlan_frame_t *tmp = (const vtss_vlan_frame_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vlan_tag_t_to_mesa_vlan_tag_t(const vtss_vlan_tag_t *in, mesa_vlan_tag_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vlan_tag_t *tmp = (const mesa_vlan_tag_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vlan_tag_t_to_vtss_vlan_tag_t(const mesa_vlan_tag_t *in, vtss_vlan_tag_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vlan_tag_t *tmp = (const vtss_vlan_tag_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vid_mac_t_to_mesa_vid_mac_t(const vtss_vid_mac_t *in, mesa_vid_mac_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vid_mac_t *tmp = (const mesa_vid_mac_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vid_mac_t_to_vtss_vid_mac_t(const mesa_vid_mac_t *in, vtss_vid_mac_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vid_mac_t *tmp = (const vtss_vid_mac_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
mesa_rc mesa_conv_vtss_packet_reg_type_t_to_mesa_packet_reg_type_t(const vtss_packet_reg_type_t *in, mesa_packet_reg_type_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_packet_reg_type_t_to_vtss_packet_reg_type_t(const mesa_packet_reg_type_t *in, vtss_packet_reg_type_t *out); /* ag.rb:1595 */
#if defined(VTSS_FEATURE_PACKET_PORT_L2CP_REG)
inline static mesa_rc mesa_conv_vtss_packet_rx_port_l2cp_conf_t_to_mesa_packet_rx_port_l2cp_conf_t(const vtss_packet_rx_port_l2cp_conf_t *in, mesa_packet_rx_port_l2cp_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_packet_rx_port_l2cp_conf_t *tmp = (const mesa_packet_rx_port_l2cp_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET_PORT_L2CP_REG)
#if defined(VTSS_FEATURE_PACKET_PORT_L2CP_REG)
inline static mesa_rc mesa_conv_mesa_packet_rx_port_l2cp_conf_t_to_vtss_packet_rx_port_l2cp_conf_t(const mesa_packet_rx_port_l2cp_conf_t *in, vtss_packet_rx_port_l2cp_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_packet_rx_port_l2cp_conf_t *tmp = (const vtss_packet_rx_port_l2cp_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET_PORT_L2CP_REG)
mesa_rc mesa_conv_vtss_packet_rx_port_conf_t_to_mesa_packet_rx_port_conf_t(const vtss_packet_rx_port_conf_t *in, mesa_packet_rx_port_conf_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_packet_rx_port_conf_t_to_vtss_packet_rx_port_conf_t(const mesa_packet_rx_port_conf_t *in, vtss_packet_rx_port_conf_t *out); /* ag.rb:1595 */
inline static mesa_rc mesa_conv_vtss_ipv6_t_to_mesa_ipv6_t(const vtss_ipv6_t *in, mesa_ipv6_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ipv6_t *tmp = (const mesa_ipv6_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_ipv6_t_to_vtss_ipv6_t(const mesa_ipv6_t *in, vtss_ipv6_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ipv6_t *tmp = (const vtss_ipv6_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_ip_type_t_to_mesa_ip_type_t(const vtss_ip_type_t *in, mesa_ip_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ip_type_t *tmp = (const mesa_ip_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_ip_type_t_to_vtss_ip_type_t(const mesa_ip_type_t *in, vtss_ip_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ip_type_t *tmp = (const vtss_ip_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_ip_addr_t_to_mesa_ip_addr_t(const vtss_ip_addr_t *in, mesa_ip_addr_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ip_addr_t *tmp = (const mesa_ip_addr_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_ip_addr_t_to_vtss_ip_addr_t(const mesa_ip_addr_t *in, vtss_ip_addr_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ip_addr_t *tmp = (const vtss_ip_addr_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_ipv4_network_t_to_mesa_ipv4_network_t(const vtss_ipv4_network_t *in, mesa_ipv4_network_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ipv4_network_t *tmp = (const mesa_ipv4_network_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_ipv4_network_t_to_vtss_ipv4_network_t(const mesa_ipv4_network_t *in, vtss_ipv4_network_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ipv4_network_t *tmp = (const vtss_ipv4_network_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_ipv6_network_t_to_mesa_ipv6_network_t(const vtss_ipv6_network_t *in, mesa_ipv6_network_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ipv6_network_t *tmp = (const mesa_ipv6_network_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_ipv6_network_t_to_vtss_ipv6_network_t(const mesa_ipv6_network_t *in, vtss_ipv6_network_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ipv6_network_t *tmp = (const vtss_ipv6_network_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_ip_network_t_to_mesa_ip_network_t(const vtss_ip_network_t *in, mesa_ip_network_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ip_network_t *tmp = (const mesa_ip_network_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_ip_network_t_to_vtss_ip_network_t(const mesa_ip_network_t *in, vtss_ip_network_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ip_network_t *tmp = (const vtss_ip_network_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_routing_entry_type_t_to_mesa_routing_entry_type_t(const vtss_routing_entry_type_t *in, mesa_routing_entry_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_routing_entry_type_t *tmp = (const mesa_routing_entry_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_routing_entry_type_t_to_vtss_routing_entry_type_t(const mesa_routing_entry_type_t *in, vtss_routing_entry_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_routing_entry_type_t *tmp = (const vtss_routing_entry_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_ipv4_uc_t_to_mesa_ipv4_uc_t(const vtss_ipv4_uc_t *in, mesa_ipv4_uc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ipv4_uc_t *tmp = (const mesa_ipv4_uc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_ipv4_uc_t_to_vtss_ipv4_uc_t(const mesa_ipv4_uc_t *in, vtss_ipv4_uc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ipv4_uc_t *tmp = (const vtss_ipv4_uc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_ipv6_uc_t_to_mesa_ipv6_uc_t(const vtss_ipv6_uc_t *in, mesa_ipv6_uc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ipv6_uc_t *tmp = (const mesa_ipv6_uc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_ipv6_uc_t_to_vtss_ipv6_uc_t(const mesa_ipv6_uc_t *in, vtss_ipv6_uc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ipv6_uc_t *tmp = (const vtss_ipv6_uc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_ipv4_mc_t_to_mesa_ipv4_mc_t(const vtss_ipv4_mc_t *in, mesa_ipv4_mc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ipv4_mc_t *tmp = (const mesa_ipv4_mc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_ipv4_mc_t_to_vtss_ipv4_mc_t(const mesa_ipv4_mc_t *in, vtss_ipv4_mc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ipv4_mc_t *tmp = (const vtss_ipv4_mc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_ipv6_mc_t_to_mesa_ipv6_mc_t(const vtss_ipv6_mc_t *in, mesa_ipv6_mc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_ipv6_mc_t *tmp = (const mesa_ipv6_mc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_ipv6_mc_t_to_vtss_ipv6_mc_t(const mesa_ipv6_mc_t *in, vtss_ipv6_mc_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_ipv6_mc_t *tmp = (const vtss_ipv6_mc_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_routing_entry_t_to_mesa_routing_entry_t(const vtss_routing_entry_t *in, mesa_routing_entry_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_routing_entry_t *tmp = (const mesa_routing_entry_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_routing_entry_t_to_vtss_routing_entry_t(const mesa_routing_entry_t *in, vtss_routing_entry_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_routing_entry_t *tmp = (const vtss_routing_entry_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_routing_mc_entry_t_to_mesa_routing_mc_entry_t(const vtss_routing_mc_entry_t *in, mesa_routing_mc_entry_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_routing_mc_entry_t *tmp = (const mesa_routing_mc_entry_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_routing_mc_entry_t_to_vtss_routing_mc_entry_t(const mesa_routing_mc_entry_t *in, vtss_routing_mc_entry_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_routing_mc_entry_t *tmp = (const vtss_routing_mc_entry_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_l3_counters_t_to_mesa_l3_counters_t(const vtss_l3_counters_t *in, mesa_l3_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_l3_counters_t *tmp = (const mesa_l3_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_l3_counters_t_to_vtss_l3_counters_t(const mesa_l3_counters_t *in, vtss_l3_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_l3_counters_t *tmp = (const vtss_l3_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vcap_bit_t_to_mesa_vcap_bit_t(const vtss_vcap_bit_t *in, mesa_vcap_bit_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcap_bit_t *tmp = (const mesa_vcap_bit_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vcap_bit_t_to_vtss_vcap_bit_t(const mesa_vcap_bit_t *in, vtss_vcap_bit_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcap_bit_t *tmp = (const vtss_vcap_bit_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vcap_u24_t_to_mesa_vcap_u24_t(const vtss_vcap_u24_t *in, mesa_vcap_u24_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcap_u24_t *tmp = (const mesa_vcap_u24_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vcap_u24_t_to_vtss_vcap_u24_t(const mesa_vcap_u24_t *in, vtss_vcap_u24_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcap_u24_t *tmp = (const vtss_vcap_u24_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vcap_vr_type_t_to_mesa_vcap_vr_type_t(const vtss_vcap_vr_type_t *in, mesa_vcap_vr_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcap_vr_type_t *tmp = (const mesa_vcap_vr_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vcap_vr_type_t_to_vtss_vcap_vr_type_t(const mesa_vcap_vr_type_t *in, vtss_vcap_vr_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcap_vr_type_t *tmp = (const vtss_vcap_vr_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vcap_vr_t_to_mesa_vcap_vr_t(const vtss_vcap_vr_t *in, mesa_vcap_vr_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcap_vr_t *tmp = (const mesa_vcap_vr_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vcap_vr_t_to_vtss_vcap_vr_t(const mesa_vcap_vr_t *in, vtss_vcap_vr_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcap_vr_t *tmp = (const vtss_vcap_vr_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_vcap_key_type_t_to_mesa_vcap_key_type_t(const vtss_vcap_key_type_t *in, mesa_vcap_key_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vcap_key_type_t *tmp = (const mesa_vcap_key_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vcap_key_type_t_to_vtss_vcap_key_type_t(const mesa_vcap_key_type_t *in, vtss_vcap_key_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vcap_key_type_t *tmp = (const vtss_vcap_key_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_counter_pair_t_to_mesa_counter_pair_t(const vtss_counter_pair_t *in, mesa_counter_pair_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_counter_pair_t *tmp = (const mesa_counter_pair_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_counter_pair_t_to_vtss_counter_pair_t(const mesa_counter_pair_t *in, vtss_counter_pair_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_counter_pair_t *tmp = (const vtss_counter_pair_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_evc_counters_t_to_mesa_evc_counters_t(const vtss_evc_counters_t *in, mesa_evc_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_evc_counters_t *tmp = (const mesa_evc_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_evc_counters_t_to_vtss_evc_counters_t(const mesa_evc_counters_t *in, vtss_evc_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_evc_counters_t *tmp = (const vtss_evc_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_timestamp_t_to_mesa_timestamp_t(const vtss_timestamp_t *in, mesa_timestamp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_timestamp_t *tmp = (const mesa_timestamp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_timestamp_t_to_vtss_timestamp_t(const mesa_timestamp_t *in, vtss_timestamp_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_timestamp_t *tmp = (const vtss_timestamp_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_scaled_ns_t_to_mesa_scaled_ns_t(const vtss_scaled_ns_t *in, mesa_scaled_ns_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_scaled_ns_t *tmp = (const mesa_scaled_ns_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_scaled_ns_t_to_vtss_scaled_ns_t(const mesa_scaled_ns_t *in, vtss_scaled_ns_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_scaled_ns_t *tmp = (const vtss_scaled_ns_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_uscaled_ns_t_to_mesa_uscaled_ns_t(const vtss_uscaled_ns_t *in, mesa_uscaled_ns_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_uscaled_ns_t *tmp = (const mesa_uscaled_ns_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_uscaled_ns_t_to_vtss_uscaled_ns_t(const mesa_uscaled_ns_t *in, vtss_uscaled_ns_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_uscaled_ns_t *tmp = (const vtss_uscaled_ns_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
mesa_rc mesa_conv_vtss_miim_controller_t_to_mesa_miim_controller_t(const vtss_miim_controller_t *in, mesa_miim_controller_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_miim_controller_t_to_vtss_miim_controller_t(const mesa_miim_controller_t *in, vtss_miim_controller_t *out); /* ag.rb:1595 */
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_vop_conf_t_to_mesa_vop_conf_t(const vtss_vop_conf_t *in, mesa_vop_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_mesa_vop_conf_t_to_vtss_vop_conf_t(const mesa_vop_conf_t *in, vtss_vop_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
inline static mesa_rc mesa_conv_vtss_voe_type_t_to_mesa_voe_type_t(const vtss_voe_type_t *in, mesa_voe_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_voe_type_t *tmp = (const mesa_voe_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
inline static mesa_rc mesa_conv_mesa_voe_type_t_to_vtss_voe_type_t(const mesa_voe_type_t *in, vtss_voe_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_voe_type_t *tmp = (const vtss_voe_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
inline static mesa_rc mesa_conv_vtss_oam_direction_t_to_mesa_oam_direction_t(const vtss_oam_direction_t *in, mesa_oam_direction_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_oam_direction_t *tmp = (const mesa_oam_direction_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
inline static mesa_rc mesa_conv_mesa_oam_direction_t_to_vtss_oam_direction_t(const mesa_oam_direction_t *in, vtss_oam_direction_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_oam_direction_t *tmp = (const vtss_oam_direction_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_voe_allocation_t_to_mesa_voe_allocation_t(const vtss_voe_allocation_t *in, mesa_voe_allocation_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_mesa_voe_allocation_t_to_vtss_voe_allocation_t(const mesa_voe_allocation_t *in, vtss_voe_allocation_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
inline static mesa_rc mesa_conv_vtss_voe_dmac_check_t_to_mesa_voe_dmac_check_t(const vtss_voe_dmac_check_t *in, mesa_voe_dmac_check_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_voe_dmac_check_t *tmp = (const mesa_voe_dmac_check_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
inline static mesa_rc mesa_conv_mesa_voe_dmac_check_t_to_vtss_voe_dmac_check_t(const mesa_voe_dmac_check_t *in, vtss_voe_dmac_check_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_voe_dmac_check_t *tmp = (const vtss_voe_dmac_check_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_voe_conf_t_to_mesa_voe_conf_t(const vtss_voe_conf_t *in, mesa_voe_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_mesa_voe_conf_t_to_vtss_voe_conf_t(const mesa_voe_conf_t *in, vtss_voe_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_voe_ccm_period_t_to_mesa_voe_ccm_period_t(const vtss_voe_ccm_period_t *in, mesa_voe_ccm_period_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_mesa_voe_ccm_period_t_to_vtss_voe_ccm_period_t(const mesa_voe_ccm_period_t *in, vtss_voe_ccm_period_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_oam_cpu_copy_t_to_mesa_oam_cpu_copy_t(const vtss_oam_cpu_copy_t *in, mesa_oam_cpu_copy_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_mesa_oam_cpu_copy_t_to_vtss_oam_cpu_copy_t(const mesa_oam_cpu_copy_t *in, vtss_oam_cpu_copy_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_voe_cc_conf_t_to_mesa_voe_cc_conf_t(const vtss_voe_cc_conf_t *in, mesa_voe_cc_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_mesa_voe_cc_conf_t_to_vtss_voe_cc_conf_t(const mesa_voe_cc_conf_t *in, vtss_voe_cc_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
inline static mesa_rc mesa_conv_vtss_voe_lt_conf_t_to_mesa_voe_lt_conf_t(const vtss_voe_lt_conf_t *in, mesa_voe_lt_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_voe_lt_conf_t *tmp = (const mesa_voe_lt_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
inline static mesa_rc mesa_conv_mesa_voe_lt_conf_t_to_vtss_voe_lt_conf_t(const mesa_voe_lt_conf_t *in, vtss_voe_lt_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_voe_lt_conf_t *tmp = (const vtss_voe_lt_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_conv_vtss_voe_lb_conf_t_to_mesa_voe_lb_conf_t(const vtss_voe_lb_conf_t *in, mesa_voe_lb_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_conv_mesa_voe_lb_conf_t_to_vtss_voe_lb_conf_t(const mesa_voe_lb_conf_t *in, vtss_voe_lb_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
inline static mesa_rc mesa_conv_vtss_voe_laps_conf_t_to_mesa_voe_laps_conf_t(const vtss_voe_laps_conf_t *in, mesa_voe_laps_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_voe_laps_conf_t *tmp = (const mesa_voe_laps_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
inline static mesa_rc mesa_conv_mesa_voe_laps_conf_t_to_vtss_voe_laps_conf_t(const mesa_voe_laps_conf_t *in, vtss_voe_laps_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_voe_laps_conf_t *tmp = (const vtss_voe_laps_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_voe_status_t_to_mesa_voe_status_t(const vtss_voe_status_t *in, mesa_voe_status_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_mesa_voe_status_t_to_vtss_voe_status_t(const mesa_voe_status_t *in, vtss_voe_status_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_voe_counters_t_to_mesa_voe_counters_t(const vtss_voe_counters_t *in, mesa_voe_counters_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_mesa_voe_counters_t_to_vtss_voe_counters_t(const mesa_voe_counters_t *in, vtss_voe_counters_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_voe_cc_status_t_to_mesa_voe_cc_status_t(const vtss_voe_cc_status_t *in, mesa_voe_cc_status_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_mesa_voe_cc_status_t_to_vtss_voe_cc_status_t(const mesa_voe_cc_status_t *in, vtss_voe_cc_status_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
inline static mesa_rc mesa_conv_vtss_voe_cc_counters_t_to_mesa_voe_cc_counters_t(const vtss_voe_cc_counters_t *in, mesa_voe_cc_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_voe_cc_counters_t *tmp = (const mesa_voe_cc_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
inline static mesa_rc mesa_conv_mesa_voe_cc_counters_t_to_vtss_voe_cc_counters_t(const mesa_voe_cc_counters_t *in, vtss_voe_cc_counters_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_voe_cc_counters_t *tmp = (const vtss_voe_cc_counters_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
inline static mesa_rc mesa_conv_vtss_voe_lt_status_t_to_mesa_voe_lt_status_t(const vtss_voe_lt_status_t *in, mesa_voe_lt_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_voe_lt_status_t *tmp = (const mesa_voe_lt_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
inline static mesa_rc mesa_conv_mesa_voe_lt_status_t_to_vtss_voe_lt_status_t(const mesa_voe_lt_status_t *in, vtss_voe_lt_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_voe_lt_status_t *tmp = (const vtss_voe_lt_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_conv_vtss_voe_lb_status_t_to_mesa_voe_lb_status_t(const vtss_voe_lb_status_t *in, mesa_voe_lb_status_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_conv_mesa_voe_lb_status_t_to_vtss_voe_lb_status_t(const mesa_voe_lb_status_t *in, vtss_voe_lb_status_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_conv_vtss_voe_lb_counters_t_to_mesa_voe_lb_counters_t(const vtss_voe_lb_counters_t *in, mesa_voe_lb_counters_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_conv_mesa_voe_lb_counters_t_to_vtss_voe_lb_counters_t(const mesa_voe_lb_counters_t *in, vtss_voe_lb_counters_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
inline static mesa_rc mesa_conv_vtss_voe_laps_status_t_to_mesa_voe_laps_status_t(const vtss_voe_laps_status_t *in, mesa_voe_laps_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_voe_laps_status_t *tmp = (const mesa_voe_laps_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
inline static mesa_rc mesa_conv_mesa_voe_laps_status_t_to_vtss_voe_laps_status_t(const mesa_voe_laps_status_t *in, vtss_voe_laps_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_voe_laps_status_t *tmp = (const vtss_voe_laps_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_V2)
inline static mesa_rc mesa_conv_vtss_voi_allocation_t_to_mesa_voi_allocation_t(const vtss_voi_allocation_t *in, mesa_voi_allocation_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_voi_allocation_t *tmp = (const mesa_voi_allocation_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_V2)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_V2)
inline static mesa_rc mesa_conv_mesa_voi_allocation_t_to_vtss_voi_allocation_t(const mesa_voi_allocation_t *in, vtss_voi_allocation_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_voi_allocation_t *tmp = (const vtss_voi_allocation_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_V2)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_V2)
inline static mesa_rc mesa_conv_vtss_oam_raps_handling_t_to_mesa_oam_raps_handling_t(const vtss_oam_raps_handling_t *in, mesa_oam_raps_handling_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_oam_raps_handling_t *tmp = (const mesa_oam_raps_handling_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_V2)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_V2)
inline static mesa_rc mesa_conv_mesa_oam_raps_handling_t_to_vtss_oam_raps_handling_t(const mesa_oam_raps_handling_t *in, vtss_oam_raps_handling_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_oam_raps_handling_t *tmp = (const vtss_oam_raps_handling_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_V2)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_V2)
inline static mesa_rc mesa_conv_vtss_voi_conf_t_to_mesa_voi_conf_t(const vtss_voi_conf_t *in, mesa_voi_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_voi_conf_t *tmp = (const mesa_voi_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_V2)
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_V2)
inline static mesa_rc mesa_conv_mesa_voi_conf_t_to_vtss_voi_conf_t(const mesa_voi_conf_t *in, vtss_voi_conf_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_voi_conf_t *tmp = (const vtss_voi_conf_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_V2)
#endif  // defined(VTSS_FEATURE_VOP)
inline static mesa_rc mesa_conv_vtss_port_speed_t_to_mesa_port_speed_t(const vtss_port_speed_t *in, mesa_port_speed_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_speed_t *tmp = (const mesa_port_speed_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_port_speed_t_to_vtss_port_speed_t(const mesa_port_speed_t *in, vtss_port_speed_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_speed_t *tmp = (const vtss_port_speed_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_fiber_port_speed_t_to_mesa_fiber_port_speed_t(const vtss_fiber_port_speed_t *in, mesa_fiber_port_speed_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_fiber_port_speed_t *tmp = (const mesa_fiber_port_speed_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_fiber_port_speed_t_to_vtss_fiber_port_speed_t(const mesa_fiber_port_speed_t *in, vtss_fiber_port_speed_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_fiber_port_speed_t *tmp = (const vtss_fiber_port_speed_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_sd10g_media_type_t_to_mesa_sd10g_media_type_t(const vtss_sd10g_media_type_t *in, mesa_sd10g_media_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_sd10g_media_type_t *tmp = (const mesa_sd10g_media_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_sd10g_media_type_t_to_vtss_sd10g_media_type_t(const mesa_sd10g_media_type_t *in, vtss_sd10g_media_type_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_sd10g_media_type_t *tmp = (const vtss_sd10g_media_type_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
mesa_rc mesa_conv_vtss_phy_power_mode_t_to_mesa_phy_power_mode_t(const vtss_phy_power_mode_t *in, mesa_phy_power_mode_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_phy_power_mode_t_to_vtss_phy_power_mode_t(const mesa_phy_power_mode_t *in, vtss_phy_power_mode_t *out); /* ag.rb:1595 */
inline static mesa_rc mesa_conv_vtss_phy_veriphy_status_t_to_mesa_phy_veriphy_status_t(const vtss_phy_veriphy_status_t *in, mesa_phy_veriphy_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_phy_veriphy_status_t *tmp = (const mesa_phy_veriphy_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_phy_veriphy_status_t_to_vtss_phy_veriphy_status_t(const mesa_phy_veriphy_status_t *in, vtss_phy_veriphy_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_phy_veriphy_status_t *tmp = (const vtss_phy_veriphy_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
mesa_rc mesa_conv_vtss_port_interface_t_to_mesa_port_interface_t(const vtss_port_interface_t *in, mesa_port_interface_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_port_interface_t_to_vtss_port_interface_t(const mesa_port_interface_t *in, vtss_port_interface_t *out); /* ag.rb:1595 */
inline static mesa_rc mesa_conv_vtss_aneg_t_to_mesa_aneg_t(const vtss_aneg_t *in, mesa_aneg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_aneg_t *tmp = (const mesa_aneg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_aneg_t_to_vtss_aneg_t(const mesa_aneg_t *in, vtss_aneg_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_aneg_t *tmp = (const vtss_aneg_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_mac_t_to_mesa_mac_t(const vtss_mac_t *in, mesa_mac_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_mac_t *tmp = (const mesa_mac_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_mac_t_to_vtss_mac_t(const mesa_mac_t *in, vtss_mac_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_mac_t *tmp = (const vtss_mac_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_vtss_port_status_t_to_mesa_port_status_t(const vtss_port_status_t *in, mesa_port_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_port_status_t *tmp = (const mesa_port_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_port_status_t_to_vtss_port_status_t(const mesa_port_status_t *in, vtss_port_status_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_port_status_t *tmp = (const vtss_port_status_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
mesa_rc mesa_conv_vtss_serdes_mode_t_to_mesa_serdes_mode_t(const vtss_serdes_mode_t *in, mesa_serdes_mode_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_serdes_mode_t_to_vtss_serdes_mode_t(const mesa_serdes_mode_t *in, vtss_serdes_mode_t *out); /* ag.rb:1595 */
inline static mesa_rc mesa_conv_vtss_vdd_t_to_mesa_vdd_t(const vtss_vdd_t *in, mesa_vdd_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const mesa_vdd_t *tmp = (const mesa_vdd_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
inline static mesa_rc mesa_conv_mesa_vdd_t_to_vtss_vdd_t(const mesa_vdd_t *in, vtss_vdd_t *out) { /* ag.rb:1585 */
#ifdef __cplusplus
    static_assert(sizeof(out) == sizeof(in), "size check");
#endif // __cplusplus
    const vtss_vdd_t *tmp = (const vtss_vdd_t *)in;
    *out = *tmp;
    return VTSS_RC_OK;
}
mesa_rc mesa_conv_uint8_t_to_mesa_port_list_t(const BOOL in[VTSS_PORT_ARRAY_SIZE], mesa_port_list_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_port_list_t_to_uint8_t(const mesa_port_list_t *in, BOOL out[VTSS_PORT_ARRAY_SIZE]); /* ag.rb:1595 */
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv2_vtss_vce_key_t_to_mesa_vce_key_t(const vtss_vce_key_t *in, mesa_vce_key_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv2_mesa_vce_key_t_to_vtss_vce_key_t(const mesa_vce_key_t *in, vtss_vce_key_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv2_vtss_vlan_trans_port2grp_conf_t_to_mesa_vlan_trans_port2grp_conf_t(const vtss_vlan_trans_port2grp_conf_t *in, mesa_vlan_trans_port2grp_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv2_mesa_vlan_trans_port2grp_conf_t_to_vtss_vlan_trans_port2grp_conf_t(const mesa_vlan_trans_port2grp_conf_t *in, vtss_vlan_trans_port2grp_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_conv_uint8_t_to_mesa_port_list_t(const BOOL in[VTSS_PORTS], mesa_port_list_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv_mesa_port_list_t_to_uint8_t(const mesa_port_list_t *in, BOOL out[VTSS_PORTS]); /* ag.rb:1595 */
#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_conv2_vtss_packet_rx_queue_conf_t_to_mesa_packet_rx_queue_conf_t(const vtss_packet_rx_queue_conf_t *in, mesa_packet_rx_queue_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PACKET)
#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_conv2_mesa_packet_rx_queue_conf_t_to_vtss_packet_rx_queue_conf_t(const mesa_packet_rx_queue_conf_t *in, vtss_packet_rx_queue_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_conv2_vtss_port_counters_t_to_mesa_port_counters_t(const vtss_port_counters_t *in, mesa_port_counters_t *out); /* ag.rb:1595 */
mesa_rc mesa_conv2_mesa_port_counters_t_to_vtss_port_counters_t(const mesa_port_counters_t *in, vtss_port_counters_t *out); /* ag.rb:1595 */
#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv2_vtss_qos_conf_t_to_mesa_qos_conf_t(const vtss_qos_conf_t *in, mesa_qos_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv2_vtss_qos_port_conf_t_to_mesa_qos_port_conf_t(const vtss_qos_port_conf_t *in, mesa_qos_port_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv2_mesa_qos_port_conf_t_to_vtss_qos_port_conf_t(const mesa_qos_port_conf_t *in, vtss_qos_port_conf_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv2_vtss_qce_key_t_to_mesa_qce_key_t(const vtss_qce_key_t *in, mesa_qce_key_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv2_mesa_qce_key_t_to_vtss_qce_key_t(const mesa_qce_key_t *in, vtss_qce_key_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv2_vtss_ace_t_to_mesa_ace_t(const vtss_ace_t *in, mesa_ace_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_ACL)
#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv2_mesa_ace_t_to_vtss_ace_t(const mesa_ace_t *in, vtss_ace_t *out); /* ag.rb:1595 */
#endif  // defined(VTSS_FEATURE_ACL)
#ifdef __cplusplus
}
#endif
#endif // __MESA_H__
