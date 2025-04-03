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

/*
******************************************************************************

    Include files

******************************************************************************
*/
#include "icfg_api.h"
#include "icli_porting_util.h"
#include "qos_api.h"
#include "qos_icfg.h"
#include "topo_api.h"
#include "misc_api.h"
#include "mgmt_api.h"

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/

/* Define module name of memory allocation */
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_QOS

#undef IC_RC
#define IC_RC ICLI_VTSS_RC

// Helper macros:
#define SHOW_(p)  ((req->all_defaults) || (c.p != dc.p))
#define PRT_(...) do { IC_RC(vtss_icfg_printf(result, __VA_ARGS__)); } while (0)

#define CAPA vtss_appl_qos_capabilities

/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/

static mesa_rc QOS_ICFG_global_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    vtss_appl_qos_conf_t dc;
    vtss_appl_qos_conf_t c;

    IC_RC(vtss_appl_qos_conf_get_default(&dc));
    IC_RC(vtss_appl_qos_conf_get(&c));

    {
        char buf[32];
        if (SHOW_(uc_policer.enable)) {
            if (c.uc_policer.enable) {
                PRT_("qos storm unicast %s\n", vtss_appl_qos_rate2txt(c.uc_policer.rate, c.uc_policer.frame_rate, buf));
            } else {
                PRT_("no qos storm unicast\n");
            }
        }
        if (SHOW_(mc_policer.enable)) {
            if (c.mc_policer.enable) {
                PRT_("qos storm multicast %s\n", vtss_appl_qos_rate2txt(c.mc_policer.rate, c.mc_policer.frame_rate, buf));
            } else {
                PRT_("no qos storm multicast\n");
            }
        }
        if (SHOW_(bc_policer.enable)) {
            if (c.bc_policer.enable) {
                PRT_("qos storm broadcast %s\n", vtss_appl_qos_rate2txt(c.bc_policer.rate, c.bc_policer.frame_rate, buf));
            } else {
                PRT_("no qos storm broadcast\n");
            }
        }
    }

    if (CAPA->has_wred2_or_wred3) {
        int group, queue, dpl;

        for (group = 0; group < CAPA->wred_group_max; group++) {
            for (queue = 0; queue < VTSS_APPL_QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
                for (dpl = 0; dpl < CAPA->wred_dpl_max; dpl++) {
                    if (SHOW_(wred[queue][dpl][group].enable)) {
                        vtss_appl_qos_wred_t *wred = &c.wred[queue][dpl][group];

                        PRT_("%sqos wred ", wred->enable ? "" : "no ");
                        if (CAPA->wred_group_max > 1) {
                            PRT_("group %d ", group + CAPA->wred_group_min);
                        }
                        PRT_("queue %d ", queue);
                        if (CAPA->wred_dpl_max > 1) {
                            PRT_("dpl %d ", dpl + CAPA->wred_dpl_min);
                        }
                        if (wred->enable) {
                            PRT_("min-fl %u max %u %s",
                                 wred->min,
                                 wred->max,
                                 wred->max_unit == VTSS_APPL_QOS_WRED_MAX_FL ? "fill-level" : "");
                        }
                        PRT_("\n");
                    }
                }
            }
        }
    }

#if defined(VTSS_SW_OPTION_QOS_ADV)
    {
        int i;
        for (i = 0; i < 64; i++) {
            if (SHOW_(dscp_map[i].trust)) {
                if (c.dscp_map[i].trust) {
                    PRT_("qos map dscp-cos %d cos %u dpl %u\n", i, c.dscp_map[i].cos, c.dscp_map[i].dpl);
                } else {
                    PRT_("no qos map dscp-cos %d\n", i);
                }
            }
        }
    }

    {
        int i;
        for (i = 0; i < 64; i++) {
            if (SHOW_(dscp_map[i].dscp)) {
                PRT_("qos map dscp-ingress-translation %d to %u\n", i, c.dscp_map[i].dscp);
            }
        }

        for (i = 0; i < 64; i++) {
            if (SHOW_(dscp_map[i].remark)) {
                if (c.dscp_map[i].remark) {
                    PRT_("qos map dscp-classify %d\n", i);
                } else {
                    PRT_("no qos map dscp-classify %d\n", i);
                }
            }
        }

        for (i = 0; i < VTSS_PRIO_ARRAY_SIZE; i++) {
            if (SHOW_(cos_dscp_map[i].dscp)) {
                if (c.cos_dscp_map[i].dscp) {
                    PRT_("qos map cos-dscp %d dpl 0 dscp %u\n", i, c.cos_dscp_map[i].dscp);
                } else {
                    PRT_("no qos map cos-dscp %d dpl 0\n", i);
                }
            }
            if (SHOW_(cos_dscp_map[i].dscp_dp1)) {
                if (c.cos_dscp_map[i].dscp_dp1) {
                    PRT_("qos map cos-dscp %d dpl 1 dscp %u\n", i, c.cos_dscp_map[i].dscp_dp1);
                } else {
                    PRT_("no qos map cos-dscp %d dpl 1\n", i);
                }
            }
            if (CAPA->has_dscp_dp2 && SHOW_(cos_dscp_map[i].dscp_dp2)) {
                if (c.cos_dscp_map[i].dscp_dp2) {
                    PRT_("qos map cos-dscp %d dpl 2 dscp %u\n", i, c.cos_dscp_map[i].dscp_dp2);
                } else {
                    PRT_("no qos map cos-dscp %d dpl 2\n", i);
                }
            }
            if (CAPA->has_dscp_dp3 && SHOW_(cos_dscp_map[i].dscp_dp3)) {
                if (c.cos_dscp_map[i].dscp_dp3) {
                    PRT_("qos map cos-dscp %d dpl 3 dscp %u\n", i, c.cos_dscp_map[i].dscp_dp3);
                } else {
                    PRT_("no qos map cos-dscp %d dpl 3\n", i);
                }
            }
        }

        for (i = 0; i < 64; i++) {
            if (CAPA->has_dscp_dpl_remark) {
                if (SHOW_(dscp_map[i].dscp_remap)) {
                    PRT_("qos map dscp-egress-translation %d 0 to %u\n", i, c.dscp_map[i].dscp_remap);
                }
                if (SHOW_(dscp_map[i].dscp_remap_dp1)) {
                    PRT_("qos map dscp-egress-translation %d 1 to %u\n", i, c.dscp_map[i].dscp_remap_dp1);
                }
            } else {
                if (SHOW_(dscp_map[i].dscp_remap)) {
                    PRT_("qos map dscp-egress-translation %d to %u\n",   i, c.dscp_map[i].dscp_remap);
                }
            }
        }
    }
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

    return VTSS_RC_OK;
}

static mesa_rc QOS_ICFG_port_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    vtss_appl_qos_port_conf_t dc;
    vtss_appl_qos_port_conf_t c;
    vtss_isid_t               isid = topo_usid2isid(req->instance_id.port.usid);
    mesa_port_no_t            iport = uport2iport(req->instance_id.port.begin_uport);
    char                      buf[32];

    IC_RC(vtss_appl_qos_port_conf_get_default(&dc));
    IC_RC(vtss_appl_qos_port_conf_get(isid, iport, &c));

    if (SHOW_(port.default_cos)) {
        PRT_(" qos cos %u\n", c.port.default_cos);
    }
    if (SHOW_(port.default_pcp)) {
        PRT_(" qos pcp %u\n", c.port.default_pcp);
    }

    if (SHOW_(port.default_dpl)) {
        PRT_(" qos dpl %u\n", c.port.default_dpl);
    }
    if (SHOW_(port.default_dei)) {
        PRT_(" qos dei %u\n", c.port.default_dei);
    }

    if (CAPA->has_cosid_classification && SHOW_(port.default_cosid)) {
        PRT_(" qos class %u\n", c.port.default_cosid);
    }

#if defined(VTSS_SW_OPTION_QOS_ADV)
    if (SHOW_(port.trust_tag)) {
        if (c.port.trust_tag) {
            PRT_(" qos trust tag\n");
        } else {
            PRT_(" no qos trust tag\n");
        }
    }

    {
        int pcp, dei;
        for (pcp = 0; pcp < VTSS_PCP_ARRAY_SIZE; pcp++) {
            for (dei = 0; dei < VTSS_DEI_ARRAY_SIZE; dei++) {
                if (SHOW_(tag_cos_map[pcp][dei].cos) || SHOW_(tag_cos_map[pcp][dei].dpl)) {
                    PRT_(" qos map tag-cos pcp %d dei %d cos %u dpl %u\n", pcp, dei, c.tag_cos_map[pcp][dei].cos, c.tag_cos_map[pcp][dei].dpl);
                }
            }
        }
    }
    if (SHOW_(port.trust_dscp)) {
        if (c.port.trust_dscp) {
            PRT_(" qos trust dscp\n");
        } else {
            PRT_(" no qos trust dscp\n");
        }
    }
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

    {
        BOOL fc = c.port_policer.flow_control;

        if (SHOW_(port_policer.enable)) {
            if (c.port_policer.enable) {
                PRT_(" qos policer %s %s\n", vtss_appl_qos_rate2txt(c.port_policer.cir, c.port_policer.frame_rate, buf), fc ? "flowcontrol" : "");
            } else {
                PRT_(" no qos policer\n");
            }
        }
    }

#if defined(VTSS_SW_OPTION_QOS_ADV)
    {
        int i;
        for (i = 0; i < VTSS_APPL_QOS_PORT_QUEUE_CNT; i++) {
            if (SHOW_(queue_policer[i].enable)) {
                if (c.queue_policer[i].enable) {
                    PRT_(" qos queue-policer queue %d %s\n", i, vtss_appl_qos_rate2txt(c.queue_policer[i].cir, FALSE, buf));
                } else {
                    PRT_(" no qos queue-policer queue %d\n", i);
                }
            }
        }
    }
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

    if (SHOW_(port_shaper.enable)) {
        if (c.port_shaper.enable) {
            PRT_(" qos shaper %s%s\n",
                 vtss_appl_qos_rate2txt(c.port_shaper.cir, FALSE, buf),
                 (CAPA->has_shapers_rt && c.port_shaper.mode == VTSS_APPL_QOS_SHAPER_MODE_DATA) ? " rate-type data" : "");
        } else {
            PRT_(" no qos shaper\n");
        }
    }

    {
        int i;
        for (i = 0; i < VTSS_APPL_QOS_PORT_QUEUE_CNT; i++) {
            if (SHOW_(queue_shaper[i].enable)) {
                if (c.queue_shaper[i].enable) {
                    PRT_(" qos queue-shaper queue %d %s%s%s%s\n",
                         i,
                         vtss_appl_qos_rate2txt(c.queue_shaper[i].cir, c.queue_shaper[i].mode == VTSS_APPL_QOS_SHAPER_MODE_FRAME, buf),
                         (CAPA->has_queue_shapers_eb && c.queue_shaper[i].excess) ? " excess" : "",
                         (CAPA->has_queue_shapers_crb && c.queue_shaper[i].credit) ? " credit" : "",
                         (CAPA->has_shapers_rt && c.queue_shaper[i].mode == VTSS_APPL_QOS_SHAPER_MODE_DATA) ? " rate-type data" : "");
                } else {
                    PRT_(" no qos queue-shaper queue %d\n", i);
                }
            }
        }
    }
    if (CAPA->has_queue_cut_through) {
        int i;
        for (i = 0; i < VTSS_APPL_QOS_PORT_QUEUE_CNT; i++) {
            if (SHOW_(scheduler[i].cut_through)) {
                if (c.scheduler[i].cut_through) {
                    PRT_(" qos cut-through queue %d\n", i);
                } else {
                    PRT_(" no qos cut-through queue %d\n", i);
                }
            }
        }
    }
    if (SHOW_(port.dwrr_cnt)) {
        if (c.port.dwrr_cnt) {
            int i;
            PRT_(" qos wrr");
            for (i = 0; i < c.port.dwrr_cnt; i++) {
                PRT_(" %u", c.scheduler[i].weight);
            }
            PRT_("\n");
        } else {
            PRT_(" no qos wrr\n");
        }
    }


#if defined(VTSS_SW_OPTION_QOS_ADV)
    if (SHOW_(port.tag_remark_mode)) {
        switch (c.port.tag_remark_mode) {
        case VTSS_APPL_QOS_TAG_REMARK_MODE_CLASSIFIED:
            PRT_(" no qos tag-remark\n");
            break;
        case VTSS_APPL_QOS_TAG_REMARK_MODE_DEFAULT:
            PRT_(" qos tag-remark pcp %u dei %u\n", c.port.tag_default_pcp, c.port.tag_default_dei);
            break;
        case VTSS_APPL_QOS_TAG_REMARK_MODE_MAPPED: {
            PRT_(" qos tag-remark mapped\n");
            break;
        }
        default:
            break;
        }
    }

    {
        int cos, dpl;
        for (cos = 0; cos < VTSS_APPL_QOS_PORT_PRIO_CNT; cos++) {
            for (dpl = 0; dpl < 2; dpl++) {
                if (SHOW_(cos_tag_map[cos][dpl].pcp) || SHOW_(cos_tag_map[cos][dpl].dei)) {
                    PRT_(" qos map cos-tag cos %d dpl %d pcp %u dei %u\n", cos, dpl, c.cos_tag_map[cos][dpl].pcp, c.cos_tag_map[cos][dpl].dei);
                }
            }
        }
    }

    if (SHOW_(port.dscp_translate)) {
        if (c.port.dscp_translate) {
            PRT_(" qos dscp-translate\n");
        } else {
            PRT_(" no qos dscp-translate\n");
        }
    }

    if (SHOW_(port.dscp_imode)) {
        switch (c.port.dscp_imode) {
        case VTSS_APPL_QOS_DSCP_MODE_NONE:
            PRT_(" no qos dscp-classify\n");
            break;
        case VTSS_APPL_QOS_DSCP_MODE_ZERO:
            PRT_(" qos dscp-classify zero\n");
            break;
        case VTSS_APPL_QOS_DSCP_MODE_SEL:
            PRT_(" qos dscp-classify selected\n");
            break;
        case VTSS_APPL_QOS_DSCP_MODE_ALL:
            PRT_(" qos dscp-classify any\n");
            break;
        default:
            break;
        }
    }

    if (SHOW_(port.dscp_emode)) {
        switch (c.port.dscp_emode) {
        case VTSS_APPL_QOS_DSCP_EMODE_DISABLE:
            PRT_(" no qos dscp-remark\n");
            break;
        case VTSS_APPL_QOS_DSCP_EMODE_REMARK:
            PRT_(" qos dscp-remark rewrite\n");
            break;
        case VTSS_APPL_QOS_DSCP_EMODE_REMAP:
            PRT_(" qos dscp-remark remap\n");
            break;
        case VTSS_APPL_QOS_DSCP_EMODE_REMAP_DPA:
            if (CAPA->has_dscp_dpl_remark) {
                PRT_(" qos dscp-remark remap-dp\n");
            }
            break;
        default:
            break;
        }
    }
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

    if (CAPA->has_port_storm_policers) {
        if (SHOW_(uc_policer.enable)) {
            if (c.uc_policer.enable) {
                PRT_(" qos storm unicast %s\n", vtss_appl_qos_rate2txt(c.uc_policer.cir, c.uc_policer.frame_rate, buf));
            } else {
                PRT_(" no qos storm unicast\n");
            }
        }
        if (SHOW_(bc_policer.enable)) {
            if (c.bc_policer.enable) {
                PRT_(" qos storm broadcast %s\n", vtss_appl_qos_rate2txt(c.bc_policer.cir, c.bc_policer.frame_rate, buf));
            } else {
                PRT_(" no qos storm broadcast\n");
            }
        }
        if (SHOW_(un_policer.enable)) {
            if (c.un_policer.enable) {
                PRT_(" qos storm unknown %s\n", vtss_appl_qos_rate2txt(c.un_policer.cir, c.un_policer.frame_rate, buf));
            } else {
                PRT_(" no qos storm unknown\n");
            }
        }
    }

    if (CAPA->has_qce_address_mode && SHOW_(port.dmac_dip)) {
        if (c.port.dmac_dip) {
            PRT_(" qos qce addr destination\n");
        } else {
            PRT_(" no qos qce addr\n");
        }
    }

    if (CAPA->has_qce_key_type && SHOW_(port.key_type)) {
        if (c.port.key_type != MESA_VCAP_KEY_TYPE_NORMAL) {
            PRT_(" qos qce key %s\n", vtss_appl_qos_qcl_key_type2txt(c.port.key_type, FALSE));
        } else {
            PRT_(" no qos qce key\n");
        }
    }

    if (CAPA->wred_group_max > 1 && SHOW_(port.wred_group)) {
        if (c.port.wred_group) {
            PRT_(" qos wred-group %u\n", c.port.wred_group);
        } else {
            PRT_(" no qos wred-group\n");
        }
    }

    if (CAPA->has_ingress_map && SHOW_(port.ingress_map)) {
        if (c.port.ingress_map != MESA_QOS_MAP_ID_NONE) {
            PRT_(" qos ingress-map %u\n", c.port.ingress_map);
        } else {
            PRT_(" no qos ingress-map\n");
        }
    }

    if (CAPA->has_egress_map && SHOW_(port.egress_map)) {
        if (c.port.egress_map != MESA_QOS_MAP_ID_NONE) {
            PRT_(" qos egress-map %u\n", c.port.egress_map);
        } else {
            PRT_(" no qos egress-map\n");
        }
    }

    return VTSS_RC_OK;
}

static mesa_rc QOS_ICFG_qce_range(vtss_icfg_query_result_t *result, const char *name, mesa_vcap_vr_t *range)
{
    if (range->type != MESA_VCAP_VR_TYPE_VALUE_MASK) {
        IC_RC(vtss_icfg_printf(result, "%s %u-%u", name, range->vr.r.low, range->vr.r.high));
    } else if (range->vr.v.mask) {
        IC_RC(vtss_icfg_printf(result, "%s %u", name, range->vr.v.value));
    }
    return VTSS_RC_OK;
}

static mesa_rc QOS_ICFG_qce_proto(vtss_icfg_query_result_t *result, const char *name, mesa_vcap_u8_t *proto)
{
    char buf[32];

    if (proto->mask) {
        IC_RC(vtss_icfg_printf(result, "%s %s", name, vtss_appl_qos_qcl_proto2txt(proto, buf)));
    }
    return VTSS_RC_OK;
}

static mesa_rc QOS_ICFG_qce_ipv4(vtss_icfg_query_result_t *result, const char *name, mesa_vcap_ip_t *ip)
{
    ulong i, n = 0;
    char  buf[64];

    if (ip->mask) {
        for (i = 0; i < 32; i++) {
            if (ip->mask & (1 << i)) {
                n++;
            }
        }
        (void)misc_ipv4_txt(ip->value, buf);
        sprintf(&buf[strlen(buf)], "/" VPRIlu, n);
        IC_RC(vtss_icfg_printf(result, "%s %s", name, buf));
    }
    return VTSS_RC_OK;
}

static mesa_rc QOS_ICFG_qce_ipv6(vtss_icfg_query_result_t *result, const char *name, mesa_vcap_u128_t *ipv6)
{
    u32            i, j;
    mesa_vcap_ip_t ipv4;

    ipv4.value = 0;
    ipv4.mask = 0;
    for (i = 0; i < 4; i++) {
        j = ((3 - i) * 8);
        ipv4.value += (ipv6->value[i + 12] << j);
        ipv4.mask += (ipv6->mask[i + 12] << j);
    }
    return QOS_ICFG_qce_ipv4(result, name, &ipv4);
}

static mesa_rc QOS_ICFG_qce_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    mesa_qce_id_t                   id = VTSS_APPL_QOS_QCE_ID_NONE;
    vtss_appl_qos_qce_intern_conf_t conf;
    mesa_qce_t                      *const q = &conf.qce;
    mesa_qce_key_t                  *const k = &q->key;
    mesa_qce_action_t               *const a = &q->action;
    char                            *buf = (char *)VTSS_MALLOC(ICLI_STR_MAX_LEN); /* Using dynamic memory to prevent stack size overflow */

#define IC_QCE_RC(expr) { mesa_rc __rc__ = (expr); if (__rc__ < VTSS_RC_OK) {VTSS_FREE(buf); return __rc__;} }

    if (buf == NULL) {
        return VTSS_RC_ERROR;
    }

    while (vtss_appl_qos_qce_intern_get(VTSS_ISID_GLOBAL, VTSS_APPL_QOS_QCL_USER_STATIC, id, &conf, TRUE) == VTSS_RC_OK) {
        const char     *txt;
        int            port_cnt;
        int            enabled_port_cnt;
        port_iter_t    pit;

        id = q->id;

        // qce id:
        IC_QCE_RC(vtss_icfg_printf(result, "qos qce %u", id));

        // interfaces:
        port_cnt = 0;
        enabled_port_cnt = 0;
        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            port_cnt++;
            if (k->port_list[pit.iport]) {
                enabled_port_cnt++;
            }
        }

        if (enabled_port_cnt < port_cnt) {
            IC_QCE_RC(vtss_icfg_printf(result, " interface %s", icli_port_list_info_txt(VTSS_ISID_START, k->port_list, buf, FALSE)));
        }

        // mac:
        {
            mesa_qce_mac_t *mac = &k->mac;

            // dmac
            txt = NULL;
            if ((mac->dmac_bc != MESA_VCAP_BIT_ANY) || (mac->dmac_mc != MESA_VCAP_BIT_ANY)) {
                txt = (mac->dmac_bc == MESA_VCAP_BIT_1 ? "broadcast" :
                       mac->dmac_mc == MESA_VCAP_BIT_1 ? "multicast" :
                       mac->dmac_mc == MESA_VCAP_BIT_0 ? "unicast" :
                       NULL);
            }
            if (CAPA->has_qce_dmac && txt == NULL) {
                if (mac->dmac.mask[0] || mac->dmac.mask[1] || mac->dmac.mask[2] ||
                    mac->dmac.mask[3] || mac->dmac.mask[4] || mac->dmac.mask[5]) {
                    txt = misc_mac_txt(mac->dmac.value, buf);
                }
            }
            if (txt) {
                IC_QCE_RC(vtss_icfg_printf(result, " dmac %s", txt));
            }

            // smac:
            if (mac->smac.mask[0] || mac->smac.mask[1] || mac->smac.mask[2] ||
                mac->smac.mask[3] || mac->smac.mask[4] || mac->smac.mask[5]) {
                IC_QCE_RC(vtss_icfg_printf(result, " smac %s", misc_mac_txt(mac->smac.value, buf)));
            }
        }

        // tag:
        if ((k->tag.tagged != MESA_VCAP_BIT_ANY)                                        ||
            ((k->tag.vid.type != MESA_VCAP_VR_TYPE_VALUE_MASK) || k->tag.vid.vr.v.mask) ||
            (k->tag.pcp.mask & 7)                                                       ||
            (k->tag.dei != MESA_VCAP_BIT_ANY)) {
            u8 pcp_mask = k->tag.pcp.mask & 7;

            IC_QCE_RC(vtss_icfg_printf(result, " tag"));
            if (k->tag.tagged != MESA_VCAP_BIT_ANY) {
                IC_QCE_RC(vtss_icfg_printf(result, " type %s", vtss_appl_qos_qcl_tag_type2txt(k->tag.tagged, k->tag.s_tag, FALSE)));
            }

            // vid:
            IC_QCE_RC(QOS_ICFG_qce_range(result, " vid", &k->tag.vid));

            // pcp:
            if (pcp_mask) {
                if (pcp_mask == 7) {
                    IC_QCE_RC(vtss_icfg_printf(result, " pcp %u", k->tag.pcp.value));
                } else {
                    IC_QCE_RC(vtss_icfg_printf(result, " pcp %u-%u", k->tag.pcp.value, k->tag.pcp.value + ((pcp_mask == 6) ? 1 : 3)));
                }
            }

            // dei:
            if (k->tag.dei != MESA_VCAP_BIT_ANY) {
                IC_QCE_RC(vtss_icfg_printf(result, " dei %u", (k->tag.dei == MESA_VCAP_BIT_0) ? 0 : 1));
            }
        }

        if (CAPA->has_qce_inner_tag) {
            // inner tag:
            if ((k->inner_tag.tagged != MESA_VCAP_BIT_ANY)                                              ||
                ((k->inner_tag.vid.type != MESA_VCAP_VR_TYPE_VALUE_MASK) || k->inner_tag.vid.vr.v.mask) ||
                (k->inner_tag.pcp.mask & 7)                                                             ||
                (k->inner_tag.dei != MESA_VCAP_BIT_ANY)) {
                u8 pcp_mask = k->inner_tag.pcp.mask & 7;

                IC_QCE_RC(vtss_icfg_printf(result, " inner-tag"));
                if (k->inner_tag.tagged != MESA_VCAP_BIT_ANY) {
                    IC_QCE_RC(vtss_icfg_printf(result, " type %s", vtss_appl_qos_qcl_tag_type2txt(k->inner_tag.tagged, k->inner_tag.s_tag, FALSE)));
                }

                // vid:
                IC_QCE_RC(QOS_ICFG_qce_range(result, " vid", &k->inner_tag.vid));

                // pcp:
                if (pcp_mask) {
                    if (pcp_mask == 7) {
                        IC_QCE_RC(vtss_icfg_printf(result, " pcp %u", k->inner_tag.pcp.value));
                    } else {
                        IC_QCE_RC(vtss_icfg_printf(result, " pcp %u-%u", k->inner_tag.pcp.value, k->inner_tag.pcp.value + ((pcp_mask == 6) ? 1 : 3)));
                    }
                }

                // dei:
                if (k->inner_tag.dei != MESA_VCAP_BIT_ANY) {
                    IC_QCE_RC(vtss_icfg_printf(result, " dei %u", (k->inner_tag.dei == MESA_VCAP_BIT_0) ? 0 : 1));
                }
            }
        }

        // frametype:
        if (k->type != MESA_QCE_TYPE_ANY) {
            IC_QCE_RC(vtss_icfg_printf(result, " frame-type"));
            switch (k->type) {
            case MESA_QCE_TYPE_ETYPE: {
                mesa_qce_frame_etype_t *etype = &k->frame.etype;
                IC_QCE_RC(vtss_icfg_printf(result, " etype"));
                if (etype->etype.mask[0] || etype->etype.mask[1]) {
                    IC_QCE_RC(vtss_icfg_printf(result, " 0x%x", (etype->etype.value[0] << 8) | etype->etype.value[1]));
                }
                break;
            }
            case MESA_QCE_TYPE_LLC: {
                mesa_qce_frame_llc_t *llc = &k->frame.llc;
                IC_QCE_RC(vtss_icfg_printf(result, " llc"));
                if (llc->data.mask[0]) {
                    IC_QCE_RC(vtss_icfg_printf(result, " dsap 0x%x", llc->data.value[0]));
                }
                if (llc->data.mask[1]) {
                    IC_QCE_RC(vtss_icfg_printf(result, " ssap 0x%x", llc->data.value[1]));
                }
                if (llc->data.mask[2]) {
                    IC_QCE_RC(vtss_icfg_printf(result, " control 0x%x", llc->data.value[2]));
                }
                break;
            }
            case MESA_QCE_TYPE_SNAP: {
                mesa_qce_frame_snap_t *snap = &k->frame.snap;
                IC_QCE_RC(vtss_icfg_printf(result, " snap"));
                if (snap->data.mask[3] || snap->data.mask[4]) {
                    IC_QCE_RC(vtss_icfg_printf(result, " 0x%x", (snap->data.value[3] << 8) | snap->data.value[4]));
                }
                break;
            }
            case MESA_QCE_TYPE_IPV4: {
                mesa_qce_frame_ipv4_t *i4 = &k->frame.ipv4;
                IC_QCE_RC(vtss_icfg_printf(result, " ipv4"));
                IC_QCE_RC(QOS_ICFG_qce_proto(result, " proto", &i4->proto));
                IC_QCE_RC(QOS_ICFG_qce_ipv4(result, " sip", &i4->sip));
                if (CAPA->has_qce_dip) {
                    IC_QCE_RC(QOS_ICFG_qce_ipv4(result, " dip", &i4->dip));
                }
                IC_QCE_RC(QOS_ICFG_qce_range(result, " dscp", &i4->dscp));
                if (i4->fragment != MESA_VCAP_BIT_ANY) {
                    IC_QCE_RC(vtss_icfg_printf(result, " frag %s", (i4->fragment == MESA_VCAP_BIT_0) ? "no" : "yes"));
                }
                if (i4->proto.mask && (i4->proto.value == 6 || i4->proto.value == 17)) {
                    IC_QCE_RC(QOS_ICFG_qce_range(result, " sport", &i4->sport));
                    IC_QCE_RC(QOS_ICFG_qce_range(result, " dport", &i4->dport));
                }
                break;
            }
            case MESA_QCE_TYPE_IPV6: {
                mesa_qce_frame_ipv6_t *i6 = &k->frame.ipv6;
                IC_QCE_RC(vtss_icfg_printf(result, " ipv6"));
                IC_QCE_RC(QOS_ICFG_qce_proto(result, " proto", &i6->proto));
                IC_QCE_RC(QOS_ICFG_qce_ipv6(result, " sip", &i6->sip));
                if (CAPA->has_qce_dip) {
                    IC_QCE_RC(QOS_ICFG_qce_ipv6(result, " dip", &i6->dip));
                }
                IC_QCE_RC(QOS_ICFG_qce_range(result, " dscp", &i6->dscp));
                if (i6->proto.mask && (i6->proto.value == 6 || i6->proto.value == 17)) {
                    IC_QCE_RC(QOS_ICFG_qce_range(result, " sport", &i6->sport));
                    IC_QCE_RC(QOS_ICFG_qce_range(result, " dport", &i6->dport));
                }
                break;
            }
            default:
                break;
            }
        }

        if (a->prio_enable      ||
            a->dp_enable        ||
            a->pcp_dei_enable   ||
            a->policy_no_enable ||
            a->map_id_enable    ||
            a->dscp_enable) {

            IC_QCE_RC(vtss_icfg_printf(result, " action"));
            if (a->prio_enable) {
                IC_QCE_RC(vtss_icfg_printf(result, " cos %u", a->prio));
            }
            if (a->dp_enable) {
                IC_QCE_RC(vtss_icfg_printf(result, " dpl %u", a->dp));
            }
            if (a->dscp_enable) {
                IC_QCE_RC(vtss_icfg_printf(result, " dscp %u", a->dscp));
            }
            if (a->pcp_dei_enable) {
                IC_QCE_RC(vtss_icfg_printf(result, " pcp-dei %u %u", a->pcp, a->dei));
            }
            if (a->policy_no_enable) {
                IC_QCE_RC(vtss_icfg_printf(result, " policy %u", a->policy_no));
            }
            if (CAPA->has_ingress_map && a->map_id_enable) {
                IC_QCE_RC(vtss_icfg_printf(result, " ingress-map %u", a->map_id));
            }
        }

        IC_QCE_RC(vtss_icfg_printf(result, "\n"));
    }

    VTSS_FREE(buf);
    return VTSS_RC_OK;
}

#if defined(VTSS_SW_OPTION_QOS_ADV)
static mesa_rc QOS_ICFG_imap_values(const vtss_icfg_query_request_t *req,
                                    vtss_icfg_query_result_t *result,
                                    vtss_appl_qos_ingress_map_values_t *c,
                                    vtss_appl_qos_ingress_map_values_t *dc)
{
    if (req->all_defaults || (c->cosid != dc->cosid)) {
        PRT_(" class %u", c->cosid);
    }
    if (req->all_defaults || (c->cos != dc->cos)) {
        PRT_(" cos %u", c->cos);
    }
    if (req->all_defaults || (c->dei != dc->dei)) {
        PRT_(" dei %u", c->dei);
    }
    if (req->all_defaults || (c->dpl != dc->dpl)) {
        PRT_(" dpl %u", c->dpl);
    }
    if (req->all_defaults || (c->dscp != dc->dscp)) {
        PRT_(" dscp %u", c->dscp);
    }
    if (req->all_defaults || (c->pcp != dc->pcp)) {
        PRT_(" pcp %u", c->pcp);
    }
    PRT_("\n");
    return VTSS_RC_OK;
}

static mesa_rc QOS_ICFG_imap_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    int                        i, j;
    vtss_appl_qos_imap_entry_t c, dc;
    mesa_qos_ingress_map_id_t  id = (mesa_qos_ingress_map_id_t)req->instance_id.map_id;

    IC_RC(vtss_appl_qos_imap_entry_get(id, &c));
    IC_RC(vtss_appl_qos_imap_entry_get_default(id, &dc));

    if (SHOW_(conf.key)) {
        PRT_(" key %s\n", vtss_appl_qos_ingress_map_key2txt(c.conf.key, FALSE));
    }

    if (req->all_defaults || memcmp(&c.conf.action, &dc.conf.action, sizeof(c.conf.action))) {
        if (memcmp(&c.conf.action, &dc.conf.action, sizeof(c.conf.action))) {
            PRT_(" action");
            if (c.conf.action.cosid) {
                PRT_(" class");
            }
            if (c.conf.action.cos) {
                PRT_(" cos");
            }
            if (c.conf.action.dei) {
                PRT_(" dei");
            }
            if (c.conf.action.dpl) {
                PRT_(" dpl");
            }
            if (c.conf.action.dscp) {
                PRT_(" dscp");
            }
            if (c.conf.action.pcp) {
                PRT_(" pcp");
            }
            PRT_("\n");
        } else {
            PRT_(" no action\n");
        }
    }

    for (i = 0; i < VTSS_PCPS; i++) {
        for (j = 0; j < VTSS_DEIS; j++) {
            if (req->all_defaults || memcmp(&c.pcp_dei[i][j], &dc.pcp_dei[i][j], sizeof(c.pcp_dei[i][j]))) {
                PRT_(" map pcp %u dei %u to", i, j);
                QOS_ICFG_imap_values(req, result, &c.pcp_dei[i][j], &dc.pcp_dei[i][j]);
            }
        }
    }
    for (i = 0; i < 64; i++) {
        if (req->all_defaults || memcmp(&c.dscp[i], &dc.dscp[i], sizeof(c.dscp[i]))) {
            PRT_(" map dscp %u to", i);
            QOS_ICFG_imap_values(req, result, &c.dscp[i], &dc.dscp[i]);
        }
    }

    return VTSS_RC_OK;
}

static mesa_rc QOS_ICFG_emap_values(const vtss_icfg_query_request_t *req,
                                    vtss_icfg_query_result_t *result,
                                    vtss_appl_qos_egress_map_values_t *c,
                                    vtss_appl_qos_egress_map_values_t *dc)
{
    if (req->all_defaults || (c->dei != dc->dei)) {
        PRT_(" dei %u", c->dei);
    }
    if (req->all_defaults || (c->dscp != dc->dscp)) {
        PRT_(" dscp %u", c->dscp);
    }
    if (req->all_defaults || (c->pcp != dc->pcp)) {
        PRT_(" pcp %u", c->pcp);
    }
    PRT_("\n");
    return VTSS_RC_OK;
}

static mesa_rc QOS_ICFG_emap_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    int                        i, j;
    vtss_appl_qos_emap_entry_t c, dc;
    mesa_qos_egress_map_id_t   id = (mesa_qos_egress_map_id_t)req->instance_id.map_id;
    u32                        dpl_cnt = fast_cap(MESA_CAP_QOS_DPL_CNT);

    IC_RC(vtss_appl_qos_emap_entry_get(id, &c));
    IC_RC(vtss_appl_qos_emap_entry_get_default(id, &dc));

    if (SHOW_(conf.key)) {
        PRT_(" key %s\n", vtss_appl_qos_egress_map_key2txt(c.conf.key, FALSE));
    }

    if (req->all_defaults || memcmp(&c.conf.action, &dc.conf.action, sizeof(c.conf.action))) {
        if (memcmp(&c.conf.action, &dc.conf.action, sizeof(c.conf.action))) {
            PRT_(" action");
            if (c.conf.action.dei) {
                PRT_(" dei");
            }
            if (c.conf.action.dscp) {
                PRT_(" dscp");
            }
            if (c.conf.action.pcp) {
                PRT_(" pcp");
            }
            PRT_("\n");
        } else {
            PRT_(" no action\n");
        }
    }

    for (i = 0; i < VTSS_COSIDS; i++) {
        for (j = 0; j < dpl_cnt; j++) {
            if (req->all_defaults || memcmp(&c.cosid_dpl[i][j], &dc.cosid_dpl[i][j], sizeof(c.cosid_dpl[i][j]))) {
                PRT_(" map class %u dpl %u to", i, j);
                QOS_ICFG_emap_values(req, result, &c.cosid_dpl[i][j], &dc.cosid_dpl[i][j]);
            }
        }
    }
    for (i = 0; i < 64; i++) {
        for (j = 0; j < dpl_cnt; j++) {
            if (req->all_defaults || memcmp(&c.dscp_dpl[i][j], &dc.dscp_dpl[i][j], sizeof(c.dscp_dpl[i][j]))) {
                PRT_(" map dscp %u dpl %u to", i, j);
                QOS_ICFG_emap_values(req, result, &c.dscp_dpl[i][j], &dc.dscp_dpl[i][j]);
            }
        }
    }

    return VTSS_RC_OK;
}

#endif /* VTSS_SW_OPTION_QOS_ADV */

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
mesa_rc qos_icfg_init(void)
{
    IC_RC(vtss_icfg_query_register(VTSS_ICFG_QOS_GLOBAL_CONF, "qos", QOS_ICFG_global_conf));
    IC_RC(vtss_icfg_query_register(VTSS_ICFG_QOS_PORT_CONF, "qos", QOS_ICFG_port_conf));
    IC_RC(vtss_icfg_query_register(VTSS_ICFG_QOS_QCE_CONF, "qos", QOS_ICFG_qce_conf));

#if defined(VTSS_SW_OPTION_QOS_ADV)
    if (CAPA->has_ingress_map) {
        IC_RC(vtss_icfg_query_register(VTSS_ICFG_QOS_INGRESS_MAP_CONF, "qos", QOS_ICFG_imap_conf));
    }

    if (CAPA->has_egress_map) {
        IC_RC(vtss_icfg_query_register(VTSS_ICFG_QOS_EGRESS_MAP_CONF, "qos", QOS_ICFG_emap_conf));
    }
#endif /* VTSS_SW_OPTION_QOS_ADV */

    return VTSS_RC_OK;
}
