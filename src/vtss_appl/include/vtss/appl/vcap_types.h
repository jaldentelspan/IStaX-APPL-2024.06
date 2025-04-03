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

/**
* \file vcap_types.h
 * \brief This file defines the VCAP types
*/

#ifndef _VCAP_TYPES_H_
#define _VCAP_TYPES_H_

#include <vtss/basics/enum-descriptor.h>    // vtss_enum_descriptor_t

/***************************************************************************
 - VCAP VLAN type
***************************************************************************/

/*! \brief VCAP VLAN tagged priority */
typedef enum {
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_ANY,  /*!< Match any */
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_0,    /*!< Match VLAN tagged priority value 0 only */
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_1,    /*!< Match VLAN tagged priority value 1 only */
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_2,    /*!< Match VLAN tagged priority value 2 only */
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_3,    /*!< Match VLAN tagged priority value 3 only */
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_4,    /*!< Match VLAN tagged priority value 4 only */
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_5,    /*!< Match VLAN tagged priority value 5 only */
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_6,    /*!< Match VLAN tagged priority value 6 only */
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_7,    /*!< Match VLAN tagged priority value 7 only */
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_01,   /*!< Match VLAN tagged priority range 0-1 only */
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_23,   /*!< Match VLAN tagged priority range 2-3 only */
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_45,   /*!< Match VLAN tagged priority range 4-5 only */
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_67,   /*!< Match VLAN tagged priority range 6-7 only */
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_03,   /*!< Match VLAN tagged priority range 0-3 only */
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_47,   /*!< Match VLAN tagged priority range 4-7 only */
    VTSS_APPL_VCAP_VLAN_PRI_TYPE_CNT   /*!< enum count */
} vtss_appl_vcap_vlan_pri_type_t;

/***************************************************************************
 - VCAP additional types.
   Some of these are probably candidates for being moved into the API.
***************************************************************************/

/** \brief VCAP 16 bit value and mask (uses integers instead of byte arrays */
typedef struct
{
    uint16_t value;   /**< Value */
    uint16_t mask;    /**< Mask, cleared bits are wildcards */
} vtss_appl_vcap_uint16_t;

/** \brief VCAP 32 bit value and mask (uses integers instead of byte arrays */
typedef struct
{
    uint32_t value;   /**< Value */
    uint32_t mask;    /**< Mask, cleared bits are wildcards */
} vtss_appl_vcap_uint32_t;

/** \brief VCAP MAC value and mask */
typedef struct
{
    mesa_mac_t value;   /**< Value */
    mesa_mac_t mask;    /**< Mask, cleared bits are wildcards */
} vtss_appl_vcap_mac_t;

/*! \brief VCAP IPv6 address value and mask */
typedef struct
{
    mesa_ipv6_t value;   /**< Value */
    mesa_ipv6_t mask;    /**< Mask, cleared bits are wildcards */
} vtss_appl_vcap_ipv6_t;


/***************************************************************************
 - VCAP universal types
***************************************************************************/

/*! \brief Any/Specific (AS) type */
typedef enum
{
    VTSS_APPL_VCAP_AS_TYPE_ANY,      /*!< Match any */
    VTSS_APPL_VCAP_AS_TYPE_SPECIFIC, /*!< Match specific value */
    VTSS_APPL_VCAP_AS_TYPE_CNT       /*!< Counting number of entries in enum */
} vtss_appl_vcap_as_type_t;

/*! \brief Any/Specific/Sange (ASR) type */
typedef enum
{
    VTSS_APPL_VCAP_ASR_TYPE_ANY,      /*!< Match any */
    VTSS_APPL_VCAP_ASR_TYPE_SPECIFIC, /*!< Match specific value in low */
    VTSS_APPL_VCAP_ASR_TYPE_RANGE,    /*!< Match range: low <= range <= high */
    VTSS_APPL_VCAP_ASR_TYPE_CNT       /*!< Counting number of entries in enum */
} vtss_appl_vcap_asr_type_t;

/*! \brief VCAP universal any/specific/range */
typedef struct
{
    vtss_appl_vcap_asr_type_t match; /*!< Match type */
    mesa_vcap_vr_value_t      low;   /*!< Low value */
    mesa_vcap_vr_value_t      high;  /*!< High value */
} vtss_appl_vcap_asr_t;

/*! \brief Range, minimum value encoded in 16 LSB, maximum value in 16 MSB */
typedef uint32_t vtss_appl_vcap_range_t;

/***************************************************************************
 - VCAP DMAC type
***************************************************************************/

/*! \brief VCAP DMAC match type */
typedef enum
{
    VTSS_APPL_VCAP_DMAC_TYPE_ANY,       /*!< Match any DMAC */
    VTSS_APPL_VCAP_DMAC_TYPE_UNICAST,   /*!< Match unicast DMAC only */
    VTSS_APPL_VCAP_DMAC_TYPE_MULTICAST, /*!< Match multicast DMAC only */
    VTSS_APPL_VCAP_DMAC_TYPE_BROADCAST, /*!< Match broadcast DMAC only */
    VTSS_APPL_VCAP_DMAC_TYPE_CNT        /*!< Counting number of entries in enum */
} vtss_appl_vcap_dmac_type_t;

/*! \brief VCAP advance DMAC match type */
typedef enum
{
    VTSS_APPL_VCAP_ADV_DMAC_TYPE_ANY,       /*!< Match any DMAC */
    VTSS_APPL_VCAP_ADV_DMAC_TYPE_UNICAST,   /*!< Match unicast DMAC only */
    VTSS_APPL_VCAP_ADV_DMAC_TYPE_MULTICAST, /*!< Match multicast DMAC only */
    VTSS_APPL_VCAP_ADV_DMAC_TYPE_BROADCAST, /*!< Match broadcast DMAC only */
    VTSS_APPL_VCAP_ADV_DMAC_TYPE_SPECIFIC,  /*!< Match specific DMAC only */
    VTSS_APPL_VCAP_ADV_DMAC_TYPE_CNT        /*!< Counting number of entries in enum */
} vtss_appl_vcap_adv_dmac_type_t;

/***************************************************************************
 - VCAP VLAN tag type
***************************************************************************/

/*! \brief VCAP vlan tag match type */
typedef enum
{
    VTSS_APPL_VCAP_VLAN_TAG_TYPE_ANY,      /*!< Match both untagged and tagged frames */
    VTSS_APPL_VCAP_VLAN_TAG_TYPE_UNTAGGED, /*!< Match untagged frames only */
    VTSS_APPL_VCAP_VLAN_TAG_TYPE_TAGGED,   /*!< Match tagged frames only */
    VTSS_APPL_VCAP_VLAN_TAG_TYPE_C_TAGGED, /*!< Match c-tagged frames only */
    VTSS_APPL_VCAP_VLAN_TAG_TYPE_S_TAGGED, /*!< Match s-tagged frames only */
    VTSS_APPL_VCAP_VLAN_TAG_TYPE_CNT       /*!< Counting number of entries in enum */
} vtss_appl_vcap_vlan_tag_type_t;

#endif  /* _VCAP_TYPES_H_ */

