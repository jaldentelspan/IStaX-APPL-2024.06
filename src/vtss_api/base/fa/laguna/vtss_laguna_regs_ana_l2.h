// Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
// SPDX-License-Identifier: MIT

#ifndef _VTSS_LAGUNA_REGS_ANA_L2_H_
#define _VTSS_LAGUNA_REGS_ANA_L2_H_

#include "vtss_laguna_regs_common.h"

/***********************************************************************
 *
 * Target: \a ANA_L2
 *
 * Layer 2 sub block of the Analyzer
 *
 ***********************************************************************/

/**
 * Register Group: \a ANA_L2:COMMON
 *
 * Common configurations for all ports
 */


/**
 * \brief Forwarding configuration
 *
 * \details
 * Configure common forwarding options.
 *
 * Upon reception of a frame the (DMAC, EFID) is used to perform a lookup
 * in the MAC table. If an entry is found the destination is known and
 * forwarded according to the entry ADDR_TYPE and ADDR. Alternatively the
 * destination is considered unknown and might be applicable for flooding.
 *
 * Register: \a ANA_L2:COMMON:FWD_CFG
 */
#define VTSS_ANA_L2_FWD_CFG                  VTSS_IOREG(VTSS_TO_ANA_L2,0x224c2)

/**
 * \brief
 * Configures MAC table Size
 *
 * \details
 * "00": No MAC tables
 * "01": 8K CMAC table
 * "10": 16K CMAC table
 * "11": 32K CMAC table
 *
 * Field: ::VTSS_ANA_L2_FWD_CFG . MAC_TBL_SPLIT_SEL
 */
#define  VTSS_F_ANA_L2_FWD_CFG_MAC_TBL_SPLIT_SEL(x)  VTSS_ENCODE_BITFIELD(x,20,2)
#define  VTSS_M_ANA_L2_FWD_CFG_MAC_TBL_SPLIT_SEL     VTSS_ENCODE_BITMASK(20,2)
#define  VTSS_X_ANA_L2_FWD_CFG_MAC_TBL_SPLIT_SEL(x)  VTSS_EXTRACT_BITFIELD(x,20,2)

/**
 * \brief
 * Enable use of a BDLB policer for per-port policing for frames with ISDX
 * == 0 or ANA_L2:ISDX:DLB_CFG.DLB_IDX == 0.If enabled, then BDLB policer
 * index is configured in PORT_DLB_CFG.PORT_DLB_IDX.Related
 * parameters:ANA_L2:COMMON:PORT_DLB_CFG.PORT_DLB_IDXANA_AC_POL:BDLB
 *
 * \details
 * 0: Disable. ANA_L2:ISDX[0]:MISC_CFG.BDLB_IDX used for all ports.
 * 1: Enable. ANA_L2::PORT_DLB_CFG[port_num].PORT_DLB_IDX used when
 * ISDX_VAL==0 or ANA_L2:ISDX:DLB_CFG.DLB_IDX == 0
 *
 * Field: ::VTSS_ANA_L2_FWD_CFG . PORT_DEFAULT_BDLB_ENA
 */
#define  VTSS_F_ANA_L2_FWD_CFG_PORT_DEFAULT_BDLB_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),18,1)
#define  VTSS_M_ANA_L2_FWD_CFG_PORT_DEFAULT_BDLB_ENA  VTSS_BIT(18)
#define  VTSS_X_ANA_L2_FWD_CFG_PORT_DEFAULT_BDLB_ENA(x)  VTSS_EXTRACT_BITFIELD(x,18,1)

/**
 * \brief
 * Enable use of an SDLB policer for frames with ISDX == 0 or
 * ANA_L2:ISDX:DLB_CFG.DLB_IDX == 0.If enabled, then SDLB policer index is
 * configured in PORT_DLB_CFG.QUEUE_DLB_IDX.Per queue DLB index configured
 * by means of ANA_L2::PORT_DLB_CFG.QUEUE_DLB_IDX.Related
 * parameters:PORT_DLB_CFG.QUEUE_DLB_IDXANA_AC_SDLB:LBSET_TBL
 *
 * \details
 * 0: Disable. ANA_L2:ISDX[0]:DLB_CFG.DLB_IDX used for all ISDX_VAL values.
 * 1: Enable. SDLB index = ANA_L2::PORT_DLB_CFG[port_num].QUEUE_DLB_IDX +
 * IPRIO used when ISDX_VAL==0 or ANA_L2:ISDX:DLB_CFG.DLB_IDX == 0
 *
 * Field: ::VTSS_ANA_L2_FWD_CFG . QUEUE_DEFAULT_SDLB_ENA
 */
#define  VTSS_F_ANA_L2_FWD_CFG_QUEUE_DEFAULT_SDLB_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),17,1)
#define  VTSS_M_ANA_L2_FWD_CFG_QUEUE_DEFAULT_SDLB_ENA  VTSS_BIT(17)
#define  VTSS_X_ANA_L2_FWD_CFG_QUEUE_DEFAULT_SDLB_ENA(x)  VTSS_EXTRACT_BITFIELD(x,17,1)

/**
 * \brief
 * Enables service forwarding based on lookup of ISDX in ISDX table.See
 * ANA_L2:ISDX
 *
 * \details
 * Field: ::VTSS_ANA_L2_FWD_CFG . ISDX_LOOKUP_ENA
 */
#define  VTSS_F_ANA_L2_FWD_CFG_ISDX_LOOKUP_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),16,1)
#define  VTSS_M_ANA_L2_FWD_CFG_ISDX_LOOKUP_ENA  VTSS_BIT(16)
#define  VTSS_X_ANA_L2_FWD_CFG_ISDX_LOOKUP_ENA(x)  VTSS_EXTRACT_BITFIELD(x,16,1)

/**
 * \brief
 * Configures the CPU queue for forwarded frames that have the CPU_COPY
 * flag set when enabled with:
 * ANA_L2::FWD_CFG.CPU_DMAC_COPY_ENA.Applicable frames are sent to this CPU
 * queue.
 *
 * \details
 * 0: CPU queue 0
 * 1: CPU queue 1
 * ...
 * n: CPU queue n.
 *
 * Field: ::VTSS_ANA_L2_FWD_CFG . CPU_DMAC_QU
 */
#define  VTSS_F_ANA_L2_FWD_CFG_CPU_DMAC_QU(x)  VTSS_ENCODE_BITFIELD(x,8,3)
#define  VTSS_M_ANA_L2_FWD_CFG_CPU_DMAC_QU     VTSS_ENCODE_BITMASK(8,3)
#define  VTSS_X_ANA_L2_FWD_CFG_CPU_DMAC_QU(x)  VTSS_EXTRACT_BITFIELD(x,8,3)

/**
 * \brief
 * Configures loopback for frames with DMAC = SMAC and bit 41 in the MAC
 * address set (= local administrated).
 *
 * \details
 * Field: ::VTSS_ANA_L2_FWD_CFG . LOOPBACK_ENA
 */
#define  VTSS_F_ANA_L2_FWD_CFG_LOOPBACK_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),7,1)
#define  VTSS_M_ANA_L2_FWD_CFG_LOOPBACK_ENA   VTSS_BIT(7)
#define  VTSS_X_ANA_L2_FWD_CFG_LOOPBACK_ENA(x)  VTSS_EXTRACT_BITFIELD(x,7,1)

/**
 * \brief
 * Configures CPU copy of frames where (DMAC, EFID) is known in the MAC
 * table and the entry has the CPU_COPY bit set.Frames are sent to the CPU
 * queue configured in ANA_L2::FWD_CFG.CPU_DMAC_QU.
 *
 * \details
 * Field: ::VTSS_ANA_L2_FWD_CFG . CPU_DMAC_COPY_ENA
 */
#define  VTSS_F_ANA_L2_FWD_CFG_CPU_DMAC_COPY_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),6,1)
#define  VTSS_M_ANA_L2_FWD_CFG_CPU_DMAC_COPY_ENA  VTSS_BIT(6)
#define  VTSS_X_ANA_L2_FWD_CFG_CPU_DMAC_COPY_ENA(x)  VTSS_EXTRACT_BITFIELD(x,6,1)

/**
 * \brief
 * Configures use of LRN::MAC_ACCESS_CFG_2.MAC_ENTRY_VLAN_IGNORE FLAG from
 * the MAC table entries as VLAN_IGNORE or as SRC_MASK_IGNORE.This also
 * configures the use of ANA_L2::FWD_CFG.FLOOD_IGNORE_VLAN_ENA when frames
 * are flooded.
 *
 * \details
 * 0: Use as VLAN ignore which means that the VLAN mask from
 * ANA_L3:VLAN:VLAN_MASK_CFG is not applied when forwarding by means of
 * entry with VLAN_IGNORE set.
 * 1: Use as Source mask ignore which means that the SOURCE mask from
 * ANA_AC:SRC:SRC_CFG is not applied when forwarding (frames can be send
 * back to the sending port)
 *
 * Field: ::VTSS_ANA_L2_FWD_CFG . FILTER_MODE_SEL
 */
#define  VTSS_F_ANA_L2_FWD_CFG_FILTER_MODE_SEL(x)  VTSS_ENCODE_BITFIELD(!!(x),4,1)
#define  VTSS_M_ANA_L2_FWD_CFG_FILTER_MODE_SEL  VTSS_BIT(4)
#define  VTSS_X_ANA_L2_FWD_CFG_FILTER_MODE_SEL(x)  VTSS_EXTRACT_BITFIELD(x,4,1)

/**
 * \brief
 * Configures mirroring operation for flooded frames. See
 * ANA_AC:MIRROR_PROBE:PROBE_CFG.PROBE_MAC_MODE.
 *
 * \details
 * Field: ::VTSS_ANA_L2_FWD_CFG . FLOOD_MIRROR_ENA
 */
#define  VTSS_F_ANA_L2_FWD_CFG_FLOOD_MIRROR_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),3,1)
#define  VTSS_M_ANA_L2_FWD_CFG_FLOOD_MIRROR_ENA  VTSS_BIT(3)
#define  VTSS_X_ANA_L2_FWD_CFG_FLOOD_MIRROR_ENA(x)  VTSS_EXTRACT_BITFIELD(x,3,1)

/**
 * \brief
 * Configures VLAN_IGNORE operation for flooded frames. See also
 * ANA_L2::FWD_CFG.FILTER_MODE_SEL.
 *
 * \details
 * Field: ::VTSS_ANA_L2_FWD_CFG . FLOOD_IGNORE_VLAN_ENA
 */
#define  VTSS_F_ANA_L2_FWD_CFG_FLOOD_IGNORE_VLAN_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),2,1)
#define  VTSS_M_ANA_L2_FWD_CFG_FLOOD_IGNORE_VLAN_ENA  VTSS_BIT(2)
#define  VTSS_X_ANA_L2_FWD_CFG_FLOOD_IGNORE_VLAN_ENA(x)  VTSS_EXTRACT_BITFIELD(x,2,1)

/**
 * \brief
 * Configures CPU copy operation for flooded frames.Frames are sent to the
 * CPU queue configured in ANA_L2::FWD_CFG.CPU_DMAC_QU.
 *
 * \details
 * Field: ::VTSS_ANA_L2_FWD_CFG . FLOOD_CPU_COPY_ENA
 */
#define  VTSS_F_ANA_L2_FWD_CFG_FLOOD_CPU_COPY_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),1,1)
#define  VTSS_M_ANA_L2_FWD_CFG_FLOOD_CPU_COPY_ENA  VTSS_BIT(1)
#define  VTSS_X_ANA_L2_FWD_CFG_FLOOD_CPU_COPY_ENA(x)  VTSS_EXTRACT_BITFIELD(x,1,1)

/**
 * \brief
 * Configures use of MAC table for forwarding.
 *
 * \details
 * 0: Disable all MAC fwd related lookups. All frames are flooded.
 * 1: Enable all MAC fwd related lookups
 *
 * Field: ::VTSS_ANA_L2_FWD_CFG . FWD_ENA
 */
#define  VTSS_F_ANA_L2_FWD_CFG_FWD_ENA(x)     VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_ANA_L2_FWD_CFG_FWD_ENA        VTSS_BIT(0)
#define  VTSS_X_ANA_L2_FWD_CFG_FWD_ENA(x)     VTSS_EXTRACT_BITFIELD(x,0,1)


/**
 * \brief Learn configuration
 *
 * \details
 * Configures learn handling. Upon reception of a frame the (SMAC, IFID) is
 * used to perform a lookup in the MAC table. If an entry is found the
 * station sending the frame is considered known and the SOURCE entry is
 * retrieved from the MAC table - alternatively the station is considered
 * unknown.
 *
 * A comparison of the received port information and the port information
 * known is performed for known stations to detection port move.
 *
 * A frame with changed or unknown station information is considered a
 * learn frame.
 *
 * Register: \a ANA_L2:COMMON:LRN_CFG
 */
#define VTSS_ANA_L2_LRN_CFG                  VTSS_IOREG(VTSS_TO_ANA_L2,0x224c3)

/**
 * \brief
 * Enable the VLAN_SEC_FWD_ENA configuration to redirect frames to CPU
 * instead of discarding.If SEC_REDIR_ENA is set,
 * ANA_L3:VLAN:VLAN_CFG.VLAN_SEC_FWD_ENA enables secure forwarding with
 * CPU-redirection on a per VLAN basis. Frames with unknown SMAC are
 * redirected to the CPU using CPU queue
 * ANA_L2:COMMON:LRN_CFG.CPU_LRN_QU.See:ANA_L3:VLAN:VLAN_CFG.VLAN_SEC_FWD_E
 * NA
 *
 * \details
 * 0: No redirection
 * 1: Redirection to CPU
 *
 * Field: ::VTSS_ANA_L2_LRN_CFG . SEC_REDIR_ENA
 */
#define  VTSS_F_ANA_L2_LRN_CFG_SEC_REDIR_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),26,1)
#define  VTSS_M_ANA_L2_LRN_CFG_SEC_REDIR_ENA  VTSS_BIT(26)
#define  VTSS_X_ANA_L2_LRN_CFG_SEC_REDIR_ENA(x)  VTSS_EXTRACT_BITFIELD(x,26,1)

/**
 * \brief
 * Setting VSTAX_LRN_SKIP_ENA=1 causes the lrn_mode field in the VStaX
 * header to be set to 1 if the auto learning is disabled for the
 * frame.I.e. if auto learning in the local unit is not allowed for the
 * frame, other units in the stack will not do auto learning of the
 * forwarded frame either.Auto learning can be disabled due
 * toANA_L2::AUTO_LRN_CFGANA_L3:MSTP:MSTP_LRN_CFGANA_L3::PORT_LRN_CTRLANA_L
 * 3:VLAN:VLAN_CFG.VLAN_LRN_DIS
 *
 * \details
 * Field: ::VTSS_ANA_L2_LRN_CFG . VSTAX_LRN_SKIP_ENA
 */
#define  VTSS_F_ANA_L2_LRN_CFG_VSTAX_LRN_SKIP_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),25,1)
#define  VTSS_M_ANA_L2_LRN_CFG_VSTAX_LRN_SKIP_ENA  VTSS_BIT(25)
#define  VTSS_X_ANA_L2_LRN_CFG_VSTAX_LRN_SKIP_ENA(x)  VTSS_EXTRACT_BITFIELD(x,25,1)

/**
 * \brief
 * Configures entries learned on a multicast index to not participate in
 * learning.
 *
 * \details
 * Field: ::VTSS_ANA_L2_LRN_CFG . IGNORE_MCIDX_PORTMOVE_ENA
 */
#define  VTSS_F_ANA_L2_LRN_CFG_IGNORE_MCIDX_PORTMOVE_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),24,1)
#define  VTSS_M_ANA_L2_LRN_CFG_IGNORE_MCIDX_PORTMOVE_ENA  VTSS_BIT(24)
#define  VTSS_X_ANA_L2_LRN_CFG_IGNORE_MCIDX_PORTMOVE_ENA(x)  VTSS_EXTRACT_BITFIELD(x,24,1)

/**
 * \brief
 * Configures the CPU queue for frames copied to CPU due to FID LEARN limit
 * check. Frames are sent to this CPU queue.
 *
 * \details
 * 0: CPU queue 0
 * 1: CPU queue 1
 * ...
 * n: CPU queue n.
 *
 * Field: ::VTSS_ANA_L2_LRN_CFG . CPU_FID_LIMIT_QU
 */
#define  VTSS_F_ANA_L2_LRN_CFG_CPU_FID_LIMIT_QU(x)  VTSS_ENCODE_BITFIELD(x,20,3)
#define  VTSS_M_ANA_L2_LRN_CFG_CPU_FID_LIMIT_QU     VTSS_ENCODE_BITMASK(20,3)
#define  VTSS_X_ANA_L2_LRN_CFG_CPU_FID_LIMIT_QU(x)  VTSS_EXTRACT_BITFIELD(x,20,3)

/**
 * \brief
 * Configures the CPU queue for frames copied to CPU due to ISDX LEARN
 * limit check. Frames are sent to this CPU queue.
 *
 * \details
 * 0: CPU queue 0
 * 1: CPU queue 1
 * ...
 * n: CPU queue n.
 *
 * Field: ::VTSS_ANA_L2_LRN_CFG . CPU_ISDX_LIMIT_QU
 */
#define  VTSS_F_ANA_L2_LRN_CFG_CPU_ISDX_LIMIT_QU(x)  VTSS_ENCODE_BITFIELD(x,7,3)
#define  VTSS_M_ANA_L2_LRN_CFG_CPU_ISDX_LIMIT_QU     VTSS_ENCODE_BITMASK(7,3)
#define  VTSS_X_ANA_L2_LRN_CFG_CPU_ISDX_LIMIT_QU(x)  VTSS_EXTRACT_BITFIELD(x,7,3)

/**
 * \brief
 * Configures if aged entries are chosen before nonaged when randomly
 * selecting a non locket entry to replace
 *
 * \details
 * 0: age flags are not considered when selecting an entry
 * 1: age flags are considered when selecting an entry.
 *
 * Field: ::VTSS_ANA_L2_LRN_CFG . AGED_BEFORE_NONAGED
 */
#define  VTSS_F_ANA_L2_LRN_CFG_AGED_BEFORE_NONAGED(x)  VTSS_ENCODE_BITFIELD(!!(x),18,1)
#define  VTSS_M_ANA_L2_LRN_CFG_AGED_BEFORE_NONAGED  VTSS_BIT(18)
#define  VTSS_X_ANA_L2_LRN_CFG_AGED_BEFORE_NONAGED(x)  VTSS_EXTRACT_BITFIELD(x,18,1)

/**
 * \brief
 * Configures the AGE_FLAG size
 *
 * \details
 * 0: one bit ageing periods used
 * 1: two bit ageing periods used.
 *
 * Field: ::VTSS_ANA_L2_LRN_CFG . AGE_SIZE
 */
#define  VTSS_F_ANA_L2_LRN_CFG_AGE_SIZE(x)    VTSS_ENCODE_BITFIELD(!!(x),17,1)
#define  VTSS_M_ANA_L2_LRN_CFG_AGE_SIZE       VTSS_BIT(17)
#define  VTSS_X_ANA_L2_LRN_CFG_AGE_SIZE(x)    VTSS_EXTRACT_BITFIELD(x,17,1)

/**
 * \brief
 * Configures use Basic or advanced VStaX2 learning mode.
 *
 * \details
 * 0: Entries are learned on UPSID and UPSPN ports (VSTAX Advanced
 * forwarding mode)
 * 1: Entries are learned on stacking ports (VSTAX basic forwarding mode).
 *
 * Field: ::VTSS_ANA_L2_LRN_CFG . VSTAX_BASIC_LRN_MODE_ENA
 */
#define  VTSS_F_ANA_L2_LRN_CFG_VSTAX_BASIC_LRN_MODE_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),16,1)
#define  VTSS_M_ANA_L2_LRN_CFG_VSTAX_BASIC_LRN_MODE_ENA  VTSS_BIT(16)
#define  VTSS_X_ANA_L2_LRN_CFG_VSTAX_BASIC_LRN_MODE_ENA(x)  VTSS_EXTRACT_BITFIELD(x,16,1)

/**
 * \brief
 * Configures entries in CAM row to be moved back to MAC table when empty
 * entries exists
 *
 * \details
 * 0: Known CAM entries are not moved back to MAC table
 * 1: Known CAM entries are moved to MAC table when empty entries exists on
 * a hashed row.
 *
 * Field: ::VTSS_ANA_L2_LRN_CFG . LRN_MOVE_CAM_ENTRY_BACK
 */
#define  VTSS_F_ANA_L2_LRN_CFG_LRN_MOVE_CAM_ENTRY_BACK(x)  VTSS_ENCODE_BITFIELD(!!(x),15,1)
#define  VTSS_M_ANA_L2_LRN_CFG_LRN_MOVE_CAM_ENTRY_BACK  VTSS_BIT(15)
#define  VTSS_X_ANA_L2_LRN_CFG_LRN_MOVE_CAM_ENTRY_BACK(x)  VTSS_EXTRACT_BITFIELD(x,15,1)

/**
 * \brief
 * Configures auto learning use of entries in the CAM. See
 * ANA_L2::AUTO_LRN_CFG
 *
 * \details
 * 0: Auto learn is limited to CMAC table
 * 1: Auto learn may also use MAC CAM entries.
 *
 * Field: ::VTSS_ANA_L2_LRN_CFG . AUTO_LRN_USE_MAC_CAM_ENA
 */
#define  VTSS_F_ANA_L2_LRN_CFG_AUTO_LRN_USE_MAC_CAM_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),14,1)
#define  VTSS_M_ANA_L2_LRN_CFG_AUTO_LRN_USE_MAC_CAM_ENA  VTSS_BIT(14)
#define  VTSS_X_ANA_L2_LRN_CFG_AUTO_LRN_USE_MAC_CAM_ENA(x)  VTSS_EXTRACT_BITFIELD(x,14,1)

/**
 * \brief
 * Configures how already known MAC table entries are replace of in case of
 * hash depletion when receiving traffic from an unknown station.
 *
 * \details
 * 0: Non locked entries are replaced
 * 1: Known entries are never replaced.
 *
 * Field: ::VTSS_ANA_L2_LRN_CFG . AUTO_LRN_REPLACE_DIS
 */
#define  VTSS_F_ANA_L2_LRN_CFG_AUTO_LRN_REPLACE_DIS(x)  VTSS_ENCODE_BITFIELD(!!(x),13,1)
#define  VTSS_M_ANA_L2_LRN_CFG_AUTO_LRN_REPLACE_DIS  VTSS_BIT(13)
#define  VTSS_X_ANA_L2_LRN_CFG_AUTO_LRN_REPLACE_DIS(x)  VTSS_EXTRACT_BITFIELD(x,13,1)

/**
 * \brief
 * Configures use of entries in the CAM row for CPU learning.
 *
 * \details
 * 0: CPU learn is limited to MAC table
 * 1: CPU learn may also use MAC CAM entries.
 *
 * Field: ::VTSS_ANA_L2_LRN_CFG . CPU_LRN_USE_MAC_CAM_ENA
 */
#define  VTSS_F_ANA_L2_LRN_CFG_CPU_LRN_USE_MAC_CAM_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),12,1)
#define  VTSS_M_ANA_L2_LRN_CFG_CPU_LRN_USE_MAC_CAM_ENA  VTSS_BIT(12)
#define  VTSS_X_ANA_L2_LRN_CFG_CPU_LRN_USE_MAC_CAM_ENA(x)  VTSS_EXTRACT_BITFIELD(x,12,1)

/**
 * \brief
 * Configures the CPU queue for CPU based learning when enabled by means of
 * ANA_L2::LRN_COPY_CFG. Frames are sent to this CPU queue.
 *
 * \details
 * 0: CPU queue 0
 * 1: CPU queue 1
 * ...
 * n: CPU queue n.
 *
 * Field: ::VTSS_ANA_L2_LRN_CFG . CPU_LRN_QU
 */
#define  VTSS_F_ANA_L2_LRN_CFG_CPU_LRN_QU(x)  VTSS_ENCODE_BITFIELD(x,4,3)
#define  VTSS_M_ANA_L2_LRN_CFG_CPU_LRN_QU     VTSS_ENCODE_BITMASK(4,3)
#define  VTSS_X_ANA_L2_LRN_CFG_CPU_LRN_QU(x)  VTSS_EXTRACT_BITFIELD(x,4,3)

/**
 * \brief
 * Configures clearing of the age bit in the source entry
 * (LRN::MAC_ACCESS_CFG_2.MAC_ENTRY_AGE_FLAG) when receiving traffic from
 * known stations which has the  LOCKED bit
 * (LRN::MAC_ACCESS_CFG_2.MAC_ENTRY_LOCKED) in the source entry set.
 *
 * \details
 * 0: LOCKED entries are not touched.
 * 1: The AGE flag for LOCKED entries will be cleared.
 *
 * Field: ::VTSS_ANA_L2_LRN_CFG . AGE_LOCKED_ENA
 */
#define  VTSS_F_ANA_L2_LRN_CFG_AGE_LOCKED_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),3,1)
#define  VTSS_M_ANA_L2_LRN_CFG_AGE_LOCKED_ENA  VTSS_BIT(3)
#define  VTSS_X_ANA_L2_LRN_CFG_AGE_LOCKED_ENA(x)  VTSS_EXTRACT_BITFIELD(x,3,1)

/**
 * \brief
 * Configures copy of frames from known stations with changed port
 * (LRN::MAC_ACCESS_CFG_2.MAC_ENTRY_ADDR_TYPE and
 * LRN::MAC_ACCESS_CFG_2.MAC_ENTRY_ADDR) in the source entry and the LOCKED
 * bit (LRN::MAC_ACCESS_CFG_2.MAC_ENTRY_LOCKED) set.Copies are sent to the
 * CPU queue specified by ANA_L2::LRN_CFG.CPU_LRN_QU.
 *
 * \details
 * Field: ::VTSS_ANA_L2_LRN_CFG . LOCKED_PORTMOVE_COPY_ENA
 */
#define  VTSS_F_ANA_L2_LRN_CFG_LOCKED_PORTMOVE_COPY_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),2,1)
#define  VTSS_M_ANA_L2_LRN_CFG_LOCKED_PORTMOVE_COPY_ENA  VTSS_BIT(2)
#define  VTSS_X_ANA_L2_LRN_CFG_LOCKED_PORTMOVE_COPY_ENA(x)  VTSS_EXTRACT_BITFIELD(x,2,1)

/**
 * \brief
 * Configures detection of frames from known stations with changed port
 * (LRN::MAC_ACCESS_CFG_2.MAC_ENTRY_ADDR_TYPE and
 * LRN::MAC_ACCESS_CFG_2.MAC_ENTRY_ADDR) in the source entry and the LOCKED
 * bit (LRN::MAC_ACCESS_CFG_2.MAC_ENTRY_LOCKED) set.When enabled, frames
 * from static (LOCKED) entries with changed port triggers
 * ANA_L2::MOVELOG_STICKY.
 *
 * \details
 * Field: ::VTSS_ANA_L2_LRN_CFG . LOCKED_PORTMOVE_DETECT_ENA
 */
#define  VTSS_F_ANA_L2_LRN_CFG_LOCKED_PORTMOVE_DETECT_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),1,1)
#define  VTSS_M_ANA_L2_LRN_CFG_LOCKED_PORTMOVE_DETECT_ENA  VTSS_BIT(1)
#define  VTSS_X_ANA_L2_LRN_CFG_LOCKED_PORTMOVE_DETECT_ENA(x)  VTSS_EXTRACT_BITFIELD(x,1,1)

/**
 * \brief
 * Configures CPU copy of frames from known stations with the CPU_COPY bit
 * set in the SOURCE entry. Copies are sent to the CPU queue specified in
 * MAC table by means of LRN::MAC_ACCESS_CFG_2.MAC_ENTRY_CPU_QU.
 *
 * \details
 * Field: ::VTSS_ANA_L2_LRN_CFG . CPU_SMAC_COPY_ENA
 */
#define  VTSS_F_ANA_L2_LRN_CFG_CPU_SMAC_COPY_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_ANA_L2_LRN_CFG_CPU_SMAC_COPY_ENA  VTSS_BIT(0)
#define  VTSS_X_ANA_L2_LRN_CFG_CPU_SMAC_COPY_ENA(x)  VTSS_EXTRACT_BITFIELD(x,0,1)


/**
 * \brief Remote scan filter configuration
 *
 * \details
 * Register: \a ANA_L2:COMMON:FILTER_OTHER_CTRL
 */
#define VTSS_ANA_L2_FILTER_OTHER_CTRL        VTSS_IOREG(VTSS_TO_ANA_L2,0x224c4)

/**
 * \brief
 * Configures additional filtering of Remote entries (i.e. entries with
 * UPSID != ANA_L2::OWN_UPSID.OWN_UPSID) for automatic ageing and CPU
 * scan.See LRN::SCAN_NEXT_CFG.SCAN_USE_PORT_FILTER_ENA and
 * LRN::AUTOAGE_CFG_1.USE_PORT_FILTER_ENA.
 *
 * \details
 * 0: handle remote entries during automatic ageing and CPU scan
 * 1: ignore remote entries during Automatic ageing and CPU scan

 *
 * Field: ::VTSS_ANA_L2_FILTER_OTHER_CTRL . FILTER_REMOTE_ENA
 */
#define  VTSS_F_ANA_L2_FILTER_OTHER_CTRL_FILTER_REMOTE_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_ANA_L2_FILTER_OTHER_CTRL_FILTER_REMOTE_ENA  VTSS_BIT(0)
#define  VTSS_X_ANA_L2_FILTER_OTHER_CTRL_FILTER_REMOTE_ENA(x)  VTSS_EXTRACT_BITFIELD(x,0,1)


/**
 * \brief Front Port scan filter configuration
 *
 * \details
 * Register: \a ANA_L2:COMMON:FILTER_LOCAL_CTRL
 */
#define VTSS_ANA_L2_FILTER_LOCAL_CTRL        VTSS_IOREG(VTSS_TO_ANA_L2,0x224c5)

/**
 * \brief
 * Configures additional front port SCAN filter for Automatic ageing and
 * CPU scan.See LRN::SCAN_NEXT_CFG.SCAN_USE_PORT_FILTER_ENA and
 * LRN::AUTOAGE_CFG_1.USE_PORT_FILTER_ENA.
 *
 * \details
 * x0x: Handle entries associated with the front port during automatic
 * ageing and CPU scan
 * x1x: Ignore entries associated with the front port during automatic
 * ageing and CPU scan

 *
 * Field: ::VTSS_ANA_L2_FILTER_LOCAL_CTRL . FILTER_FRONTPORT_ENA
 */
#define  VTSS_F_ANA_L2_FILTER_LOCAL_CTRL_FILTER_FRONTPORT_ENA(x)  VTSS_ENCODE_BITFIELD(x,0,30)
#define  VTSS_M_ANA_L2_FILTER_LOCAL_CTRL_FILTER_FRONTPORT_ENA     VTSS_ENCODE_BITMASK(0,30)
#define  VTSS_X_ANA_L2_FILTER_LOCAL_CTRL_FILTER_FRONTPORT_ENA(x)  VTSS_EXTRACT_BITFIELD(x,0,30)


/**
 * \brief Auto learn configuration
 *
 * \details
 * Configures automatic learning per port
 *
 * Register: \a ANA_L2:COMMON:AUTO_LRN_CFG
 */
#define VTSS_ANA_L2_AUTO_LRN_CFG             VTSS_IOREG(VTSS_TO_ANA_L2,0x224c8)

/**
 * \brief
 * Configures per port automatic learning of incoming learn frames received
 * on this port. Forwarding to CPU is still allowed. This does not
 * influence the port move detection in ANA_L2::MOVELOG_STICKY.
 *
 * \details
 * x0x: Disable automatic learning for the port
 * x1x: Enable automatic learning of incoming learn frames received on this
 * port
 *
 * Field: ::VTSS_ANA_L2_AUTO_LRN_CFG . AUTO_LRN_ENA
 */
#define  VTSS_F_ANA_L2_AUTO_LRN_CFG_AUTO_LRN_ENA(x)  VTSS_ENCODE_BITFIELD(x,0,30)
#define  VTSS_M_ANA_L2_AUTO_LRN_CFG_AUTO_LRN_ENA     VTSS_ENCODE_BITMASK(0,30)
#define  VTSS_X_ANA_L2_AUTO_LRN_CFG_AUTO_LRN_ENA(x)  VTSS_EXTRACT_BITFIELD(x,0,30)


/**
 * \brief Secure learn configuration
 *
 * \details
 * Configures secure forwarding per port
 *
 * Register: \a ANA_L2:COMMON:LRN_SECUR_CFG
 */
#define VTSS_ANA_L2_LRN_SECUR_CFG            VTSS_IOREG(VTSS_TO_ANA_L2,0x224cb)

/**
 * \brief
 * Configures per port discard of incoming learn frames received on this
 * port. Forwarding to CPU is still allowed. This does not influence the
 * port move detection in ANA_L2::MOVELOG_STICKY.
 *
 * \details
 * x0x: Disable drop of incoming learn frames for the port
 * x1x: Enable drop of incoming learn frames received on this port
 *
 * Field: ::VTSS_ANA_L2_LRN_SECUR_CFG . DROP_UNKNOWN_SRC_ENA
 */
#define  VTSS_F_ANA_L2_LRN_SECUR_CFG_DROP_UNKNOWN_SRC_ENA(x)  VTSS_ENCODE_BITFIELD(x,0,30)
#define  VTSS_M_ANA_L2_LRN_SECUR_CFG_DROP_UNKNOWN_SRC_ENA     VTSS_ENCODE_BITMASK(0,30)
#define  VTSS_X_ANA_L2_LRN_SECUR_CFG_DROP_UNKNOWN_SRC_ENA(x)  VTSS_EXTRACT_BITFIELD(x,0,30)


/**
 * \brief Secure learn configuration for locked entries
 *
 * \details
 * Configures secure forwarding for static (LOCKED) entries per port
 *
 * Register: \a ANA_L2:COMMON:LRN_SECUR_LOCKED_CFG
 */
#define VTSS_ANA_L2_LRN_SECUR_LOCKED_CFG     VTSS_IOREG(VTSS_TO_ANA_L2,0x224ce)

/**
 * \brief
 * Configures per port discard of incoming frames triggering a port move
 * for a locked entry in the MAC table received on this port. Forwarding to
 * CPU is still allowed. This does not influence the port move detection in
 * ANA_L2::MOVELOG_STICKY.
 *
 * \details
 * x0x: Disable
 * x1x: Enable drop of incoming frames triggering a port move for a locked
 * entry in the MAC table received on this port
 *
 * Field: ::VTSS_ANA_L2_LRN_SECUR_LOCKED_CFG . DROP_MOVED_LOCKED_ENA
 */
#define  VTSS_F_ANA_L2_LRN_SECUR_LOCKED_CFG_DROP_MOVED_LOCKED_ENA(x)  VTSS_ENCODE_BITFIELD(x,0,30)
#define  VTSS_M_ANA_L2_LRN_SECUR_LOCKED_CFG_DROP_MOVED_LOCKED_ENA     VTSS_ENCODE_BITMASK(0,30)
#define  VTSS_X_ANA_L2_LRN_SECUR_LOCKED_CFG_DROP_MOVED_LOCKED_ENA(x)  VTSS_EXTRACT_BITFIELD(x,0,30)


/**
 * \brief Learn copy configuration
 *
 * \details
 * Configures CPU copy of learn frames per port
 *
 * Register: \a ANA_L2:COMMON:LRN_COPY_CFG
 */
#define VTSS_ANA_L2_LRN_COPY_CFG             VTSS_IOREG(VTSS_TO_ANA_L2,0x224d1)

/**
 * \brief
 * Configures per port copy of incoming learn frames received on this port.
 * Copies are sent to the CPU queue specified by
 * ANA_L2::LRN_CFG.CPU_LRN_QU.This does not influence the port move
 * detection in ANA_L2::MOVELOG_STICKY.
 *
 * \details
 * x0x: Disable
 * x1x: incoming learn frames received on this port are copied to the CPU.
 *
 * Field: ::VTSS_ANA_L2_LRN_COPY_CFG . CPU_LRN_COPY_ENA
 */
#define  VTSS_F_ANA_L2_LRN_COPY_CFG_CPU_LRN_COPY_ENA(x)  VTSS_ENCODE_BITFIELD(x,0,30)
#define  VTSS_M_ANA_L2_LRN_COPY_CFG_CPU_LRN_COPY_ENA     VTSS_ENCODE_BITMASK(0,30)
#define  VTSS_X_ANA_L2_LRN_COPY_CFG_CPU_LRN_COPY_ENA(x)  VTSS_EXTRACT_BITFIELD(x,0,30)


/**
 * \brief Service DLB Port default configuration
 *
 * \details
 * Configures per port dual leaky bucket indexes
 *
 * Register: \a ANA_L2:COMMON:PORT_DLB_CFG
 *
 * @param ri Replicator: x_FFL_CHIP_NUM_ETH_PORTS (??), 0-29
 */
#define VTSS_ANA_L2_PORT_DLB_CFG(ri)         VTSS_IOREG(VTSS_TO_ANA_L2,0x224d4 + (ri))

/**
 * \brief
 * Specifies the base address used for indexing queue SDLBs for frames with
 * ISDX=0 if QUEUE_DEFAULT_SDLB_ENA=1.To calculated the address into SDLB
 * table, IPRIO is added to the base address.Related
 * parameters:ANA_L2:COMMON:FWD_CFG.QUEUE_DEFAULT_SDLB_ENAANA_AC_SDLB:LBSET
 * _TBL
 *
 * \details
 * Field: ::VTSS_ANA_L2_PORT_DLB_CFG . QUEUE_DLB_IDX
 */
#define  VTSS_F_ANA_L2_PORT_DLB_CFG_QUEUE_DLB_IDX(x)  VTSS_ENCODE_BITFIELD(x,10,9)
#define  VTSS_M_ANA_L2_PORT_DLB_CFG_QUEUE_DLB_IDX     VTSS_ENCODE_BITMASK(10,9)
#define  VTSS_X_ANA_L2_PORT_DLB_CFG_QUEUE_DLB_IDX(x)  VTSS_EXTRACT_BITFIELD(x,10,9)

/**
 * \brief
 * Specifies the default port BDLB index to be used for frames with ISDX=0
 * if PORT_DEFAULT_BDLB_ENA=1 is enabled.Related
 * parameters:ANA_L2::FWD_CFG.PORT_DEFAULT_BDLB_ENAANA_AC_POL:BDLB
 *
 * \details
 * Field: ::VTSS_ANA_L2_PORT_DLB_CFG . PORT_DLB_IDX
 */
#define  VTSS_F_ANA_L2_PORT_DLB_CFG_PORT_DLB_IDX(x)  VTSS_ENCODE_BITFIELD(x,0,7)
#define  VTSS_M_ANA_L2_PORT_DLB_CFG_PORT_DLB_IDX     VTSS_ENCODE_BITMASK(0,7)
#define  VTSS_X_ANA_L2_PORT_DLB_CFG_PORT_DLB_IDX(x)  VTSS_EXTRACT_BITFIELD(x,0,7)


/**
 * \brief ISDX learn limit Port default configuration
 *
 * \details
 * Configures per port ISDX learn limit indexes
 *
 * Register: \a ANA_L2:COMMON:PORT_ISDX_LIMIT_CFG
 *
 * @param ri Replicator: x_FFL_CHIP_NUM_ETH_PORTS (??), 0-29
 */
#define VTSS_ANA_L2_PORT_ISDX_LIMIT_CFG(ri)  VTSS_IOREG(VTSS_TO_ANA_L2,0x22515 + (ri))

/**
 * \brief
 * Specifies the default port ISDX learn limit index to be used for frames
 * with ISDX=0 or with ISDX>0 and ANA_L2::ISDX_LIMIT_CFG.ISDX_LIMIT_IDX=0.
 *
 * \details
 * Field: ::VTSS_ANA_L2_PORT_ISDX_LIMIT_CFG . PORT_ISDX_LIMIT_IDX
 */
#define  VTSS_F_ANA_L2_PORT_ISDX_LIMIT_CFG_PORT_ISDX_LIMIT_IDX(x)  VTSS_ENCODE_BITFIELD(x,0,8)
#define  VTSS_M_ANA_L2_PORT_ISDX_LIMIT_CFG_PORT_ISDX_LIMIT_IDX     VTSS_ENCODE_BITMASK(0,8)
#define  VTSS_X_ANA_L2_PORT_ISDX_LIMIT_CFG_PORT_ISDX_LIMIT_IDX(x)  VTSS_EXTRACT_BITFIELD(x,0,8)


/**
 * \brief Configures if additional VID/FID are used when doing scan
 *
 * \details
 * This is to allow faster ring protection update
 *
 * Register: \a ANA_L2:COMMON:SCAN_FID_CTRL
 */
#define VTSS_ANA_L2_SCAN_FID_CTRL            VTSS_IOREG(VTSS_TO_ANA_L2,0x22556)

/**
 * \brief
 * Controls use of additional FID filters used during scan when
 * LRN:COMMON:SCAN_NEXT_CFG.FID_FILTER_ENA is set.
 *
 * \details
 * 0: FID/VID filters only depend on
 * LRN:COMMON:SCAN_NEXT_CFG.FID_FILTER_ENA.
 * 1: Additional FID filter values are specified in
 * ANA_L2:COMMON:SCAN_FID_CFG.SCAN_FID_VAL if
 * LRN:COMMON:SCAN_NEXT_CFG.FID_FILTER_ENA is set.
 *
 * Field: ::VTSS_ANA_L2_SCAN_FID_CTRL . SCAN_FID_ENA
 */
#define  VTSS_F_ANA_L2_SCAN_FID_CTRL_SCAN_FID_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_ANA_L2_SCAN_FID_CTRL_SCAN_FID_ENA  VTSS_BIT(0)
#define  VTSS_X_ANA_L2_SCAN_FID_CTRL_SCAN_FID_ENA(x)  VTSS_EXTRACT_BITFIELD(x,0,1)


/**
 * \brief Configures a number of additional VID/FID to be used when doing scan
 *
 * \details
 * This is to allow faster ring protection update
 *
 * Register: \a ANA_L2:COMMON:SCAN_FID_CFG
 *
 * @param ri Replicator: x_FFL_ANA_NUM_SCAN_FID (??), 0-15
 */
#define VTSS_ANA_L2_SCAN_FID_CFG(ri)         VTSS_IOREG(VTSS_TO_ANA_L2,0x22557 + (ri))

/**
 * \brief
 * Configures additional FID/VID filters doing scan if
 * LRN:COMMON:SCAN_NEXT_CFG.FID_FILTER_ENA and
 * ANA_L2:COMMON:SCAN_FID_CTRL.SCAN_FID_ENA is set.
 *
 * \details
 * 0x1fff: Disable FID/VID filter
 *
 * Field: ::VTSS_ANA_L2_SCAN_FID_CFG . SCAN_FID_VAL
 */
#define  VTSS_F_ANA_L2_SCAN_FID_CFG_SCAN_FID_VAL(x)  VTSS_ENCODE_BITFIELD(x,0,13)
#define  VTSS_M_ANA_L2_SCAN_FID_CFG_SCAN_FID_VAL     VTSS_ENCODE_BITMASK(0,13)
#define  VTSS_X_ANA_L2_SCAN_FID_CFG_SCAN_FID_VAL(x)  VTSS_EXTRACT_BITFIELD(x,0,13)


/**
 * \brief Sticky Portmove status
 *
 * \details
 * Identifies ports with moved stations
 *
 * Register: \a ANA_L2:COMMON:MOVELOG_STICKY
 */
#define VTSS_ANA_L2_MOVELOG_STICKY           VTSS_IOREG(VTSS_TO_ANA_L2,0x22567)

/**
 * \brief
 * When a known station identified by (IFID, SMAC) pair with port
 * information (LRN::MAC_ACCESS_CFG_2.MAC_ENTRY_ADDR_TYPE and
 * LRN::MAC_ACCESS_CFG_2.MAC_ENTRY_ADDR) is seen with changed port
 * information, the new port is set in this mask. Write 1 to clear the
 * corresponding bit.Notice: If operation VSTAX AF mode
 * (ANA_L2::LRN_CFG.VSTAX_BASIC_LRN_MODE_ENA)  port move operates a little
 * different since different types of port moves are supported
 * see:ANA_L2::STICKY.LOCAL_TO_REMOTE_PORTMOVE_STICKYANA_L2::STICKY.REMOTE_
 * TO_LOCAL_PORTMOVE_STICKYANA_L2::STICKY.REMOTE_TO_REMOTE_PORTMOVE_STICKYA
 * NA_L2::STICKY.GLOBAL_TO_GLOBAL_PORTMOVE_STICKYANA_L2::STICKY.GLOBAL_TO_L
 * OCAL_PORTMOVE_STICKYANA_L2::STICKY.LOCAL_TO_GLOBAL_PORTMOVE_STICKYANA_L2
 * ::STICKY.LOCAL_TO_LOCAL_PORTMOVE_STICKY
 *
 * \details
 * 'XX...XXX': If port X is set, at least one station has moved to logical
 * port X.
 *
 * Field: ::VTSS_ANA_L2_MOVELOG_STICKY . PORTMOVE_LOG_STICKY
 */
#define  VTSS_F_ANA_L2_MOVELOG_STICKY_PORTMOVE_LOG_STICKY(x)  VTSS_ENCODE_BITFIELD(x,0,30)
#define  VTSS_M_ANA_L2_MOVELOG_STICKY_PORTMOVE_LOG_STICKY     VTSS_ENCODE_BITMASK(0,30)
#define  VTSS_X_ANA_L2_MOVELOG_STICKY_PORTMOVE_LOG_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,0,30)


/**
 * \brief UPSID(s) for local ports
 *
 * \details
 * First replication covers port number 0-31
 * Second replication covers port numbers 32-63 (if applicable)
 * etc.
 *
 * If not all OWN_UPSID replications are required in a given port
 * configuration, then any unused replications must be set to the value of
 * one of the replications in use.
 *
 * Register: \a ANA_L2:COMMON:OWN_UPSID
 */
#define VTSS_ANA_L2_OWN_UPSID                VTSS_IOREG(VTSS_TO_ANA_L2,0x2256a)

/**
 * \brief
 * Configures own UPSID to be used for stacking.This must be configured
 * consistently across the following
 * registers:ANA_CL:COMMON:OWN_UPSIDANA_ACL:COMMON:OWN_UPSIDANA_L2:COMMON:O
 * WN_UPSIDANA_AC:PS_COMMON:OWN_UPSIDREW:COMMON:OWN_UPSID
 *
 * \details
 * Field: ::VTSS_ANA_L2_OWN_UPSID . OWN_UPSID
 */
#define  VTSS_F_ANA_L2_OWN_UPSID_OWN_UPSID(x)  VTSS_ENCODE_BITFIELD(x,0,5)
#define  VTSS_M_ANA_L2_OWN_UPSID_OWN_UPSID     VTSS_ENCODE_BITMASK(0,5)
#define  VTSS_X_ANA_L2_OWN_UPSID_OWN_UPSID(x)  VTSS_EXTRACT_BITFIELD(x,0,5)


/**
 * \brief VSTAX configuration
 *
 * \details
 * Configures stacking learning and forwarding operation
 *
 * Register: \a ANA_L2:COMMON:VSTAX_CTRL
 */
#define VTSS_ANA_L2_VSTAX_CTRL               VTSS_IOREG(VTSS_TO_ANA_L2,0x2256d)

/**
 * \brief
 * Enable setting nxt_lrn_all for frames route modified in Analyzer that
 * ought to trigger lrn_all.
 *
 * \details
 * Field: ::VTSS_ANA_L2_VSTAX_CTRL . VSTAX2_NXT_LRN_ALL_RT_ACL_ENA
 */
#define  VTSS_F_ANA_L2_VSTAX_CTRL_VSTAX2_NXT_LRN_ALL_RT_ACL_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),25,1)
#define  VTSS_M_ANA_L2_VSTAX_CTRL_VSTAX2_NXT_LRN_ALL_RT_ACL_ENA  VTSS_BIT(25)
#define  VTSS_X_ANA_L2_VSTAX_CTRL_VSTAX2_NXT_LRN_ALL_RT_ACL_ENA(x)  VTSS_EXTRACT_BITFIELD(x,25,1)

/**
 * \brief
 * Enable copy to local CPU for frames route modified in Analyzer that
 * ought to trigger lrn_all.Copies are sent to the CPU queue specified by
 * ANA_L2::VSTAX_CTRL.CPU_VSTAX_QU.
 *
 * \details
 * Field: ::VTSS_ANA_L2_VSTAX_CTRL . VSTAX2_LCPU_CP_ACL_RT_ENA
 */
#define  VTSS_F_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_ACL_RT_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),24,1)
#define  VTSS_M_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_ACL_RT_ENA  VTSS_BIT(24)
#define  VTSS_X_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_ACL_RT_ENA(x)  VTSS_EXTRACT_BITFIELD(x,24,1)

/**
 * \brief
 * Configures CPU queue for VSTAX2 triggered events (Configurable using
 * ANA_L2::VSTAX_CTRL).
 *
 * \details
 * 0: CPU queue 0
 * 1: CPU queue 1
 * ...
 * n: CPU queue n.
 *
 * Field: ::VTSS_ANA_L2_VSTAX_CTRL . CPU_VSTAX_QU
 */
#define  VTSS_F_ANA_L2_VSTAX_CTRL_CPU_VSTAX_QU(x)  VTSS_ENCODE_BITFIELD(x,12,3)
#define  VTSS_M_ANA_L2_VSTAX_CTRL_CPU_VSTAX_QU     VTSS_ENCODE_BITMASK(12,3)
#define  VTSS_X_ANA_L2_VSTAX_CTRL_CPU_VSTAX_QU(x)  VTSS_EXTRACT_BITFIELD(x,12,3)

/**
 * \brief
 * Enable copy to local CPU for frames received with different destination
 * in VStaX2 header than specified in local MAC table or if DMAC unknown is
 * received for a frame known in MAC table.Copies are sent to the CPU queue
 * specified by ANA_L2::VSTAX_CTRL.CPU_VSTAX_QU.
 *
 * \details
 * Field: ::VTSS_ANA_L2_VSTAX_CTRL . VSTAX2_LCPU_CP_UNEXP_DST_PORT_ENA
 */
#define  VTSS_F_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_UNEXP_DST_PORT_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),11,1)
#define  VTSS_M_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_UNEXP_DST_PORT_ENA  VTSS_BIT(11)
#define  VTSS_X_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_UNEXP_DST_PORT_ENA(x)  VTSS_EXTRACT_BITFIELD(x,11,1)

/**
 * \brief
 * Enable copy to local CPU for frames received on a front port with SMAC
 * entry nxt_lrn_all set.Copies are sent to the CPU queue specified by
 * ANA_L2::VSTAX_CTRL.CPU_VSTAX_QU.
 *
 * \details
 * Field: ::VTSS_ANA_L2_VSTAX_CTRL . VSTAX2_LCPU_CP_NXT_LRN_ALL_FRONT_ENA
 */
#define  VTSS_F_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_NXT_LRN_ALL_FRONT_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),10,1)
#define  VTSS_M_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_NXT_LRN_ALL_FRONT_ENA  VTSS_BIT(10)
#define  VTSS_X_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_NXT_LRN_ALL_FRONT_ENA(x)  VTSS_EXTRACT_BITFIELD(x,10,1)

/**
 * \brief
 * Enable copy to local CPU for frames received on a stack port with SMAC
 * address which causes a refresh of the age bit. Copies are sent to the
 * CPU queue specified by ANA_L2::VSTAX_CTRL.CPU_VSTAX_QU.Note: No copy
 * will be generated if learning is disabled due to VLAN learn state etc.
 *
 * \details
 * Field: ::VTSS_ANA_L2_VSTAX_CTRL . VSTAX2_LCPU_CP_REFRESH_STACK_ENA
 */
#define  VTSS_F_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_REFRESH_STACK_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),9,1)
#define  VTSS_M_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_REFRESH_STACK_ENA  VTSS_BIT(9)
#define  VTSS_X_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_REFRESH_STACK_ENA(x)  VTSS_EXTRACT_BITFIELD(x,9,1)

/**
 * \brief
 * Enable copy to local CPU for frames received on a front port with SMAC
 * address which causes a refresh of the age bit. Copies are sent to the
 * CPU queue specified by ANA_L2::VSTAX_CTRL.CPU_VSTAX_QU.Note: No copy
 * will be generated if learning is disabled due to VLAN learn state etc.
 *
 * \details
 * Field: ::VTSS_ANA_L2_VSTAX_CTRL . VSTAX2_LCPU_CP_REFRESH_FRONT_ENA
 */
#define  VTSS_F_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_REFRESH_FRONT_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),8,1)
#define  VTSS_M_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_REFRESH_FRONT_ENA  VTSS_BIT(8)
#define  VTSS_X_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_REFRESH_FRONT_ENA(x)  VTSS_EXTRACT_BITFIELD(x,8,1)

/**
 * \brief
 * Enable copy to local CPU for frames received on a stack port with SMAC
 * address previous learned on another port. Copies are sent to the CPU
 * queue specified by ANA_L2::VSTAX_CTRL.CPU_VSTAX_QU.Note: No copy will be
 * generated if learning is disabled due to VLAN learn state etc.
 *
 * \details
 * Field: ::VTSS_ANA_L2_VSTAX_CTRL . VSTAX2_LCPU_CP_MOVE_STACK_ENA
 */
#define  VTSS_F_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_MOVE_STACK_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),7,1)
#define  VTSS_M_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_MOVE_STACK_ENA  VTSS_BIT(7)
#define  VTSS_X_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_MOVE_STACK_ENA(x)  VTSS_EXTRACT_BITFIELD(x,7,1)

/**
 * \brief
 * Enable copy to local CPU for frames received on a stack port with new
 * SMAC address. Copies are sent to the CPU queue specified by
 * ANA_L2::VSTAX_CTRL.CPU_VSTAX_QU.Note: No copy will be generated if
 * learning is disabled due to VLAN learn state etc.
 *
 * \details
 * Field: ::VTSS_ANA_L2_VSTAX_CTRL . VSTAX2_LCPU_CP_NEW_STACK_ENA
 */
#define  VTSS_F_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_NEW_STACK_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),6,1)
#define  VTSS_M_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_NEW_STACK_ENA  VTSS_BIT(6)
#define  VTSS_X_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_NEW_STACK_ENA(x)  VTSS_EXTRACT_BITFIELD(x,6,1)

/**
 * \brief
 * Enable copy to local CPU for frames received on a front port with SMAC
 * address previous learned on another port. Copies are sent to the CPU
 * queue specified by ANA_L2::VSTAX_CTRL.CPU_VSTAX_QU.Note: No copy will be
 * generated if learning is disabled due to VLAN learn state etc.
 *
 * \details
 * Field: ::VTSS_ANA_L2_VSTAX_CTRL . VSTAX2_LCPU_CP_MOVE_FRONT_ENA
 */
#define  VTSS_F_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_MOVE_FRONT_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),5,1)
#define  VTSS_M_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_MOVE_FRONT_ENA  VTSS_BIT(5)
#define  VTSS_X_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_MOVE_FRONT_ENA(x)  VTSS_EXTRACT_BITFIELD(x,5,1)

/**
 * \brief
 * Enable copy to local CPU for frames with new SMAC address received on a
 * front port. Copies are sent to the CPU queue specified by
 * ANA_L2::VSTAX_CTRL.CPU_VSTAX_QU.Note: No copy will be generated if
 * learning is disabled due to VLAN learn state etc.
 *
 * \details
 * Field: ::VTSS_ANA_L2_VSTAX_CTRL . VSTAX2_LCPU_CP_NEW_FRONT_ENA
 */
#define  VTSS_F_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_NEW_FRONT_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),4,1)
#define  VTSS_M_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_NEW_FRONT_ENA  VTSS_BIT(4)
#define  VTSS_X_ANA_L2_VSTAX_CTRL_VSTAX2_LCPU_CP_NEW_FRONT_ENA(x)  VTSS_EXTRACT_BITFIELD(x,4,1)

/**
 * \brief
 * Enable lrn-all generation triggered by unexpected flooding, where MAC
 * address is known on local port. Note: No copy will be generated if
 * learning is disabled due to VLAN learn state etc.
 *
 * \details
 * Field: ::VTSS_ANA_L2_VSTAX_CTRL . VSTAX2_LRN_ALL_UNEXP_FLOOD_ENA
 */
#define  VTSS_F_ANA_L2_VSTAX_CTRL_VSTAX2_LRN_ALL_UNEXP_FLOOD_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),3,1)
#define  VTSS_M_ANA_L2_VSTAX_CTRL_VSTAX2_LRN_ALL_UNEXP_FLOOD_ENA  VTSS_BIT(3)
#define  VTSS_X_ANA_L2_VSTAX_CTRL_VSTAX2_LRN_ALL_UNEXP_FLOOD_ENA(x)  VTSS_EXTRACT_BITFIELD(x,3,1)

/**
 * \brief
 * Enable lrn-all generation triggered by MAC entry refresh for MAC
 * addresses learned on local port. Note: No copy will be generated if
 * learning is disabled due to VLAN learn state etc.
 *
 * \details
 * Field: ::VTSS_ANA_L2_VSTAX_CTRL . VSTAX2_LRN_ALL_REFRESH_ENA
 */
#define  VTSS_F_ANA_L2_VSTAX_CTRL_VSTAX2_LRN_ALL_REFRESH_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),2,1)
#define  VTSS_M_ANA_L2_VSTAX_CTRL_VSTAX2_LRN_ALL_REFRESH_ENA  VTSS_BIT(2)
#define  VTSS_X_ANA_L2_VSTAX_CTRL_VSTAX2_LRN_ALL_REFRESH_ENA(x)  VTSS_EXTRACT_BITFIELD(x,2,1)

/**
 * \brief
 * Enable lrn-all generation triggered by MAC address moved to local port.
 * Note: No copy will be generated if learning is disabled due to VLAN
 * learn state etc.
 *
 * \details
 * Field: ::VTSS_ANA_L2_VSTAX_CTRL . VSTAX2_LRN_ALL_MOVE_ENA
 */
#define  VTSS_F_ANA_L2_VSTAX_CTRL_VSTAX2_LRN_ALL_MOVE_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),1,1)
#define  VTSS_M_ANA_L2_VSTAX_CTRL_VSTAX2_LRN_ALL_MOVE_ENA  VTSS_BIT(1)
#define  VTSS_X_ANA_L2_VSTAX_CTRL_VSTAX2_LRN_ALL_MOVE_ENA(x)  VTSS_EXTRACT_BITFIELD(x,1,1)

/**
 * \brief
 * Enable lrn-all generation triggered by learn of new MAC address. Note:
 * No copy will be generated if learning is disabled due to VLAN learn
 * state etc.
 *
 * \details
 * Field: ::VTSS_ANA_L2_VSTAX_CTRL . VSTAX2_LRN_ALL_NEW_ENA
 */
#define  VTSS_F_ANA_L2_VSTAX_CTRL_VSTAX2_LRN_ALL_NEW_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_ANA_L2_VSTAX_CTRL_VSTAX2_LRN_ALL_NEW_ENA  VTSS_BIT(0)
#define  VTSS_X_ANA_L2_VSTAX_CTRL_VSTAX2_LRN_ALL_NEW_ENA(x)  VTSS_EXTRACT_BITFIELD(x,0,1)


/**
 * \brief ANA_L2 Interrupt events
 *
 * \details
 * ANA_L2 Interrupt events if interrupt enable is set in ANA_L2::INTR_ENA
 * and the corresponding event is triggered in ANA_L2::INTR_IDENT
 *
 * Register: \a ANA_L2:COMMON:INTR
 */
#define VTSS_ANA_L2_INTR                     VTSS_IOREG(VTSS_TO_ANA_L2,0x2256e)

/**
 * \brief
 * Set if CHANGE2SW triggered an interrupt. To enable the interrupt, set
 * ANA_L2::INTR_ENA.CHANGE2SW_INTR_ENA
 *
 * \details
 * 0: No event
 * 1: Event
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_INTR . CHANGE2SW_INTR
 */
#define  VTSS_F_ANA_L2_INTR_CHANGE2SW_INTR(x)  VTSS_ENCODE_BITFIELD(!!(x),7,1)
#define  VTSS_M_ANA_L2_INTR_CHANGE2SW_INTR    VTSS_BIT(7)
#define  VTSS_X_ANA_L2_INTR_CHANGE2SW_INTR(x)  VTSS_EXTRACT_BITFIELD(x,7,1)

/**
 * \brief
 * Set if IP6PFX_VCAP triggered an interrupt. To enable the interrupt, set
 * ANA_L2::INTR_ENA.IP6PFX_VCAP_INTR_ENA
 *
 * \details
 * 0: No event
 * 1: Event
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_INTR . IP6PFX_VCAP_INTR
 */
#define  VTSS_F_ANA_L2_INTR_IP6PFX_VCAP_INTR(x)  VTSS_ENCODE_BITFIELD(!!(x),6,1)
#define  VTSS_M_ANA_L2_INTR_IP6PFX_VCAP_INTR  VTSS_BIT(6)
#define  VTSS_X_ANA_L2_INTR_IP6PFX_VCAP_INTR(x)  VTSS_EXTRACT_BITFIELD(x,6,1)

/**
 * \brief
 * Set if SUPER_VCAP triggered an interrupt. To enable the interrupt, set
 * ANA_L2::INTR_ENA.SUPER_VCAP_INTR_ENA
 *
 * \details
 * 0: No event
 * 1: Event
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_INTR . SUPER_VCAP_INTR
 */
#define  VTSS_F_ANA_L2_INTR_SUPER_VCAP_INTR(x)  VTSS_ENCODE_BITFIELD(!!(x),5,1)
#define  VTSS_M_ANA_L2_INTR_SUPER_VCAP_INTR   VTSS_BIT(5)
#define  VTSS_X_ANA_L2_INTR_SUPER_VCAP_INTR(x)  VTSS_EXTRACT_BITFIELD(x,5,1)

/**
 * \brief
 * Set if LRN CPU access complete triggered an interrupt. To enable the
 * interrupt, set ANA_L2::INTR_ENA.LRN_ACCESS_COMPLETE_INTR_ENA
 *
 * \details
 * 0: No event
 * 1: Event
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_INTR . LRN_ACCESS_COMPLETE_INTR
 */
#define  VTSS_F_ANA_L2_INTR_LRN_ACCESS_COMPLETE_INTR(x)  VTSS_ENCODE_BITFIELD(!!(x),4,1)
#define  VTSS_M_ANA_L2_INTR_LRN_ACCESS_COMPLETE_INTR  VTSS_BIT(4)
#define  VTSS_X_ANA_L2_INTR_LRN_ACCESS_COMPLETE_INTR(x)  VTSS_EXTRACT_BITFIELD(x,4,1)

/**
 * \brief
 * Set if VCAP_S2 triggered an interrupt. To enable this interrupt, set
 * ANA_L2::INTR_ENA.VCAP_S2_INTR_ENA
 *
 * \details
 * 0: No event
 * 1: Event
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_INTR . VCAP_S2_INTR
 */
#define  VTSS_F_ANA_L2_INTR_VCAP_S2_INTR(x)   VTSS_ENCODE_BITFIELD(!!(x),3,1)
#define  VTSS_M_ANA_L2_INTR_VCAP_S2_INTR      VTSS_BIT(3)
#define  VTSS_X_ANA_L2_INTR_VCAP_S2_INTR(x)   VTSS_EXTRACT_BITFIELD(x,3,1)

/**
 * \brief
 * Set if port learn limit triggered an interrupt. To enable this
 * interrupt, set ANA_L2::INTR_ENA.PORT_LRN_LIMIT_INTR_ENA
 *
 * \details
 * 0: No event
 * 1: Event
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_INTR . PORT_LRN_LIMIT_INTR
 */
#define  VTSS_F_ANA_L2_INTR_PORT_LRN_LIMIT_INTR(x)  VTSS_ENCODE_BITFIELD(!!(x),2,1)
#define  VTSS_M_ANA_L2_INTR_PORT_LRN_LIMIT_INTR  VTSS_BIT(2)
#define  VTSS_X_ANA_L2_INTR_PORT_LRN_LIMIT_INTR(x)  VTSS_EXTRACT_BITFIELD(x,2,1)

/**
 * \brief
 * Set if FID learn limit triggered an interrupt. To enable this interrupt,
 * set ANA_L2::INTR_ENA.FID_LIMIT_INTR_ENA
 *
 * \details
 * 0: No event
 * 1: Event
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_INTR . FID_LIMIT_INTR
 */
#define  VTSS_F_ANA_L2_INTR_FID_LIMIT_INTR(x)  VTSS_ENCODE_BITFIELD(!!(x),1,1)
#define  VTSS_M_ANA_L2_INTR_FID_LIMIT_INTR    VTSS_BIT(1)
#define  VTSS_X_ANA_L2_INTR_FID_LIMIT_INTR(x)  VTSS_EXTRACT_BITFIELD(x,1,1)

/**
 * \brief
 * Set if ISDX learn limit triggered an interrupt. To enable this
 * interrupt, set ANA_L2::INTR_ENA.ISDX_LIMIT_INTR_ENA
 *
 * \details
 * 0: No event
 * 1: Event
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_INTR . ISDX_LIMIT_INTR
 */
#define  VTSS_F_ANA_L2_INTR_ISDX_LIMIT_INTR(x)  VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_ANA_L2_INTR_ISDX_LIMIT_INTR   VTSS_BIT(0)
#define  VTSS_X_ANA_L2_INTR_ISDX_LIMIT_INTR(x)  VTSS_EXTRACT_BITFIELD(x,0,1)


/**
 * \brief ANA_L2 interrupt enable mask
 *
 * \details
 * Controls interrupt of CPU.
 *
 * Events can be found in ANA_L2::INTR
 * Identity of interrupt events can be found in ANA_L2::INTR_IDENT

 *
 * Register: \a ANA_L2:COMMON:INTR_ENA
 */
#define VTSS_ANA_L2_INTR_ENA                 VTSS_IOREG(VTSS_TO_ANA_L2,0x2256f)

/**
 * \brief
 * Configures CHANGE2SW_INTR interrupts
 *
 * \details
 * 0: Disable interrupt
 * 1: Enable interrupt
 *
 * Field: ::VTSS_ANA_L2_INTR_ENA . CHANGE2SW_INTR_ENA
 */
#define  VTSS_F_ANA_L2_INTR_ENA_CHANGE2SW_INTR_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),7,1)
#define  VTSS_M_ANA_L2_INTR_ENA_CHANGE2SW_INTR_ENA  VTSS_BIT(7)
#define  VTSS_X_ANA_L2_INTR_ENA_CHANGE2SW_INTR_ENA(x)  VTSS_EXTRACT_BITFIELD(x,7,1)

/**
 * \brief
 * Configures IP6PFX_VCAP_INTR interrupts
 *
 * \details
 * 0: Disable interrupt
 * 1: Enable interrupt
 *
 * Field: ::VTSS_ANA_L2_INTR_ENA . IP6PFX_VCAP_INTR_ENA
 */
#define  VTSS_F_ANA_L2_INTR_ENA_IP6PFX_VCAP_INTR_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),6,1)
#define  VTSS_M_ANA_L2_INTR_ENA_IP6PFX_VCAP_INTR_ENA  VTSS_BIT(6)
#define  VTSS_X_ANA_L2_INTR_ENA_IP6PFX_VCAP_INTR_ENA(x)  VTSS_EXTRACT_BITFIELD(x,6,1)

/**
 * \brief
 * Configures SUPER_VCAP_INTR interrupts
 *
 * \details
 * 0: Disable interrupt
 * 1: Enable interrupt
 *
 * Field: ::VTSS_ANA_L2_INTR_ENA . SUPER_VCAP_INTR_ENA
 */
#define  VTSS_F_ANA_L2_INTR_ENA_SUPER_VCAP_INTR_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),5,1)
#define  VTSS_M_ANA_L2_INTR_ENA_SUPER_VCAP_INTR_ENA  VTSS_BIT(5)
#define  VTSS_X_ANA_L2_INTR_ENA_SUPER_VCAP_INTR_ENA(x)  VTSS_EXTRACT_BITFIELD(x,5,1)

/**
 * \brief
 * Configures LRN_ACCESS_COMPLETE_INTR interrupts
 *
 * \details
 * 0: Disable interrupt
 * 1: Enable interrupt
 *
 * Field: ::VTSS_ANA_L2_INTR_ENA . LRN_ACCESS_COMPLETE_INTR_ENA
 */
#define  VTSS_F_ANA_L2_INTR_ENA_LRN_ACCESS_COMPLETE_INTR_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),4,1)
#define  VTSS_M_ANA_L2_INTR_ENA_LRN_ACCESS_COMPLETE_INTR_ENA  VTSS_BIT(4)
#define  VTSS_X_ANA_L2_INTR_ENA_LRN_ACCESS_COMPLETE_INTR_ENA(x)  VTSS_EXTRACT_BITFIELD(x,4,1)

/**
 * \brief
 * Configures VCAP_S2_INTR interrupts
 *
 * \details
 * 0: Disable interrupt
 * 1: Enable interrupt
 *
 * Field: ::VTSS_ANA_L2_INTR_ENA . VCAP_S2_INTR_ENA
 */
#define  VTSS_F_ANA_L2_INTR_ENA_VCAP_S2_INTR_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),3,1)
#define  VTSS_M_ANA_L2_INTR_ENA_VCAP_S2_INTR_ENA  VTSS_BIT(3)
#define  VTSS_X_ANA_L2_INTR_ENA_VCAP_S2_INTR_ENA(x)  VTSS_EXTRACT_BITFIELD(x,3,1)

/**
 * \brief
 * Configures PORT_LRN_LIMIT interrupts
 *
 * \details
 * 0: Disable interrupt
 * 1: Enable interrupt
 *
 * Field: ::VTSS_ANA_L2_INTR_ENA . PORT_LRN_LIMIT_INTR_ENA
 */
#define  VTSS_F_ANA_L2_INTR_ENA_PORT_LRN_LIMIT_INTR_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),2,1)
#define  VTSS_M_ANA_L2_INTR_ENA_PORT_LRN_LIMIT_INTR_ENA  VTSS_BIT(2)
#define  VTSS_X_ANA_L2_INTR_ENA_PORT_LRN_LIMIT_INTR_ENA(x)  VTSS_EXTRACT_BITFIELD(x,2,1)

/**
 * \brief
 * Configures FID_LIMIT_INTR interrupts
 *
 * \details
 * 0: Disable interrupt
 * 1: Enable interrupt
 *
 * Field: ::VTSS_ANA_L2_INTR_ENA . FID_LIMIT_INTR_ENA
 */
#define  VTSS_F_ANA_L2_INTR_ENA_FID_LIMIT_INTR_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),1,1)
#define  VTSS_M_ANA_L2_INTR_ENA_FID_LIMIT_INTR_ENA  VTSS_BIT(1)
#define  VTSS_X_ANA_L2_INTR_ENA_FID_LIMIT_INTR_ENA(x)  VTSS_EXTRACT_BITFIELD(x,1,1)

/**
 * \brief
 * Configures ISDX_LIMIT_INTR interrupts
 *
 * \details
 * 0: Disable interrupt
 * 1: Enable interrupt
 *
 * Field: ::VTSS_ANA_L2_INTR_ENA . ISDX_LIMIT_INTR_ENA
 */
#define  VTSS_F_ANA_L2_INTR_ENA_ISDX_LIMIT_INTR_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_ANA_L2_INTR_ENA_ISDX_LIMIT_INTR_ENA  VTSS_BIT(0)
#define  VTSS_X_ANA_L2_INTR_ENA_ISDX_LIMIT_INTR_ENA(x)  VTSS_EXTRACT_BITFIELD(x,0,1)


/**
 * \brief ANA_L2 interrupt status
 *
 * \details
 * Register: \a ANA_L2:COMMON:INTR_IDENT
 */
#define VTSS_ANA_L2_INTR_IDENT               VTSS_IOREG(VTSS_TO_ANA_L2,0x22570)

/**
 * \brief
 * Status of CHANGE2SW_INTR interrupts
 *
 * \details
 * 0: No interrupt
 * 1: Interrupt to CPU
 *
 * Field: ::VTSS_ANA_L2_INTR_IDENT . CHANGE2SW_INTR_IDENT
 */
#define  VTSS_F_ANA_L2_INTR_IDENT_CHANGE2SW_INTR_IDENT(x)  VTSS_ENCODE_BITFIELD(!!(x),7,1)
#define  VTSS_M_ANA_L2_INTR_IDENT_CHANGE2SW_INTR_IDENT  VTSS_BIT(7)
#define  VTSS_X_ANA_L2_INTR_IDENT_CHANGE2SW_INTR_IDENT(x)  VTSS_EXTRACT_BITFIELD(x,7,1)

/**
 * \brief
 * Status of IP6PFX_VCAP_INTR interrupts
 *
 * \details
 * 0: No interrupt
 * 1: Interrupt to CPU
 *
 * Field: ::VTSS_ANA_L2_INTR_IDENT . IP6PFX_VCAP_INTR_IDENT
 */
#define  VTSS_F_ANA_L2_INTR_IDENT_IP6PFX_VCAP_INTR_IDENT(x)  VTSS_ENCODE_BITFIELD(!!(x),6,1)
#define  VTSS_M_ANA_L2_INTR_IDENT_IP6PFX_VCAP_INTR_IDENT  VTSS_BIT(6)
#define  VTSS_X_ANA_L2_INTR_IDENT_IP6PFX_VCAP_INTR_IDENT(x)  VTSS_EXTRACT_BITFIELD(x,6,1)

/**
 * \brief
 * Status of SUPER_VCAP_INTR interrupts
 *
 * \details
 * 0: No interrupt
 * 1: Interrupt to CPU
 *
 * Field: ::VTSS_ANA_L2_INTR_IDENT . SUPER_VCAP_INTR_IDENT
 */
#define  VTSS_F_ANA_L2_INTR_IDENT_SUPER_VCAP_INTR_IDENT(x)  VTSS_ENCODE_BITFIELD(!!(x),5,1)
#define  VTSS_M_ANA_L2_INTR_IDENT_SUPER_VCAP_INTR_IDENT  VTSS_BIT(5)
#define  VTSS_X_ANA_L2_INTR_IDENT_SUPER_VCAP_INTR_IDENT(x)  VTSS_EXTRACT_BITFIELD(x,5,1)

/**
 * \brief
 * Status of LRN_ACCESS_COMPLETE_INTR interrupts
 *
 * \details
 * 0: No interrupt
 * 1: Interrupt to CPU
 *
 * Field: ::VTSS_ANA_L2_INTR_IDENT . LRN_ACCESS_COMPLETE_INTR_IDENT
 */
#define  VTSS_F_ANA_L2_INTR_IDENT_LRN_ACCESS_COMPLETE_INTR_IDENT(x)  VTSS_ENCODE_BITFIELD(!!(x),4,1)
#define  VTSS_M_ANA_L2_INTR_IDENT_LRN_ACCESS_COMPLETE_INTR_IDENT  VTSS_BIT(4)
#define  VTSS_X_ANA_L2_INTR_IDENT_LRN_ACCESS_COMPLETE_INTR_IDENT(x)  VTSS_EXTRACT_BITFIELD(x,4,1)

/**
 * \brief
 * Status of VCAP_S2_INTR interrupts
 *
 * \details
 * 0: No interrupt
 * 1: Interrupt to CPU
 *
 * Field: ::VTSS_ANA_L2_INTR_IDENT . VCAP_S2_INTR_IDENT
 */
#define  VTSS_F_ANA_L2_INTR_IDENT_VCAP_S2_INTR_IDENT(x)  VTSS_ENCODE_BITFIELD(!!(x),3,1)
#define  VTSS_M_ANA_L2_INTR_IDENT_VCAP_S2_INTR_IDENT  VTSS_BIT(3)
#define  VTSS_X_ANA_L2_INTR_IDENT_VCAP_S2_INTR_IDENT(x)  VTSS_EXTRACT_BITFIELD(x,3,1)

/**
 * \brief
 * Status of PORT_LRN_LIMIT_INTR interrupts
 *
 * \details
 * 0: No interrupt
 * 1: Interrupt to CPU
 *
 * Field: ::VTSS_ANA_L2_INTR_IDENT . PORT_LRN_LIMIT_INTR_IDENT
 */
#define  VTSS_F_ANA_L2_INTR_IDENT_PORT_LRN_LIMIT_INTR_IDENT(x)  VTSS_ENCODE_BITFIELD(!!(x),2,1)
#define  VTSS_M_ANA_L2_INTR_IDENT_PORT_LRN_LIMIT_INTR_IDENT  VTSS_BIT(2)
#define  VTSS_X_ANA_L2_INTR_IDENT_PORT_LRN_LIMIT_INTR_IDENT(x)  VTSS_EXTRACT_BITFIELD(x,2,1)

/**
 * \brief
 * Status of FID_LIMIT_INTR interrupts
 *
 * \details
 * 0: No interrupt
 * 1: Interrupt to CPU
 *
 * Field: ::VTSS_ANA_L2_INTR_IDENT . FID_LIMIT_INTR_IDENT
 */
#define  VTSS_F_ANA_L2_INTR_IDENT_FID_LIMIT_INTR_IDENT(x)  VTSS_ENCODE_BITFIELD(!!(x),1,1)
#define  VTSS_M_ANA_L2_INTR_IDENT_FID_LIMIT_INTR_IDENT  VTSS_BIT(1)
#define  VTSS_X_ANA_L2_INTR_IDENT_FID_LIMIT_INTR_IDENT(x)  VTSS_EXTRACT_BITFIELD(x,1,1)

/**
 * \brief
 * Status of ISDX_LIMIT_INTR interrupts
 *
 * \details
 * 0: No interrupt
 * 1: Interrupt to CPU
 *
 * Field: ::VTSS_ANA_L2_INTR_IDENT . ISDX_LIMIT_INTR_IDENT
 */
#define  VTSS_F_ANA_L2_INTR_IDENT_ISDX_LIMIT_INTR_IDENT(x)  VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_ANA_L2_INTR_IDENT_ISDX_LIMIT_INTR_IDENT  VTSS_BIT(0)
#define  VTSS_X_ANA_L2_INTR_IDENT_ISDX_LIMIT_INTR_IDENT(x)  VTSS_EXTRACT_BITFIELD(x,0,1)


/**
 * \brief Secure learn configuration
 *
 * \details
 * Configures secure forwarding per port
 *
 * Register: \a ANA_L2:COMMON:LRN_SECUR_LOCKED_COPY_CFG
 */
#define VTSS_ANA_L2_LRN_SECUR_LOCKED_COPY_CFG  VTSS_IOREG(VTSS_TO_ANA_L2,0x22571)

/**
 * \brief
 * Configures copy of frames from known stations with changed port
 * (LRN::MAC_ACCESS_CFG_2.MAC_ENTRY_ADDR_TYPE and
 * LRN::MAC_ACCESS_CFG_2.MAC_ENTRY_ADDR) in the source entry and the LOCKED
 * bit (LRN::MAC_ACCESS_CFG_2.MAC_ENTRY_LOCKED) set.Copies are sent to the
 * CPU queue specified by ANA_L2::LRN_CFG.CPU_LRN_QU.
 *
 * \details
 * x0x: Disable drop of incoming learn frames for the port
 * x1x: Enable drop of incoming learn frames received on this port
 *
 * Field: ::VTSS_ANA_L2_LRN_SECUR_LOCKED_COPY_CFG . COPY_MOVED_LOCKED_ENA
 */
#define  VTSS_F_ANA_L2_LRN_SECUR_LOCKED_COPY_CFG_COPY_MOVED_LOCKED_ENA(x)  VTSS_ENCODE_BITFIELD(x,0,30)
#define  VTSS_M_ANA_L2_LRN_SECUR_LOCKED_COPY_CFG_COPY_MOVED_LOCKED_ENA     VTSS_ENCODE_BITMASK(0,30)
#define  VTSS_X_ANA_L2_LRN_SECUR_LOCKED_COPY_CFG_COPY_MOVED_LOCKED_ENA(x)  VTSS_EXTRACT_BITFIELD(x,0,30)

/**
 * Register Group: \a ANA_L2:LRN_LIMIT
 *
 * Learn limits per FID
 */


/**
 * \brief Controls automatic learn limits
 *
 * \details
 * Per FID configuration of autolearn limits
 *
 * Register: \a ANA_L2:LRN_LIMIT:FID_LIMIT_STATUS
 *
 * @param gi Replicator: x_FFL_ANA_NUM_FIDS (??), 0-4607
 */
#define VTSS_ANA_L2_FID_LIMIT_STATUS(gi)     VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x20000,gi,2,0,0)

/**
 * \brief
 * Set if specified MAX learn cnt limit is exceeded and max learn cnt was
 * enabled. Write '1' to clear this field.
 *
 * \details
 * 0: Learn cnt not exceeded.
 * 1: Learning operation has failed due to FID max learn cnt exceeded.
 * Write '1' to clear this field.
 *
 * Field: ::VTSS_ANA_L2_FID_LIMIT_STATUS . FID_LRN_LIMIT_EXCEEDED_STICKY
 */
#define  VTSS_F_ANA_L2_FID_LIMIT_STATUS_FID_LRN_LIMIT_EXCEEDED_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),16,1)
#define  VTSS_M_ANA_L2_FID_LIMIT_STATUS_FID_LRN_LIMIT_EXCEEDED_STICKY  VTSS_BIT(16)
#define  VTSS_X_ANA_L2_FID_LIMIT_STATUS_FID_LRN_LIMIT_EXCEEDED_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,16,1)

/**
 * \brief
 * Contains the number of MAC table entries currently learned associated
 * with a given FID.
 *
 * \details
 * 0 : no entries
 *
 * Field: ::VTSS_ANA_L2_FID_LIMIT_STATUS . FID_LRN_CNT
 */
#define  VTSS_F_ANA_L2_FID_LIMIT_STATUS_FID_LRN_CNT(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_ANA_L2_FID_LIMIT_STATUS_FID_LRN_CNT     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_ANA_L2_FID_LIMIT_STATUS_FID_LRN_CNT(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/**
 * \brief Controls automatic learn limits per FID
 *
 * \details
 * Per FID configuration of MAC table learn limits
 *
 * Register: \a ANA_L2:LRN_LIMIT:FID_LIMIT_CTRL
 *
 * @param gi Replicator: x_FFL_ANA_NUM_FIDS (??), 0-4607
 */
#define VTSS_ANA_L2_FID_LIMIT_CTRL(gi)       VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x20000,gi,2,0,1)

/**
 * \brief
 * Allow setting FID_LIMIT_INTR when exceeding limit on learning (happens
 * when MAC address are supposed to be installed in the MAC table).
 *
 * \details
 * 0: Disable
 * 1: allow FID_LIMIT_INTR to be set upon trying to learn a MAC address
 * that causes learn limit to be exceeded
 *
 * Field: ::VTSS_ANA_L2_FID_LIMIT_CTRL . FID_LIMIT_EXCEED_IRQ_ENA
 */
#define  VTSS_F_ANA_L2_FID_LIMIT_CTRL_FID_LIMIT_EXCEED_IRQ_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),17,1)
#define  VTSS_M_ANA_L2_FID_LIMIT_CTRL_FID_LIMIT_EXCEED_IRQ_ENA  VTSS_BIT(17)
#define  VTSS_X_ANA_L2_FID_LIMIT_CTRL_FID_LIMIT_EXCEED_IRQ_ENA(x)  VTSS_EXTRACT_BITFIELD(x,17,1)

/**
 * \brief
 * Action for traffic when learn limit is exceeded.
 *
 * \details
 * 00: Normal forward
 * 01: Enable redirection to CPU queue
 * 10: Enable copy to CPU queue
 * 11: Discard the frame
 *
 * Field: ::VTSS_ANA_L2_FID_LIMIT_CTRL . FID_LIMIT_EXCEED_SEL
 */
#define  VTSS_F_ANA_L2_FID_LIMIT_CTRL_FID_LIMIT_EXCEED_SEL(x)  VTSS_ENCODE_BITFIELD(x,15,2)
#define  VTSS_M_ANA_L2_FID_LIMIT_CTRL_FID_LIMIT_EXCEED_SEL     VTSS_ENCODE_BITMASK(15,2)
#define  VTSS_X_ANA_L2_FID_LIMIT_CTRL_FID_LIMIT_EXCEED_SEL(x)  VTSS_EXTRACT_BITFIELD(x,15,2)

/**
 * \brief
 * Configures the number of MAC table entries that can be used for a given
 * FID (through Automatic learning and CPU based learning with LOCK bit
 * cleared and not multicast).
 *
 * \details
 * 0: Disable i.e. no learn limit for the FID
 * 1: Only learning of one MAC address allowed for this FID
 * ...
 * n: Learning of n MAC address allowed for this FID
 *
 * Field: ::VTSS_ANA_L2_FID_LIMIT_CTRL . FID_LRN_CNT_LIMIT
 */
#define  VTSS_F_ANA_L2_FID_LIMIT_CTRL_FID_LRN_CNT_LIMIT(x)  VTSS_ENCODE_BITFIELD(x,0,15)
#define  VTSS_M_ANA_L2_FID_LIMIT_CTRL_FID_LRN_CNT_LIMIT     VTSS_ENCODE_BITMASK(0,15)
#define  VTSS_X_ANA_L2_FID_LIMIT_CTRL_FID_LRN_CNT_LIMIT(x)  VTSS_EXTRACT_BITFIELD(x,0,15)

/**
 * Register Group: \a ANA_L2:ISDX_LIMIT
 *
 * Learn limits per set of ISDXs
 */


/**
 * \brief Controls automatic learn limits
 *
 * \details
 * Per set of ISDXs configuration of autolearn limits
 *
 * Register: \a ANA_L2:ISDX_LIMIT:ISDX_LIMIT_STATUS
 *
 * @param gi Replicator: x_FFL_ANA_NUM_ISDX_LIMITS (??), 0-255
 */
#define VTSS_ANA_L2_ISDX_LIMIT_STATUS(gi)    VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x23000,gi,2,0,0)

/**
 * \brief
 * Set if specified MAX learn cnt limit is exceeded and max learn cnt was
 * enabled. Write '1' to clear this field.
 *
 * \details
 * 0: Learn cnt not exceeded.
 * 1: Learning operation has failed due to ISDX max learn cnt exceeded.
 * Write '1' to clear this field.
 *
 * Field: ::VTSS_ANA_L2_ISDX_LIMIT_STATUS . ISDX_LRN_LIMIT_EXCEEDED_STICKY
 */
#define  VTSS_F_ANA_L2_ISDX_LIMIT_STATUS_ISDX_LRN_LIMIT_EXCEEDED_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),16,1)
#define  VTSS_M_ANA_L2_ISDX_LIMIT_STATUS_ISDX_LRN_LIMIT_EXCEEDED_STICKY  VTSS_BIT(16)
#define  VTSS_X_ANA_L2_ISDX_LIMIT_STATUS_ISDX_LRN_LIMIT_EXCEEDED_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,16,1)

/**
 * \brief
 * Contains the number of MAC table entries currently learned associated
 * with a given set of ISDXs.
 *
 * \details
 * 0 : no entries
 *
 * Field: ::VTSS_ANA_L2_ISDX_LIMIT_STATUS . ISDX_LRN_CNT
 */
#define  VTSS_F_ANA_L2_ISDX_LIMIT_STATUS_ISDX_LRN_CNT(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_ANA_L2_ISDX_LIMIT_STATUS_ISDX_LRN_CNT     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_ANA_L2_ISDX_LIMIT_STATUS_ISDX_LRN_CNT(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/**
 * \brief Controls automatic learn limits per set of ISDXs
 *
 * \details
 * Per set of ISDXs configuration of MAC table learn limits
 *
 * Register: \a ANA_L2:ISDX_LIMIT:ISDX_LIMIT_CTRL
 *
 * @param gi Replicator: x_FFL_ANA_NUM_ISDX_LIMITS (??), 0-255
 */
#define VTSS_ANA_L2_ISDX_LIMIT_CTRL(gi)      VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x23000,gi,2,0,1)

/**
 * \brief
 * Allow setting ISDX_LIMIT_INTR when exceeding limit on learning (happens
 * when MAC address are supposed to be installed in the MAC table).
 *
 * \details
 * 0: Disable
 * 1: allow ISDX_LIMIT_INTR to be set upon trying to learn a MAC address
 * that causes learn limit to be exceeded
 *
 * Field: ::VTSS_ANA_L2_ISDX_LIMIT_CTRL . ISDX_LIMIT_EXCEED_IRQ_ENA
 */
#define  VTSS_F_ANA_L2_ISDX_LIMIT_CTRL_ISDX_LIMIT_EXCEED_IRQ_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),17,1)
#define  VTSS_M_ANA_L2_ISDX_LIMIT_CTRL_ISDX_LIMIT_EXCEED_IRQ_ENA  VTSS_BIT(17)
#define  VTSS_X_ANA_L2_ISDX_LIMIT_CTRL_ISDX_LIMIT_EXCEED_IRQ_ENA(x)  VTSS_EXTRACT_BITFIELD(x,17,1)

/**
 * \brief
 * Action for traffic when learn limit is exceeded.
 *
 * \details
 * 00: Normal forward
 * 01: Enable redirection to CPU queue
 * 10: Enable copy to CPU queue
 * 11: Discard the frame
 *
 * Field: ::VTSS_ANA_L2_ISDX_LIMIT_CTRL . ISDX_LIMIT_EXCEED_SEL
 */
#define  VTSS_F_ANA_L2_ISDX_LIMIT_CTRL_ISDX_LIMIT_EXCEED_SEL(x)  VTSS_ENCODE_BITFIELD(x,15,2)
#define  VTSS_M_ANA_L2_ISDX_LIMIT_CTRL_ISDX_LIMIT_EXCEED_SEL     VTSS_ENCODE_BITMASK(15,2)
#define  VTSS_X_ANA_L2_ISDX_LIMIT_CTRL_ISDX_LIMIT_EXCEED_SEL(x)  VTSS_EXTRACT_BITFIELD(x,15,2)

/**
 * \brief
 * Configures the number of MAC table entries that can be used for a given
 * set of ISDXs (through Automatic learning and CPU based learning with
 * LOCK bit cleared and not multicast).
 *
 * \details
 * 0: Disable i.e. no learn limit for the set of ISDXs
 * 1: Only learning of one MAC address allowed for this set of ISDXs
 * ...
 * n: Learning of n MAC address allowed for this set of ISDXs
 *
 * Field: ::VTSS_ANA_L2_ISDX_LIMIT_CTRL . ISDX_LRN_CNT_LIMIT
 */
#define  VTSS_F_ANA_L2_ISDX_LIMIT_CTRL_ISDX_LRN_CNT_LIMIT(x)  VTSS_ENCODE_BITFIELD(x,0,15)
#define  VTSS_M_ANA_L2_ISDX_LIMIT_CTRL_ISDX_LRN_CNT_LIMIT     VTSS_ENCODE_BITMASK(0,15)
#define  VTSS_X_ANA_L2_ISDX_LIMIT_CTRL_ISDX_LRN_CNT_LIMIT(x)  VTSS_EXTRACT_BITFIELD(x,0,15)

/**
 * Register Group: \a ANA_L2:PORT_LIMIT
 *
 * Learn limits per PORT and GLAG
 */


/**
 * \brief Controls automatic learn limits
 *
 * \details
 * Per port configuration of autolearn limits
 *
 * Register: \a ANA_L2:PORT_LIMIT:PORT_LIMIT_STATUS
 *
 * @param gi Replicator: x_FFL_ANA_NUM_PORT_LIMITS (??), 0-61
 */
#define VTSS_ANA_L2_PORT_LIMIT_STATUS(gi)    VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x22400,gi,2,0,0)

/**
 * \brief
 * Set if specified MAX learn cnt limit is exceeded and max learn cnt was
 * enabled. Write '1' to clear this field.
 *
 * \details
 * 0: Learn cnt not exceeded.
 * 1: Learning operation has failed due to PORT max learn cnt exceeded.
 * Write '1' to clear this field.
 *
 * Field: ::VTSS_ANA_L2_PORT_LIMIT_STATUS . PORT_LRN_LIMIT_EXCEEDED_STICKY
 */
#define  VTSS_F_ANA_L2_PORT_LIMIT_STATUS_PORT_LRN_LIMIT_EXCEEDED_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),16,1)
#define  VTSS_M_ANA_L2_PORT_LIMIT_STATUS_PORT_LRN_LIMIT_EXCEEDED_STICKY  VTSS_BIT(16)
#define  VTSS_X_ANA_L2_PORT_LIMIT_STATUS_PORT_LRN_LIMIT_EXCEEDED_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,16,1)

/**
 * \brief
 * Contains the number of MAC table entries currently learned associated
 * with a given logical PORT or GLAG.
 *
 * \details
 * 0 : no entries
 *
 * Field: ::VTSS_ANA_L2_PORT_LIMIT_STATUS . PORT_LRN_CNT
 */
#define  VTSS_F_ANA_L2_PORT_LIMIT_STATUS_PORT_LRN_CNT(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_ANA_L2_PORT_LIMIT_STATUS_PORT_LRN_CNT     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_ANA_L2_PORT_LIMIT_STATUS_PORT_LRN_CNT(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/**
 * \brief Controls automatic learn limits per logical port or GLAG
 *
 * \details
 * Per Port configuration of MAC table learn limits
 *
 * Register: \a ANA_L2:PORT_LIMIT:PORT_LIMIT_CTRL
 *
 * @param gi Replicator: x_FFL_ANA_NUM_PORT_LIMITS (??), 0-61
 */
#define VTSS_ANA_L2_PORT_LIMIT_CTRL(gi)      VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x22400,gi,2,0,1)

/**
 * \brief
 * Allow setting PORT_LIMIT_INTR when exceeding limit on learning (happens
 * when MAC address are supposed to be installed in the MAC table.
 *
 * \details
 * 0: Disable
 * 1: allow PORT_LIMIT_INTR to be set upon trying to learn a MAC address
 * that causes learn limit to be exceeded
 *
 * Field: ::VTSS_ANA_L2_PORT_LIMIT_CTRL . PORT_LIMIT_EXCEED_IRQ_ENA
 */
#define  VTSS_F_ANA_L2_PORT_LIMIT_CTRL_PORT_LIMIT_EXCEED_IRQ_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),17,1)
#define  VTSS_M_ANA_L2_PORT_LIMIT_CTRL_PORT_LIMIT_EXCEED_IRQ_ENA  VTSS_BIT(17)
#define  VTSS_X_ANA_L2_PORT_LIMIT_CTRL_PORT_LIMIT_EXCEED_IRQ_ENA(x)  VTSS_EXTRACT_BITFIELD(x,17,1)

/**
 * \brief
 * Action for traffic when learn limit is exceeded.
 *
 * \details
 * 00: Normal forward
 * 01: Enable redirection to CPU queue
 * 10: Enable copy to CPU queue
 * 11: Discard the frame
 *
 * Field: ::VTSS_ANA_L2_PORT_LIMIT_CTRL . PORT_LIMIT_EXCEED_SEL
 */
#define  VTSS_F_ANA_L2_PORT_LIMIT_CTRL_PORT_LIMIT_EXCEED_SEL(x)  VTSS_ENCODE_BITFIELD(x,15,2)
#define  VTSS_M_ANA_L2_PORT_LIMIT_CTRL_PORT_LIMIT_EXCEED_SEL     VTSS_ENCODE_BITMASK(15,2)
#define  VTSS_X_ANA_L2_PORT_LIMIT_CTRL_PORT_LIMIT_EXCEED_SEL(x)  VTSS_EXTRACT_BITFIELD(x,15,2)

/**
 * \brief
 * Configures the number of MAC table entries that can be used for a given
 * PORT (through Automatic learning and CPU based learning with LOCK bit
 * cleared and not multicast).
 *
 * \details
 * 0: Disable i.e. no learn limit for the PORT
 * 1: Only learning of one MAC address allowed for this logical port
 * ...
 * n: Learning of n MAC address allowed for this port
 *
 * Field: ::VTSS_ANA_L2_PORT_LIMIT_CTRL . PORT_LRN_CNT_LIMIT
 */
#define  VTSS_F_ANA_L2_PORT_LIMIT_CTRL_PORT_LRN_CNT_LIMIT(x)  VTSS_ENCODE_BITFIELD(x,0,15)
#define  VTSS_M_ANA_L2_PORT_LIMIT_CTRL_PORT_LRN_CNT_LIMIT     VTSS_ENCODE_BITMASK(0,15)
#define  VTSS_X_ANA_L2_PORT_LIMIT_CTRL_PORT_LRN_CNT_LIMIT(x)  VTSS_EXTRACT_BITFIELD(x,0,15)

/**
 * Register Group: \a ANA_L2:ISDX
 *
 * Ingress service table configuration
 */


/**
 * \brief Ingress service forwarding configuration
 *
 * \details
 * Configures service based forwarding.
 *
 * Register: \a ANA_L2:ISDX:SERVICE_CTRL
 *
 * @param gi Replicator: x_FFL_CHIP_NUM_SDX (??), 0-1023
 */
#define VTSS_ANA_L2_SERVICE_CTRL(gi)         VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x0,gi,32,0,0)

/**
 * \brief
 * Remove the logical ingress port (LPORT) from the destination set. Note:
 * LPORT_REMOVE_ENA does not change the normal source mask filtering.
 * Normal source mask filtering can be disabled through SRC_MASK_DIS.
 *
 * \details
 * Field: ::VTSS_ANA_L2_SERVICE_CTRL . LPORT_REMOVE_ENA
 */
#define  VTSS_F_ANA_L2_SERVICE_CTRL_LPORT_REMOVE_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),26,1)
#define  VTSS_M_ANA_L2_SERVICE_CTRL_LPORT_REMOVE_ENA  VTSS_BIT(26)
#define  VTSS_X_ANA_L2_SERVICE_CTRL_LPORT_REMOVE_ENA(x)  VTSS_EXTRACT_BITFIELD(x,26,1)

/**
 * \brief
 * Configures replacement of VLAN PORTMASK with ANA_L2:ISDX:PORT_MASK_CFG
 *
 * \details
 * Field: ::VTSS_ANA_L2_SERVICE_CTRL . PORT_MASK_REPLACE_ENA
 */
#define  VTSS_F_ANA_L2_SERVICE_CTRL_PORT_MASK_REPLACE_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),25,1)
#define  VTSS_M_ANA_L2_SERVICE_CTRL_PORT_MASK_REPLACE_ENA  VTSS_BIT(25)
#define  VTSS_X_ANA_L2_SERVICE_CTRL_PORT_MASK_REPLACE_ENA(x)  VTSS_EXTRACT_BITFIELD(x,25,1)

/**
 * \brief
 * Aggregation code value.
 *
 * \details
 * If AGGR_REPLACE_ENA is set then the aggregation code is replaced with
 * this value.
 *
 * If AGGR_REPLACE_ENA is cleared then AGGR_VAL selects between the
 * following modes:
 * AGGR_VAL = 0: No changes to aggregation and CSD masks.
 * AGGR_VAL = 1: No aggregation mask is applied.
 * AGGR_VAL = 2: No CSD mask is applied.
 * AGGR_VAL = > 2: No aggregation and CSD masks are applied.
 *
 * Field: ::VTSS_ANA_L2_SERVICE_CTRL . AGGR_VAL
 */
#define  VTSS_F_ANA_L2_SERVICE_CTRL_AGGR_VAL(x)  VTSS_ENCODE_BITFIELD(x,21,4)
#define  VTSS_M_ANA_L2_SERVICE_CTRL_AGGR_VAL     VTSS_ENCODE_BITMASK(21,4)
#define  VTSS_X_ANA_L2_SERVICE_CTRL_AGGR_VAL(x)  VTSS_EXTRACT_BITFIELD(x,21,4)

/**
 * \brief
 * Avoids applying source mask
 *
 * \details
 * 0: Source port mask is applied.
 * 1: Source mask is ignored.
 *
 * Field: ::VTSS_ANA_L2_SERVICE_CTRL . SRC_MASK_DIS
 */
#define  VTSS_F_ANA_L2_SERVICE_CTRL_SRC_MASK_DIS(x)  VTSS_ENCODE_BITFIELD(!!(x),20,1)
#define  VTSS_M_ANA_L2_SERVICE_CTRL_SRC_MASK_DIS  VTSS_BIT(20)
#define  VTSS_X_ANA_L2_SERVICE_CTRL_SRC_MASK_DIS(x)  VTSS_EXTRACT_BITFIELD(x,20,1)

/**
 * \brief
 * Overrule aggregation code from ANA_CL.
 *
 * \details
 * 0: Use aggregation code calculated in classifier. See AGGR_VAL for mode
 * to disable aggregation.
 * 1: Use SERVICE_CTRL.AGGR_VAL as aggregation code.
 *
 * Field: ::VTSS_ANA_L2_SERVICE_CTRL . AGGR_REPLACE_ENA
 */
#define  VTSS_F_ANA_L2_SERVICE_CTRL_AGGR_REPLACE_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),19,1)
#define  VTSS_M_ANA_L2_SERVICE_CTRL_AGGR_REPLACE_ENA  VTSS_BIT(19)
#define  VTSS_X_ANA_L2_SERVICE_CTRL_AGGR_REPLACE_ENA(x)  VTSS_EXTRACT_BITFIELD(x,19,1)

/**
 * \brief
 * Select ES0 key type.
 *
 * \details
 * 0: Use VID key in ES0.
 * 1: Use ISDX key in ES0.
 *
 * Field: ::VTSS_ANA_L2_SERVICE_CTRL . ES0_ISDX_KEY_ENA
 */
#define  VTSS_F_ANA_L2_SERVICE_CTRL_ES0_ISDX_KEY_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),18,1)
#define  VTSS_M_ANA_L2_SERVICE_CTRL_ES0_ISDX_KEY_ENA  VTSS_BIT(18)
#define  VTSS_X_ANA_L2_SERVICE_CTRL_ES0_ISDX_KEY_ENA(x)  VTSS_EXTRACT_BITFIELD(x,18,1)

/**
 * \brief
 * Enable forwarding based on CDA
 *
 * \details
 * Field: ::VTSS_ANA_L2_SERVICE_CTRL . CDA_FWD_ENA
 */
#define  VTSS_F_ANA_L2_SERVICE_CTRL_CDA_FWD_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),17,1)
#define  VTSS_M_ANA_L2_SERVICE_CTRL_CDA_FWD_ENA  VTSS_BIT(17)
#define  VTSS_X_ANA_L2_SERVICE_CTRL_CDA_FWD_ENA(x)  VTSS_EXTRACT_BITFIELD(x,17,1)

/**
 * \brief
 * Controls service based forwarding. Note: Setting this bit disables use
 * of ISDX_BASED_SRC_ENA.
 *
 * \details
 * 0: DMAC based forwarding.
 * 1: SERVICE_CTRL.CDA_FWD_ENA=1: DMAC based forwarding
 *     SERVICE_CTRL.CDA_FWD_ENA=0: (forwarding not influenced by DMAC
 * lookup):
 *	   SERVICE_CTRL.FWD_TYPE = 0 (UPSID_PN):
 *	      Forward to port in SERVICE_CTRL.FWD_ADDR
 *	   SERVICE_CTRL.FWD_TYPE = 3 (MC_IDX):
 *	      Forward by means of MC_IDX specified in SERVICE_CTRL.FWD_ADDR
 *	   SERVICE_CTRL.FWD_TYPE = 7 (NO_ADDR):
 *	      Flood forward.

 *
 * Field: ::VTSS_ANA_L2_SERVICE_CTRL . ISDX_BASED_FWD_ENA
 */
#define  VTSS_F_ANA_L2_SERVICE_CTRL_ISDX_BASED_FWD_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),16,1)
#define  VTSS_M_ANA_L2_SERVICE_CTRL_ISDX_BASED_FWD_ENA  VTSS_BIT(16)
#define  VTSS_X_ANA_L2_SERVICE_CTRL_ISDX_BASED_FWD_ENA(x)  VTSS_EXTRACT_BITFIELD(x,16,1)

/**
 * \brief
 * Enable service based learning. When set traffic associated with a ISDX
 * are seen as received on the interface configured for the ISDX instead of
 * the port interface configured in ANA_CL:PORT:PORT_ID_CFG.Note: This
 * cannot be use together with ISDX_BASED_FWD_ENA.
 *
 * \details
 * 0: Traffic is learned as received on logical port (configured in
 * ANA_CL:PORT:PORT_ID_CFG).
 * 1: Traffic is learned as received on the interface configured in
 * ANA_L2:ISDX:SERVICE_CTRL.FWD_TYPE and
 * ANA_L2:ISDX:SERVICE_CTRL.FWD_ADDR).
 *
 * Field: ::VTSS_ANA_L2_SERVICE_CTRL . ISDX_BASED_SRC_ENA
 */
#define  VTSS_F_ANA_L2_SERVICE_CTRL_ISDX_BASED_SRC_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),15,1)
#define  VTSS_M_ANA_L2_SERVICE_CTRL_ISDX_BASED_SRC_ENA  VTSS_BIT(15)
#define  VTSS_X_ANA_L2_SERVICE_CTRL_ISDX_BASED_SRC_ENA(x)  VTSS_EXTRACT_BITFIELD(x,15,1)

/**
 * \brief
 * Address type when ISDX_BASED_FWD_ENA or ISDX_BASED_SRC_ENA is set.
 *
 * \details
 * 0: UPSID_PN
 * 3: MC_IDX
 * 7: NO_ADDR
 * other: Reserved
 *
 * Field: ::VTSS_ANA_L2_SERVICE_CTRL . FWD_TYPE
 */
#define  VTSS_F_ANA_L2_SERVICE_CTRL_FWD_TYPE(x)  VTSS_ENCODE_BITFIELD(x,12,3)
#define  VTSS_M_ANA_L2_SERVICE_CTRL_FWD_TYPE     VTSS_ENCODE_BITMASK(12,3)
#define  VTSS_X_ANA_L2_SERVICE_CTRL_FWD_TYPE(x)  VTSS_EXTRACT_BITFIELD(x,12,3)

/**
 * \brief
 * Configured address when ISDX_BASED_FWD_ENA or ISDX_BASED_SRC_ENA is set.
 * The encoding of this field is specified by FWD_TYPE
 *
 * \details
 * FWD_TYPE= UPSID_PN:
 *   FWD_ADDR(9:5) = UPSID
 *   FWD_ADDR(4:0) = UPSPN
 * Specifies static unicast forwarding to lport
 *
 * FWD_TYPE = MC_IDX:
 * Specifies static multicast forwarding to the ports indexed by MC_IDX
 * into ANA_AC:PGID
 *
 * Field: ::VTSS_ANA_L2_SERVICE_CTRL . FWD_ADDR
 */
#define  VTSS_F_ANA_L2_SERVICE_CTRL_FWD_ADDR(x)  VTSS_ENCODE_BITFIELD(x,0,12)
#define  VTSS_M_ANA_L2_SERVICE_CTRL_FWD_ADDR     VTSS_ENCODE_BITMASK(0,12)
#define  VTSS_X_ANA_L2_SERVICE_CTRL_FWD_ADDR(x)  VTSS_EXTRACT_BITFIELD(x,0,12)


/**
 * \brief Ingress service port mask configuration
 *
 * \details
 * Configures port mask to be used in relation to service
 *
 * Register: \a ANA_L2:ISDX:PORT_MASK_CFG
 *
 * @param gi Replicator: x_FFL_CHIP_NUM_SDX (??), 0-1023
 */
#define VTSS_ANA_L2_PORT_MASK_CFG(gi)        VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x0,gi,32,0,1)

/**
 * \brief
 * Configures ingress service portmask. Can be used as replacement for VLAN
 * PORTMASK or as an further filtering of VLAN PORTMASK depending on
 * ANA_L2:ISDX:SERVICE_CTRL.PORT_MASK_REPLACE_ENA.
 *
 * \details
 * 'XX...XXX': Where X is '0' or '1', representing a port mask.
 *
 * Field: ::VTSS_ANA_L2_PORT_MASK_CFG . PORT_MASK
 */
#define  VTSS_F_ANA_L2_PORT_MASK_CFG_PORT_MASK(x)  VTSS_ENCODE_BITFIELD(x,0,30)
#define  VTSS_M_ANA_L2_PORT_MASK_CFG_PORT_MASK     VTSS_ENCODE_BITMASK(0,30)
#define  VTSS_X_ANA_L2_PORT_MASK_CFG_PORT_MASK(x)  VTSS_EXTRACT_BITFIELD(x,0,30)


/**
 * \brief Controls QGRP handling
 *
 * \details
 * Controls QGRP index and QSYS OAM drop counting.
 *
 * Register: \a ANA_L2:ISDX:QGRP_CFG
 *
 * @param gi Replicator: x_FFL_CHIP_NUM_SDX (??), 0-1023
 */
#define VTSS_ANA_L2_QGRP_CFG(gi)             VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x0,gi,32,0,4)

/**
 * \brief
 * Enables use of QGRP_IDX and QGRP_OAM_TYPE. If set, values from ISDX
 * table overwrites default values from ANA_L3:VLAN:QGRP_CFG.For the RSDX
 * lookup, MISC_CFG.RSDX_DIS from the ISDX lookup must be cleared for
 * QGRP_ENA to be considered.
 *
 * \details
 * Field: ::VTSS_ANA_L2_QGRP_CFG . QGRP_ENA
 */
#define  VTSS_F_ANA_L2_QGRP_CFG_QGRP_ENA(x)   VTSS_ENCODE_BITFIELD(!!(x),12,1)
#define  VTSS_M_ANA_L2_QGRP_CFG_QGRP_ENA      VTSS_BIT(12)
#define  VTSS_X_ANA_L2_QGRP_CFG_QGRP_ENA(x)   VTSS_EXTRACT_BITFIELD(x,12,1)

/**
 * \brief
 * Configures QSYS group
 *
 * \details
 * Field: ::VTSS_ANA_L2_QGRP_CFG . QGRP_IDX
 */
#define  VTSS_F_ANA_L2_QGRP_CFG_QGRP_IDX(x)   VTSS_ENCODE_BITFIELD(x,2,7)
#define  VTSS_M_ANA_L2_QGRP_CFG_QGRP_IDX      VTSS_ENCODE_BITMASK(2,7)
#define  VTSS_X_ANA_L2_QGRP_CFG_QGRP_IDX(x)   VTSS_EXTRACT_BITFIELD(x,2,7)

/**
 * \brief
 * Configures OAM type for traffic detected as OAM by Classification to be
 * used by QSYS to determining if dropped frames should be counted or
 * not.Ref: XQS:QMAP_QOS_TBL:DROP_STAT_CTRL.DROP_STAT_OAM_CNT_SEL
 *
 * \details
 * 0: Not OAM - Frames dropped by QSYS	are always counted in QSYS drop
 * stat.
 * 1: EVC OAM - Frames dropped by QSYS which are classified as OAM will
 * selectively be counted  in QSYS drop stat as EVC OAM.
 * 2: OVC / PW OAM - Frames dropped by QSYS which are classified as OAM
 * will selectively be counted	in QSYS drop stat as OVC / PW OAM.
 * 3: DOWN MEP OAM - Frames dropped by QSYS which are classified as OAM
 * will never be counted  in QSYS drop stat.
 *
 * Field: ::VTSS_ANA_L2_QGRP_CFG . QGRP_OAM_TYPE
 */
#define  VTSS_F_ANA_L2_QGRP_CFG_QGRP_OAM_TYPE(x)  VTSS_ENCODE_BITFIELD(x,0,2)
#define  VTSS_M_ANA_L2_QGRP_CFG_QGRP_OAM_TYPE     VTSS_ENCODE_BITMASK(0,2)
#define  VTSS_X_ANA_L2_QGRP_CFG_QGRP_OAM_TYPE(x)  VTSS_EXTRACT_BITFIELD(x,0,2)


/**
 * \brief Controls various indexes.
 *
 * \details
 * Controls BDLB and BUM indexes and SDLB policer pipeline point.
 *
 * Register: \a ANA_L2:ISDX:MISC_CFG
 *
 * @param gi Replicator: x_FFL_CHIP_NUM_SDX (??), 0-1023
 */
#define VTSS_ANA_L2_MISC_CFG(gi)             VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x0,gi,32,0,5)

/**
 * \brief
 * Disables cut-through in queue system.
 *
 * \details
 * Field: ::VTSS_ANA_L2_MISC_CFG . CT_DIS
 */
#define  VTSS_F_ANA_L2_MISC_CFG_CT_DIS(x)     VTSS_ENCODE_BITFIELD(!!(x),27,1)
#define  VTSS_M_ANA_L2_MISC_CFG_CT_DIS        VTSS_BIT(27)
#define  VTSS_X_ANA_L2_MISC_CFG_CT_DIS(x)     VTSS_EXTRACT_BITFIELD(x,27,1)

/**
 * \brief
 * Disable use of the RSDX lookup and keep values from the ISDX lookup.
 * When RSDX use is enabled and frame is to be routed, the ANA_L2:ISDX will
 * be looked up using RSDX and the following fields will be
 * replaced:SERVICE_CTRL.LPORT_REMOVE_ENASERVICE_CTRL.PORT_MASK_REPLACE_ENA
 * SERVICE_CTRL.AGGR_VALSERVICE_CTRL.SRC_MASK_DISSERVICE_CTRL.AGGR_REPLACE_
 * ENASERVICE_CTRL.ES0_ISDX_KEY_ENAQGRP_CFG.QGRP_IDXQGRP_CFG.QGRP_OAM_TYPEM
 * ISC_CFG.CT_DISPORT_MASK_CFG.PORT_MASKRelated
 * parameters:ANA_L3:VMID:VMID_MISC.RSDX
 *
 * \details
 * Field: ::VTSS_ANA_L2_MISC_CFG . RSDX_DIS
 */
#define  VTSS_F_ANA_L2_MISC_CFG_RSDX_DIS(x)   VTSS_ENCODE_BITFIELD(!!(x),26,1)
#define  VTSS_M_ANA_L2_MISC_CFG_RSDX_DIS      VTSS_BIT(26)
#define  VTSS_X_ANA_L2_MISC_CFG_RSDX_DIS(x)   VTSS_EXTRACT_BITFIELD(x,26,1)

/**
 * \brief
 * Configures the pipeline point for stat and SDLB policing.When injecting
 * or looping at a pipeline point after PIPELINE_PT will not cause SDLB
 * policing and ISDX counter updates.When extracting at a pipeline point
 * before PIPELINE_PT will not cause SDLB policing and ISDX counter
 * updates.
 *
 * \details
 * 0: NONE
 * 1: ANA_RB
 * 2: ANA_VRAP
 * 3: ANA_PORT_VOE
 * 4: ANA_CL
 * 5: ANA_CLM
 * 6: ANA_IPT_PROT
 * 7: ANA_OU_MIP
 * 8: ANA_OU_SW
 * 9: ANA_OU_PROT
 * 10: ANA_OU_VOE
 * 11: ANA_MID_PROT
 * 12: ANA_IN_VOE
 * 13: ANA_IN_PROT
 * 14: ANA_IN_SW
 * 15: ANA_IN_MIP
 * 16: ANA_VLAN
 *
 * Field: ::VTSS_ANA_L2_MISC_CFG . PIPELINE_PT
 */
#define  VTSS_F_ANA_L2_MISC_CFG_PIPELINE_PT(x)  VTSS_ENCODE_BITFIELD(x,21,5)
#define  VTSS_M_ANA_L2_MISC_CFG_PIPELINE_PT     VTSS_ENCODE_BITMASK(21,5)
#define  VTSS_X_ANA_L2_MISC_CFG_PIPELINE_PT(x)  VTSS_EXTRACT_BITFIELD(x,21,5)

/**
 * \brief
 * Controls if BUM index from VLAN table is overruled.
 *
 * \details
 * Field: ::VTSS_ANA_L2_MISC_CFG . BUM_SLB_ENA
 */
#define  VTSS_F_ANA_L2_MISC_CFG_BUM_SLB_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),20,1)
#define  VTSS_M_ANA_L2_MISC_CFG_BUM_SLB_ENA   VTSS_BIT(20)
#define  VTSS_X_ANA_L2_MISC_CFG_BUM_SLB_ENA(x)  VTSS_EXTRACT_BITFIELD(x,20,1)

/**
 * \brief
 * BUM policer index overrules index from VLAN table if BUM_SLB_ENA is
 * set.Related parameters:ANA_L3:VLAN:BUM_CFGANA_AC_POL:BUM_SLB
 *
 * \details
 * Field: ::VTSS_ANA_L2_MISC_CFG . BUM_SLB_IDX
 */
#define  VTSS_F_ANA_L2_MISC_CFG_BUM_SLB_IDX(x)  VTSS_ENCODE_BITFIELD(x,10,7)
#define  VTSS_M_ANA_L2_MISC_CFG_BUM_SLB_IDX     VTSS_ENCODE_BITMASK(10,7)
#define  VTSS_X_ANA_L2_MISC_CFG_BUM_SLB_IDX(x)  VTSS_EXTRACT_BITFIELD(x,10,7)

/**
 * \brief
 * Bundle Policer Dual leaky bucket index.If
 * ANA_L2::FWD_CFG.PORT_DEFAULT_BDLB_ENA is set some of the indexes are
 * used for port default DLB (see
 * ANA_L2::PORT_DLB_CFG.PORT_DLB_IDX).Related parameters:ANA_AC_POL:BDLB
 *
 * \details
 * Field: ::VTSS_ANA_L2_MISC_CFG . BDLB_IDX
 */
#define  VTSS_F_ANA_L2_MISC_CFG_BDLB_IDX(x)   VTSS_ENCODE_BITFIELD(x,0,7)
#define  VTSS_M_ANA_L2_MISC_CFG_BDLB_IDX      VTSS_ENCODE_BITMASK(0,7)
#define  VTSS_X_ANA_L2_MISC_CFG_BDLB_IDX(x)   VTSS_EXTRACT_BITFIELD(x,0,7)


/**
 * \brief Ingress service dual leaky bucket policer, offset per COSID
 *
 * \details
 * Register: \a ANA_L2:ISDX:DLB_COS_CFG
 *
 * @param gi Replicator: x_FFL_CHIP_NUM_SDX (??), 0-1023
 * @param ri Register: DLB_COS_CFG (??), 0-7
 */
#define VTSS_ANA_L2_DLB_COS_CFG(gi,ri)       VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x0,gi,32,ri,6)

/**
 * \brief
 * Dual leaky bucket offset per COSID.
 *
 * \details
 * Field: ::VTSS_ANA_L2_DLB_COS_CFG . DLB_COS_OFFSET
 */
#define  VTSS_F_ANA_L2_DLB_COS_CFG_DLB_COS_OFFSET(x)  VTSS_ENCODE_BITFIELD(x,0,3)
#define  VTSS_M_ANA_L2_DLB_COS_CFG_DLB_COS_OFFSET     VTSS_ENCODE_BITMASK(0,3)
#define  VTSS_X_ANA_L2_DLB_COS_CFG_DLB_COS_OFFSET(x)  VTSS_EXTRACT_BITFIELD(x,0,3)


/**
 * \brief Ingress service dual leaky bucket policer, base address
 *
 * \details
 * Specifies service DLB policer index.
 *
 * If ANA_L2::FWD_CFG.QUEUE_DEFAULT_SDLB_ENA is set some of the indexes are
 * used for queue default DLB (see ANA_L2::PORT_DLB_CFG.QUEUE_DLB_IDX).
 *
 * The ingress service DLB policer is selected as DLB_IDX +
 * DLB_COS_OFFSET[frame's classified  COSID].
 *
 * Register: \a ANA_L2:ISDX:DLB_CFG
 *
 * @param gi Replicator: x_FFL_CHIP_NUM_SDX (??), 0-1023
 */
#define VTSS_ANA_L2_DLB_CFG(gi)              VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x0,gi,32,0,14)

/**
 * \brief
 * Dual leaky bucket base address.Related parameters:ANA_AC_SDLB:LBSET_TBL
 *
 * \details
 * Field: ::VTSS_ANA_L2_DLB_CFG . DLB_IDX
 */
#define  VTSS_F_ANA_L2_DLB_CFG_DLB_IDX(x)     VTSS_ENCODE_BITFIELD(x,0,9)
#define  VTSS_M_ANA_L2_DLB_CFG_DLB_IDX        VTSS_ENCODE_BITMASK(0,9)
#define  VTSS_X_ANA_L2_DLB_CFG_DLB_IDX(x)     VTSS_EXTRACT_BITFIELD(x,0,9)


/**
 * \brief Ingress service counter set, base address
 *
 * \details
 * Register: \a ANA_L2:ISDX:ISDX_BASE_CFG
 *
 * @param gi Replicator: x_FFL_CHIP_NUM_SDX (??), 0-1023
 */
#define VTSS_ANA_L2_ISDX_BASE_CFG(gi)        VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x0,gi,32,0,15)

/**
 * \brief
 * Ingress service counter set base address.The ingress service counter set
 * in ANA_AC:STAT_CNT_CFG_ISDX is selected as ISDX_BASE_ADDR +
 * ISDX_COS_OFFSET[frame's classified  COSID].Related
 * parameters:ANA_AC:STAT_CNT_CFG_ISDXANA_L2:ISDX:ISDX_COS_CFG.ISDX_COS_OFF
 * SET
 *
 * \details
 * Field: ::VTSS_ANA_L2_ISDX_BASE_CFG . ISDX_BASE_ADDR
 */
#define  VTSS_F_ANA_L2_ISDX_BASE_CFG_ISDX_BASE_ADDR(x)  VTSS_ENCODE_BITFIELD(x,0,10)
#define  VTSS_M_ANA_L2_ISDX_BASE_CFG_ISDX_BASE_ADDR     VTSS_ENCODE_BITMASK(0,10)
#define  VTSS_X_ANA_L2_ISDX_BASE_CFG_ISDX_BASE_ADDR(x)  VTSS_EXTRACT_BITFIELD(x,0,10)


/**
 * \brief Ingress service counter set, offset per COSID
 *
 * \details
 * Register: \a ANA_L2:ISDX:ISDX_COS_CFG
 *
 * @param gi Replicator: x_FFL_CHIP_NUM_SDX (??), 0-1023
 * @param ri Register: ISDX_COS_CFG (??), 0-7
 */
#define VTSS_ANA_L2_ISDX_COS_CFG(gi,ri)      VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x0,gi,32,ri,16)

/**
 * \brief
 * Ingress service counter set offset per COSID.See ISDX_BASE_ADDR for
 * details.Related
 * parameters:ANA_AC:STAT_CNT_CFG_ISDXANA_L2:ISDX:ISDX_BASE_CFG.ISDX_BASE_A
 * DDR
 *
 * \details
 * Field: ::VTSS_ANA_L2_ISDX_COS_CFG . ISDX_COS_OFFSET
 */
#define  VTSS_F_ANA_L2_ISDX_COS_CFG_ISDX_COS_OFFSET(x)  VTSS_ENCODE_BITFIELD(x,0,3)
#define  VTSS_M_ANA_L2_ISDX_COS_CFG_ISDX_COS_OFFSET     VTSS_ENCODE_BITMASK(0,3)
#define  VTSS_X_ANA_L2_ISDX_COS_CFG_ISDX_COS_OFFSET(x)  VTSS_EXTRACT_BITFIELD(x,0,3)


/**
 * \brief ISDX learn limit configuration
 *
 * \details
 * Register: \a ANA_L2:ISDX:ISDX_LIMIT_CFG
 *
 * @param gi Replicator: x_FFL_CHIP_NUM_SDX (??), 0-1023
 */
#define VTSS_ANA_L2_ISDX_LIMIT_CFG(gi)       VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x0,gi,32,0,24)

/**
 * \brief
 * Select ISDX learn limit (ISDX_LIMIT) index applicable for this ISDX. Per
 * set of ISDXs learn limit is achieved by having multiple ISDXs pointing
 * to the same ISDX_LIMIT index.
 *
 * \details
 * Field: ::VTSS_ANA_L2_ISDX_LIMIT_CFG . ISDX_LIMIT_IDX
 */
#define  VTSS_F_ANA_L2_ISDX_LIMIT_CFG_ISDX_LIMIT_IDX(x)  VTSS_ENCODE_BITFIELD(x,0,8)
#define  VTSS_M_ANA_L2_ISDX_LIMIT_CFG_ISDX_LIMIT_IDX     VTSS_ENCODE_BITMASK(0,8)
#define  VTSS_X_ANA_L2_ISDX_LIMIT_CFG_ISDX_LIMIT_IDX(x)  VTSS_EXTRACT_BITFIELD(x,0,8)


/**
 * \brief TSN related parameters.
 *
 * \details
 * Register: \a ANA_L2:ISDX:TSN_CFG
 *
 * @param gi Replicator: x_FFL_CHIP_NUM_SDX (??), 0-1023
 */
#define VTSS_ANA_L2_TSN_CFG(gi)              VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x0,gi,32,0,25)

/**
 * \brief
 * Stream filter ID.Used as index into ANA_AC:TSN_SF_CFG.If set to 0, no
 * Stream Filter is applied to the frame.Related
 * parameters:ANA_AC:TSN_SF_CFG.
 *
 * \details
 * Field: ::VTSS_ANA_L2_TSN_CFG . TSN_SFID
 */
#define  VTSS_F_ANA_L2_TSN_CFG_TSN_SFID(x)    VTSS_ENCODE_BITFIELD(x,0,8)
#define  VTSS_M_ANA_L2_TSN_CFG_TSN_SFID       VTSS_ENCODE_BITMASK(0,8)
#define  VTSS_X_ANA_L2_TSN_CFG_TSN_SFID(x)    VTSS_EXTRACT_BITFIELD(x,0,8)

/**
 * Register Group: \a ANA_L2:STICKY
 *
 * Sticky diagnostic status
 */


/**
 * \brief Sticky diagnostic status
 *
 * \details
 * Register: \a ANA_L2:STICKY:STICKY
 */
#define VTSS_ANA_L2_STICKY                   VTSS_IOREG(VTSS_TO_ANA_L2,0x22574)

/**
 * \brief
 * Set if a valid entry was found in the PMAC table to be used for
 * forwarding.To enable the event as one of four counter events to the PORT
 * STAT block set the corresponding *_STICKY_MASK
 *
 * \details
 * 0: No entries have been found
 * 1: A valid entry has been found
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_STICKY . PMAC_ENTRY_FOUND_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_PMAC_ENTRY_FOUND_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),16,1)
#define  VTSS_M_ANA_L2_STICKY_PMAC_ENTRY_FOUND_STICKY  VTSS_BIT(16)
#define  VTSS_X_ANA_L2_STICKY_PMAC_ENTRY_FOUND_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,16,1)

/**
 * \brief
 * Set if an autonomous learning operation has failed due to specified lrn
 * rate is exceeded and LEARN max cnt was enabled. Write '1' to clear this
 * field.
 *
 * \details
 * 0: No cnt exceeded.
 * 1: An autonomous learning operation has failed due to cnt exceeded.
 * Write '1' to clear this field.
 *
 * Field: ::VTSS_ANA_L2_STICKY . AUTO_LRN_RATE_EXCEED_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_AUTO_LRN_RATE_EXCEED_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),15,1)
#define  VTSS_M_ANA_L2_STICKY_AUTO_LRN_RATE_EXCEED_STICKY  VTSS_BIT(15)
#define  VTSS_X_ANA_L2_STICKY_AUTO_LRN_RATE_EXCEED_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,15,1)

/**
 * \brief
 * Set if flood has been removed due to indication from VLAN table.To
 * enable the event as one of four counter events to the PORT STAT block
 * set the corresponding *_STICKY_MASK
 *
 * \details
 * 0: No event has occurred
 * 1: An integrity error has occurred
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_STICKY . FWD_FLOOD_KILL_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_FWD_FLOOD_KILL_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),14,1)
#define  VTSS_M_ANA_L2_STICKY_FWD_FLOOD_KILL_STICKY  VTSS_BIT(14)
#define  VTSS_X_ANA_L2_STICKY_FWD_FLOOD_KILL_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,14,1)

/**
 * \brief
 * Set if forwarding is performed without VLAN contribution either because
 * of a known Destination entry with VLAN ignore set
 * (LRN.MAC_ACCESS_CFG_2.MAC_ENTRY_VLAN_IGNORE FLAG) set or for an unknown
 * destination with flood VLAN ignore set
 * (ANA_L2::FWD_CFG.FLOOD_IGNORE_VLAN_ENA).To enable the event as one of
 * four counter events to the PORT STAT block set the corresponding
 * *_STICKY_MASK
 *
 * \details
 * 0: No event has occurred
 * 1: VLAN ignore as occurred
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_STICKY . VLAN_IGNORE_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_VLAN_IGNORE_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),13,1)
#define  VTSS_M_ANA_L2_STICKY_VLAN_IGNORE_STICKY  VTSS_BIT(13)
#define  VTSS_X_ANA_L2_STICKY_VLAN_IGNORE_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,13,1)

/**
 * \brief
 * Set if forwarding is performed without SRC contribution either because
 * of a known Destination entry with VLAN ignore set
 * (LRN.MAC_ACCESS_CFG_2.MAC_ENTRY_VLAN_IGNORE FLAG) set or for an unknown
 * destination with flood VLAN ignore set
 * (ANA_L2::FWD_CFG.FLOOD_IGNORE_VLAN_ENA) and filter_mode_sel set to SRC
 * ignore.To enable the event as one of four counter events to the PORT
 * STAT block set the corresponding *_STICKY_MASK
 *
 * \details
 * 0: No event has occurred
 * 1: VLAN ignore as occurred
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_STICKY . SRC_IGNORE_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_SRC_IGNORE_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),12,1)
#define  VTSS_M_ANA_L2_STICKY_SRC_IGNORE_STICKY  VTSS_BIT(12)
#define  VTSS_X_ANA_L2_STICKY_SRC_IGNORE_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,12,1)

/**
 * \brief
 * Indication of a frame received with an UPSID != own UPSID
 * (ANA_L2::OWN_UPSID.OWN_UPSID) which was previously learned on own front
 * local port.To enable the event as one of four counter events to the PORT
 * STAT block set the corresponding *_STICKY_MASK
 *
 * \details
 * 0: No event has occurred
 * 1: A Local to remote port move has been detected
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_STICKY . LOCAL_TO_REMOTE_PORTMOVE_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_LOCAL_TO_REMOTE_PORTMOVE_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),11,1)
#define  VTSS_M_ANA_L2_STICKY_LOCAL_TO_REMOTE_PORTMOVE_STICKY  VTSS_BIT(11)
#define  VTSS_X_ANA_L2_STICKY_LOCAL_TO_REMOTE_PORTMOVE_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,11,1)

/**
 * \brief
 * Indication of a frame received on a local front port, which was
 * previously learned on a remote UPSID front port.To enable the event as
 * one of four counter events to the PORT STAT block set the corresponding
 * *_STICKY_MASK
 *
 * \details
 * 0: No event has occurred
 * 1: A remote port to local port move has been detected
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_STICKY . REMOTE_TO_LOCAL_PORTMOVE_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_REMOTE_TO_LOCAL_PORTMOVE_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),10,1)
#define  VTSS_M_ANA_L2_STICKY_REMOTE_TO_LOCAL_PORTMOVE_STICKY  VTSS_BIT(10)
#define  VTSS_X_ANA_L2_STICKY_REMOTE_TO_LOCAL_PORTMOVE_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,10,1)

/**
 * \brief
 * Indication of a frame received on a remote front port, which was
 * previously learned on another remote UPSID front port.To enable the
 * event as one of four counter events to the PORT STAT block set the
 * corresponding *_STICKY_MASK
 *
 * \details
 * 0: No event has occurred
 * 1: A remote port to remote port move has been detected
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_STICKY . REMOTE_TO_REMOTE_PORTMOVE_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_REMOTE_TO_REMOTE_PORTMOVE_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),9,1)
#define  VTSS_M_ANA_L2_STICKY_REMOTE_TO_REMOTE_PORTMOVE_STICKY  VTSS_BIT(9)
#define  VTSS_X_ANA_L2_STICKY_REMOTE_TO_REMOTE_PORTMOVE_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,9,1)

/**
 * \brief
 * Indication of a frame received on a GLAG which was previously learned
 * with a different GLAG.To enable the event as one of four counter events
 * to the PORT STAT block set the corresponding *_STICKY_MASK
 *
 * \details
 * 0: No event has occurred
 * 1: A GLAG to GLAG port move has been detected
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_STICKY . GLOBAL_TO_GLOBAL_PORTMOVE_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_GLOBAL_TO_GLOBAL_PORTMOVE_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),8,1)
#define  VTSS_M_ANA_L2_STICKY_GLOBAL_TO_GLOBAL_PORTMOVE_STICKY  VTSS_BIT(8)
#define  VTSS_X_ANA_L2_STICKY_GLOBAL_TO_GLOBAL_PORTMOVE_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,8,1)

/**
 * \brief
 * Indication of a frame received at a front local port which was
 * previously learned with a GLAG .To enable the event as one of four
 * counter events to the PORT STAT block set the corresponding
 * *_STICKY_MASK
 *
 * \details
 * 0: No event has occurred
 * 1: A GLAG to local port move has been detected
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_STICKY . GLOBAL_TO_LOCAL_PORTMOVE_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_GLOBAL_TO_LOCAL_PORTMOVE_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),7,1)
#define  VTSS_M_ANA_L2_STICKY_GLOBAL_TO_LOCAL_PORTMOVE_STICKY  VTSS_BIT(7)
#define  VTSS_X_ANA_L2_STICKY_GLOBAL_TO_LOCAL_PORTMOVE_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,7,1)

/**
 * \brief
 * Indication of a frame received with GLAG which was previously learned at
 * a front local port.To enable the event as one of four counter events to
 * the PORT STAT block set the corresponding *_STICKY_MASK
 *
 * \details
 * 0: No event has occurred
 * 1: A Local to GLAG port move has been detected
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_STICKY . LOCAL_TO_GLOBAL_PORTMOVE_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_LOCAL_TO_GLOBAL_PORTMOVE_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),6,1)
#define  VTSS_M_ANA_L2_STICKY_LOCAL_TO_GLOBAL_PORTMOVE_STICKY  VTSS_BIT(6)
#define  VTSS_X_ANA_L2_STICKY_LOCAL_TO_GLOBAL_PORTMOVE_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,6,1)

/**
 * \brief
 * Indication of a frame received on a local front port, which was
 * previously learned at a different local front port.To enable the event
 * as one of four counter events to the PORT STAT block set the
 * corresponding *_STICKY_MASK
 *
 * \details
 * 0: No event has occurred
 * 1: A Local to local port move has been detected
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_STICKY . LOCAL_TO_LOCAL_PORTMOVE_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_LOCAL_TO_LOCAL_PORTMOVE_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),5,1)
#define  VTSS_M_ANA_L2_STICKY_LOCAL_TO_LOCAL_PORTMOVE_STICKY  VTSS_BIT(5)
#define  VTSS_X_ANA_L2_STICKY_LOCAL_TO_LOCAL_PORTMOVE_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,5,1)

/**
 * \brief
 * Set if forwarding is based on flood.To enable the event as one of four
 * counter events to the PORT STAT block set the corresponding
 * *_STICKY_MASK
 *
 * \details
 * 0:  No flood event
 * 1: Flood event
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_STICKY . FWD_FLOOD_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_FWD_FLOOD_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),4,1)
#define  VTSS_M_ANA_L2_STICKY_FWD_FLOOD_STICKY  VTSS_BIT(4)
#define  VTSS_X_ANA_L2_STICKY_FWD_FLOOD_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,4,1)

/**
 * \brief
 * Set if a valid (EFID, DMAC) entry was found to be used for forwarding.To
 * enable the event as one of four counter events to the PORT STAT block
 * set the corresponding *_STICKY_MASK
 *
 * \details
 * 0: No (EFID, DMAC) entries have been found
 * 1: An (EFID, DMAC) entry has been found.
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_STICKY . FWD_ENTRY_FOUND_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_FWD_ENTRY_FOUND_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),3,1)
#define  VTSS_M_ANA_L2_STICKY_FWD_ENTRY_FOUND_STICKY  VTSS_BIT(3)
#define  VTSS_X_ANA_L2_STICKY_FWD_ENTRY_FOUND_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,3,1)

/**
 * \brief
 * Set if a frame has been dropped due to
 * ANA_L2::LRN_SECUR_CFG.DROP_UNKNOWN_SRC_ENA.To enable the event as one of
 * four counter events to the PORT STAT block set the corresponding
 * *_STICKY_MASK
 *
 * \details
 * 0: No frames have been dropped
 * 1: A frame has been dropped.
 *
 * Bit is cleared by writing a 1 to this position.
 *
 * Field: ::VTSS_ANA_L2_STICKY . DROP_UNKNOWN_SRC_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_DROP_UNKNOWN_SRC_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),2,1)
#define  VTSS_M_ANA_L2_STICKY_DROP_UNKNOWN_SRC_STICKY  VTSS_BIT(2)
#define  VTSS_X_ANA_L2_STICKY_DROP_UNKNOWN_SRC_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,2,1)

/**
 * \brief
 * Set if a valid (IFID, SMAC) entry was found.To enable the event as one
 * of four counter events to the PORT STAT block set the corresponding
 * *_STICKY_MASK
 *
 * \details
 * 0: No known source entries (IFID, SMAC) has been found
 * 1: At least one known entry (IFID, SMAC) has been found.
 *
 * Write 1 to clear this field.
 *
 * Field: ::VTSS_ANA_L2_STICKY . LEARN_KNOWN_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_LEARN_KNOWN_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),1,1)
#define  VTSS_M_ANA_L2_STICKY_LEARN_KNOWN_STICKY  VTSS_BIT(1)
#define  VTSS_X_ANA_L2_STICKY_LEARN_KNOWN_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,1,1)

/**
 * \brief
 * Set if no valid (IFID, SMAC) entry was found.To enable the event as one
 * of four counter events to the PORT STAT block set the corresponding
 * *_STICKY_MASK
 *
 * \details
 * 0: No unknown sources (IFID, SMAC) has been found
 * 1: At least one unknown source IFID, SMAC) has been found.
 *
 * Write 1 to clear this field.
 *
 * Field: ::VTSS_ANA_L2_STICKY . LEARN_UNKNOWN_STICKY
 */
#define  VTSS_F_ANA_L2_STICKY_LEARN_UNKNOWN_STICKY(x)  VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_ANA_L2_STICKY_LEARN_UNKNOWN_STICKY  VTSS_BIT(0)
#define  VTSS_X_ANA_L2_STICKY_LEARN_UNKNOWN_STICKY(x)  VTSS_EXTRACT_BITFIELD(x,0,1)

/**
 * Register Group: \a ANA_L2:STICKY_MASK
 *
 * Sticky diagnostic global port counter event configuration
 */


/**
 * \brief Sticky diagnostic counter mask
 *
 * \details
 * Register: \a ANA_L2:STICKY_MASK:STICKY_MASK
 *
 * @param gi Replicator: x_FFL_ANA_NUM_CONCURRENT_CNTS (??), 0-3
 */
#define VTSS_ANA_L2_STICKY_MASK(gi)          VTSS_IOREG_IX(VTSS_TO_ANA_L2,0x22575,gi,1,0,0)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . PMAC_ENTRY_FOUND_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_PMAC_ENTRY_FOUND_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),16,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_PMAC_ENTRY_FOUND_STICKY_MASK  VTSS_BIT(16)
#define  VTSS_X_ANA_L2_STICKY_MASK_PMAC_ENTRY_FOUND_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,16,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . AUTO_LRN_RATE_EXCEED_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_AUTO_LRN_RATE_EXCEED_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),15,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_AUTO_LRN_RATE_EXCEED_STICKY_MASK  VTSS_BIT(15)
#define  VTSS_X_ANA_L2_STICKY_MASK_AUTO_LRN_RATE_EXCEED_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,15,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . FWD_FLOOD_KILL_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_FWD_FLOOD_KILL_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),14,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_FWD_FLOOD_KILL_STICKY_MASK  VTSS_BIT(14)
#define  VTSS_X_ANA_L2_STICKY_MASK_FWD_FLOOD_KILL_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,14,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . VLAN_IGNORE_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_VLAN_IGNORE_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),13,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_VLAN_IGNORE_STICKY_MASK  VTSS_BIT(13)
#define  VTSS_X_ANA_L2_STICKY_MASK_VLAN_IGNORE_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,13,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . SRC_IGNORE_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_SRC_IGNORE_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),12,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_SRC_IGNORE_STICKY_MASK  VTSS_BIT(12)
#define  VTSS_X_ANA_L2_STICKY_MASK_SRC_IGNORE_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,12,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . LOCAL_TO_REMOTE_PORTMOVE_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_LOCAL_TO_REMOTE_PORTMOVE_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),11,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_LOCAL_TO_REMOTE_PORTMOVE_STICKY_MASK  VTSS_BIT(11)
#define  VTSS_X_ANA_L2_STICKY_MASK_LOCAL_TO_REMOTE_PORTMOVE_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,11,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . REMOTE_TO_LOCAL_PORTMOVE_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_REMOTE_TO_LOCAL_PORTMOVE_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),10,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_REMOTE_TO_LOCAL_PORTMOVE_STICKY_MASK  VTSS_BIT(10)
#define  VTSS_X_ANA_L2_STICKY_MASK_REMOTE_TO_LOCAL_PORTMOVE_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,10,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . REMOTE_TO_REMOTE_PORTMOVE_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_REMOTE_TO_REMOTE_PORTMOVE_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),9,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_REMOTE_TO_REMOTE_PORTMOVE_STICKY_MASK  VTSS_BIT(9)
#define  VTSS_X_ANA_L2_STICKY_MASK_REMOTE_TO_REMOTE_PORTMOVE_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,9,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . GLOBAL_TO_GLOBAL_PORTMOVE_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_GLOBAL_TO_GLOBAL_PORTMOVE_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),8,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_GLOBAL_TO_GLOBAL_PORTMOVE_STICKY_MASK  VTSS_BIT(8)
#define  VTSS_X_ANA_L2_STICKY_MASK_GLOBAL_TO_GLOBAL_PORTMOVE_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,8,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . GLOBAL_TO_LOCAL_PORTMOVE_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_GLOBAL_TO_LOCAL_PORTMOVE_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),7,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_GLOBAL_TO_LOCAL_PORTMOVE_STICKY_MASK  VTSS_BIT(7)
#define  VTSS_X_ANA_L2_STICKY_MASK_GLOBAL_TO_LOCAL_PORTMOVE_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,7,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . LOCAL_TO_GLOBAL_PORTMOVE_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_LOCAL_TO_GLOBAL_PORTMOVE_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),6,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_LOCAL_TO_GLOBAL_PORTMOVE_STICKY_MASK  VTSS_BIT(6)
#define  VTSS_X_ANA_L2_STICKY_MASK_LOCAL_TO_GLOBAL_PORTMOVE_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,6,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . LOCAL_TO_LOCAL_PORTMOVE_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_LOCAL_TO_LOCAL_PORTMOVE_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),5,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_LOCAL_TO_LOCAL_PORTMOVE_STICKY_MASK  VTSS_BIT(5)
#define  VTSS_X_ANA_L2_STICKY_MASK_LOCAL_TO_LOCAL_PORTMOVE_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,5,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . FWD_FLOOD_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_FWD_FLOOD_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),4,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_FWD_FLOOD_STICKY_MASK  VTSS_BIT(4)
#define  VTSS_X_ANA_L2_STICKY_MASK_FWD_FLOOD_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,4,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . FWD_ENTRY_FOUND_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_FWD_ENTRY_FOUND_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),3,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_FWD_ENTRY_FOUND_STICKY_MASK  VTSS_BIT(3)
#define  VTSS_X_ANA_L2_STICKY_MASK_FWD_ENTRY_FOUND_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,3,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . DROP_UNKNOWN_SRC_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_DROP_UNKNOWN_SRC_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),2,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_DROP_UNKNOWN_SRC_STICKY_MASK  VTSS_BIT(2)
#define  VTSS_X_ANA_L2_STICKY_MASK_DROP_UNKNOWN_SRC_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,2,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . LEARN_KNOWN_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_LEARN_KNOWN_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),1,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_LEARN_KNOWN_STICKY_MASK  VTSS_BIT(1)
#define  VTSS_X_ANA_L2_STICKY_MASK_LEARN_KNOWN_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,1,1)

/**
 * \brief
 * Mask count of sticky event.
 *
 * \details
 * 0: Disable event count
 * 1: Enable event count
 *
 * Field: ::VTSS_ANA_L2_STICKY_MASK . LEARN_UNKNOWN_STICKY_MASK
 */
#define  VTSS_F_ANA_L2_STICKY_MASK_LEARN_UNKNOWN_STICKY_MASK(x)  VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_ANA_L2_STICKY_MASK_LEARN_UNKNOWN_STICKY_MASK  VTSS_BIT(0)
#define  VTSS_X_ANA_L2_STICKY_MASK_LEARN_UNKNOWN_STICKY_MASK(x)  VTSS_EXTRACT_BITFIELD(x,0,1)

/**
 * Register Group: \a ANA_L2:PMAC
 *
 * Configure PMAC table
 */


/**
 * \brief Control CPU access to PMAC table
 *
 * \details
 * The PMAC_ACCESS_CMD field of this register is used for updating and
 * reading the PMAC table. The command (PMAC_ACCESS_CMD) selects between
 * different operations and uses the following encoding:
 *   00 - IDLE:
 *   The previous operation has completed.
 *
 *   01 - READ:
 *   The PMAC table entry set in PMAC_INDEX.PMAC_INDEX is returned in the
 * relevant register bits within PMAC_ACCESS_CFG_2.
 *
 *   10 - WRITE:
 *   The PMAC table entry pointed to by PMAC_INDEX.PMAC_INDEX is updated
 * with relevant register bits within PMAC_ACCESS_CFG_2
 *
 *   11 - INIT:
 *   The PMAC table is initialized to default values.
 *
 * The PMAC_ACCESS_CMD must be IDLE before a new command can be issued. The
 * INIT command run for approximately 120 us whereas the other commands
 * execute immediately. When an operation has completed PMAC_ACCESS_CMD
 * changes to IDLE.
 *
 * Register: \a ANA_L2:PMAC:PMAC_ACCESS_CTRL
 */
#define VTSS_ANA_L2_PMAC_ACCESS_CTRL         VTSS_IOREG(VTSS_TO_ANA_L2,0x22579)

/**
 * \brief
 * Control access to PMAC table
 *
 * \details
 * 0: IDLE
 * 1: READ Read PMAC table entry for specific index.
 * 2: WRITE. Write PMAC table entry for specific index.
 * 3: INIT: Initialize the table.

 *
 * Field: ::VTSS_ANA_L2_PMAC_ACCESS_CTRL . PMAC_ACCESS_CMD
 */
#define  VTSS_F_ANA_L2_PMAC_ACCESS_CTRL_PMAC_ACCESS_CMD(x)  VTSS_ENCODE_BITFIELD(x,0,2)
#define  VTSS_M_ANA_L2_PMAC_ACCESS_CTRL_PMAC_ACCESS_CMD     VTSS_ENCODE_BITMASK(0,2)
#define  VTSS_X_ANA_L2_PMAC_ACCESS_CTRL_PMAC_ACCESS_CMD(x)  VTSS_EXTRACT_BITFIELD(x,0,2)


/**
 * \brief Index used to select PMAC table entry for read/write operations (see PMAC_ACCESS_CTRL)
 *
 * \details
 * Register: \a ANA_L2:PMAC:PMAC_INDEX
 */
#define VTSS_ANA_L2_PMAC_INDEX               VTSS_IOREG(VTSS_TO_ANA_L2,0x2257a)

/**
 * \brief
 * Index used to select PMAC table entry for read/write operations (see
 * PMAC_ACCESS_CTRL).The index is auto incremented after each read or write
 * command (See PMAC_ACCESS_CMD)
 *
 * \details
 * Field: ::VTSS_ANA_L2_PMAC_INDEX . PMAC_INDEX
 */
#define  VTSS_F_ANA_L2_PMAC_INDEX_PMAC_INDEX(x)  VTSS_ENCODE_BITFIELD(x,0,14)
#define  VTSS_M_ANA_L2_PMAC_INDEX_PMAC_INDEX     VTSS_ENCODE_BITMASK(0,14)
#define  VTSS_X_ANA_L2_PMAC_INDEX_PMAC_INDEX(x)  VTSS_EXTRACT_BITFIELD(x,0,14)


/**
 * \brief Entry in PMAC table
 *
 * \details
 * Register: \a ANA_L2:PMAC:PMAC_ACCESS_CFG_2
 */
#define VTSS_ANA_L2_PMAC_ACCESS_CFG_2        VTSS_IOREG(VTSS_TO_ANA_L2,0x2257b)

/**
 * \brief
 * CPU extraction queue
 *
 * \details
 * Field: ::VTSS_ANA_L2_PMAC_ACCESS_CFG_2 . PMAC_ENTRY_CPU_QU
 */
#define  VTSS_F_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_CPU_QU(x)  VTSS_ENCODE_BITFIELD(x,18,3)
#define  VTSS_M_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_CPU_QU     VTSS_ENCODE_BITMASK(18,3)
#define  VTSS_X_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_CPU_QU(x)  VTSS_EXTRACT_BITFIELD(x,18,3)

/**
 * \brief
 * Copy frames to this station to the CPU.
 *
 * \details
 * Field: ::VTSS_ANA_L2_PMAC_ACCESS_CFG_2 . PMAC_ENTRY_CPU_COPY
 */
#define  VTSS_F_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_CPU_COPY(x)  VTSS_ENCODE_BITFIELD(!!(x),17,1)
#define  VTSS_M_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_CPU_COPY  VTSS_BIT(17)
#define  VTSS_X_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_CPU_COPY(x)  VTSS_EXTRACT_BITFIELD(x,17,1)

/**
 * \brief
 * Configures PMAC_ENTRY VLAN ignore flag.
 *
 * \details
 * Field: ::VTSS_ANA_L2_PMAC_ACCESS_CFG_2 . PMAC_ENTRY_VLAN_IGNORE
 */
#define  VTSS_F_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_VLAN_IGNORE(x)  VTSS_ENCODE_BITFIELD(!!(x),16,1)
#define  VTSS_M_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_VLAN_IGNORE  VTSS_BIT(16)
#define  VTSS_X_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_VLAN_IGNORE(x)  VTSS_EXTRACT_BITFIELD(x,16,1)

/**
 * \brief
 * Configures PMAC_ENTRY address type.
 *
 * \details
 * 0: UPSID_PN
 * 1: UPSID_CPU_OR_INT
 * 2: GLAG
 * 3: MC_IDX
 *
 * Field: ::VTSS_ANA_L2_PMAC_ACCESS_CFG_2 . PMAC_ENTRY_ADDR_TYPE
 */
#define  VTSS_F_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_ADDR_TYPE(x)  VTSS_ENCODE_BITFIELD(x,13,3)
#define  VTSS_M_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_ADDR_TYPE     VTSS_ENCODE_BITMASK(13,3)
#define  VTSS_X_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_ADDR_TYPE(x)  VTSS_EXTRACT_BITFIELD(x,13,3)

/**
 * \brief
 * Configures the address for the PMAC entry. Encoding of this field is
 * determined by PMAC_ENTRY_ADDR_TYPE.
 *
 * \details
 * Field: ::VTSS_ANA_L2_PMAC_ACCESS_CFG_2 . PMAC_ENTRY_ADDR
 */
#define  VTSS_F_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_ADDR(x)  VTSS_ENCODE_BITFIELD(x,1,12)
#define  VTSS_M_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_ADDR     VTSS_ENCODE_BITMASK(1,12)
#define  VTSS_X_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_ADDR(x)  VTSS_EXTRACT_BITFIELD(x,1,12)

/**
 * \brief
 * Configures the MAC_ENTRY valid flag
 *
 * \details
 * Field: ::VTSS_ANA_L2_PMAC_ACCESS_CFG_2 . PMAC_ENTRY_VLD
 */
#define  VTSS_F_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_VLD(x)  VTSS_ENCODE_BITFIELD(!!(x),0,1)
#define  VTSS_M_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_VLD  VTSS_BIT(0)
#define  VTSS_X_ANA_L2_PMAC_ACCESS_CFG_2_PMAC_ENTRY_VLD(x)  VTSS_EXTRACT_BITFIELD(x,0,1)


/**
 * \brief PMAC table configuration
 *
 * \details
 * Register: \a ANA_L2:PMAC:PMAC_CFG
 */
#define VTSS_ANA_L2_PMAC_CFG                 VTSS_IOREG(VTSS_TO_ANA_L2,0x2257c)

/**
 * \brief
 * Enable use of PMAC table
 *
 * \details
 * Field: ::VTSS_ANA_L2_PMAC_CFG . PMAC_ENA
 */
#define  VTSS_F_ANA_L2_PMAC_CFG_PMAC_ENA(x)   VTSS_ENCODE_BITFIELD(!!(x),24,1)
#define  VTSS_M_ANA_L2_PMAC_CFG_PMAC_ENA      VTSS_BIT(24)
#define  VTSS_X_ANA_L2_PMAC_CFG_PMAC_ENA(x)   VTSS_EXTRACT_BITFIELD(x,24,1)

/**
 * \brief
 * The OUI (upper 24-bits of MAC address) from the frame's destination MAC
 * address must match the configured value
 *
 * \details
 * Field: ::VTSS_ANA_L2_PMAC_CFG . PMAC_OUI
 */
#define  VTSS_F_ANA_L2_PMAC_CFG_PMAC_OUI(x)   VTSS_ENCODE_BITFIELD(x,0,24)
#define  VTSS_M_ANA_L2_PMAC_CFG_PMAC_OUI      VTSS_ENCODE_BITMASK(0,24)
#define  VTSS_X_ANA_L2_PMAC_CFG_PMAC_OUI(x)   VTSS_EXTRACT_BITFIELD(x,0,24)


/**
 * \brief PMAC table configuration. DMAC
 *
 * \details
 * DMAC configuration
 *
 * Register: \a ANA_L2:PMAC:PMAC_CFG_2
 */
#define VTSS_ANA_L2_PMAC_CFG_2               VTSS_IOREG(VTSS_TO_ANA_L2,0x2257d)

/**
 * \brief
 * Enable check of bits 12 to 23 of the frame DMAC. See PMAC_DMAC_LSBBits
 * 12 to 23 of the frame DMAC are ignored if this is disabled.
 *
 * \details
 * Field: ::VTSS_ANA_L2_PMAC_CFG_2 . PMAC_CHK_DMAC_LSB_ENA
 */
#define  VTSS_F_ANA_L2_PMAC_CFG_2_PMAC_CHK_DMAC_LSB_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),12,1)
#define  VTSS_M_ANA_L2_PMAC_CFG_2_PMAC_CHK_DMAC_LSB_ENA  VTSS_BIT(12)
#define  VTSS_X_ANA_L2_PMAC_CFG_2_PMAC_CHK_DMAC_LSB_ENA(x)  VTSS_EXTRACT_BITFIELD(x,12,1)

/**
 * \brief
 * Bits 12 to 23 from the frame's destination MAC address must match the
 * configured value. Enabled by PMAC_CHK_DMAC_LSB_ENA
 *
 * \details
 * Field: ::VTSS_ANA_L2_PMAC_CFG_2 . PMAC_DMAC_LSB
 */
#define  VTSS_F_ANA_L2_PMAC_CFG_2_PMAC_DMAC_LSB(x)  VTSS_ENCODE_BITFIELD(x,0,12)
#define  VTSS_M_ANA_L2_PMAC_CFG_2_PMAC_DMAC_LSB     VTSS_ENCODE_BITMASK(0,12)
#define  VTSS_X_ANA_L2_PMAC_CFG_2_PMAC_DMAC_LSB(x)  VTSS_EXTRACT_BITFIELD(x,0,12)


/**
 * \brief PMAC table VLAN configuration
 *
 * \details
 * The PMAC table is organized as 4,096 entries per VLAN supporting up to 4
 * VLANs. The PMAC table is enabled in ANA_L2::PMAC_CFG.PMAC_ENA.
 *
 * Register: \a ANA_L2:PMAC:PMAC_VLAN_CFG
 *
 * @param ri Replicator: x_FFL_ANA_NUM_PMAC_TABLE_VLANS (??), 0-3
 */
#define VTSS_ANA_L2_PMAC_VLAN_CFG(ri)        VTSS_IOREG(VTSS_TO_ANA_L2,0x2257e + (ri))

/**
 * \brief
 * Set to 1 to enable this PMAC VLAN for the VID specified in PMAC_VLAN.
 *
 * \details
 * Field: ::VTSS_ANA_L2_PMAC_VLAN_CFG . PMAC_VLAN_ENA
 */
#define  VTSS_F_ANA_L2_PMAC_VLAN_CFG_PMAC_VLAN_ENA(x)  VTSS_ENCODE_BITFIELD(!!(x),13,1)
#define  VTSS_M_ANA_L2_PMAC_VLAN_CFG_PMAC_VLAN_ENA  VTSS_BIT(13)
#define  VTSS_X_ANA_L2_PMAC_VLAN_CFG_PMAC_VLAN_ENA(x)  VTSS_EXTRACT_BITFIELD(x,13,1)

/**
 * \brief
 * ID of VLAN used for PMAC table.
 *
 * \details
 * Field: ::VTSS_ANA_L2_PMAC_VLAN_CFG . PMAC_VLAN
 */
#define  VTSS_F_ANA_L2_PMAC_VLAN_CFG_PMAC_VLAN(x)  VTSS_ENCODE_BITFIELD(x,0,13)
#define  VTSS_M_ANA_L2_PMAC_VLAN_CFG_PMAC_VLAN     VTSS_ENCODE_BITMASK(0,13)
#define  VTSS_X_ANA_L2_PMAC_VLAN_CFG_PMAC_VLAN(x)  VTSS_EXTRACT_BITFIELD(x,0,13)


#endif /* _VTSS_LAGUNA_REGS_ANA_L2_H_ */
