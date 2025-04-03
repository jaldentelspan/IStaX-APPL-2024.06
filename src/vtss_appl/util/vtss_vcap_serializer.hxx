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

#ifndef __VTSS_VCAP_SERIALIZER_HXX__
#define __VTSS_VCAP_SERIALIZER_HXX__

#include <main_types.h>
#include <vtss/appl/vcap_types.h>
#include "vtss_appl_serialize.hxx"

/****************************************************************************
 * Shared serializers for VCAP enums
 ****************************************************************************/
extern const vtss_enum_descriptor_t vtss_appl_vcap_vlan_pri_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM_SHARED(vtss_appl_vcap_vlan_pri_type_t,
                         "VlanTagPriority",
                         vtss_appl_vcap_vlan_pri_type_txt,
                         "VLAN priority.");

extern const vtss_enum_descriptor_t vtss_appl_vcap_bit_txt[];
VTSS_XXXX_SERIALIZE_ENUM_SHARED(mesa_vcap_bit_t,
                         "BitType",
                         vtss_appl_vcap_bit_txt,
                         "Represents a bit selector.");

extern const vtss_enum_descriptor_t vtss_appl_vcap_as_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM_SHARED(vtss_appl_vcap_as_type_t,
                         "ASType",
                         vtss_appl_vcap_as_type_txt,
                         "Represents an Any/Specific selector.");

extern const vtss_enum_descriptor_t vtss_appl_vcap_asr_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM_SHARED(vtss_appl_vcap_asr_type_t,
                         "ASRType",
                         vtss_appl_vcap_asr_type_txt,
                         "Represents an Any/Specific/Range selector.");

extern const vtss_enum_descriptor_t vtss_appl_vcap_dmac_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM_SHARED(vtss_appl_vcap_dmac_type_t,
                         "DestMacType",
                         vtss_appl_vcap_dmac_type_txt,
                         "Represents a destination MAC type selector.");

extern const vtss_enum_descriptor_t vtss_appl_vcap_adv_dmac_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM_SHARED(vtss_appl_vcap_adv_dmac_type_t,
                         "AdvDestMacType",
                         vtss_appl_vcap_adv_dmac_type_txt,
                         "Represents a destination MAC type selector.");

extern const vtss_enum_descriptor_t vtss_appl_vcap_vlan_tag_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM_SHARED(vtss_appl_vcap_vlan_tag_type_t,
                         "VlanTagType",
                         vtss_appl_vcap_vlan_tag_type_txt,
                         "Represents a VLAN tag type selector.");

extern const vtss_enum_descriptor_t vtss_appl_vcap_key_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM_SHARED(mesa_vcap_key_type_t,
                         "VcapKeyType",
                         vtss_appl_vcap_key_type_txt,
                         "Represents a VCAP key type selector.");

#endif /* __VTSS_VCAP_SERIALIZER_HXX__ */
