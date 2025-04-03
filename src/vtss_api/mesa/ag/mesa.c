#include "mesa.h"

#define __RC(expr) { vtss_rc __rc__ = (expr); if (__rc__ < VTSS_RC_OK) return __rc__; }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wformat-security"
#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V1)
mesa_rc mesa_conv_vtss_afi_frm_dscr_t_to_mesa_afi_frm_dscr_t(const vtss_afi_frm_dscr_t *in, mesa_afi_frm_dscr_t *out) /* ag.rb:1656 */
{
    // skipped: actual_fps@vtss_afi_frm_dscr_t
    out->fps = in->fps;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_afi_frm_dscr_t_to_vtss_afi_frm_dscr_t(const mesa_afi_frm_dscr_t *in, vtss_afi_frm_dscr_t *out) /* ag.rb:1664 */
{
    // skipped: actual_fps@vtss_afi_frm_dscr_t
    out->fps = in->fps;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_AFI_V1)
#endif  // defined(VTSS_FEATURE_AFI_SWC)

#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
mesa_rc mesa_conv_vtss_afi_fast_inj_start_cfg_t_to_mesa_afi_fast_inj_start_cfg_t(const vtss_afi_fast_inj_start_cfg_t *in, mesa_afi_fast_inj_start_cfg_t *out) /* ag.rb:1656 */
{
    // skipped: bps_actual@vtss_afi_fast_inj_start_cfg_t
    out->bps = in->bps;
    out->seq_cnt = in->seq_cnt;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_afi_fast_inj_start_cfg_t_to_vtss_afi_fast_inj_start_cfg_t(const mesa_afi_fast_inj_start_cfg_t *in, vtss_afi_fast_inj_start_cfg_t *out) /* ag.rb:1664 */
{
    // skipped: bps_actual@vtss_afi_fast_inj_start_cfg_t
    out->bps = in->bps;
    out->seq_cnt = in->seq_cnt;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_AFI_V2)
#endif  // defined(VTSS_FEATURE_AFI_SWC)

mesa_rc mesa_conv_vtss_target_type_t_to_mesa_target_type_t(const vtss_target_type_t *in, mesa_target_type_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        // skipped: VTSS_TARGET_SPARX_III_10_UM
        // skipped: VTSS_TARGET_SPARX_III_17_UM
        // skipped: VTSS_TARGET_SPARX_III_25_UM
        // skipped: VTSS_TARGET_SPARX_III_10_01
        case VTSS_TARGET_CU_PHY:
            *out = MESA_TARGET_CU_PHY;
            return VTSS_RC_OK;
        case VTSS_TARGET_10G_PHY:
            *out = MESA_TARGET_10G_PHY;
            return VTSS_RC_OK;
        case VTSS_TARGET_CARACAL_LITE:
            *out = MESA_TARGET_CARACAL_LITE;
            return VTSS_RC_OK;
        case VTSS_TARGET_SPARX_III_10:
            *out = MESA_TARGET_SPARX_III_10;
            return VTSS_RC_OK;
        case VTSS_TARGET_SPARX_III_18:
            *out = MESA_TARGET_SPARX_III_18;
            return VTSS_RC_OK;
        case VTSS_TARGET_SPARX_III_24:
            *out = MESA_TARGET_SPARX_III_24;
            return VTSS_RC_OK;
        case VTSS_TARGET_SPARX_III_26:
            *out = MESA_TARGET_SPARX_III_26;
            return VTSS_RC_OK;
        case VTSS_TARGET_CARACAL_1:
            *out = MESA_TARGET_CARACAL_1;
            return VTSS_RC_OK;
        case VTSS_TARGET_CARACAL_2:
            *out = MESA_TARGET_CARACAL_2;
            return VTSS_RC_OK;
        case VTSS_TARGET_SPARX_IV_52:
            *out = MESA_TARGET_SPARX_IV_52;
            return VTSS_RC_OK;
        case VTSS_TARGET_SPARX_IV_44:
            *out = MESA_TARGET_SPARX_IV_44;
            return VTSS_RC_OK;
        case VTSS_TARGET_SPARX_IV_80:
            *out = MESA_TARGET_SPARX_IV_80;
            return VTSS_RC_OK;
        case VTSS_TARGET_SPARX_IV_90:
            *out = MESA_TARGET_SPARX_IV_90;
            return VTSS_RC_OK;
        case VTSS_TARGET_SERVAL_2:
            *out = MESA_TARGET_SERVAL_2;
            return VTSS_RC_OK;
        case VTSS_TARGET_LYNX_2:
            *out = MESA_TARGET_LYNX_2;
            return VTSS_RC_OK;
        case VTSS_TARGET_JAGUAR_2:
            *out = MESA_TARGET_JAGUAR_2;
            return VTSS_RC_OK;
        case VTSS_TARGET_SERVAL_T:
            *out = MESA_TARGET_SERVAL_T;
            return VTSS_RC_OK;
        case VTSS_TARGET_SERVAL_TP:
            *out = MESA_TARGET_SERVAL_TP;
            return VTSS_RC_OK;
        case VTSS_TARGET_SERVAL_TE:
            *out = MESA_TARGET_SERVAL_TE;
            return VTSS_RC_OK;
        case VTSS_TARGET_SERVAL_TEP:
            *out = MESA_TARGET_SERVAL_TEP;
            return VTSS_RC_OK;
        case VTSS_TARGET_SERVAL_2_LITE:
            *out = MESA_TARGET_SERVAL_2_LITE;
            return VTSS_RC_OK;
        case VTSS_TARGET_SERVAL_TE10:
            *out = MESA_TARGET_SERVAL_TE10;
            return VTSS_RC_OK;
        case VTSS_TARGET_SPARX_IV_34:
            *out = MESA_TARGET_SPARX_IV_34;
            return VTSS_RC_OK;
        case VTSS_TARGET_7511:
            *out = MESA_TARGET_7511;
            return VTSS_RC_OK;
        case VTSS_TARGET_7512:
            *out = MESA_TARGET_7512;
            return VTSS_RC_OK;
        case VTSS_TARGET_7513:
            *out = MESA_TARGET_7513;
            return VTSS_RC_OK;
        case VTSS_TARGET_7514:
            *out = MESA_TARGET_7514;
            return VTSS_RC_OK;
        case VTSS_TARGET_7546:
            *out = MESA_TARGET_7546;
            return VTSS_RC_OK;
        case VTSS_TARGET_7549:
            *out = MESA_TARGET_7549;
            return VTSS_RC_OK;
        case VTSS_TARGET_7552:
            *out = MESA_TARGET_7552;
            return VTSS_RC_OK;
        case VTSS_TARGET_7556:
            *out = MESA_TARGET_7556;
            return VTSS_RC_OK;
        case VTSS_TARGET_7558:
            *out = MESA_TARGET_7558;
            return VTSS_RC_OK;
        case VTSS_TARGET_7546TSN:
            *out = MESA_TARGET_7546TSN;
            return VTSS_RC_OK;
        case VTSS_TARGET_7549TSN:
            *out = MESA_TARGET_7549TSN;
            return VTSS_RC_OK;
        case VTSS_TARGET_7552TSN:
            *out = MESA_TARGET_7552TSN;
            return VTSS_RC_OK;
        case VTSS_TARGET_7556TSN:
            *out = MESA_TARGET_7556TSN;
            return VTSS_RC_OK;
        case VTSS_TARGET_7558TSN:
            *out = MESA_TARGET_7558TSN;
            return VTSS_RC_OK;
        case VTSS_TARGET_LAN9662:
            *out = MESA_TARGET_LAN9662;
            return VTSS_RC_OK;
        case VTSS_TARGET_LAN9668:
            *out = MESA_TARGET_LAN9668;
            return VTSS_RC_OK;
        case VTSS_TARGET_LAN9694:
            *out = MESA_TARGET_LAN9694;
            return VTSS_RC_OK;
        case VTSS_TARGET_LAN9691VAO:
            *out = MESA_TARGET_LAN9691VAO;
            return VTSS_RC_OK;
        case VTSS_TARGET_LAN9694TSN:
            *out = MESA_TARGET_LAN9694TSN;
            return VTSS_RC_OK;
        case VTSS_TARGET_LAN9694RED:
            *out = MESA_TARGET_LAN9694RED;
            return VTSS_RC_OK;
        case VTSS_TARGET_LAN9696:
            *out = MESA_TARGET_LAN9696;
            return VTSS_RC_OK;
        case VTSS_TARGET_LAN9692VAO:
            *out = MESA_TARGET_LAN9692VAO;
            return VTSS_RC_OK;
        case VTSS_TARGET_LAN9696TSN:
            *out = MESA_TARGET_LAN9696TSN;
            return VTSS_RC_OK;
        case VTSS_TARGET_LAN9696RED:
            *out = MESA_TARGET_LAN9696RED;
            return VTSS_RC_OK;
        case VTSS_TARGET_LAN9698:
            *out = MESA_TARGET_LAN9698;
            return VTSS_RC_OK;
        case VTSS_TARGET_LAN9693VAO:
            *out = MESA_TARGET_LAN9693VAO;
            return VTSS_RC_OK;
        case VTSS_TARGET_LAN9698TSN:
            *out = MESA_TARGET_LAN9698TSN;
            return VTSS_RC_OK;
        case VTSS_TARGET_LAN9698RED:
            *out = MESA_TARGET_LAN9698RED;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_target_type_t_to_vtss_target_type_t(const mesa_target_type_t *in, vtss_target_type_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        // skipped: VTSS_TARGET_SPARX_III_10_UM
        // skipped: VTSS_TARGET_SPARX_III_17_UM
        // skipped: VTSS_TARGET_SPARX_III_25_UM
        // skipped: VTSS_TARGET_SPARX_III_10_01
        case MESA_TARGET_CU_PHY:
            *out = VTSS_TARGET_CU_PHY;
            return VTSS_RC_OK;
        case MESA_TARGET_10G_PHY:
            *out = VTSS_TARGET_10G_PHY;
            return VTSS_RC_OK;
        case MESA_TARGET_CARACAL_LITE:
            *out = VTSS_TARGET_CARACAL_LITE;
            return VTSS_RC_OK;
        case MESA_TARGET_SPARX_III_10:
            *out = VTSS_TARGET_SPARX_III_10;
            return VTSS_RC_OK;
        case MESA_TARGET_SPARX_III_18:
            *out = VTSS_TARGET_SPARX_III_18;
            return VTSS_RC_OK;
        case MESA_TARGET_SPARX_III_24:
            *out = VTSS_TARGET_SPARX_III_24;
            return VTSS_RC_OK;
        case MESA_TARGET_SPARX_III_26:
            *out = VTSS_TARGET_SPARX_III_26;
            return VTSS_RC_OK;
        case MESA_TARGET_CARACAL_1:
            *out = VTSS_TARGET_CARACAL_1;
            return VTSS_RC_OK;
        case MESA_TARGET_CARACAL_2:
            *out = VTSS_TARGET_CARACAL_2;
            return VTSS_RC_OK;
        case MESA_TARGET_SPARX_IV_52:
            *out = VTSS_TARGET_SPARX_IV_52;
            return VTSS_RC_OK;
        case MESA_TARGET_SPARX_IV_44:
            *out = VTSS_TARGET_SPARX_IV_44;
            return VTSS_RC_OK;
        case MESA_TARGET_SPARX_IV_80:
            *out = VTSS_TARGET_SPARX_IV_80;
            return VTSS_RC_OK;
        case MESA_TARGET_SPARX_IV_90:
            *out = VTSS_TARGET_SPARX_IV_90;
            return VTSS_RC_OK;
        case MESA_TARGET_SERVAL_2:
            *out = VTSS_TARGET_SERVAL_2;
            return VTSS_RC_OK;
        case MESA_TARGET_LYNX_2:
            *out = VTSS_TARGET_LYNX_2;
            return VTSS_RC_OK;
        case MESA_TARGET_JAGUAR_2:
            *out = VTSS_TARGET_JAGUAR_2;
            return VTSS_RC_OK;
        case MESA_TARGET_SERVAL_T:
            *out = VTSS_TARGET_SERVAL_T;
            return VTSS_RC_OK;
        case MESA_TARGET_SERVAL_TP:
            *out = VTSS_TARGET_SERVAL_TP;
            return VTSS_RC_OK;
        case MESA_TARGET_SERVAL_TE:
            *out = VTSS_TARGET_SERVAL_TE;
            return VTSS_RC_OK;
        case MESA_TARGET_SERVAL_TEP:
            *out = VTSS_TARGET_SERVAL_TEP;
            return VTSS_RC_OK;
        case MESA_TARGET_SERVAL_2_LITE:
            *out = VTSS_TARGET_SERVAL_2_LITE;
            return VTSS_RC_OK;
        case MESA_TARGET_SERVAL_TE10:
            *out = VTSS_TARGET_SERVAL_TE10;
            return VTSS_RC_OK;
        case MESA_TARGET_SPARX_IV_34:
            *out = VTSS_TARGET_SPARX_IV_34;
            return VTSS_RC_OK;
        case MESA_TARGET_7511:
            *out = VTSS_TARGET_7511;
            return VTSS_RC_OK;
        case MESA_TARGET_7512:
            *out = VTSS_TARGET_7512;
            return VTSS_RC_OK;
        case MESA_TARGET_7513:
            *out = VTSS_TARGET_7513;
            return VTSS_RC_OK;
        case MESA_TARGET_7514:
            *out = VTSS_TARGET_7514;
            return VTSS_RC_OK;
        case MESA_TARGET_7546:
            *out = VTSS_TARGET_7546;
            return VTSS_RC_OK;
        case MESA_TARGET_7549:
            *out = VTSS_TARGET_7549;
            return VTSS_RC_OK;
        case MESA_TARGET_7552:
            *out = VTSS_TARGET_7552;
            return VTSS_RC_OK;
        case MESA_TARGET_7556:
            *out = VTSS_TARGET_7556;
            return VTSS_RC_OK;
        case MESA_TARGET_7558:
            *out = VTSS_TARGET_7558;
            return VTSS_RC_OK;
        case MESA_TARGET_7546TSN:
            *out = VTSS_TARGET_7546TSN;
            return VTSS_RC_OK;
        case MESA_TARGET_7549TSN:
            *out = VTSS_TARGET_7549TSN;
            return VTSS_RC_OK;
        case MESA_TARGET_7552TSN:
            *out = VTSS_TARGET_7552TSN;
            return VTSS_RC_OK;
        case MESA_TARGET_7556TSN:
            *out = VTSS_TARGET_7556TSN;
            return VTSS_RC_OK;
        case MESA_TARGET_7558TSN:
            *out = VTSS_TARGET_7558TSN;
            return VTSS_RC_OK;
        case MESA_TARGET_LAN9662:
            *out = VTSS_TARGET_LAN9662;
            return VTSS_RC_OK;
        case MESA_TARGET_LAN9668:
            *out = VTSS_TARGET_LAN9668;
            return VTSS_RC_OK;
        case MESA_TARGET_LAN9694:
            *out = VTSS_TARGET_LAN9694;
            return VTSS_RC_OK;
        case MESA_TARGET_LAN9691VAO:
            *out = VTSS_TARGET_LAN9691VAO;
            return VTSS_RC_OK;
        case MESA_TARGET_LAN9694TSN:
            *out = VTSS_TARGET_LAN9694TSN;
            return VTSS_RC_OK;
        case MESA_TARGET_LAN9694RED:
            *out = VTSS_TARGET_LAN9694RED;
            return VTSS_RC_OK;
        case MESA_TARGET_LAN9696:
            *out = VTSS_TARGET_LAN9696;
            return VTSS_RC_OK;
        case MESA_TARGET_LAN9692VAO:
            *out = VTSS_TARGET_LAN9692VAO;
            return VTSS_RC_OK;
        case MESA_TARGET_LAN9696TSN:
            *out = VTSS_TARGET_LAN9696TSN;
            return VTSS_RC_OK;
        case MESA_TARGET_LAN9696RED:
            *out = VTSS_TARGET_LAN9696RED;
            return VTSS_RC_OK;
        case MESA_TARGET_LAN9698:
            *out = VTSS_TARGET_LAN9698;
            return VTSS_RC_OK;
        case MESA_TARGET_LAN9693VAO:
            *out = VTSS_TARGET_LAN9693VAO;
            return VTSS_RC_OK;
        case MESA_TARGET_LAN9698TSN:
            *out = VTSS_TARGET_LAN9698TSN;
            return VTSS_RC_OK;
        case MESA_TARGET_LAN9698RED:
            *out = VTSS_TARGET_LAN9698RED;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_vtss_inst_create_t_to_mesa_inst_create_t(const vtss_inst_create_t *in, mesa_inst_create_t *out) /* ag.rb:1656 */
{
    __RC(mesa_conv_vtss_target_type_t_to_mesa_target_type_t(&in->target, &out->target)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_inst_create_t_to_vtss_inst_create_t(const mesa_inst_create_t *in, vtss_inst_create_t *out) /* ag.rb:1664 */
{
    __RC(mesa_conv_mesa_target_type_t_to_vtss_target_type_t(&in->target, &out->target)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}

#if defined(VTSS_FEATURE_PORT_MUX)
mesa_rc mesa_conv_vtss_port_mux_mode_t_to_mesa_port_mux_mode_t(const vtss_port_mux_mode_t *in, mesa_port_mux_mode_t *out) /* ag.rb:1656 */
{
    switch (*in) {
#if defined(VTSS_ARCH_LUTON26)
        case VTSS_PORT_MUX_MODE_0:
            *out = MESA_PORT_MUX_MODE_0;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LUTON26)
#if defined(VTSS_ARCH_JAGUAR_2)
        case VTSS_PORT_MUX_MODE_0:
            *out = MESA_PORT_MUX_MODE_0;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_JAGUAR_2)
#if defined(VTSS_ARCH_OCELOT)
        case VTSS_PORT_MUX_MODE_0:
            *out = MESA_PORT_MUX_MODE_0;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_LAN966X)
        case VTSS_PORT_MUX_MODE_0:
            *out = MESA_PORT_MUX_MODE_0;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_LUTON26)
        case VTSS_PORT_MUX_MODE_1:
            *out = MESA_PORT_MUX_MODE_1;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LUTON26)
#if defined(VTSS_ARCH_JAGUAR_2)
        case VTSS_PORT_MUX_MODE_1:
            *out = MESA_PORT_MUX_MODE_1;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_JAGUAR_2)
#if defined(VTSS_ARCH_OCELOT)
        case VTSS_PORT_MUX_MODE_1:
            *out = MESA_PORT_MUX_MODE_1;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_LAN966X)
        case VTSS_PORT_MUX_MODE_1:
            *out = MESA_PORT_MUX_MODE_1;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_LUTON26)
        case VTSS_PORT_MUX_MODE_2:
            *out = MESA_PORT_MUX_MODE_2;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LUTON26)
#if defined(VTSS_ARCH_JAGUAR_2)
        case VTSS_PORT_MUX_MODE_2:
            *out = MESA_PORT_MUX_MODE_2;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_JAGUAR_2)
#if defined(VTSS_ARCH_OCELOT)
        case VTSS_PORT_MUX_MODE_2:
            *out = MESA_PORT_MUX_MODE_2;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_LAN966X)
        case VTSS_PORT_MUX_MODE_2:
            *out = MESA_PORT_MUX_MODE_2;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_JAGUAR_2)
        case VTSS_PORT_MUX_MODE_AUTO:
            *out = MESA_PORT_MUX_MODE_AUTO;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_JAGUAR_2)
#if defined(VTSS_ARCH_OCELOT)
        case VTSS_PORT_MUX_MODE_3:
            *out = MESA_PORT_MUX_MODE_3;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_LAN966X)
        case VTSS_PORT_MUX_MODE_3:
            *out = MESA_PORT_MUX_MODE_3;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_OCELOT)
        case VTSS_PORT_MUX_MODE_4:
            *out = MESA_PORT_MUX_MODE_4;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_LAN966X)
        case VTSS_PORT_MUX_MODE_4:
            *out = MESA_PORT_MUX_MODE_4;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_OCELOT)
        case VTSS_PORT_MUX_MODE_5:
            *out = MESA_PORT_MUX_MODE_5;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_LAN966X)
        case VTSS_PORT_MUX_MODE_5:
            *out = MESA_PORT_MUX_MODE_5;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LAN966X)
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_port_mux_mode_t_to_vtss_port_mux_mode_t(const mesa_port_mux_mode_t *in, vtss_port_mux_mode_t *out) /* ag.rb:1664 */
{
    switch (*in) {
#if defined(VTSS_ARCH_JAGUAR_2)
        case MESA_PORT_MUX_MODE_AUTO:
            *out = VTSS_PORT_MUX_MODE_AUTO;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_JAGUAR_2)
#if defined(VTSS_ARCH_LUTON26)
        case MESA_PORT_MUX_MODE_0:
            *out = VTSS_PORT_MUX_MODE_0;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LUTON26)
#if defined(VTSS_ARCH_JAGUAR_2)
        case MESA_PORT_MUX_MODE_0:
            *out = VTSS_PORT_MUX_MODE_0;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_JAGUAR_2)
#if defined(VTSS_ARCH_OCELOT)
        case MESA_PORT_MUX_MODE_0:
            *out = VTSS_PORT_MUX_MODE_0;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_LAN966X)
        case MESA_PORT_MUX_MODE_0:
            *out = VTSS_PORT_MUX_MODE_0;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_LUTON26)
        case MESA_PORT_MUX_MODE_1:
            *out = VTSS_PORT_MUX_MODE_1;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LUTON26)
#if defined(VTSS_ARCH_JAGUAR_2)
        case MESA_PORT_MUX_MODE_1:
            *out = VTSS_PORT_MUX_MODE_1;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_JAGUAR_2)
#if defined(VTSS_ARCH_OCELOT)
        case MESA_PORT_MUX_MODE_1:
            *out = VTSS_PORT_MUX_MODE_1;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_LAN966X)
        case MESA_PORT_MUX_MODE_1:
            *out = VTSS_PORT_MUX_MODE_1;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_LUTON26)
        case MESA_PORT_MUX_MODE_2:
            *out = VTSS_PORT_MUX_MODE_2;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LUTON26)
#if defined(VTSS_ARCH_JAGUAR_2)
        case MESA_PORT_MUX_MODE_2:
            *out = VTSS_PORT_MUX_MODE_2;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_JAGUAR_2)
#if defined(VTSS_ARCH_OCELOT)
        case MESA_PORT_MUX_MODE_2:
            *out = VTSS_PORT_MUX_MODE_2;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_LAN966X)
        case MESA_PORT_MUX_MODE_2:
            *out = VTSS_PORT_MUX_MODE_2;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_OCELOT)
        case MESA_PORT_MUX_MODE_3:
            *out = VTSS_PORT_MUX_MODE_3;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_LAN966X)
        case MESA_PORT_MUX_MODE_3:
            *out = VTSS_PORT_MUX_MODE_3;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_OCELOT)
        case MESA_PORT_MUX_MODE_4:
            *out = VTSS_PORT_MUX_MODE_4;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_LAN966X)
        case MESA_PORT_MUX_MODE_4:
            *out = VTSS_PORT_MUX_MODE_4;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_OCELOT)
        case MESA_PORT_MUX_MODE_5:
            *out = VTSS_PORT_MUX_MODE_5;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_LAN966X)
        case MESA_PORT_MUX_MODE_5:
            *out = VTSS_PORT_MUX_MODE_5;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LAN966X)
        default:
            return VTSS_RC_ERROR;
    }
}
#endif  // defined(VTSS_FEATURE_PORT_MUX)

#if defined(VTSS_FEATURE_CORE_CLOCK)
mesa_rc mesa_conv_vtss_core_clock_freq_t_to_mesa_core_clock_freq_t(const vtss_core_clock_freq_t *in, mesa_core_clock_freq_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        // skipped: VTSS_CORE_CLOCK_180MHZ
        case VTSS_CORE_CLOCK_DEFAULT:
            *out = MESA_CORE_CLOCK_DEFAULT;
            return VTSS_RC_OK;
        case VTSS_CORE_CLOCK_250MHZ:
            *out = MESA_CORE_CLOCK_250MHZ;
            return VTSS_RC_OK;
        case VTSS_CORE_CLOCK_328MHZ:
            *out = MESA_CORE_CLOCK_328MHZ;
            return VTSS_RC_OK;
        case VTSS_CORE_CLOCK_500MHZ:
            *out = MESA_CORE_CLOCK_500MHZ;
            return VTSS_RC_OK;
        case VTSS_CORE_CLOCK_625MHZ:
            *out = MESA_CORE_CLOCK_625MHZ;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_core_clock_freq_t_to_vtss_core_clock_freq_t(const mesa_core_clock_freq_t *in, vtss_core_clock_freq_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        // skipped: VTSS_CORE_CLOCK_180MHZ
        case MESA_CORE_CLOCK_DEFAULT:
            *out = VTSS_CORE_CLOCK_DEFAULT;
            return VTSS_RC_OK;
        case MESA_CORE_CLOCK_250MHZ:
            *out = VTSS_CORE_CLOCK_250MHZ;
            return VTSS_RC_OK;
        case MESA_CORE_CLOCK_328MHZ:
            *out = VTSS_CORE_CLOCK_328MHZ;
            return VTSS_RC_OK;
        case MESA_CORE_CLOCK_500MHZ:
            *out = VTSS_CORE_CLOCK_500MHZ;
            return VTSS_RC_OK;
        case MESA_CORE_CLOCK_625MHZ:
            *out = VTSS_CORE_CLOCK_625MHZ;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}
#endif  // defined(VTSS_FEATURE_CORE_CLOCK)

#if defined(VTSS_FEATURE_CORE_CLOCK)
mesa_rc mesa_conv_vtss_core_clock_conf_t_to_mesa_core_clock_conf_t(const vtss_core_clock_conf_t *in, mesa_core_clock_conf_t *out) /* ag.rb:1656 */
{
    __RC(mesa_conv_vtss_core_clock_freq_t_to_mesa_core_clock_freq_t(&in->freq, &out->freq)); /* ag.rb:1432 */
    {
        const mesa_core_ref_clk_t *tmp = (const mesa_core_ref_clk_t *)&in->ref_freq;
#ifdef __cplusplus
        static_assert(sizeof(out.ref_freq) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ref_freq = *tmp;
    }

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_core_clock_conf_t_to_vtss_core_clock_conf_t(const mesa_core_clock_conf_t *in, vtss_core_clock_conf_t *out) /* ag.rb:1664 */
{
    __RC(mesa_conv_mesa_core_clock_freq_t_to_vtss_core_clock_freq_t(&in->freq, &out->freq)); /* ag.rb:1432 */
    {
        const vtss_core_ref_clk_t *tmp = (const vtss_core_ref_clk_t *)&in->ref_freq;
#ifdef __cplusplus
        static_assert(sizeof(out.ref_freq) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ref_freq = *tmp;
    }

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_CORE_CLOCK)

mesa_rc mesa_conv_vtss_init_conf_t_to_mesa_init_conf_t(const vtss_init_conf_t *in, mesa_init_conf_t *out) /* ag.rb:1656 */
{
    // skipped: pi@vtss_init_conf_t
    out->reg_read = in->reg_read;
    out->reg_write = in->reg_write;
#if defined(VTSS_FEATURE_CLOCK)
    out->clock_read = in->clock_read;
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
    out->clock_write = in->clock_write;
#endif  // defined(VTSS_FEATURE_CLOCK)
    {
        const mesa_miim_read_t *tmp = (const mesa_miim_read_t *)&in->miim_read;
#ifdef __cplusplus
        static_assert(sizeof(out.miim_read) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->miim_read = *tmp;
    }
    {
        const mesa_miim_write_t *tmp = (const mesa_miim_write_t *)&in->miim_write;
#ifdef __cplusplus
        static_assert(sizeof(out.miim_write) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->miim_write = *tmp;
    }
    {
        const mesa_mmd_read_t *tmp = (const mesa_mmd_read_t *)&in->mmd_read;
#ifdef __cplusplus
        static_assert(sizeof(out.mmd_read) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->mmd_read = *tmp;
    }
    {
        const mesa_mmd_read_inc_t *tmp = (const mesa_mmd_read_inc_t *)&in->mmd_read_inc;
#ifdef __cplusplus
        static_assert(sizeof(out.mmd_read_inc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->mmd_read_inc = *tmp;
    }
    {
        const mesa_mmd_write_t *tmp = (const mesa_mmd_write_t *)&in->mmd_write;
#ifdef __cplusplus
        static_assert(sizeof(out.mmd_write) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->mmd_write = *tmp;
    }
    {
        const mesa_spi_read_write_t *tmp = (const mesa_spi_read_write_t *)&in->spi_read_write;
#ifdef __cplusplus
        static_assert(sizeof(out.spi_read_write) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->spi_read_write = *tmp;
    }
    {
        const mesa_spi_32bit_read_write_t *tmp = (const mesa_spi_32bit_read_write_t *)&in->spi_32bit_read_write;
#ifdef __cplusplus
        static_assert(sizeof(out.spi_32bit_read_write) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->spi_32bit_read_write = *tmp;
    }
    {
        const mesa_spi_64bit_read_write_t *tmp = (const mesa_spi_64bit_read_write_t *)&in->spi_64bit_read_write;
#ifdef __cplusplus
        static_assert(sizeof(out.spi_64bit_read_write) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->spi_64bit_read_write = *tmp;
    }
#if defined(VTSS_GPIOS)
    {
        const mesa_gpio_func_info_get_t *tmp = (const mesa_gpio_func_info_get_t *)&in->gpio_func_info_get;
#ifdef __cplusplus
        static_assert(sizeof(out.gpio_func_info_get) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->gpio_func_info_get = *tmp;
    }
#endif  // defined(VTSS_GPIOS)
    {
        const mesa_port_serdes_tap_get_t *tmp = (const mesa_port_serdes_tap_get_t *)&in->serdes_tap_get;
#ifdef __cplusplus
        static_assert(sizeof(out.serdes_tap_get) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->serdes_tap_get = *tmp;
    }
    out->skip_switch_reset = in->skip_switch_reset;
    out->spi_bus = in->spi_bus;
#if defined(VTSS_FEATURE_WARM_START)
    out->warm_start_enable = in->warm_start_enable;
#endif  // defined(VTSS_FEATURE_WARM_START)
#if defined(VTSS_FEATURE_WARM_START)
    {
        const mesa_restart_info_src_t *tmp = (const mesa_restart_info_src_t *)&in->restart_info_src;
#ifdef __cplusplus
        static_assert(sizeof(out.restart_info_src) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->restart_info_src = *tmp;
    }
#endif  // defined(VTSS_FEATURE_WARM_START)
#if defined(VTSS_FEATURE_WARM_START)
    out->restart_info_port = in->restart_info_port;
#endif  // defined(VTSS_FEATURE_WARM_START)
#if defined(VTSS_FEATURE_PORT_MUX)
    __RC(mesa_conv_vtss_port_mux_mode_t_to_mesa_port_mux_mode_t(&in->mux_mode, &out->mux_mode)); /* ag.rb:1432 */
#endif  // defined(VTSS_FEATURE_PORT_MUX)
#if defined(VTSS_FEATURE_SERDES_MACRO_SETTINGS)
    {
        const mesa_serdes_macro_conf_t *tmp = (const mesa_serdes_macro_conf_t *)&in->serdes;
#ifdef __cplusplus
        static_assert(sizeof(out.serdes) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->serdes = *tmp;
    }
#endif  // defined(VTSS_FEATURE_SERDES_MACRO_SETTINGS)
    out->using_ufdma = in->using_ufdma;
    out->loopback_bw_mbps = in->loopback_bw_mbps;
#if defined(VTSS_ARCH_JAGUAR_2_CE)
    {
        const mesa_qs_conf_t *tmp = (const mesa_qs_conf_t *)&in->qs_conf;
#ifdef __cplusplus
        static_assert(sizeof(out.qs_conf) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->qs_conf = *tmp;
    }
#endif  // defined(VTSS_ARCH_JAGUAR_2_CE)
#if defined(VTSS_FEATURE_VLAN_COUNTERS)
    out->vlan_counters_disable = in->vlan_counters_disable;
#endif  // defined(VTSS_FEATURE_VLAN_COUNTERS)
#if defined(VTSS_FEATURE_PSFP)
    out->psfp_counters_enable = in->psfp_counters_enable;
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_CORE_CLOCK)
    __RC(mesa_conv_vtss_core_clock_conf_t_to_mesa_core_clock_conf_t(&in->core_clock, &out->core_clock)); /* ag.rb:1432 */
#endif  // defined(VTSS_FEATURE_CORE_CLOCK)
#if defined(VTSS_ARCH_LUTON26)
    out->packet_init_disable = in->packet_init_disable;
#endif  // defined(VTSS_ARCH_LUTON26)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_init_conf_t_to_vtss_init_conf_t(const mesa_init_conf_t *in, vtss_init_conf_t *out) /* ag.rb:1664 */
{
    // skipped: pi@vtss_init_conf_t
    out->reg_read = in->reg_read;
    out->reg_write = in->reg_write;
#if defined(VTSS_FEATURE_CLOCK)
    out->clock_read = in->clock_read;
#endif  // defined(VTSS_FEATURE_CLOCK)
#if defined(VTSS_FEATURE_CLOCK)
    out->clock_write = in->clock_write;
#endif  // defined(VTSS_FEATURE_CLOCK)
    {
        const vtss_miim_read_t *tmp = (const vtss_miim_read_t *)&in->miim_read;
#ifdef __cplusplus
        static_assert(sizeof(out.miim_read) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->miim_read = *tmp;
    }
    {
        const vtss_miim_write_t *tmp = (const vtss_miim_write_t *)&in->miim_write;
#ifdef __cplusplus
        static_assert(sizeof(out.miim_write) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->miim_write = *tmp;
    }
    {
        const vtss_mmd_read_t *tmp = (const vtss_mmd_read_t *)&in->mmd_read;
#ifdef __cplusplus
        static_assert(sizeof(out.mmd_read) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->mmd_read = *tmp;
    }
    {
        const vtss_mmd_read_inc_t *tmp = (const vtss_mmd_read_inc_t *)&in->mmd_read_inc;
#ifdef __cplusplus
        static_assert(sizeof(out.mmd_read_inc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->mmd_read_inc = *tmp;
    }
    {
        const vtss_mmd_write_t *tmp = (const vtss_mmd_write_t *)&in->mmd_write;
#ifdef __cplusplus
        static_assert(sizeof(out.mmd_write) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->mmd_write = *tmp;
    }
    {
        const vtss_spi_read_write_t *tmp = (const vtss_spi_read_write_t *)&in->spi_read_write;
#ifdef __cplusplus
        static_assert(sizeof(out.spi_read_write) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->spi_read_write = *tmp;
    }
    {
        const vtss_spi_32bit_read_write_t *tmp = (const vtss_spi_32bit_read_write_t *)&in->spi_32bit_read_write;
#ifdef __cplusplus
        static_assert(sizeof(out.spi_32bit_read_write) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->spi_32bit_read_write = *tmp;
    }
    {
        const vtss_spi_64bit_read_write_t *tmp = (const vtss_spi_64bit_read_write_t *)&in->spi_64bit_read_write;
#ifdef __cplusplus
        static_assert(sizeof(out.spi_64bit_read_write) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->spi_64bit_read_write = *tmp;
    }
#if defined(VTSS_GPIOS)
    {
        const vtss_gpio_func_info_get_t *tmp = (const vtss_gpio_func_info_get_t *)&in->gpio_func_info_get;
#ifdef __cplusplus
        static_assert(sizeof(out.gpio_func_info_get) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->gpio_func_info_get = *tmp;
    }
#endif  // defined(VTSS_GPIOS)
    {
        const vtss_port_serdes_tap_get_t *tmp = (const vtss_port_serdes_tap_get_t *)&in->serdes_tap_get;
#ifdef __cplusplus
        static_assert(sizeof(out.serdes_tap_get) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->serdes_tap_get = *tmp;
    }
    out->skip_switch_reset = in->skip_switch_reset;
    out->spi_bus = in->spi_bus;
#if defined(VTSS_FEATURE_WARM_START)
    out->warm_start_enable = in->warm_start_enable;
#endif  // defined(VTSS_FEATURE_WARM_START)
#if defined(VTSS_FEATURE_WARM_START)
    {
        const vtss_restart_info_src_t *tmp = (const vtss_restart_info_src_t *)&in->restart_info_src;
#ifdef __cplusplus
        static_assert(sizeof(out.restart_info_src) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->restart_info_src = *tmp;
    }
#endif  // defined(VTSS_FEATURE_WARM_START)
#if defined(VTSS_FEATURE_WARM_START)
    out->restart_info_port = in->restart_info_port;
#endif  // defined(VTSS_FEATURE_WARM_START)
#if defined(VTSS_FEATURE_PORT_MUX)
    __RC(mesa_conv_mesa_port_mux_mode_t_to_vtss_port_mux_mode_t(&in->mux_mode, &out->mux_mode)); /* ag.rb:1432 */
#endif  // defined(VTSS_FEATURE_PORT_MUX)
#if defined(VTSS_FEATURE_SERDES_MACRO_SETTINGS)
    {
        const vtss_serdes_macro_conf_t *tmp = (const vtss_serdes_macro_conf_t *)&in->serdes;
#ifdef __cplusplus
        static_assert(sizeof(out.serdes) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->serdes = *tmp;
    }
#endif  // defined(VTSS_FEATURE_SERDES_MACRO_SETTINGS)
    out->using_ufdma = in->using_ufdma;
    out->loopback_bw_mbps = in->loopback_bw_mbps;
#if defined(VTSS_ARCH_JAGUAR_2_CE)
    {
        const vtss_qs_conf_t *tmp = (const vtss_qs_conf_t *)&in->qs_conf;
#ifdef __cplusplus
        static_assert(sizeof(out.qs_conf) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->qs_conf = *tmp;
    }
#endif  // defined(VTSS_ARCH_JAGUAR_2_CE)
#if defined(VTSS_FEATURE_VLAN_COUNTERS)
    out->vlan_counters_disable = in->vlan_counters_disable;
#endif  // defined(VTSS_FEATURE_VLAN_COUNTERS)
#if defined(VTSS_FEATURE_PSFP)
    out->psfp_counters_enable = in->psfp_counters_enable;
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_CORE_CLOCK)
    __RC(mesa_conv_mesa_core_clock_conf_t_to_vtss_core_clock_conf_t(&in->core_clock, &out->core_clock)); /* ag.rb:1432 */
#endif  // defined(VTSS_FEATURE_CORE_CLOCK)
#if defined(VTSS_ARCH_LUTON26)
    out->packet_init_disable = in->packet_init_disable;
#endif  // defined(VTSS_ARCH_LUTON26)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_vtss_spi_slave_init_t_to_mesa_spi_slave_init_t(const vtss_spi_slave_init_t *in, mesa_spi_slave_init_t *out) /* ag.rb:1656 */
{
    out->reg_write = in->reg_write;
    out->reg_read = in->reg_read;
    {
        const mesa_spi_endian_t *tmp = (const mesa_spi_endian_t *)&in->endian;
#ifdef __cplusplus
        static_assert(sizeof(out.endian) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->endian = *tmp;
    }
    {
        const mesa_spi_bit_order_t *tmp = (const mesa_spi_bit_order_t *)&in->bit_order;
#ifdef __cplusplus
        static_assert(sizeof(out.bit_order) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->bit_order = *tmp;
    }
    out->padding = in->padding;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_spi_slave_init_t_to_vtss_spi_slave_init_t(const mesa_spi_slave_init_t *in, vtss_spi_slave_init_t *out) /* ag.rb:1664 */
{
    out->reg_write = in->reg_write;
    out->reg_read = in->reg_read;
    {
        const vtss_spi_endian_t *tmp = (const vtss_spi_endian_t *)&in->endian;
#ifdef __cplusplus
        static_assert(sizeof(out.endian) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->endian = *tmp;
    }
    {
        const vtss_spi_bit_order_t *tmp = (const vtss_spi_bit_order_t *)&in->bit_order;
#ifdef __cplusplus
        static_assert(sizeof(out.bit_order) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->bit_order = *tmp;
    }
    out->padding = in->padding;

    return VTSS_RC_OK;
}

#if defined(VTSS_FEATURE_WARM_START)
mesa_rc mesa_conv_vtss_restart_t_to_mesa_restart_t(const vtss_restart_t *in, mesa_restart_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        case VTSS_RESTART_COLD:
            *out = MESA_RESTART_COLD;
            return VTSS_RC_OK;
        case VTSS_RESTART_COOL:
            *out = MESA_RESTART_COOL;
            return VTSS_RC_OK;
        case VTSS_RESTART_WARM:
            *out = MESA_RESTART_WARM;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_restart_t_to_vtss_restart_t(const mesa_restart_t *in, vtss_restart_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        case MESA_RESTART_COLD:
            *out = VTSS_RESTART_COLD;
            return VTSS_RC_OK;
        case MESA_RESTART_COOL:
            *out = VTSS_RESTART_COOL;
            return VTSS_RC_OK;
        case MESA_RESTART_WARM:
            *out = VTSS_RESTART_WARM;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}
#endif  // defined(VTSS_FEATURE_WARM_START)

#if defined(VTSS_FEATURE_WARM_START)
mesa_rc mesa_conv_vtss_restart_status_t_to_mesa_restart_status_t(const vtss_restart_status_t *in, mesa_restart_status_t *out) /* ag.rb:1656 */
{
    __RC(mesa_conv_vtss_restart_t_to_mesa_restart_t(&in->restart, &out->restart)); /* ag.rb:1432 */
    out->prev_version = in->prev_version;
    out->cur_version = in->cur_version;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_restart_status_t_to_vtss_restart_status_t(const mesa_restart_status_t *in, vtss_restart_status_t *out) /* ag.rb:1664 */
{
    __RC(mesa_conv_mesa_restart_t_to_vtss_restart_t(&in->restart, &out->restart)); /* ag.rb:1432 */
    out->prev_version = in->prev_version;
    out->cur_version = in->cur_version;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_WARM_START)

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_conv_vtss_mac_table_entry_t_to_mesa_mac_table_entry_t(const vtss_mac_table_entry_t *in, mesa_mac_table_entry_t *out) /* ag.rb:1656 */
{
    {
        const mesa_vid_mac_t *tmp = (const mesa_vid_mac_t *)&in->vid_mac;
#ifdef __cplusplus
        static_assert(sizeof(out.vid_mac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->vid_mac = *tmp;
    }
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(in->destination, &out->destination)); /* ag.rb:1432 */
    out->copy_to_cpu = in->copy_to_cpu;
    out->copy_to_cpu_smac = in->copy_to_cpu_smac;
    out->locked = in->locked;
#if defined(VTSS_FEATURE_MAC_INDEX_TABLE)
    out->index_table = in->index_table;
#endif  // defined(VTSS_FEATURE_MAC_INDEX_TABLE)
    out->aged = in->aged;
    out->cpu_queue = in->cpu_queue;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_mac_table_entry_t_to_vtss_mac_table_entry_t(const mesa_mac_table_entry_t *in, vtss_mac_table_entry_t *out) /* ag.rb:1664 */
{
    {
        const vtss_vid_mac_t *tmp = (const vtss_vid_mac_t *)&in->vid_mac;
#ifdef __cplusplus
        static_assert(sizeof(out.vid_mac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->vid_mac = *tmp;
    }
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(&in->destination, out->destination)); /* ag.rb:1432 */
    out->copy_to_cpu = in->copy_to_cpu;
    out->copy_to_cpu_smac = in->copy_to_cpu_smac;
    out->locked = in->locked;
#if defined(VTSS_FEATURE_MAC_INDEX_TABLE)
    out->index_table = in->index_table;
#endif  // defined(VTSS_FEATURE_MAC_INDEX_TABLE)
    out->aged = in->aged;
    out->cpu_queue = in->cpu_queue;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_conv_vtss_vlan_port_conf_t_to_mesa_vlan_port_conf_t(const vtss_vlan_port_conf_t *in, mesa_vlan_port_conf_t *out) /* ag.rb:1656 */
{
    // skipped: s_etype@vtss_vlan_port_conf_t
    {
        const mesa_vlan_port_type_t *tmp = (const mesa_vlan_port_type_t *)&in->port_type;
#ifdef __cplusplus
        static_assert(sizeof(out.port_type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->port_type = *tmp;
    }
    out->pvid = in->pvid;
    out->untagged_vid = in->untagged_vid;
    {
        const mesa_vlan_frame_t *tmp = (const mesa_vlan_frame_t *)&in->frame_type;
#ifdef __cplusplus
        static_assert(sizeof(out.frame_type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->frame_type = *tmp;
    }
    out->ingress_filter = in->ingress_filter;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_vlan_port_conf_t_to_vtss_vlan_port_conf_t(const mesa_vlan_port_conf_t *in, vtss_vlan_port_conf_t *out) /* ag.rb:1664 */
{
    // skipped: s_etype@vtss_vlan_port_conf_t
    {
        const vtss_vlan_port_type_t *tmp = (const vtss_vlan_port_type_t *)&in->port_type;
#ifdef __cplusplus
        static_assert(sizeof(out.port_type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->port_type = *tmp;
    }
    out->pvid = in->pvid;
    out->untagged_vid = in->untagged_vid;
    {
        const vtss_vlan_frame_t *tmp = (const vtss_vlan_frame_t *)&in->frame_type;
#ifdef __cplusplus
        static_assert(sizeof(out.frame_type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->frame_type = *tmp;
    }
    out->ingress_filter = in->ingress_filter;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_conv_vtss_vlan_vid_conf_t_to_mesa_vlan_vid_conf_t(const vtss_vlan_vid_conf_t *in, mesa_vlan_vid_conf_t *out) /* ag.rb:1656 */
{
    out->learning = in->learning;
    out->flooding = in->flooding;
    out->mirror = in->mirror;
    out->ingress_filter = in->ingress_filter;
#if defined(VTSS_FEATURE_QOS_OT)
    out->ot = in->ot;
#endif  // defined(VTSS_FEATURE_QOS_OT)
#if defined(VTSS_FEATURE_VLAN_SVL)
    out->fid = in->fid;
#endif  // defined(VTSS_FEATURE_VLAN_SVL)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_vlan_vid_conf_t_to_vtss_vlan_vid_conf_t(const mesa_vlan_vid_conf_t *in, vtss_vlan_vid_conf_t *out) /* ag.rb:1664 */
{
    out->learning = in->learning;
    out->flooding = in->flooding;
    out->mirror = in->mirror;
    out->ingress_filter = in->ingress_filter;
#if defined(VTSS_FEATURE_QOS_OT)
    out->ot = in->ot;
#endif  // defined(VTSS_FEATURE_QOS_OT)
#if defined(VTSS_FEATURE_VLAN_SVL)
    out->fid = in->fid;
#endif  // defined(VTSS_FEATURE_VLAN_SVL)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_vce_mac_t_to_mesa_vce_mac_t(const vtss_vce_mac_t *in, mesa_vce_mac_t *out) /* ag.rb:1656 */
{
    {
        const mesa_vcap_bit_t *tmp = (const mesa_vcap_bit_t *)&in->dmac_mc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_mc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_mc = *tmp;
    }
    {
        const mesa_vcap_bit_t *tmp = (const mesa_vcap_bit_t *)&in->dmac_bc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_bc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_bc = *tmp;
    }
#if defined(VTSS_FEATURE_VCL_KEY_DMAC)
    {
        const mesa_vcap_u48_t *tmp = (const mesa_vcap_u48_t *)&in->dmac;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac = *tmp;
    }
#endif  // defined(VTSS_FEATURE_VCL_KEY_DMAC)
    {
        const mesa_vcap_u48_t *tmp = (const mesa_vcap_u48_t *)&in->smac;
#ifdef __cplusplus
        static_assert(sizeof(out.smac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->smac = *tmp;
    }

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_vce_mac_t_to_vtss_vce_mac_t(const mesa_vce_mac_t *in, vtss_vce_mac_t *out) /* ag.rb:1664 */
{
    {
        const vtss_vcap_bit_t *tmp = (const vtss_vcap_bit_t *)&in->dmac_mc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_mc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_mc = *tmp;
    }
    {
        const vtss_vcap_bit_t *tmp = (const vtss_vcap_bit_t *)&in->dmac_bc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_bc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_bc = *tmp;
    }
#if defined(VTSS_FEATURE_VCL_KEY_DMAC)
    {
        const vtss_vcap_u48_t *tmp = (const vtss_vcap_u48_t *)&in->dmac;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac = *tmp;
    }
#endif  // defined(VTSS_FEATURE_VCL_KEY_DMAC)
    {
        const vtss_vcap_u48_t *tmp = (const vtss_vcap_u48_t *)&in->smac;
#ifdef __cplusplus
        static_assert(sizeof(out.smac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->smac = *tmp;
    }

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_vce_frame_ipv4_t_to_mesa_vce_frame_ipv4_t(const vtss_vce_frame_ipv4_t *in, mesa_vce_frame_ipv4_t *out) /* ag.rb:1656 */
{
    {
        const mesa_vcap_bit_t *tmp = (const mesa_vcap_bit_t *)&in->fragment;
#ifdef __cplusplus
        static_assert(sizeof(out.fragment) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->fragment = *tmp;
    }
    {
        const mesa_vcap_bit_t *tmp = (const mesa_vcap_bit_t *)&in->options;
#ifdef __cplusplus
        static_assert(sizeof(out.options) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->options = *tmp;
    }
    {
        const mesa_vcap_vr_t *tmp = (const mesa_vcap_vr_t *)&in->dscp;
#ifdef __cplusplus
        static_assert(sizeof(out.dscp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dscp = *tmp;
    }
    {
        const mesa_vcap_u8_t *tmp = (const mesa_vcap_u8_t *)&in->proto;
#ifdef __cplusplus
        static_assert(sizeof(out.proto) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->proto = *tmp;
    }
    {
        const mesa_vcap_ip_t *tmp = (const mesa_vcap_ip_t *)&in->sip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip = *tmp;
    }
#if defined(VTSS_FEATURE_VCL_KEY_DIP)
    {
        const mesa_vcap_ip_t *tmp = (const mesa_vcap_ip_t *)&in->dip;
#ifdef __cplusplus
        static_assert(sizeof(out.dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dip = *tmp;
    }
#endif  // defined(VTSS_FEATURE_VCL_KEY_DIP)
    {
        const mesa_vcap_vr_t *tmp = (const mesa_vcap_vr_t *)&in->sport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport = *tmp;
    }
    {
        const mesa_vcap_vr_t *tmp = (const mesa_vcap_vr_t *)&in->dport;
#ifdef __cplusplus
        static_assert(sizeof(out.dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dport = *tmp;
    }

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_vce_frame_ipv4_t_to_vtss_vce_frame_ipv4_t(const mesa_vce_frame_ipv4_t *in, vtss_vce_frame_ipv4_t *out) /* ag.rb:1664 */
{
    {
        const vtss_vcap_bit_t *tmp = (const vtss_vcap_bit_t *)&in->fragment;
#ifdef __cplusplus
        static_assert(sizeof(out.fragment) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->fragment = *tmp;
    }
    {
        const vtss_vcap_bit_t *tmp = (const vtss_vcap_bit_t *)&in->options;
#ifdef __cplusplus
        static_assert(sizeof(out.options) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->options = *tmp;
    }
    {
        const vtss_vcap_vr_t *tmp = (const vtss_vcap_vr_t *)&in->dscp;
#ifdef __cplusplus
        static_assert(sizeof(out.dscp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dscp = *tmp;
    }
    {
        const vtss_vcap_u8_t *tmp = (const vtss_vcap_u8_t *)&in->proto;
#ifdef __cplusplus
        static_assert(sizeof(out.proto) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->proto = *tmp;
    }
    {
        const vtss_vcap_ip_t *tmp = (const vtss_vcap_ip_t *)&in->sip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip = *tmp;
    }
#if defined(VTSS_FEATURE_VCL_KEY_DIP)
    {
        const vtss_vcap_ip_t *tmp = (const vtss_vcap_ip_t *)&in->dip;
#ifdef __cplusplus
        static_assert(sizeof(out.dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dip = *tmp;
    }
#endif  // defined(VTSS_FEATURE_VCL_KEY_DIP)
    {
        const vtss_vcap_vr_t *tmp = (const vtss_vcap_vr_t *)&in->sport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport = *tmp;
    }
    {
        const vtss_vcap_vr_t *tmp = (const vtss_vcap_vr_t *)&in->dport;
#ifdef __cplusplus
        static_assert(sizeof(out.dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dport = *tmp;
    }

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_vce_frame_ipv6_t_to_mesa_vce_frame_ipv6_t(const vtss_vce_frame_ipv6_t *in, mesa_vce_frame_ipv6_t *out) /* ag.rb:1656 */
{
    {
        const mesa_vcap_vr_t *tmp = (const mesa_vcap_vr_t *)&in->dscp;
#ifdef __cplusplus
        static_assert(sizeof(out.dscp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dscp = *tmp;
    }
    {
        const mesa_vcap_u8_t *tmp = (const mesa_vcap_u8_t *)&in->proto;
#ifdef __cplusplus
        static_assert(sizeof(out.proto) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->proto = *tmp;
    }
    {
        const mesa_vcap_u128_t *tmp = (const mesa_vcap_u128_t *)&in->sip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip = *tmp;
    }
#if defined(VTSS_FEATURE_VCL_KEY_DIP)
    {
        const mesa_vcap_u128_t *tmp = (const mesa_vcap_u128_t *)&in->dip;
#ifdef __cplusplus
        static_assert(sizeof(out.dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dip = *tmp;
    }
#endif  // defined(VTSS_FEATURE_VCL_KEY_DIP)
    {
        const mesa_vcap_vr_t *tmp = (const mesa_vcap_vr_t *)&in->sport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport = *tmp;
    }
    {
        const mesa_vcap_vr_t *tmp = (const mesa_vcap_vr_t *)&in->dport;
#ifdef __cplusplus
        static_assert(sizeof(out.dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dport = *tmp;
    }

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_vce_frame_ipv6_t_to_vtss_vce_frame_ipv6_t(const mesa_vce_frame_ipv6_t *in, vtss_vce_frame_ipv6_t *out) /* ag.rb:1664 */
{
    {
        const vtss_vcap_vr_t *tmp = (const vtss_vcap_vr_t *)&in->dscp;
#ifdef __cplusplus
        static_assert(sizeof(out.dscp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dscp = *tmp;
    }
    {
        const vtss_vcap_u8_t *tmp = (const vtss_vcap_u8_t *)&in->proto;
#ifdef __cplusplus
        static_assert(sizeof(out.proto) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->proto = *tmp;
    }
    {
        const vtss_vcap_u128_t *tmp = (const vtss_vcap_u128_t *)&in->sip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip = *tmp;
    }
#if defined(VTSS_FEATURE_VCL_KEY_DIP)
    {
        const vtss_vcap_u128_t *tmp = (const vtss_vcap_u128_t *)&in->dip;
#ifdef __cplusplus
        static_assert(sizeof(out.dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dip = *tmp;
    }
#endif  // defined(VTSS_FEATURE_VCL_KEY_DIP)
    {
        const vtss_vcap_vr_t *tmp = (const vtss_vcap_vr_t *)&in->sport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport = *tmp;
    }
    {
        const vtss_vcap_vr_t *tmp = (const vtss_vcap_vr_t *)&in->dport;
#ifdef __cplusplus
        static_assert(sizeof(out.dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dport = *tmp;
    }

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_vce_key_t_to_mesa_vce_key_t(const vtss_vce_key_t *in, mesa_vce_key_t *out) /* ag.rb:1656 */
{
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(in->port_list, &out->port_list)); /* ag.rb:1432 */
    __RC(mesa_conv_vtss_vce_mac_t_to_mesa_vce_mac_t(&in->mac, &out->mac)); /* ag.rb:1432 */
    {
        const mesa_vce_tag_t *tmp = (const mesa_vce_tag_t *)&in->tag;
#ifdef __cplusplus
        static_assert(sizeof(out.tag) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tag = *tmp;
    }
#if defined(VTSS_FEATURE_QCL_KEY_INNER_TAG)
    {
        const mesa_vce_tag_t *tmp = (const mesa_vce_tag_t *)&in->inner_tag;
#ifdef __cplusplus
        static_assert(sizeof(out.inner_tag) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->inner_tag = *tmp;
    }
#endif  // defined(VTSS_FEATURE_QCL_KEY_INNER_TAG)
    {
        const mesa_vce_type_t *tmp = (const mesa_vce_type_t *)&in->type;
#ifdef __cplusplus
        static_assert(sizeof(out.type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->type = *tmp;
    }

    return mesa_conv2_vtss_vce_key_t_to_mesa_vce_key_t(in, out); /* ag.rb:1477 */
}

mesa_rc mesa_conv_mesa_vce_key_t_to_vtss_vce_key_t(const mesa_vce_key_t *in, vtss_vce_key_t *out) /* ag.rb:1664 */
{
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(&in->port_list, out->port_list)); /* ag.rb:1432 */
    __RC(mesa_conv_mesa_vce_mac_t_to_vtss_vce_mac_t(&in->mac, &out->mac)); /* ag.rb:1432 */
    {
        const vtss_vce_tag_t *tmp = (const vtss_vce_tag_t *)&in->tag;
#ifdef __cplusplus
        static_assert(sizeof(out.tag) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tag = *tmp;
    }
#if defined(VTSS_FEATURE_QCL_KEY_INNER_TAG)
    {
        const vtss_vce_tag_t *tmp = (const vtss_vce_tag_t *)&in->inner_tag;
#ifdef __cplusplus
        static_assert(sizeof(out.inner_tag) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->inner_tag = *tmp;
    }
#endif  // defined(VTSS_FEATURE_QCL_KEY_INNER_TAG)
    {
        const vtss_vce_type_t *tmp = (const vtss_vce_type_t *)&in->type;
#ifdef __cplusplus
        static_assert(sizeof(out.type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->type = *tmp;
    }

    return mesa_conv2_mesa_vce_key_t_to_vtss_vce_key_t(in, out); /* ag.rb:1477 */
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_vce_action_t_to_mesa_vce_action_t(const vtss_vce_action_t *in, mesa_vce_action_t *out) /* ag.rb:1656 */
{
    out->vid = in->vid;
    out->policy_no = in->policy_no;
    out->pop_enable = in->pop_enable;
    out->pop_cnt = in->pop_cnt;
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP)
    {
        const mesa_imap_sel_t *tmp = (const mesa_imap_sel_t *)&in->map_sel;
#ifdef __cplusplus
        static_assert(sizeof(out.map_sel) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->map_sel = *tmp;
    }
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP)
    out->map_id = in->map_id;
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP)
#if defined(VTSS_FEATURE_XFLOW)
    out->flow_id = in->flow_id;
#endif  // defined(VTSS_FEATURE_XFLOW)
#if defined(VTSS_FEATURE_XFLOW)
    {
        const mesa_oam_detect_t *tmp = (const mesa_oam_detect_t *)&in->oam_detect;
#ifdef __cplusplus
        static_assert(sizeof(out.oam_detect) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->oam_detect = *tmp;
    }
#endif  // defined(VTSS_FEATURE_XFLOW)
#if defined(VTSS_FEATURE_XFLOW)
    out->mrp_enable = in->mrp_enable;
#endif  // defined(VTSS_FEATURE_XFLOW)
    out->prio_enable = in->prio_enable;
    out->prio = in->prio;
    out->dp_enable = in->dp_enable;
    out->dp = in->dp;
    out->dscp_enable = in->dscp_enable;
    out->dscp = in->dscp;
    out->pcp_enable = in->pcp_enable;
    out->pcp = in->pcp;
    out->dei_enable = in->dei_enable;
    out->dei = in->dei;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_vce_action_t_to_vtss_vce_action_t(const mesa_vce_action_t *in, vtss_vce_action_t *out) /* ag.rb:1664 */
{
    out->vid = in->vid;
    out->policy_no = in->policy_no;
    out->pop_enable = in->pop_enable;
    out->pop_cnt = in->pop_cnt;
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP)
    {
        const vtss_imap_sel_t *tmp = (const vtss_imap_sel_t *)&in->map_sel;
#ifdef __cplusplus
        static_assert(sizeof(out.map_sel) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->map_sel = *tmp;
    }
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP)
    out->map_id = in->map_id;
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP)
#if defined(VTSS_FEATURE_XFLOW)
    out->flow_id = in->flow_id;
#endif  // defined(VTSS_FEATURE_XFLOW)
#if defined(VTSS_FEATURE_XFLOW)
    {
        const vtss_oam_detect_t *tmp = (const vtss_oam_detect_t *)&in->oam_detect;
#ifdef __cplusplus
        static_assert(sizeof(out.oam_detect) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->oam_detect = *tmp;
    }
#endif  // defined(VTSS_FEATURE_XFLOW)
#if defined(VTSS_FEATURE_XFLOW)
    out->mrp_enable = in->mrp_enable;
#endif  // defined(VTSS_FEATURE_XFLOW)
    out->prio_enable = in->prio_enable;
    out->prio = in->prio;
    out->dp_enable = in->dp_enable;
    out->dp = in->dp;
    out->dscp_enable = in->dscp_enable;
    out->dscp = in->dscp;
    out->pcp_enable = in->pcp_enable;
    out->pcp = in->pcp;
    out->dei_enable = in->dei_enable;
    out->dei = in->dei;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_vce_t_to_mesa_vce_t(const vtss_vce_t *in, mesa_vce_t *out) /* ag.rb:1656 */
{
    out->id = in->id;
    __RC(mesa_conv_vtss_vce_key_t_to_mesa_vce_key_t(&in->key, &out->key)); /* ag.rb:1432 */
    __RC(mesa_conv_vtss_vce_action_t_to_mesa_vce_action_t(&in->action, &out->action)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_vce_t_to_vtss_vce_t(const mesa_vce_t *in, vtss_vce_t *out) /* ag.rb:1664 */
{
    out->id = in->id;
    __RC(mesa_conv_mesa_vce_key_t_to_vtss_vce_key_t(&in->key, &out->key)); /* ag.rb:1432 */
    __RC(mesa_conv_mesa_vce_action_t_to_vtss_vce_action_t(&in->action, &out->action)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XSTAT)
mesa_rc mesa_conv_vtss_ingress_counters_t_to_mesa_ingress_counters_t(const vtss_ingress_counters_t *in, mesa_ingress_counters_t *out) /* ag.rb:1656 */
{
    {
        const mesa_counter_pair_t *tmp = (const mesa_counter_pair_t *)&in->rx_green;
#ifdef __cplusplus
        static_assert(sizeof(out.rx_green) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rx_green = *tmp;
    }
    {
        const mesa_counter_pair_t *tmp = (const mesa_counter_pair_t *)&in->rx_yellow;
#ifdef __cplusplus
        static_assert(sizeof(out.rx_yellow) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rx_yellow = *tmp;
    }
    {
        const mesa_counter_pair_t *tmp = (const mesa_counter_pair_t *)&in->rx_red;
#ifdef __cplusplus
        static_assert(sizeof(out.rx_red) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rx_red = *tmp;
    }
#if defined(VTSS_FEATURE_PSFP)
    out->rx_match = in->rx_match;
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
    out->rx_gate_pass = in->rx_gate_pass;
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
    out->rx_gate_discard = in->rx_gate_discard;
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
    out->rx_sdu_pass = in->rx_sdu_pass;
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
    out->rx_sdu_discard = in->rx_sdu_discard;
#endif  // defined(VTSS_FEATURE_PSFP)
    {
        const mesa_counter_pair_t *tmp = (const mesa_counter_pair_t *)&in->rx_discard;
#ifdef __cplusplus
        static_assert(sizeof(out.rx_discard) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rx_discard = *tmp;
    }
    {
        const mesa_counter_pair_t *tmp = (const mesa_counter_pair_t *)&in->tx_discard;
#ifdef __cplusplus
        static_assert(sizeof(out.tx_discard) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tx_discard = *tmp;
    }

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_ingress_counters_t_to_vtss_ingress_counters_t(const mesa_ingress_counters_t *in, vtss_ingress_counters_t *out) /* ag.rb:1664 */
{
    {
        const vtss_counter_pair_t *tmp = (const vtss_counter_pair_t *)&in->rx_green;
#ifdef __cplusplus
        static_assert(sizeof(out.rx_green) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rx_green = *tmp;
    }
    {
        const vtss_counter_pair_t *tmp = (const vtss_counter_pair_t *)&in->rx_yellow;
#ifdef __cplusplus
        static_assert(sizeof(out.rx_yellow) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rx_yellow = *tmp;
    }
    {
        const vtss_counter_pair_t *tmp = (const vtss_counter_pair_t *)&in->rx_red;
#ifdef __cplusplus
        static_assert(sizeof(out.rx_red) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rx_red = *tmp;
    }
#if defined(VTSS_FEATURE_PSFP)
    out->rx_match = in->rx_match;
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
    out->rx_gate_pass = in->rx_gate_pass;
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
    out->rx_gate_discard = in->rx_gate_discard;
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
    out->rx_sdu_pass = in->rx_sdu_pass;
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
    out->rx_sdu_discard = in->rx_sdu_discard;
#endif  // defined(VTSS_FEATURE_PSFP)
    {
        const vtss_counter_pair_t *tmp = (const vtss_counter_pair_t *)&in->rx_discard;
#ifdef __cplusplus
        static_assert(sizeof(out.rx_discard) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rx_discard = *tmp;
    }
    {
        const vtss_counter_pair_t *tmp = (const vtss_counter_pair_t *)&in->tx_discard;
#ifdef __cplusplus
        static_assert(sizeof(out.tx_discard) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tx_discard = *tmp;
    }

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_XSTAT)
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XFLOW)
mesa_rc mesa_conv_vtss_iflow_conf_t_to_mesa_iflow_conf_t(const vtss_iflow_conf_t *in, mesa_iflow_conf_t *out) /* ag.rb:1656 */
{
#if defined(VTSS_FEATURE_XSTAT)
    out->cnt_enable = in->cnt_enable;
#endif  // defined(VTSS_FEATURE_XSTAT)
#if defined(VTSS_FEATURE_XSTAT)
    out->cnt_id = in->cnt_id;
#endif  // defined(VTSS_FEATURE_XSTAT)
#if defined(VTSS_FEATURE_XDLB)
    out->dlb_enable = in->dlb_enable;
#endif  // defined(VTSS_FEATURE_XDLB)
#if defined(VTSS_FEATURE_XDLB)
    out->dlb_id = in->dlb_id;
#endif  // defined(VTSS_FEATURE_XDLB)
#if defined(VTSS_FEATURE_VOP)
    out->voe_idx = in->voe_idx;
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_V2)
    out->voi_idx = in->voi_idx;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_FRER)
    __RC(mesa_conv_vtss_frer_iflow_conf_t_to_mesa_frer_iflow_conf_t(&in->frer, &out->frer)); /* ag.rb:1432 */
#endif  // defined(VTSS_FEATURE_FRER)
#if defined(VTSS_FEATURE_PSFP)
    {
        const mesa_psfp_iflow_conf_t *tmp = (const mesa_psfp_iflow_conf_t *)&in->psfp;
#ifdef __cplusplus
        static_assert(sizeof(out.psfp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->psfp = *tmp;
    }
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_CUT_THROUGH)
    out->cut_through_disable = in->cut_through_disable;
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_CUT_THROUGH)
#if defined(VTSS_FEATURE_QOS_OT)
    out->ot = in->ot;
#endif  // defined(VTSS_FEATURE_QOS_OT)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_iflow_conf_t_to_vtss_iflow_conf_t(const mesa_iflow_conf_t *in, vtss_iflow_conf_t *out) /* ag.rb:1664 */
{
#if defined(VTSS_FEATURE_XSTAT)
    out->cnt_enable = in->cnt_enable;
#endif  // defined(VTSS_FEATURE_XSTAT)
#if defined(VTSS_FEATURE_XSTAT)
    out->cnt_id = in->cnt_id;
#endif  // defined(VTSS_FEATURE_XSTAT)
#if defined(VTSS_FEATURE_XDLB)
    out->dlb_enable = in->dlb_enable;
#endif  // defined(VTSS_FEATURE_XDLB)
#if defined(VTSS_FEATURE_XDLB)
    out->dlb_id = in->dlb_id;
#endif  // defined(VTSS_FEATURE_XDLB)
#if defined(VTSS_FEATURE_VOP)
    out->voe_idx = in->voe_idx;
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_V2)
    out->voi_idx = in->voi_idx;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_FRER)
    __RC(mesa_conv_mesa_frer_iflow_conf_t_to_vtss_frer_iflow_conf_t(&in->frer, &out->frer)); /* ag.rb:1432 */
#endif  // defined(VTSS_FEATURE_FRER)
#if defined(VTSS_FEATURE_PSFP)
    {
        const vtss_psfp_iflow_conf_t *tmp = (const vtss_psfp_iflow_conf_t *)&in->psfp;
#ifdef __cplusplus
        static_assert(sizeof(out.psfp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->psfp = *tmp;
    }
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_CUT_THROUGH)
    out->cut_through_disable = in->cut_through_disable;
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_CUT_THROUGH)
#if defined(VTSS_FEATURE_QOS_OT)
    out->ot = in->ot;
#endif  // defined(VTSS_FEATURE_QOS_OT)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_XFLOW)
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_tce_key_t_to_mesa_tce_key_t(const vtss_tce_key_t *in, mesa_tce_key_t *out) /* ag.rb:1656 */
{
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(in->port_list, &out->port_list)); /* ag.rb:1432 */
    out->vid = in->vid;
#if defined(VTSS_FEATURE_XFLOW)
    out->flow_enable = in->flow_enable;
#endif  // defined(VTSS_FEATURE_XFLOW)
#if defined(VTSS_FEATURE_XFLOW)
    out->flow_id = in->flow_id;
#endif  // defined(VTSS_FEATURE_XFLOW)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_tce_key_t_to_vtss_tce_key_t(const mesa_tce_key_t *in, vtss_tce_key_t *out) /* ag.rb:1664 */
{
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(&in->port_list, out->port_list)); /* ag.rb:1432 */
    out->vid = in->vid;
#if defined(VTSS_FEATURE_XFLOW)
    out->flow_enable = in->flow_enable;
#endif  // defined(VTSS_FEATURE_XFLOW)
#if defined(VTSS_FEATURE_XFLOW)
    out->flow_id = in->flow_id;
#endif  // defined(VTSS_FEATURE_XFLOW)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_tce_tag_t_to_mesa_tce_tag_t(const vtss_tce_tag_t *in, mesa_tce_tag_t *out) /* ag.rb:1656 */
{
    {
        const mesa_tpid_sel_t *tmp = (const mesa_tpid_sel_t *)&in->tpid;
#ifdef __cplusplus
        static_assert(sizeof(out.tpid) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tpid = *tmp;
    }
    out->vid = in->vid;
    {
        const mesa_pcp_sel_t *tmp = (const mesa_pcp_sel_t *)&in->pcp_sel;
#ifdef __cplusplus
        static_assert(sizeof(out.pcp_sel) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->pcp_sel = *tmp;
    }
    out->pcp = in->pcp;
    {
        const mesa_dei_sel_t *tmp = (const mesa_dei_sel_t *)&in->dei_sel;
#ifdef __cplusplus
        static_assert(sizeof(out.dei_sel) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dei_sel = *tmp;
    }
    out->dei = in->dei;
#if defined(VTSS_FEATURE_QOS_EGRESS_MAP)
    out->map_id = in->map_id;
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_MAP)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_tce_tag_t_to_vtss_tce_tag_t(const mesa_tce_tag_t *in, vtss_tce_tag_t *out) /* ag.rb:1664 */
{
    {
        const vtss_tpid_sel_t *tmp = (const vtss_tpid_sel_t *)&in->tpid;
#ifdef __cplusplus
        static_assert(sizeof(out.tpid) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tpid = *tmp;
    }
    out->vid = in->vid;
    {
        const vtss_pcp_sel_t *tmp = (const vtss_pcp_sel_t *)&in->pcp_sel;
#ifdef __cplusplus
        static_assert(sizeof(out.pcp_sel) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->pcp_sel = *tmp;
    }
    out->pcp = in->pcp;
    {
        const vtss_dei_sel_t *tmp = (const vtss_dei_sel_t *)&in->dei_sel;
#ifdef __cplusplus
        static_assert(sizeof(out.dei_sel) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dei_sel = *tmp;
    }
    out->dei = in->dei;
#if defined(VTSS_FEATURE_QOS_EGRESS_MAP)
    out->map_id = in->map_id;
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_MAP)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_tce_action_t_to_mesa_tce_action_t(const vtss_tce_action_t *in, mesa_tce_action_t *out) /* ag.rb:1656 */
{
    __RC(mesa_conv_vtss_tce_tag_t_to_mesa_tce_tag_t(&in->tag, &out->tag)); /* ag.rb:1432 */
    __RC(mesa_conv_vtss_tce_tag_t_to_mesa_tce_tag_t(&in->inner_tag, &out->inner_tag)); /* ag.rb:1432 */
    out->pop_cnt = in->pop_cnt;
#if defined(VTSS_FEATURE_XFLOW)
    out->flow_id = in->flow_id;
#endif  // defined(VTSS_FEATURE_XFLOW)
#if defined(VTSS_FEATURE_FRER)
    {
        const mesa_tce_rtag_t *tmp = (const mesa_tce_rtag_t *)&in->rtag;
#ifdef __cplusplus
        static_assert(sizeof(out.rtag) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rtag = *tmp;
    }
#endif  // defined(VTSS_FEATURE_FRER)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_tce_action_t_to_vtss_tce_action_t(const mesa_tce_action_t *in, vtss_tce_action_t *out) /* ag.rb:1664 */
{
    __RC(mesa_conv_mesa_tce_tag_t_to_vtss_tce_tag_t(&in->tag, &out->tag)); /* ag.rb:1432 */
    __RC(mesa_conv_mesa_tce_tag_t_to_vtss_tce_tag_t(&in->inner_tag, &out->inner_tag)); /* ag.rb:1432 */
    out->pop_cnt = in->pop_cnt;
#if defined(VTSS_FEATURE_XFLOW)
    out->flow_id = in->flow_id;
#endif  // defined(VTSS_FEATURE_XFLOW)
#if defined(VTSS_FEATURE_FRER)
    {
        const vtss_tce_rtag_t *tmp = (const vtss_tce_rtag_t *)&in->rtag;
#ifdef __cplusplus
        static_assert(sizeof(out.rtag) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rtag = *tmp;
    }
#endif  // defined(VTSS_FEATURE_FRER)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_tce_t_to_mesa_tce_t(const vtss_tce_t *in, mesa_tce_t *out) /* ag.rb:1656 */
{
    out->id = in->id;
    __RC(mesa_conv_vtss_tce_key_t_to_mesa_tce_key_t(&in->key, &out->key)); /* ag.rb:1432 */
    __RC(mesa_conv_vtss_tce_action_t_to_mesa_tce_action_t(&in->action, &out->action)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_tce_t_to_vtss_tce_t(const mesa_tce_t *in, vtss_tce_t *out) /* ag.rb:1664 */
{
    out->id = in->id;
    __RC(mesa_conv_mesa_tce_key_t_to_vtss_tce_key_t(&in->key, &out->key)); /* ag.rb:1432 */
    __RC(mesa_conv_mesa_tce_action_t_to_vtss_tce_action_t(&in->action, &out->action)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XFLOW)
mesa_rc mesa_conv_vtss_eflow_conf_t_to_mesa_eflow_conf_t(const vtss_eflow_conf_t *in, mesa_eflow_conf_t *out) /* ag.rb:1656 */
{
#if defined(VTSS_FEATURE_XSTAT)
    out->cnt_enable = in->cnt_enable;
#endif  // defined(VTSS_FEATURE_XSTAT)
#if defined(VTSS_FEATURE_XSTAT)
    out->cnt_id = in->cnt_id;
#endif  // defined(VTSS_FEATURE_XSTAT)
#if defined(VTSS_FEATURE_VOP)
    out->voe_idx = in->voe_idx;
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_V2)
    out->voi_idx = in->voi_idx;
#endif  // defined(VTSS_FEATURE_VOP_V2)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_eflow_conf_t_to_vtss_eflow_conf_t(const mesa_eflow_conf_t *in, vtss_eflow_conf_t *out) /* ag.rb:1664 */
{
#if defined(VTSS_FEATURE_XSTAT)
    out->cnt_enable = in->cnt_enable;
#endif  // defined(VTSS_FEATURE_XSTAT)
#if defined(VTSS_FEATURE_XSTAT)
    out->cnt_id = in->cnt_id;
#endif  // defined(VTSS_FEATURE_XSTAT)
#if defined(VTSS_FEATURE_VOP)
    out->voe_idx = in->voe_idx;
#endif  // defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_V2)
    out->voi_idx = in->voi_idx;
#endif  // defined(VTSS_FEATURE_VOP_V2)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_XFLOW)
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_conv_vtss_vlan_trans_port2grp_conf_t_to_mesa_vlan_trans_port2grp_conf_t(const vtss_vlan_trans_port2grp_conf_t *in, mesa_vlan_trans_port2grp_conf_t *out) /* ag.rb:1656 */
{
    // skipped: ports@vtss_vlan_trans_port2grp_conf_t
    // skipped: port_list@mesa_vlan_trans_port2grp_conf_t
    out->group_id = in->group_id;

    return mesa_conv2_vtss_vlan_trans_port2grp_conf_t_to_mesa_vlan_trans_port2grp_conf_t(in, out); /* ag.rb:1477 */
}

mesa_rc mesa_conv_mesa_vlan_trans_port2grp_conf_t_to_vtss_vlan_trans_port2grp_conf_t(const mesa_vlan_trans_port2grp_conf_t *in, vtss_vlan_trans_port2grp_conf_t *out) /* ag.rb:1664 */
{
    // skipped: port_list@mesa_vlan_trans_port2grp_conf_t
    // skipped: ports@vtss_vlan_trans_port2grp_conf_t
    out->group_id = in->group_id;

    return mesa_conv2_mesa_vlan_trans_port2grp_conf_t_to_vtss_vlan_trans_port2grp_conf_t(in, out); /* ag.rb:1477 */
}
#endif  // defined(VTSS_FEATURE_VCAP)
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_conv_vtss_mirror_conf_t_to_mesa_mirror_conf_t(const vtss_mirror_conf_t *in, mesa_mirror_conf_t *out) /* ag.rb:1656 */
{
    out->port_no = in->port_no;
    out->fwd_enable = in->fwd_enable;
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    {
        const mesa_mirror_tag_t *tmp = (const mesa_mirror_tag_t *)&in->tag;
#ifdef __cplusplus
        static_assert(sizeof(out.tag) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tag = *tmp;
    }
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    out->vid = in->vid;
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    out->pcp = in->pcp;
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    out->dei = in->dei;
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_mirror_conf_t_to_vtss_mirror_conf_t(const mesa_mirror_conf_t *in, vtss_mirror_conf_t *out) /* ag.rb:1664 */
{
    out->port_no = in->port_no;
    out->fwd_enable = in->fwd_enable;
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    {
        const vtss_mirror_tag_t *tmp = (const vtss_mirror_tag_t *)&in->tag;
#ifdef __cplusplus
        static_assert(sizeof(out.tag) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tag = *tmp;
    }
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    out->vid = in->vid;
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    out->pcp = in->pcp;
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    out->dei = in->dei;
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_LAYER2)

#if defined(VTSS_FEATURE_RCL)
mesa_rc mesa_conv_vtss_rcl_vid_conf_t_to_mesa_rcl_vid_conf_t(const vtss_rcl_vid_conf_t *in, mesa_rcl_vid_conf_t *out) /* ag.rb:1656 */
{
    for (size_t i = 0; i < MESA_PCP_CNT; ++i) {
        out->pcp[i] = in->pcp[i];
    }

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_rcl_vid_conf_t_to_vtss_rcl_vid_conf_t(const mesa_rcl_vid_conf_t *in, vtss_rcl_vid_conf_t *out) /* ag.rb:1664 */
{
    for (size_t i = 0; i < VTSS_PCPS; ++i) {
        out->pcp[i] = in->pcp[i];
    }

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_RCL)

#if defined(VTSS_FEATURE_RCL)
mesa_rc mesa_conv_vtss_rce_action_t_to_mesa_rce_action_t(const vtss_rce_action_t *in, mesa_rce_action_t *out) /* ag.rb:1656 */
{
    out->rtp_id = in->rtp_id;
    out->rtp_sub_id = in->rtp_sub_id;
    out->rtp_inbound = in->rtp_inbound;
    out->port_enable = in->port_enable;
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(in->port_list, &out->port_list)); /* ag.rb:1432 */
    out->llct_enable = in->llct_enable;
    out->llct_port_no = in->llct_port_no;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_rce_action_t_to_vtss_rce_action_t(const mesa_rce_action_t *in, vtss_rce_action_t *out) /* ag.rb:1664 */
{
    out->rtp_id = in->rtp_id;
    out->rtp_sub_id = in->rtp_sub_id;
    out->rtp_inbound = in->rtp_inbound;
    out->port_enable = in->port_enable;
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(&in->port_list, out->port_list)); /* ag.rb:1432 */
    out->llct_enable = in->llct_enable;
    out->llct_port_no = in->llct_port_no;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_RCL)

#if defined(VTSS_FEATURE_RCL)
mesa_rc mesa_conv_vtss_rce_t_to_mesa_rce_t(const vtss_rce_t *in, mesa_rce_t *out) /* ag.rb:1656 */
{
    out->id = in->id;
    {
        const mesa_rce_key_t *tmp = (const mesa_rce_key_t *)&in->key;
#ifdef __cplusplus
        static_assert(sizeof(out.key) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->key = *tmp;
    }
    __RC(mesa_conv_vtss_rce_action_t_to_mesa_rce_action_t(&in->action, &out->action)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_rce_t_to_vtss_rce_t(const mesa_rce_t *in, vtss_rce_t *out) /* ag.rb:1664 */
{
    out->id = in->id;
    {
        const vtss_rce_key_t *tmp = (const vtss_rce_key_t *)&in->key;
#ifdef __cplusplus
        static_assert(sizeof(out.key) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->key = *tmp;
    }
    __RC(mesa_conv_mesa_rce_action_t_to_vtss_rce_action_t(&in->action, &out->action)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_RCL)

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_conv_vtss_rb_cap_t_to_mesa_rb_cap_t(const vtss_rb_cap_t *in, mesa_rb_cap_t *out) /* ag.rb:1656 */
{
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(in->port_list, &out->port_list)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_rb_cap_t_to_vtss_rb_cap_t(const mesa_rb_cap_t *in, vtss_rb_cap_t *out) /* ag.rb:1664 */
{
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(&in->port_list, out->port_list)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_REDBOX)

#if VTSS_OPT_DEBUG_PRINT
mesa_rc mesa_conv_vtss_debug_info_t_to_mesa_debug_info_t(const vtss_debug_info_t *in, mesa_debug_info_t *out) /* ag.rb:1656 */
{
    // skipped: prm@vtss_debug_info_t
    {
        const mesa_debug_layer_t *tmp = (const mesa_debug_layer_t *)&in->layer;
#ifdef __cplusplus
        static_assert(sizeof(out.layer) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->layer = *tmp;
    }
    {
        const mesa_debug_group_t *tmp = (const mesa_debug_group_t *)&in->group;
#ifdef __cplusplus
        static_assert(sizeof(out.group) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->group = *tmp;
    }
    out->chip_no = in->chip_no;
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(in->port_list, &out->port_list)); /* ag.rb:1432 */
    out->full = in->full;
    out->clear = in->clear;
    out->vml_format = in->vml_format;
    out->has_action = in->has_action;
    out->action = in->action;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_debug_info_t_to_vtss_debug_info_t(const mesa_debug_info_t *in, vtss_debug_info_t *out) /* ag.rb:1664 */
{
    // skipped: prm@vtss_debug_info_t
    {
        const vtss_debug_layer_t *tmp = (const vtss_debug_layer_t *)&in->layer;
#ifdef __cplusplus
        static_assert(sizeof(out.layer) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->layer = *tmp;
    }
    {
        const vtss_debug_group_t *tmp = (const vtss_debug_group_t *)&in->group;
#ifdef __cplusplus
        static_assert(sizeof(out.group) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->group = *tmp;
    }
    out->chip_no = in->chip_no;
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(&in->port_list, out->port_list)); /* ag.rb:1432 */
    out->full = in->full;
    out->clear = in->clear;
    out->vml_format = in->vml_format;
    out->has_action = in->has_action;
    out->action = in->action;

    return VTSS_RC_OK;
}
#endif  // VTSS_OPT_DEBUG_PRINT

#if defined(VTSS_FEATURE_INTERRUPTS)
mesa_rc mesa_conv_vtss_intr_t_to_mesa_intr_t(const vtss_intr_t *in, mesa_intr_t *out) /* ag.rb:1656 */
{
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(in->link_change, &out->link_change)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_intr_t_to_vtss_intr_t(const mesa_intr_t *in, vtss_intr_t *out) /* ag.rb:1664 */
{
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(&in->link_change, out->link_change)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_INTERRUPTS)

#ifdef VTSS_FEATURE_IRQ_CONTROL
mesa_rc mesa_conv_vtss_irq_t_to_mesa_irq_t(const vtss_irq_t *in, mesa_irq_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        // skipped: MESA_IRQ_PUSH_BUTTON
        case VTSS_IRQ_XTR:
            *out = MESA_IRQ_XTR;
            return VTSS_RC_OK;
        case VTSS_IRQ_FDMA_XTR:
            *out = MESA_IRQ_FDMA_XTR;
            return VTSS_RC_OK;
        case VTSS_IRQ_SOFTWARE:
            *out = MESA_IRQ_SOFTWARE;
            return VTSS_RC_OK;
        case VTSS_IRQ_PTP_RDY:
            *out = MESA_IRQ_PTP_RDY;
            return VTSS_RC_OK;
        case VTSS_IRQ_PTP_SYNC:
            *out = MESA_IRQ_PTP_SYNC;
            return VTSS_RC_OK;
        case VTSS_IRQ_EXT0:
            *out = MESA_IRQ_EXT0;
            return VTSS_RC_OK;
        case VTSS_IRQ_EXT1:
            *out = MESA_IRQ_EXT1;
            return VTSS_RC_OK;
        case VTSS_IRQ_OAM:
            *out = MESA_IRQ_OAM;
            return VTSS_RC_OK;
        case VTSS_IRQ_SGPIO:
            *out = MESA_IRQ_SGPIO;
            return VTSS_RC_OK;
        case VTSS_IRQ_SGPIO2:
            *out = MESA_IRQ_SGPIO2;
            return VTSS_RC_OK;
        case VTSS_IRQ_DPLL:
            *out = MESA_IRQ_DPLL;
            return VTSS_RC_OK;
        case VTSS_IRQ_GPIO:
            *out = MESA_IRQ_GPIO;
            return VTSS_RC_OK;
        case VTSS_IRQ_DEV_ALL:
            *out = MESA_IRQ_DEV_ALL;
            return VTSS_RC_OK;
        case VTSS_IRQ_CU_PHY_0:
            *out = MESA_IRQ_CU_PHY_0;
            return VTSS_RC_OK;
        case VTSS_IRQ_CU_PHY_1:
            *out = MESA_IRQ_CU_PHY_1;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_0:
            *out = MESA_IRQ_KR_SD10G_0;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_1:
            *out = MESA_IRQ_KR_SD10G_1;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_2:
            *out = MESA_IRQ_KR_SD10G_2;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_3:
            *out = MESA_IRQ_KR_SD10G_3;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_4:
            *out = MESA_IRQ_KR_SD10G_4;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_5:
            *out = MESA_IRQ_KR_SD10G_5;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_6:
            *out = MESA_IRQ_KR_SD10G_6;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_7:
            *out = MESA_IRQ_KR_SD10G_7;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_8:
            *out = MESA_IRQ_KR_SD10G_8;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_9:
            *out = MESA_IRQ_KR_SD10G_9;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_10:
            *out = MESA_IRQ_KR_SD10G_10;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_11:
            *out = MESA_IRQ_KR_SD10G_11;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_12:
            *out = MESA_IRQ_KR_SD10G_12;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_13:
            *out = MESA_IRQ_KR_SD10G_13;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_14:
            *out = MESA_IRQ_KR_SD10G_14;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_15:
            *out = MESA_IRQ_KR_SD10G_15;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_16:
            *out = MESA_IRQ_KR_SD10G_16;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_17:
            *out = MESA_IRQ_KR_SD10G_17;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_18:
            *out = MESA_IRQ_KR_SD10G_18;
            return VTSS_RC_OK;
        case VTSS_IRQ_KR_SD10G_19:
            *out = MESA_IRQ_KR_SD10G_19;
            return VTSS_RC_OK;
        case VTSS_IRQ_MAX:
            *out = MESA_IRQ_MAX;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_irq_t_to_vtss_irq_t(const mesa_irq_t *in, vtss_irq_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        // skipped: MESA_IRQ_PUSH_BUTTON
        case MESA_IRQ_XTR:
            *out = VTSS_IRQ_XTR;
            return VTSS_RC_OK;
        case MESA_IRQ_FDMA_XTR:
            *out = VTSS_IRQ_FDMA_XTR;
            return VTSS_RC_OK;
        case MESA_IRQ_SOFTWARE:
            *out = VTSS_IRQ_SOFTWARE;
            return VTSS_RC_OK;
        case MESA_IRQ_PTP_RDY:
            *out = VTSS_IRQ_PTP_RDY;
            return VTSS_RC_OK;
        case MESA_IRQ_PTP_SYNC:
            *out = VTSS_IRQ_PTP_SYNC;
            return VTSS_RC_OK;
        case MESA_IRQ_EXT0:
            *out = VTSS_IRQ_EXT0;
            return VTSS_RC_OK;
        case MESA_IRQ_EXT1:
            *out = VTSS_IRQ_EXT1;
            return VTSS_RC_OK;
        case MESA_IRQ_OAM:
            *out = VTSS_IRQ_OAM;
            return VTSS_RC_OK;
        case MESA_IRQ_SGPIO:
            *out = VTSS_IRQ_SGPIO;
            return VTSS_RC_OK;
        case MESA_IRQ_SGPIO2:
            *out = VTSS_IRQ_SGPIO2;
            return VTSS_RC_OK;
        case MESA_IRQ_DPLL:
            *out = VTSS_IRQ_DPLL;
            return VTSS_RC_OK;
        case MESA_IRQ_GPIO:
            *out = VTSS_IRQ_GPIO;
            return VTSS_RC_OK;
        case MESA_IRQ_DEV_ALL:
            *out = VTSS_IRQ_DEV_ALL;
            return VTSS_RC_OK;
        case MESA_IRQ_CU_PHY_0:
            *out = VTSS_IRQ_CU_PHY_0;
            return VTSS_RC_OK;
        case MESA_IRQ_CU_PHY_1:
            *out = VTSS_IRQ_CU_PHY_1;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_0:
            *out = VTSS_IRQ_KR_SD10G_0;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_1:
            *out = VTSS_IRQ_KR_SD10G_1;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_2:
            *out = VTSS_IRQ_KR_SD10G_2;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_3:
            *out = VTSS_IRQ_KR_SD10G_3;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_4:
            *out = VTSS_IRQ_KR_SD10G_4;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_5:
            *out = VTSS_IRQ_KR_SD10G_5;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_6:
            *out = VTSS_IRQ_KR_SD10G_6;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_7:
            *out = VTSS_IRQ_KR_SD10G_7;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_8:
            *out = VTSS_IRQ_KR_SD10G_8;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_9:
            *out = VTSS_IRQ_KR_SD10G_9;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_10:
            *out = VTSS_IRQ_KR_SD10G_10;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_11:
            *out = VTSS_IRQ_KR_SD10G_11;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_12:
            *out = VTSS_IRQ_KR_SD10G_12;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_13:
            *out = VTSS_IRQ_KR_SD10G_13;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_14:
            *out = VTSS_IRQ_KR_SD10G_14;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_15:
            *out = VTSS_IRQ_KR_SD10G_15;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_16:
            *out = VTSS_IRQ_KR_SD10G_16;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_17:
            *out = VTSS_IRQ_KR_SD10G_17;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_18:
            *out = VTSS_IRQ_KR_SD10G_18;
            return VTSS_RC_OK;
        case MESA_IRQ_KR_SD10G_19:
            *out = VTSS_IRQ_KR_SD10G_19;
            return VTSS_RC_OK;
        case MESA_IRQ_MAX:
            *out = VTSS_IRQ_MAX;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}
#endif  // VTSS_FEATURE_IRQ_CONTROL

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_conv_vtss_packet_rx_queue_conf_t_to_mesa_packet_rx_queue_conf_t(const vtss_packet_rx_queue_conf_t *in, mesa_packet_rx_queue_conf_t *out) /* ag.rb:1656 */
{
    // skipped: grp_map@mesa_packet_rx_queue_conf_t
    out->size = in->size;
    {
        const mesa_packet_rx_queue_npi_conf_t *tmp = (const mesa_packet_rx_queue_npi_conf_t *)&in->npi;
#ifdef __cplusplus
        static_assert(sizeof(out.npi) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->npi = *tmp;
    }
#if defined(VTSS_FEATURE_QOS_CPU_QUEUE_SHAPER)
    out->rate = in->rate;
#endif  // defined(VTSS_FEATURE_QOS_CPU_QUEUE_SHAPER)

    return mesa_conv2_vtss_packet_rx_queue_conf_t_to_mesa_packet_rx_queue_conf_t(in, out); /* ag.rb:1477 */
}

mesa_rc mesa_conv_mesa_packet_rx_queue_conf_t_to_vtss_packet_rx_queue_conf_t(const mesa_packet_rx_queue_conf_t *in, vtss_packet_rx_queue_conf_t *out) /* ag.rb:1664 */
{
    // skipped: grp_map@mesa_packet_rx_queue_conf_t
    out->size = in->size;
    {
        const vtss_packet_rx_queue_npi_conf_t *tmp = (const vtss_packet_rx_queue_npi_conf_t *)&in->npi;
#ifdef __cplusplus
        static_assert(sizeof(out.npi) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->npi = *tmp;
    }
#if defined(VTSS_FEATURE_QOS_CPU_QUEUE_SHAPER)
    out->rate = in->rate;
#endif  // defined(VTSS_FEATURE_QOS_CPU_QUEUE_SHAPER)

    return mesa_conv2_mesa_packet_rx_queue_conf_t_to_vtss_packet_rx_queue_conf_t(in, out); /* ag.rb:1477 */
}
#endif  // defined(VTSS_FEATURE_PACKET)

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_conv_vtss_packet_rx_queue_map_t_to_mesa_packet_rx_queue_map_t(const vtss_packet_rx_queue_map_t *in, mesa_packet_rx_queue_map_t *out) /* ag.rb:1656 */
{
    out->bpdu_queue = in->bpdu_queue;
    out->garp_queue = in->garp_queue;
    out->learn_queue = in->learn_queue;
    out->igmp_queue = in->igmp_queue;
    out->ipmc_ctrl_queue = in->ipmc_ctrl_queue;
    out->mac_vid_queue = in->mac_vid_queue;
    out->sflow_queue = in->sflow_queue;
    out->lrn_all_queue = in->lrn_all_queue;
#if defined(VTSS_FEATURE_LAYER3)
    out->l3_uc_queue = in->l3_uc_queue;
#endif  // defined(VTSS_FEATURE_LAYER3)
#if defined(VTSS_FEATURE_LAYER3)
    out->l3_other_queue = in->l3_other_queue;
#endif  // defined(VTSS_FEATURE_LAYER3)
#if defined(VTSS_FEATURE_REDBOX)
    out->sv_queue = in->sv_queue;
#endif  // defined(VTSS_FEATURE_REDBOX)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_packet_rx_queue_map_t_to_vtss_packet_rx_queue_map_t(const mesa_packet_rx_queue_map_t *in, vtss_packet_rx_queue_map_t *out) /* ag.rb:1664 */
{
    out->bpdu_queue = in->bpdu_queue;
    out->garp_queue = in->garp_queue;
    out->learn_queue = in->learn_queue;
    out->igmp_queue = in->igmp_queue;
    out->ipmc_ctrl_queue = in->ipmc_ctrl_queue;
    out->mac_vid_queue = in->mac_vid_queue;
    out->sflow_queue = in->sflow_queue;
    out->lrn_all_queue = in->lrn_all_queue;
#if defined(VTSS_FEATURE_LAYER3)
    out->l3_uc_queue = in->l3_uc_queue;
#endif  // defined(VTSS_FEATURE_LAYER3)
#if defined(VTSS_FEATURE_LAYER3)
    out->l3_other_queue = in->l3_other_queue;
#endif  // defined(VTSS_FEATURE_LAYER3)
#if defined(VTSS_FEATURE_REDBOX)
    out->sv_queue = in->sv_queue;
#endif  // defined(VTSS_FEATURE_REDBOX)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_conv_vtss_packet_rx_conf_t_to_mesa_packet_rx_conf_t(const vtss_packet_rx_conf_t *in, mesa_packet_rx_conf_t *out) /* ag.rb:1656 */
{
    // skipped: grp_map@vtss_packet_rx_conf_t
    for (size_t i = 0; i < MESA_PACKET_RX_QUEUE_CNT; ++i) {
        __RC(mesa_conv_vtss_packet_rx_queue_conf_t_to_mesa_packet_rx_queue_conf_t(&in->queue[i], &out->queue[i])); /* ag.rb:1432 */
    }
    {
        const mesa_packet_rx_reg_t *tmp = (const mesa_packet_rx_reg_t *)&in->reg;
#ifdef __cplusplus
        static_assert(sizeof(out.reg) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->reg = *tmp;
    }
    __RC(mesa_conv_vtss_packet_rx_queue_map_t_to_mesa_packet_rx_queue_map_t(&in->map, &out->map)); /* ag.rb:1432 */
#if defined(VTSS_FEATURE_QOS_CPU_PORT_SHAPER)
    out->shaper_rate = in->shaper_rate;
#endif  // defined(VTSS_FEATURE_QOS_CPU_PORT_SHAPER)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_packet_rx_conf_t_to_vtss_packet_rx_conf_t(const mesa_packet_rx_conf_t *in, vtss_packet_rx_conf_t *out) /* ag.rb:1664 */
{
    // skipped: grp_map@vtss_packet_rx_conf_t
    for (size_t i = 0; i < VTSS_PACKET_RX_QUEUE_CNT; ++i) {
        __RC(mesa_conv_mesa_packet_rx_queue_conf_t_to_vtss_packet_rx_queue_conf_t(&in->queue[i], &out->queue[i])); /* ag.rb:1432 */
    }
    {
        const vtss_packet_rx_reg_t *tmp = (const vtss_packet_rx_reg_t *)&in->reg;
#ifdef __cplusplus
        static_assert(sizeof(out.reg) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->reg = *tmp;
    }
    __RC(mesa_conv_mesa_packet_rx_queue_map_t_to_vtss_packet_rx_queue_map_t(&in->map, &out->map)); /* ag.rb:1432 */
#if defined(VTSS_FEATURE_QOS_CPU_PORT_SHAPER)
    out->shaper_rate = in->shaper_rate;
#endif  // defined(VTSS_FEATURE_QOS_CPU_PORT_SHAPER)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_conv_vtss_packet_tx_info_t_to_mesa_packet_tx_info_t(const vtss_packet_tx_info_t *in, mesa_packet_tx_info_t *out) /* ag.rb:1656 */
{
    out->switch_frm = in->switch_frm;
    out->dst_port_mask = in->dst_port_mask;
    out->dst_port = in->dst_port;
    {
        const mesa_vlan_tag_t *tmp = (const mesa_vlan_tag_t *)&in->tag;
#ifdef __cplusplus
        static_assert(sizeof(out.tag) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tag = *tmp;
    }
    out->cos = in->cos;
    out->cosid = in->cosid;
    out->dp = in->dp;
    {
        const mesa_packet_ptp_action_t *tmp = (const mesa_packet_ptp_action_t *)&in->ptp_action;
#ifdef __cplusplus
        static_assert(sizeof(out.ptp_action) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ptp_action = *tmp;
    }
    out->ptp_domain = in->ptp_domain;
    out->ptp_id = in->ptp_id;
    out->ptp_timestamp = in->ptp_timestamp;
    {
        const mesa_packet_oam_type_t *tmp = (const mesa_packet_oam_type_t *)&in->oam_type;
#ifdef __cplusplus
        static_assert(sizeof(out.oam_type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->oam_type = *tmp;
    }
#if defined VTSS_FEATURE_PACKET_INJ_ENCAP
    {
        const mesa_packet_inj_encap_t *tmp = (const mesa_packet_inj_encap_t *)&in->inj_encap;
#ifdef __cplusplus
        static_assert(sizeof(out.inj_encap) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->inj_encap = *tmp;
    }
#endif  // defined VTSS_FEATURE_PACKET_INJ_ENCAP
    out->iflow_id = in->iflow_id;
    out->masquerade_port = in->masquerade_port;
    out->pdu_offset = in->pdu_offset;
    out->sequence_idx = in->sequence_idx;
#if defined(VTSS_FEATURE_AFI_SWC)
    out->afi_id = in->afi_id;
#endif  // defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_FEATURE_PACKET_PIPELINE_PT)
    {
        const mesa_packet_pipeline_pt_t *tmp = (const mesa_packet_pipeline_pt_t *)&in->pipeline_pt;
#ifdef __cplusplus
        static_assert(sizeof(out.pipeline_pt) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->pipeline_pt = *tmp;
    }
#endif  // defined(VTSS_FEATURE_PACKET_PIPELINE_PT)
    out->rb_tag_disable = in->rb_tag_disable;
    out->rb_dd_disable = in->rb_dd_disable;
    out->rb_ring_netid_enable = in->rb_ring_netid_enable;
    {
        const mesa_packet_rb_fwd_t *tmp = (const mesa_packet_rb_fwd_t *)&in->rb_fwd;
#ifdef __cplusplus
        static_assert(sizeof(out.rb_fwd) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rb_fwd = *tmp;
    }

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_packet_tx_info_t_to_vtss_packet_tx_info_t(const mesa_packet_tx_info_t *in, vtss_packet_tx_info_t *out) /* ag.rb:1664 */
{
    out->switch_frm = in->switch_frm;
    out->dst_port_mask = in->dst_port_mask;
    out->dst_port = in->dst_port;
    {
        const vtss_vlan_tag_t *tmp = (const vtss_vlan_tag_t *)&in->tag;
#ifdef __cplusplus
        static_assert(sizeof(out.tag) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tag = *tmp;
    }
    out->cos = in->cos;
    out->cosid = in->cosid;
    out->dp = in->dp;
    {
        const vtss_packet_ptp_action_t *tmp = (const vtss_packet_ptp_action_t *)&in->ptp_action;
#ifdef __cplusplus
        static_assert(sizeof(out.ptp_action) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ptp_action = *tmp;
    }
    out->ptp_domain = in->ptp_domain;
    out->ptp_id = in->ptp_id;
    out->ptp_timestamp = in->ptp_timestamp;
    {
        const vtss_packet_oam_type_t *tmp = (const vtss_packet_oam_type_t *)&in->oam_type;
#ifdef __cplusplus
        static_assert(sizeof(out.oam_type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->oam_type = *tmp;
    }
#if defined VTSS_FEATURE_PACKET_INJ_ENCAP
    {
        const vtss_packet_inj_encap_t *tmp = (const vtss_packet_inj_encap_t *)&in->inj_encap;
#ifdef __cplusplus
        static_assert(sizeof(out.inj_encap) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->inj_encap = *tmp;
    }
#endif  // defined VTSS_FEATURE_PACKET_INJ_ENCAP
    out->iflow_id = in->iflow_id;
    out->masquerade_port = in->masquerade_port;
    out->pdu_offset = in->pdu_offset;
    out->sequence_idx = in->sequence_idx;
#if defined(VTSS_FEATURE_AFI_SWC)
    out->afi_id = in->afi_id;
#endif  // defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_FEATURE_PACKET_PIPELINE_PT)
    {
        const vtss_packet_pipeline_pt_t *tmp = (const vtss_packet_pipeline_pt_t *)&in->pipeline_pt;
#ifdef __cplusplus
        static_assert(sizeof(out.pipeline_pt) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->pipeline_pt = *tmp;
    }
#endif  // defined(VTSS_FEATURE_PACKET_PIPELINE_PT)
    out->rb_tag_disable = in->rb_tag_disable;
    out->rb_dd_disable = in->rb_dd_disable;
    out->rb_ring_netid_enable = in->rb_ring_netid_enable;
    {
        const vtss_packet_rb_fwd_t *tmp = (const vtss_packet_rb_fwd_t *)&in->rb_fwd;
#ifdef __cplusplus
        static_assert(sizeof(out.rb_fwd) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rb_fwd = *tmp;
    }

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PACKET)

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_vtss_port_sgpio_map_t_to_mesa_port_sgpio_map_t(const vtss_port_sgpio_map_t *in, mesa_port_sgpio_map_t *out) /* ag.rb:1656 */
{
    {
        const mesa_sd_sgpio_action_t *tmp = (const mesa_sd_sgpio_action_t *)&in->action;
#ifdef __cplusplus
        static_assert(sizeof(out.action) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->action = *tmp;
    }
#if defined(VTSS_FEATURE_SERIAL_GPIO)
    out->group = in->group;
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO)
    out->port = in->port;
    out->bit = in->bit;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_port_sgpio_map_t_to_vtss_port_sgpio_map_t(const mesa_port_sgpio_map_t *in, vtss_port_sgpio_map_t *out) /* ag.rb:1664 */
{
    {
        const vtss_sd_sgpio_action_t *tmp = (const vtss_sd_sgpio_action_t *)&in->action;
#ifdef __cplusplus
        static_assert(sizeof(out.action) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->action = *tmp;
    }
#if defined(VTSS_FEATURE_SERIAL_GPIO)
    out->group = in->group;
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO)
    out->port = in->port;
    out->bit = in->bit;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_vtss_port_map_t_to_mesa_port_map_t(const vtss_port_map_t *in, mesa_port_map_t *out) /* ag.rb:1656 */
{
    out->chip_port = in->chip_port;
    out->chip_no = in->chip_no;
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    {
        const mesa_internal_bw_t *tmp = (const mesa_internal_bw_t *)&in->max_bw;
#ifdef __cplusplus
        static_assert(sizeof(out.max_bw) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->max_bw = *tmp;
    }
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    __RC(mesa_conv_vtss_miim_controller_t_to_mesa_miim_controller_t(&in->miim_controller, &out->miim_controller)); /* ag.rb:1432 */
    out->miim_addr = in->miim_addr;
    out->miim_chip_no = in->miim_chip_no;
#if defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    __RC(mesa_conv_vtss_port_sgpio_map_t_to_mesa_port_sgpio_map_t(&in->sd_map, &out->sd_map)); /* ag.rb:1432 */
#endif  // defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_port_map_t_to_vtss_port_map_t(const mesa_port_map_t *in, vtss_port_map_t *out) /* ag.rb:1664 */
{
    out->chip_port = in->chip_port;
    out->chip_no = in->chip_no;
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    {
        const vtss_internal_bw_t *tmp = (const vtss_internal_bw_t *)&in->max_bw;
#ifdef __cplusplus
        static_assert(sizeof(out.max_bw) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->max_bw = *tmp;
    }
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    __RC(mesa_conv_mesa_miim_controller_t_to_vtss_miim_controller_t(&in->miim_controller, &out->miim_controller)); /* ag.rb:1432 */
    out->miim_addr = in->miim_addr;
    out->miim_chip_no = in->miim_chip_no;
#if defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    __RC(mesa_conv_mesa_port_sgpio_map_t_to_vtss_port_sgpio_map_t(&in->sd_map, &out->sd_map)); /* ag.rb:1432 */
#endif  // defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_vtss_port_clause_37_remote_fault_t_to_mesa_port_clause_37_remote_fault_t(const vtss_port_clause_37_remote_fault_t *in, mesa_port_clause_37_remote_fault_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        case VTSS_PORT_CLAUSE_37_RF_LINK_OK:
            *out = MESA_PORT_CLAUSE_37_RF_LINK_OK;
            return VTSS_RC_OK;
        case VTSS_PORT_CLAUSE_37_RF_OFFLINE:
            *out = MESA_PORT_CLAUSE_37_RF_OFFLINE;
            return VTSS_RC_OK;
        case VTSS_PORT_CLAUSE_37_RF_LINK_FAILURE:
            *out = MESA_PORT_CLAUSE_37_RF_LINK_FAILURE;
            return VTSS_RC_OK;
        case VTSS_PORT_CLAUSE_37_RF_AUTONEG_ERROR:
            *out = MESA_PORT_CLAUSE_37_RF_AUTONEG_ERROR;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_port_clause_37_remote_fault_t_to_vtss_port_clause_37_remote_fault_t(const mesa_port_clause_37_remote_fault_t *in, vtss_port_clause_37_remote_fault_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        case MESA_PORT_CLAUSE_37_RF_LINK_OK:
            *out = VTSS_PORT_CLAUSE_37_RF_LINK_OK;
            return VTSS_RC_OK;
        case MESA_PORT_CLAUSE_37_RF_OFFLINE:
            *out = VTSS_PORT_CLAUSE_37_RF_OFFLINE;
            return VTSS_RC_OK;
        case MESA_PORT_CLAUSE_37_RF_LINK_FAILURE:
            *out = VTSS_PORT_CLAUSE_37_RF_LINK_FAILURE;
            return VTSS_RC_OK;
        case MESA_PORT_CLAUSE_37_RF_AUTONEG_ERROR:
            *out = VTSS_PORT_CLAUSE_37_RF_AUTONEG_ERROR;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_vtss_port_clause_37_adv_t_to_mesa_port_clause_37_adv_t(const vtss_port_clause_37_adv_t *in, mesa_port_clause_37_adv_t *out) /* ag.rb:1656 */
{
    out->fdx = in->fdx;
    out->hdx = in->hdx;
    out->symmetric_pause = in->symmetric_pause;
    out->asymmetric_pause = in->asymmetric_pause;
    __RC(mesa_conv_vtss_port_clause_37_remote_fault_t_to_mesa_port_clause_37_remote_fault_t(&in->remote_fault, &out->remote_fault)); /* ag.rb:1432 */
    out->acknowledge = in->acknowledge;
    out->next_page = in->next_page;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_port_clause_37_adv_t_to_vtss_port_clause_37_adv_t(const mesa_port_clause_37_adv_t *in, vtss_port_clause_37_adv_t *out) /* ag.rb:1664 */
{
    out->fdx = in->fdx;
    out->hdx = in->hdx;
    out->symmetric_pause = in->symmetric_pause;
    out->asymmetric_pause = in->asymmetric_pause;
    __RC(mesa_conv_mesa_port_clause_37_remote_fault_t_to_vtss_port_clause_37_remote_fault_t(&in->remote_fault, &out->remote_fault)); /* ag.rb:1432 */
    out->acknowledge = in->acknowledge;
    out->next_page = in->next_page;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_vtss_port_clause_37_control_t_to_mesa_port_clause_37_control_t(const vtss_port_clause_37_control_t *in, mesa_port_clause_37_control_t *out) /* ag.rb:1656 */
{
    out->enable = in->enable;
    __RC(mesa_conv_vtss_port_clause_37_adv_t_to_mesa_port_clause_37_adv_t(&in->advertisement, &out->advertisement)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_port_clause_37_control_t_to_vtss_port_clause_37_control_t(const mesa_port_clause_37_control_t *in, vtss_port_clause_37_control_t *out) /* ag.rb:1664 */
{
    out->enable = in->enable;
    __RC(mesa_conv_mesa_port_clause_37_adv_t_to_vtss_port_clause_37_adv_t(&in->advertisement, &out->advertisement)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_vtss_port_flow_control_conf_t_to_mesa_port_flow_control_conf_t(const vtss_port_flow_control_conf_t *in, mesa_port_flow_control_conf_t *out) /* ag.rb:1656 */
{
    out->obey = in->obey;
    out->generate = in->generate;
    {
        const mesa_mac_t *tmp = (const mesa_mac_t *)&in->smac;
#ifdef __cplusplus
        static_assert(sizeof(out.smac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->smac = *tmp;
    }
#if defined(VTSS_FEATURE_PFC)
    for (size_t i = 0; i < MESA_PRIO_ARRAY_SIZE; ++i) {
        out->pfc[i] = in->pfc[i];
    }
#endif  // defined(VTSS_FEATURE_PFC)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_port_flow_control_conf_t_to_vtss_port_flow_control_conf_t(const mesa_port_flow_control_conf_t *in, vtss_port_flow_control_conf_t *out) /* ag.rb:1664 */
{
    out->obey = in->obey;
    out->generate = in->generate;
    {
        const vtss_mac_t *tmp = (const vtss_mac_t *)&in->smac;
#ifdef __cplusplus
        static_assert(sizeof(out.smac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->smac = *tmp;
    }
#if defined(VTSS_FEATURE_PFC)
    for (size_t i = 0; i < VTSS_PRIOS; ++i) {
        out->pfc[i] = in->pfc[i];
    }
#endif  // defined(VTSS_FEATURE_PFC)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_conv_vtss_port_conf_t_to_mesa_port_conf_t(const vtss_port_conf_t *in, mesa_port_conf_t *out) /* ag.rb:1656 */
{
    __RC(mesa_conv_vtss_port_interface_t_to_mesa_port_interface_t(&in->if_type, &out->if_type)); /* ag.rb:1432 */
    out->sd_enable = in->sd_enable;
    out->sd_active_high = in->sd_active_high;
    out->sd_internal = in->sd_internal;
    {
        const mesa_port_frame_gaps_t *tmp = (const mesa_port_frame_gaps_t *)&in->frame_gaps;
#ifdef __cplusplus
        static_assert(sizeof(out.frame_gaps) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->frame_gaps = *tmp;
    }
    out->power_down = in->power_down;
    {
        const mesa_port_speed_t *tmp = (const mesa_port_speed_t *)&in->speed;
#ifdef __cplusplus
        static_assert(sizeof(out.speed) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->speed = *tmp;
    }
    out->fdx = in->fdx;
    __RC(mesa_conv_vtss_port_flow_control_conf_t_to_mesa_port_flow_control_conf_t(&in->flow_control, &out->flow_control)); /* ag.rb:1432 */
    out->max_frame_length = in->max_frame_length;
    out->frame_length_chk = in->frame_length_chk;
    {
        const mesa_port_max_tags_t *tmp = (const mesa_port_max_tags_t *)&in->max_tags;
#ifdef __cplusplus
        static_assert(sizeof(out.max_tags) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->max_tags = *tmp;
    }
    out->exc_col_cont = in->exc_col_cont;
    out->xaui_rx_lane_flip = in->xaui_rx_lane_flip;
    out->xaui_tx_lane_flip = in->xaui_tx_lane_flip;
    {
        const mesa_port_loop_t *tmp = (const mesa_port_loop_t *)&in->loop;
#ifdef __cplusplus
        static_assert(sizeof(out.loop) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->loop = *tmp;
    }
    {
        const mesa_port_serdes_conf_t *tmp = (const mesa_port_serdes_conf_t *)&in->serdes;
#ifdef __cplusplus
        static_assert(sizeof(out.serdes) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->serdes = *tmp;
    }
#if defined(VTSS_FEATURE_PORT_PCS_CONF)
    {
        const mesa_port_pcs_conf_t *tmp = (const mesa_port_pcs_conf_t *)&in->pcs;
#ifdef __cplusplus
        static_assert(sizeof(out.pcs) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->pcs = *tmp;
    }
#endif  // defined(VTSS_FEATURE_PORT_PCS_CONF)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_port_conf_t_to_vtss_port_conf_t(const mesa_port_conf_t *in, vtss_port_conf_t *out) /* ag.rb:1664 */
{
    __RC(mesa_conv_mesa_port_interface_t_to_vtss_port_interface_t(&in->if_type, &out->if_type)); /* ag.rb:1432 */
    out->sd_enable = in->sd_enable;
    out->sd_active_high = in->sd_active_high;
    out->sd_internal = in->sd_internal;
    {
        const vtss_port_frame_gaps_t *tmp = (const vtss_port_frame_gaps_t *)&in->frame_gaps;
#ifdef __cplusplus
        static_assert(sizeof(out.frame_gaps) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->frame_gaps = *tmp;
    }
    out->power_down = in->power_down;
    {
        const vtss_port_speed_t *tmp = (const vtss_port_speed_t *)&in->speed;
#ifdef __cplusplus
        static_assert(sizeof(out.speed) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->speed = *tmp;
    }
    out->fdx = in->fdx;
    __RC(mesa_conv_mesa_port_flow_control_conf_t_to_vtss_port_flow_control_conf_t(&in->flow_control, &out->flow_control)); /* ag.rb:1432 */
    out->max_frame_length = in->max_frame_length;
    out->frame_length_chk = in->frame_length_chk;
    {
        const vtss_port_max_tags_t *tmp = (const vtss_port_max_tags_t *)&in->max_tags;
#ifdef __cplusplus
        static_assert(sizeof(out.max_tags) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->max_tags = *tmp;
    }
    out->exc_col_cont = in->exc_col_cont;
    out->xaui_rx_lane_flip = in->xaui_rx_lane_flip;
    out->xaui_tx_lane_flip = in->xaui_tx_lane_flip;
    {
        const vtss_port_loop_t *tmp = (const vtss_port_loop_t *)&in->loop;
#ifdef __cplusplus
        static_assert(sizeof(out.loop) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->loop = *tmp;
    }
    {
        const vtss_port_serdes_conf_t *tmp = (const vtss_port_serdes_conf_t *)&in->serdes;
#ifdef __cplusplus
        static_assert(sizeof(out.serdes) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->serdes = *tmp;
    }
#if defined(VTSS_FEATURE_PORT_PCS_CONF)
    {
        const vtss_port_pcs_conf_t *tmp = (const vtss_port_pcs_conf_t *)&in->pcs;
#ifdef __cplusplus
        static_assert(sizeof(out.pcs) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->pcs = *tmp;
    }
#endif  // defined(VTSS_FEATURE_PORT_PCS_CONF)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)

mesa_rc mesa_conv_vtss_port_ethernet_like_counters_t_to_mesa_port_ethernet_like_counters_t(const vtss_port_ethernet_like_counters_t *in, mesa_port_ethernet_like_counters_t *out) /* ag.rb:1656 */
{
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsAlignmentErrors = in->dot3StatsAlignmentErrors;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsFCSErrors = in->dot3StatsFCSErrors;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsFrameTooLongs = in->dot3StatsFrameTooLongs;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsSymbolErrors = in->dot3StatsSymbolErrors;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3ControlInUnknownOpcodes = in->dot3ControlInUnknownOpcodes;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3InPauseFrames = in->dot3InPauseFrames;
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsSingleCollisionFrames = in->dot3StatsSingleCollisionFrames;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsMultipleCollisionFrames = in->dot3StatsMultipleCollisionFrames;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsDeferredTransmissions = in->dot3StatsDeferredTransmissions;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsLateCollisions = in->dot3StatsLateCollisions;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsExcessiveCollisions = in->dot3StatsExcessiveCollisions;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsCarrierSenseErrors = in->dot3StatsCarrierSenseErrors;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3OutPauseFrames = in->dot3OutPauseFrames;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_port_ethernet_like_counters_t_to_vtss_port_ethernet_like_counters_t(const mesa_port_ethernet_like_counters_t *in, vtss_port_ethernet_like_counters_t *out) /* ag.rb:1664 */
{
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsAlignmentErrors = in->dot3StatsAlignmentErrors;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsFCSErrors = in->dot3StatsFCSErrors;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsFrameTooLongs = in->dot3StatsFrameTooLongs;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsSymbolErrors = in->dot3StatsSymbolErrors;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3ControlInUnknownOpcodes = in->dot3ControlInUnknownOpcodes;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3InPauseFrames = in->dot3InPauseFrames;
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsSingleCollisionFrames = in->dot3StatsSingleCollisionFrames;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsMultipleCollisionFrames = in->dot3StatsMultipleCollisionFrames;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsDeferredTransmissions = in->dot3StatsDeferredTransmissions;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsLateCollisions = in->dot3StatsLateCollisions;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsExcessiveCollisions = in->dot3StatsExcessiveCollisions;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
#if defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3StatsCarrierSenseErrors = in->dot3StatsCarrierSenseErrors;
#endif  // defined(VTSS_FEATURE_PORT_CNT_ETHER_LIKE)
    out->dot3OutPauseFrames = in->dot3OutPauseFrames;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_vtss_port_dot3br_counters_t_to_mesa_port_dot3br_counters_t(const vtss_port_dot3br_counters_t *in, mesa_port_dot3br_counters_t *out) /* ag.rb:1656 */
{
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
    out->aMACMergeFrameAssErrorCount = in->aMACMergeFrameAssErrorCount;
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
    out->aMACMergeFrameSmdErrorCount = in->aMACMergeFrameSmdErrorCount;
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
    out->aMACMergeFrameAssOkCount = in->aMACMergeFrameAssOkCount;
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
    out->aMACMergeFragCountRx = in->aMACMergeFragCountRx;
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
    out->aMACMergeFragCountTx = in->aMACMergeFragCountTx;
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
    out->aMACMergeHoldCount = in->aMACMergeHoldCount;
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_port_dot3br_counters_t_to_vtss_port_dot3br_counters_t(const mesa_port_dot3br_counters_t *in, vtss_port_dot3br_counters_t *out) /* ag.rb:1664 */
{
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
    out->aMACMergeFrameAssErrorCount = in->aMACMergeFrameAssErrorCount;
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
    out->aMACMergeFrameSmdErrorCount = in->aMACMergeFrameSmdErrorCount;
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
    out->aMACMergeFrameAssOkCount = in->aMACMergeFrameAssOkCount;
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
    out->aMACMergeFragCountRx = in->aMACMergeFragCountRx;
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
    out->aMACMergeFragCountTx = in->aMACMergeFragCountTx;
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
    out->aMACMergeHoldCount = in->aMACMergeHoldCount;
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_vtss_port_counters_t_to_mesa_port_counters_t(const vtss_port_counters_t *in, mesa_port_counters_t *out) /* ag.rb:1656 */
{
    // skipped: prop@vtss_port_counters_t
    // skipped: evc@vtss_port_counters_t
    // skipped: prio@mesa_port_counters_t
    {
        const mesa_port_rmon_counters_t *tmp = (const mesa_port_rmon_counters_t *)&in->rmon;
#ifdef __cplusplus
        static_assert(sizeof(out.rmon) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rmon = *tmp;
    }
    {
        const mesa_port_if_group_counters_t *tmp = (const mesa_port_if_group_counters_t *)&in->if_group;
#ifdef __cplusplus
        static_assert(sizeof(out.if_group) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->if_group = *tmp;
    }
    __RC(mesa_conv_vtss_port_ethernet_like_counters_t_to_mesa_port_ethernet_like_counters_t(&in->ethernet_like, &out->ethernet_like)); /* ag.rb:1432 */
#if defined(VTSS_FEATURE_PORT_CNT_BRIDGE)
    {
        const mesa_port_bridge_counters_t *tmp = (const mesa_port_bridge_counters_t *)&in->bridge;
#ifdef __cplusplus
        static_assert(sizeof(out.bridge) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->bridge = *tmp;
    }
#endif  // defined(VTSS_FEATURE_PORT_CNT_BRIDGE)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
    __RC(mesa_conv_vtss_port_dot3br_counters_t_to_mesa_port_dot3br_counters_t(&in->dot3br, &out->dot3br)); /* ag.rb:1432 */
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)

    return mesa_conv2_vtss_port_counters_t_to_mesa_port_counters_t(in, out); /* ag.rb:1477 */
}

mesa_rc mesa_conv_mesa_port_counters_t_to_vtss_port_counters_t(const mesa_port_counters_t *in, vtss_port_counters_t *out) /* ag.rb:1664 */
{
    // skipped: prio@mesa_port_counters_t
    // skipped: prop@vtss_port_counters_t
    // skipped: evc@vtss_port_counters_t
    {
        const vtss_port_rmon_counters_t *tmp = (const vtss_port_rmon_counters_t *)&in->rmon;
#ifdef __cplusplus
        static_assert(sizeof(out.rmon) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rmon = *tmp;
    }
    {
        const vtss_port_if_group_counters_t *tmp = (const vtss_port_if_group_counters_t *)&in->if_group;
#ifdef __cplusplus
        static_assert(sizeof(out.if_group) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->if_group = *tmp;
    }
    __RC(mesa_conv_mesa_port_ethernet_like_counters_t_to_vtss_port_ethernet_like_counters_t(&in->ethernet_like, &out->ethernet_like)); /* ag.rb:1432 */
#if defined(VTSS_FEATURE_PORT_CNT_BRIDGE)
    {
        const vtss_port_bridge_counters_t *tmp = (const vtss_port_bridge_counters_t *)&in->bridge;
#ifdef __cplusplus
        static_assert(sizeof(out.bridge) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->bridge = *tmp;
    }
#endif  // defined(VTSS_FEATURE_PORT_CNT_BRIDGE)
#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
    __RC(mesa_conv_mesa_port_dot3br_counters_t_to_vtss_port_dot3br_counters_t(&in->dot3br, &out->dot3br)); /* ag.rb:1432 */
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)

    return mesa_conv2_mesa_port_counters_t_to_vtss_port_counters_t(in, out); /* ag.rb:1477 */
}

#if defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_IFH)
mesa_rc mesa_conv_vtss_port_ifh_t_to_mesa_port_ifh_t(const vtss_port_ifh_t *in, mesa_port_ifh_t *out) /* ag.rb:1656 */
{
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
    out->ena_inj_header = in->ena_inj_header;
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_JAGUAR_2)
    out->ena_inj_header = in->ena_inj_header;
#endif  // defined(VTSS_ARCH_JAGUAR_2)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
    out->ena_xtr_header = in->ena_xtr_header;
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_JAGUAR_2)
    out->ena_xtr_header = in->ena_xtr_header;
#endif  // defined(VTSS_ARCH_JAGUAR_2)
#if defined(VTSS_ARCH_JAGUAR_2)
    out->ena_ifh_header = in->ena_ifh_header;
#endif  // defined(VTSS_ARCH_JAGUAR_2)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_port_ifh_t_to_vtss_port_ifh_t(const mesa_port_ifh_t *in, vtss_port_ifh_t *out) /* ag.rb:1664 */
{
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
    out->ena_inj_header = in->ena_inj_header;
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_JAGUAR_2)
    out->ena_inj_header = in->ena_inj_header;
#endif  // defined(VTSS_ARCH_JAGUAR_2)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
    out->ena_xtr_header = in->ena_xtr_header;
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_JAGUAR_2)
    out->ena_xtr_header = in->ena_xtr_header;
#endif  // defined(VTSS_ARCH_JAGUAR_2)
#if defined(VTSS_ARCH_JAGUAR_2)
    out->ena_ifh_header = in->ena_ifh_header;
#endif  // defined(VTSS_ARCH_JAGUAR_2)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_PORT_IFH)
#endif  // defined(VTSS_FEATURE_PORT_CONTROL)

#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv_vtss_dscp_emode_t_to_mesa_dscp_emode_t(const vtss_dscp_emode_t *in, mesa_dscp_emode_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        case VTSS_DSCP_EMODE_DISABLE:
            *out = MESA_DSCP_EMODE_DISABLE;
            return VTSS_RC_OK;
        case VTSS_DSCP_EMODE_REMARK:
            *out = MESA_DSCP_EMODE_REMARK;
            return VTSS_RC_OK;
        case VTSS_DSCP_EMODE_REMAP:
            *out = MESA_DSCP_EMODE_REMAP;
            return VTSS_RC_OK;
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE)
        case VTSS_DSCP_EMODE_REMAP_DPA:
            *out = MESA_DSCP_EMODE_REMAP_DPA;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE)
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_dscp_emode_t_to_vtss_dscp_emode_t(const mesa_dscp_emode_t *in, vtss_dscp_emode_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        case MESA_DSCP_EMODE_DISABLE:
            *out = VTSS_DSCP_EMODE_DISABLE;
            return VTSS_RC_OK;
        case MESA_DSCP_EMODE_REMARK:
            *out = VTSS_DSCP_EMODE_REMARK;
            return VTSS_RC_OK;
        case MESA_DSCP_EMODE_REMAP:
            *out = VTSS_DSCP_EMODE_REMAP;
            return VTSS_RC_OK;
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE)
        case MESA_DSCP_EMODE_REMAP_DPA:
            *out = VTSS_DSCP_EMODE_REMAP_DPA;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE)
        default:
            return VTSS_RC_ERROR;
    }
}
#endif  // defined(VTSS_FEATURE_QOS)

#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv_vtss_tag_remark_mode_t_to_mesa_tag_remark_mode_t(const vtss_tag_remark_mode_t *in, mesa_tag_remark_mode_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        case VTSS_TAG_REMARK_MODE_CLASSIFIED:
            *out = MESA_TAG_REMARK_MODE_CLASSIFIED;
            return VTSS_RC_OK;
        case VTSS_TAG_REMARK_MODE_DEFAULT:
            *out = MESA_TAG_REMARK_MODE_DEFAULT;
            return VTSS_RC_OK;
        case VTSS_TAG_REMARK_MODE_MAPPED:
            *out = MESA_TAG_REMARK_MODE_MAPPED;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_tag_remark_mode_t_to_vtss_tag_remark_mode_t(const mesa_tag_remark_mode_t *in, vtss_tag_remark_mode_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        case MESA_TAG_REMARK_MODE_CLASSIFIED:
            *out = VTSS_TAG_REMARK_MODE_CLASSIFIED;
            return VTSS_RC_OK;
        case MESA_TAG_REMARK_MODE_DEFAULT:
            *out = VTSS_TAG_REMARK_MODE_DEFAULT;
            return VTSS_RC_OK;
        case MESA_TAG_REMARK_MODE_MAPPED:
            *out = VTSS_TAG_REMARK_MODE_MAPPED;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}
#endif  // defined(VTSS_FEATURE_QOS)

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_RT)
mesa_rc mesa_conv_vtss_shaper_mode_t_to_mesa_shaper_mode_t(const vtss_shaper_mode_t *in, mesa_shaper_mode_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        case VTSS_SHAPER_MODE_LINE:
            *out = MESA_SHAPER_MODE_LINE;
            return VTSS_RC_OK;
        case VTSS_SHAPER_MODE_DATA:
            *out = MESA_SHAPER_MODE_DATA;
            return VTSS_RC_OK;
#if defined(VTSS_FEATURE_QOS_EGRESS_SHAPER_FRAME)
        case VTSS_SHAPER_MODE_FRAME:
            *out = MESA_SHAPER_MODE_FRAME;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_SHAPER_FRAME)
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_shaper_mode_t_to_vtss_shaper_mode_t(const mesa_shaper_mode_t *in, vtss_shaper_mode_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        case MESA_SHAPER_MODE_LINE:
            *out = VTSS_SHAPER_MODE_LINE;
            return VTSS_RC_OK;
        case MESA_SHAPER_MODE_DATA:
            *out = VTSS_SHAPER_MODE_DATA;
            return VTSS_RC_OK;
#if defined(VTSS_FEATURE_QOS_EGRESS_SHAPER_FRAME)
        case MESA_SHAPER_MODE_FRAME:
            *out = VTSS_SHAPER_MODE_FRAME;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_SHAPER_FRAME)
        default:
            return VTSS_RC_ERROR;
    }
}
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_RT)
#endif  // defined(VTSS_FEATURE_QOS)

#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv_vtss_shaper_t_to_mesa_shaper_t(const vtss_shaper_t *in, mesa_shaper_t *out) /* ag.rb:1656 */
{
    out->level = in->level;
    out->rate = in->rate;
#if defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_DLB)
    out->ebs = in->ebs;
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_DLB)
#if defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_DLB)
    out->eir = in->eir;
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_DLB)
#if defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_RT)
    __RC(mesa_conv_vtss_shaper_mode_t_to_mesa_shaper_mode_t(&in->mode, &out->mode)); /* ag.rb:1432 */
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_RT)
#if defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS_CRB)
    out->credit_enable = in->credit_enable;
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS_CRB)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_shaper_t_to_vtss_shaper_t(const mesa_shaper_t *in, vtss_shaper_t *out) /* ag.rb:1664 */
{
    out->level = in->level;
    out->rate = in->rate;
#if defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_DLB)
    out->ebs = in->ebs;
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_DLB)
#if defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_DLB)
    out->eir = in->eir;
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_DLB)
#if defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_RT)
    __RC(mesa_conv_mesa_shaper_mode_t_to_vtss_shaper_mode_t(&in->mode, &out->mode)); /* ag.rb:1432 */
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_SHAPERS_RT)
#if defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS_CRB)
    out->credit_enable = in->credit_enable;
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS_CRB)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS)

#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_conv_vtss_qos_port_conf_t_to_mesa_qos_port_conf_t(const vtss_qos_port_conf_t *in, mesa_qos_port_conf_t *out) /* ag.rb:1656 */
{
    // skipped: policer_port@vtss_qos_port_conf_t
    // skipped: policer_ext_port@vtss_qos_port_conf_t
    // skipped: policer_queue@vtss_qos_port_conf_t
    // skipped: shaper_port@vtss_qos_port_conf_t
    // skipped: shaper_queue@vtss_qos_port_conf_t
    // skipped: excess_enable@vtss_qos_port_conf_t
    // skipped: cut_through_enable@vtss_qos_port_conf_t
    // skipped: usr_prio@vtss_qos_port_conf_t
    // skipped: default_dei@vtss_qos_port_conf_t
    // skipped: tag_class_enable@vtss_qos_port_conf_t
    // skipped: qos_class_map@vtss_qos_port_conf_t
    // skipped: dp_level_map@vtss_qos_port_conf_t
    // skipped: dscp_class_enable@vtss_qos_port_conf_t
    // skipped: dscp_mode@vtss_qos_port_conf_t
    // skipped: dscp_emode@vtss_qos_port_conf_t
    // skipped: dscp_translate@vtss_qos_port_conf_t
    // skipped: tag_remark_mode@vtss_qos_port_conf_t
    // skipped: tag_default_pcp@vtss_qos_port_conf_t
    // skipped: tag_default_dei@vtss_qos_port_conf_t
    // skipped: tag_pcp_map@vtss_qos_port_conf_t
    // skipped: tag_dei_map@vtss_qos_port_conf_t
    // skipped: queue_pct@vtss_qos_port_conf_t
    // skipped: ot_queue_pct@vtss_qos_port_conf_t
    // skipped: ot_shaper_queue@vtss_qos_port_conf_t
    // skipped: shaper@mesa_qos_port_conf_t
    // skipped: tag@mesa_qos_port_conf_t
    // skipped: dscp@mesa_qos_port_conf_t
    // skipped: queue@mesa_qos_port_conf_t
    // skipped: ot_queue@mesa_qos_port_conf_t
    out->default_prio = in->default_prio;
    out->default_dpl = in->default_dpl;
    out->dwrr_enable = in->dwrr_enable;
#if defined(VTSS_FEATURE_QOS_SCHEDULER_DWRR_CNT)
    out->dwrr_cnt = in->dwrr_cnt;
#endif  // defined(VTSS_FEATURE_QOS_SCHEDULER_DWRR_CNT)
#if defined(VTSS_FEATURE_QOS_OT)
    out->ot_dwrr_enable = in->ot_dwrr_enable;
#endif  // defined(VTSS_FEATURE_QOS_OT)
#if defined(VTSS_FEATURE_QOS_OT)
    out->ot_dwrr_cnt = in->ot_dwrr_cnt;
#endif  // defined(VTSS_FEATURE_QOS_OT)
#if defined(VTSS_FEATURE_QOS_OT)
    out->ot_it_dwrr_enable = in->ot_it_dwrr_enable;
#endif  // defined(VTSS_FEATURE_QOS_OT)
#if defined(VTSS_FEATURE_QOS_OT)
    out->ot_pct = in->ot_pct;
#endif  // defined(VTSS_FEATURE_QOS_OT)
#if defined(VTSS_FEATURE_QOS_OT)
    __RC(mesa_conv_vtss_shaper_t_to_mesa_shaper_t(&in->ot_shaper, &out->ot_shaper)); /* ag.rb:1432 */
#endif  // defined(VTSS_FEATURE_QOS_OT)
#if defined(VTSS_FEATURE_QOS_OT)
    __RC(mesa_conv_vtss_shaper_t_to_mesa_shaper_t(&in->it_shaper, &out->it_shaper)); /* ag.rb:1432 */
#endif  // defined(VTSS_FEATURE_QOS_OT)
#if defined(VTSS_FEATURE_QCL_DMAC_DIP)
    out->dmac_dip = in->dmac_dip;
#endif  // defined(VTSS_FEATURE_QCL_DMAC_DIP)
#if defined(VTSS_FEATURE_QCL_KEY_TYPE)
    {
        const mesa_vcap_key_type_t *tmp = (const mesa_vcap_key_type_t *)&in->key_type;
#ifdef __cplusplus
        static_assert(sizeof(out.key_type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->key_type = *tmp;
    }
#endif  // defined(VTSS_FEATURE_QCL_KEY_TYPE)
#if defined(VTSS_FEATURE_QOS_WRED_V3)
    out->wred_group = in->wred_group;
#endif  // defined(VTSS_FEATURE_QOS_WRED_V3)
#if (defined VTSS_FEATURE_QOS_COSID_CLASSIFICATION)
    out->cosid = in->cosid;
#endif  // (defined VTSS_FEATURE_QOS_COSID_CLASSIFICATION)
#if (defined VTSS_FEATURE_QOS_INGRESS_MAP)
    out->ingress_map = in->ingress_map;
#endif  // (defined VTSS_FEATURE_QOS_INGRESS_MAP)
#if (defined VTSS_FEATURE_QOS_EGRESS_MAP)
    out->egress_map = in->egress_map;
#endif  // (defined VTSS_FEATURE_QOS_EGRESS_MAP)

    return mesa_conv2_vtss_qos_port_conf_t_to_mesa_qos_port_conf_t(in, out); /* ag.rb:1477 */
}

mesa_rc mesa_conv_mesa_qos_port_conf_t_to_vtss_qos_port_conf_t(const mesa_qos_port_conf_t *in, vtss_qos_port_conf_t *out) /* ag.rb:1664 */
{
    // skipped: shaper@mesa_qos_port_conf_t
    // skipped: tag@mesa_qos_port_conf_t
    // skipped: dscp@mesa_qos_port_conf_t
    // skipped: queue@mesa_qos_port_conf_t
    // skipped: ot_queue@mesa_qos_port_conf_t
    // skipped: policer_port@vtss_qos_port_conf_t
    // skipped: policer_ext_port@vtss_qos_port_conf_t
    // skipped: policer_queue@vtss_qos_port_conf_t
    // skipped: shaper_port@vtss_qos_port_conf_t
    // skipped: shaper_queue@vtss_qos_port_conf_t
    // skipped: excess_enable@vtss_qos_port_conf_t
    // skipped: cut_through_enable@vtss_qos_port_conf_t
    // skipped: usr_prio@vtss_qos_port_conf_t
    // skipped: default_dei@vtss_qos_port_conf_t
    // skipped: tag_class_enable@vtss_qos_port_conf_t
    // skipped: qos_class_map@vtss_qos_port_conf_t
    // skipped: dp_level_map@vtss_qos_port_conf_t
    // skipped: dscp_class_enable@vtss_qos_port_conf_t
    // skipped: dscp_mode@vtss_qos_port_conf_t
    // skipped: dscp_emode@vtss_qos_port_conf_t
    // skipped: dscp_translate@vtss_qos_port_conf_t
    // skipped: tag_remark_mode@vtss_qos_port_conf_t
    // skipped: tag_default_pcp@vtss_qos_port_conf_t
    // skipped: tag_default_dei@vtss_qos_port_conf_t
    // skipped: tag_pcp_map@vtss_qos_port_conf_t
    // skipped: tag_dei_map@vtss_qos_port_conf_t
    // skipped: queue_pct@vtss_qos_port_conf_t
    // skipped: ot_queue_pct@vtss_qos_port_conf_t
    // skipped: ot_shaper_queue@vtss_qos_port_conf_t
    out->default_prio = in->default_prio;
    out->default_dpl = in->default_dpl;
    out->dwrr_enable = in->dwrr_enable;
#if defined(VTSS_FEATURE_QOS_SCHEDULER_DWRR_CNT)
    out->dwrr_cnt = in->dwrr_cnt;
#endif  // defined(VTSS_FEATURE_QOS_SCHEDULER_DWRR_CNT)
#if defined(VTSS_FEATURE_QCL_DMAC_DIP)
    out->dmac_dip = in->dmac_dip;
#endif  // defined(VTSS_FEATURE_QCL_DMAC_DIP)
#if defined(VTSS_FEATURE_QCL_KEY_TYPE)
    {
        const vtss_vcap_key_type_t *tmp = (const vtss_vcap_key_type_t *)&in->key_type;
#ifdef __cplusplus
        static_assert(sizeof(out.key_type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->key_type = *tmp;
    }
#endif  // defined(VTSS_FEATURE_QCL_KEY_TYPE)
#if defined(VTSS_FEATURE_QOS_WRED_V3)
    out->wred_group = in->wred_group;
#endif  // defined(VTSS_FEATURE_QOS_WRED_V3)
#if (defined VTSS_FEATURE_QOS_COSID_CLASSIFICATION)
    out->cosid = in->cosid;
#endif  // (defined VTSS_FEATURE_QOS_COSID_CLASSIFICATION)
#if (defined VTSS_FEATURE_QOS_INGRESS_MAP)
    out->ingress_map = in->ingress_map;
#endif  // (defined VTSS_FEATURE_QOS_INGRESS_MAP)
#if (defined VTSS_FEATURE_QOS_EGRESS_MAP)
    out->egress_map = in->egress_map;
#endif  // (defined VTSS_FEATURE_QOS_EGRESS_MAP)
#if defined(VTSS_FEATURE_QOS_OT)
    out->ot_dwrr_enable = in->ot_dwrr_enable;
#endif  // defined(VTSS_FEATURE_QOS_OT)
#if defined(VTSS_FEATURE_QOS_OT)
    out->ot_dwrr_cnt = in->ot_dwrr_cnt;
#endif  // defined(VTSS_FEATURE_QOS_OT)
#if defined(VTSS_FEATURE_QOS_OT)
    out->ot_it_dwrr_enable = in->ot_it_dwrr_enable;
#endif  // defined(VTSS_FEATURE_QOS_OT)
#if defined(VTSS_FEATURE_QOS_OT)
    out->ot_pct = in->ot_pct;
#endif  // defined(VTSS_FEATURE_QOS_OT)
#if defined(VTSS_FEATURE_QOS_OT)
    __RC(mesa_conv_mesa_shaper_t_to_vtss_shaper_t(&in->ot_shaper, &out->ot_shaper)); /* ag.rb:1432 */
#endif  // defined(VTSS_FEATURE_QOS_OT)
#if defined(VTSS_FEATURE_QOS_OT)
    __RC(mesa_conv_mesa_shaper_t_to_vtss_shaper_t(&in->it_shaper, &out->it_shaper)); /* ag.rb:1432 */
#endif  // defined(VTSS_FEATURE_QOS_OT)

    return mesa_conv2_mesa_qos_port_conf_t_to_vtss_qos_port_conf_t(in, out); /* ag.rb:1477 */
}
#endif  // defined(VTSS_FEATURE_QOS)

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_vtss_qce_mac_t_to_mesa_qce_mac_t(const vtss_qce_mac_t *in, mesa_qce_mac_t *out) /* ag.rb:1656 */
{
    {
        const mesa_vcap_bit_t *tmp = (const mesa_vcap_bit_t *)&in->dmac_mc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_mc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_mc = *tmp;
    }
    {
        const mesa_vcap_bit_t *tmp = (const mesa_vcap_bit_t *)&in->dmac_bc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_bc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_bc = *tmp;
    }
#if defined(VTSS_FEATURE_QCL_KEY_DMAC)
    {
        const mesa_vcap_u48_t *tmp = (const mesa_vcap_u48_t *)&in->dmac;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac = *tmp;
    }
#endif  // defined(VTSS_FEATURE_QCL_KEY_DMAC)
    {
        const mesa_vcap_u48_t *tmp = (const mesa_vcap_u48_t *)&in->smac;
#ifdef __cplusplus
        static_assert(sizeof(out.smac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->smac = *tmp;
    }

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_qce_mac_t_to_vtss_qce_mac_t(const mesa_qce_mac_t *in, vtss_qce_mac_t *out) /* ag.rb:1664 */
{
    {
        const vtss_vcap_bit_t *tmp = (const vtss_vcap_bit_t *)&in->dmac_mc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_mc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_mc = *tmp;
    }
    {
        const vtss_vcap_bit_t *tmp = (const vtss_vcap_bit_t *)&in->dmac_bc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_bc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_bc = *tmp;
    }
#if defined(VTSS_FEATURE_QCL_KEY_DMAC)
    {
        const vtss_vcap_u48_t *tmp = (const vtss_vcap_u48_t *)&in->dmac;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac = *tmp;
    }
#endif  // defined(VTSS_FEATURE_QCL_KEY_DMAC)
    {
        const vtss_vcap_u48_t *tmp = (const vtss_vcap_u48_t *)&in->smac;
#ifdef __cplusplus
        static_assert(sizeof(out.smac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->smac = *tmp;
    }

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_vtss_qce_frame_ipv4_t_to_mesa_qce_frame_ipv4_t(const vtss_qce_frame_ipv4_t *in, mesa_qce_frame_ipv4_t *out) /* ag.rb:1656 */
{
    {
        const mesa_vcap_bit_t *tmp = (const mesa_vcap_bit_t *)&in->fragment;
#ifdef __cplusplus
        static_assert(sizeof(out.fragment) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->fragment = *tmp;
    }
    {
        const mesa_vcap_vr_t *tmp = (const mesa_vcap_vr_t *)&in->dscp;
#ifdef __cplusplus
        static_assert(sizeof(out.dscp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dscp = *tmp;
    }
    {
        const mesa_vcap_u8_t *tmp = (const mesa_vcap_u8_t *)&in->proto;
#ifdef __cplusplus
        static_assert(sizeof(out.proto) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->proto = *tmp;
    }
    {
        const mesa_vcap_ip_t *tmp = (const mesa_vcap_ip_t *)&in->sip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip = *tmp;
    }
#if defined(VTSS_FEATURE_QCL_KEY_DIP)
    {
        const mesa_vcap_ip_t *tmp = (const mesa_vcap_ip_t *)&in->dip;
#ifdef __cplusplus
        static_assert(sizeof(out.dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dip = *tmp;
    }
#endif  // defined(VTSS_FEATURE_QCL_KEY_DIP)
    {
        const mesa_vcap_vr_t *tmp = (const mesa_vcap_vr_t *)&in->sport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport = *tmp;
    }
    {
        const mesa_vcap_vr_t *tmp = (const mesa_vcap_vr_t *)&in->dport;
#ifdef __cplusplus
        static_assert(sizeof(out.dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dport = *tmp;
    }

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_qce_frame_ipv4_t_to_vtss_qce_frame_ipv4_t(const mesa_qce_frame_ipv4_t *in, vtss_qce_frame_ipv4_t *out) /* ag.rb:1664 */
{
    {
        const vtss_vcap_bit_t *tmp = (const vtss_vcap_bit_t *)&in->fragment;
#ifdef __cplusplus
        static_assert(sizeof(out.fragment) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->fragment = *tmp;
    }
    {
        const vtss_vcap_vr_t *tmp = (const vtss_vcap_vr_t *)&in->dscp;
#ifdef __cplusplus
        static_assert(sizeof(out.dscp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dscp = *tmp;
    }
    {
        const vtss_vcap_u8_t *tmp = (const vtss_vcap_u8_t *)&in->proto;
#ifdef __cplusplus
        static_assert(sizeof(out.proto) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->proto = *tmp;
    }
    {
        const vtss_vcap_ip_t *tmp = (const vtss_vcap_ip_t *)&in->sip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip = *tmp;
    }
#if defined(VTSS_FEATURE_QCL_KEY_DIP)
    {
        const vtss_vcap_ip_t *tmp = (const vtss_vcap_ip_t *)&in->dip;
#ifdef __cplusplus
        static_assert(sizeof(out.dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dip = *tmp;
    }
#endif  // defined(VTSS_FEATURE_QCL_KEY_DIP)
    {
        const vtss_vcap_vr_t *tmp = (const vtss_vcap_vr_t *)&in->sport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport = *tmp;
    }
    {
        const vtss_vcap_vr_t *tmp = (const vtss_vcap_vr_t *)&in->dport;
#ifdef __cplusplus
        static_assert(sizeof(out.dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dport = *tmp;
    }

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_vtss_qce_frame_ipv6_t_to_mesa_qce_frame_ipv6_t(const vtss_qce_frame_ipv6_t *in, mesa_qce_frame_ipv6_t *out) /* ag.rb:1656 */
{
    {
        const mesa_vcap_vr_t *tmp = (const mesa_vcap_vr_t *)&in->dscp;
#ifdef __cplusplus
        static_assert(sizeof(out.dscp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dscp = *tmp;
    }
    {
        const mesa_vcap_u8_t *tmp = (const mesa_vcap_u8_t *)&in->proto;
#ifdef __cplusplus
        static_assert(sizeof(out.proto) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->proto = *tmp;
    }
    {
        const mesa_vcap_u128_t *tmp = (const mesa_vcap_u128_t *)&in->sip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip = *tmp;
    }
#if defined(VTSS_FEATURE_QCL_KEY_DIP)
    {
        const mesa_vcap_u128_t *tmp = (const mesa_vcap_u128_t *)&in->dip;
#ifdef __cplusplus
        static_assert(sizeof(out.dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dip = *tmp;
    }
#endif  // defined(VTSS_FEATURE_QCL_KEY_DIP)
    {
        const mesa_vcap_vr_t *tmp = (const mesa_vcap_vr_t *)&in->sport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport = *tmp;
    }
    {
        const mesa_vcap_vr_t *tmp = (const mesa_vcap_vr_t *)&in->dport;
#ifdef __cplusplus
        static_assert(sizeof(out.dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dport = *tmp;
    }

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_qce_frame_ipv6_t_to_vtss_qce_frame_ipv6_t(const mesa_qce_frame_ipv6_t *in, vtss_qce_frame_ipv6_t *out) /* ag.rb:1664 */
{
    {
        const vtss_vcap_vr_t *tmp = (const vtss_vcap_vr_t *)&in->dscp;
#ifdef __cplusplus
        static_assert(sizeof(out.dscp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dscp = *tmp;
    }
    {
        const vtss_vcap_u8_t *tmp = (const vtss_vcap_u8_t *)&in->proto;
#ifdef __cplusplus
        static_assert(sizeof(out.proto) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->proto = *tmp;
    }
    {
        const vtss_vcap_u128_t *tmp = (const vtss_vcap_u128_t *)&in->sip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip = *tmp;
    }
#if defined(VTSS_FEATURE_QCL_KEY_DIP)
    {
        const vtss_vcap_u128_t *tmp = (const vtss_vcap_u128_t *)&in->dip;
#ifdef __cplusplus
        static_assert(sizeof(out.dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dip = *tmp;
    }
#endif  // defined(VTSS_FEATURE_QCL_KEY_DIP)
    {
        const vtss_vcap_vr_t *tmp = (const vtss_vcap_vr_t *)&in->sport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport = *tmp;
    }
    {
        const vtss_vcap_vr_t *tmp = (const vtss_vcap_vr_t *)&in->dport;
#ifdef __cplusplus
        static_assert(sizeof(out.dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dport = *tmp;
    }

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_vtss_qce_key_t_to_mesa_qce_key_t(const vtss_qce_key_t *in, mesa_qce_key_t *out) /* ag.rb:1656 */
{
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(in->port_list, &out->port_list)); /* ag.rb:1432 */
    __RC(mesa_conv_vtss_qce_mac_t_to_mesa_qce_mac_t(&in->mac, &out->mac)); /* ag.rb:1432 */
    {
        const mesa_qce_tag_t *tmp = (const mesa_qce_tag_t *)&in->tag;
#ifdef __cplusplus
        static_assert(sizeof(out.tag) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tag = *tmp;
    }
#if defined(VTSS_FEATURE_QCL_KEY_INNER_TAG)
    {
        const mesa_qce_tag_t *tmp = (const mesa_qce_tag_t *)&in->inner_tag;
#ifdef __cplusplus
        static_assert(sizeof(out.inner_tag) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->inner_tag = *tmp;
    }
#endif  // defined(VTSS_FEATURE_QCL_KEY_INNER_TAG)
    {
        const mesa_qce_type_t *tmp = (const mesa_qce_type_t *)&in->type;
#ifdef __cplusplus
        static_assert(sizeof(out.type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->type = *tmp;
    }

    return mesa_conv2_vtss_qce_key_t_to_mesa_qce_key_t(in, out); /* ag.rb:1477 */
}

mesa_rc mesa_conv_mesa_qce_key_t_to_vtss_qce_key_t(const mesa_qce_key_t *in, vtss_qce_key_t *out) /* ag.rb:1664 */
{
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(&in->port_list, out->port_list)); /* ag.rb:1432 */
    __RC(mesa_conv_mesa_qce_mac_t_to_vtss_qce_mac_t(&in->mac, &out->mac)); /* ag.rb:1432 */
    {
        const vtss_qce_tag_t *tmp = (const vtss_qce_tag_t *)&in->tag;
#ifdef __cplusplus
        static_assert(sizeof(out.tag) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tag = *tmp;
    }
#if defined(VTSS_FEATURE_QCL_KEY_INNER_TAG)
    {
        const vtss_qce_tag_t *tmp = (const vtss_qce_tag_t *)&in->inner_tag;
#ifdef __cplusplus
        static_assert(sizeof(out.inner_tag) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->inner_tag = *tmp;
    }
#endif  // defined(VTSS_FEATURE_QCL_KEY_INNER_TAG)
    {
        const vtss_qce_type_t *tmp = (const vtss_qce_type_t *)&in->type;
#ifdef __cplusplus
        static_assert(sizeof(out.type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->type = *tmp;
    }

    return mesa_conv2_mesa_qce_key_t_to_vtss_qce_key_t(in, out); /* ag.rb:1477 */
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_vtss_qce_action_t_to_mesa_qce_action_t(const vtss_qce_action_t *in, mesa_qce_action_t *out) /* ag.rb:1656 */
{
    out->prio_enable = in->prio_enable;
    out->prio = in->prio;
    out->dp_enable = in->dp_enable;
    out->dp = in->dp;
    out->dscp_enable = in->dscp_enable;
    out->dscp = in->dscp;
    out->pcp_dei_enable = in->pcp_dei_enable;
    out->pcp = in->pcp;
    out->dei = in->dei;
    out->policy_no_enable = in->policy_no_enable;
    out->policy_no = in->policy_no;
#if (defined VTSS_FEATURE_QCL_MAP_ACTION)
    out->map_id_enable = in->map_id_enable;
#endif  // (defined VTSS_FEATURE_QCL_MAP_ACTION)
#if (defined VTSS_FEATURE_QCL_MAP_ACTION)
    out->map_id = in->map_id;
#endif  // (defined VTSS_FEATURE_QCL_MAP_ACTION)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_qce_action_t_to_vtss_qce_action_t(const mesa_qce_action_t *in, vtss_qce_action_t *out) /* ag.rb:1664 */
{
    out->prio_enable = in->prio_enable;
    out->prio = in->prio;
    out->dp_enable = in->dp_enable;
    out->dp = in->dp;
    out->dscp_enable = in->dscp_enable;
    out->dscp = in->dscp;
    out->pcp_dei_enable = in->pcp_dei_enable;
    out->pcp = in->pcp;
    out->dei = in->dei;
    out->policy_no_enable = in->policy_no_enable;
    out->policy_no = in->policy_no;
#if (defined VTSS_FEATURE_QCL_MAP_ACTION)
    out->map_id_enable = in->map_id_enable;
#endif  // (defined VTSS_FEATURE_QCL_MAP_ACTION)
#if (defined VTSS_FEATURE_QCL_MAP_ACTION)
    out->map_id = in->map_id;
#endif  // (defined VTSS_FEATURE_QCL_MAP_ACTION)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_conv_vtss_qce_t_to_mesa_qce_t(const vtss_qce_t *in, mesa_qce_t *out) /* ag.rb:1656 */
{
    out->id = in->id;
    __RC(mesa_conv_vtss_qce_key_t_to_mesa_qce_key_t(&in->key, &out->key)); /* ag.rb:1432 */
    __RC(mesa_conv_vtss_qce_action_t_to_mesa_qce_action_t(&in->action, &out->action)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_qce_t_to_vtss_qce_t(const mesa_qce_t *in, vtss_qce_t *out) /* ag.rb:1664 */
{
    out->id = in->id;
    __RC(mesa_conv_mesa_qce_key_t_to_vtss_qce_key_t(&in->key, &out->key)); /* ag.rb:1432 */
    __RC(mesa_conv_mesa_qce_action_t_to_vtss_qce_action_t(&in->action, &out->action)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QCL)
#endif  // defined(VTSS_FEATURE_QOS)

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
mesa_rc mesa_conv_vtss_dlb_policer_conf_t_to_mesa_dlb_policer_conf_t(const vtss_dlb_policer_conf_t *in, mesa_dlb_policer_conf_t *out) /* ag.rb:1656 */
{
    {
        const mesa_policer_type_t *tmp = (const mesa_policer_type_t *)&in->type;
#ifdef __cplusplus
        static_assert(sizeof(out.type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->type = *tmp;
    }
    out->enable = in->enable;
#if defined(VTSS_ARCH_JAGUAR_2) || defined (VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    out->cm = in->cm;
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined (VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    out->cf = in->cf;
    out->line_rate = in->line_rate;
    out->cir = in->cir;
    out->cbs = in->cbs;
    out->eir = in->eir;
    out->ebs = in->ebs;
#if defined(VTSS_FEATURE_PSFP)
    out->drop_yellow = in->drop_yellow;
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
    {
        const mesa_opt_bool_t *tmp = (const mesa_opt_bool_t *)&in->mark_all_red;
#ifdef __cplusplus
        static_assert(sizeof(out.mark_all_red) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->mark_all_red = *tmp;
    }
#endif  // defined(VTSS_FEATURE_PSFP)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_dlb_policer_conf_t_to_vtss_dlb_policer_conf_t(const mesa_dlb_policer_conf_t *in, vtss_dlb_policer_conf_t *out) /* ag.rb:1664 */
{
    {
        const vtss_policer_type_t *tmp = (const vtss_policer_type_t *)&in->type;
#ifdef __cplusplus
        static_assert(sizeof(out.type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->type = *tmp;
    }
    out->enable = in->enable;
#if defined(VTSS_ARCH_JAGUAR_2) || defined (VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    out->cm = in->cm;
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined (VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    out->cf = in->cf;
    out->line_rate = in->line_rate;
    out->cir = in->cir;
    out->cbs = in->cbs;
    out->eir = in->eir;
    out->ebs = in->ebs;
#if defined(VTSS_FEATURE_PSFP)
    out->drop_yellow = in->drop_yellow;
#endif  // defined(VTSS_FEATURE_PSFP)
#if defined(VTSS_FEATURE_PSFP)
    {
        const vtss_opt_bool_t *tmp = (const vtss_opt_bool_t *)&in->mark_all_red;
#ifdef __cplusplus
        static_assert(sizeof(out.mark_all_red) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->mark_all_red = *tmp;
    }
#endif  // defined(VTSS_FEATURE_PSFP)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_POLICER_DLB)
#endif  // defined(VTSS_FEATURE_QOS)

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_vtss_acl_ptp_action_t_to_mesa_acl_ptp_action_t(const vtss_acl_ptp_action_t *in, mesa_acl_ptp_action_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        case VTSS_ACL_PTP_ACTION_NONE:
            *out = MESA_ACL_PTP_ACTION_NONE;
            return VTSS_RC_OK;
        case VTSS_ACL_PTP_ACTION_ONE_STEP:
            *out = MESA_ACL_PTP_ACTION_ONE_STEP;
            return VTSS_RC_OK;
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        case VTSS_ACL_PTP_ACTION_ONE_STEP_ADD_DELAY:
            *out = MESA_ACL_PTP_ACTION_ONE_STEP_ADD_DELAY;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        case VTSS_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_1:
            *out = MESA_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_1;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        case VTSS_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_2:
            *out = MESA_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_2;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_LUTON26)
        case VTSS_ACL_PTP_ACTION_ONE_AND_TWO_STEP:
            *out = MESA_ACL_PTP_ACTION_ONE_AND_TWO_STEP;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LUTON26)
        case VTSS_ACL_PTP_ACTION_TWO_STEP:
            *out = MESA_ACL_PTP_ACTION_TWO_STEP;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_acl_ptp_action_t_to_vtss_acl_ptp_action_t(const mesa_acl_ptp_action_t *in, vtss_acl_ptp_action_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        case MESA_ACL_PTP_ACTION_NONE:
            *out = VTSS_ACL_PTP_ACTION_NONE;
            return VTSS_RC_OK;
        case MESA_ACL_PTP_ACTION_ONE_STEP:
            *out = VTSS_ACL_PTP_ACTION_ONE_STEP;
            return VTSS_RC_OK;
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        case MESA_ACL_PTP_ACTION_ONE_STEP_ADD_DELAY:
            *out = VTSS_ACL_PTP_ACTION_ONE_STEP_ADD_DELAY;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        case MESA_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_1:
            *out = VTSS_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_1;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        case MESA_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_2:
            *out = VTSS_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_2;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_LUTON26)
        case MESA_ACL_PTP_ACTION_ONE_AND_TWO_STEP:
            *out = VTSS_ACL_PTP_ACTION_ONE_AND_TWO_STEP;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_LUTON26)
        case MESA_ACL_PTP_ACTION_TWO_STEP:
            *out = VTSS_ACL_PTP_ACTION_TWO_STEP;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}
#endif  // defined(VTSS_FEATURE_ACL)

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_vtss_acl_action_t_to_mesa_acl_action_t(const vtss_acl_action_t *in, mesa_acl_action_t *out) /* ag.rb:1656 */
{
    out->cpu = in->cpu;
    out->cpu_once = in->cpu_once;
    out->cpu_disable = in->cpu_disable;
    out->cpu_queue = in->cpu_queue;
    out->police = in->police;
    out->policer_no = in->policer_no;
#if defined(VTSS_FEATURE_EVC_POLICERS)
    out->evc_police = in->evc_police;
#endif  // defined(VTSS_FEATURE_EVC_POLICERS)
#if defined(VTSS_FEATURE_EVC_POLICERS)
    out->evc_policer_id = in->evc_policer_id;
#endif  // defined(VTSS_FEATURE_EVC_POLICERS)
    out->learn = in->learn;
    {
        const mesa_acl_port_action_t *tmp = (const mesa_acl_port_action_t *)&in->port_action;
#ifdef __cplusplus
        static_assert(sizeof(out.port_action) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->port_action = *tmp;
    }
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(in->port_list, &out->port_list)); /* ag.rb:1432 */
    out->mirror = in->mirror;
    __RC(mesa_conv_vtss_acl_ptp_action_t_to_mesa_acl_ptp_action_t(&in->ptp_action, &out->ptp_action)); /* ag.rb:1432 */
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
    {
        const mesa_acl_ptp_action_conf_t *tmp = (const mesa_acl_ptp_action_conf_t *)&in->ptp;
#ifdef __cplusplus
        static_assert(sizeof(out.ptp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ptp = *tmp;
    }
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    {
        const mesa_acl_addr_action_t *tmp = (const mesa_acl_addr_action_t *)&in->addr;
#ifdef __cplusplus
        static_assert(sizeof(out.addr) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->addr = *tmp;
    }
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_OCELOT)
    out->lm_cnt_disable = in->lm_cnt_disable;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
    out->mac_swap = in->mac_swap;
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
    out->ifh_flag = in->ifh_flag;
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_acl_action_t_to_vtss_acl_action_t(const mesa_acl_action_t *in, vtss_acl_action_t *out) /* ag.rb:1664 */
{
    out->cpu = in->cpu;
    out->cpu_once = in->cpu_once;
    out->cpu_disable = in->cpu_disable;
    out->cpu_queue = in->cpu_queue;
    out->police = in->police;
    out->policer_no = in->policer_no;
#if defined(VTSS_FEATURE_EVC_POLICERS)
    out->evc_police = in->evc_police;
#endif  // defined(VTSS_FEATURE_EVC_POLICERS)
#if defined(VTSS_FEATURE_EVC_POLICERS)
    out->evc_policer_id = in->evc_policer_id;
#endif  // defined(VTSS_FEATURE_EVC_POLICERS)
    out->learn = in->learn;
    {
        const vtss_acl_port_action_t *tmp = (const vtss_acl_port_action_t *)&in->port_action;
#ifdef __cplusplus
        static_assert(sizeof(out.port_action) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->port_action = *tmp;
    }
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(&in->port_list, out->port_list)); /* ag.rb:1432 */
    out->mirror = in->mirror;
    __RC(mesa_conv_mesa_acl_ptp_action_t_to_vtss_acl_ptp_action_t(&in->ptp_action, &out->ptp_action)); /* ag.rb:1432 */
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
    {
        const vtss_acl_ptp_action_conf_t *tmp = (const vtss_acl_ptp_action_conf_t *)&in->ptp;
#ifdef __cplusplus
        static_assert(sizeof(out.ptp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ptp = *tmp;
    }
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    {
        const vtss_acl_addr_action_t *tmp = (const vtss_acl_addr_action_t *)&in->addr;
#ifdef __cplusplus
        static_assert(sizeof(out.addr) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->addr = *tmp;
    }
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_OCELOT)
    out->lm_cnt_disable = in->lm_cnt_disable;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
    out->mac_swap = in->mac_swap;
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
    out->ifh_flag = in->ifh_flag;
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_vtss_acl_port_conf_t_to_mesa_acl_port_conf_t(const vtss_acl_port_conf_t *in, mesa_acl_port_conf_t *out) /* ag.rb:1656 */
{
    out->policy_no = in->policy_no;
    __RC(mesa_conv_vtss_acl_action_t_to_mesa_acl_action_t(&in->action, &out->action)); /* ag.rb:1432 */
    {
        const mesa_acl_frame_key_t *tmp = (const mesa_acl_frame_key_t *)&in->key;
#ifdef __cplusplus
        static_assert(sizeof(out.key) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->key = *tmp;
    }

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_acl_port_conf_t_to_vtss_acl_port_conf_t(const mesa_acl_port_conf_t *in, vtss_acl_port_conf_t *out) /* ag.rb:1664 */
{
    out->policy_no = in->policy_no;
    __RC(mesa_conv_mesa_acl_action_t_to_vtss_acl_action_t(&in->action, &out->action)); /* ag.rb:1432 */
    {
        const vtss_acl_frame_key_t *tmp = (const vtss_acl_frame_key_t *)&in->key;
#ifdef __cplusplus
        static_assert(sizeof(out.key) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->key = *tmp;
    }

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_vtss_ace_frame_ipv4_t_to_mesa_ace_frame_ipv4_t(const vtss_ace_frame_ipv4_t *in, mesa_ace_frame_ipv4_t *out) /* ag.rb:1656 */
{
#if defined(VTSS_FEATURE_ACL_EXT_MAC)
    {
        const mesa_ace_u48_t *tmp = (const mesa_ace_u48_t *)&in->dmac;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac = *tmp;
    }
#endif  // defined(VTSS_FEATURE_ACL_EXT_MAC)
#if defined(VTSS_FEATURE_ACL_EXT_MAC)
    {
        const mesa_ace_u48_t *tmp = (const mesa_ace_u48_t *)&in->smac;
#ifdef __cplusplus
        static_assert(sizeof(out.smac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->smac = *tmp;
    }
#endif  // defined(VTSS_FEATURE_ACL_EXT_MAC)
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->ttl;
#ifdef __cplusplus
        static_assert(sizeof(out.ttl) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ttl = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->fragment;
#ifdef __cplusplus
        static_assert(sizeof(out.fragment) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->fragment = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->options;
#ifdef __cplusplus
        static_assert(sizeof(out.options) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->options = *tmp;
    }
    {
        const mesa_ace_u8_t *tmp = (const mesa_ace_u8_t *)&in->ds;
#ifdef __cplusplus
        static_assert(sizeof(out.ds) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ds = *tmp;
    }
    {
        const mesa_ace_u8_t *tmp = (const mesa_ace_u8_t *)&in->proto;
#ifdef __cplusplus
        static_assert(sizeof(out.proto) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->proto = *tmp;
    }
    {
        const mesa_ace_ip_t *tmp = (const mesa_ace_ip_t *)&in->sip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip = *tmp;
    }
    {
        const mesa_ace_ip_t *tmp = (const mesa_ace_ip_t *)&in->dip;
#ifdef __cplusplus
        static_assert(sizeof(out.dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dip = *tmp;
    }
    {
        const mesa_ace_u48_t *tmp = (const mesa_ace_u48_t *)&in->data;
#ifdef __cplusplus
        static_assert(sizeof(out.data) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->data = *tmp;
    }
    {
        const mesa_ace_udp_tcp_t *tmp = (const mesa_ace_udp_tcp_t *)&in->sport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport = *tmp;
    }
    {
        const mesa_ace_udp_tcp_t *tmp = (const mesa_ace_udp_tcp_t *)&in->dport;
#ifdef __cplusplus
        static_assert(sizeof(out.dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dport = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->tcp_fin;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_fin) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_fin = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->tcp_syn;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_syn) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_syn = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->tcp_rst;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_rst) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_rst = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->tcp_psh;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_psh) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_psh = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->tcp_ack;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_ack) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_ack = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->tcp_urg;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_urg) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_urg = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->sip_eq_dip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip_eq_dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip_eq_dip = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->sport_eq_dport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport_eq_dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport_eq_dport = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->seq_zero;
#ifdef __cplusplus
        static_assert(sizeof(out.seq_zero) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->seq_zero = *tmp;
    }
    {
        const mesa_ace_ptp_t *tmp = (const mesa_ace_ptp_t *)&in->ptp;
#ifdef __cplusplus
        static_assert(sizeof(out.ptp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ptp = *tmp;
    }
    {
        const mesa_ace_sip_smac_t *tmp = (const mesa_ace_sip_smac_t *)&in->sip_smac;
#ifdef __cplusplus
        static_assert(sizeof(out.sip_smac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip_smac = *tmp;
    }

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_ace_frame_ipv4_t_to_vtss_ace_frame_ipv4_t(const mesa_ace_frame_ipv4_t *in, vtss_ace_frame_ipv4_t *out) /* ag.rb:1664 */
{
#if defined(VTSS_FEATURE_ACL_EXT_MAC)
    {
        const vtss_ace_u48_t *tmp = (const vtss_ace_u48_t *)&in->dmac;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac = *tmp;
    }
#endif  // defined(VTSS_FEATURE_ACL_EXT_MAC)
#if defined(VTSS_FEATURE_ACL_EXT_MAC)
    {
        const vtss_ace_u48_t *tmp = (const vtss_ace_u48_t *)&in->smac;
#ifdef __cplusplus
        static_assert(sizeof(out.smac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->smac = *tmp;
    }
#endif  // defined(VTSS_FEATURE_ACL_EXT_MAC)
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->ttl;
#ifdef __cplusplus
        static_assert(sizeof(out.ttl) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ttl = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->fragment;
#ifdef __cplusplus
        static_assert(sizeof(out.fragment) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->fragment = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->options;
#ifdef __cplusplus
        static_assert(sizeof(out.options) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->options = *tmp;
    }
    {
        const vtss_ace_u8_t *tmp = (const vtss_ace_u8_t *)&in->ds;
#ifdef __cplusplus
        static_assert(sizeof(out.ds) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ds = *tmp;
    }
    {
        const vtss_ace_u8_t *tmp = (const vtss_ace_u8_t *)&in->proto;
#ifdef __cplusplus
        static_assert(sizeof(out.proto) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->proto = *tmp;
    }
    {
        const vtss_ace_ip_t *tmp = (const vtss_ace_ip_t *)&in->sip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip = *tmp;
    }
    {
        const vtss_ace_ip_t *tmp = (const vtss_ace_ip_t *)&in->dip;
#ifdef __cplusplus
        static_assert(sizeof(out.dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dip = *tmp;
    }
    {
        const vtss_ace_u48_t *tmp = (const vtss_ace_u48_t *)&in->data;
#ifdef __cplusplus
        static_assert(sizeof(out.data) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->data = *tmp;
    }
    {
        const vtss_ace_udp_tcp_t *tmp = (const vtss_ace_udp_tcp_t *)&in->sport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport = *tmp;
    }
    {
        const vtss_ace_udp_tcp_t *tmp = (const vtss_ace_udp_tcp_t *)&in->dport;
#ifdef __cplusplus
        static_assert(sizeof(out.dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dport = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->tcp_fin;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_fin) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_fin = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->tcp_syn;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_syn) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_syn = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->tcp_rst;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_rst) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_rst = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->tcp_psh;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_psh) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_psh = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->tcp_ack;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_ack) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_ack = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->tcp_urg;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_urg) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_urg = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->sip_eq_dip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip_eq_dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip_eq_dip = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->sport_eq_dport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport_eq_dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport_eq_dport = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->seq_zero;
#ifdef __cplusplus
        static_assert(sizeof(out.seq_zero) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->seq_zero = *tmp;
    }
    {
        const vtss_ace_ptp_t *tmp = (const vtss_ace_ptp_t *)&in->ptp;
#ifdef __cplusplus
        static_assert(sizeof(out.ptp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ptp = *tmp;
    }
    {
        const vtss_ace_sip_smac_t *tmp = (const vtss_ace_sip_smac_t *)&in->sip_smac;
#ifdef __cplusplus
        static_assert(sizeof(out.sip_smac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip_smac = *tmp;
    }

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_vtss_ace_frame_ipv6_t_to_mesa_ace_frame_ipv6_t(const vtss_ace_frame_ipv6_t *in, mesa_ace_frame_ipv6_t *out) /* ag.rb:1656 */
{
#if defined(VTSS_FEATURE_ACL_EXT_MAC)
    {
        const mesa_ace_u48_t *tmp = (const mesa_ace_u48_t *)&in->dmac;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac = *tmp;
    }
#endif  // defined(VTSS_FEATURE_ACL_EXT_MAC)
#if defined(VTSS_FEATURE_ACL_EXT_MAC)
    {
        const mesa_ace_u48_t *tmp = (const mesa_ace_u48_t *)&in->smac;
#ifdef __cplusplus
        static_assert(sizeof(out.smac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->smac = *tmp;
    }
#endif  // defined(VTSS_FEATURE_ACL_EXT_MAC)
    {
        const mesa_ace_u8_t *tmp = (const mesa_ace_u8_t *)&in->proto;
#ifdef __cplusplus
        static_assert(sizeof(out.proto) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->proto = *tmp;
    }
    {
        const mesa_ace_u128_t *tmp = (const mesa_ace_u128_t *)&in->sip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip = *tmp;
    }
#if defined(VTSS_FEATURE_ACL_EXT_DIP)
    {
        const mesa_ace_u128_t *tmp = (const mesa_ace_u128_t *)&in->dip;
#ifdef __cplusplus
        static_assert(sizeof(out.dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dip = *tmp;
    }
#endif  // defined(VTSS_FEATURE_ACL_EXT_DIP)
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->ttl;
#ifdef __cplusplus
        static_assert(sizeof(out.ttl) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ttl = *tmp;
    }
    {
        const mesa_ace_u8_t *tmp = (const mesa_ace_u8_t *)&in->ds;
#ifdef __cplusplus
        static_assert(sizeof(out.ds) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ds = *tmp;
    }
    {
        const mesa_ace_u48_t *tmp = (const mesa_ace_u48_t *)&in->data;
#ifdef __cplusplus
        static_assert(sizeof(out.data) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->data = *tmp;
    }
    {
        const mesa_ace_udp_tcp_t *tmp = (const mesa_ace_udp_tcp_t *)&in->sport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport = *tmp;
    }
    {
        const mesa_ace_udp_tcp_t *tmp = (const mesa_ace_udp_tcp_t *)&in->dport;
#ifdef __cplusplus
        static_assert(sizeof(out.dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dport = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->tcp_fin;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_fin) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_fin = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->tcp_syn;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_syn) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_syn = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->tcp_rst;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_rst) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_rst = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->tcp_psh;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_psh) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_psh = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->tcp_ack;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_ack) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_ack = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->tcp_urg;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_urg) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_urg = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->sip_eq_dip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip_eq_dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip_eq_dip = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->sport_eq_dport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport_eq_dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport_eq_dport = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->seq_zero;
#ifdef __cplusplus
        static_assert(sizeof(out.seq_zero) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->seq_zero = *tmp;
    }
    {
        const mesa_ace_ptp_t *tmp = (const mesa_ace_ptp_t *)&in->ptp;
#ifdef __cplusplus
        static_assert(sizeof(out.ptp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ptp = *tmp;
    }

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_ace_frame_ipv6_t_to_vtss_ace_frame_ipv6_t(const mesa_ace_frame_ipv6_t *in, vtss_ace_frame_ipv6_t *out) /* ag.rb:1664 */
{
#if defined(VTSS_FEATURE_ACL_EXT_MAC)
    {
        const vtss_ace_u48_t *tmp = (const vtss_ace_u48_t *)&in->dmac;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac = *tmp;
    }
#endif  // defined(VTSS_FEATURE_ACL_EXT_MAC)
#if defined(VTSS_FEATURE_ACL_EXT_MAC)
    {
        const vtss_ace_u48_t *tmp = (const vtss_ace_u48_t *)&in->smac;
#ifdef __cplusplus
        static_assert(sizeof(out.smac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->smac = *tmp;
    }
#endif  // defined(VTSS_FEATURE_ACL_EXT_MAC)
    {
        const vtss_ace_u8_t *tmp = (const vtss_ace_u8_t *)&in->proto;
#ifdef __cplusplus
        static_assert(sizeof(out.proto) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->proto = *tmp;
    }
    {
        const vtss_ace_u128_t *tmp = (const vtss_ace_u128_t *)&in->sip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip = *tmp;
    }
#if defined(VTSS_FEATURE_ACL_EXT_DIP)
    {
        const vtss_ace_u128_t *tmp = (const vtss_ace_u128_t *)&in->dip;
#ifdef __cplusplus
        static_assert(sizeof(out.dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dip = *tmp;
    }
#endif  // defined(VTSS_FEATURE_ACL_EXT_DIP)
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->ttl;
#ifdef __cplusplus
        static_assert(sizeof(out.ttl) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ttl = *tmp;
    }
    {
        const vtss_ace_u8_t *tmp = (const vtss_ace_u8_t *)&in->ds;
#ifdef __cplusplus
        static_assert(sizeof(out.ds) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ds = *tmp;
    }
    {
        const vtss_ace_u48_t *tmp = (const vtss_ace_u48_t *)&in->data;
#ifdef __cplusplus
        static_assert(sizeof(out.data) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->data = *tmp;
    }
    {
        const vtss_ace_udp_tcp_t *tmp = (const vtss_ace_udp_tcp_t *)&in->sport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport = *tmp;
    }
    {
        const vtss_ace_udp_tcp_t *tmp = (const vtss_ace_udp_tcp_t *)&in->dport;
#ifdef __cplusplus
        static_assert(sizeof(out.dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dport = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->tcp_fin;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_fin) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_fin = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->tcp_syn;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_syn) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_syn = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->tcp_rst;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_rst) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_rst = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->tcp_psh;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_psh) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_psh = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->tcp_ack;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_ack) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_ack = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->tcp_urg;
#ifdef __cplusplus
        static_assert(sizeof(out.tcp_urg) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tcp_urg = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->sip_eq_dip;
#ifdef __cplusplus
        static_assert(sizeof(out.sip_eq_dip) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sip_eq_dip = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->sport_eq_dport;
#ifdef __cplusplus
        static_assert(sizeof(out.sport_eq_dport) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->sport_eq_dport = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->seq_zero;
#ifdef __cplusplus
        static_assert(sizeof(out.seq_zero) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->seq_zero = *tmp;
    }
    {
        const vtss_ace_ptp_t *tmp = (const vtss_ace_ptp_t *)&in->ptp;
#ifdef __cplusplus
        static_assert(sizeof(out.ptp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ptp = *tmp;
    }

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_ACL)

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_conv_vtss_ace_t_to_mesa_ace_t(const vtss_ace_t *in, mesa_ace_t *out) /* ag.rb:1656 */
{
    out->id = in->id;
#if defined(VTSS_ARCH_OCELOT)
    out->lookup = in->lookup;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_OCELOT)
    out->isdx_enable = in->isdx_enable;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_OCELOT)
    out->isdx_disable = in->isdx_disable;
#endif  // defined(VTSS_ARCH_OCELOT)
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(in->port_list, &out->port_list)); /* ag.rb:1432 */
    {
        const mesa_ace_u8_t *tmp = (const mesa_ace_u8_t *)&in->policy;
#ifdef __cplusplus
        static_assert(sizeof(out.policy) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->policy = *tmp;
    }
    {
        const mesa_ace_type_t *tmp = (const mesa_ace_type_t *)&in->type;
#ifdef __cplusplus
        static_assert(sizeof(out.type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->type = *tmp;
    }
    out->type_ext = in->type_ext;
    __RC(mesa_conv_vtss_acl_action_t_to_mesa_acl_action_t(&in->action, &out->action)); /* ag.rb:1432 */
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->dmac_mc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_mc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_mc = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->dmac_bc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_bc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_bc = *tmp;
    }
    {
        const mesa_ace_vlan_t *tmp = (const mesa_ace_vlan_t *)&in->vlan;
#ifdef __cplusplus
        static_assert(sizeof(out.vlan) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->vlan = *tmp;
    }

    return mesa_conv2_vtss_ace_t_to_mesa_ace_t(in, out); /* ag.rb:1477 */
}

mesa_rc mesa_conv_mesa_ace_t_to_vtss_ace_t(const mesa_ace_t *in, vtss_ace_t *out) /* ag.rb:1664 */
{
    out->id = in->id;
#if defined(VTSS_ARCH_OCELOT)
    out->lookup = in->lookup;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_OCELOT)
    out->isdx_enable = in->isdx_enable;
#endif  // defined(VTSS_ARCH_OCELOT)
#if defined(VTSS_ARCH_OCELOT)
    out->isdx_disable = in->isdx_disable;
#endif  // defined(VTSS_ARCH_OCELOT)
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(&in->port_list, out->port_list)); /* ag.rb:1432 */
    {
        const vtss_ace_u8_t *tmp = (const vtss_ace_u8_t *)&in->policy;
#ifdef __cplusplus
        static_assert(sizeof(out.policy) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->policy = *tmp;
    }
    {
        const vtss_ace_type_t *tmp = (const vtss_ace_type_t *)&in->type;
#ifdef __cplusplus
        static_assert(sizeof(out.type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->type = *tmp;
    }
    out->type_ext = in->type_ext;
    __RC(mesa_conv_mesa_acl_action_t_to_vtss_acl_action_t(&in->action, &out->action)); /* ag.rb:1432 */
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->dmac_mc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_mc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_mc = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->dmac_bc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_bc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_bc = *tmp;
    }
    {
        const vtss_ace_vlan_t *tmp = (const vtss_ace_vlan_t *)&in->vlan;
#ifdef __cplusplus
        static_assert(sizeof(out.vlan) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->vlan = *tmp;
    }

    return mesa_conv2_mesa_ace_t_to_vtss_ace_t(in, out); /* ag.rb:1477 */
}
#endif  // defined(VTSS_FEATURE_ACL)

#if defined(VTSS_FEATURE_HACL)
mesa_rc mesa_conv_vtss_hace_key_t_to_mesa_hace_key_t(const vtss_hace_key_t *in, mesa_hace_key_t *out) /* ag.rb:1656 */
{
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(in->port_list, &out->port_list)); /* ag.rb:1432 */
    {
        const mesa_rleg_list_t *tmp = (const mesa_rleg_list_t *)&in->rleg_list;
#ifdef __cplusplus
        static_assert(sizeof(out.rleg_list) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rleg_list = *tmp;
    }
    {
        const mesa_ace_u8_t *tmp = (const mesa_ace_u8_t *)&in->policy;
#ifdef __cplusplus
        static_assert(sizeof(out.policy) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->policy = *tmp;
    }
    out->type_ext = in->type_ext;
    {
        const mesa_ace_type_t *tmp = (const mesa_ace_type_t *)&in->type;
#ifdef __cplusplus
        static_assert(sizeof(out.type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->type = *tmp;
    }
    {
        const mesa_ace_vlan_t *tmp = (const mesa_ace_vlan_t *)&in->vlan;
#ifdef __cplusplus
        static_assert(sizeof(out.vlan) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->vlan = *tmp;
    }
    {
        const mesa_ace_ptp_t *tmp = (const mesa_ace_ptp_t *)&in->ptp;
#ifdef __cplusplus
        static_assert(sizeof(out.ptp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ptp = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->dmac_mc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_mc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_mc = *tmp;
    }
    {
        const mesa_ace_bit_t *tmp = (const mesa_ace_bit_t *)&in->dmac_bc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_bc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_bc = *tmp;
    }
    {
        const mesa_ace_u48_t *tmp = (const mesa_ace_u48_t *)&in->dmac;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac = *tmp;
    }
    {
        const mesa_ace_u48_t *tmp = (const mesa_ace_u48_t *)&in->smac;
#ifdef __cplusplus
        static_assert(sizeof(out.smac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->smac = *tmp;
    }
    {
        const mesa_hace_frame_etype_t *tmp = (const mesa_hace_frame_etype_t *)&in->etype;
#ifdef __cplusplus
        static_assert(sizeof(out.etype) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->etype = *tmp;
    }
    {
        const mesa_hace_frame_llc_t *tmp = (const mesa_hace_frame_llc_t *)&in->llc;
#ifdef __cplusplus
        static_assert(sizeof(out.llc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->llc = *tmp;
    }
    {
        const mesa_hace_frame_snap_t *tmp = (const mesa_hace_frame_snap_t *)&in->snap;
#ifdef __cplusplus
        static_assert(sizeof(out.snap) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->snap = *tmp;
    }
    {
        const mesa_hace_frame_arp_t *tmp = (const mesa_hace_frame_arp_t *)&in->arp;
#ifdef __cplusplus
        static_assert(sizeof(out.arp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->arp = *tmp;
    }
    {
        const mesa_hace_frame_ipv4_t *tmp = (const mesa_hace_frame_ipv4_t *)&in->ipv4;
#ifdef __cplusplus
        static_assert(sizeof(out.ipv4) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ipv4 = *tmp;
    }
    {
        const mesa_hace_frame_ipv6_t *tmp = (const mesa_hace_frame_ipv6_t *)&in->ipv6;
#ifdef __cplusplus
        static_assert(sizeof(out.ipv6) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ipv6 = *tmp;
    }

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_hace_key_t_to_vtss_hace_key_t(const mesa_hace_key_t *in, vtss_hace_key_t *out) /* ag.rb:1664 */
{
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(&in->port_list, out->port_list)); /* ag.rb:1432 */
    {
        const vtss_rleg_list_t *tmp = (const vtss_rleg_list_t *)&in->rleg_list;
#ifdef __cplusplus
        static_assert(sizeof(out.rleg_list) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->rleg_list = *tmp;
    }
    {
        const vtss_ace_u8_t *tmp = (const vtss_ace_u8_t *)&in->policy;
#ifdef __cplusplus
        static_assert(sizeof(out.policy) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->policy = *tmp;
    }
    {
        const vtss_ace_type_t *tmp = (const vtss_ace_type_t *)&in->type;
#ifdef __cplusplus
        static_assert(sizeof(out.type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->type = *tmp;
    }
    out->type_ext = in->type_ext;
    {
        const vtss_ace_vlan_t *tmp = (const vtss_ace_vlan_t *)&in->vlan;
#ifdef __cplusplus
        static_assert(sizeof(out.vlan) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->vlan = *tmp;
    }
    {
        const vtss_ace_ptp_t *tmp = (const vtss_ace_ptp_t *)&in->ptp;
#ifdef __cplusplus
        static_assert(sizeof(out.ptp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ptp = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->dmac_mc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_mc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_mc = *tmp;
    }
    {
        const vtss_ace_bit_t *tmp = (const vtss_ace_bit_t *)&in->dmac_bc;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_bc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_bc = *tmp;
    }
    {
        const vtss_ace_u48_t *tmp = (const vtss_ace_u48_t *)&in->dmac;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac = *tmp;
    }
    {
        const vtss_ace_u48_t *tmp = (const vtss_ace_u48_t *)&in->smac;
#ifdef __cplusplus
        static_assert(sizeof(out.smac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->smac = *tmp;
    }
    {
        const vtss_hace_frame_etype_t *tmp = (const vtss_hace_frame_etype_t *)&in->etype;
#ifdef __cplusplus
        static_assert(sizeof(out.etype) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->etype = *tmp;
    }
    {
        const vtss_hace_frame_llc_t *tmp = (const vtss_hace_frame_llc_t *)&in->llc;
#ifdef __cplusplus
        static_assert(sizeof(out.llc) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->llc = *tmp;
    }
    {
        const vtss_hace_frame_snap_t *tmp = (const vtss_hace_frame_snap_t *)&in->snap;
#ifdef __cplusplus
        static_assert(sizeof(out.snap) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->snap = *tmp;
    }
    {
        const vtss_hace_frame_arp_t *tmp = (const vtss_hace_frame_arp_t *)&in->arp;
#ifdef __cplusplus
        static_assert(sizeof(out.arp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->arp = *tmp;
    }
    {
        const vtss_hace_frame_ipv4_t *tmp = (const vtss_hace_frame_ipv4_t *)&in->ipv4;
#ifdef __cplusplus
        static_assert(sizeof(out.ipv4) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ipv4 = *tmp;
    }
    {
        const vtss_hace_frame_ipv6_t *tmp = (const vtss_hace_frame_ipv6_t *)&in->ipv6;
#ifdef __cplusplus
        static_assert(sizeof(out.ipv6) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ipv6 = *tmp;
    }

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_HACL)

#if defined(VTSS_FEATURE_HACL)
mesa_rc mesa_conv_vtss_hacl_action_t_to_mesa_hacl_action_t(const vtss_hacl_action_t *in, mesa_hacl_action_t *out) /* ag.rb:1656 */
{
    {
        const mesa_acl_port_action_t *tmp = (const mesa_acl_port_action_t *)&in->port_action;
#ifdef __cplusplus
        static_assert(sizeof(out.port_action) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->port_action = *tmp;
    }
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(in->port_list, &out->port_list)); /* ag.rb:1432 */
    out->cpu = in->cpu;
    out->cpu_once = in->cpu_once;
    out->cpu_disable = in->cpu_disable;
    out->cpu_queue = in->cpu_queue;
    out->police = in->police;
    out->policer_no = in->policer_no;
    out->mirror = in->mirror;
    __RC(mesa_conv_vtss_acl_ptp_action_t_to_mesa_acl_ptp_action_t(&in->ptp_action, &out->ptp_action)); /* ag.rb:1432 */
    {
        const mesa_acl_ptp_action_conf_t *tmp = (const mesa_acl_ptp_action_conf_t *)&in->ptp;
#ifdef __cplusplus
        static_assert(sizeof(out.ptp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ptp = *tmp;
    }
    {
        const mesa_acl_addr_action_t *tmp = (const mesa_acl_addr_action_t *)&in->addr;
#ifdef __cplusplus
        static_assert(sizeof(out.addr) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->addr = *tmp;
    }
    out->ifh_flag = in->ifh_flag;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_hacl_action_t_to_vtss_hacl_action_t(const mesa_hacl_action_t *in, vtss_hacl_action_t *out) /* ag.rb:1664 */
{
    {
        const vtss_acl_port_action_t *tmp = (const vtss_acl_port_action_t *)&in->port_action;
#ifdef __cplusplus
        static_assert(sizeof(out.port_action) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->port_action = *tmp;
    }
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(&in->port_list, out->port_list)); /* ag.rb:1432 */
    out->cpu = in->cpu;
    out->cpu_once = in->cpu_once;
    out->cpu_disable = in->cpu_disable;
    out->cpu_queue = in->cpu_queue;
    out->police = in->police;
    out->policer_no = in->policer_no;
    out->mirror = in->mirror;
    __RC(mesa_conv_mesa_acl_ptp_action_t_to_vtss_acl_ptp_action_t(&in->ptp_action, &out->ptp_action)); /* ag.rb:1432 */
    {
        const vtss_acl_ptp_action_conf_t *tmp = (const vtss_acl_ptp_action_conf_t *)&in->ptp;
#ifdef __cplusplus
        static_assert(sizeof(out.ptp) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->ptp = *tmp;
    }
    {
        const vtss_acl_addr_action_t *tmp = (const vtss_acl_addr_action_t *)&in->addr;
#ifdef __cplusplus
        static_assert(sizeof(out.addr) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->addr = *tmp;
    }
    out->ifh_flag = in->ifh_flag;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_HACL)

#if defined(VTSS_FEATURE_HACL)
mesa_rc mesa_conv_vtss_hace_t_to_mesa_hace_t(const vtss_hace_t *in, mesa_hace_t *out) /* ag.rb:1656 */
{
    out->id = in->id;
    __RC(mesa_conv_vtss_hace_key_t_to_mesa_hace_key_t(&in->key, &out->key)); /* ag.rb:1432 */
    __RC(mesa_conv_vtss_hacl_action_t_to_mesa_hacl_action_t(&in->action, &out->action)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_hace_t_to_vtss_hace_t(const mesa_hace_t *in, vtss_hace_t *out) /* ag.rb:1664 */
{
    out->id = in->id;
    __RC(mesa_conv_mesa_hace_key_t_to_vtss_hace_key_t(&in->key, &out->key)); /* ag.rb:1432 */
    __RC(mesa_conv_mesa_hacl_action_t_to_vtss_hacl_action_t(&in->action, &out->action)); /* ag.rb:1432 */

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_HACL)

#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_conv_vtss_synce_divider_t_to_mesa_synce_divider_t(const vtss_synce_divider_t *in, mesa_synce_divider_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        case VTSS_SYNCE_DIVIDER_1:
            *out = MESA_SYNCE_DIVIDER_1;
            return VTSS_RC_OK;
        case VTSS_SYNCE_DIVIDER_4:
            *out = MESA_SYNCE_DIVIDER_4;
            return VTSS_RC_OK;
        case VTSS_SYNCE_DIVIDER_5:
            *out = MESA_SYNCE_DIVIDER_5;
            return VTSS_RC_OK;
#if defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        case VTSS_SYNCE_DIVIDER_2:
            *out = MESA_SYNCE_DIVIDER_2;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        case VTSS_SYNCE_DIVIDER_8:
            *out = MESA_SYNCE_DIVIDER_8;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        case VTSS_SYNCE_DIVIDER_16:
            *out = MESA_SYNCE_DIVIDER_16;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        case VTSS_SYNCE_DIVIDER_25:
            *out = MESA_SYNCE_DIVIDER_25;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_synce_divider_t_to_vtss_synce_divider_t(const mesa_synce_divider_t *in, vtss_synce_divider_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        case MESA_SYNCE_DIVIDER_1:
            *out = VTSS_SYNCE_DIVIDER_1;
            return VTSS_RC_OK;
        case MESA_SYNCE_DIVIDER_4:
            *out = VTSS_SYNCE_DIVIDER_4;
            return VTSS_RC_OK;
        case MESA_SYNCE_DIVIDER_5:
            *out = VTSS_SYNCE_DIVIDER_5;
            return VTSS_RC_OK;
#if defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        case MESA_SYNCE_DIVIDER_2:
            *out = VTSS_SYNCE_DIVIDER_2;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        case MESA_SYNCE_DIVIDER_8:
            *out = VTSS_SYNCE_DIVIDER_8;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        case MESA_SYNCE_DIVIDER_16:
            *out = VTSS_SYNCE_DIVIDER_16;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        case MESA_SYNCE_DIVIDER_25:
            *out = VTSS_SYNCE_DIVIDER_25;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X)
        default:
            return VTSS_RC_ERROR;
    }
}
#endif  // defined(VTSS_FEATURE_SYNCE)

#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_conv_vtss_synce_clock_out_t_to_mesa_synce_clock_out_t(const vtss_synce_clock_out_t *in, mesa_synce_clock_out_t *out) /* ag.rb:1656 */
{
    __RC(mesa_conv_vtss_synce_divider_t_to_mesa_synce_divider_t(&in->divider, &out->divider)); /* ag.rb:1432 */
    out->enable = in->enable;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_synce_clock_out_t_to_vtss_synce_clock_out_t(const mesa_synce_clock_out_t *in, vtss_synce_clock_out_t *out) /* ag.rb:1664 */
{
    __RC(mesa_conv_mesa_synce_divider_t_to_vtss_synce_divider_t(&in->divider, &out->divider)); /* ag.rb:1432 */
    out->enable = in->enable;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SYNCE)

#if defined(VTSS_FEATURE_SYNCE)
#if defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
mesa_rc mesa_conv_vtss_synce_clock_in_type_t_to_mesa_synce_clock_in_type_t(const vtss_synce_clock_in_type_t *in, mesa_synce_clock_in_type_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        case VTSS_SYNCE_CLOCK_INTERFACE:
            *out = MESA_SYNCE_CLOCK_INTERFACE;
            return VTSS_RC_OK;
#if defined(VTSS_ARCH_SERVAL_T)
        case VTSS_SYNCE_CLOCK_STATION_CLK:
            *out = MESA_SYNCE_CLOCK_STATION_CLK;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SERVAL_T)
#if defined(VTSS_ARCH_SERVAL_T)
        case VTSS_SYNCE_CLOCK_DIFF:
            *out = MESA_SYNCE_CLOCK_DIFF;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SERVAL_T)
#if defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
        case VTSS_SYNCE_CLOCK_AUX:
            *out = MESA_SYNCE_CLOCK_AUX;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_synce_clock_in_type_t_to_vtss_synce_clock_in_type_t(const mesa_synce_clock_in_type_t *in, vtss_synce_clock_in_type_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        case MESA_SYNCE_CLOCK_INTERFACE:
            *out = VTSS_SYNCE_CLOCK_INTERFACE;
            return VTSS_RC_OK;
#if defined(VTSS_ARCH_SERVAL_T)
        case MESA_SYNCE_CLOCK_STATION_CLK:
            *out = VTSS_SYNCE_CLOCK_STATION_CLK;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SERVAL_T)
#if defined(VTSS_ARCH_SERVAL_T)
        case MESA_SYNCE_CLOCK_DIFF:
            *out = VTSS_SYNCE_CLOCK_DIFF;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SERVAL_T)
#if defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
        case MESA_SYNCE_CLOCK_AUX:
            *out = VTSS_SYNCE_CLOCK_AUX;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
        default:
            return VTSS_RC_ERROR;
    }
}
#endif  // defined(VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#endif  // defined(VTSS_FEATURE_SYNCE)

#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_conv_vtss_synce_clock_in_t_to_mesa_synce_clock_in_t(const vtss_synce_clock_in_t *in, mesa_synce_clock_in_t *out) /* ag.rb:1656 */
{
    out->port_no = in->port_no;
    out->squelsh = in->squelsh;
    out->enable = in->enable;
#if defined (VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    __RC(mesa_conv_vtss_synce_clock_in_type_t_to_mesa_synce_clock_in_type_t(&in->clk_in, &out->clk_in)); /* ag.rb:1432 */
#endif  // defined (VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_synce_clock_in_t_to_vtss_synce_clock_in_t(const mesa_synce_clock_in_t *in, vtss_synce_clock_in_t *out) /* ag.rb:1664 */
{
    out->port_no = in->port_no;
    out->squelsh = in->squelsh;
    out->enable = in->enable;
#if defined (VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
    __RC(mesa_conv_mesa_synce_clock_in_type_t_to_vtss_synce_clock_in_type_t(&in->clk_in, &out->clk_in)); /* ag.rb:1432 */
#endif  // defined (VTSS_ARCH_SERVAL_T) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SYNCE)

#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_conv_vtss_synce_station_clock_out_t_to_mesa_synce_station_clock_out_t(const vtss_synce_station_clock_out_t *in, mesa_synce_station_clock_out_t *out) /* ag.rb:1656 */
{
    __RC(mesa_conv_vtss_synce_divider_t_to_mesa_synce_divider_t(&in->divider, &out->divider)); /* ag.rb:1432 */
    out->enable = in->enable;
    out->dpll_out_no = in->dpll_out_no;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_synce_station_clock_out_t_to_vtss_synce_station_clock_out_t(const mesa_synce_station_clock_out_t *in, vtss_synce_station_clock_out_t *out) /* ag.rb:1664 */
{
    __RC(mesa_conv_mesa_synce_divider_t_to_vtss_synce_divider_t(&in->divider, &out->divider)); /* ag.rb:1432 */
    out->enable = in->enable;
    out->dpll_out_no = in->dpll_out_no;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_SYNCE)

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_conv_vtss_ts_operation_mode_t_to_mesa_ts_operation_mode_t(const vtss_ts_operation_mode_t *in, mesa_ts_operation_mode_t *out) /* ag.rb:1656 */
{
    {
        const mesa_ts_mode_t *tmp = (const mesa_ts_mode_t *)&in->mode;
#ifdef __cplusplus
        static_assert(sizeof(out.mode) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->mode = *tmp;
    }
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X)
    out->domain = in->domain;
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_ts_operation_mode_t_to_vtss_ts_operation_mode_t(const mesa_ts_operation_mode_t *in, vtss_ts_operation_mode_t *out) /* ag.rb:1664 */
{
    {
        const vtss_ts_mode_t *tmp = (const vtss_ts_mode_t *)&in->mode;
#ifdef __cplusplus
        static_assert(sizeof(out.mode) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->mode = *tmp;
    }
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X)
    out->domain = in->domain;
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_conv_vtss_ts_timestamp_alloc_t_to_mesa_ts_timestamp_alloc_t(const vtss_ts_timestamp_alloc_t *in, mesa_ts_timestamp_alloc_t *out) /* ag.rb:1656 */
{
    out->port_mask = in->port_mask;
    out->context = in->context;
    {
        const mesa_ts_timestamp_alloc_cb_t *tmp = (const mesa_ts_timestamp_alloc_cb_t *)&in->cb;
#ifdef __cplusplus
        static_assert(sizeof(out.cb) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->cb = *tmp;
    }

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_ts_timestamp_alloc_t_to_vtss_ts_timestamp_alloc_t(const mesa_ts_timestamp_alloc_t *in, vtss_ts_timestamp_alloc_t *out) /* ag.rb:1664 */
{
    out->port_mask = in->port_mask;
    out->context = in->context;
    {
        const vtss_ts_timestamp_alloc_cb_t *tmp = (const vtss_ts_timestamp_alloc_cb_t *)&in->cb;
#ifdef __cplusplus
        static_assert(sizeof(out.cb) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->cb = *tmp;
    }

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP)

#if defined(VTSS_FEATURE_FRER)
mesa_rc mesa_conv_vtss_frer_iflow_conf_t_to_mesa_frer_iflow_conf_t(const vtss_frer_iflow_conf_t *in, mesa_frer_iflow_conf_t *out) /* ag.rb:1656 */
{
    out->mstream_enable = in->mstream_enable;
    out->mstream_id = in->mstream_id;
    out->generation = in->generation;
#if defined(VTSS_FEATURE_FRER_IFLOW_POP)
    out->pop = in->pop;
#endif  // defined(VTSS_FEATURE_FRER_IFLOW_POP)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_frer_iflow_conf_t_to_vtss_frer_iflow_conf_t(const mesa_frer_iflow_conf_t *in, vtss_frer_iflow_conf_t *out) /* ag.rb:1664 */
{
    out->mstream_enable = in->mstream_enable;
    out->mstream_id = in->mstream_id;
    out->generation = in->generation;
#if defined(VTSS_FEATURE_FRER_IFLOW_POP)
    out->pop = in->pop;
#endif  // defined(VTSS_FEATURE_FRER_IFLOW_POP)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_FRER)

#if defined(VTSS_FEATURE_QOS_TAS)
mesa_rc mesa_conv_vtss_qos_tas_port_conf_t_to_mesa_qos_tas_port_conf_t(const vtss_qos_tas_port_conf_t *in, mesa_qos_tas_port_conf_t *out) /* ag.rb:1656 */
{
    // skipped: gcl_length@vtss_qos_tas_port_conf_t
    // skipped: gcl@vtss_qos_tas_port_conf_t
#if defined(VTSS_FEATURE_QOS_OT)
    out->ot = in->ot;
#endif  // defined(VTSS_FEATURE_QOS_OT)
    for (size_t i = 0; i < MESA_QUEUE_ARRAY_SIZE; ++i) {
        out->max_sdu[i] = in->max_sdu[i];
    }
    out->gate_enabled = in->gate_enabled;
    for (size_t i = 0; i < MESA_QUEUE_ARRAY_SIZE; ++i) {
        out->gate_open[i] = in->gate_open[i];
    }
    out->cycle_time = in->cycle_time;
    out->cycle_time_ext = in->cycle_time_ext;
    {
        const mesa_timestamp_t *tmp = (const mesa_timestamp_t *)&in->base_time;
#ifdef __cplusplus
        static_assert(sizeof(out.base_time) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->base_time = *tmp;
    }
    out->config_change = in->config_change;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_qos_tas_port_conf_t_to_vtss_qos_tas_port_conf_t(const mesa_qos_tas_port_conf_t *in, vtss_qos_tas_port_conf_t *out) /* ag.rb:1664 */
{
    // skipped: gcl_length@vtss_qos_tas_port_conf_t
    // skipped: gcl@vtss_qos_tas_port_conf_t
#if defined(VTSS_FEATURE_QOS_OT)
    out->ot = in->ot;
#endif  // defined(VTSS_FEATURE_QOS_OT)
    for (size_t i = 0; i < VTSS_QUEUE_ARRAY_SIZE; ++i) {
        out->max_sdu[i] = in->max_sdu[i];
    }
    out->gate_enabled = in->gate_enabled;
    for (size_t i = 0; i < VTSS_QUEUE_ARRAY_SIZE; ++i) {
        out->gate_open[i] = in->gate_open[i];
    }
    out->cycle_time = in->cycle_time;
    out->cycle_time_ext = in->cycle_time_ext;
    {
        const vtss_timestamp_t *tmp = (const vtss_timestamp_t *)&in->base_time;
#ifdef __cplusplus
        static_assert(sizeof(out.base_time) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->base_time = *tmp;
    }
    out->config_change = in->config_change;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_QOS_TAS)

mesa_rc mesa_conv_vtss_packet_reg_type_t_to_mesa_packet_reg_type_t(const vtss_packet_reg_type_t *in, mesa_packet_reg_type_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        case VTSS_PACKET_REG_NORMAL:
            *out = MESA_PACKET_REG_NORMAL;
            return VTSS_RC_OK;
        case VTSS_PACKET_REG_FORWARD:
            *out = MESA_PACKET_REG_FORWARD;
            return VTSS_RC_OK;
#if defined(VTSS_FEATURE_PACKET_PORT_REG_DISCARD)
        case VTSS_PACKET_REG_DISCARD:
            *out = MESA_PACKET_REG_DISCARD;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_PACKET_PORT_REG_DISCARD)
#if defined(VTSS_FEATURE_PACKET_PORT_REG_DISCARD)
        case VTSS_PACKET_REG_CPU_COPY:
            *out = MESA_PACKET_REG_CPU_COPY;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_PACKET_PORT_REG_DISCARD)
        case VTSS_PACKET_REG_CPU_ONLY:
            *out = MESA_PACKET_REG_CPU_ONLY;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_packet_reg_type_t_to_vtss_packet_reg_type_t(const mesa_packet_reg_type_t *in, vtss_packet_reg_type_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        case MESA_PACKET_REG_NORMAL:
            *out = VTSS_PACKET_REG_NORMAL;
            return VTSS_RC_OK;
        case MESA_PACKET_REG_FORWARD:
            *out = VTSS_PACKET_REG_FORWARD;
            return VTSS_RC_OK;
#if defined(VTSS_FEATURE_PACKET_PORT_REG_DISCARD)
        case MESA_PACKET_REG_DISCARD:
            *out = VTSS_PACKET_REG_DISCARD;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_PACKET_PORT_REG_DISCARD)
#if defined(VTSS_FEATURE_PACKET_PORT_REG_DISCARD)
        case MESA_PACKET_REG_CPU_COPY:
            *out = VTSS_PACKET_REG_CPU_COPY;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_PACKET_PORT_REG_DISCARD)
        case MESA_PACKET_REG_CPU_ONLY:
            *out = VTSS_PACKET_REG_CPU_ONLY;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_vtss_packet_rx_port_conf_t_to_mesa_packet_rx_port_conf_t(const vtss_packet_rx_port_conf_t *in, mesa_packet_rx_port_conf_t *out) /* ag.rb:1656 */
{
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
    __RC(mesa_conv_vtss_packet_reg_type_t_to_mesa_packet_reg_type_t(&in->ipmc_ctrl_reg, &out->ipmc_ctrl_reg)); /* ag.rb:1432 */
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
    __RC(mesa_conv_vtss_packet_reg_type_t_to_mesa_packet_reg_type_t(&in->igmp_reg, &out->igmp_reg)); /* ag.rb:1432 */
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
    __RC(mesa_conv_vtss_packet_reg_type_t_to_mesa_packet_reg_type_t(&in->mld_reg, &out->mld_reg)); /* ag.rb:1432 */
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
    for (size_t i = 0; i < 16; ++i) {
        __RC(mesa_conv_vtss_packet_reg_type_t_to_mesa_packet_reg_type_t(&in->bpdu_reg[i], &out->bpdu_reg[i])); /* ag.rb:1432 */
    }
    for (size_t i = 0; i < 16; ++i) {
        __RC(mesa_conv_vtss_packet_reg_type_t_to_mesa_packet_reg_type_t(&in->garp_reg[i], &out->garp_reg[i])); /* ag.rb:1432 */
    }
#if defined(VTSS_FEATURE_PACKET_PORT_L2CP_REG)
    for (size_t i = 0; i < 16; ++i) {
    const     mesa_packet_rx_port_l2cp_conf_t *tmp = (const mesa_packet_rx_port_l2cp_conf_t *)&in->bpdu[i];
        out->bpdu[i] = *tmp;
    }
#endif  // defined(VTSS_FEATURE_PACKET_PORT_L2CP_REG)
#if defined(VTSS_FEATURE_PACKET_PORT_L2CP_REG)
    for (size_t i = 0; i < 16; ++i) {
    const     mesa_packet_rx_port_l2cp_conf_t *tmp = (const mesa_packet_rx_port_l2cp_conf_t *)&in->garp[i];
        out->garp[i] = *tmp;
    }
#endif  // defined(VTSS_FEATURE_PACKET_PORT_L2CP_REG)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_packet_rx_port_conf_t_to_vtss_packet_rx_port_conf_t(const mesa_packet_rx_port_conf_t *in, vtss_packet_rx_port_conf_t *out) /* ag.rb:1664 */
{
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
    __RC(mesa_conv_mesa_packet_reg_type_t_to_vtss_packet_reg_type_t(&in->ipmc_ctrl_reg, &out->ipmc_ctrl_reg)); /* ag.rb:1432 */
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
    __RC(mesa_conv_mesa_packet_reg_type_t_to_vtss_packet_reg_type_t(&in->igmp_reg, &out->igmp_reg)); /* ag.rb:1432 */
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
    __RC(mesa_conv_mesa_packet_reg_type_t_to_vtss_packet_reg_type_t(&in->mld_reg, &out->mld_reg)); /* ag.rb:1432 */
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
    for (size_t i = 0; i < 16; ++i) {
        __RC(mesa_conv_mesa_packet_reg_type_t_to_vtss_packet_reg_type_t(&in->bpdu_reg[i], &out->bpdu_reg[i])); /* ag.rb:1432 */
    }
    for (size_t i = 0; i < 16; ++i) {
        __RC(mesa_conv_mesa_packet_reg_type_t_to_vtss_packet_reg_type_t(&in->garp_reg[i], &out->garp_reg[i])); /* ag.rb:1432 */
    }
#if defined(VTSS_FEATURE_PACKET_PORT_L2CP_REG)
    for (size_t i = 0; i < 16; ++i) {
    const     vtss_packet_rx_port_l2cp_conf_t *tmp = (const vtss_packet_rx_port_l2cp_conf_t *)&in->bpdu[i];
        out->bpdu[i] = *tmp;
    }
#endif  // defined(VTSS_FEATURE_PACKET_PORT_L2CP_REG)
#if defined(VTSS_FEATURE_PACKET_PORT_L2CP_REG)
    for (size_t i = 0; i < 16; ++i) {
    const     vtss_packet_rx_port_l2cp_conf_t *tmp = (const vtss_packet_rx_port_l2cp_conf_t *)&in->garp[i];
        out->garp[i] = *tmp;
    }
#endif  // defined(VTSS_FEATURE_PACKET_PORT_L2CP_REG)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_vtss_miim_controller_t_to_mesa_miim_controller_t(const vtss_miim_controller_t *in, mesa_miim_controller_t *out) /* ag.rb:1656 */
{
    switch (*in) {
#if defined(VTSS_FEATURE_LAYER2)
        case VTSS_MIIM_CONTROLLER_0:
            *out = MESA_MIIM_CONTROLLER_0;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
        case VTSS_MIIM_CONTROLLER_1:
            *out = MESA_MIIM_CONTROLLER_1;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
        case VTSS_MIIM_CONTROLLER_2:
            *out = MESA_MIIM_CONTROLLER_2;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
        case VTSS_MIIM_CONTROLLER_3:
            *out = MESA_MIIM_CONTROLLER_3;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
        case VTSS_MIIM_CONTROLLERS:
            *out = MESA_MIIM_CONTROLLERS;
            return VTSS_RC_OK;
        case VTSS_MIIM_CONTROLLER_NONE:
            *out = MESA_MIIM_CONTROLLER_NONE;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_miim_controller_t_to_vtss_miim_controller_t(const mesa_miim_controller_t *in, vtss_miim_controller_t *out) /* ag.rb:1664 */
{
    switch (*in) {
#if defined(VTSS_FEATURE_LAYER2)
        case MESA_MIIM_CONTROLLER_0:
            *out = VTSS_MIIM_CONTROLLER_0;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_LAYER2)
        case MESA_MIIM_CONTROLLER_1:
            *out = VTSS_MIIM_CONTROLLER_1;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
        case MESA_MIIM_CONTROLLER_2:
            *out = VTSS_MIIM_CONTROLLER_2;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
#if defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
        case MESA_MIIM_CONTROLLER_3:
            *out = VTSS_MIIM_CONTROLLER_3;
            return VTSS_RC_OK;
#endif  // defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
        case MESA_MIIM_CONTROLLERS:
            *out = VTSS_MIIM_CONTROLLERS;
            return VTSS_RC_OK;
        case MESA_MIIM_CONTROLLER_NONE:
            *out = VTSS_MIIM_CONTROLLER_NONE;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_vop_conf_t_to_mesa_vop_conf_t(const vtss_vop_conf_t *in, mesa_vop_conf_t *out) /* ag.rb:1656 */
{
    {
        const mesa_mac_t *tmp = (const mesa_mac_t *)&in->multicast_dmac;
#ifdef __cplusplus
        static_assert(sizeof(out.multicast_dmac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->multicast_dmac = *tmp;
    }
#if defined(VTSS_FEATURE_VOP_V2)
    for (size_t i = 0; i < 2; ++i) {
        out->auto_copy_period[i] = in->auto_copy_period[i];
    }
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->auto_copy_ccm_valid = in->auto_copy_ccm_valid;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->auto_copy_ccm_tlv = in->auto_copy_ccm_tlv;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->auto_copy_ccm_err = in->auto_copy_ccm_err;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->auto_copy_lbr = in->auto_copy_lbr;
#endif  // defined(VTSS_FEATURE_VOP_V2)
    out->voe_queue_ccm = in->voe_queue_ccm;
#if defined(VTSS_FEATURE_VOP_CFM)
    out->voe_queue_lt = in->voe_queue_lt;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#if defined(VTSS_FEATURE_VOP_CFM)
    out->voe_queue_lbm = in->voe_queue_lbm;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#if defined(VTSS_FEATURE_VOP_CFM)
    out->voe_queue_lbr = in->voe_queue_lbr;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#if defined(VTSS_FEATURE_VOP_CFM)
    out->voe_queue_aps = in->voe_queue_aps;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
    out->voe_queue_err = in->voe_queue_err;
#if defined(VTSS_FEATURE_VOP_V2)
    out->voi_queue = in->voi_queue;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_MRP)
    out->mrp_queue = in->mrp_queue;
#endif  // defined(VTSS_FEATURE_MRP)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_vop_conf_t_to_vtss_vop_conf_t(const mesa_vop_conf_t *in, vtss_vop_conf_t *out) /* ag.rb:1664 */
{
    {
        const vtss_mac_t *tmp = (const vtss_mac_t *)&in->multicast_dmac;
#ifdef __cplusplus
        static_assert(sizeof(out.multicast_dmac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->multicast_dmac = *tmp;
    }
#if defined(VTSS_FEATURE_VOP_V2)
    for (size_t i = 0; i < 2; ++i) {
        out->auto_copy_period[i] = in->auto_copy_period[i];
    }
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->auto_copy_ccm_valid = in->auto_copy_ccm_valid;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->auto_copy_ccm_tlv = in->auto_copy_ccm_tlv;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->auto_copy_ccm_err = in->auto_copy_ccm_err;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->auto_copy_lbr = in->auto_copy_lbr;
#endif  // defined(VTSS_FEATURE_VOP_V2)
    out->voe_queue_ccm = in->voe_queue_ccm;
#if defined(VTSS_FEATURE_VOP_CFM)
    out->voe_queue_lt = in->voe_queue_lt;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#if defined(VTSS_FEATURE_VOP_CFM)
    out->voe_queue_lbm = in->voe_queue_lbm;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#if defined(VTSS_FEATURE_VOP_CFM)
    out->voe_queue_lbr = in->voe_queue_lbr;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#if defined(VTSS_FEATURE_VOP_CFM)
    out->voe_queue_aps = in->voe_queue_aps;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
    out->voe_queue_err = in->voe_queue_err;
#if defined(VTSS_FEATURE_VOP_V2)
    out->voi_queue = in->voi_queue;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_MRP)
    out->mrp_queue = in->mrp_queue;
#endif  // defined(VTSS_FEATURE_MRP)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP)

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_voe_allocation_t_to_mesa_voe_allocation_t(const vtss_voe_allocation_t *in, mesa_voe_allocation_t *out) /* ag.rb:1656 */
{
#if defined(VTSS_FEATURE_VOP_CFM)
    {
        const mesa_voe_type_t *tmp = (const mesa_voe_type_t *)&in->type;
#ifdef __cplusplus
        static_assert(sizeof(out.type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->type = *tmp;
    }
#endif  // defined(VTSS_FEATURE_VOP_CFM)
    out->port = in->port;
#if defined(VTSS_FEATURE_VOP_CFM)
    {
        const mesa_oam_direction_t *tmp = (const mesa_oam_direction_t *)&in->direction;
#ifdef __cplusplus
        static_assert(sizeof(out.direction) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->direction = *tmp;
    }
#endif  // defined(VTSS_FEATURE_VOP_CFM)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_voe_allocation_t_to_vtss_voe_allocation_t(const mesa_voe_allocation_t *in, vtss_voe_allocation_t *out) /* ag.rb:1664 */
{
#if defined(VTSS_FEATURE_VOP_CFM)
    {
        const vtss_voe_type_t *tmp = (const vtss_voe_type_t *)&in->type;
#ifdef __cplusplus
        static_assert(sizeof(out.type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->type = *tmp;
    }
#endif  // defined(VTSS_FEATURE_VOP_CFM)
    out->port = in->port;
#if defined(VTSS_FEATURE_VOP_CFM)
    {
        const vtss_oam_direction_t *tmp = (const vtss_oam_direction_t *)&in->direction;
#ifdef __cplusplus
        static_assert(sizeof(out.direction) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->direction = *tmp;
    }
#endif  // defined(VTSS_FEATURE_VOP_CFM)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP)

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_voe_conf_t_to_mesa_voe_conf_t(const vtss_voe_conf_t *in, mesa_voe_conf_t *out) /* ag.rb:1656 */
{
    out->enable = in->enable;
    {
        const mesa_mac_t *tmp = (const mesa_mac_t *)&in->unicast_mac;
#ifdef __cplusplus
        static_assert(sizeof(out.unicast_mac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->unicast_mac = *tmp;
    }
    out->meg_level = in->meg_level;
    {
        const mesa_voe_dmac_check_t *tmp = (const mesa_voe_dmac_check_t *)&in->dmac_check_type;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_check_type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_check_type = *tmp;
    }
#if defined(VTSS_FEATURE_VOP_CFM)
    out->loop_iflow_id = in->loop_iflow_id;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#if defined(VTSS_FEATURE_VOP_V2)
    out->block_mel_high = in->block_mel_high;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_TAGGING)
    {
        const mesa_port_max_tags_t *tmp = (const mesa_port_max_tags_t *)&in->tagging;
#ifdef __cplusplus
        static_assert(sizeof(out.tagging) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tagging = *tmp;
    }
#endif  // defined(VTSS_FEATURE_VOP_TAGGING)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_voe_conf_t_to_vtss_voe_conf_t(const mesa_voe_conf_t *in, vtss_voe_conf_t *out) /* ag.rb:1664 */
{
    out->enable = in->enable;
    {
        const vtss_mac_t *tmp = (const vtss_mac_t *)&in->unicast_mac;
#ifdef __cplusplus
        static_assert(sizeof(out.unicast_mac) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->unicast_mac = *tmp;
    }
    out->meg_level = in->meg_level;
    {
        const vtss_voe_dmac_check_t *tmp = (const vtss_voe_dmac_check_t *)&in->dmac_check_type;
#ifdef __cplusplus
        static_assert(sizeof(out.dmac_check_type) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->dmac_check_type = *tmp;
    }
#if defined(VTSS_FEATURE_VOP_CFM)
    out->loop_iflow_id = in->loop_iflow_id;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#if defined(VTSS_FEATURE_VOP_V2)
    out->block_mel_high = in->block_mel_high;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_TAGGING)
    {
        const vtss_port_max_tags_t *tmp = (const vtss_port_max_tags_t *)&in->tagging;
#ifdef __cplusplus
        static_assert(sizeof(out.tagging) == sizeof(*tmp), "size check");
#endif // __cplusplus
        out->tagging = *tmp;
    }
#endif  // defined(VTSS_FEATURE_VOP_TAGGING)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP)

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_voe_ccm_period_t_to_mesa_voe_ccm_period_t(const vtss_voe_ccm_period_t *in, mesa_voe_ccm_period_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        case VTSS_VOE_CCM_PERIOD_3_3_MS:
            *out = MESA_VOE_CCM_PERIOD_3_3_MS;
            return VTSS_RC_OK;
        case VTSS_VOE_CCM_PERIOD_10_MS:
            *out = MESA_VOE_CCM_PERIOD_10_MS;
            return VTSS_RC_OK;
        case VTSS_VOE_CCM_PERIOD_100_MS:
            *out = MESA_VOE_CCM_PERIOD_100_MS;
            return VTSS_RC_OK;
        case VTSS_VOE_CCM_PERIOD_1_SEC:
            *out = MESA_VOE_CCM_PERIOD_1_SEC;
            return VTSS_RC_OK;
#if defined(VTSS_FEATURE_VOP_V2)
        case VTSS_VOE_CCM_PERIOD_10_SEC:
            *out = MESA_VOE_CCM_PERIOD_10_SEC;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_VOP_V2)
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_voe_ccm_period_t_to_vtss_voe_ccm_period_t(const mesa_voe_ccm_period_t *in, vtss_voe_ccm_period_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        case MESA_VOE_CCM_PERIOD_3_3_MS:
            *out = VTSS_VOE_CCM_PERIOD_3_3_MS;
            return VTSS_RC_OK;
        case MESA_VOE_CCM_PERIOD_10_MS:
            *out = VTSS_VOE_CCM_PERIOD_10_MS;
            return VTSS_RC_OK;
        case MESA_VOE_CCM_PERIOD_100_MS:
            *out = VTSS_VOE_CCM_PERIOD_100_MS;
            return VTSS_RC_OK;
        case MESA_VOE_CCM_PERIOD_1_SEC:
            *out = VTSS_VOE_CCM_PERIOD_1_SEC;
            return VTSS_RC_OK;
#if defined(VTSS_FEATURE_VOP_V2)
        case MESA_VOE_CCM_PERIOD_10_SEC:
            *out = VTSS_VOE_CCM_PERIOD_10_SEC;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_VOP_V2)
        default:
            return VTSS_RC_ERROR;
    }
}
#endif  // defined(VTSS_FEATURE_VOP)

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_oam_cpu_copy_t_to_mesa_oam_cpu_copy_t(const vtss_oam_cpu_copy_t *in, mesa_oam_cpu_copy_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        case VTSS_OAM_CPU_COPY_NONE:
            *out = MESA_OAM_CPU_COPY_NONE;
            return VTSS_RC_OK;
        case VTSS_OAM_CPU_COPY_ALL:
            *out = MESA_OAM_CPU_COPY_ALL;
            return VTSS_RC_OK;
#if defined(VTSS_FEATURE_VOP_V2)
        case VTSS_OAM_CPU_COPY_AUTO:
            *out = MESA_OAM_CPU_COPY_AUTO;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_VOP_V2)
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_oam_cpu_copy_t_to_vtss_oam_cpu_copy_t(const mesa_oam_cpu_copy_t *in, vtss_oam_cpu_copy_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        case MESA_OAM_CPU_COPY_NONE:
            *out = VTSS_OAM_CPU_COPY_NONE;
            return VTSS_RC_OK;
        case MESA_OAM_CPU_COPY_ALL:
            *out = VTSS_OAM_CPU_COPY_ALL;
            return VTSS_RC_OK;
#if defined(VTSS_FEATURE_VOP_V2)
        case MESA_OAM_CPU_COPY_AUTO:
            *out = VTSS_OAM_CPU_COPY_AUTO;
            return VTSS_RC_OK;
#endif  // defined(VTSS_FEATURE_VOP_V2)
        default:
            return VTSS_RC_ERROR;
    }
}
#endif  // defined(VTSS_FEATURE_VOP)

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_voe_cc_conf_t_to_mesa_voe_cc_conf_t(const vtss_voe_cc_conf_t *in, mesa_voe_cc_conf_t *out) /* ag.rb:1656 */
{
    out->enable = in->enable;
    __RC(mesa_conv_vtss_oam_cpu_copy_t_to_mesa_oam_cpu_copy_t(&in->cpu_copy, &out->cpu_copy)); /* ag.rb:1432 */
    out->seq_no_update = in->seq_no_update;
    out->count_as_selected = in->count_as_selected;
    __RC(mesa_conv_vtss_voe_ccm_period_t_to_mesa_voe_ccm_period_t(&in->expected_period, &out->expected_period)); /* ag.rb:1432 */
    out->expected_priority = in->expected_priority;
    for (size_t i = 0; i < MESA_OAM_MEG_ID_LENGTH; ++i) {
        out->expected_megid[i] = in->expected_megid[i];
    }
    out->expected_peer_mepid = in->expected_peer_mepid;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_voe_cc_conf_t_to_vtss_voe_cc_conf_t(const mesa_voe_cc_conf_t *in, vtss_voe_cc_conf_t *out) /* ag.rb:1664 */
{
    out->enable = in->enable;
    __RC(mesa_conv_mesa_oam_cpu_copy_t_to_vtss_oam_cpu_copy_t(&in->cpu_copy, &out->cpu_copy)); /* ag.rb:1432 */
    out->seq_no_update = in->seq_no_update;
    out->count_as_selected = in->count_as_selected;
    __RC(mesa_conv_mesa_voe_ccm_period_t_to_vtss_voe_ccm_period_t(&in->expected_period, &out->expected_period)); /* ag.rb:1432 */
    out->expected_priority = in->expected_priority;
    for (size_t i = 0; i < VTSS_OAM_MEG_ID_LENGTH; ++i) {
        out->expected_megid[i] = in->expected_megid[i];
    }
    out->expected_peer_mepid = in->expected_peer_mepid;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP)

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_conv_vtss_voe_lb_conf_t_to_mesa_voe_lb_conf_t(const vtss_voe_lb_conf_t *in, mesa_voe_lb_conf_t *out) /* ag.rb:1656 */
{
    out->enable = in->enable;
    out->count_as_selected = in->count_as_selected;
    out->lbm_cpu_copy = in->lbm_cpu_copy;
    __RC(mesa_conv_vtss_oam_cpu_copy_t_to_mesa_oam_cpu_copy_t(&in->lbr_cpu_copy, &out->lbr_cpu_copy)); /* ag.rb:1432 */
    out->trans_id = in->trans_id;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_voe_lb_conf_t_to_vtss_voe_lb_conf_t(const mesa_voe_lb_conf_t *in, vtss_voe_lb_conf_t *out) /* ag.rb:1664 */
{
    out->enable = in->enable;
    out->count_as_selected = in->count_as_selected;
    out->lbm_cpu_copy = in->lbm_cpu_copy;
    __RC(mesa_conv_mesa_oam_cpu_copy_t_to_vtss_oam_cpu_copy_t(&in->lbr_cpu_copy, &out->lbr_cpu_copy)); /* ag.rb:1432 */
    out->trans_id = in->trans_id;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_voe_status_t_to_mesa_voe_status_t(const vtss_voe_status_t *in, mesa_voe_status_t *out) /* ag.rb:1656 */
{
    out->opcode_unexp_seen = in->opcode_unexp_seen;
    out->dmac_unexp_seen = in->dmac_unexp_seen;
#if defined(VTSS_FEATURE_VOP_CFM)
    out->tx_level_low_seen = in->tx_level_low_seen;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#if defined(VTSS_FEATURE_VOP_V2)
    out->version_unexp_seen = in->version_unexp_seen;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->rx_level_low_seen = in->rx_level_low_seen;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->rx_level_high_seen = in->rx_level_high_seen;
#endif  // defined(VTSS_FEATURE_VOP_V2)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_voe_status_t_to_vtss_voe_status_t(const mesa_voe_status_t *in, vtss_voe_status_t *out) /* ag.rb:1664 */
{
    out->opcode_unexp_seen = in->opcode_unexp_seen;
    out->dmac_unexp_seen = in->dmac_unexp_seen;
#if defined(VTSS_FEATURE_VOP_CFM)
    out->tx_level_low_seen = in->tx_level_low_seen;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#if defined(VTSS_FEATURE_VOP_V2)
    out->version_unexp_seen = in->version_unexp_seen;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->rx_level_low_seen = in->rx_level_low_seen;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->rx_level_high_seen = in->rx_level_high_seen;
#endif  // defined(VTSS_FEATURE_VOP_V2)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP)

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_voe_counters_t_to_mesa_voe_counters_t(const vtss_voe_counters_t *in, mesa_voe_counters_t *out) /* ag.rb:1656 */
{
    out->rx_counter = in->rx_counter;
#if defined(VTSS_FEATURE_VOP_CFM)
    out->tx_counter = in->tx_counter;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
    out->rx_selected_counter = in->rx_selected_counter;
#if defined(VTSS_FEATURE_VOP_CFM)
    out->tx_selected_counter = in->tx_selected_counter;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#if defined(VTSS_FEATURE_VOP_V2)
    out->rx_discard_counter = in->rx_discard_counter;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->tx_discard_counter = in->tx_discard_counter;
#endif  // defined(VTSS_FEATURE_VOP_V2)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_voe_counters_t_to_vtss_voe_counters_t(const mesa_voe_counters_t *in, vtss_voe_counters_t *out) /* ag.rb:1664 */
{
    out->rx_counter = in->rx_counter;
#if defined(VTSS_FEATURE_VOP_CFM)
    out->tx_counter = in->tx_counter;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
    out->rx_selected_counter = in->rx_selected_counter;
#if defined(VTSS_FEATURE_VOP_CFM)
    out->tx_selected_counter = in->tx_selected_counter;
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#if defined(VTSS_FEATURE_VOP_V2)
    out->rx_discard_counter = in->rx_discard_counter;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->tx_discard_counter = in->tx_discard_counter;
#endif  // defined(VTSS_FEATURE_VOP_V2)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP)

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_conv_vtss_voe_cc_status_t_to_mesa_voe_cc_status_t(const vtss_voe_cc_status_t *in, mesa_voe_cc_status_t *out) /* ag.rb:1656 */
{
    out->zero_period = in->zero_period;
    out->rdi = in->rdi;
    out->loc = in->loc;
    out->period_unexp = in->period_unexp;
    out->priority_unexp = in->priority_unexp;
    out->mep_id_unexp = in->mep_id_unexp;
    out->meg_id_unexp = in->meg_id_unexp;
#if defined(VTSS_FEATURE_VOP_V2)
    out->mel_unexp = in->mel_unexp;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->rx_port = in->rx_port;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->port_status_tlv = in->port_status_tlv;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->if_status_tlv = in->if_status_tlv;
#endif  // defined(VTSS_FEATURE_VOP_V2)
    out->seen = in->seen;
#if defined(VTSS_FEATURE_VOP_V2)
    out->tlv_seen = in->tlv_seen;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->seq_unexp_seen = in->seq_unexp_seen;
#endif  // defined(VTSS_FEATURE_VOP_V2)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_voe_cc_status_t_to_vtss_voe_cc_status_t(const mesa_voe_cc_status_t *in, vtss_voe_cc_status_t *out) /* ag.rb:1664 */
{
    out->zero_period = in->zero_period;
    out->rdi = in->rdi;
    out->loc = in->loc;
    out->period_unexp = in->period_unexp;
    out->priority_unexp = in->priority_unexp;
    out->mep_id_unexp = in->mep_id_unexp;
    out->meg_id_unexp = in->meg_id_unexp;
#if defined(VTSS_FEATURE_VOP_V2)
    out->mel_unexp = in->mel_unexp;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->rx_port = in->rx_port;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->port_status_tlv = in->port_status_tlv;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->if_status_tlv = in->if_status_tlv;
#endif  // defined(VTSS_FEATURE_VOP_V2)
    out->seen = in->seen;
#if defined(VTSS_FEATURE_VOP_V2)
    out->tlv_seen = in->tlv_seen;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->seq_unexp_seen = in->seq_unexp_seen;
#endif  // defined(VTSS_FEATURE_VOP_V2)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP)

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_conv_vtss_voe_lb_status_t_to_mesa_voe_lb_status_t(const vtss_voe_lb_status_t *in, mesa_voe_lb_status_t *out) /* ag.rb:1656 */
{
    out->lbm_seen = in->lbm_seen;
    out->lbr_seen = in->lbr_seen;
#if defined(VTSS_FEATURE_VOP_V2)
    out->trans_unexp_seen = in->trans_unexp_seen;
#endif  // defined(VTSS_FEATURE_VOP_V2)
    out->tx_trans_id = in->tx_trans_id;
    out->rx_trans_id = in->rx_trans_id;

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_voe_lb_status_t_to_vtss_voe_lb_status_t(const mesa_voe_lb_status_t *in, vtss_voe_lb_status_t *out) /* ag.rb:1664 */
{
    out->lbm_seen = in->lbm_seen;
    out->lbr_seen = in->lbr_seen;
#if defined(VTSS_FEATURE_VOP_V2)
    out->trans_unexp_seen = in->trans_unexp_seen;
#endif  // defined(VTSS_FEATURE_VOP_V2)
    out->tx_trans_id = in->tx_trans_id;
    out->rx_trans_id = in->rx_trans_id;

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_conv_vtss_voe_lb_counters_t_to_mesa_voe_lb_counters_t(const vtss_voe_lb_counters_t *in, mesa_voe_lb_counters_t *out) /* ag.rb:1656 */
{
    out->rx_lbr_counter = in->rx_lbr_counter;
    out->tx_lbm_counter = in->tx_lbm_counter;
    out->rx_lbr_oo_counter = in->rx_lbr_oo_counter;
#if defined(VTSS_FEATURE_VOP_V2)
    out->rx_lbr_crc_counter = in->rx_lbr_crc_counter;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->tx_lbr_counter = in->tx_lbr_counter;
#endif  // defined(VTSS_FEATURE_VOP_V2)

    return VTSS_RC_OK;
}

mesa_rc mesa_conv_mesa_voe_lb_counters_t_to_vtss_voe_lb_counters_t(const mesa_voe_lb_counters_t *in, vtss_voe_lb_counters_t *out) /* ag.rb:1664 */
{
    out->rx_lbr_counter = in->rx_lbr_counter;
    out->tx_lbm_counter = in->tx_lbm_counter;
    out->rx_lbr_oo_counter = in->rx_lbr_oo_counter;
#if defined(VTSS_FEATURE_VOP_V2)
    out->rx_lbr_crc_counter = in->rx_lbr_crc_counter;
#endif  // defined(VTSS_FEATURE_VOP_V2)
#if defined(VTSS_FEATURE_VOP_V2)
    out->tx_lbr_counter = in->tx_lbr_counter;
#endif  // defined(VTSS_FEATURE_VOP_V2)

    return VTSS_RC_OK;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM)
#endif  // defined(VTSS_FEATURE_VOP)

mesa_rc mesa_conv_vtss_phy_power_mode_t_to_mesa_phy_power_mode_t(const vtss_phy_power_mode_t *in, mesa_phy_power_mode_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        case VTSS_PHY_POWER_NOMINAL:
            *out = MESA_PHY_POWER_NOMINAL;
            return VTSS_RC_OK;
        case VTSS_PHY_POWER_ACTIPHY:
            *out = MESA_PHY_POWER_ACTIPHY;
            return VTSS_RC_OK;
        case VTSS_PHY_POWER_DYNAMIC:
            *out = MESA_PHY_POWER_DYNAMIC;
            return VTSS_RC_OK;
        case VTSS_PHY_POWER_ENABLED:
            *out = MESA_PHY_POWER_ENABLED;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_phy_power_mode_t_to_vtss_phy_power_mode_t(const mesa_phy_power_mode_t *in, vtss_phy_power_mode_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        case MESA_PHY_POWER_NOMINAL:
            *out = VTSS_PHY_POWER_NOMINAL;
            return VTSS_RC_OK;
        case MESA_PHY_POWER_ACTIPHY:
            *out = VTSS_PHY_POWER_ACTIPHY;
            return VTSS_RC_OK;
        case MESA_PHY_POWER_DYNAMIC:
            *out = VTSS_PHY_POWER_DYNAMIC;
            return VTSS_RC_OK;
        case MESA_PHY_POWER_ENABLED:
            *out = VTSS_PHY_POWER_ENABLED;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_vtss_port_interface_t_to_mesa_port_interface_t(const vtss_port_interface_t *in, mesa_port_interface_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        // skipped: VTSS_PORT_INTERFACE_CPU
        case VTSS_PORT_INTERFACE_NO_CONNECTION:
            *out = MESA_PORT_INTERFACE_NO_CONNECTION;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_LOOPBACK:
            *out = MESA_PORT_INTERFACE_LOOPBACK;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_INTERNAL:
            *out = MESA_PORT_INTERFACE_INTERNAL;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_MII:
            *out = MESA_PORT_INTERFACE_MII;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_GMII:
            *out = MESA_PORT_INTERFACE_GMII;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_RGMII:
            *out = MESA_PORT_INTERFACE_RGMII;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_RGMII_ID:
            *out = MESA_PORT_INTERFACE_RGMII_ID;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_RGMII_RXID:
            *out = MESA_PORT_INTERFACE_RGMII_RXID;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_RGMII_TXID:
            *out = MESA_PORT_INTERFACE_RGMII_TXID;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_TBI:
            *out = MESA_PORT_INTERFACE_TBI;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_RTBI:
            *out = MESA_PORT_INTERFACE_RTBI;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_SGMII:
            *out = MESA_PORT_INTERFACE_SGMII;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_SGMII_2G5:
            *out = MESA_PORT_INTERFACE_SGMII_2G5;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_SGMII_CISCO:
            *out = MESA_PORT_INTERFACE_SGMII_CISCO;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_SERDES:
            *out = MESA_PORT_INTERFACE_SERDES;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_VAUI:
            *out = MESA_PORT_INTERFACE_VAUI;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_100FX:
            *out = MESA_PORT_INTERFACE_100FX;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_XAUI:
            *out = MESA_PORT_INTERFACE_XAUI;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_RXAUI:
            *out = MESA_PORT_INTERFACE_RXAUI;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_XGMII:
            *out = MESA_PORT_INTERFACE_XGMII;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_SPI4:
            *out = MESA_PORT_INTERFACE_SPI4;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_QSGMII:
            *out = MESA_PORT_INTERFACE_QSGMII;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_SFI:
            *out = MESA_PORT_INTERFACE_SFI;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_USXGMII:
            *out = MESA_PORT_INTERFACE_USXGMII;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_USGMII:
            *out = MESA_PORT_INTERFACE_USGMII;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_QXGMII:
            *out = MESA_PORT_INTERFACE_QXGMII;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_DXGMII_5G:
            *out = MESA_PORT_INTERFACE_DXGMII_5G;
            return VTSS_RC_OK;
        case VTSS_PORT_INTERFACE_DXGMII_10G:
            *out = MESA_PORT_INTERFACE_DXGMII_10G;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_port_interface_t_to_vtss_port_interface_t(const mesa_port_interface_t *in, vtss_port_interface_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        // skipped: VTSS_PORT_INTERFACE_CPU
        case MESA_PORT_INTERFACE_NO_CONNECTION:
            *out = VTSS_PORT_INTERFACE_NO_CONNECTION;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_LOOPBACK:
            *out = VTSS_PORT_INTERFACE_LOOPBACK;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_INTERNAL:
            *out = VTSS_PORT_INTERFACE_INTERNAL;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_MII:
            *out = VTSS_PORT_INTERFACE_MII;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_GMII:
            *out = VTSS_PORT_INTERFACE_GMII;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_RGMII:
            *out = VTSS_PORT_INTERFACE_RGMII;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_RGMII_ID:
            *out = VTSS_PORT_INTERFACE_RGMII_ID;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_RGMII_RXID:
            *out = VTSS_PORT_INTERFACE_RGMII_RXID;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_RGMII_TXID:
            *out = VTSS_PORT_INTERFACE_RGMII_TXID;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_TBI:
            *out = VTSS_PORT_INTERFACE_TBI;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_RTBI:
            *out = VTSS_PORT_INTERFACE_RTBI;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_SGMII:
            *out = VTSS_PORT_INTERFACE_SGMII;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_SGMII_2G5:
            *out = VTSS_PORT_INTERFACE_SGMII_2G5;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_SGMII_CISCO:
            *out = VTSS_PORT_INTERFACE_SGMII_CISCO;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_SERDES:
            *out = VTSS_PORT_INTERFACE_SERDES;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_VAUI:
            *out = VTSS_PORT_INTERFACE_VAUI;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_100FX:
            *out = VTSS_PORT_INTERFACE_100FX;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_XAUI:
            *out = VTSS_PORT_INTERFACE_XAUI;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_RXAUI:
            *out = VTSS_PORT_INTERFACE_RXAUI;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_XGMII:
            *out = VTSS_PORT_INTERFACE_XGMII;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_SPI4:
            *out = VTSS_PORT_INTERFACE_SPI4;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_QSGMII:
            *out = VTSS_PORT_INTERFACE_QSGMII;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_SFI:
            *out = VTSS_PORT_INTERFACE_SFI;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_USXGMII:
            *out = VTSS_PORT_INTERFACE_USXGMII;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_USGMII:
            *out = VTSS_PORT_INTERFACE_USGMII;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_QXGMII:
            *out = VTSS_PORT_INTERFACE_QXGMII;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_DXGMII_5G:
            *out = VTSS_PORT_INTERFACE_DXGMII_5G;
            return VTSS_RC_OK;
        case MESA_PORT_INTERFACE_DXGMII_10G:
            *out = VTSS_PORT_INTERFACE_DXGMII_10G;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_vtss_serdes_mode_t_to_mesa_serdes_mode_t(const vtss_serdes_mode_t *in, mesa_serdes_mode_t *out) /* ag.rb:1656 */
{
    switch (*in) {
        // skipped: VTSS_SERDES_MODE_SFI_KR
        // skipped: VTSS_SERDES_MODE_USXGMII
        // skipped: MESA_SERDES_MODE_SXGMII
        case VTSS_SERDES_MODE_DISABLE:
            *out = MESA_SERDES_MODE_DISABLE;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_NONE:
            *out = MESA_SERDES_MODE_NONE;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_XAUI_12G:
            *out = MESA_SERDES_MODE_XAUI_12G;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_XAUI:
            *out = MESA_SERDES_MODE_XAUI;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_RXAUI:
            *out = MESA_SERDES_MODE_RXAUI;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_RXAUI_12G:
            *out = MESA_SERDES_MODE_RXAUI_12G;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_2G5:
            *out = MESA_SERDES_MODE_2G5;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_QSGMII:
            *out = MESA_SERDES_MODE_QSGMII;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_SGMII:
            *out = MESA_SERDES_MODE_SGMII;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_100FX:
            *out = MESA_SERDES_MODE_100FX;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_1000BaseX:
            *out = MESA_SERDES_MODE_1000BaseX;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_SFI:
            *out = MESA_SERDES_MODE_SFI;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_SFI_DAC:
            *out = MESA_SERDES_MODE_SFI_DAC;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_SFI_SR:
            *out = MESA_SERDES_MODE_SFI_SR;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_SFI_ZR:
            *out = MESA_SERDES_MODE_SFI_ZR;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_SFI_BP:
            *out = MESA_SERDES_MODE_SFI_BP;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_SFI_B2B:
            *out = MESA_SERDES_MODE_SFI_B2B;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_SFI_PR_NONE:
            *out = MESA_SERDES_MODE_SFI_PR_NONE;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_IDLE:
            *out = MESA_SERDES_MODE_IDLE;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_TEST_MODE:
            *out = MESA_SERDES_MODE_TEST_MODE;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_USGMII:
            *out = MESA_SERDES_MODE_USGMII;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_QXGMII:
            *out = MESA_SERDES_MODE_QXGMII;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_DXGMII_10G:
            *out = MESA_SERDES_MODE_DXGMII_10G;
            return VTSS_RC_OK;
        case VTSS_SERDES_MODE_DXGMII_5G:
            *out = MESA_SERDES_MODE_DXGMII_5G;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

mesa_rc mesa_conv_mesa_serdes_mode_t_to_vtss_serdes_mode_t(const mesa_serdes_mode_t *in, vtss_serdes_mode_t *out) /* ag.rb:1664 */
{
    switch (*in) {
        // skipped: MESA_SERDES_MODE_SXGMII
        // skipped: VTSS_SERDES_MODE_SFI_KR
        // skipped: VTSS_SERDES_MODE_USXGMII
        case MESA_SERDES_MODE_DISABLE:
            *out = VTSS_SERDES_MODE_DISABLE;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_NONE:
            *out = VTSS_SERDES_MODE_NONE;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_XAUI_12G:
            *out = VTSS_SERDES_MODE_XAUI_12G;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_XAUI:
            *out = VTSS_SERDES_MODE_XAUI;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_RXAUI:
            *out = VTSS_SERDES_MODE_RXAUI;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_RXAUI_12G:
            *out = VTSS_SERDES_MODE_RXAUI_12G;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_2G5:
            *out = VTSS_SERDES_MODE_2G5;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_QSGMII:
            *out = VTSS_SERDES_MODE_QSGMII;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_SGMII:
            *out = VTSS_SERDES_MODE_SGMII;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_100FX:
            *out = VTSS_SERDES_MODE_100FX;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_1000BaseX:
            *out = VTSS_SERDES_MODE_1000BaseX;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_SFI:
            *out = VTSS_SERDES_MODE_SFI;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_SFI_DAC:
            *out = VTSS_SERDES_MODE_SFI_DAC;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_SFI_SR:
            *out = VTSS_SERDES_MODE_SFI_SR;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_SFI_ZR:
            *out = VTSS_SERDES_MODE_SFI_ZR;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_SFI_BP:
            *out = VTSS_SERDES_MODE_SFI_BP;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_SFI_B2B:
            *out = VTSS_SERDES_MODE_SFI_B2B;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_SFI_PR_NONE:
            *out = VTSS_SERDES_MODE_SFI_PR_NONE;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_IDLE:
            *out = VTSS_SERDES_MODE_IDLE;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_TEST_MODE:
            *out = VTSS_SERDES_MODE_TEST_MODE;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_USGMII:
            *out = VTSS_SERDES_MODE_USGMII;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_QXGMII:
            *out = VTSS_SERDES_MODE_QXGMII;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_DXGMII_10G:
            *out = VTSS_SERDES_MODE_DXGMII_10G;
            return VTSS_RC_OK;
        case MESA_SERDES_MODE_DXGMII_5G:
            *out = VTSS_SERDES_MODE_DXGMII_5G;
            return VTSS_RC_OK;
        default:
            return VTSS_RC_ERROR;
    }
}

#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V1)
mesa_rc mesa_afi_free(const mesa_inst_t inst, mesa_afi_id_t id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_afi_free((const vtss_inst_t)inst /* ag.rb:1847 */, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_afi_free(const mesa_inst_t inst, mesa_afi_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_AFI_V1) /* ag.rb:1965 */
#else
mesa_rc mesa_afi_free(const mesa_inst_t inst, mesa_afi_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_AFI_SWC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V1)
mesa_rc mesa_afi_hijack(const mesa_inst_t inst, mesa_afi_id_t id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_afi_hijack((const vtss_inst_t)inst /* ag.rb:1847 */, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_afi_hijack(const mesa_inst_t inst, mesa_afi_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_AFI_V1) /* ag.rb:1965 */
#else
mesa_rc mesa_afi_hijack(const mesa_inst_t inst, mesa_afi_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_AFI_SWC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
mesa_rc mesa_afi_fast_inj_alloc(const mesa_inst_t inst, const mesa_afi_fast_inj_alloc_cfg_t *const cfg, mesa_afi_fastid_t *const fastid)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_afi_fast_inj_alloc_cfg_t) == sizeof(mesa_afi_fast_inj_alloc_cfg_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_afi_fast_inj_alloc((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_afi_fast_inj_alloc_cfg_t *)cfg /* ag.rb:1844 */, fastid); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_afi_fast_inj_alloc(const mesa_inst_t inst, const mesa_afi_fast_inj_alloc_cfg_t *const cfg, mesa_afi_fastid_t *const fastid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_AFI_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_afi_fast_inj_alloc(const mesa_inst_t inst, const mesa_afi_fast_inj_alloc_cfg_t *const cfg, mesa_afi_fastid_t *const fastid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_AFI_SWC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
mesa_rc mesa_afi_fast_inj_free(const mesa_inst_t inst, mesa_afi_fastid_t fastid)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_afi_fast_inj_free((const vtss_inst_t)inst /* ag.rb:1847 */, fastid); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_afi_fast_inj_free(const mesa_inst_t inst, mesa_afi_fastid_t fastid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_AFI_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_afi_fast_inj_free(const mesa_inst_t inst, mesa_afi_fastid_t fastid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_AFI_SWC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
mesa_rc mesa_afi_fast_inj_frm_hijack(const mesa_inst_t inst, mesa_afi_fastid_t fastid, const mesa_afi_fast_inj_frm_cfg_t *const cfg)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_afi_fast_inj_frm_cfg_t) == sizeof(mesa_afi_fast_inj_frm_cfg_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_afi_fast_inj_frm_hijack((const vtss_inst_t)inst /* ag.rb:1847 */, fastid, (const vtss_afi_fast_inj_frm_cfg_t *)cfg /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_afi_fast_inj_frm_hijack(const mesa_inst_t inst, mesa_afi_fastid_t fastid, const mesa_afi_fast_inj_frm_cfg_t *const cfg)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_AFI_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_afi_fast_inj_frm_hijack(const mesa_inst_t inst, mesa_afi_fastid_t fastid, const mesa_afi_fast_inj_frm_cfg_t *const cfg)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_AFI_SWC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
mesa_rc mesa_afi_fast_inj_stop(const mesa_inst_t inst, mesa_afi_fastid_t fastid)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_afi_fast_inj_stop((const vtss_inst_t)inst /* ag.rb:1847 */, fastid); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_afi_fast_inj_stop(const mesa_inst_t inst, mesa_afi_fastid_t fastid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_AFI_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_afi_fast_inj_stop(const mesa_inst_t inst, mesa_afi_fastid_t fastid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_AFI_SWC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
mesa_rc mesa_afi_fast_inj_seq_cnt_get(const mesa_inst_t inst, mesa_afi_fastid_t fastid, uint32_t *const seq_cnt)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_afi_fast_inj_seq_cnt_get((const vtss_inst_t)inst /* ag.rb:1847 */, fastid, seq_cnt); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_afi_fast_inj_seq_cnt_get(const mesa_inst_t inst, mesa_afi_fastid_t fastid, uint32_t *const seq_cnt)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_AFI_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_afi_fast_inj_seq_cnt_get(const mesa_inst_t inst, mesa_afi_fastid_t fastid, uint32_t *const seq_cnt)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_AFI_SWC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
mesa_rc mesa_afi_slow_inj_alloc(const mesa_inst_t inst, const mesa_afi_slow_inj_alloc_cfg_t *const cfg, mesa_afi_slowid_t *const slowid)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_afi_slow_inj_alloc_cfg_t) == sizeof(mesa_afi_slow_inj_alloc_cfg_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_afi_slow_inj_alloc((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_afi_slow_inj_alloc_cfg_t *)cfg /* ag.rb:1844 */, slowid); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_afi_slow_inj_alloc(const mesa_inst_t inst, const mesa_afi_slow_inj_alloc_cfg_t *const cfg, mesa_afi_slowid_t *const slowid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_AFI_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_afi_slow_inj_alloc(const mesa_inst_t inst, const mesa_afi_slow_inj_alloc_cfg_t *const cfg, mesa_afi_slowid_t *const slowid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_AFI_SWC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
mesa_rc mesa_afi_slow_inj_free(const mesa_inst_t inst, mesa_afi_slowid_t slowid)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_afi_slow_inj_free((const vtss_inst_t)inst /* ag.rb:1847 */, slowid); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_afi_slow_inj_free(const mesa_inst_t inst, mesa_afi_slowid_t slowid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_AFI_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_afi_slow_inj_free(const mesa_inst_t inst, mesa_afi_slowid_t slowid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_AFI_SWC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
mesa_rc mesa_afi_slow_inj_frm_hijack(const mesa_inst_t inst, mesa_afi_slowid_t slowid)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_afi_slow_inj_frm_hijack((const vtss_inst_t)inst /* ag.rb:1847 */, slowid); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_afi_slow_inj_frm_hijack(const mesa_inst_t inst, mesa_afi_slowid_t slowid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_AFI_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_afi_slow_inj_frm_hijack(const mesa_inst_t inst, mesa_afi_slowid_t slowid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_AFI_SWC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
mesa_rc mesa_afi_slow_inj_start(const mesa_inst_t inst, mesa_afi_slowid_t slowid, const mesa_afi_slow_inj_start_cfg_t *const cfg)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_afi_slow_inj_start_cfg_t) == sizeof(mesa_afi_slow_inj_start_cfg_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_afi_slow_inj_start((const vtss_inst_t)inst /* ag.rb:1847 */, slowid, (const vtss_afi_slow_inj_start_cfg_t *)cfg /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_afi_slow_inj_start(const mesa_inst_t inst, mesa_afi_slowid_t slowid, const mesa_afi_slow_inj_start_cfg_t *const cfg)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_AFI_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_afi_slow_inj_start(const mesa_inst_t inst, mesa_afi_slowid_t slowid, const mesa_afi_slow_inj_start_cfg_t *const cfg)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_AFI_SWC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
mesa_rc mesa_afi_slow_inj_stop(const mesa_inst_t inst, mesa_afi_slowid_t slowid)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_afi_slow_inj_stop((const vtss_inst_t)inst /* ag.rb:1847 */, slowid); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_afi_slow_inj_stop(const mesa_inst_t inst, mesa_afi_slowid_t slowid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_AFI_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_afi_slow_inj_stop(const mesa_inst_t inst, mesa_afi_slowid_t slowid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_AFI_SWC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
mesa_rc mesa_afi_port_start(const mesa_inst_t inst, mesa_port_no_t port_no)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_afi_port_start((const vtss_inst_t)inst /* ag.rb:1847 */, port_no); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_afi_port_start(const mesa_inst_t inst, mesa_port_no_t port_no)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_AFI_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_afi_port_start(const mesa_inst_t inst, mesa_port_no_t port_no)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_AFI_SWC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_AFI_SWC)
#if defined(VTSS_AFI_V2)
mesa_rc mesa_afi_port_stop(const mesa_inst_t inst, mesa_port_no_t port_no)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_afi_port_stop((const vtss_inst_t)inst /* ag.rb:1847 */, port_no); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_afi_port_stop(const mesa_inst_t inst, mesa_port_no_t port_no)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_AFI_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_afi_port_stop(const mesa_inst_t inst, mesa_port_no_t port_no)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_AFI_SWC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_rd(const mesa_inst_t inst, const uint32_t addr, uint32_t *const value)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_rd((const vtss_inst_t)inst /* ag.rb:1847 */, addr, value); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_rd(const mesa_inst_t inst, const uint32_t addr, uint32_t *const value)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_wr(const mesa_inst_t inst, const uint32_t addr, const uint32_t value)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_wr((const vtss_inst_t)inst /* ag.rb:1847 */, addr, value); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_wr(const mesa_inst_t inst, const uint32_t addr, const uint32_t value)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_wrm(const mesa_inst_t inst, const uint32_t addr, const uint32_t value, const uint32_t mask)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_wrm((const vtss_inst_t)inst /* ag.rb:1847 */, addr, value, mask); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_wrm(const mesa_inst_t inst, const uint32_t addr, const uint32_t value, const uint32_t mask)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_global_enable_set(const mesa_inst_t inst, const mesa_clock_global_enable_t ena)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_global_enable_set((const vtss_inst_t)inst /* ag.rb:1847 */, ena); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_global_enable_set(const mesa_inst_t inst, const mesa_clock_global_enable_t ena)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_global_enable_get(const mesa_inst_t inst, mesa_clock_global_enable_t * ena)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_global_enable_get((const vtss_inst_t)inst /* ag.rb:1847 */, ena); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_global_enable_get(const mesa_inst_t inst, mesa_clock_global_enable_t * ena)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_global_sw_reset(const mesa_inst_t inst)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_global_sw_reset((const vtss_inst_t)inst /* ag.rb:1847 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_global_sw_reset(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_shutdown(const mesa_inst_t inst)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_shutdown((const vtss_inst_t)inst /* ag.rb:1847 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_shutdown(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_selection_mode_set(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, const mesa_clock_selection_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_selection_conf_t) == sizeof(mesa_clock_selection_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_selection_mode_set((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, (const vtss_clock_selection_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_selection_mode_set(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, const mesa_clock_selection_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_selection_mode_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, mesa_clock_selection_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_selection_conf_t) == sizeof(mesa_clock_selection_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_selection_mode_get((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, (vtss_clock_selection_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_selection_mode_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, mesa_clock_selection_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_operation_conf_set(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, const mesa_clock_dpll_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_dpll_conf_t) == sizeof(mesa_clock_dpll_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_operation_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, (const vtss_clock_dpll_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_operation_conf_set(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, const mesa_clock_dpll_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_operation_conf_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, mesa_clock_dpll_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_dpll_conf_t) == sizeof(mesa_clock_dpll_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_operation_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, (vtss_clock_dpll_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_operation_conf_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, mesa_clock_dpll_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_ho_stack_conf_set(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, const mesa_clock_ho_stack_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_ho_stack_conf_t) == sizeof(mesa_clock_ho_stack_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_ho_stack_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, (const vtss_clock_ho_stack_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_ho_stack_conf_set(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, const mesa_clock_ho_stack_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_ho_stack_conf_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, mesa_clock_ho_stack_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_ho_stack_conf_t) == sizeof(mesa_clock_ho_stack_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_ho_stack_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, (vtss_clock_ho_stack_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_ho_stack_conf_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, mesa_clock_ho_stack_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_ho_stack_content_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, mesa_clock_ho_stack_content_t *const cont)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_ho_stack_content_t) == sizeof(mesa_clock_ho_stack_content_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_ho_stack_content_get((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, (vtss_clock_ho_stack_content_t *)cont /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_ho_stack_content_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, mesa_clock_ho_stack_content_t *const cont)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_dco_frequency_offset_set(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, const int64_t offset)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_dco_frequency_offset_set((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, offset); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_dco_frequency_offset_set(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, const int64_t offset)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_dco_frequency_offset_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, int64_t *const offset)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_dco_frequency_offset_get((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, offset); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_dco_frequency_offset_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, int64_t *const offset)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_output_filter_bw_set(const mesa_inst_t inst, const uint8_t clock_output, const uint32_t bw_100uhz)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_output_filter_bw_set((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, bw_100uhz); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_output_filter_bw_set(const mesa_inst_t inst, const uint8_t clock_output, const uint32_t bw_100uhz)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_output_filter_bw_get(const mesa_inst_t inst, const uint8_t clock_output, uint32_t * bw_100uhz)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_output_filter_bw_get((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, bw_100uhz); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_output_filter_bw_get(const mesa_inst_t inst, const uint8_t clock_output, uint32_t * bw_100uhz)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_output_filter_lock_fast_set(const mesa_inst_t inst, const uint8_t clock_output)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_output_filter_lock_fast_set((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_output_filter_lock_fast_set(const mesa_inst_t inst, const uint8_t clock_output)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_output_filter_lock_fast_get(const mesa_inst_t inst, const uint8_t clock_output, mesa_bool_t * lock_completed)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_output_filter_lock_fast_get((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, lock_completed); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_output_filter_lock_fast_get(const mesa_inst_t inst, const uint8_t clock_output, mesa_bool_t * lock_completed)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_output_psl_conf_set(const mesa_inst_t inst, const uint8_t clock_output, const mesa_clock_psl_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_psl_conf_t) == sizeof(mesa_clock_psl_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_output_psl_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, (const vtss_clock_psl_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_output_psl_conf_set(const mesa_inst_t inst, const uint8_t clock_output, const mesa_clock_psl_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_output_psl_conf_get(const mesa_inst_t inst, const uint8_t clock_output, mesa_clock_psl_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_psl_conf_t) == sizeof(mesa_clock_psl_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_output_psl_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, (vtss_clock_psl_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_output_psl_conf_get(const mesa_inst_t inst, const uint8_t clock_output, mesa_clock_psl_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_adj_frequency_set(const mesa_inst_t inst, const uint8_t clock_output, const int64_t adj)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_adj_frequency_set((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, adj); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_adj_frequency_set(const mesa_inst_t inst, const uint8_t clock_output, const int64_t adj)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_adj_frequency_get(const mesa_inst_t inst, const uint8_t clock_output, int64_t *const adj)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_adj_frequency_get((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, adj); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_adj_frequency_get(const mesa_inst_t inst, const uint8_t clock_output, int64_t *const adj)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_adj_phase_set(const mesa_inst_t inst, const uint8_t clock_output, const int32_t adj)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_adj_phase_set((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, adj); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_adj_phase_set(const mesa_inst_t inst, const uint8_t clock_output, const int32_t adj)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_adj_phase_get(const mesa_inst_t inst, mesa_bool_t *const adj_ongoing)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_adj_phase_get((const vtss_inst_t)inst /* ag.rb:1847 */, adj_ongoing); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_adj_phase_get(const mesa_inst_t inst, mesa_bool_t *const adj_ongoing)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_priority_set(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, const uint8_t clock_input, const mesa_clock_priority_selector_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_priority_selector_t) == sizeof(mesa_clock_priority_selector_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_priority_set((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, clock_input, (const vtss_clock_priority_selector_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_priority_set(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, const uint8_t clock_input, const mesa_clock_priority_selector_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_priority_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, const uint8_t clock_input, mesa_clock_priority_selector_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_priority_selector_t) == sizeof(mesa_clock_priority_selector_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_priority_get((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, clock_input, (vtss_clock_priority_selector_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_priority_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, const uint8_t clock_input, mesa_clock_priority_selector_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_input_frequency_set(const mesa_inst_t inst, const uint8_t clock_input, const uint32_t freq_khz, const mesa_bool_t use_internal_clock_src)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_input_frequency_set((const vtss_inst_t)inst /* ag.rb:1847 */, clock_input, freq_khz, use_internal_clock_src); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_input_frequency_set(const mesa_inst_t inst, const uint8_t clock_input, const uint32_t freq_khz, const mesa_bool_t use_internal_clock_src)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_input_frequency_get(const mesa_inst_t inst, const uint8_t clock_input, uint32_t *const freq_khz, mesa_bool_t *const use_internal_clock_src)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_input_frequency_get((const vtss_inst_t)inst /* ag.rb:1847 */, clock_input, freq_khz, use_internal_clock_src); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_input_frequency_get(const mesa_inst_t inst, const uint8_t clock_input, uint32_t *const freq_khz, mesa_bool_t *const use_internal_clock_src)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_input_frequency_ratio_set(const mesa_inst_t inst, const uint8_t clock_input, const uint32_t freq_khz, const mesa_clock_ratio_t *const ratio, const mesa_bool_t use_internal_clock_src)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_ratio_t) == sizeof(mesa_clock_ratio_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_input_frequency_ratio_set((const vtss_inst_t)inst /* ag.rb:1847 */, clock_input, freq_khz, (const vtss_clock_ratio_t *)ratio /* ag.rb:1844 */, use_internal_clock_src); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_input_frequency_ratio_set(const mesa_inst_t inst, const uint8_t clock_input, const uint32_t freq_khz, const mesa_clock_ratio_t *const ratio, const mesa_bool_t use_internal_clock_src)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_input_frequency_ratio_get(const mesa_inst_t inst, const uint8_t clock_input, uint32_t *const freq_khz, mesa_clock_ratio_t *const ratio, mesa_bool_t *const use_internal_clock_src)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_ratio_t) == sizeof(mesa_clock_ratio_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_input_frequency_ratio_get((const vtss_inst_t)inst /* ag.rb:1847 */, clock_input, freq_khz, (vtss_clock_ratio_t *)ratio /* ag.rb:1844 */, use_internal_clock_src); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_input_frequency_ratio_get(const mesa_inst_t inst, const uint8_t clock_input, uint32_t *const freq_khz, mesa_clock_ratio_t *const ratio, mesa_bool_t *const use_internal_clock_src)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_output_frequency_set(const mesa_inst_t inst, const uint8_t clock_output, const uint32_t freq_khz, const uint32_t par_freq_khz)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_output_frequency_set((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, freq_khz, par_freq_khz); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_output_frequency_set(const mesa_inst_t inst, const uint8_t clock_output, const uint32_t freq_khz, const uint32_t par_freq_khz)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_output_frequency_get(const mesa_inst_t inst, const uint8_t clock_output, uint32_t *const freq_khz, uint32_t *const par_freq_khz)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_output_frequency_get((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, freq_khz, par_freq_khz); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_output_frequency_get(const mesa_inst_t inst, const uint8_t clock_output, uint32_t *const freq_khz, uint32_t *const par_freq_khz)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_output_frequency_ratio_set(const mesa_inst_t inst, const uint8_t clock_output, const uint32_t freq_khz, const uint32_t par_freq_khz, const mesa_clock_ratio_t *const ratio)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_ratio_t) == sizeof(mesa_clock_ratio_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_output_frequency_ratio_set((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, freq_khz, par_freq_khz, (const vtss_clock_ratio_t *)ratio /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_output_frequency_ratio_set(const mesa_inst_t inst, const uint8_t clock_output, const uint32_t freq_khz, const uint32_t par_freq_khz, const mesa_clock_ratio_t *const ratio)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_output_frequency_ratio_get(const mesa_inst_t inst, const uint8_t clock_output, uint32_t *const freq_khz, uint32_t *const par_freq_khz, mesa_clock_ratio_t *const ratio)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_ratio_t) == sizeof(mesa_clock_ratio_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_output_frequency_ratio_get((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, freq_khz, par_freq_khz, (vtss_clock_ratio_t *)ratio /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_output_frequency_ratio_get(const mesa_inst_t inst, const uint8_t clock_output, uint32_t *const freq_khz, uint32_t *const par_freq_khz, mesa_clock_ratio_t *const ratio)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_output_level_set(const mesa_inst_t inst, const uint8_t clock_output, const uint16_t level_mv)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_output_level_set((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, level_mv); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_output_level_set(const mesa_inst_t inst, const uint8_t clock_output, const uint16_t level_mv)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_output_level_get(const mesa_inst_t inst, const uint8_t clock_output, uint16_t *const level_mv)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_output_level_get((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, level_mv); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_output_level_get(const mesa_inst_t inst, const uint8_t clock_output, uint16_t *const level_mv)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_output_selector_set(const mesa_inst_t inst, const uint8_t clock_output, const mesa_clock_input_selector_t *const input)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_input_selector_t) == sizeof(mesa_clock_input_selector_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_output_selector_set((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, (const vtss_clock_input_selector_t *)input /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_output_selector_set(const mesa_inst_t inst, const uint8_t clock_output, const mesa_clock_input_selector_t *const input)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_output_selector_get(const mesa_inst_t inst, const uint8_t clock_output, mesa_clock_input_selector_t *const input)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_input_selector_t) == sizeof(mesa_clock_input_selector_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_output_selector_get((const vtss_inst_t)inst /* ag.rb:1847 */, clock_output, (vtss_clock_input_selector_t *)input /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_output_selector_get(const mesa_inst_t inst, const uint8_t clock_output, mesa_clock_input_selector_t *const input)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_input_alarm_conf_set(const mesa_inst_t inst, const uint8_t clock_input, const mesa_clock_input_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_input_conf_t) == sizeof(mesa_clock_input_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_input_alarm_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, clock_input, (const vtss_clock_input_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_input_alarm_conf_set(const mesa_inst_t inst, const uint8_t clock_input, const mesa_clock_input_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_input_alarm_conf_get(const mesa_inst_t inst, const uint8_t clock_input, mesa_clock_input_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_input_conf_t) == sizeof(mesa_clock_input_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_input_alarm_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, clock_input, (vtss_clock_input_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_input_alarm_conf_get(const mesa_inst_t inst, const uint8_t clock_input, mesa_clock_input_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_input_cfm_conf_set(const mesa_inst_t inst, const uint8_t clock_input, const mesa_clock_cfm_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_cfm_conf_t) == sizeof(mesa_clock_cfm_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_input_cfm_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, clock_input, (const vtss_clock_cfm_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_input_cfm_conf_set(const mesa_inst_t inst, const uint8_t clock_input, const mesa_clock_cfm_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_input_cfm_conf_get(const mesa_inst_t inst, const uint8_t clock_input, mesa_clock_cfm_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_cfm_conf_t) == sizeof(mesa_clock_cfm_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_input_cfm_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, clock_input, (vtss_clock_cfm_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_input_cfm_conf_get(const mesa_inst_t inst, const uint8_t clock_input, mesa_clock_cfm_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_input_pfm_conf_set(const mesa_inst_t inst, const uint8_t clock_input, const mesa_clock_pfm_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_pfm_conf_t) == sizeof(mesa_clock_pfm_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_input_pfm_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, clock_input, (const vtss_clock_pfm_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_input_pfm_conf_set(const mesa_inst_t inst, const uint8_t clock_input, const mesa_clock_pfm_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_input_pfm_conf_get(const mesa_inst_t inst, const uint8_t clock_input, mesa_clock_pfm_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_pfm_conf_t) == sizeof(mesa_clock_pfm_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_input_pfm_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, clock_input, (vtss_clock_pfm_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_input_pfm_conf_get(const mesa_inst_t inst, const uint8_t clock_input, mesa_clock_pfm_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_input_gst_conf_set(const mesa_inst_t inst, const uint8_t clock_input, const mesa_clock_gst_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_gst_conf_t) == sizeof(mesa_clock_gst_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_input_gst_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, clock_input, (const vtss_clock_gst_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_input_gst_conf_set(const mesa_inst_t inst, const uint8_t clock_input, const mesa_clock_gst_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_input_gst_conf_get(const mesa_inst_t inst, const uint8_t clock_input, mesa_clock_gst_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_gst_conf_t) == sizeof(mesa_clock_gst_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_input_gst_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, clock_input, (vtss_clock_gst_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_input_gst_conf_get(const mesa_inst_t inst, const uint8_t clock_input, mesa_clock_gst_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_selector_state_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, mesa_clock_selector_state_t *const selector_state, uint8_t *const clock_input)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_selector_state_t) == sizeof(mesa_clock_selector_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_selector_state_get((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, (vtss_clock_selector_state_t *)selector_state /* ag.rb:1844 */, clock_input); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_selector_state_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, mesa_clock_selector_state_t *const selector_state, uint8_t *const clock_input)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_dpll_state_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, mesa_clock_dpll_state_t *const pll_state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_dpll_state_t) == sizeof(mesa_clock_dpll_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_dpll_state_get((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, (vtss_clock_dpll_state_t *)pll_state /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_dpll_state_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, mesa_clock_dpll_state_t *const pll_state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_ho_stack_frequency_offset_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, int64_t *const offset)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_ho_stack_frequency_offset_get((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, offset); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_ho_stack_frequency_offset_get(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, int64_t *const offset)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_input_state_get(const mesa_inst_t inst, const uint8_t clock_input, mesa_clock_input_state_t *const input_state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_clock_input_state_t) == sizeof(mesa_clock_input_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_clock_input_state_get((const vtss_inst_t)inst /* ag.rb:1847 */, clock_input, (vtss_clock_input_state_t *)input_state /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_input_state_get(const mesa_inst_t inst, const uint8_t clock_input, mesa_clock_input_state_t *const input_state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_input_event_poll(const mesa_inst_t inst, const uint8_t clock_input, mesa_clock_input_event_type_t *const ev_mask)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_input_event_poll((const vtss_inst_t)inst /* ag.rb:1847 */, clock_input, ev_mask); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_input_event_poll(const mesa_inst_t inst, const uint8_t clock_input, mesa_clock_input_event_type_t *const ev_mask)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_input_event_enable(const mesa_inst_t inst, const uint8_t clock_input, const mesa_clock_input_event_type_t ev_mask, const mesa_bool_t enable)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_input_event_enable((const vtss_inst_t)inst /* ag.rb:1847 */, clock_input, ev_mask, enable); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_input_event_enable(const mesa_inst_t inst, const uint8_t clock_input, const mesa_clock_input_event_type_t ev_mask, const mesa_bool_t enable)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_dpll_event_poll(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, mesa_clock_dpll_event_type_t *const ev_mask)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_dpll_event_poll((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, ev_mask); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_dpll_event_poll(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, mesa_clock_dpll_event_type_t *const ev_mask)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_CLOCK)
mesa_rc mesa_clock_dpll_event_enable(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, const mesa_clock_dpll_event_type_t ev_mask, const mesa_bool_t enable)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_clock_dpll_event_enable((const vtss_inst_t)inst /* ag.rb:1847 */, dpll, ev_mask, enable); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_clock_dpll_event_enable(const mesa_inst_t inst, const mesa_clock_dpll_inst_t dpll, const mesa_clock_dpll_event_type_t ev_mask, const mesa_bool_t enable)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_CLOCK) /* ag.rb:1965 */

mesa_rc mesa_inst_get(const mesa_target_type_t target, mesa_inst_create_t *const create)
{ /* ag.rb:1822 */
    vtss_target_type_t __target;
    vtss_inst_create_t __create;

    memset(&__target, 0, sizeof(__target));
    memset(&__create, 0, sizeof(__create));

    __RC(mesa_conv_mesa_target_type_t_to_vtss_target_type_t(&target, &__target)) /* ag.rb:1893 */; /* ag.rb:1941 */

    mesa_rc rc = vtss_inst_get(__target, &__create); /* ag.rb:1946 */

    __RC(mesa_conv_vtss_inst_create_t_to_mesa_inst_create_t(&__create, create)) /* ag.rb:1893 */; /* ag.rb:1948 */

    return rc; /* ag.rb:1950 */
}

mesa_rc mesa_inst_create(const mesa_inst_create_t *const create, mesa_inst_t *const inst)
{ /* ag.rb:1822 */
    vtss_inst_create_t __create;
    memset(&__create, 0, sizeof(__create));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_inst_create_t_to_vtss_inst_create_t(create, &__create)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_inst_create(&__create, (vtss_inst_t *)inst /* ag.rb:1844 */); /* ag.rb:1952*/ 
}

mesa_rc mesa_inst_destroy(const mesa_inst_t inst)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_inst_destroy((const vtss_inst_t)inst /* ag.rb:1847 */); /* ag.rb:1952*/ 
}

mesa_rc mesa_init_conf_get(const mesa_inst_t inst, mesa_init_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_init_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_init_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_init_conf_t_to_mesa_init_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}

mesa_rc mesa_init_conf_set(const mesa_inst_t inst, const mesa_init_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_init_conf_t __conf;
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    vtss_init_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, &__conf);  /* ag.rb:1938 */ // get-before-set
    __RC(mesa_conv_mesa_init_conf_t_to_vtss_init_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_init_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, &__conf); /* ag.rb:1952*/ 
}

mesa_rc mesa_register_access_mode_set(const mesa_inst_t inst, mesa_bool_t spi_bus)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_register_access_mode_set((const vtss_inst_t)inst /* ag.rb:1847 */, spi_bus); /* ag.rb:1952*/ 
}

mesa_rc mesa_register_access_mode_get(const mesa_inst_t inst, mesa_bool_t * spi_bus)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_register_access_mode_get((const vtss_inst_t)inst /* ag.rb:1847 */, spi_bus); /* ag.rb:1952*/ 
}

mesa_rc mesa_spi_slave_init(const mesa_spi_slave_init_t *const conf)
{ /* ag.rb:1822 */
    vtss_spi_slave_init_t __conf;
    memset(&__conf, 0, sizeof(__conf));
    __RC(mesa_conv_mesa_spi_slave_init_t_to_vtss_spi_slave_init_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_spi_slave_init(&__conf); /* ag.rb:1952*/ 
}

#if defined(VTSS_FEATURE_WARM_START)
mesa_rc mesa_restart_conf_end(const mesa_inst_t inst)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_restart_conf_end((const vtss_inst_t)inst /* ag.rb:1847 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_restart_conf_end(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_WARM_START) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_WARM_START)
mesa_rc mesa_restart_status_get(const mesa_inst_t inst, mesa_restart_status_t *const status)
{ /* ag.rb:1822 */
    vtss_restart_status_t __status;
    memset(&__status, 0, sizeof(__status));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_restart_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, &__status); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_restart_status_t_to_mesa_restart_status_t(&__status, status)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_restart_status_get(const mesa_inst_t inst, mesa_restart_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_WARM_START) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_WARM_START)
mesa_rc mesa_restart_conf_get(const mesa_inst_t inst, mesa_restart_t *const restart)
{ /* ag.rb:1822 */
    vtss_restart_t __restart;
    memset(&__restart, 0, sizeof(__restart));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_restart_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, &__restart); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_restart_t_to_mesa_restart_t(&__restart, restart)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_restart_conf_get(const mesa_inst_t inst, mesa_restart_t *const restart)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_WARM_START) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_WARM_START)
mesa_rc mesa_restart_conf_set(const mesa_inst_t inst, const mesa_restart_t restart)
{ /* ag.rb:1822 */
    vtss_restart_t __restart;
    memset(&__restart, 0, sizeof(__restart));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_restart_t_to_vtss_restart_t(&restart, &__restart)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_restart_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, __restart); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_restart_conf_set(const mesa_inst_t inst, const mesa_restart_t restart)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_WARM_START) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mac_table_add(const mesa_inst_t inst, const mesa_mac_table_entry_t *const entry)
{ /* ag.rb:1822 */
    vtss_mac_table_entry_t __entry;
    memset(&__entry, 0, sizeof(__entry));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_mac_table_entry_t_to_vtss_mac_table_entry_t(entry, &__entry)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_mac_table_add((const vtss_inst_t)inst /* ag.rb:1847 */, &__entry); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mac_table_add(const mesa_inst_t inst, const mesa_mac_table_entry_t *const entry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mac_table_del(const mesa_inst_t inst, const mesa_vid_mac_t *const vid_mac)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_vid_mac_t) == sizeof(mesa_vid_mac_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mac_table_del((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_vid_mac_t *)vid_mac /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mac_table_del(const mesa_inst_t inst, const mesa_vid_mac_t *const vid_mac)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mac_table_get(const mesa_inst_t inst, const mesa_vid_mac_t *const vid_mac, mesa_mac_table_entry_t *const entry)
{ /* ag.rb:1822 */
    vtss_mac_table_entry_t __entry;

    memset(&__entry, 0, sizeof(__entry));

#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_vid_mac_t) == sizeof(mesa_vid_mac_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    mesa_rc rc = vtss_mac_table_get((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_vid_mac_t *)vid_mac /* ag.rb:1844 */, &__entry); /* ag.rb:1946 */

    __RC(mesa_conv_vtss_mac_table_entry_t_to_mesa_mac_table_entry_t(&__entry, entry)) /* ag.rb:1893 */; /* ag.rb:1948 */

    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_mac_table_get(const mesa_inst_t inst, const mesa_vid_mac_t *const vid_mac, mesa_mac_table_entry_t *const entry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mac_table_get_next(const mesa_inst_t inst, const mesa_vid_mac_t *const vid_mac, mesa_mac_table_entry_t *const entry)
{ /* ag.rb:1822 */
    vtss_mac_table_entry_t __entry;

    memset(&__entry, 0, sizeof(__entry));

#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_vid_mac_t) == sizeof(mesa_vid_mac_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    mesa_rc rc = vtss_mac_table_get_next((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_vid_mac_t *)vid_mac /* ag.rb:1844 */, &__entry); /* ag.rb:1946 */

    __RC(mesa_conv_vtss_mac_table_entry_t_to_mesa_mac_table_entry_t(&__entry, entry)) /* ag.rb:1893 */; /* ag.rb:1948 */

    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_mac_table_get_next(const mesa_inst_t inst, const mesa_vid_mac_t *const vid_mac, mesa_mac_table_entry_t *const entry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mac_table_age_time_get(const mesa_inst_t inst, mesa_mac_table_age_time_t *const age_time)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mac_table_age_time_get((const vtss_inst_t)inst /* ag.rb:1847 */, age_time); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mac_table_age_time_get(const mesa_inst_t inst, mesa_mac_table_age_time_t *const age_time)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mac_table_age_time_set(const mesa_inst_t inst, const mesa_mac_table_age_time_t age_time)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mac_table_age_time_set((const vtss_inst_t)inst /* ag.rb:1847 */, age_time); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mac_table_age_time_set(const mesa_inst_t inst, const mesa_mac_table_age_time_t age_time)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mac_table_age(const mesa_inst_t inst)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mac_table_age((const vtss_inst_t)inst /* ag.rb:1847 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mac_table_age(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mac_table_vlan_age(const mesa_inst_t inst, const mesa_vid_t vid)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mac_table_vlan_age((const vtss_inst_t)inst /* ag.rb:1847 */, vid); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mac_table_vlan_age(const mesa_inst_t inst, const mesa_vid_t vid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mac_table_flush(const mesa_inst_t inst)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mac_table_flush((const vtss_inst_t)inst /* ag.rb:1847 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mac_table_flush(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mac_table_port_flush(const mesa_inst_t inst, const mesa_port_no_t port_no)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mac_table_port_flush((const vtss_inst_t)inst /* ag.rb:1847 */, port_no); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mac_table_port_flush(const mesa_inst_t inst, const mesa_port_no_t port_no)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mac_table_vlan_flush(const mesa_inst_t inst, const mesa_vid_t vid)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mac_table_vlan_flush((const vtss_inst_t)inst /* ag.rb:1847 */, vid); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mac_table_vlan_flush(const mesa_inst_t inst, const mesa_vid_t vid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mac_table_vlan_port_flush(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_vid_t vid)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mac_table_vlan_port_flush((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, vid); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mac_table_vlan_port_flush(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_vid_t vid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mac_table_status_get(const mesa_inst_t inst, mesa_mac_table_status_t *const status)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mac_table_status_t) == sizeof(mesa_mac_table_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mac_table_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_mac_table_status_t *)status /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mac_table_status_get(const mesa_inst_t inst, mesa_mac_table_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_learn_port_mode_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_learn_mode_t *const mode)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_learn_mode_t) == sizeof(mesa_learn_mode_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_learn_port_mode_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_learn_mode_t *)mode /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_learn_port_mode_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_learn_mode_t *const mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_learn_port_mode_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_learn_mode_t *const mode)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_learn_mode_t) == sizeof(mesa_learn_mode_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_learn_port_mode_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (const vtss_learn_mode_t *)mode /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_learn_port_mode_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_learn_mode_t *const mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_port_state_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_bool_t *const state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_port_state_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, state); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_state_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_bool_t *const state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_port_state_set(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_bool_t state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_port_state_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, state); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_state_set(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_bool_t state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_stp_port_state_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_stp_state_t *const state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_stp_state_t) == sizeof(mesa_stp_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_stp_port_state_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_stp_state_t *)state /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_stp_port_state_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_stp_state_t *const state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_stp_port_state_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_stp_state_t state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_stp_state_t) == sizeof(mesa_stp_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_stp_port_state_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, *((const vtss_stp_state_t *)&state) /* ag.rb:1849 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_stp_port_state_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_stp_state_t state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_L2_MSTP)
mesa_rc mesa_mstp_vlan_msti_get(const mesa_inst_t inst, const mesa_vid_t vid, mesa_msti_t *const msti)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mstp_vlan_msti_get((const vtss_inst_t)inst /* ag.rb:1847 */, vid, msti); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mstp_vlan_msti_get(const mesa_inst_t inst, const mesa_vid_t vid, mesa_msti_t *const msti)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_L2_MSTP) /* ag.rb:1965 */
#else
mesa_rc mesa_mstp_vlan_msti_get(const mesa_inst_t inst, const mesa_vid_t vid, mesa_msti_t *const msti)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_L2_MSTP)
mesa_rc mesa_mstp_vlan_msti_set(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_msti_t msti)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mstp_vlan_msti_set((const vtss_inst_t)inst /* ag.rb:1847 */, vid, msti); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mstp_vlan_msti_set(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_msti_t msti)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_L2_MSTP) /* ag.rb:1965 */
#else
mesa_rc mesa_mstp_vlan_msti_set(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_msti_t msti)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_L2_MSTP)
mesa_rc mesa_mstp_port_msti_state_get(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_msti_t msti, mesa_stp_state_t *const state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_stp_state_t) == sizeof(mesa_stp_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mstp_port_msti_state_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, msti, (vtss_stp_state_t *)state /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mstp_port_msti_state_get(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_msti_t msti, mesa_stp_state_t *const state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_L2_MSTP) /* ag.rb:1965 */
#else
mesa_rc mesa_mstp_port_msti_state_get(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_msti_t msti, mesa_stp_state_t *const state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_L2_MSTP)
mesa_rc mesa_mstp_port_msti_state_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_msti_t msti, const mesa_stp_state_t state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_stp_state_t) == sizeof(mesa_stp_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mstp_port_msti_state_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, msti, *((const vtss_stp_state_t *)&state) /* ag.rb:1849 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mstp_port_msti_state_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_msti_t msti, const mesa_stp_state_t state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_L2_MSTP) /* ag.rb:1965 */
#else
mesa_rc mesa_mstp_port_msti_state_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_msti_t msti, const mesa_stp_state_t state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_vlan_conf_get(const mesa_inst_t inst, mesa_vlan_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_vlan_conf_t) == sizeof(mesa_vlan_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_vlan_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_vlan_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vlan_conf_get(const mesa_inst_t inst, mesa_vlan_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_vlan_conf_set(const mesa_inst_t inst, const mesa_vlan_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_vlan_conf_t) == sizeof(mesa_vlan_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_vlan_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_vlan_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vlan_conf_set(const mesa_inst_t inst, const mesa_vlan_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_vlan_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_vlan_port_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_vlan_port_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_vlan_port_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_vlan_port_conf_t_to_mesa_vlan_port_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_vlan_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_vlan_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_vlan_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_vlan_port_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_vlan_port_conf_t __conf;
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    vtss_vlan_port_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf);  /* ag.rb:1938 */ // get-before-set
    __RC(mesa_conv_mesa_vlan_port_conf_t_to_vtss_vlan_port_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_vlan_port_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vlan_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_vlan_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_vlan_port_members_get(const mesa_inst_t inst, const mesa_vid_t vid, mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_vlan_port_members_get((const vtss_inst_t)inst /* ag.rb:1847 */, vid, __port_list); /* ag.rb:1946 */
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(__port_list, port_list)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_vlan_port_members_get(const mesa_inst_t inst, const mesa_vid_t vid, mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_vlan_port_members_set(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(port_list, __port_list)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_vlan_port_members_set((const vtss_inst_t)inst /* ag.rb:1847 */, vid, __port_list); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vlan_port_members_set(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_vlan_vid_conf_get(const mesa_inst_t inst, const mesa_vid_t vid, mesa_vlan_vid_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_vlan_vid_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_vlan_vid_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, vid, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_vlan_vid_conf_t_to_mesa_vlan_vid_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_vlan_vid_conf_get(const mesa_inst_t inst, const mesa_vid_t vid, mesa_vlan_vid_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_vlan_vid_conf_set(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_vlan_vid_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_vlan_vid_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_vlan_vid_conf_t_to_vtss_vlan_vid_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_vlan_vid_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, vid, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vlan_vid_conf_set(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_vlan_vid_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VLAN_COUNTERS)
mesa_rc mesa_vlan_counters_get(const mesa_inst_t inst, const mesa_vid_t vid, mesa_vlan_counters_t *const counters)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_vlan_counters_t) == sizeof(mesa_vlan_counters_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_vlan_counters_get((const vtss_inst_t)inst /* ag.rb:1847 */, vid, (vtss_vlan_counters_t *)counters /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vlan_counters_get(const mesa_inst_t inst, const mesa_vid_t vid, mesa_vlan_counters_t *const counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VLAN_COUNTERS) /* ag.rb:1965 */
#else
mesa_rc mesa_vlan_counters_get(const mesa_inst_t inst, const mesa_vid_t vid, mesa_vlan_counters_t *const counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VLAN_COUNTERS)
mesa_rc mesa_vlan_counters_clear(const mesa_inst_t inst, const mesa_vid_t vid)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_vlan_counters_clear((const vtss_inst_t)inst /* ag.rb:1847 */, vid); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vlan_counters_clear(const mesa_inst_t inst, const mesa_vid_t vid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VLAN_COUNTERS) /* ag.rb:1965 */
#else
mesa_rc mesa_vlan_counters_clear(const mesa_inst_t inst, const mesa_vid_t vid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_vcl_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_vcl_port_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_vcl_port_conf_t) == sizeof(mesa_vcl_port_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_vcl_port_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_vcl_port_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vcl_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_vcl_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VCAP) /* ag.rb:1965 */
#else
mesa_rc mesa_vcl_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_vcl_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_vcl_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_vcl_port_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_vcl_port_conf_t) == sizeof(mesa_vcl_port_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_vcl_port_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (const vtss_vcl_port_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vcl_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_vcl_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VCAP) /* ag.rb:1965 */
#else
mesa_rc mesa_vcl_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_vcl_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_vce_init(const mesa_inst_t inst, const mesa_vce_type_t type, mesa_vce_t *const vce)
{ /* ag.rb:1822 */
    vtss_vce_t __vce;

    memset(&__vce, 0, sizeof(__vce));

#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_vce_type_t) == sizeof(mesa_vce_type_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    mesa_rc rc = vtss_vce_init((const vtss_inst_t)inst /* ag.rb:1847 */, *((const vtss_vce_type_t *)&type) /* ag.rb:1849 */, &__vce); /* ag.rb:1946 */

    __RC(mesa_conv_vtss_vce_t_to_mesa_vce_t(&__vce, vce)) /* ag.rb:1893 */; /* ag.rb:1948 */

    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_vce_init(const mesa_inst_t inst, const mesa_vce_type_t type, mesa_vce_t *const vce)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VCAP) /* ag.rb:1965 */
#else
mesa_rc mesa_vce_init(const mesa_inst_t inst, const mesa_vce_type_t type, mesa_vce_t *const vce)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_vce_add(const mesa_inst_t inst, const mesa_vce_id_t vce_id, const mesa_vce_t *const vce)
{ /* ag.rb:1822 */
    vtss_vce_t __vce;
    memset(&__vce, 0, sizeof(__vce));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_vce_t_to_vtss_vce_t(vce, &__vce)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_vce_add((const vtss_inst_t)inst /* ag.rb:1847 */, vce_id, &__vce); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vce_add(const mesa_inst_t inst, const mesa_vce_id_t vce_id, const mesa_vce_t *const vce)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VCAP) /* ag.rb:1965 */
#else
mesa_rc mesa_vce_add(const mesa_inst_t inst, const mesa_vce_id_t vce_id, const mesa_vce_t *const vce)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_vce_del(const mesa_inst_t inst, const mesa_vce_id_t vce_id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_vce_del((const vtss_inst_t)inst /* ag.rb:1847 */, vce_id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vce_del(const mesa_inst_t inst, const mesa_vce_id_t vce_id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VCAP) /* ag.rb:1965 */
#else
mesa_rc mesa_vce_del(const mesa_inst_t inst, const mesa_vce_id_t vce_id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XSTAT)
mesa_rc mesa_ingress_cnt_alloc(const mesa_inst_t inst, const mesa_class_cnt_t cnt, mesa_ingress_cnt_id_t *const id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ingress_cnt_alloc((const vtss_inst_t)inst /* ag.rb:1847 */, cnt, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ingress_cnt_alloc(const mesa_inst_t inst, const mesa_class_cnt_t cnt, mesa_ingress_cnt_id_t *const id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XSTAT) /* ag.rb:1965 */
#else
mesa_rc mesa_ingress_cnt_alloc(const mesa_inst_t inst, const mesa_class_cnt_t cnt, mesa_ingress_cnt_id_t *const id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XSTAT)
mesa_rc mesa_ingress_cnt_free(const mesa_inst_t inst, const mesa_ingress_cnt_id_t id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ingress_cnt_free((const vtss_inst_t)inst /* ag.rb:1847 */, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ingress_cnt_free(const mesa_inst_t inst, const mesa_ingress_cnt_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XSTAT) /* ag.rb:1965 */
#else
mesa_rc mesa_ingress_cnt_free(const mesa_inst_t inst, const mesa_ingress_cnt_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XSTAT)
mesa_rc mesa_ingress_cnt_get(const mesa_inst_t inst, const mesa_ingress_cnt_id_t id, const mesa_cosid_t cosid, mesa_ingress_counters_t * counters)
{ /* ag.rb:1822 */
    vtss_ingress_counters_t __counters;
    memset(&__counters, 0, sizeof(__counters));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_ingress_cnt_get((const vtss_inst_t)inst /* ag.rb:1847 */, id, cosid, &__counters); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_ingress_counters_t_to_mesa_ingress_counters_t(&__counters, counters)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_ingress_cnt_get(const mesa_inst_t inst, const mesa_ingress_cnt_id_t id, const mesa_cosid_t cosid, mesa_ingress_counters_t * counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XSTAT) /* ag.rb:1965 */
#else
mesa_rc mesa_ingress_cnt_get(const mesa_inst_t inst, const mesa_ingress_cnt_id_t id, const mesa_cosid_t cosid, mesa_ingress_counters_t * counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XSTAT)
mesa_rc mesa_ingress_cnt_clear(const mesa_inst_t inst, const mesa_ingress_cnt_id_t id, const mesa_cosid_t cosid)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ingress_cnt_clear((const vtss_inst_t)inst /* ag.rb:1847 */, id, cosid); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ingress_cnt_clear(const mesa_inst_t inst, const mesa_ingress_cnt_id_t id, const mesa_cosid_t cosid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XSTAT) /* ag.rb:1965 */
#else
mesa_rc mesa_ingress_cnt_clear(const mesa_inst_t inst, const mesa_ingress_cnt_id_t id, const mesa_cosid_t cosid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XDLB)
mesa_rc mesa_dlb_policer_alloc(const mesa_inst_t inst, const mesa_class_cnt_t cnt, mesa_dlb_policer_id_t *const id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_dlb_policer_alloc((const vtss_inst_t)inst /* ag.rb:1847 */, cnt, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_dlb_policer_alloc(const mesa_inst_t inst, const mesa_class_cnt_t cnt, mesa_dlb_policer_id_t *const id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XDLB) /* ag.rb:1965 */
#else
mesa_rc mesa_dlb_policer_alloc(const mesa_inst_t inst, const mesa_class_cnt_t cnt, mesa_dlb_policer_id_t *const id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XDLB)
mesa_rc mesa_dlb_policer_free(const mesa_inst_t inst, const mesa_dlb_policer_id_t id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_dlb_policer_free((const vtss_inst_t)inst /* ag.rb:1847 */, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_dlb_policer_free(const mesa_inst_t inst, const mesa_dlb_policer_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XDLB) /* ag.rb:1965 */
#else
mesa_rc mesa_dlb_policer_free(const mesa_inst_t inst, const mesa_dlb_policer_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XDLB)
mesa_rc mesa_dlb_policer_conf_get(const mesa_inst_t inst, const mesa_dlb_policer_id_t id, const mesa_cosid_t cosid, mesa_dlb_policer_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_dlb_policer_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_dlb_policer_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, id, cosid, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_dlb_policer_conf_t_to_mesa_dlb_policer_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_dlb_policer_conf_get(const mesa_inst_t inst, const mesa_dlb_policer_id_t id, const mesa_cosid_t cosid, mesa_dlb_policer_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XDLB) /* ag.rb:1965 */
#else
mesa_rc mesa_dlb_policer_conf_get(const mesa_inst_t inst, const mesa_dlb_policer_id_t id, const mesa_cosid_t cosid, mesa_dlb_policer_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XDLB)
mesa_rc mesa_dlb_policer_conf_set(const mesa_inst_t inst, const mesa_dlb_policer_id_t id, const mesa_cosid_t cosid, const mesa_dlb_policer_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_dlb_policer_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_dlb_policer_conf_t_to_vtss_dlb_policer_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_dlb_policer_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, id, cosid, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_dlb_policer_conf_set(const mesa_inst_t inst, const mesa_dlb_policer_id_t id, const mesa_cosid_t cosid, const mesa_dlb_policer_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XDLB) /* ag.rb:1965 */
#else
mesa_rc mesa_dlb_policer_conf_set(const mesa_inst_t inst, const mesa_dlb_policer_id_t id, const mesa_cosid_t cosid, const mesa_dlb_policer_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XDLB)
#if defined(VTSS_FEATURE_PSFP)
mesa_rc mesa_dlb_policer_status_get(const mesa_inst_t inst, const mesa_dlb_policer_id_t id, const mesa_cosid_t cosid, mesa_dlb_policer_status_t *const status)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_dlb_policer_status_t) == sizeof(mesa_dlb_policer_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_dlb_policer_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, id, cosid, (vtss_dlb_policer_status_t *)status /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_dlb_policer_status_get(const mesa_inst_t inst, const mesa_dlb_policer_id_t id, const mesa_cosid_t cosid, mesa_dlb_policer_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PSFP) /* ag.rb:1965 */
#else
mesa_rc mesa_dlb_policer_status_get(const mesa_inst_t inst, const mesa_dlb_policer_id_t id, const mesa_cosid_t cosid, mesa_dlb_policer_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XDLB) /* ag.rb:1965 */
#else
mesa_rc mesa_dlb_policer_status_get(const mesa_inst_t inst, const mesa_dlb_policer_id_t id, const mesa_cosid_t cosid, mesa_dlb_policer_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XFLOW)
mesa_rc mesa_iflow_alloc(const mesa_inst_t inst, mesa_iflow_id_t *const id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_iflow_alloc((const vtss_inst_t)inst /* ag.rb:1847 */, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_iflow_alloc(const mesa_inst_t inst, mesa_iflow_id_t *const id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XFLOW) /* ag.rb:1965 */
#else
mesa_rc mesa_iflow_alloc(const mesa_inst_t inst, mesa_iflow_id_t *const id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XFLOW)
mesa_rc mesa_iflow_free(const mesa_inst_t inst, const mesa_iflow_id_t id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_iflow_free((const vtss_inst_t)inst /* ag.rb:1847 */, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_iflow_free(const mesa_inst_t inst, const mesa_iflow_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XFLOW) /* ag.rb:1965 */
#else
mesa_rc mesa_iflow_free(const mesa_inst_t inst, const mesa_iflow_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XFLOW)
mesa_rc mesa_iflow_conf_get(const mesa_inst_t inst, const mesa_iflow_id_t id, mesa_iflow_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_iflow_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_iflow_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, id, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_iflow_conf_t_to_mesa_iflow_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_iflow_conf_get(const mesa_inst_t inst, const mesa_iflow_id_t id, mesa_iflow_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XFLOW) /* ag.rb:1965 */
#else
mesa_rc mesa_iflow_conf_get(const mesa_inst_t inst, const mesa_iflow_id_t id, mesa_iflow_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XFLOW)
mesa_rc mesa_iflow_conf_set(const mesa_inst_t inst, const mesa_iflow_id_t id, const mesa_iflow_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_iflow_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_iflow_conf_t_to_vtss_iflow_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_iflow_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, id, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_iflow_conf_set(const mesa_inst_t inst, const mesa_iflow_id_t id, const mesa_iflow_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XFLOW) /* ag.rb:1965 */
#else
mesa_rc mesa_iflow_conf_set(const mesa_inst_t inst, const mesa_iflow_id_t id, const mesa_iflow_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_tce_init(const mesa_inst_t inst, mesa_tce_t *const tce)
{ /* ag.rb:1822 */
    vtss_tce_t __tce;
    memset(&__tce, 0, sizeof(__tce));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_tce_init((const vtss_inst_t)inst /* ag.rb:1847 */, &__tce); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_tce_t_to_mesa_tce_t(&__tce, tce)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_tce_init(const mesa_inst_t inst, mesa_tce_t *const tce)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VCAP) /* ag.rb:1965 */
#else
mesa_rc mesa_tce_init(const mesa_inst_t inst, mesa_tce_t *const tce)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_tce_add(const mesa_inst_t inst, const mesa_tce_id_t tce_id, const mesa_tce_t *const tce)
{ /* ag.rb:1822 */
    vtss_tce_t __tce;
    memset(&__tce, 0, sizeof(__tce));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_tce_t_to_vtss_tce_t(tce, &__tce)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_tce_add((const vtss_inst_t)inst /* ag.rb:1847 */, tce_id, &__tce); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_tce_add(const mesa_inst_t inst, const mesa_tce_id_t tce_id, const mesa_tce_t *const tce)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VCAP) /* ag.rb:1965 */
#else
mesa_rc mesa_tce_add(const mesa_inst_t inst, const mesa_tce_id_t tce_id, const mesa_tce_t *const tce)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
mesa_rc mesa_tce_del(const mesa_inst_t inst, const mesa_tce_id_t tce_id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_tce_del((const vtss_inst_t)inst /* ag.rb:1847 */, tce_id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_tce_del(const mesa_inst_t inst, const mesa_tce_id_t tce_id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VCAP) /* ag.rb:1965 */
#else
mesa_rc mesa_tce_del(const mesa_inst_t inst, const mesa_tce_id_t tce_id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XSTAT)
mesa_rc mesa_egress_cnt_alloc(const mesa_inst_t inst, const mesa_class_cnt_t cnt, mesa_egress_cnt_id_t *const id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_egress_cnt_alloc((const vtss_inst_t)inst /* ag.rb:1847 */, cnt, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_egress_cnt_alloc(const mesa_inst_t inst, const mesa_class_cnt_t cnt, mesa_egress_cnt_id_t *const id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XSTAT) /* ag.rb:1965 */
#else
mesa_rc mesa_egress_cnt_alloc(const mesa_inst_t inst, const mesa_class_cnt_t cnt, mesa_egress_cnt_id_t *const id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XSTAT)
mesa_rc mesa_egress_cnt_free(const mesa_inst_t inst, const mesa_egress_cnt_id_t id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_egress_cnt_free((const vtss_inst_t)inst /* ag.rb:1847 */, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_egress_cnt_free(const mesa_inst_t inst, const mesa_egress_cnt_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XSTAT) /* ag.rb:1965 */
#else
mesa_rc mesa_egress_cnt_free(const mesa_inst_t inst, const mesa_egress_cnt_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XSTAT)
mesa_rc mesa_egress_cnt_get(const mesa_inst_t inst, const mesa_egress_cnt_id_t id, const mesa_cosid_t cosid, mesa_egress_counters_t * counters)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_egress_counters_t) == sizeof(mesa_egress_counters_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_egress_cnt_get((const vtss_inst_t)inst /* ag.rb:1847 */, id, cosid, (vtss_egress_counters_t *)counters /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_egress_cnt_get(const mesa_inst_t inst, const mesa_egress_cnt_id_t id, const mesa_cosid_t cosid, mesa_egress_counters_t * counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XSTAT) /* ag.rb:1965 */
#else
mesa_rc mesa_egress_cnt_get(const mesa_inst_t inst, const mesa_egress_cnt_id_t id, const mesa_cosid_t cosid, mesa_egress_counters_t * counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XSTAT)
mesa_rc mesa_egress_cnt_clear(const mesa_inst_t inst, const mesa_egress_cnt_id_t id, const mesa_cosid_t cosid)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_egress_cnt_clear((const vtss_inst_t)inst /* ag.rb:1847 */, id, cosid); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_egress_cnt_clear(const mesa_inst_t inst, const mesa_egress_cnt_id_t id, const mesa_cosid_t cosid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XSTAT) /* ag.rb:1965 */
#else
mesa_rc mesa_egress_cnt_clear(const mesa_inst_t inst, const mesa_egress_cnt_id_t id, const mesa_cosid_t cosid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XFLOW)
mesa_rc mesa_eflow_alloc(const mesa_inst_t inst, mesa_eflow_id_t *const id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_eflow_alloc((const vtss_inst_t)inst /* ag.rb:1847 */, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_eflow_alloc(const mesa_inst_t inst, mesa_eflow_id_t *const id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XFLOW) /* ag.rb:1965 */
#else
mesa_rc mesa_eflow_alloc(const mesa_inst_t inst, mesa_eflow_id_t *const id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XFLOW)
mesa_rc mesa_eflow_free(const mesa_inst_t inst, const mesa_eflow_id_t id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_eflow_free((const vtss_inst_t)inst /* ag.rb:1847 */, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_eflow_free(const mesa_inst_t inst, const mesa_eflow_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XFLOW) /* ag.rb:1965 */
#else
mesa_rc mesa_eflow_free(const mesa_inst_t inst, const mesa_eflow_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XFLOW)
mesa_rc mesa_eflow_conf_get(const mesa_inst_t inst, const mesa_eflow_id_t id, mesa_eflow_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_eflow_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_eflow_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, id, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_eflow_conf_t_to_mesa_eflow_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_eflow_conf_get(const mesa_inst_t inst, const mesa_eflow_id_t id, mesa_eflow_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XFLOW) /* ag.rb:1965 */
#else
mesa_rc mesa_eflow_conf_get(const mesa_inst_t inst, const mesa_eflow_id_t id, mesa_eflow_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_XFLOW)
mesa_rc mesa_eflow_conf_set(const mesa_inst_t inst, const mesa_eflow_id_t id, const mesa_eflow_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_eflow_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_eflow_conf_t_to_vtss_eflow_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_eflow_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, id, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_eflow_conf_set(const mesa_inst_t inst, const mesa_eflow_id_t id, const mesa_eflow_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_XFLOW) /* ag.rb:1965 */
#else
mesa_rc mesa_eflow_conf_set(const mesa_inst_t inst, const mesa_eflow_id_t id, const mesa_eflow_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
mesa_rc mesa_vcap_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_vcap_port_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_vcap_port_conf_t) == sizeof(mesa_vcap_port_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_vcap_port_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_vcap_port_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vcap_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_vcap_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X) /* ag.rb:1965 */
#else
mesa_rc mesa_vcap_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_vcap_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VCAP) /* ag.rb:1965 */
#else
mesa_rc mesa_vcap_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_vcap_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_VCAP)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X)
mesa_rc mesa_vcap_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_vcap_port_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_vcap_port_conf_t) == sizeof(mesa_vcap_port_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_vcap_port_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (const vtss_vcap_port_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vcap_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_vcap_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_LAN966X) /* ag.rb:1965 */
#else
mesa_rc mesa_vcap_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_vcap_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VCAP) /* ag.rb:1965 */
#else
mesa_rc mesa_vcap_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_vcap_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_isolated_vlan_get(const mesa_inst_t inst, const mesa_vid_t vid, mesa_bool_t *const isolated)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_isolated_vlan_get((const vtss_inst_t)inst /* ag.rb:1847 */, vid, isolated); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_isolated_vlan_get(const mesa_inst_t inst, const mesa_vid_t vid, mesa_bool_t *const isolated)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_isolated_vlan_set(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_bool_t isolated)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_isolated_vlan_set((const vtss_inst_t)inst /* ag.rb:1847 */, vid, isolated); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_isolated_vlan_set(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_bool_t isolated)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_isolated_port_members_get(const mesa_inst_t inst, mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_isolated_port_members_get((const vtss_inst_t)inst /* ag.rb:1847 */, __port_list); /* ag.rb:1946 */
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(__port_list, port_list)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_isolated_port_members_get(const mesa_inst_t inst, mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_isolated_port_members_set(const mesa_inst_t inst, const mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(port_list, __port_list)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_isolated_port_members_set((const vtss_inst_t)inst /* ag.rb:1847 */, __port_list); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_isolated_port_members_set(const mesa_inst_t inst, const mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_pvlan_port_members_get(const mesa_inst_t inst, const mesa_pvlan_no_t pvlan_no, mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_pvlan_port_members_get((const vtss_inst_t)inst /* ag.rb:1847 */, pvlan_no, __port_list); /* ag.rb:1946 */
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(__port_list, port_list)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_pvlan_port_members_get(const mesa_inst_t inst, const mesa_pvlan_no_t pvlan_no, mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_pvlan_port_members_set(const mesa_inst_t inst, const mesa_pvlan_no_t pvlan_no, const mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(port_list, __port_list)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_pvlan_port_members_set((const vtss_inst_t)inst /* ag.rb:1847 */, pvlan_no, __port_list); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_pvlan_port_members_set(const mesa_inst_t inst, const mesa_pvlan_no_t pvlan_no, const mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_apvlan_port_members_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_apvlan_port_members_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, __port_list); /* ag.rb:1946 */
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(__port_list, port_list)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_apvlan_port_members_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_apvlan_port_members_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(port_list, __port_list)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_apvlan_port_members_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, __port_list); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_apvlan_port_members_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_dgroup_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_dgroup_port_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_dgroup_port_conf_t) == sizeof(mesa_dgroup_port_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_dgroup_port_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_dgroup_port_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_dgroup_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_dgroup_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_dgroup_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_dgroup_port_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_dgroup_port_conf_t) == sizeof(mesa_dgroup_port_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_dgroup_port_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (const vtss_dgroup_port_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_dgroup_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_dgroup_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_sflow_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_sflow_port_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_sflow_port_conf_t) == sizeof(mesa_sflow_port_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_sflow_port_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_sflow_port_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_sflow_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_sflow_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_sflow_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_sflow_port_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_sflow_port_conf_t) == sizeof(mesa_sflow_port_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_sflow_port_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (const vtss_sflow_port_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_sflow_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_sflow_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_sflow_sampling_rate_convert(const mesa_inst_t inst, const mesa_bool_t power2, const uint32_t rate_in, uint32_t *const rate_out)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_sflow_sampling_rate_convert((const vtss_inst_t)inst /* ag.rb:1847 */, power2, rate_in, rate_out); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_sflow_sampling_rate_convert(const mesa_inst_t inst, const mesa_bool_t power2, const uint32_t rate_in, uint32_t *const rate_out)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_aggr_port_members_get(const mesa_inst_t inst, const mesa_aggr_no_t aggr_no, mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_aggr_port_members_get((const vtss_inst_t)inst /* ag.rb:1847 */, aggr_no, __port_list); /* ag.rb:1946 */
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(__port_list, port_list)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_aggr_port_members_get(const mesa_inst_t inst, const mesa_aggr_no_t aggr_no, mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_aggr_port_members_set(const mesa_inst_t inst, const mesa_aggr_no_t aggr_no, const mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(port_list, __port_list)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_aggr_port_members_set((const vtss_inst_t)inst /* ag.rb:1847 */, aggr_no, __port_list); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_aggr_port_members_set(const mesa_inst_t inst, const mesa_aggr_no_t aggr_no, const mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_aggr_mode_get(const mesa_inst_t inst, mesa_aggr_mode_t *const mode)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_aggr_mode_t) == sizeof(mesa_aggr_mode_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_aggr_mode_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_aggr_mode_t *)mode /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_aggr_mode_get(const mesa_inst_t inst, mesa_aggr_mode_t *const mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_aggr_mode_set(const mesa_inst_t inst, const mesa_aggr_mode_t *const mode)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_aggr_mode_t) == sizeof(mesa_aggr_mode_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_aggr_mode_set((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_aggr_mode_t *)mode /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_aggr_mode_set(const mesa_inst_t inst, const mesa_aggr_mode_t *const mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mirror_conf_get(const mesa_inst_t inst, mesa_mirror_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_mirror_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_mirror_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_mirror_conf_t_to_mesa_mirror_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_mirror_conf_get(const mesa_inst_t inst, mesa_mirror_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mirror_conf_set(const mesa_inst_t inst, const mesa_mirror_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_mirror_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_mirror_conf_t_to_vtss_mirror_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_mirror_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mirror_conf_set(const mesa_inst_t inst, const mesa_mirror_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mirror_monitor_port_get(const mesa_inst_t inst, mesa_port_no_t *const port_no)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mirror_monitor_port_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mirror_monitor_port_get(const mesa_inst_t inst, mesa_port_no_t *const port_no)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mirror_monitor_port_set(const mesa_inst_t inst, const mesa_port_no_t port_no)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mirror_monitor_port_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mirror_monitor_port_set(const mesa_inst_t inst, const mesa_port_no_t port_no)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mirror_ingress_ports_get(const mesa_inst_t inst, mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_mirror_ingress_ports_get((const vtss_inst_t)inst /* ag.rb:1847 */, __port_list); /* ag.rb:1946 */
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(__port_list, port_list)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_mirror_ingress_ports_get(const mesa_inst_t inst, mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mirror_ingress_ports_set(const mesa_inst_t inst, const mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(port_list, __port_list)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_mirror_ingress_ports_set((const vtss_inst_t)inst /* ag.rb:1847 */, __port_list); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mirror_ingress_ports_set(const mesa_inst_t inst, const mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mirror_egress_ports_get(const mesa_inst_t inst, mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_mirror_egress_ports_get((const vtss_inst_t)inst /* ag.rb:1847 */, __port_list); /* ag.rb:1946 */
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(__port_list, port_list)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_mirror_egress_ports_get(const mesa_inst_t inst, mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mirror_egress_ports_set(const mesa_inst_t inst, const mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(port_list, __port_list)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_mirror_egress_ports_set((const vtss_inst_t)inst /* ag.rb:1847 */, __port_list); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mirror_egress_ports_set(const mesa_inst_t inst, const mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mirror_cpu_ingress_get(const mesa_inst_t inst, mesa_bool_t * member)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mirror_cpu_ingress_get((const vtss_inst_t)inst /* ag.rb:1847 */, member); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mirror_cpu_ingress_get(const mesa_inst_t inst, mesa_bool_t * member)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mirror_cpu_ingress_set(const mesa_inst_t inst, const mesa_bool_t member)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mirror_cpu_ingress_set((const vtss_inst_t)inst /* ag.rb:1847 */, member); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mirror_cpu_ingress_set(const mesa_inst_t inst, const mesa_bool_t member)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mirror_cpu_egress_get(const mesa_inst_t inst, mesa_bool_t * member)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mirror_cpu_egress_get((const vtss_inst_t)inst /* ag.rb:1847 */, member); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mirror_cpu_egress_get(const mesa_inst_t inst, mesa_bool_t * member)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mirror_cpu_egress_set(const mesa_inst_t inst, const mesa_bool_t member)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mirror_cpu_egress_set((const vtss_inst_t)inst /* ag.rb:1847 */, member); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mirror_cpu_egress_set(const mesa_inst_t inst, const mesa_bool_t member)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_uc_flood_members_get(const mesa_inst_t inst, mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_uc_flood_members_get((const vtss_inst_t)inst /* ag.rb:1847 */, __port_list); /* ag.rb:1946 */
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(__port_list, port_list)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_uc_flood_members_get(const mesa_inst_t inst, mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_uc_flood_members_set(const mesa_inst_t inst, const mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(port_list, __port_list)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_uc_flood_members_set((const vtss_inst_t)inst /* ag.rb:1847 */, __port_list); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_uc_flood_members_set(const mesa_inst_t inst, const mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mc_flood_members_get(const mesa_inst_t inst, mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_mc_flood_members_get((const vtss_inst_t)inst /* ag.rb:1847 */, __port_list); /* ag.rb:1946 */
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(__port_list, port_list)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_mc_flood_members_get(const mesa_inst_t inst, mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_mc_flood_members_set(const mesa_inst_t inst, const mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(port_list, __port_list)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_mc_flood_members_set((const vtss_inst_t)inst /* ag.rb:1847 */, __port_list); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mc_flood_members_set(const mesa_inst_t inst, const mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_ipv4_mc_flood_members_get(const mesa_inst_t inst, mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_ipv4_mc_flood_members_get((const vtss_inst_t)inst /* ag.rb:1847 */, __port_list); /* ag.rb:1946 */
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(__port_list, port_list)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_ipv4_mc_flood_members_get(const mesa_inst_t inst, mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_ipv4_mc_flood_members_set(const mesa_inst_t inst, const mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(port_list, __port_list)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_ipv4_mc_flood_members_set((const vtss_inst_t)inst /* ag.rb:1847 */, __port_list); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ipv4_mc_flood_members_set(const mesa_inst_t inst, const mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_IPV4_MC_SIP)
mesa_rc mesa_ipv4_mc_add(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_ip_t sip, const mesa_ip_t dip, const mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(port_list, __port_list)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_ipv4_mc_add((const vtss_inst_t)inst /* ag.rb:1847 */, vid, sip, dip, __port_list); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ipv4_mc_add(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_ip_t sip, const mesa_ip_t dip, const mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_IPV4_MC_SIP) /* ag.rb:1965 */
#else
mesa_rc mesa_ipv4_mc_add(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_ip_t sip, const mesa_ip_t dip, const mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_IPV4_MC_SIP)
mesa_rc mesa_ipv4_mc_del(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_ip_t sip, const mesa_ip_t dip)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ipv4_mc_del((const vtss_inst_t)inst /* ag.rb:1847 */, vid, sip, dip); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ipv4_mc_del(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_ip_t sip, const mesa_ip_t dip)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_IPV4_MC_SIP) /* ag.rb:1965 */
#else
mesa_rc mesa_ipv4_mc_del(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_ip_t sip, const mesa_ip_t dip)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_ipv6_mc_flood_members_get(const mesa_inst_t inst, mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_ipv6_mc_flood_members_get((const vtss_inst_t)inst /* ag.rb:1847 */, __port_list); /* ag.rb:1946 */
    __RC(mesa_conv_uint8_t_to_mesa_port_list_t(__port_list, port_list)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_ipv6_mc_flood_members_get(const mesa_inst_t inst, mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_ipv6_mc_flood_members_set(const mesa_inst_t inst, const mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(port_list, __port_list)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_ipv6_mc_flood_members_set((const vtss_inst_t)inst /* ag.rb:1847 */, __port_list); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ipv6_mc_flood_members_set(const mesa_inst_t inst, const mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_ipv6_mc_ctrl_flood_get(const mesa_inst_t inst, mesa_bool_t *const scope)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ipv6_mc_ctrl_flood_get((const vtss_inst_t)inst /* ag.rb:1847 */, scope); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ipv6_mc_ctrl_flood_get(const mesa_inst_t inst, mesa_bool_t *const scope)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_ipv6_mc_ctrl_flood_set(const mesa_inst_t inst, const mesa_bool_t scope)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ipv6_mc_ctrl_flood_set((const vtss_inst_t)inst /* ag.rb:1847 */, scope); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ipv6_mc_ctrl_flood_set(const mesa_inst_t inst, const mesa_bool_t scope)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_IPV6_MC_SIP)
mesa_rc mesa_ipv6_mc_add(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_ipv6_t sip, const mesa_ipv6_t dip, const mesa_port_list_t * port_list)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORT_ARRAY_SIZE];

    memset(&__port_list, 0, sizeof(__port_list));

#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ipv6_t) == sizeof(mesa_ipv6_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ipv6_t) == sizeof(mesa_ipv6_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(port_list, __port_list)) /* ag.rb:1893 */; /* ag.rb:1941 */

    return vtss_ipv6_mc_add((const vtss_inst_t)inst /* ag.rb:1847 */, vid, *((const vtss_ipv6_t *)&sip) /* ag.rb:1849 */, *((const vtss_ipv6_t *)&dip) /* ag.rb:1849 */, __port_list); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ipv6_mc_add(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_ipv6_t sip, const mesa_ipv6_t dip, const mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_IPV6_MC_SIP) /* ag.rb:1965 */
#else
mesa_rc mesa_ipv6_mc_add(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_ipv6_t sip, const mesa_ipv6_t dip, const mesa_port_list_t * port_list)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_IPV6_MC_SIP)
mesa_rc mesa_ipv6_mc_del(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_ipv6_t sip, const mesa_ipv6_t dip)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ipv6_t) == sizeof(mesa_ipv6_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ipv6_t) == sizeof(mesa_ipv6_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ipv6_mc_del((const vtss_inst_t)inst /* ag.rb:1847 */, vid, *((const vtss_ipv6_t *)&sip) /* ag.rb:1849 */, *((const vtss_ipv6_t *)&dip) /* ag.rb:1849 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ipv6_mc_del(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_ipv6_t sip, const mesa_ipv6_t dip)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_IPV6_MC_SIP) /* ag.rb:1965 */
#else
mesa_rc mesa_ipv6_mc_del(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_ipv6_t sip, const mesa_ipv6_t dip)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_eps_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_eps_port_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_eps_port_conf_t) == sizeof(mesa_eps_port_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_eps_port_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_eps_port_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_eps_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_eps_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_eps_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_eps_port_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_eps_port_conf_t) == sizeof(mesa_eps_port_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_eps_port_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (const vtss_eps_port_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_eps_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_eps_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_eps_port_selector_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_eps_selector_t *const selector)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_eps_selector_t) == sizeof(mesa_eps_selector_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_eps_port_selector_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_eps_selector_t *)selector /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_eps_port_selector_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_eps_selector_t *const selector)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_eps_port_selector_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_eps_selector_t selector)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_eps_selector_t) == sizeof(mesa_eps_selector_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_eps_port_selector_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, *((const vtss_eps_selector_t *)&selector) /* ag.rb:1849 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_eps_port_selector_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_eps_selector_t selector)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_L2_ERPS)
mesa_rc mesa_erps_vlan_member_get(const mesa_inst_t inst, const mesa_erpi_t erpi, const mesa_vid_t vid, mesa_bool_t *const member)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_erps_vlan_member_get((const vtss_inst_t)inst /* ag.rb:1847 */, erpi, vid, member); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_erps_vlan_member_get(const mesa_inst_t inst, const mesa_erpi_t erpi, const mesa_vid_t vid, mesa_bool_t *const member)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_L2_ERPS) /* ag.rb:1965 */
#else
mesa_rc mesa_erps_vlan_member_get(const mesa_inst_t inst, const mesa_erpi_t erpi, const mesa_vid_t vid, mesa_bool_t *const member)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_L2_ERPS)
mesa_rc mesa_erps_vlan_member_set(const mesa_inst_t inst, const mesa_erpi_t erpi, const mesa_vid_t vid, const mesa_bool_t member)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_erps_vlan_member_set((const vtss_inst_t)inst /* ag.rb:1847 */, erpi, vid, member); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_erps_vlan_member_set(const mesa_inst_t inst, const mesa_erpi_t erpi, const mesa_vid_t vid, const mesa_bool_t member)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_L2_ERPS) /* ag.rb:1965 */
#else
mesa_rc mesa_erps_vlan_member_set(const mesa_inst_t inst, const mesa_erpi_t erpi, const mesa_vid_t vid, const mesa_bool_t member)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_L2_ERPS)
mesa_rc mesa_erps_port_state_get(const mesa_inst_t inst, const mesa_erpi_t erpi, const mesa_port_no_t port_no, mesa_erps_state_t *const state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_erps_state_t) == sizeof(mesa_erps_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_erps_port_state_get((const vtss_inst_t)inst /* ag.rb:1847 */, erpi, port_no, (vtss_erps_state_t *)state /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_erps_port_state_get(const mesa_inst_t inst, const mesa_erpi_t erpi, const mesa_port_no_t port_no, mesa_erps_state_t *const state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_L2_ERPS) /* ag.rb:1965 */
#else
mesa_rc mesa_erps_port_state_get(const mesa_inst_t inst, const mesa_erpi_t erpi, const mesa_port_no_t port_no, mesa_erps_state_t *const state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
#if defined(VTSS_FEATURE_L2_ERPS)
mesa_rc mesa_erps_port_state_set(const mesa_inst_t inst, const mesa_erpi_t erpi, const mesa_port_no_t port_no, const mesa_erps_state_t state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_erps_state_t) == sizeof(mesa_erps_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_erps_port_state_set((const vtss_inst_t)inst /* ag.rb:1847 */, erpi, port_no, *((const vtss_erps_state_t *)&state) /* ag.rb:1849 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_erps_port_state_set(const mesa_inst_t inst, const mesa_erpi_t erpi, const mesa_port_no_t port_no, const mesa_erps_state_t state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_L2_ERPS) /* ag.rb:1965 */
#else
mesa_rc mesa_erps_port_state_set(const mesa_inst_t inst, const mesa_erpi_t erpi, const mesa_port_no_t port_no, const mesa_erps_state_t state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_RCL)
mesa_rc mesa_rcl_vid_add(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_rcl_vid_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_rcl_vid_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_rcl_vid_conf_t_to_vtss_rcl_vid_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_rcl_vid_add((const vtss_inst_t)inst /* ag.rb:1847 */, vid, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rcl_vid_add(const mesa_inst_t inst, const mesa_vid_t vid, const mesa_rcl_vid_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_RCL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_RCL)
mesa_rc mesa_rcl_vid_del(const mesa_inst_t inst, const mesa_vid_t vid)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_rcl_vid_del((const vtss_inst_t)inst /* ag.rb:1847 */, vid); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rcl_vid_del(const mesa_inst_t inst, const mesa_vid_t vid)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_RCL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_RCL)
mesa_rc mesa_rce_init(const mesa_inst_t inst, mesa_rce_t *const rce)
{ /* ag.rb:1822 */
    vtss_rce_t __rce;
    memset(&__rce, 0, sizeof(__rce));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_rce_init((const vtss_inst_t)inst /* ag.rb:1847 */, &__rce); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_rce_t_to_mesa_rce_t(&__rce, rce)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_rce_init(const mesa_inst_t inst, mesa_rce_t *const rce)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_RCL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_RCL)
mesa_rc mesa_rce_add(const mesa_inst_t inst, const mesa_rce_id_t rce_id, const mesa_rce_t *const rce)
{ /* ag.rb:1822 */
    vtss_rce_t __rce;
    memset(&__rce, 0, sizeof(__rce));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_rce_t_to_vtss_rce_t(rce, &__rce)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_rce_add((const vtss_inst_t)inst /* ag.rb:1847 */, rce_id, &__rce); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rce_add(const mesa_inst_t inst, const mesa_rce_id_t rce_id, const mesa_rce_t *const rce)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_RCL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_RCL)
mesa_rc mesa_rce_del(const mesa_inst_t inst, const mesa_rce_id_t rce_id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_rce_del((const vtss_inst_t)inst /* ag.rb:1847 */, rce_id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rce_del(const mesa_inst_t inst, const mesa_rce_id_t rce_id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_RCL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_cap_get(const mesa_inst_t inst, const mesa_rb_id_t rb_id, mesa_rb_cap_t *const cap)
{ /* ag.rb:1822 */
    vtss_rb_cap_t __cap;
    memset(&__cap, 0, sizeof(__cap));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_rb_cap_get((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, &__cap); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_rb_cap_t_to_mesa_rb_cap_t(&__cap, cap)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_rb_cap_get(const mesa_inst_t inst, const mesa_rb_id_t rb_id, mesa_rb_cap_t *const cap)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_conf_get(const mesa_inst_t inst, const mesa_rb_id_t rb_id, mesa_rb_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_rb_conf_t) == sizeof(mesa_rb_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rb_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, (vtss_rb_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_conf_get(const mesa_inst_t inst, const mesa_rb_id_t rb_id, mesa_rb_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_conf_set(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_rb_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_rb_conf_t) == sizeof(mesa_rb_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rb_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, (const vtss_rb_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_conf_set(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_rb_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_counters_get(const mesa_inst_t inst, const mesa_rb_id_t rb_id, mesa_rb_counters_t *const counters)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_rb_counters_t) == sizeof(mesa_rb_counters_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rb_counters_get((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, (vtss_rb_counters_t *)counters /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_counters_get(const mesa_inst_t inst, const mesa_rb_id_t rb_id, mesa_rb_counters_t *const counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_counters_clear(const mesa_inst_t inst, const mesa_rb_id_t rb_id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_rb_counters_clear((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_counters_clear(const mesa_inst_t inst, const mesa_rb_id_t rb_id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_node_add(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac, const mesa_rb_node_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mac_t) == sizeof(mesa_mac_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_rb_node_conf_t) == sizeof(mesa_rb_node_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rb_node_add((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, (const vtss_mac_t *)mac /* ag.rb:1844 */, (const vtss_rb_node_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_node_add(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac, const mesa_rb_node_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_node_del(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mac_t) == sizeof(mesa_mac_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rb_node_del((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, (const vtss_mac_t *)mac /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_node_del(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_node_table_clear(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_rb_clear_t clear)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_rb_clear_t) == sizeof(mesa_rb_clear_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rb_node_table_clear((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, *((const vtss_rb_clear_t *)&clear) /* ag.rb:1849 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_node_table_clear(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_rb_clear_t clear)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_node_get(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac, mesa_rb_node_t *const entry)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mac_t) == sizeof(mesa_mac_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_rb_node_t) == sizeof(mesa_rb_node_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rb_node_get((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, (const vtss_mac_t *)mac /* ag.rb:1844 */, (vtss_rb_node_t *)entry /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_node_get(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac, mesa_rb_node_t *const entry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_node_get_next(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac, mesa_rb_node_t *const entry)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mac_t) == sizeof(mesa_mac_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_rb_node_t) == sizeof(mesa_rb_node_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rb_node_get_next((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, (const vtss_mac_t *)mac /* ag.rb:1844 */, (vtss_rb_node_t *)entry /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_node_get_next(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac, mesa_rb_node_t *const entry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_node_id_get_next(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_rb_node_id_t id, mesa_rb_node_t *const entry)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_rb_node_t) == sizeof(mesa_rb_node_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rb_node_id_get_next((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, id, (vtss_rb_node_t *)entry /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_node_id_get_next(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_rb_node_id_t id, mesa_rb_node_t *const entry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_proxy_node_add(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac, const mesa_rb_proxy_node_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mac_t) == sizeof(mesa_mac_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_rb_proxy_node_conf_t) == sizeof(mesa_rb_proxy_node_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rb_proxy_node_add((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, (const vtss_mac_t *)mac /* ag.rb:1844 */, (const vtss_rb_proxy_node_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_proxy_node_add(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac, const mesa_rb_proxy_node_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_proxy_node_del(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mac_t) == sizeof(mesa_mac_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rb_proxy_node_del((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, (const vtss_mac_t *)mac /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_proxy_node_del(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_proxy_node_table_clear(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_rb_clear_t clear)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_rb_clear_t) == sizeof(mesa_rb_clear_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rb_proxy_node_table_clear((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, *((const vtss_rb_clear_t *)&clear) /* ag.rb:1849 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_proxy_node_table_clear(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_rb_clear_t clear)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_proxy_node_get(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac, mesa_rb_proxy_node_t *const entry)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mac_t) == sizeof(mesa_mac_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_rb_proxy_node_t) == sizeof(mesa_rb_proxy_node_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rb_proxy_node_get((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, (const vtss_mac_t *)mac /* ag.rb:1844 */, (vtss_rb_proxy_node_t *)entry /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_proxy_node_get(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac, mesa_rb_proxy_node_t *const entry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_proxy_node_get_next(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac, mesa_rb_proxy_node_t *const entry)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mac_t) == sizeof(mesa_mac_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_rb_proxy_node_t) == sizeof(mesa_rb_proxy_node_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rb_proxy_node_get_next((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, (const vtss_mac_t *)mac /* ag.rb:1844 */, (vtss_rb_proxy_node_t *)entry /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_proxy_node_get_next(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_mac_t *const mac, mesa_rb_proxy_node_t *const entry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_REDBOX)
mesa_rc mesa_rb_proxy_node_id_get_next(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_rb_proxy_node_id_t id, mesa_rb_proxy_node_t *const entry)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_rb_proxy_node_t) == sizeof(mesa_rb_proxy_node_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rb_proxy_node_id_get_next((const vtss_inst_t)inst /* ag.rb:1847 */, rb_id, id, (vtss_rb_proxy_node_t *)entry /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rb_proxy_node_id_get_next(const mesa_inst_t inst, const mesa_rb_id_t rb_id, const mesa_rb_proxy_node_id_t id, mesa_rb_proxy_node_t *const entry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_REDBOX) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_flush(const mesa_inst_t inst)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_l3_flush((const vtss_inst_t)inst /* ag.rb:1847 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_flush(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_common_get(const mesa_inst_t inst, mesa_l3_common_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_l3_common_conf_t) == sizeof(mesa_l3_common_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_common_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_l3_common_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_common_get(const mesa_inst_t inst, mesa_l3_common_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_common_set(const mesa_inst_t inst, const mesa_l3_common_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_l3_common_conf_t) == sizeof(mesa_l3_common_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_common_set((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_l3_common_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_common_set(const mesa_inst_t inst, const mesa_l3_common_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_rleg_get_specific(const mesa_inst_t inst, mesa_vid_t vid, mesa_l3_rleg_conf_t * conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_l3_rleg_conf_t) == sizeof(mesa_l3_rleg_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_rleg_get_specific((const vtss_inst_t)inst /* ag.rb:1847 */, vid, (vtss_l3_rleg_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_rleg_get_specific(const mesa_inst_t inst, mesa_vid_t vid, mesa_l3_rleg_conf_t * conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_rleg_add(const mesa_inst_t inst, const mesa_l3_rleg_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_l3_rleg_conf_t) == sizeof(mesa_l3_rleg_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_rleg_add((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_l3_rleg_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_rleg_add(const mesa_inst_t inst, const mesa_l3_rleg_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_rleg_update(const mesa_inst_t inst, const mesa_l3_rleg_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_l3_rleg_conf_t) == sizeof(mesa_l3_rleg_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_rleg_update((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_l3_rleg_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_rleg_update(const mesa_inst_t inst, const mesa_l3_rleg_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_rleg_del(const mesa_inst_t inst, const mesa_vid_t vlan)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_l3_rleg_del((const vtss_inst_t)inst /* ag.rb:1847 */, vlan); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_rleg_del(const mesa_inst_t inst, const mesa_vid_t vlan)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_route_add(const mesa_inst_t inst, const mesa_routing_entry_t *const entry)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_routing_entry_t) == sizeof(mesa_routing_entry_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_route_add((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_routing_entry_t *)entry /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_route_add(const mesa_inst_t inst, const mesa_routing_entry_t *const entry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_route_bulk_add(const mesa_inst_t inst, const uint32_t cnt, const mesa_routing_entry_t * entry, uint32_t *const rt_added)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_routing_entry_t) == sizeof(mesa_routing_entry_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_route_bulk_add((const vtss_inst_t)inst /* ag.rb:1847 */, cnt, (const vtss_routing_entry_t *)entry /* ag.rb:1844 */, rt_added); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_route_bulk_add(const mesa_inst_t inst, const uint32_t cnt, const mesa_routing_entry_t * entry, uint32_t *const rt_added)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_route_del(const mesa_inst_t inst, const mesa_routing_entry_t *const entry)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_routing_entry_t) == sizeof(mesa_routing_entry_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_route_del((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_routing_entry_t *)entry /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_route_del(const mesa_inst_t inst, const mesa_routing_entry_t *const entry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_route_bulk_del(const mesa_inst_t inst, const uint32_t cnt, const mesa_routing_entry_t * entry, uint32_t *const rt_deleted)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_routing_entry_t) == sizeof(mesa_routing_entry_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_route_bulk_del((const vtss_inst_t)inst /* ag.rb:1847 */, cnt, (const vtss_routing_entry_t *)entry /* ag.rb:1844 */, rt_deleted); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_route_bulk_del(const mesa_inst_t inst, const uint32_t cnt, const mesa_routing_entry_t * entry, uint32_t *const rt_deleted)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_neighbour_add(const mesa_inst_t inst, const mesa_l3_neighbour_t *const entry)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_l3_neighbour_t) == sizeof(mesa_l3_neighbour_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_neighbour_add((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_l3_neighbour_t *)entry /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_neighbour_add(const mesa_inst_t inst, const mesa_l3_neighbour_t *const entry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_neighbour_del(const mesa_inst_t inst, const mesa_l3_neighbour_t *const entry)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_l3_neighbour_t) == sizeof(mesa_l3_neighbour_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_neighbour_del((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_l3_neighbour_t *)entry /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_neighbour_del(const mesa_inst_t inst, const mesa_l3_neighbour_t *const entry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_mc_route_add(const mesa_inst_t inst, const mesa_routing_mc_entry_t *const entry)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_routing_mc_entry_t) == sizeof(mesa_routing_mc_entry_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_mc_route_add((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_routing_mc_entry_t *)entry /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_mc_route_add(const mesa_inst_t inst, const mesa_routing_mc_entry_t *const entry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_mc_route_del(const mesa_inst_t inst, const mesa_routing_mc_entry_t *const entry)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_routing_mc_entry_t) == sizeof(mesa_routing_mc_entry_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_mc_route_del((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_routing_mc_entry_t *)entry /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_mc_route_del(const mesa_inst_t inst, const mesa_routing_mc_entry_t *const entry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_mc_route_rleg_add(const mesa_inst_t inst, const mesa_routing_mc_entry_t *const entry, const mesa_vid_t dest_rleg)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_routing_mc_entry_t) == sizeof(mesa_routing_mc_entry_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_mc_route_rleg_add((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_routing_mc_entry_t *)entry /* ag.rb:1844 */, dest_rleg); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_mc_route_rleg_add(const mesa_inst_t inst, const mesa_routing_mc_entry_t *const entry, const mesa_vid_t dest_rleg)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_mc_route_rleg_del(const mesa_inst_t inst, const mesa_routing_mc_entry_t *const entry, const mesa_vid_t dest_rleg)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_routing_mc_entry_t) == sizeof(mesa_routing_mc_entry_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_mc_route_rleg_del((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_routing_mc_entry_t *)entry /* ag.rb:1844 */, dest_rleg); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_mc_route_rleg_del(const mesa_inst_t inst, const mesa_routing_mc_entry_t *const entry, const mesa_vid_t dest_rleg)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_mc_route_active_get(const mesa_inst_t inst, const mesa_routing_mc_entry_t *const entry, mesa_bool_t *const active)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_routing_mc_entry_t) == sizeof(mesa_routing_mc_entry_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_mc_route_active_get((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_routing_mc_entry_t *)entry /* ag.rb:1844 */, active); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_mc_route_active_get(const mesa_inst_t inst, const mesa_routing_mc_entry_t *const entry, mesa_bool_t *const active)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_counters_reset(const mesa_inst_t inst)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_l3_counters_reset((const vtss_inst_t)inst /* ag.rb:1847 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_counters_reset(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_counters_system_get(const mesa_inst_t inst, mesa_l3_counters_t *const counters)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_l3_counters_t) == sizeof(mesa_l3_counters_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_counters_system_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_l3_counters_t *)counters /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_counters_system_get(const mesa_inst_t inst, mesa_l3_counters_t *const counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_counters_rleg_get(const mesa_inst_t inst, const mesa_vid_t vlan, mesa_l3_counters_t *const counters)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_l3_counters_t) == sizeof(mesa_l3_counters_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_l3_counters_rleg_get((const vtss_inst_t)inst /* ag.rb:1847 */, vlan, (vtss_l3_counters_t *)counters /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_counters_rleg_get(const mesa_inst_t inst, const mesa_vid_t vlan, mesa_l3_counters_t *const counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER3)
mesa_rc mesa_l3_counters_rleg_clear(const mesa_inst_t inst, const mesa_vid_t vlan)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_l3_counters_rleg_clear((const vtss_inst_t)inst /* ag.rb:1847 */, vlan); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_l3_counters_rleg_clear(const mesa_inst_t inst, const mesa_vid_t vlan)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER3) /* ag.rb:1965 */

mesa_rc mesa_trace_conf_get(const mesa_trace_group_t group, mesa_trace_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_trace_group_t) == sizeof(mesa_trace_group_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_trace_conf_t) == sizeof(mesa_trace_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_trace_conf_get(*((const vtss_trace_group_t *)&group) /* ag.rb:1849 */, (vtss_trace_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}

mesa_rc mesa_trace_conf_set(const mesa_trace_group_t group, const mesa_trace_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_trace_group_t) == sizeof(mesa_trace_group_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_trace_conf_t) == sizeof(mesa_trace_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_trace_conf_set(*((const vtss_trace_group_t *)&group) /* ag.rb:1849 */, (const vtss_trace_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}

#if VTSS_OPT_DEBUG_PRINT
mesa_rc mesa_debug_info_get(mesa_debug_info_t *const info)
{ /* ag.rb:1822 */
    vtss_debug_info_t __info;
    memset(&__info, 0, sizeof(__info));
    mesa_rc rc = vtss_debug_info_get(&__info); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_debug_info_t_to_mesa_debug_info_t(&__info, info)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_debug_info_get(mesa_debug_info_t *const info)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // VTSS_OPT_DEBUG_PRINT /* ag.rb:1965 */

#if VTSS_OPT_DEBUG_PRINT
mesa_rc mesa_debug_info_print(const mesa_inst_t inst, const mesa_debug_printf_t prntf, const mesa_debug_info_t *const info)
{ /* ag.rb:1822 */
    vtss_debug_info_t __info;
    memset(&__info, 0, sizeof(__info));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_debug_info_t_to_vtss_debug_info_t(info, &__info)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_debug_info_print((const vtss_inst_t)inst /* ag.rb:1847 */, prntf, &__info); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_debug_info_print(const mesa_inst_t inst, const mesa_debug_printf_t prntf, const mesa_debug_info_t *const info)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // VTSS_OPT_DEBUG_PRINT /* ag.rb:1965 */

mesa_rc mesa_debug_lock(const mesa_inst_t inst, const mesa_debug_lock_t *const lock)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_debug_lock_t) == sizeof(mesa_debug_lock_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_debug_lock((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_debug_lock_t *)lock /* ag.rb:1844 */); /* ag.rb:1952*/ 
}

mesa_rc mesa_debug_unlock(const mesa_inst_t inst, mesa_debug_lock_t *const lock)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_debug_lock_t) == sizeof(mesa_debug_lock_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_debug_unlock((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_debug_lock_t *)lock /* ag.rb:1844 */); /* ag.rb:1952*/ 
}

#if defined(VTSS_FEATURE_MISC)
mesa_rc mesa_reg_read(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const uint32_t addr, uint32_t *const value)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_reg_read((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, addr, value); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_reg_read(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const uint32_t addr, uint32_t *const value)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MISC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MISC)
mesa_rc mesa_reg_write(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const uint32_t addr, const uint32_t value)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_reg_write((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, addr, value); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_reg_write(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const uint32_t addr, const uint32_t value)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MISC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MISC)
mesa_rc mesa_reg_write_masked(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const uint32_t addr, const uint32_t value, const uint32_t mask)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_reg_write_masked((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, addr, value, mask); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_reg_write_masked(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const uint32_t addr, const uint32_t value, const uint32_t mask)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MISC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MISC)
mesa_rc mesa_intr_sticky_clear(const mesa_inst_t inst, uint32_t ext)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_intr_sticky_clear((const vtss_inst_t)inst /* ag.rb:1847 */, ext); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_intr_sticky_clear(const mesa_inst_t inst, uint32_t ext)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MISC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MISC)
mesa_rc mesa_chip_id_get(const mesa_inst_t inst, mesa_chip_id_t *const chip_id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_chip_id_t) == sizeof(mesa_chip_id_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_chip_id_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_chip_id_t *)chip_id /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_chip_id_get(const mesa_inst_t inst, mesa_chip_id_t *const chip_id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MISC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MISC)
mesa_rc mesa_poll_1sec(const mesa_inst_t inst)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_poll_1sec((const vtss_inst_t)inst /* ag.rb:1847 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_poll_1sec(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MISC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MISC)
mesa_rc mesa_ptp_event_poll(const mesa_inst_t inst, mesa_ptp_event_type_t *const ev_mask)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ptp_event_type_t) == sizeof(mesa_ptp_event_type_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ptp_event_poll((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_ptp_event_type_t *)ev_mask /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ptp_event_poll(const mesa_inst_t inst, mesa_ptp_event_type_t *const ev_mask)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MISC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MISC)
mesa_rc mesa_ptp_event_enable(const mesa_inst_t inst, const mesa_ptp_event_type_t ev_mask, const mesa_bool_t enable)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ptp_event_type_t) == sizeof(mesa_ptp_event_type_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ptp_event_enable((const vtss_inst_t)inst /* ag.rb:1847 */, *((const vtss_ptp_event_type_t *)&ev_mask) /* ag.rb:1849 */, enable); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ptp_event_enable(const mesa_inst_t inst, const mesa_ptp_event_type_t ev_mask, const mesa_bool_t enable)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MISC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MISC)
mesa_rc mesa_dev_all_event_poll(const mesa_inst_t inst, const mesa_dev_all_event_poll_t poll_type, mesa_dev_all_event_type_t *const ev_mask)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_dev_all_event_poll_t) == sizeof(mesa_dev_all_event_poll_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_dev_all_event_type_t) == sizeof(mesa_dev_all_event_type_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_dev_all_event_poll((const vtss_inst_t)inst /* ag.rb:1847 */, *((const vtss_dev_all_event_poll_t *)&poll_type) /* ag.rb:1849 */, (vtss_dev_all_event_type_t *)ev_mask /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_dev_all_event_poll(const mesa_inst_t inst, const mesa_dev_all_event_poll_t poll_type, mesa_dev_all_event_type_t *const ev_mask)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MISC) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MISC)
mesa_rc mesa_dev_all_event_enable(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_dev_all_event_type_t ev_mask, const mesa_bool_t enable)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_dev_all_event_type_t) == sizeof(mesa_dev_all_event_type_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_dev_all_event_enable((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, *((const vtss_dev_all_event_type_t *)&ev_mask) /* ag.rb:1849 */, enable); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_dev_all_event_enable(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_dev_all_event_type_t ev_mask, const mesa_bool_t enable)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MISC) /* ag.rb:1965 */

#if defined(VTSS_GPIOS)
mesa_rc mesa_gpio_mode_set(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_gpio_no_t gpio_no, const mesa_gpio_mode_t mode)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_gpio_mode_t) == sizeof(mesa_gpio_mode_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_gpio_mode_set((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, gpio_no, *((const vtss_gpio_mode_t *)&mode) /* ag.rb:1849 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_gpio_mode_set(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_gpio_no_t gpio_no, const mesa_gpio_mode_t mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_GPIOS) /* ag.rb:1965 */

#if defined(VTSS_GPIOS)
mesa_rc mesa_gpio_direction_set(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_gpio_no_t gpio_no, const mesa_bool_t output)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_gpio_direction_set((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, gpio_no, output); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_gpio_direction_set(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_gpio_no_t gpio_no, const mesa_bool_t output)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_GPIOS) /* ag.rb:1965 */

#if defined(VTSS_GPIOS)
mesa_rc mesa_gpio_read(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_gpio_no_t gpio_no, mesa_bool_t *const value)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_gpio_read((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, gpio_no, value); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_gpio_read(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_gpio_no_t gpio_no, mesa_bool_t *const value)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_GPIOS) /* ag.rb:1965 */

#if defined(VTSS_GPIOS)
mesa_rc mesa_gpio_write(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_gpio_no_t gpio_no, const mesa_bool_t value)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_gpio_write((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, gpio_no, value); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_gpio_write(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_gpio_no_t gpio_no, const mesa_bool_t value)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_GPIOS) /* ag.rb:1965 */

#if defined(VTSS_GPIOS)
mesa_rc mesa_gpio_event_poll(const mesa_inst_t inst, const mesa_chip_no_t chip_no, mesa_bool_t *const events)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_gpio_event_poll((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, events); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_gpio_event_poll(const mesa_inst_t inst, const mesa_chip_no_t chip_no, mesa_bool_t *const events)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_GPIOS) /* ag.rb:1965 */

#if defined(VTSS_GPIOS)
mesa_rc mesa_gpio_event_enable(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_gpio_no_t gpio_no, mesa_bool_t enable)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_gpio_event_enable((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, gpio_no, enable); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_gpio_event_enable(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_gpio_no_t gpio_no, mesa_bool_t enable)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_GPIOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_SERIAL_GPIO)
mesa_rc mesa_sgpio_conf_get(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_sgpio_group_t group, mesa_sgpio_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_sgpio_conf_t) == sizeof(mesa_sgpio_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_sgpio_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, group, (vtss_sgpio_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_sgpio_conf_get(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_sgpio_group_t group, mesa_sgpio_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_SERIAL_GPIO)
mesa_rc mesa_sgpio_conf_set(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_sgpio_group_t group, const mesa_sgpio_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_sgpio_conf_t) == sizeof(mesa_sgpio_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_sgpio_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, group, (const vtss_sgpio_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_sgpio_conf_set(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_sgpio_group_t group, const mesa_sgpio_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_SERIAL_GPIO)
mesa_rc mesa_sgpio_event_poll(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_sgpio_group_t group, const uint32_t bit, mesa_bool_t *const events)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_sgpio_event_poll((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, group, bit, events); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_sgpio_event_poll(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_sgpio_group_t group, const uint32_t bit, mesa_bool_t *const events)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_SERIAL_GPIO)
mesa_rc mesa_sgpio_event_enable(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_sgpio_group_t group, const mesa_port_no_t port, const uint32_t bit, mesa_bool_t enable)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_sgpio_event_enable((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, group, port, bit, enable); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_sgpio_event_enable(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_sgpio_group_t group, const mesa_port_no_t port, const uint32_t bit, mesa_bool_t enable)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_SERIAL_GPIO) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_INTERRUPTS)
mesa_rc mesa_intr_cfg(const mesa_inst_t inst, const uint32_t mask, const mesa_bool_t polarity, const mesa_bool_t enable)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_intr_cfg((const vtss_inst_t)inst /* ag.rb:1847 */, mask, polarity, enable); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_intr_cfg(const mesa_inst_t inst, const uint32_t mask, const mesa_bool_t polarity, const mesa_bool_t enable)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_INTERRUPTS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_INTERRUPTS)
mesa_rc mesa_intr_mask_set(const mesa_inst_t inst, mesa_intr_t * mask)
{ /* ag.rb:1822 */
    vtss_intr_t __mask;
    memset(&__mask, 0, sizeof(__mask));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_intr_mask_set((const vtss_inst_t)inst /* ag.rb:1847 */, &__mask); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_intr_t_to_mesa_intr_t(&__mask, mask)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_intr_mask_set(const mesa_inst_t inst, mesa_intr_t * mask)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_INTERRUPTS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_INTERRUPTS)
mesa_rc mesa_intr_status_get(const mesa_inst_t inst, mesa_intr_t * status)
{ /* ag.rb:1822 */
    vtss_intr_t __status;
    memset(&__status, 0, sizeof(__status));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_intr_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, &__status); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_intr_t_to_mesa_intr_t(&__status, status)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_intr_status_get(const mesa_inst_t inst, mesa_intr_t * status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_INTERRUPTS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_INTERRUPTS)
mesa_rc mesa_intr_pol_negation(const mesa_inst_t inst)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_intr_pol_negation((const vtss_inst_t)inst /* ag.rb:1847 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_intr_pol_negation(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_INTERRUPTS) /* ag.rb:1965 */

#ifdef VTSS_FEATURE_IRQ_CONTROL
mesa_rc mesa_irq_conf_get(const mesa_inst_t inst, const mesa_irq_t irq, mesa_irq_conf_t * conf)
{ /* ag.rb:1822 */
    vtss_irq_t __irq;

    memset(&__irq, 0, sizeof(__irq));

#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_irq_conf_t) == sizeof(mesa_irq_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    __RC(mesa_conv_mesa_irq_t_to_vtss_irq_t(&irq, &__irq)) /* ag.rb:1893 */; /* ag.rb:1941 */

    return vtss_irq_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, __irq, (vtss_irq_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_irq_conf_get(const mesa_inst_t inst, const mesa_irq_t irq, mesa_irq_conf_t * conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // VTSS_FEATURE_IRQ_CONTROL /* ag.rb:1965 */

#ifdef VTSS_FEATURE_IRQ_CONTROL
mesa_rc mesa_irq_conf_set(const mesa_inst_t inst, const mesa_irq_t irq, const mesa_irq_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_irq_t __irq;

    memset(&__irq, 0, sizeof(__irq));

#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_irq_conf_t) == sizeof(mesa_irq_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    __RC(mesa_conv_mesa_irq_t_to_vtss_irq_t(&irq, &__irq)) /* ag.rb:1893 */; /* ag.rb:1941 */

    return vtss_irq_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, __irq, (const vtss_irq_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_irq_conf_set(const mesa_inst_t inst, const mesa_irq_t irq, const mesa_irq_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // VTSS_FEATURE_IRQ_CONTROL /* ag.rb:1965 */

#ifdef VTSS_FEATURE_IRQ_CONTROL
mesa_rc mesa_irq_status_get_and_mask(const mesa_inst_t inst, mesa_irq_status_t * status)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_irq_status_t) == sizeof(mesa_irq_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_irq_status_get_and_mask((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_irq_status_t *)status /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_irq_status_get_and_mask(const mesa_inst_t inst, mesa_irq_status_t * status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // VTSS_FEATURE_IRQ_CONTROL /* ag.rb:1965 */

#ifdef VTSS_FEATURE_IRQ_CONTROL
mesa_rc mesa_irq_enable(const mesa_inst_t inst, const mesa_irq_t irq, mesa_bool_t enable)
{ /* ag.rb:1822 */
    vtss_irq_t __irq;
    memset(&__irq, 0, sizeof(__irq));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_irq_t_to_vtss_irq_t(&irq, &__irq)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_irq_enable((const vtss_inst_t)inst /* ag.rb:1847 */, __irq, enable); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_irq_enable(const mesa_inst_t inst, const mesa_irq_t irq, mesa_bool_t enable)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // VTSS_FEATURE_IRQ_CONTROL /* ag.rb:1965 */

uint32_t mesa_tod_get_ns_cnt()
{ /* ag.rb:1822 */
    return vtss_tod_get_ns_cnt(); /* ag.rb:1952*/ 
}

void mesa_tod_set_ns_cnt_cb(tod_get_ns_cnt_cb_t cb)
{ /* ag.rb:1822 */
    return vtss_tod_set_ns_cnt_cb(cb); /* ag.rb:1952*/ 
}

#if defined(VTSS_FEATURE_FAN)
#if defined(VTSS_FEATURE_TEMP_SENSOR)
mesa_rc mesa_temp_sensor_init(const mesa_inst_t inst, const mesa_bool_t enable)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_temp_sensor_init((const vtss_inst_t)inst /* ag.rb:1847 */, enable); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_temp_sensor_init(const mesa_inst_t inst, const mesa_bool_t enable)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TEMP_SENSOR) /* ag.rb:1965 */
#else
mesa_rc mesa_temp_sensor_init(const mesa_inst_t inst, const mesa_bool_t enable)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_FAN) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_FAN)
#if defined(VTSS_FEATURE_TEMP_SENSOR)
mesa_rc mesa_temp_sensor_get(const mesa_inst_t inst, int16_t * temperature)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_temp_sensor_get((const vtss_inst_t)inst /* ag.rb:1847 */, temperature); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_temp_sensor_get(const mesa_inst_t inst, int16_t * temperature)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TEMP_SENSOR) /* ag.rb:1965 */
#else
mesa_rc mesa_temp_sensor_get(const mesa_inst_t inst, int16_t * temperature)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_FAN) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_FAN)
mesa_rc mesa_fan_cool_lvl_set(const mesa_inst_t inst, uint8_t lvl)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_fan_cool_lvl_set((const vtss_inst_t)inst /* ag.rb:1847 */, lvl); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_fan_cool_lvl_set(const mesa_inst_t inst, uint8_t lvl)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_FAN) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_FAN)
mesa_rc mesa_fan_controller_init(const mesa_inst_t inst, const mesa_fan_conf_t *const spec)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_fan_conf_t) == sizeof(mesa_fan_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_fan_controller_init((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_fan_conf_t *)spec /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_fan_controller_init(const mesa_inst_t inst, const mesa_fan_conf_t *const spec)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_FAN) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_FAN)
mesa_rc mesa_fan_cool_lvl_get(const mesa_inst_t inst, uint8_t * lvl)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_fan_cool_lvl_get((const vtss_inst_t)inst /* ag.rb:1847 */, lvl); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_fan_cool_lvl_get(const mesa_inst_t inst, uint8_t * lvl)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_FAN) /* ag.rb:1965 */

#ifdef VTSS_FEATURE_EEE
mesa_rc mesa_eee_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_eee_port_conf_t *const eee_conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_eee_port_conf_t) == sizeof(mesa_eee_port_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_eee_port_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (const vtss_eee_port_conf_t *)eee_conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_eee_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_eee_port_conf_t *const eee_conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // VTSS_FEATURE_EEE /* ag.rb:1965 */

mesa_rc mesa_symreg_data_get(const mesa_inst_t inst, mesa_symreg_data_t *const data)
{ /* ag.rb:1822 */
    vtss_symreg_data_t __data;
    memset(&__data, 0, sizeof(__data));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_symreg_data_get((const vtss_inst_t)inst /* ag.rb:1847 */, &__data); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_symreg_data_t_to_mesa_symreg_data_t(&__data, data)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}

#if defined(VTSS_FEATURE_VSCOPE)
mesa_rc mesa_vscope_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_vscope_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_vscope_conf_t) == sizeof(mesa_vscope_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_vscope_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (const vtss_vscope_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vscope_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_vscope_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VSCOPE) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VSCOPE)
mesa_rc mesa_vscope_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_vscope_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_vscope_conf_t) == sizeof(mesa_vscope_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_vscope_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_vscope_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vscope_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_vscope_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VSCOPE) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VSCOPE)
mesa_rc mesa_vscope_scan_status_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_vscope_scan_status_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_vscope_scan_status_t) == sizeof(mesa_vscope_scan_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_vscope_scan_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_vscope_scan_status_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vscope_scan_status_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_vscope_scan_status_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VSCOPE) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_add(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_mrp_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_conf_t) == sizeof(mesa_mrp_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_add((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, (const vtss_mrp_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_add(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_mrp_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_conf_t) == sizeof(mesa_mrp_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_get((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, (vtss_mrp_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_voe_index_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_voe_idx_t *const voe_idx)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_voe_idx_t) == sizeof(mesa_mrp_voe_idx_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_voe_index_get((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, (vtss_mrp_voe_idx_t *)voe_idx /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_voe_index_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_voe_idx_t *const voe_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_del(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mrp_del((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_del(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_ring_role_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_mrp_ring_role_t role)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_ring_role_t) == sizeof(mesa_mrp_ring_role_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_ring_role_set((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, *((const vtss_mrp_ring_role_t *)&role) /* ag.rb:1849 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_ring_role_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_mrp_ring_role_t role)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_ring_role_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_ring_role_t *const role)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_ring_role_t) == sizeof(mesa_mrp_ring_role_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_ring_role_get((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, (vtss_mrp_ring_role_t *)role /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_ring_role_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_ring_role_t *const role)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_primary_port_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_port_no_t port_no)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mrp_primary_port_set((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, port_no); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_primary_port_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_port_no_t port_no)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_primary_port_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_port_no_t *const port_no)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mrp_primary_port_get((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, port_no); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_primary_port_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_port_no_t *const port_no)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_ring_state_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_mrp_ring_state_t state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_ring_state_t) == sizeof(mesa_mrp_ring_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_ring_state_set((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, *((const vtss_mrp_ring_state_t *)&state) /* ag.rb:1849 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_ring_state_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_mrp_ring_state_t state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_ring_state_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_ring_state_t *const state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_ring_state_t) == sizeof(mesa_mrp_ring_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_ring_state_get((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, (vtss_mrp_ring_state_t *)state /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_ring_state_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_ring_state_t *const state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_in_ring_state_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_mrp_ring_state_t state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_ring_state_t) == sizeof(mesa_mrp_ring_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_in_ring_state_set((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, *((const vtss_mrp_ring_state_t *)&state) /* ag.rb:1849 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_in_ring_state_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_mrp_ring_state_t state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_in_ring_state_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_ring_state_t *const state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_ring_state_t) == sizeof(mesa_mrp_ring_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_in_ring_state_get((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, (vtss_mrp_ring_state_t *)state /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_in_ring_state_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_ring_state_t *const state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_best_mrm_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_mrp_best_mrm_t *const best)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_best_mrm_t) == sizeof(mesa_mrp_best_mrm_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_best_mrm_set((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, (const vtss_mrp_best_mrm_t *)best /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_best_mrm_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_mrp_best_mrm_t *const best)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_best_mrm_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_best_mrm_t *const best)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_best_mrm_t) == sizeof(mesa_mrp_best_mrm_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_best_mrm_get((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, (vtss_mrp_best_mrm_t *)best /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_best_mrm_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_best_mrm_t *const best)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_port_state_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_port_no_t port, const mesa_mrp_port_state_t state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_port_state_t) == sizeof(mesa_mrp_port_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_port_state_set((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, port, *((const vtss_mrp_port_state_t *)&state) /* ag.rb:1849 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_port_state_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_port_no_t port, const mesa_mrp_port_state_t state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_port_state_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_port_no_t port, mesa_mrp_port_state_t *const state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_port_state_t) == sizeof(mesa_mrp_port_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_port_state_get((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, port, (vtss_mrp_port_state_t *)state /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_port_state_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_port_no_t port, mesa_mrp_port_state_t *const state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_tst_loc_conf_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_mrp_tst_loc_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_tst_loc_conf_t) == sizeof(mesa_mrp_tst_loc_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_tst_loc_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, (const vtss_mrp_tst_loc_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_tst_loc_conf_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_mrp_tst_loc_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_tst_loc_conf_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_tst_loc_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_tst_loc_conf_t) == sizeof(mesa_mrp_tst_loc_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_tst_loc_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, (vtss_mrp_tst_loc_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_tst_loc_conf_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_tst_loc_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_tst_hitme_once(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mrp_tst_hitme_once((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_tst_hitme_once(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_itst_hitme_once(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mrp_itst_hitme_once((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_itst_hitme_once(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_tst_copy_conf_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_mrp_tst_copy_conf_t *const copy)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_tst_copy_conf_t) == sizeof(mesa_mrp_tst_copy_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_tst_copy_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, (const vtss_mrp_tst_copy_conf_t *)copy /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_tst_copy_conf_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const mesa_mrp_tst_copy_conf_t *const copy)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_tst_copy_conf_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_tst_copy_conf_t *const copy)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_tst_copy_conf_t) == sizeof(mesa_mrp_tst_copy_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_tst_copy_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, (vtss_mrp_tst_copy_conf_t *)copy /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_tst_copy_conf_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_tst_copy_conf_t *const copy)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_status_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_status_t *const status)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_status_t) == sizeof(mesa_mrp_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, (vtss_mrp_status_t *)status /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_status_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_counters_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_counters_t *const counters)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_counters_t) == sizeof(mesa_mrp_counters_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_counters_get((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, (vtss_mrp_counters_t *)counters /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_counters_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_counters_t *const counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_counters_clear(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mrp_counters_clear((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_counters_clear(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_event_mask_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const uint32_t mask, const mesa_bool_t enable)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mrp_event_mask_set((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, mask, enable); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_event_mask_set(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, const uint32_t mask, const mesa_bool_t enable)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_event_mask_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, uint32_t *const mask)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_mrp_event_mask_get((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, mask); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_event_mask_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, uint32_t *const mask)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_MRP)
mesa_rc mesa_mrp_event_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_event_t *const events)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mrp_event_t) == sizeof(mesa_mrp_event_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_mrp_event_get((const vtss_inst_t)inst /* ag.rb:1847 */, mrp_idx, (vtss_mrp_event_t *)events /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_mrp_event_get(const mesa_inst_t inst, const mesa_mrp_idx_t mrp_idx, mesa_mrp_event_t *const events)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_MRP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_npi_conf_get(const mesa_inst_t inst, mesa_npi_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_npi_conf_t) == sizeof(mesa_npi_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_npi_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_npi_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_npi_conf_get(const mesa_inst_t inst, mesa_npi_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_npi_conf_set(const mesa_inst_t inst, const mesa_npi_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_npi_conf_t) == sizeof(mesa_npi_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_npi_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_npi_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_npi_conf_set(const mesa_inst_t inst, const mesa_npi_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_packet_rx_conf_get(const mesa_inst_t inst, mesa_packet_rx_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_packet_rx_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_packet_rx_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_packet_rx_conf_t_to_mesa_packet_rx_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_packet_rx_conf_get(const mesa_inst_t inst, mesa_packet_rx_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_packet_rx_conf_set(const mesa_inst_t inst, const mesa_packet_rx_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_packet_rx_conf_t __conf;
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    vtss_packet_rx_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, &__conf);  /* ag.rb:1938 */ // get-before-set
    __RC(mesa_conv_mesa_packet_rx_conf_t_to_vtss_packet_rx_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_packet_rx_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_packet_rx_conf_set(const mesa_inst_t inst, const mesa_packet_rx_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_packet_rx_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_packet_rx_port_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_packet_rx_port_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_packet_rx_port_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_packet_rx_port_conf_t_to_mesa_packet_rx_port_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_packet_rx_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_packet_rx_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_packet_rx_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_packet_rx_port_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_packet_rx_port_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_packet_rx_port_conf_t_to_vtss_packet_rx_port_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_packet_rx_port_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_packet_rx_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_packet_rx_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
void mesa_packet_frame_info_init(mesa_packet_frame_info_t *const info)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_packet_frame_info_t) == sizeof(mesa_packet_frame_info_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_packet_frame_info_init((vtss_packet_frame_info_t *)info /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_packet_frame_filter(const mesa_inst_t inst, const mesa_packet_frame_info_t *const info, mesa_packet_filter_t *const filter)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_packet_frame_info_t) == sizeof(mesa_packet_frame_info_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_packet_filter_t) == sizeof(mesa_packet_filter_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_packet_frame_filter((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_packet_frame_info_t *)info /* ag.rb:1844 */, (vtss_packet_filter_t *)filter /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_packet_frame_filter(const mesa_inst_t inst, const mesa_packet_frame_info_t *const info, mesa_packet_filter_t *const filter)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_packet_port_info_init(mesa_packet_port_info_t *const info)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_packet_port_info_t) == sizeof(mesa_packet_port_info_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_packet_port_info_init((vtss_packet_port_info_t *)info /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_packet_port_info_init(mesa_packet_port_info_t *const info)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_packet_vlan_status_get(const mesa_inst_t inst, mesa_packet_vlan_status_t *const status)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_packet_vlan_status_t) == sizeof(mesa_packet_vlan_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_packet_vlan_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_packet_vlan_status_t *)status /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_packet_vlan_status_get(const mesa_inst_t inst, mesa_packet_vlan_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_packet_rx_hdr_decode(const mesa_inst_t inst, const mesa_packet_rx_meta_t *const meta, const uint8_t hdr[MESA_PACKET_HDR_SIZE_BYTES], mesa_packet_rx_info_t *const info)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_packet_rx_meta_t) == sizeof(mesa_packet_rx_meta_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_packet_rx_info_t) == sizeof(mesa_packet_rx_info_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_packet_rx_hdr_decode((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_packet_rx_meta_t *)meta /* ag.rb:1844 */, hdr, (vtss_packet_rx_info_t *)info /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_packet_rx_hdr_decode(const mesa_inst_t inst, const mesa_packet_rx_meta_t *const meta, const uint8_t hdr[MESA_PACKET_HDR_SIZE_BYTES], mesa_packet_rx_info_t *const info)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_packet_tx_frame(const mesa_inst_t inst, const mesa_packet_tx_info_t *const tx_info, const uint8_t *const frame, const uint32_t length)
{ /* ag.rb:1822 */
    vtss_packet_tx_info_t __tx_info;
    memset(&__tx_info, 0, sizeof(__tx_info));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_packet_tx_info_t_to_vtss_packet_tx_info_t(tx_info, &__tx_info)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_packet_tx_frame((const vtss_inst_t)inst /* ag.rb:1847 */, &__tx_info, frame, length); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_packet_tx_frame(const mesa_inst_t inst, const mesa_packet_tx_info_t *const tx_info, const uint8_t *const frame, const uint32_t length)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_packet_tx_info_init(const mesa_inst_t inst, mesa_packet_tx_info_t *const info)
{ /* ag.rb:1822 */
    vtss_packet_tx_info_t __info;
    memset(&__info, 0, sizeof(__info));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_packet_tx_info_init((const vtss_inst_t)inst /* ag.rb:1847 */, &__info); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_packet_tx_info_t_to_mesa_packet_tx_info_t(&__info, info)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_packet_tx_info_init(const mesa_inst_t inst, mesa_packet_tx_info_t *const info)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_packet_rx_frame(const mesa_inst_t inst, uint8_t *const data, const uint32_t buflen, mesa_packet_rx_info_t *const rx_info)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_packet_rx_info_t) == sizeof(mesa_packet_rx_info_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_packet_rx_frame((const vtss_inst_t)inst /* ag.rb:1847 */, data, buflen, (vtss_packet_rx_info_t *)rx_info /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_packet_rx_frame(const mesa_inst_t inst, uint8_t *const data, const uint32_t buflen, mesa_packet_rx_info_t *const rx_info)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_packet_phy_cnt_to_ts_cnt(const mesa_inst_t inst, const uint32_t phy_cnt, uint64_t * ts_cnt)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_packet_phy_cnt_to_ts_cnt((const vtss_inst_t)inst /* ag.rb:1847 */, phy_cnt, ts_cnt); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_packet_phy_cnt_to_ts_cnt(const mesa_inst_t inst, const uint32_t phy_cnt, uint64_t * ts_cnt)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_packet_ns_to_ts_cnt(const mesa_inst_t inst, const uint32_t ns, uint64_t * ts_cnt)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_packet_ns_to_ts_cnt((const vtss_inst_t)inst /* ag.rb:1847 */, ns, ts_cnt); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_packet_ns_to_ts_cnt(const mesa_inst_t inst, const uint32_t ns, uint64_t * ts_cnt)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PACKET)
mesa_rc mesa_ptp_get_timestamp(const mesa_inst_t inst, const uint8_t frm[MESA_PTP_FRAME_TS_LENGTH], const mesa_packet_rx_info_t *const rx_info, mesa_packet_ptp_message_type_t message_type, mesa_packet_timestamp_props_t ts_props, uint64_t * rxTime, mesa_bool_t * timestamp_ok)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_packet_rx_info_t) == sizeof(mesa_packet_rx_info_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_packet_ptp_message_type_t) == sizeof(mesa_packet_ptp_message_type_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_packet_timestamp_props_t) == sizeof(mesa_packet_timestamp_props_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ptp_get_timestamp((const vtss_inst_t)inst /* ag.rb:1847 */, frm, (const vtss_packet_rx_info_t *)rx_info /* ag.rb:1844 */, *((vtss_packet_ptp_message_type_t *)&message_type) /* ag.rb:1849 */, *((vtss_packet_timestamp_props_t *)&ts_props) /* ag.rb:1849 */, rxTime, timestamp_ok); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ptp_get_timestamp(const mesa_inst_t inst, const uint8_t frm[MESA_PTP_FRAME_TS_LENGTH], const mesa_packet_rx_info_t *const rx_info, mesa_packet_ptp_message_type_t message_type, mesa_packet_timestamp_props_t ts_props, uint64_t * rxTime, mesa_bool_t * timestamp_ok)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PACKET) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_port_clause_37_control_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_clause_37_control_t *const control)
{ /* ag.rb:1822 */
    vtss_port_clause_37_control_t __control;
    memset(&__control, 0, sizeof(__control));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_port_clause_37_control_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__control); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_port_clause_37_control_t_to_mesa_port_clause_37_control_t(&__control, control)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_port_clause_37_control_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_clause_37_control_t *const control)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_port_clause_37_control_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_clause_37_control_t *const control)
{ /* ag.rb:1822 */
    vtss_port_clause_37_control_t __control;
    memset(&__control, 0, sizeof(__control));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_clause_37_control_t_to_vtss_port_clause_37_control_t(control, &__control)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_port_clause_37_control_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__control); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_clause_37_control_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_clause_37_control_t *const control)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_port_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_conf_t_to_vtss_port_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_port_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_port_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_port_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_port_conf_t_to_mesa_port_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_port_status_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_status_t *const status)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_port_status_t) == sizeof(mesa_port_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_port_status_t *)status /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_status_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_CONF_BULK)
mesa_rc mesa_port_conf_bulk_set(const mesa_inst_t inst, const mesa_port_bulk_t bulk)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_port_bulk_t) == sizeof(mesa_port_bulk_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_conf_bulk_set((const vtss_inst_t)inst /* ag.rb:1847 */, *((const vtss_port_bulk_t *)&bulk) /* ag.rb:1849 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_conf_bulk_set(const mesa_inst_t inst, const mesa_port_bulk_t bulk)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONF_BULK) /* ag.rb:1965 */
#else
mesa_rc mesa_port_conf_bulk_set(const mesa_inst_t inst, const mesa_port_bulk_t bulk)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_port_counters_update(const mesa_inst_t inst, const mesa_port_no_t port_no)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_port_counters_update((const vtss_inst_t)inst /* ag.rb:1847 */, port_no); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_counters_update(const mesa_inst_t inst, const mesa_port_no_t port_no)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_port_counters_clear(const mesa_inst_t inst, const mesa_port_no_t port_no)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_port_counters_clear((const vtss_inst_t)inst /* ag.rb:1847 */, port_no); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_counters_clear(const mesa_inst_t inst, const mesa_port_no_t port_no)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_port_counters_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_counters_t *const counters)
{ /* ag.rb:1822 */
    vtss_port_counters_t __counters;
    memset(&__counters, 0, sizeof(__counters));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_port_counters_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__counters); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_port_counters_t_to_mesa_port_counters_t(&__counters, counters)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_port_counters_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_counters_t *const counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_port_basic_counters_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_basic_counters_t *const counters)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_basic_counters_t) == sizeof(mesa_basic_counters_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_basic_counters_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_basic_counters_t *)counters /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_basic_counters_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_basic_counters_t *const counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_port_forward_state_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_forward_t *const forward)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_port_forward_t) == sizeof(mesa_port_forward_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_forward_state_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_port_forward_t *)forward /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_forward_state_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_forward_t *const forward)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_port_forward_state_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_forward_t forward)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_port_forward_t) == sizeof(mesa_port_forward_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_forward_state_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, *((const vtss_port_forward_t *)&forward) /* ag.rb:1849 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_forward_state_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_forward_t forward)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_IFH)
mesa_rc mesa_port_ifh_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_ifh_t *const conf)
{ /* ag.rb:1822 */
    vtss_port_ifh_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_ifh_t_to_vtss_port_ifh_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_port_ifh_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_ifh_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_ifh_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_IFH) /* ag.rb:1965 */
#else
mesa_rc mesa_port_ifh_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_ifh_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
#if defined(VTSS_FEATURE_PORT_IFH)
mesa_rc mesa_port_ifh_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_ifh_t *const conf)
{ /* ag.rb:1822 */
    vtss_port_ifh_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_port_ifh_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_port_ifh_t_to_mesa_port_ifh_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_port_ifh_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_ifh_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_IFH) /* ag.rb:1965 */
#else
mesa_rc mesa_port_ifh_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_ifh_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_port_miim_read(const mesa_inst_t inst, const mesa_port_no_t port_no, const uint8_t addr, uint16_t *const value)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_port_miim_read((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, addr, value); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_miim_read(const mesa_inst_t inst, const mesa_port_no_t port_no, const uint8_t addr, uint16_t *const value)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_port_miim_write(const mesa_inst_t inst, const mesa_port_no_t port_no, const uint8_t addr, const uint16_t value)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_port_miim_write((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, addr, value); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_miim_write(const mesa_inst_t inst, const mesa_port_no_t port_no, const uint8_t addr, const uint16_t value)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_miim_read(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_miim_controller_t miim_controller, const uint8_t miim_addr, const uint8_t addr, uint16_t *const value)
{ /* ag.rb:1822 */
    vtss_miim_controller_t __miim_controller;
    memset(&__miim_controller, 0, sizeof(__miim_controller));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_miim_controller_t_to_vtss_miim_controller_t(&miim_controller, &__miim_controller)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_miim_read((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, __miim_controller, miim_addr, addr, value); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_miim_read(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_miim_controller_t miim_controller, const uint8_t miim_addr, const uint8_t addr, uint16_t *const value)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
mesa_rc mesa_miim_write(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_miim_controller_t miim_controller, const uint8_t miim_addr, const uint8_t addr, const uint16_t value)
{ /* ag.rb:1822 */
    vtss_miim_controller_t __miim_controller;
    memset(&__miim_controller, 0, sizeof(__miim_controller));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_miim_controller_t_to_vtss_miim_controller_t(&miim_controller, &__miim_controller)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_miim_write((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, __miim_controller, miim_addr, addr, value); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_miim_write(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_miim_controller_t miim_controller, const uint8_t miim_addr, const uint8_t addr, const uint16_t value)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_CONTROL) /* ag.rb:1965 */

mesa_rc mesa_port_mmd_read(const mesa_inst_t inst, const mesa_port_no_t port_no, const uint8_t mmd, const uint16_t addr, uint16_t *const value)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_port_mmd_read((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, mmd, addr, value); /* ag.rb:1952*/ 
}

mesa_rc mesa_port_mmd_read_inc(const mesa_inst_t inst, const mesa_port_no_t port_no, const uint8_t mmd, const uint16_t addr, uint16_t *const buf, uint8_t count)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_port_mmd_read_inc((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, mmd, addr, buf, count); /* ag.rb:1952*/ 
}

mesa_rc mesa_port_mmd_write(const mesa_inst_t inst, const mesa_port_no_t port_no, const uint8_t mmd, const uint16_t addr, const uint16_t value)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_port_mmd_write((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, mmd, addr, value); /* ag.rb:1952*/ 
}

mesa_rc mesa_port_mmd_masked_write(const mesa_inst_t inst, const mesa_port_no_t port_no, const uint8_t mmd, const uint16_t addr, const uint16_t value, const uint16_t mask)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_port_mmd_masked_write((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, mmd, addr, value, mask); /* ag.rb:1952*/ 
}

mesa_rc mesa_mmd_read(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_miim_controller_t miim_controller, const uint8_t miim_addr, const uint8_t mmd, const uint16_t addr, uint16_t *const value)
{ /* ag.rb:1822 */
    vtss_miim_controller_t __miim_controller;
    memset(&__miim_controller, 0, sizeof(__miim_controller));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_miim_controller_t_to_vtss_miim_controller_t(&miim_controller, &__miim_controller)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_mmd_read((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, __miim_controller, miim_addr, mmd, addr, value); /* ag.rb:1952*/ 
}

mesa_rc mesa_mmd_write(const mesa_inst_t inst, const mesa_chip_no_t chip_no, const mesa_miim_controller_t miim_controller, const uint8_t miim_addr, const uint8_t mmd, const uint16_t addr, const uint16_t value)
{ /* ag.rb:1822 */
    vtss_miim_controller_t __miim_controller;
    memset(&__miim_controller, 0, sizeof(__miim_controller));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_miim_controller_t_to_vtss_miim_controller_t(&miim_controller, &__miim_controller)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_mmd_write((const vtss_inst_t)inst /* ag.rb:1847 */, chip_no, __miim_controller, miim_addr, mmd, addr, value); /* ag.rb:1952*/ 
}

#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
mesa_rc mesa_port_kr_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_kr_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_port_kr_conf_t) == sizeof(mesa_port_kr_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_kr_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (const vtss_port_kr_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_kr_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_kr_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
mesa_rc mesa_port_kr_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_kr_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_port_kr_conf_t) == sizeof(mesa_port_kr_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_kr_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_port_kr_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_kr_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_kr_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
mesa_rc mesa_port_kr_status_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_kr_status_t *const status)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_port_kr_status_t) == sizeof(mesa_port_kr_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_kr_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_port_kr_status_t *)status /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_kr_status_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_kr_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_KR_IRQ)
mesa_rc mesa_port_kr_state_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_kr_state_t *const state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_port_kr_state_t) == sizeof(mesa_port_kr_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_kr_state_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_port_kr_state_t *)state /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_kr_state_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_kr_state_t *const state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
mesa_rc mesa_port_kr_fec_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_kr_fec_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_port_kr_fec_t) == sizeof(mesa_port_kr_fec_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_kr_fec_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_port_kr_fec_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_kr_fec_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_kr_fec_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ)
mesa_rc mesa_port_kr_fec_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_kr_fec_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_port_kr_fec_t) == sizeof(mesa_port_kr_fec_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_kr_fec_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (const vtss_port_kr_fec_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_kr_fec_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_kr_fec_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_KR) || defined(VTSS_FEATURE_PORT_KR_IRQ) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_KR_IRQ)
mesa_rc mesa_port_kr_irq_apply(const mesa_inst_t inst, const mesa_port_no_t port_no, const uint32_t *const irq_vec)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_port_kr_irq_apply((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, irq_vec); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_kr_irq_apply(const mesa_inst_t inst, const mesa_port_no_t port_no, const uint32_t *const irq_vec)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_KR_IRQ)
mesa_rc mesa_port_kr_irq_get(const mesa_inst_t inst, const mesa_port_no_t port_no, uint32_t *const irq_vec)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_port_kr_irq_get((vtss_inst_t)inst /* ag.rb:1847 */, port_no, irq_vec); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_kr_irq_get(const mesa_inst_t inst, const mesa_port_no_t port_no, uint32_t *const irq_vec)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_KR_IRQ)
mesa_rc mesa_port_kr_event_enable(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_bool_t enable)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_port_kr_event_enable((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, enable); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_kr_event_enable(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_bool_t enable)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_KR_IRQ)
mesa_rc mesa_port_kr_irq_activity(mesa_inst_t inst, uint32_t *const mask)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_port_kr_irq_activity((vtss_inst_t)inst /* ag.rb:1847 */, mask); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_kr_irq_activity(mesa_inst_t inst, uint32_t *const mask)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_KR_IRQ)
mesa_rc mesa_port_kr_eye_get(mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_kr_eye_dim_t *const eye)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_port_kr_eye_dim_t) == sizeof(mesa_port_kr_eye_dim_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_kr_eye_get((vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_port_kr_eye_dim_t *)eye /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_kr_eye_get(mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_kr_eye_dim_t *const eye)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_KR_IRQ)
mesa_rc mesa_port_kr_ctle_adjust(mesa_inst_t inst, const mesa_port_no_t port_no)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_port_kr_ctle_adjust((vtss_inst_t)inst /* ag.rb:1847 */, port_no); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_kr_ctle_adjust(mesa_inst_t inst, const mesa_port_no_t port_no)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PORT_KR_IRQ)
mesa_rc mesa_port_kr_ctle_get(mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_ctle_t *const ctle)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_port_ctle_t) == sizeof(mesa_port_ctle_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_kr_ctle_get((vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_port_ctle_t *)ctle /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_port_kr_ctle_get(mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_ctle_t *const ctle)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PORT_KR_IRQ) /* ag.rb:1965 */

mesa_rc mesa_port_test_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_port_test_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_port_test_conf_t) == sizeof(mesa_port_test_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_test_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_port_test_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}

mesa_rc mesa_port_test_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_test_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_port_test_conf_t) == sizeof(mesa_port_test_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_test_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (const vtss_port_test_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}

mesa_rc mesa_port_serdes_debug_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_port_serdes_debug_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_port_serdes_debug_t) == sizeof(mesa_port_serdes_debug_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_port_serdes_debug_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (const vtss_port_serdes_debug_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}

#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_qos_conf_set(const mesa_inst_t inst, const mesa_qos_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_qos_conf_t __conf;
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    vtss_qos_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, &__conf);  /* ag.rb:1938 */ // get-before-set
    __RC(mesa_conv_mesa_qos_conf_t_to_vtss_qos_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_qos_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_conf_set(const mesa_inst_t inst, const mesa_qos_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_qos_conf_get(const mesa_inst_t inst, mesa_qos_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_qos_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_qos_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_qos_conf_t_to_mesa_qos_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_qos_conf_get(const mesa_inst_t inst, mesa_qos_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_qos_status_get(const mesa_inst_t inst, mesa_qos_status_t *const status)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_qos_status_t) == sizeof(mesa_qos_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_qos_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_qos_status_t *)status /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_status_get(const mesa_inst_t inst, mesa_qos_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_qos_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_qos_port_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_qos_port_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_qos_port_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_qos_port_conf_t_to_mesa_qos_port_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_qos_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_qos_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
mesa_rc mesa_qos_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_qos_port_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_qos_port_conf_t __conf;
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    vtss_qos_port_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf);  /* ag.rb:1938 */ // get-before-set
    __RC(mesa_conv_mesa_qos_port_conf_t_to_vtss_qos_port_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_qos_port_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_qos_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QCL)
mesa_rc mesa_qce_init(const mesa_inst_t inst, const mesa_qce_type_t type, mesa_qce_t *const qce)
{ /* ag.rb:1822 */
    vtss_qce_t __qce;

    memset(&__qce, 0, sizeof(__qce));

#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_qce_type_t) == sizeof(mesa_qce_type_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    mesa_rc rc = vtss_qce_init((const vtss_inst_t)inst /* ag.rb:1847 */, *((const vtss_qce_type_t *)&type) /* ag.rb:1849 */, &__qce); /* ag.rb:1946 */

    __RC(mesa_conv_vtss_qce_t_to_mesa_qce_t(&__qce, qce)) /* ag.rb:1893 */; /* ag.rb:1948 */

    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_qce_init(const mesa_inst_t inst, const mesa_qce_type_t type, mesa_qce_t *const qce)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QCL) /* ag.rb:1965 */
#else
mesa_rc mesa_qce_init(const mesa_inst_t inst, const mesa_qce_type_t type, mesa_qce_t *const qce)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP)
mesa_rc mesa_qos_ingress_map_init(const mesa_inst_t inst, const mesa_qos_ingress_map_key_t key, mesa_qos_ingress_map_t *const map)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_qos_ingress_map_key_t) == sizeof(mesa_qos_ingress_map_key_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_qos_ingress_map_t) == sizeof(mesa_qos_ingress_map_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_qos_ingress_map_init((const vtss_inst_t)inst /* ag.rb:1847 */, *((const vtss_qos_ingress_map_key_t *)&key) /* ag.rb:1849 */, (vtss_qos_ingress_map_t *)map /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_ingress_map_init(const mesa_inst_t inst, const mesa_qos_ingress_map_key_t key, mesa_qos_ingress_map_t *const map)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) /* ag.rb:1965 */
#else
mesa_rc mesa_qos_ingress_map_init(const mesa_inst_t inst, const mesa_qos_ingress_map_key_t key, mesa_qos_ingress_map_t *const map)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP)
mesa_rc mesa_qos_ingress_map_add(const mesa_inst_t inst, const mesa_qos_ingress_map_t *const map)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_qos_ingress_map_t) == sizeof(mesa_qos_ingress_map_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_qos_ingress_map_add((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_qos_ingress_map_t *)map /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_ingress_map_add(const mesa_inst_t inst, const mesa_qos_ingress_map_t *const map)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) /* ag.rb:1965 */
#else
mesa_rc mesa_qos_ingress_map_add(const mesa_inst_t inst, const mesa_qos_ingress_map_t *const map)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP)
mesa_rc mesa_qos_ingress_map_del(const mesa_inst_t inst, const mesa_qos_ingress_map_id_t id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_qos_ingress_map_del((const vtss_inst_t)inst /* ag.rb:1847 */, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_ingress_map_del(const mesa_inst_t inst, const mesa_qos_ingress_map_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) /* ag.rb:1965 */
#else
mesa_rc mesa_qos_ingress_map_del(const mesa_inst_t inst, const mesa_qos_ingress_map_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_INGRESS_MAP)
mesa_rc mesa_qos_ingress_map_del_all(const mesa_inst_t inst)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_qos_ingress_map_del_all((const vtss_inst_t)inst /* ag.rb:1847 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_ingress_map_del_all(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_INGRESS_MAP) /* ag.rb:1965 */
#else
mesa_rc mesa_qos_ingress_map_del_all(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_EGRESS_MAP)
mesa_rc mesa_qos_egress_map_init(const mesa_inst_t inst, const mesa_qos_egress_map_key_t key, mesa_qos_egress_map_t *const map)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_qos_egress_map_key_t) == sizeof(mesa_qos_egress_map_key_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_qos_egress_map_t) == sizeof(mesa_qos_egress_map_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_qos_egress_map_init((const vtss_inst_t)inst /* ag.rb:1847 */, *((const vtss_qos_egress_map_key_t *)&key) /* ag.rb:1849 */, (vtss_qos_egress_map_t *)map /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_egress_map_init(const mesa_inst_t inst, const mesa_qos_egress_map_key_t key, mesa_qos_egress_map_t *const map)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_MAP) /* ag.rb:1965 */
#else
mesa_rc mesa_qos_egress_map_init(const mesa_inst_t inst, const mesa_qos_egress_map_key_t key, mesa_qos_egress_map_t *const map)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_EGRESS_MAP)
mesa_rc mesa_qos_egress_map_add(const mesa_inst_t inst, const mesa_qos_egress_map_t *const map)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_qos_egress_map_t) == sizeof(mesa_qos_egress_map_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_qos_egress_map_add((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_qos_egress_map_t *)map /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_egress_map_add(const mesa_inst_t inst, const mesa_qos_egress_map_t *const map)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_MAP) /* ag.rb:1965 */
#else
mesa_rc mesa_qos_egress_map_add(const mesa_inst_t inst, const mesa_qos_egress_map_t *const map)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_EGRESS_MAP)
mesa_rc mesa_qos_egress_map_del(const mesa_inst_t inst, const mesa_qos_egress_map_id_t id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_qos_egress_map_del((const vtss_inst_t)inst /* ag.rb:1847 */, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_egress_map_del(const mesa_inst_t inst, const mesa_qos_egress_map_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_MAP) /* ag.rb:1965 */
#else
mesa_rc mesa_qos_egress_map_del(const mesa_inst_t inst, const mesa_qos_egress_map_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_EGRESS_MAP)
mesa_rc mesa_qos_egress_map_del_all(const mesa_inst_t inst)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_qos_egress_map_del_all((const vtss_inst_t)inst /* ag.rb:1847 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_egress_map_del_all(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_EGRESS_MAP) /* ag.rb:1965 */
#else
mesa_rc mesa_qos_egress_map_del_all(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_ARCH_OCELOT)
mesa_rc mesa_qos_shaper_calibrate(const mesa_inst_t inst)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_qos_shaper_calibrate((const vtss_inst_t)inst /* ag.rb:1847 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_shaper_calibrate(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_OCELOT) /* ag.rb:1965 */
#else
mesa_rc mesa_qos_shaper_calibrate(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_EVC_POLICERS)
mesa_rc mesa_evc_policer_conf_get(const mesa_inst_t inst, const mesa_evc_policer_id_t policer_id, mesa_evc_policer_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_evc_policer_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_evc_policer_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, policer_id, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_dlb_policer_conf_t_to_mesa_dlb_policer_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_evc_policer_conf_get(const mesa_inst_t inst, const mesa_evc_policer_id_t policer_id, mesa_evc_policer_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_EVC_POLICERS) /* ag.rb:1965 */
#else
mesa_rc mesa_evc_policer_conf_get(const mesa_inst_t inst, const mesa_evc_policer_id_t policer_id, mesa_evc_policer_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_EVC_POLICERS)
mesa_rc mesa_evc_policer_conf_set(const mesa_inst_t inst, const mesa_evc_policer_id_t policer_id, const mesa_evc_policer_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_evc_policer_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_dlb_policer_conf_t_to_vtss_dlb_policer_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_evc_policer_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, policer_id, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_evc_policer_conf_set(const mesa_inst_t inst, const mesa_evc_policer_id_t policer_id, const mesa_evc_policer_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_EVC_POLICERS) /* ag.rb:1965 */
#else
mesa_rc mesa_evc_policer_conf_set(const mesa_inst_t inst, const mesa_evc_policer_id_t policer_id, const mesa_evc_policer_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_auth_port_state_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_auth_state_t *const state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_auth_state_t) == sizeof(mesa_auth_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_auth_port_state_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_auth_state_t *)state /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_auth_port_state_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_auth_state_t *const state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_LAYER2)
mesa_rc mesa_auth_port_state_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_auth_state_t state)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_auth_state_t) == sizeof(mesa_auth_state_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_auth_port_state_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, *((const vtss_auth_state_t *)&state) /* ag.rb:1849 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_auth_port_state_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_auth_state_t state)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_LAYER2) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_acl_policer_conf_get(const mesa_inst_t inst, const mesa_acl_policer_no_t policer_no, mesa_acl_policer_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_acl_policer_conf_t) == sizeof(mesa_acl_policer_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_acl_policer_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, policer_no, (vtss_acl_policer_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_acl_policer_conf_get(const mesa_inst_t inst, const mesa_acl_policer_no_t policer_no, mesa_acl_policer_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_ACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_acl_policer_conf_set(const mesa_inst_t inst, const mesa_acl_policer_no_t policer_no, const mesa_acl_policer_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_acl_policer_conf_t) == sizeof(mesa_acl_policer_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_acl_policer_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, policer_no, (const vtss_acl_policer_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_acl_policer_conf_set(const mesa_inst_t inst, const mesa_acl_policer_no_t policer_no, const mesa_acl_policer_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_ACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_ACL)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X)
mesa_rc mesa_acl_sip_conf_set(const mesa_inst_t inst, const mesa_acl_sip_idx_t idx, const mesa_acl_sip_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_acl_sip_conf_t) == sizeof(mesa_acl_sip_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_acl_sip_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, idx, (const vtss_acl_sip_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_acl_sip_conf_set(const mesa_inst_t inst, const mesa_acl_sip_idx_t idx, const mesa_acl_sip_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN969X) /* ag.rb:1965 */
#else
mesa_rc mesa_acl_sip_conf_set(const mesa_inst_t inst, const mesa_acl_sip_idx_t idx, const mesa_acl_sip_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_ACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_acl_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_acl_port_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_acl_port_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_acl_port_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_acl_port_conf_t_to_mesa_acl_port_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_acl_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_acl_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_ACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_acl_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_acl_port_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_acl_port_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_acl_port_conf_t_to_vtss_acl_port_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_acl_port_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_acl_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_acl_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_ACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_acl_port_counter_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_acl_port_counter_t *const counter)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_acl_port_counter_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, counter); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_acl_port_counter_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_acl_port_counter_t *const counter)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_ACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_acl_port_counter_clear(const mesa_inst_t inst, const mesa_port_no_t port_no)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_acl_port_counter_clear((const vtss_inst_t)inst /* ag.rb:1847 */, port_no); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_acl_port_counter_clear(const mesa_inst_t inst, const mesa_port_no_t port_no)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_ACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_ace_init(const mesa_inst_t inst, const mesa_ace_type_t type, mesa_ace_t *const ace)
{ /* ag.rb:1822 */
    vtss_ace_t __ace;

    memset(&__ace, 0, sizeof(__ace));

#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ace_type_t) == sizeof(mesa_ace_type_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    mesa_rc rc = vtss_ace_init((const vtss_inst_t)inst /* ag.rb:1847 */, *((const vtss_ace_type_t *)&type) /* ag.rb:1849 */, &__ace); /* ag.rb:1946 */

    __RC(mesa_conv_vtss_ace_t_to_mesa_ace_t(&__ace, ace)) /* ag.rb:1893 */; /* ag.rb:1948 */

    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_ace_init(const mesa_inst_t inst, const mesa_ace_type_t type, mesa_ace_t *const ace)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_ACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_ace_add(const mesa_inst_t inst, const mesa_ace_id_t ace_id_next, const mesa_ace_t *const ace)
{ /* ag.rb:1822 */
    vtss_ace_t __ace;
    memset(&__ace, 0, sizeof(__ace));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_ace_t_to_vtss_ace_t(ace, &__ace)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_ace_add((const vtss_inst_t)inst /* ag.rb:1847 */, ace_id_next, &__ace); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ace_add(const mesa_inst_t inst, const mesa_ace_id_t ace_id_next, const mesa_ace_t *const ace)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_ACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_ace_del(const mesa_inst_t inst, const mesa_ace_id_t ace_id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ace_del((const vtss_inst_t)inst /* ag.rb:1847 */, ace_id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ace_del(const mesa_inst_t inst, const mesa_ace_id_t ace_id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_ACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_ace_counter_get(const mesa_inst_t inst, const mesa_ace_id_t ace_id, mesa_ace_counter_t *const counter)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ace_counter_get((const vtss_inst_t)inst /* ag.rb:1847 */, ace_id, counter); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ace_counter_get(const mesa_inst_t inst, const mesa_ace_id_t ace_id, mesa_ace_counter_t *const counter)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_ACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_ACL)
mesa_rc mesa_ace_counter_clear(const mesa_inst_t inst, const mesa_ace_id_t ace_id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ace_counter_clear((const vtss_inst_t)inst /* ag.rb:1847 */, ace_id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ace_counter_clear(const mesa_inst_t inst, const mesa_ace_id_t ace_id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_ACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_ACL)
#if defined(VTSS_ARCH_LUTON26)
mesa_rc mesa_ace_status_get(const mesa_inst_t inst, const mesa_ace_id_t ace_id, mesa_ace_status_t *const status)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ace_status_t) == sizeof(mesa_ace_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ace_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, ace_id, (vtss_ace_status_t *)status /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ace_status_get(const mesa_inst_t inst, const mesa_ace_id_t ace_id, mesa_ace_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_LUTON26) /* ag.rb:1965 */
#else
mesa_rc mesa_ace_status_get(const mesa_inst_t inst, const mesa_ace_id_t ace_id, mesa_ace_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_ACL) /* ag.rb:1965 */

void mesa_rleg_list_set(mesa_rleg_list_t * l, mesa_l3_rleg_id_t rleg, mesa_bool_t val)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_rleg_list_t) == sizeof(mesa_rleg_list_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_rleg_list_set((vtss_rleg_list_t *)l /* ag.rb:1844 */, rleg, val); /* ag.rb:1952*/ 
}

mesa_bool_t mesa_rleg_list_get(const mesa_rleg_list_t * l, mesa_l3_rleg_id_t rleg)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_rleg_list_t) == sizeof(mesa_rleg_list_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_rleg_list_get((const vtss_rleg_list_t *)l /* ag.rb:1844 */, rleg); /* ag.rb:1952*/ 
}

#if defined(VTSS_FEATURE_HACL)
mesa_rc mesa_hace_init(const mesa_inst_t inst, const mesa_ace_type_t type, mesa_hace_t *const hace)
{ /* ag.rb:1822 */
    vtss_hace_t __hace;

    memset(&__hace, 0, sizeof(__hace));

#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ace_type_t) == sizeof(mesa_ace_type_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    mesa_rc rc = vtss_hace_init((const vtss_inst_t)inst /* ag.rb:1847 */, *((const vtss_ace_type_t *)&type) /* ag.rb:1849 */, &__hace); /* ag.rb:1946 */

    __RC(mesa_conv_vtss_hace_t_to_mesa_hace_t(&__hace, hace)) /* ag.rb:1893 */; /* ag.rb:1948 */

    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_hace_init(const mesa_inst_t inst, const mesa_ace_type_t type, mesa_hace_t *const hace)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_HACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_HACL)
mesa_rc mesa_hace_add(const mesa_inst_t inst, const mesa_hacl_type_t type, const mesa_ace_id_t ace_id_next, const mesa_hace_t *const hace)
{ /* ag.rb:1822 */
    vtss_hace_t __hace;

    memset(&__hace, 0, sizeof(__hace));

#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_hacl_type_t) == sizeof(mesa_hacl_type_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    __RC(mesa_conv_mesa_hace_t_to_vtss_hace_t(hace, &__hace)) /* ag.rb:1893 */; /* ag.rb:1941 */

    return vtss_hace_add((const vtss_inst_t)inst /* ag.rb:1847 */, *((const vtss_hacl_type_t *)&type) /* ag.rb:1849 */, ace_id_next, &__hace); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_hace_add(const mesa_inst_t inst, const mesa_hacl_type_t type, const mesa_ace_id_t ace_id_next, const mesa_hace_t *const hace)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_HACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_HACL)
mesa_rc mesa_hace_del(const mesa_inst_t inst, const mesa_hacl_type_t type, const mesa_ace_id_t ace_id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_hacl_type_t) == sizeof(mesa_hacl_type_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_hace_del((const vtss_inst_t)inst /* ag.rb:1847 */, *((const vtss_hacl_type_t *)&type) /* ag.rb:1849 */, ace_id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_hace_del(const mesa_inst_t inst, const mesa_hacl_type_t type, const mesa_ace_id_t ace_id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_HACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_HACL)
mesa_rc mesa_hace_counter_get(const mesa_inst_t inst, const mesa_hacl_type_t type, const mesa_ace_id_t ace_id, mesa_ace_counter_t *const counter)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_hacl_type_t) == sizeof(mesa_hacl_type_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_hace_counter_get((const vtss_inst_t)inst /* ag.rb:1847 */, *((const vtss_hacl_type_t *)&type) /* ag.rb:1849 */, ace_id, counter); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_hace_counter_get(const mesa_inst_t inst, const mesa_hacl_type_t type, const mesa_ace_id_t ace_id, mesa_ace_counter_t *const counter)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_HACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_HACL)
mesa_rc mesa_hace_counter_clear(const mesa_inst_t inst, const mesa_hacl_type_t type, const mesa_ace_id_t ace_id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_hacl_type_t) == sizeof(mesa_hacl_type_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_hace_counter_clear((const vtss_inst_t)inst /* ag.rb:1847 */, *((const vtss_hacl_type_t *)&type) /* ag.rb:1849 */, ace_id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_hace_counter_clear(const mesa_inst_t inst, const mesa_hacl_type_t type, const mesa_ace_id_t ace_id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_HACL) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_synce_clock_out_set(const mesa_inst_t inst, const mesa_synce_clk_port_t clk_port, const mesa_synce_clock_out_t *const conf)
{ /* ag.rb:1822 */
    vtss_synce_clock_out_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_synce_clock_out_t_to_vtss_synce_clock_out_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_synce_clock_out_set((const vtss_inst_t)inst /* ag.rb:1847 */, clk_port, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_synce_clock_out_set(const mesa_inst_t inst, const mesa_synce_clk_port_t clk_port, const mesa_synce_clock_out_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_SYNCE) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_synce_clock_out_get(const mesa_inst_t inst, const mesa_synce_clk_port_t clk_port, mesa_synce_clock_out_t *const conf)
{ /* ag.rb:1822 */
    vtss_synce_clock_out_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_synce_clock_out_get((const vtss_inst_t)inst /* ag.rb:1847 */, clk_port, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_synce_clock_out_t_to_mesa_synce_clock_out_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_synce_clock_out_get(const mesa_inst_t inst, const mesa_synce_clk_port_t clk_port, mesa_synce_clock_out_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_SYNCE) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_synce_clock_in_set(const mesa_inst_t inst, const mesa_synce_clk_port_t clk_port, const mesa_synce_clock_in_t *const conf)
{ /* ag.rb:1822 */
    vtss_synce_clock_in_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_synce_clock_in_t_to_vtss_synce_clock_in_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_synce_clock_in_set((const vtss_inst_t)inst /* ag.rb:1847 */, clk_port, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_synce_clock_in_set(const mesa_inst_t inst, const mesa_synce_clk_port_t clk_port, const mesa_synce_clock_in_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_SYNCE) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_synce_clock_in_get(const mesa_inst_t inst, const mesa_synce_clk_port_t clk_port, mesa_synce_clock_in_t *const conf)
{ /* ag.rb:1822 */
    vtss_synce_clock_in_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_synce_clock_in_get((const vtss_inst_t)inst /* ag.rb:1847 */, clk_port, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_synce_clock_in_t_to_mesa_synce_clock_in_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_synce_clock_in_get(const mesa_inst_t inst, const mesa_synce_clk_port_t clk_port, mesa_synce_clock_in_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_SYNCE) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_SYNCE)
mesa_rc mesa_synce_synce_station_clk_out_set(const mesa_inst_t inst, const mesa_synce_clk_port_t clk_port, const mesa_synce_station_clock_out_t *const conf)
{ /* ag.rb:1822 */
    vtss_synce_station_clock_out_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_synce_station_clock_out_t_to_vtss_synce_station_clock_out_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_synce_synce_station_clk_out_set((const vtss_inst_t)inst /* ag.rb:1847 */, clk_port, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_synce_synce_station_clk_out_set(const mesa_inst_t inst, const mesa_synce_clk_port_t clk_port, const mesa_synce_station_clock_out_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_SYNCE) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_SYNCE)
#if defined(VTSS_ARCH_SERVAL_T)     /* TBD_henrikb */
mesa_rc mesa_synce_synce_station_clk_out_get(const mesa_inst_t inst, const mesa_synce_clk_port_t clk_port, mesa_synce_station_clock_out_t *const conf)
{ /* ag.rb:1822 */
    vtss_synce_station_clock_out_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_synce_synce_station_clk_out_get((const vtss_inst_t)inst /* ag.rb:1847 */, clk_port, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_synce_station_clock_out_t_to_mesa_synce_station_clock_out_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_synce_synce_station_clk_out_get(const mesa_inst_t inst, const mesa_synce_clk_port_t clk_port, mesa_synce_station_clock_out_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_SERVAL_T)     /* TBD_henrikb */ /* ag.rb:1965 */
#else
mesa_rc mesa_synce_synce_station_clk_out_get(const mesa_inst_t inst, const mesa_synce_clk_port_t clk_port, mesa_synce_station_clock_out_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_SYNCE) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_conf_set(const mesa_inst_t inst, const mesa_ts_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_conf_t) == sizeof(mesa_ts_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_ts_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_conf_set(const mesa_inst_t inst, const mesa_ts_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_conf_get(const mesa_inst_t inst, mesa_ts_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_conf_t) == sizeof(mesa_ts_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_ts_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_conf_get(const mesa_inst_t inst, mesa_ts_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_timeofday_set(const mesa_inst_t inst, const mesa_timestamp_t *const ts)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_timestamp_t) == sizeof(mesa_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_timeofday_set((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_timestamp_t *)ts /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_timeofday_set(const mesa_inst_t inst, const mesa_timestamp_t *const ts)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_domain_timeofday_set(const mesa_inst_t inst, const uint32_t domain, const mesa_timestamp_t *const ts)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_timestamp_t) == sizeof(mesa_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_domain_timeofday_set((const vtss_inst_t)inst /* ag.rb:1847 */, domain, (const vtss_timestamp_t *)ts /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_domain_timeofday_set(const mesa_inst_t inst, const uint32_t domain, const mesa_timestamp_t *const ts)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_timeofday_set_delta(const mesa_inst_t inst, const mesa_timestamp_t * ts, mesa_bool_t negative)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_timestamp_t) == sizeof(mesa_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_timeofday_set_delta((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_timestamp_t *)ts /* ag.rb:1844 */, negative); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_timeofday_set_delta(const mesa_inst_t inst, const mesa_timestamp_t * ts, mesa_bool_t negative)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_domain_timeofday_set_delta(const mesa_inst_t inst, const uint32_t domain, const mesa_timestamp_t * ts, mesa_bool_t negative)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_timestamp_t) == sizeof(mesa_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_domain_timeofday_set_delta((const vtss_inst_t)inst /* ag.rb:1847 */, domain, (const vtss_timestamp_t *)ts /* ag.rb:1844 */, negative); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_domain_timeofday_set_delta(const mesa_inst_t inst, const uint32_t domain, const mesa_timestamp_t * ts, mesa_bool_t negative)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_timeofday_offset_set(const mesa_inst_t inst, const int32_t offset)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_timeofday_offset_set((const vtss_inst_t)inst /* ag.rb:1847 */, offset); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_timeofday_offset_set(const mesa_inst_t inst, const int32_t offset)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_domain_timeofday_offset_set(const mesa_inst_t inst, const uint32_t domain, const int32_t offset)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_domain_timeofday_offset_set((const vtss_inst_t)inst /* ag.rb:1847 */, domain, offset); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_domain_timeofday_offset_set(const mesa_inst_t inst, const uint32_t domain, const int32_t offset)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_adjtimer_one_sec(const mesa_inst_t inst, mesa_bool_t *const ongoing_adjustment)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_adjtimer_one_sec((const vtss_inst_t)inst /* ag.rb:1847 */, ongoing_adjustment); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_adjtimer_one_sec(const mesa_inst_t inst, mesa_bool_t *const ongoing_adjustment)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_ongoing_adjustment(const mesa_inst_t inst, mesa_bool_t *const ongoing_adjustment)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_ongoing_adjustment((const vtss_inst_t)inst /* ag.rb:1847 */, ongoing_adjustment); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_ongoing_adjustment(const mesa_inst_t inst, mesa_bool_t *const ongoing_adjustment)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_timeofday_get(const mesa_inst_t inst, mesa_timestamp_t *const ts, uint64_t *const tc)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_timestamp_t) == sizeof(mesa_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_timeofday_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_timestamp_t *)ts /* ag.rb:1844 */, tc); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_timeofday_get(const mesa_inst_t inst, mesa_timestamp_t *const ts, uint64_t *const tc)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_timeofday_raw(const mesa_inst_t inst, mesa_timestamp_t *const ts, uint64_t *const tc)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_timestamp_t) == sizeof(mesa_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_timeofday_raw((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_timestamp_t *)ts /* ag.rb:1844 */, tc); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_timeofday_raw(const mesa_inst_t inst, mesa_timestamp_t *const ts, uint64_t *const tc)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_domain_timeofday_get(const mesa_inst_t inst, const uint32_t domain, mesa_timestamp_t *const ts, uint64_t *const tc)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_timestamp_t) == sizeof(mesa_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_domain_timeofday_get((const vtss_inst_t)inst /* ag.rb:1847 */, domain, (vtss_timestamp_t *)ts /* ag.rb:1844 */, tc); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_domain_timeofday_get(const mesa_inst_t inst, const uint32_t domain, mesa_timestamp_t *const ts, uint64_t *const tc)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_multi_domain_timeofday_get(const mesa_inst_t inst, const uint32_t domain1, const uint32_t domain2, mesa_timestamp_t *const ts1, mesa_timestamp_t *const ts2)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_timestamp_t) == sizeof(mesa_timestamp_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_timestamp_t) == sizeof(mesa_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_multi_domain_timeofday_get((const vtss_inst_t)inst /* ag.rb:1847 */, domain1, domain2, (vtss_timestamp_t *)ts1 /* ag.rb:1844 */, (vtss_timestamp_t *)ts2 /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_multi_domain_timeofday_get(const mesa_inst_t inst, const uint32_t domain1, const uint32_t domain2, mesa_timestamp_t *const ts1, mesa_timestamp_t *const ts2)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_timeofday_next_pps_get(const mesa_inst_t inst, mesa_timestamp_t *const ts)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_timestamp_t) == sizeof(mesa_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_timeofday_next_pps_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_timestamp_t *)ts /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_timeofday_next_pps_get(const mesa_inst_t inst, mesa_timestamp_t *const ts)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_timeofday_prev_pps_get(const mesa_inst_t inst, mesa_timestamp_t *const ts)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_timestamp_t) == sizeof(mesa_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_timeofday_prev_pps_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_timestamp_t *)ts /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_timeofday_prev_pps_get(const mesa_inst_t inst, mesa_timestamp_t *const ts)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_domain_timeofday_next_pps_get(const mesa_inst_t inst, const uint32_t domain, mesa_timestamp_t *const ts)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_timestamp_t) == sizeof(mesa_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_domain_timeofday_next_pps_get((const vtss_inst_t)inst /* ag.rb:1847 */, domain, (vtss_timestamp_t *)ts /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_domain_timeofday_next_pps_get(const mesa_inst_t inst, const uint32_t domain, mesa_timestamp_t *const ts)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_adjtimer_set(const mesa_inst_t inst, const int32_t adj)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_adjtimer_set((const vtss_inst_t)inst /* ag.rb:1847 */, adj); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_adjtimer_set(const mesa_inst_t inst, const int32_t adj)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_adjtimer_get(const mesa_inst_t inst, int32_t *const adj)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_adjtimer_get((const vtss_inst_t)inst /* ag.rb:1847 */, adj); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_adjtimer_get(const mesa_inst_t inst, int32_t *const adj)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_domain_adjtimer_set(const mesa_inst_t inst, const uint32_t domain, const int32_t adj)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_domain_adjtimer_set((const vtss_inst_t)inst /* ag.rb:1847 */, domain, adj); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_domain_adjtimer_set(const mesa_inst_t inst, const uint32_t domain, const int32_t adj)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_domain_adjtimer_get(const mesa_inst_t inst, const uint32_t domain, int32_t *const adj)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_domain_adjtimer_get((const vtss_inst_t)inst /* ag.rb:1847 */, domain, adj); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_domain_adjtimer_get(const mesa_inst_t inst, const uint32_t domain, int32_t *const adj)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_freq_offset_get(const mesa_inst_t inst, int32_t *const adj)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_freq_offset_get((const vtss_inst_t)inst /* ag.rb:1847 */, adj); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_freq_offset_get(const mesa_inst_t inst, int32_t *const adj)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X) /* TBD_henrikb */
mesa_rc mesa_ts_alt_clock_saved_get(const mesa_inst_t inst, uint64_t *const saved)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_alt_clock_saved_get((const vtss_inst_t)inst /* ag.rb:1847 */, saved); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_alt_clock_saved_get(const mesa_inst_t inst, uint64_t *const saved)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X) /* TBD_henrikb */ /* ag.rb:1965 */
#else
mesa_rc mesa_ts_alt_clock_saved_get(const mesa_inst_t inst, uint64_t *const saved)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X) /* TBD_henrikb */
#if defined(VTSS_ARCH_OCELOT)
mesa_rc mesa_ts_alt_clock_saved_timeofday_get(const mesa_inst_t inst, mesa_timestamp_t *const ts)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_timestamp_t) == sizeof(mesa_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_alt_clock_saved_timeofday_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_timestamp_t *)ts /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_alt_clock_saved_timeofday_get(const mesa_inst_t inst, mesa_timestamp_t *const ts)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_OCELOT) /* ag.rb:1965 */
#else
mesa_rc mesa_ts_alt_clock_saved_timeofday_get(const mesa_inst_t inst, mesa_timestamp_t *const ts)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X) /* TBD_henrikb */ /* ag.rb:1965 */
#else
mesa_rc mesa_ts_alt_clock_saved_timeofday_get(const mesa_inst_t inst, mesa_timestamp_t *const ts)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X) /* TBD_henrikb */
mesa_rc mesa_ts_alt_clock_mode_set(const mesa_inst_t inst, const mesa_ts_alt_clock_mode_t *const alt_clock_mode)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_alt_clock_mode_t) == sizeof(mesa_ts_alt_clock_mode_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_alt_clock_mode_set((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_ts_alt_clock_mode_t *)alt_clock_mode /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_alt_clock_mode_set(const mesa_inst_t inst, const mesa_ts_alt_clock_mode_t *const alt_clock_mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X) /* TBD_henrikb */ /* ag.rb:1965 */
#else
mesa_rc mesa_ts_alt_clock_mode_set(const mesa_inst_t inst, const mesa_ts_alt_clock_mode_t *const alt_clock_mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X) /* TBD_henrikb */
mesa_rc mesa_ts_alt_clock_mode_get(const mesa_inst_t inst, mesa_ts_alt_clock_mode_t *const alt_clock_mode)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_alt_clock_mode_t) == sizeof(mesa_ts_alt_clock_mode_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_alt_clock_mode_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_ts_alt_clock_mode_t *)alt_clock_mode /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_alt_clock_mode_get(const mesa_inst_t inst, mesa_ts_alt_clock_mode_t *const alt_clock_mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X) /* TBD_henrikb */ /* ag.rb:1965 */
#else
mesa_rc mesa_ts_alt_clock_mode_get(const mesa_inst_t inst, mesa_ts_alt_clock_mode_t *const alt_clock_mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X) /* TBD_henrikb */
mesa_rc mesa_ts_timeofday_next_pps_set(const mesa_inst_t inst, const mesa_timestamp_t *const ts)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_timestamp_t) == sizeof(mesa_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_timeofday_next_pps_set((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_timestamp_t *)ts /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_timeofday_next_pps_set(const mesa_inst_t inst, const mesa_timestamp_t *const ts)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_OCELOT) || defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LAN969X) /* TBD_henrikb */ /* ag.rb:1965 */
#else
mesa_rc mesa_ts_timeofday_next_pps_set(const mesa_inst_t inst, const mesa_timestamp_t *const ts)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_external_clock_mode_get(const mesa_inst_t inst, mesa_ts_ext_clock_mode_t *const ext_clock_mode)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_ext_clock_mode_t) == sizeof(mesa_ts_ext_clock_mode_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_external_clock_mode_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_ts_ext_clock_mode_t *)ext_clock_mode /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_external_clock_mode_get(const mesa_inst_t inst, mesa_ts_ext_clock_mode_t *const ext_clock_mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_external_clock_mode_set(const mesa_inst_t inst, const mesa_ts_ext_clock_mode_t *const ext_clock_mode)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_ext_clock_mode_t) == sizeof(mesa_ts_ext_clock_mode_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_external_clock_mode_set((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_ts_ext_clock_mode_t *)ext_clock_mode /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_external_clock_mode_set(const mesa_inst_t inst, const mesa_ts_ext_clock_mode_t *const ext_clock_mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT)
mesa_rc mesa_ts_external_io_mode_get(const mesa_inst_t inst, const uint32_t io, mesa_ts_ext_io_mode_t *const ext_io_mode)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_ext_io_mode_t) == sizeof(mesa_ts_ext_io_mode_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_external_io_mode_get((const vtss_inst_t)inst /* ag.rb:1847 */, io, (vtss_ts_ext_io_mode_t *)ext_io_mode /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_external_io_mode_get(const mesa_inst_t inst, const uint32_t io, mesa_ts_ext_io_mode_t *const ext_io_mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT) /* ag.rb:1965 */
#else
mesa_rc mesa_ts_external_io_mode_get(const mesa_inst_t inst, const uint32_t io, mesa_ts_ext_io_mode_t *const ext_io_mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT)
mesa_rc mesa_ts_external_io_mode_set(const mesa_inst_t inst, const uint32_t io, const mesa_ts_ext_io_mode_t *const ext_io_mode)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_ext_io_mode_t) == sizeof(mesa_ts_ext_io_mode_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_external_io_mode_set((const vtss_inst_t)inst /* ag.rb:1847 */, io, (const vtss_ts_ext_io_mode_t *)ext_io_mode /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_external_io_mode_set(const mesa_inst_t inst, const uint32_t io, const mesa_ts_ext_io_mode_t *const ext_io_mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT) /* ag.rb:1965 */
#else
mesa_rc mesa_ts_external_io_mode_set(const mesa_inst_t inst, const uint32_t io, const mesa_ts_ext_io_mode_t *const ext_io_mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT)
mesa_rc mesa_ts_saved_timeofday_get(const mesa_inst_t inst, const uint32_t io, mesa_timestamp_t *const ts, uint64_t *const tc)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_timestamp_t) == sizeof(mesa_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_saved_timeofday_get((const vtss_inst_t)inst /* ag.rb:1847 */, io, (vtss_timestamp_t *)ts /* ag.rb:1844 */, tc); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_saved_timeofday_get(const mesa_inst_t inst, const uint32_t io, mesa_timestamp_t *const ts, uint64_t *const tc)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT) /* ag.rb:1965 */
#else
mesa_rc mesa_ts_saved_timeofday_get(const mesa_inst_t inst, const uint32_t io, mesa_timestamp_t *const ts, uint64_t *const tc)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT)
mesa_rc mesa_ts_output_clock_edge_offset_get(const mesa_inst_t inst, const uint32_t io, uint32_t *const offset)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_output_clock_edge_offset_get((const vtss_inst_t)inst /* ag.rb:1847 */, io, offset); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_output_clock_edge_offset_get(const mesa_inst_t inst, const uint32_t io, uint32_t *const offset)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_ARCH_JAGUAR_2) || defined(VTSS_ARCH_SPARX5) || defined(VTSS_ARCH_LAN966X) || defined(VTSS_ARCH_LAN969X) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_OCELOT) /* ag.rb:1965 */
#else
mesa_rc mesa_ts_output_clock_edge_offset_get(const mesa_inst_t inst, const uint32_t io, uint32_t *const offset)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_external_clock_saved_get(const mesa_inst_t inst, uint32_t *const saved)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_external_clock_saved_get((const vtss_inst_t)inst /* ag.rb:1847 */, saved); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_external_clock_saved_get(const mesa_inst_t inst, uint32_t *const saved)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_ingress_latency_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_timeinterval_t *const ingress_latency)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_ingress_latency_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, ingress_latency); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_ingress_latency_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_timeinterval_t *const ingress_latency)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_ingress_latency_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_timeinterval_t *const ingress_latency)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_ingress_latency_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, ingress_latency); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_ingress_latency_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_timeinterval_t *const ingress_latency)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_p2p_delay_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_timeinterval_t *const p2p_delay)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_p2p_delay_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, p2p_delay); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_p2p_delay_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_timeinterval_t *const p2p_delay)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_p2p_delay_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_timeinterval_t *const p2p_delay)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_p2p_delay_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, p2p_delay); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_p2p_delay_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_timeinterval_t *const p2p_delay)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_egress_latency_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_timeinterval_t *const egress_latency)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_egress_latency_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, egress_latency); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_egress_latency_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_timeinterval_t *const egress_latency)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_egress_latency_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_timeinterval_t *const egress_latency)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_egress_latency_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, egress_latency); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_egress_latency_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_timeinterval_t *const egress_latency)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_delay_asymmetry_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_timeinterval_t *const delay_asymmetry)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_delay_asymmetry_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, delay_asymmetry); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_delay_asymmetry_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_timeinterval_t *const delay_asymmetry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_delay_asymmetry_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_timeinterval_t *const delay_asymmetry)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_delay_asymmetry_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, delay_asymmetry); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_delay_asymmetry_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_timeinterval_t *const delay_asymmetry)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_operation_mode_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_ts_operation_mode_t *const mode)
{ /* ag.rb:1822 */
    vtss_ts_operation_mode_t __mode;
    memset(&__mode, 0, sizeof(__mode));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_ts_operation_mode_t_to_vtss_ts_operation_mode_t(mode, &__mode)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_ts_operation_mode_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__mode); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_operation_mode_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_ts_operation_mode_t *const mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_operation_mode_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_ts_operation_mode_t *const mode)
{ /* ag.rb:1822 */
    vtss_ts_operation_mode_t __mode;
    memset(&__mode, 0, sizeof(__mode));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_ts_operation_mode_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__mode); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_ts_operation_mode_t_to_mesa_ts_operation_mode_t(&__mode, mode)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_ts_operation_mode_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_ts_operation_mode_t *const mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_internal_mode_set(const mesa_inst_t inst, const mesa_ts_internal_mode_t *const mode)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_internal_mode_t) == sizeof(mesa_ts_internal_mode_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_internal_mode_set((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_ts_internal_mode_t *)mode /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_internal_mode_set(const mesa_inst_t inst, const mesa_ts_internal_mode_t *const mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_internal_mode_get(const mesa_inst_t inst, mesa_ts_internal_mode_t *const mode)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_internal_mode_t) == sizeof(mesa_ts_internal_mode_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_internal_mode_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_ts_internal_mode_t *)mode /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_internal_mode_get(const mesa_inst_t inst, mesa_ts_internal_mode_t *const mode)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_tx_timestamp_update(const mesa_inst_t inst)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_tx_timestamp_update((const vtss_inst_t)inst /* ag.rb:1847 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_tx_timestamp_update(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_rx_timestamp_get(const mesa_inst_t inst, const mesa_ts_id_t *const ts_id, mesa_ts_timestamp_t *const ts)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_id_t) == sizeof(mesa_ts_id_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_timestamp_t) == sizeof(mesa_ts_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rx_timestamp_get((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_ts_id_t *)ts_id /* ag.rb:1844 */, (vtss_ts_timestamp_t *)ts /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rx_timestamp_get(const mesa_inst_t inst, const mesa_ts_id_t *const ts_id, mesa_ts_timestamp_t *const ts)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_rx_timestamp_id_release(const mesa_inst_t inst, const mesa_ts_id_t *const ts_id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_id_t) == sizeof(mesa_ts_id_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rx_timestamp_id_release((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_ts_id_t *)ts_id /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rx_timestamp_id_release(const mesa_inst_t inst, const mesa_ts_id_t *const ts_id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_rx_master_timestamp_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_ts_timestamp_t *const ts)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_timestamp_t) == sizeof(mesa_ts_timestamp_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_rx_master_timestamp_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_ts_timestamp_t *)ts /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_rx_master_timestamp_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_ts_timestamp_t *const ts)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_tx_timestamp_idx_alloc(const mesa_inst_t inst, const mesa_ts_timestamp_alloc_t *const alloc_parm, mesa_ts_id_t *const ts_id)
{ /* ag.rb:1822 */
    vtss_ts_timestamp_alloc_t __alloc_parm;

    memset(&__alloc_parm, 0, sizeof(__alloc_parm));

#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_id_t) == sizeof(mesa_ts_id_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    __RC(mesa_conv_mesa_ts_timestamp_alloc_t_to_vtss_ts_timestamp_alloc_t(alloc_parm, &__alloc_parm)) /* ag.rb:1893 */; /* ag.rb:1941 */

    return vtss_tx_timestamp_idx_alloc((const vtss_inst_t)inst /* ag.rb:1847 */, &__alloc_parm, (vtss_ts_id_t *)ts_id /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_tx_timestamp_idx_alloc(const mesa_inst_t inst, const mesa_ts_timestamp_alloc_t *const alloc_parm, mesa_ts_id_t *const ts_id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_timestamp_age(const mesa_inst_t inst)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_timestamp_age((const vtss_inst_t)inst /* ag.rb:1847 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_timestamp_age(const mesa_inst_t inst)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_status_change(const mesa_inst_t inst, const mesa_port_no_t port_no)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_status_change((const vtss_inst_t)inst /* ag.rb:1847 */, port_no); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_status_change(const mesa_inst_t inst, const mesa_port_no_t port_no)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined (VTSS_FEATURE_DELAY_REQ_AUTO_RESP)
mesa_rc mesa_ts_autoresp_dom_cfg_set(const mesa_inst_t inst, const uint8_t domain, const mesa_ts_autoresp_dom_cfg_t *const cfg)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_autoresp_dom_cfg_t) == sizeof(mesa_ts_autoresp_dom_cfg_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_autoresp_dom_cfg_set((const vtss_inst_t)inst /* ag.rb:1847 */, domain, (const vtss_ts_autoresp_dom_cfg_t *)cfg /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_autoresp_dom_cfg_set(const mesa_inst_t inst, const uint8_t domain, const mesa_ts_autoresp_dom_cfg_t *const cfg)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined (VTSS_FEATURE_DELAY_REQ_AUTO_RESP) /* ag.rb:1965 */
#else
mesa_rc mesa_ts_autoresp_dom_cfg_set(const mesa_inst_t inst, const uint8_t domain, const mesa_ts_autoresp_dom_cfg_t *const cfg)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined (VTSS_FEATURE_DELAY_REQ_AUTO_RESP)
mesa_rc mesa_ts_autoresp_dom_cfg_get(const mesa_inst_t inst, const uint8_t domain, mesa_ts_autoresp_dom_cfg_t *const cfg)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_ts_autoresp_dom_cfg_t) == sizeof(mesa_ts_autoresp_dom_cfg_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_autoresp_dom_cfg_get((const vtss_inst_t)inst /* ag.rb:1847 */, domain, (vtss_ts_autoresp_dom_cfg_t *)cfg /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_autoresp_dom_cfg_get(const mesa_inst_t inst, const uint8_t domain, mesa_ts_autoresp_dom_cfg_t *const cfg)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined (VTSS_FEATURE_DELAY_REQ_AUTO_RESP) /* ag.rb:1965 */
#else
mesa_rc mesa_ts_autoresp_dom_cfg_get(const mesa_inst_t inst, const uint8_t domain, mesa_ts_autoresp_dom_cfg_t *const cfg)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined (VTSS_FEATURE_DELAY_REQ_AUTO_RESP)
mesa_rc mesa_ts_smac_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_mac_t *const smac)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mac_t) == sizeof(mesa_mac_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_smac_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (const vtss_mac_t *)smac /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_smac_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_mac_t *const smac)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined (VTSS_FEATURE_DELAY_REQ_AUTO_RESP) /* ag.rb:1965 */
#else
mesa_rc mesa_ts_smac_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_mac_t *const smac)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
#if defined (VTSS_FEATURE_DELAY_REQ_AUTO_RESP)
mesa_rc mesa_ts_smac_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_mac_t *const smac)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_mac_t) == sizeof(mesa_mac_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_ts_smac_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_mac_t *)smac /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_smac_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_mac_t *const smac)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined (VTSS_FEATURE_DELAY_REQ_AUTO_RESP) /* ag.rb:1965 */
#else
mesa_rc mesa_ts_smac_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_mac_t *const smac)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_TIMESTAMP)
mesa_rc mesa_ts_seq_cnt_get(const mesa_inst_t inst, const uint32_t sec_cntr, uint16_t *const cnt_val)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_ts_seq_cnt_get((const vtss_inst_t)inst /* ag.rb:1847 */, sec_cntr, cnt_val); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_ts_seq_cnt_get(const mesa_inst_t inst, const uint32_t sec_cntr, uint16_t *const cnt_val)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_TIMESTAMP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_FRER)
mesa_rc mesa_frer_cstream_conf_get(const mesa_inst_t inst, const mesa_frer_cstream_id_t id, mesa_frer_stream_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_frer_stream_conf_t) == sizeof(mesa_frer_stream_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_frer_cstream_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, id, (vtss_frer_stream_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_frer_cstream_conf_get(const mesa_inst_t inst, const mesa_frer_cstream_id_t id, mesa_frer_stream_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_FRER) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_FRER)
mesa_rc mesa_frer_cstream_conf_set(const mesa_inst_t inst, const mesa_frer_cstream_id_t id, const mesa_frer_stream_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_frer_stream_conf_t) == sizeof(mesa_frer_stream_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_frer_cstream_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, id, (const vtss_frer_stream_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_frer_cstream_conf_set(const mesa_inst_t inst, const mesa_frer_cstream_id_t id, const mesa_frer_stream_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_FRER) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_FRER)
mesa_rc mesa_frer_cstream_cnt_get(const mesa_inst_t inst, const mesa_frer_cstream_id_t id, mesa_frer_counters_t *const counters)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_frer_counters_t) == sizeof(mesa_frer_counters_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_frer_cstream_cnt_get((const vtss_inst_t)inst /* ag.rb:1847 */, id, (vtss_frer_counters_t *)counters /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_frer_cstream_cnt_get(const mesa_inst_t inst, const mesa_frer_cstream_id_t id, mesa_frer_counters_t *const counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_FRER) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_FRER)
mesa_rc mesa_frer_cstream_cnt_clear(const mesa_inst_t inst, const mesa_frer_cstream_id_t id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_frer_cstream_cnt_clear((const vtss_inst_t)inst /* ag.rb:1847 */, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_frer_cstream_cnt_clear(const mesa_inst_t inst, const mesa_frer_cstream_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_FRER) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_FRER)
mesa_rc mesa_frer_mstream_alloc(const mesa_inst_t inst, const mesa_port_list_t * port_list, mesa_frer_mstream_id_t *const id)
{ /* ag.rb:1822 */
    BOOL __port_list[VTSS_PORTS];
    memset(&__port_list, 0, sizeof(__port_list));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_port_list_t_to_uint8_t(port_list, __port_list)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_frer_mstream_alloc((const vtss_inst_t)inst /* ag.rb:1847 */, __port_list, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_frer_mstream_alloc(const mesa_inst_t inst, const mesa_port_list_t * port_list, mesa_frer_mstream_id_t *const id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_FRER) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_FRER)
mesa_rc mesa_frer_mstream_free(const mesa_inst_t inst, const mesa_frer_mstream_id_t id)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_frer_mstream_free((const vtss_inst_t)inst /* ag.rb:1847 */, id); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_frer_mstream_free(const mesa_inst_t inst, const mesa_frer_mstream_id_t id)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_FRER) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_FRER)
mesa_rc mesa_frer_mstream_conf_get(const mesa_inst_t inst, const mesa_frer_mstream_id_t id, const mesa_port_no_t port_no, mesa_frer_stream_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_frer_stream_conf_t) == sizeof(mesa_frer_stream_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_frer_mstream_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, id, port_no, (vtss_frer_stream_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_frer_mstream_conf_get(const mesa_inst_t inst, const mesa_frer_mstream_id_t id, const mesa_port_no_t port_no, mesa_frer_stream_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_FRER) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_FRER)
mesa_rc mesa_frer_mstream_conf_set(const mesa_inst_t inst, const mesa_frer_mstream_id_t id, const mesa_port_no_t port_no, const mesa_frer_stream_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_frer_stream_conf_t) == sizeof(mesa_frer_stream_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_frer_mstream_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, id, port_no, (const vtss_frer_stream_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_frer_mstream_conf_set(const mesa_inst_t inst, const mesa_frer_mstream_id_t id, const mesa_port_no_t port_no, const mesa_frer_stream_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_FRER) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_FRER)
mesa_rc mesa_frer_mstream_cnt_get(const mesa_inst_t inst, const mesa_frer_mstream_id_t id, const mesa_port_no_t port_no, mesa_frer_counters_t *const counters)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_frer_counters_t) == sizeof(mesa_frer_counters_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_frer_mstream_cnt_get((const vtss_inst_t)inst /* ag.rb:1847 */, id, port_no, (vtss_frer_counters_t *)counters /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_frer_mstream_cnt_get(const mesa_inst_t inst, const mesa_frer_mstream_id_t id, const mesa_port_no_t port_no, mesa_frer_counters_t *const counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_FRER) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_FRER)
mesa_rc mesa_frer_mstream_cnt_clear(const mesa_inst_t inst, const mesa_frer_mstream_id_t id, const mesa_port_no_t port_no)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_frer_mstream_cnt_clear((const vtss_inst_t)inst /* ag.rb:1847 */, id, port_no); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_frer_mstream_cnt_clear(const mesa_inst_t inst, const mesa_frer_mstream_id_t id, const mesa_port_no_t port_no)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_FRER) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PSFP)
mesa_rc mesa_psfp_gcl_conf_get(const mesa_inst_t inst, const mesa_psfp_gate_id_t id, const uint32_t max_cnt, mesa_psfp_gce_t *const gcl, uint32_t *const gce_cnt)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_psfp_gce_t) == sizeof(mesa_psfp_gce_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_psfp_gcl_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, id, max_cnt, (vtss_psfp_gce_t *)gcl /* ag.rb:1844 */, gce_cnt); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_psfp_gcl_conf_get(const mesa_inst_t inst, const mesa_psfp_gate_id_t id, const uint32_t max_cnt, mesa_psfp_gce_t *const gcl, uint32_t *const gce_cnt)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PSFP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PSFP)
mesa_rc mesa_psfp_gcl_conf_set(const mesa_inst_t inst, const mesa_psfp_gate_id_t id, const uint32_t gce_cnt, const mesa_psfp_gce_t *const gcl)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_psfp_gce_t) == sizeof(mesa_psfp_gce_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_psfp_gcl_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, id, gce_cnt, (const vtss_psfp_gce_t *)gcl /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_psfp_gcl_conf_set(const mesa_inst_t inst, const mesa_psfp_gate_id_t id, const uint32_t gce_cnt, const mesa_psfp_gce_t *const gcl)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PSFP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PSFP)
mesa_rc mesa_psfp_gate_conf_get(const mesa_inst_t inst, const mesa_psfp_gate_id_t id, mesa_psfp_gate_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_psfp_gate_conf_t) == sizeof(mesa_psfp_gate_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_psfp_gate_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, id, (vtss_psfp_gate_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_psfp_gate_conf_get(const mesa_inst_t inst, const mesa_psfp_gate_id_t id, mesa_psfp_gate_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PSFP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PSFP)
mesa_rc mesa_psfp_gate_conf_set(const mesa_inst_t inst, const mesa_psfp_gate_id_t id, const mesa_psfp_gate_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_psfp_gate_conf_t) == sizeof(mesa_psfp_gate_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_psfp_gate_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, id, (const vtss_psfp_gate_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_psfp_gate_conf_set(const mesa_inst_t inst, const mesa_psfp_gate_id_t id, const mesa_psfp_gate_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PSFP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PSFP)
mesa_rc mesa_psfp_gate_status_get(const mesa_inst_t inst, const mesa_psfp_gate_id_t id, mesa_psfp_gate_status_t *const status)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_psfp_gate_status_t) == sizeof(mesa_psfp_gate_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_psfp_gate_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, id, (vtss_psfp_gate_status_t *)status /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_psfp_gate_status_get(const mesa_inst_t inst, const mesa_psfp_gate_id_t id, mesa_psfp_gate_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PSFP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PSFP)
mesa_rc mesa_psfp_filter_conf_get(const mesa_inst_t inst, const mesa_psfp_filter_id_t id, mesa_psfp_filter_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_psfp_filter_conf_t) == sizeof(mesa_psfp_filter_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_psfp_filter_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, id, (vtss_psfp_filter_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_psfp_filter_conf_get(const mesa_inst_t inst, const mesa_psfp_filter_id_t id, mesa_psfp_filter_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PSFP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PSFP)
mesa_rc mesa_psfp_filter_conf_set(const mesa_inst_t inst, const mesa_psfp_filter_id_t id, const mesa_psfp_filter_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_psfp_filter_conf_t) == sizeof(mesa_psfp_filter_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_psfp_filter_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, id, (const vtss_psfp_filter_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_psfp_filter_conf_set(const mesa_inst_t inst, const mesa_psfp_filter_id_t id, const mesa_psfp_filter_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PSFP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_PSFP)
mesa_rc mesa_psfp_filter_status_get(const mesa_inst_t inst, const mesa_psfp_filter_id_t id, mesa_psfp_filter_status_t *const status)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_psfp_filter_status_t) == sizeof(mesa_psfp_filter_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_psfp_filter_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, id, (vtss_psfp_filter_status_t *)status /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_psfp_filter_status_get(const mesa_inst_t inst, const mesa_psfp_filter_id_t id, mesa_psfp_filter_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_PSFP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS_TAS)
mesa_rc mesa_qos_tas_conf_get(const mesa_inst_t inst, mesa_qos_tas_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_qos_tas_conf_t) == sizeof(mesa_qos_tas_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_qos_tas_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, (vtss_qos_tas_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_tas_conf_get(const mesa_inst_t inst, mesa_qos_tas_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_TAS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS_TAS)
mesa_rc mesa_qos_tas_conf_set(const mesa_inst_t inst, const mesa_qos_tas_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_qos_tas_conf_t) == sizeof(mesa_qos_tas_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_qos_tas_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_qos_tas_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_tas_conf_set(const mesa_inst_t inst, const mesa_qos_tas_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_TAS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS_TAS)
mesa_rc mesa_qos_tas_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_qos_tas_port_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_qos_tas_port_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_qos_tas_port_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_qos_tas_port_conf_t_to_mesa_qos_tas_port_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_qos_tas_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_qos_tas_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_TAS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS_TAS)
mesa_rc mesa_qos_tas_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_qos_tas_port_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_qos_tas_port_conf_t __conf;
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    vtss_qos_tas_port_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf);  /* ag.rb:1938 */ // get-before-set
    __RC(mesa_conv_mesa_qos_tas_port_conf_t_to_vtss_qos_tas_port_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_qos_tas_port_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_tas_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_qos_tas_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_TAS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS_TAS)
mesa_rc mesa_qos_tas_port_status_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_qos_tas_port_status_t *const status)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_qos_tas_port_status_t) == sizeof(mesa_qos_tas_port_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_qos_tas_port_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_qos_tas_port_status_t *)status /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_tas_port_status_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_qos_tas_port_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_TAS) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
mesa_rc mesa_qos_fp_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_qos_fp_port_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_qos_fp_port_conf_t) == sizeof(mesa_qos_fp_port_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_qos_fp_port_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_qos_fp_port_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_fp_port_conf_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_qos_fp_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
mesa_rc mesa_qos_fp_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_qos_fp_port_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_qos_fp_port_conf_t) == sizeof(mesa_qos_fp_port_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_qos_fp_port_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (const vtss_qos_fp_port_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_fp_port_conf_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_qos_fp_port_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION)
mesa_rc mesa_qos_fp_port_status_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_qos_fp_port_status_t *const status)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_qos_fp_port_status_t) == sizeof(mesa_qos_fp_port_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_qos_fp_port_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, port_no, (vtss_qos_fp_port_status_t *)status /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_qos_fp_port_status_get(const mesa_inst_t inst, const mesa_port_no_t port_no, mesa_qos_fp_port_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_QOS_FRAME_PREEMPTION) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_vop_conf_set(const mesa_inst_t inst, const mesa_vop_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_vop_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_vop_conf_t_to_vtss_vop_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_vop_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_vop_conf_set(const mesa_inst_t inst, const mesa_vop_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_vop_conf_get(const mesa_inst_t inst, mesa_vop_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_vop_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_vop_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_vop_conf_t_to_mesa_vop_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_vop_conf_get(const mesa_inst_t inst, mesa_vop_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_alloc(const mesa_inst_t inst, const mesa_voe_allocation_t *const param, mesa_voe_idx_t *const voe_idx)
{ /* ag.rb:1822 */
    vtss_voe_allocation_t __param;
    memset(&__param, 0, sizeof(__param));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_voe_allocation_t_to_vtss_voe_allocation_t(param, &__param)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_voe_alloc((const vtss_inst_t)inst /* ag.rb:1847 */, &__param, voe_idx); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_alloc(const mesa_inst_t inst, const mesa_voe_allocation_t *const param, mesa_voe_idx_t *const voe_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_free(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_voe_free((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_free(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_conf_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const mesa_voe_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_voe_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_voe_conf_t_to_vtss_voe_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_voe_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_conf_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const mesa_voe_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_conf_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_voe_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_voe_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_voe_conf_t_to_mesa_voe_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_voe_conf_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_cc_conf_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const mesa_voe_cc_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_voe_cc_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_voe_cc_conf_t_to_vtss_voe_cc_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_voe_cc_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_cc_conf_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const mesa_voe_cc_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_cc_conf_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_cc_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_voe_cc_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_voe_cc_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_voe_cc_conf_t_to_mesa_voe_cc_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_voe_cc_conf_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_cc_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_cc_rdi_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const mesa_bool_t rdi)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_voe_cc_rdi_set((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, rdi); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_cc_rdi_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const mesa_bool_t rdi)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_cc_rdi_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_bool_t *const rdi)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_voe_cc_rdi_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, rdi); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_cc_rdi_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_bool_t *const rdi)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_cc_cpu_copy_next_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_voe_cc_cpu_copy_next_set((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_cc_cpu_copy_next_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_voe_lt_conf_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const mesa_voe_lt_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_voe_lt_conf_t) == sizeof(mesa_voe_lt_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_voe_lt_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, (const vtss_voe_lt_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_lt_conf_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const mesa_voe_lt_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM) /* ag.rb:1965 */
#else
mesa_rc mesa_voe_lt_conf_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const mesa_voe_lt_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_voe_lt_conf_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_lt_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_voe_lt_conf_t) == sizeof(mesa_voe_lt_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_voe_lt_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, (vtss_voe_lt_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_lt_conf_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_lt_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM) /* ag.rb:1965 */
#else
mesa_rc mesa_voe_lt_conf_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_lt_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_voe_lb_conf_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const mesa_voe_lb_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_voe_lb_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    __RC(mesa_conv_mesa_voe_lb_conf_t_to_vtss_voe_lb_conf_t(conf, &__conf)) /* ag.rb:1893 */; /* ag.rb:1941 */
    return vtss_voe_lb_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, &__conf); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_lb_conf_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const mesa_voe_lb_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM) /* ag.rb:1965 */
#else
mesa_rc mesa_voe_lb_conf_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const mesa_voe_lb_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_voe_lb_conf_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_lb_conf_t *const conf)
{ /* ag.rb:1822 */
    vtss_voe_lb_conf_t __conf;
    memset(&__conf, 0, sizeof(__conf));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_voe_lb_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, &__conf); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_voe_lb_conf_t_to_mesa_voe_lb_conf_t(&__conf, conf)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_voe_lb_conf_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_lb_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM) /* ag.rb:1965 */
#else
mesa_rc mesa_voe_lb_conf_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_lb_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_voe_laps_conf_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const mesa_voe_laps_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_voe_laps_conf_t) == sizeof(mesa_voe_laps_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_voe_laps_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, (const vtss_voe_laps_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_laps_conf_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const mesa_voe_laps_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM) /* ag.rb:1965 */
#else
mesa_rc mesa_voe_laps_conf_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const mesa_voe_laps_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_voe_laps_conf_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_laps_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_voe_laps_conf_t) == sizeof(mesa_voe_laps_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_voe_laps_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, (vtss_voe_laps_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_laps_conf_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_laps_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM) /* ag.rb:1965 */
#else
mesa_rc mesa_voe_laps_conf_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_laps_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_status_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_status_t *const status)
{ /* ag.rb:1822 */
    vtss_voe_status_t __status;
    memset(&__status, 0, sizeof(__status));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_voe_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, &__status); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_voe_status_t_to_mesa_voe_status_t(&__status, status)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_voe_status_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_status_t *const status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_counters_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_counters_t * counters)
{ /* ag.rb:1822 */
    vtss_voe_counters_t __counters;
    memset(&__counters, 0, sizeof(__counters));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_voe_counters_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, &__counters); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_voe_counters_t_to_mesa_voe_counters_t(&__counters, counters)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_voe_counters_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_counters_t * counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_counters_clear(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_voe_counters_clear((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_counters_clear(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_cc_status_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_cc_status_t * status)
{ /* ag.rb:1822 */
    vtss_voe_cc_status_t __status;
    memset(&__status, 0, sizeof(__status));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_voe_cc_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, &__status); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_voe_cc_status_t_to_mesa_voe_cc_status_t(&__status, status)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_voe_cc_status_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_cc_status_t * status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_cc_counters_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_cc_counters_t * counters)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_voe_cc_counters_t) == sizeof(mesa_voe_cc_counters_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_voe_cc_counters_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, (vtss_voe_cc_counters_t *)counters /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_cc_counters_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_cc_counters_t * counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_cc_counters_clear(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_voe_cc_counters_clear((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_cc_counters_clear(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_voe_lt_status_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_lt_status_t * status)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_voe_lt_status_t) == sizeof(mesa_voe_lt_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_voe_lt_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, (vtss_voe_lt_status_t *)status /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_lt_status_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_lt_status_t * status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM) /* ag.rb:1965 */
#else
mesa_rc mesa_voe_lt_status_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_lt_status_t * status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_voe_lb_status_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_lb_status_t * status)
{ /* ag.rb:1822 */
    vtss_voe_lb_status_t __status;
    memset(&__status, 0, sizeof(__status));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_voe_lb_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, &__status); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_voe_lb_status_t_to_mesa_voe_lb_status_t(&__status, status)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_voe_lb_status_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_lb_status_t * status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM) /* ag.rb:1965 */
#else
mesa_rc mesa_voe_lb_status_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_lb_status_t * status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_voe_lb_counters_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_lb_counters_t * counters)
{ /* ag.rb:1822 */
    vtss_voe_lb_counters_t __counters;
    memset(&__counters, 0, sizeof(__counters));
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    mesa_rc rc = vtss_voe_lb_counters_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, &__counters); /* ag.rb:1946 */
    __RC(mesa_conv_vtss_voe_lb_counters_t_to_mesa_voe_lb_counters_t(&__counters, counters)) /* ag.rb:1893 */; /* ag.rb:1948 */
    return rc; /* ag.rb:1950 */
}
#else
mesa_rc mesa_voe_lb_counters_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_lb_counters_t * counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM) /* ag.rb:1965 */
#else
mesa_rc mesa_voe_lb_counters_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_lb_counters_t * counters)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_voe_lb_counters_clear(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_voe_lb_counters_clear((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_lb_counters_clear(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM) /* ag.rb:1965 */
#else
mesa_rc mesa_voe_lb_counters_clear(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_CFM)
mesa_rc mesa_voe_laps_status_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_laps_status_t * status)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_voe_laps_status_t) == sizeof(mesa_voe_laps_status_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_voe_laps_status_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, (vtss_voe_laps_status_t *)status /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_laps_status_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_laps_status_t * status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP_CFM) /* ag.rb:1965 */
#else
mesa_rc mesa_voe_laps_status_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, mesa_voe_laps_status_t * status)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_event_active_get(const mesa_inst_t inst, const uint32_t active_size, uint32_t *const active)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_voe_event_active_get((const vtss_inst_t)inst /* ag.rb:1847 */, active_size, active); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_event_active_get(const mesa_inst_t inst, const uint32_t active_size, uint32_t *const active)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_event_mask_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const uint32_t mask, const mesa_bool_t enable)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_voe_event_mask_set((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, mask, enable); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_event_mask_set(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, const uint32_t mask, const mesa_bool_t enable)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_event_mask_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, uint32_t *const mask)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_voe_event_mask_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, mask); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_event_mask_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, uint32_t *const mask)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
mesa_rc mesa_voe_event_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, uint32_t *const mask)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_voe_event_get((const vtss_inst_t)inst /* ag.rb:1847 */, voe_idx, mask); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voe_event_get(const mesa_inst_t inst, const mesa_voe_idx_t voe_idx, uint32_t *const mask)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_V2)
mesa_rc mesa_voi_alloc(const mesa_inst_t inst, const mesa_voi_allocation_t *const param, mesa_voi_idx_t *const voi_idx)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_voi_allocation_t) == sizeof(mesa_voi_allocation_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_voi_alloc((const vtss_inst_t)inst /* ag.rb:1847 */, (const vtss_voi_allocation_t *)param /* ag.rb:1844 */, voi_idx); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voi_alloc(const mesa_inst_t inst, const mesa_voi_allocation_t *const param, mesa_voi_idx_t *const voi_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_voi_alloc(const mesa_inst_t inst, const mesa_voi_allocation_t *const param, mesa_voi_idx_t *const voi_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_V2)
mesa_rc mesa_voi_free(const mesa_inst_t inst, const mesa_voi_idx_t voi_idx)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus
    return vtss_voi_free((const vtss_inst_t)inst /* ag.rb:1847 */, voi_idx); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voi_free(const mesa_inst_t inst, const mesa_voi_idx_t voi_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_voi_free(const mesa_inst_t inst, const mesa_voi_idx_t voi_idx)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_V2)
mesa_rc mesa_voi_conf_set(const mesa_inst_t inst, const mesa_voi_idx_t voi_idx, const mesa_voi_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_voi_conf_t) == sizeof(mesa_voi_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_voi_conf_set((const vtss_inst_t)inst /* ag.rb:1847 */, voi_idx, (const vtss_voi_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voi_conf_set(const mesa_inst_t inst, const mesa_voi_idx_t voi_idx, const mesa_voi_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_voi_conf_set(const mesa_inst_t inst, const mesa_voi_idx_t voi_idx, const mesa_voi_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#if defined(VTSS_FEATURE_VOP)
#if defined(VTSS_FEATURE_VOP_V2)
mesa_rc mesa_voi_conf_get(const mesa_inst_t inst, const mesa_voi_idx_t voi_idx, mesa_voi_conf_t *const conf)
{ /* ag.rb:1822 */
#ifdef __cplusplus
    static_assert(sizeof(vtss_inst_t) == sizeof(mesa_inst_t), "Check size"); /* ag.rb:1932 */
    static_assert(sizeof(vtss_voi_conf_t) == sizeof(mesa_voi_conf_t), "Check size"); /* ag.rb:1932 */
#endif // __cplusplus

    return vtss_voi_conf_get((const vtss_inst_t)inst /* ag.rb:1847 */, voi_idx, (vtss_voi_conf_t *)conf /* ag.rb:1844 */); /* ag.rb:1952*/ 
}
#else
mesa_rc mesa_voi_conf_get(const mesa_inst_t inst, const mesa_voi_idx_t voi_idx, mesa_voi_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP_V2) /* ag.rb:1965 */
#else
mesa_rc mesa_voi_conf_get(const mesa_inst_t inst, const mesa_voi_idx_t voi_idx, mesa_voi_conf_t *const conf)
{
    return MESA_RC_NOT_IMPLEMENTED;
}
#endif  // defined(VTSS_FEATURE_VOP) /* ag.rb:1965 */

#pragma GCC diagnostic pop
