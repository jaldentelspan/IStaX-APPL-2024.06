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

#ifndef __VTSS_VCAP_API_H__
#define __VTSS_VCAP_API_H__

#include <main_types.h>
#include <vtss/appl/vcap_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Public functions
 ****************************************************************************/

/*!
 * \brief Convert VCAP mesa_vcap_vid_t to mesa_vid_t.
 *
 * \param vcap_vid [IN] VCAP VID.
 *
 * \return mesa_vid_t type value. Possible values are: 0(disabled), 1-4095.
 */
mesa_vid_t vtss_appl_vcap_vid_type2value(mesa_vcap_vid_t vcap_vid);

/*!
 * \brief Convert mesa_vid_t VCAP mesa_vcap_vid_t.
 *
 * 0(disabled): vcap.value = vcap.mask = 0;
 * 1-4095     : vcap.value = 1-4095; vcap.mask = 0xFFF;
 *
 * \param vid_value [IN]  mesa_vid_t type value.
 * \param vcap_vid  [OUT] VCAP VID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 *         VTSS_RC_ERROR if the operation failed.
 */
mesa_rc vtss_appl_vcap_vid_value2type(mesa_vid_t vid_value, mesa_vcap_vid_t *const vcap_vid);

/*!
 * \brief Convert VCAP mesa_vcap_u8_t (3 bits only) to vlan_pri_type.
 *
 * \param vcap_vlan_pri [IN] VCAP VLAN tagged priority.
 *
 * \return vlan_pri_type type. Possible values are: 0-7, 8(0-1), 9(2-3), 10(4-5), 11(6-7), 12(0-3), 13(4-7), 14(any).
 */
vtss_appl_vcap_vlan_pri_type_t vtss_appl_vcap_vlan_pri2pri_type(mesa_vcap_u8_t vcap_vlan_pri);

/*!
 * \brief Convert vlan_pri_type to VCAP mesa_vcap_u8_t (3 bits only).
 *
 *   0-7    : vcap.value = 0-7; vcap.mask = 7;
 *   8(0-1) : vcap.value = 1;   vcap.mask = 6;
 *   9(2-3) : vcap.value = 3;   vcap.mask = 6;
 *   10(4-5): vcap.value = 5;   vcap.mask = 6;
 *   11(6-7): vcap.value = 7;   vcap.mask = 6;
 *   12(0-3): vcap.value = 1;   vcap.mask = 4;
 *   13(4-7): vcap.value = 4;   vcap.mask = 4;
 *   14(any): vcap.value = vcap.mask = 0;
 *
 * \param pri_type      [IN]  vlan_pri_type.
 * \param vcap_vlan_pri [OUT] VCAP VLAN tagged priority.
 *
 * \return Void.
 */
void vtss_appl_vcap_pri_type2vlan_pri(vtss_appl_vcap_vlan_pri_type_t pri_type, mesa_vcap_u8_t *const vcap_vlan_pri);

/*!
 * \brief Convert VCAP tag bits to tag_type.
 *
 * \param tagged [IN] VCAP 'tagged' bit.
 * \param s_tag  [IN] VCAP 's_tag' bit.
 *
 * \return tag_type.
 */
vtss_appl_vcap_vlan_tag_type_t vtss_appl_vcap_tag_bits2tag_type(mesa_vcap_bit_t tagged, mesa_vcap_bit_t s_tag);

/*!
 * \brief Convert tag_type to VCAP tag bits.
 *
 * \param tag_type [IN]  VCAP tag_type.
 * \param tagged   [OUT] VCAP 'tagged' bit.
 * \param s_tag    [OUT] VCAP 's_tag' bit.
 *
 * \return Void.
 */
void vtss_appl_vcap_tag_type2tag_bits(vtss_appl_vcap_vlan_tag_type_t tag_type, mesa_vcap_bit_t *tagged, mesa_vcap_bit_t *s_tag);

/*!
 * \brief Convert VCAP Value/Range to Any/Specific/Range.
 *
 * \param vr  [IN]  The Value/Range.
 * \param asr [OUT] The Any/Specific/Range.
 *
 * \return Void.
 */
void vtss_appl_vcap_vr2asr(const mesa_vcap_vr_t *vr, vtss_appl_vcap_asr_t *asr);

/*!
 * \brief Convert Any/Specific/Range to VCAP Value/Range.
 *
 * \param asr [IN]  The Any/Specific/Range.
 * \param vr  [OUT] The Value/Range.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_vcap_asr2vr(const vtss_appl_vcap_asr_t *asr, mesa_vcap_vr_t *vr, u16 mask);

/*!
 * \brief Convert VCAP Value/Range to Range.
 *
 * \param vr    [IN]  The Value/Range.
 * \param range [OUT] The Range.
 * \param max   [IN]  The maximum value of the Range
 *
 * \return Void.
 */
void vtss_appl_vcap_vr2range(const mesa_vcap_vr_t *vr, vtss_appl_vcap_range_t *range, u16 max);

/*!
 * \brief Convert Range to VCAP Value/Rangee.
 *
 * \param range    [IN]  The Range.
 * \param vr       [OUT] The Value/Range.
 * \param max      [IN]  The maximum value of the Range
 * \param no_range [IN]  The indication that range checkers are not allowed
 *
 * \return Error code.
 */
mesa_rc vtss_appl_vcap_range2vr(const vtss_appl_vcap_range_t *range, mesa_vcap_vr_t *vr, u16 max, BOOL no_range);


/*!
 * \brief Convert VCAP mesa_vcap_u16_t to mesa_etype_t.
 *
 * \param vcap_etype [IN] VCAP Ethernet type.
 *
 * \return mesa_etype_t type value. Possible values are: 0(disabled), 0x600-0xFFFF(exclude 0x0800, 0x0806 and 0x86DD).
 */
mesa_etype_t vtss_appl_vcap_etype_type2value(mesa_vcap_u16_t vcap_etype);

/*!
 * \brief Convert mesa_etype_t VCAP mesa_vcap_u16_t.
 *
 * 0(disabled) : vcap.value = vcap.mask = 0;
 * 0x600-0xFFFF: vcap.value = 0x600-0xFFFF; vcap.mask = 0xFFFF;
 *
 * \param etype_value [IN]  mesa_etype_t type value.
 * \param vcap_etype  [OUT] VCAP Ethernet type.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 *         VTSS_RC_ERROR if the operation failed.
 */
mesa_rc vtss_appl_vcap_etype_value2type(mesa_etype_t etype_value, mesa_vcap_u16_t *const vcap_etype);

#ifdef __cplusplus
}
#endif
#endif /* __VTSS_VCAP_API_H__ */
