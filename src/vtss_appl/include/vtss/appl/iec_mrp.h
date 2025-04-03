/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 * \file iec_mrp.h
 * \brief Public MRP (Media Redundancy Protocol; IEC 62439-2, Edition 2.0,
 * 2016-03) API.
 * \details This header file describes the MRP control functions and types.
 */

#ifndef _VTSS_APPL_IEC_MRP_H_
#define _VTSS_APPL_IEC_MRP_H_

#include <microchip/ethernet/switch/api.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/vlan.h>
#include <vtss/appl/module_id.h>
#include <vtss/appl/cfm.hxx>
#include <vtss/basics/enum_macros.hxx> /* For VTSS_ENUM_BITWISE() */

/**
 * Definition of error return codes.
 * See also iec_mrp_error_txt() in iec_mrp.cxx.
 */
enum {
    VTSS_APPL_IEC_MRP_RC_INVALID_PARAMETER = MODULE_ERROR_START(VTSS_MODULE_ID_IEC_MRP), /**< Invalid parameter                                                          */
    VTSS_APPL_IEC_MRP_RC_INTERNAL_ERROR,                                                 /**< Internal error. Requires code update                                       */
    VTSS_APPL_IEC_MRP_RC_NO_SUCH_INSTANCE,                                               /**< MRP instance is not created                                                */
    VTSS_APPL_IEC_MRP_RC_INVALID_ROLE,                                                   /**< Invalid role                                                               */
    VTSS_APPL_IEC_MRP_RC_INVALID_IN_ROLE,                                                /**< Invalid interconnection role                                               */
    VTSS_APPL_IEC_MRP_RC_INVALID_IN_MODE,                                                /**< Invalid interconnection mode                                               */
    VTSS_APPL_IEC_MRP_RC_NAME_NOT_NULL_TERMINATED,                                       /**< Name string is not NULL-terminated                                         */
    VTSS_APPL_IEC_MRP_RC_NAME_NOT_NULL_TERMINATED_INTERCONNECTION,                       /**< Interconnection name string is not NULL-terminated                         */
    VTSS_APPL_IEC_MRP_RC_NAME_INVALID,                                                   /**< Name contains invalid characters                                           */
    VTSS_APPL_IEC_MRP_RC_NAME_INVALID_INTERCONNECTION,                                   /**< Interconnection name contains invalid characetrs                           */
    VTSS_APPL_IEC_MRP_RC_UUID_MUST_BE_EXACTLY_36_CHARS_LONG,                             /**< A UUID must be exactly 36 chars long                                       */
    VTSS_APPL_IEC_MRP_RC_UUID_MUST_CONTAIN_DASHES_AT_POS_9_14_19_24,                     /**< A UUID must contain dashes at position 9, 14, 19 and 24                    */
    VTSS_APPL_IEC_MRP_RC_UUID_MUST_ONLY_CONTAIN_HEX_DIGITS,                              /**< A UUID must only contain hex digits                                        */
    VTSS_APPL_IEC_MRP_RC_UUID_MAY_NOT_BE_ALL_ZEROS,                                      /**< Domain ID/UUID may not be all-zeros                                        */
    VTSS_APPL_IEC_MRP_RC_INVALID_OUI_TYPE,                                               /**< Invalid OUI type                                                           */
    VTSS_APPL_IEC_MRP_RC_INVALID_PORT1_IFINDEX,                                          /**< Invalid port1 ifindex                                                      */
    VTSS_APPL_IEC_MRP_RC_INVALID_PORT2_IFINDEX,                                          /**< Invalid port2 ifindex                                                      */
    VTSS_APPL_IEC_MRP_RC_INVALID_INTERCONNECTION_IFINDEX,                                /**< Invalid interconnection ifindex                                            */
    VTSS_APPL_IEC_MRP_RC_INVALID_SF_TRIGGER_PORT1,                                       /**< Invalid SF trigger for port1                                               */
    VTSS_APPL_IEC_MRP_RC_INVALID_SF_TRIGGER_PORT2,                                       /**< Invalid SF trigger for port2                                               */
    VTSS_APPL_IEC_MRP_RC_INVALID_SF_TRIGGER_INTERCONNECTION,                             /**< Invalid SF trigger for interconnection port                                */
    VTSS_APPL_IEC_MRP_RC_PORT1_MEP_MUST_BE_SPECIFIED,                                    /**< Port1 MEP must be specified when SF-trigger is MEP                         */
    VTSS_APPL_IEC_MRP_RC_PORT2_MEP_MUST_BE_SPECIFIED,                                    /**< Port2 MEP must be specified when SF-trigger is MEP                         */
    VTSS_APPL_IEC_MRP_RC_INTERCONNECTION_MEP_MUST_BE_SPECIFIED,                          /**< Interconnection MEP must be specified when SF-trigger is MEP               */
    VTSS_APPL_IEC_MRP_RC_INVALID_CONTROL_VLAN_RING,                                      /**< Invalid ring control VLAN                                                  */
    VTSS_APPL_IEC_MRP_RC_INVALID_CONTROL_VLAN_INTERCONNECTION,                           /**< Invalid interconnection control VLAN                                       */
    VTSS_APPL_IEC_MRP_RC_INVALID_RECOVERY_PROFILE,                                       /**< Invalid recovery profile                                                   */
    VTSS_APPL_IEC_MRP_RC_RECOVERY_PROFILE_NOT_SUPPORTED_ON_THIS_SWITCH,                  /**< Recovery profile not supported on this switch                              */
    VTSS_APPL_IEC_MRP_RC_INVALID_RECOVERY_PROFILE_INTERCONNECTION,                       /**< Invalid interconnection recovery profile                                   */
    VTSS_APPL_IEC_MRP_RC_INVALID_MRM_PRIO,                                               /**< Invalid MRM prio                                                           */
    VTSS_APPL_IEC_MRP_RC_INVALID_MRA_PRIO,                                               /**< Invalid MRA prio                                                           */
    VTSS_APPL_IEC_MRP_RC_PORT_1_2_MEP_IDENTICAL,                                         /**< Port1 and Port2 MEPs cannot be the same                                    */
    VTSS_APPL_IEC_MRP_RC_PORT_1_IN_MEP_IDENTICAL,                                        /**< Port1 and interconnection MEPs cannot be the same                          */
    VTSS_APPL_IEC_MRP_RC_PORT_2_IN_MEP_IDENTICAL,                                        /**< Port2 and interconnection MEPs cannot be the same                          */
    VTSS_APPL_IEC_MRP_RC_PORT_1_2_IFINDEX_IDENTICAL,                                     /**< Port1 and Port2 interfaces cannot be the same                              */
    VTSS_APPL_IEC_MRP_RC_PORT_1_IN_IFINDEX_IDENTICAL,                                    /**< Port1 and interconnection interfaces cannot be the same                    */
    VTSS_APPL_IEC_MRP_RC_PORT_2_IN_IFINDEX_IDENTICAL,                                    /**< Port2 and interconnection interfaces cannot be the same                    */
    VTSS_APPL_IEC_MRP_RC_TWO_INST_WITH_SAME_PORT1_IFINDEX,                               /**< Another MRP instance uses same ifindex as port1                            */
    VTSS_APPL_IEC_MRP_RC_TWO_INST_WITH_SAME_PORT2_IFINDEX,                               /**< Another MRP instance uses same ifindex as port2                            */
    VTSS_APPL_IEC_MRP_RC_TWO_INST_WITH_SAME_IN_IFINDEX,                                  /**< Another MRP instance uses same ifindex as interconnection port             */
    VTSS_APPL_IEC_MRP_RC_ONLY_ONE_MIM_MIC_ON_SAME_NODE,                                  /**< Only one MIM or MIC on the same node at a time.                            */
    VTSS_APPL_IEC_MRP_RC_LIMIT_REACHED,                                                  /**< The maximum number of MRP instances is reached                             */
    VTSS_APPL_IEC_MRP_RC_OUT_OF_MEMORY,                                                  /**< Out of memory                                                              */
    VTSS_APPL_IEC_MRP_RC_NOT_READY_TRY_AGAIN_IN_A_FEW_SECONDS,                           /**< MRP is not yet ready. Try again in a few seconds                           */
    VTSS_APPL_IEC_MRP_RC_HW_RESOURCES,                                                   /**< Out of H/W resources                                                       */
};

/**
 * Enumeration of the four different "profiles" listed in Table 59, 60, 61 and
 * 62 on p. 129 and p. 130.
 */
typedef enum {
    VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_10MS,   /**<  10 msec recovery time */
    VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_30MS,   /**<  30 msec recovery time */
    VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_200MS,  /**< 200 msec recovery time */
    VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_500MS,  /**< 500 msec recovery time */
} vtss_appl_iec_mrp_recovery_profile_t;

/**
 * Capabilities of MRP.
 */
typedef struct {
    /**
     * Maximum number of configurable MRP instances.
     */
    uint32_t inst_cnt_max;

    /**
     * This is the fastest recovery profile this switch supports.
     * If for instance, the value if VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_200MS,
     * the switch supports 200ms and 500ms recovery profiles, only.
     */
    vtss_appl_iec_mrp_recovery_profile_t fastest_recovery_profile;

    /**
     * The following is false if S/W transmits MRP_Test and MRP_InTest PDUs and
     * true if H/W performs the transmission automatically.
     *
     * This is important, because if H/W performs the transmission, the Tx
     * counters for VTSS_APPL_IEC_MRP_PDU_TYPE_TEST and
     * VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TEST do not count.
     */
    mesa_bool_t hw_tx_test_pdus;
} vtss_appl_iec_mrp_capabilities_t;

/**
 * \brief Get MRP capabilities.
 *
 * \param cap [OUT] MRP capabilities
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_iec_mrp_capabilities_get(vtss_appl_iec_mrp_capabilities_t *cap);

/**
 * Enumeration of MRP roles.
 */
typedef enum {
    VTSS_APPL_IEC_MRP_ROLE_MRC, /**< This node is Media Redundancy Client  (MRC) for this ring                                 */
    VTSS_APPL_IEC_MRP_ROLE_MRM, /**< This node is Media Redundancy Manager (MRM) for this ring                                 */
    VTSS_APPL_IEC_MRP_ROLE_MRA, /**< This node is Media Redundancy Auto Manager (MRA) for this ring (not complying to Annex A) */
} vtss_appl_iec_mrp_role_t;

/**
 * Enumeration of MRP interconnect roles.
 */
typedef enum {
    VTSS_APPL_IEC_MRP_IN_ROLE_NONE, /**< This node is not part of an interconnection                */
    VTSS_APPL_IEC_MRP_IN_ROLE_MIC, /**< This node is Media redundancy Interconnection Client (MIC)  */
    VTSS_APPL_IEC_MRP_IN_ROLE_MIM, /**< This node is Media redundancy Interconnection Manager (MIM) */
} vtss_appl_iec_mrp_in_role_t;

/**
 * Enumeration of ring- and interconnection-ports
 */
typedef enum {
    VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1,     /**< Ring Port 1          */
    VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2,     /**< Ring Port 2          */
    VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION /**< Interconnection port */
} vtss_appl_iec_mrp_port_type_t;

/**
 * Signal fail can either come from the physical link on a given port or from a
 * Down-MEP.
 */
typedef enum {
    VTSS_APPL_IEC_MRP_SF_TRIGGER_LINK, /**< Signal Fail comes from the physical port's link state */
    VTSS_APPL_IEC_MRP_SF_TRIGGER_MEP,  /**< Signal Fail comes from a down-MEP                     */
} vtss_appl_iec_mrp_sf_trigger_t;

/**
 * Configuration of a particular ring- or interconnection-port.
 */
typedef struct {
    /**
     * Interface index of ring- or interconnection port.
     * Must always be filled in.
     */
    vtss_ifindex_t ifindex;

    /**
     * Selects whether Signal Fail (SF) comes from the link state of a given
     * interface, or from a Down-MEP.
     */
    vtss_appl_iec_mrp_sf_trigger_t sf_trigger;

    /**
     * Reference to a down-MEP that provides SF for the ring port that this
     * structure instance represents.
     *
     * The MEP's residence port must be the same as \p ifindex or the instance
     * will get an operational warning and use link as SF trigger until the
     * problem is remedied.
     *
     * Only used when sf_trigger is VTSS_APPL_IEC_MRP_SF_TRIGGER_MEP.
     */
    vtss_appl_cfm_mep_key_t mep;
} vtss_appl_iec_mrp_port_conf_t;

/**
 * Configuration used when ring role is MRM or MRA.
 */
typedef struct {
    /**
     * Manager Priority.
     *
     * Lower values indicate a higher priority.
     *
     * Valid and default values depend on the MRP instance's role.
     *
     * The following is from Table 30, p. 60, MRP_Prio.
     *
     * Valid values are:
     *   MRM: 0x0000, 0x1000-0x7000, 0x8000.
     *   MRA: 0x9000-0xF000, 0xFFFF
     *
     * Defaults are based on the fact that the default for \p role is MRA.
     *   MRA: 0xA000.
     */
    uint16_t prio;

    /**
     * Indicates whether the MRM reacts immediately on MRP_LinkChange PDUs or
     * not.
     *
     * Default is false.
     */
    mesa_bool_t react_on_link_change;
} vtss_appl_iec_mrp_mrm_conf_t;

/**
 * Controls the mode of an interconnection node.
 */
typedef enum  {
    VTSS_APPL_IEC_MRP_IN_MODE_LC, /**< The MIM or MIC is in Link Check mode */
    VTSS_APPL_IEC_MRP_IN_MODE_RC, /**< The MIM or MIC is in Ring Check mode */
} vtss_appl_iec_mrp_in_mode_t;

/**
 * This enumeration controls what OUI to use inside MRP_Option TLVs.
 *
 * MRP_Option TLVs are used for the following PDU types:
 *   - MRP_Test when the configured role is MRA
 *   - MRP_TestMgrNAck
 *   - MRP_TestPropagate
 */
typedef enum {
    VTSS_APPL_IEC_MRP_OUI_TYPE_DEFAULT, /**< Use the switch's own OUI   */
    VTSS_APPL_IEC_MRP_OUI_TYPE_SIEMENS, /**< Use Siemens OUI (08-00-06) */
    VTSS_APPL_IEC_MRP_OUI_TYPE_CUSTOM,  /**< Use a custom OUI           */
} vtss_appl_iec_mrp_oui_type_t;

/**
 * Configuration of one MRP instance.
 *
 * The individual parameters correspond to the attributes listed in clause 6.3.
 */
typedef struct {
    /**
     * The ring role of this instance.
     *
     * Defaults to VTSS_APPL_IEC_MRP_ROLE_MRA.
     */
    vtss_appl_iec_mrp_role_t role;

    /**
     * Domain Name
     *
     * This is an arbitrary - yet unique on this device - NULL-terminated
     * string used to help identifying the ring.
     *
     * Allowed chars is in the range [32; 126] (isprint()).
     *
     * Default is empty.
     */
    char name[241];

    /**
     * Domain ID.
     *
     * Defines the redundancy domain representing the ring this MRP instance
     * belongs to.
     *
     * It's encoded as a Universally Unique Identifier (UUID) and goes into the
     * MRP PDUs.
     *
     * Default is all-ones (FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF).
     */
    uint8_t domain_id[16];

    /**
     * MRP_Option TLVs inside certain MRP PDU types contain an OUI.
     *
     * This OUI defaults to the three first bytes of the MAC address of this.
     * switch, but can be changed to both the Siemens OUI and a custom OUI.
     *
     * The reason for changing it to the Siemens OUI is that Wireshark currently
     * cannot dissect frames with any other OUI than that, so if you are
     * debugging the protocol you might want to change to this OUI. It doesn't
     * really mean anything to how the ring works - only how Wireshark dissects
     * it.
     *
     * Default is VTSS_APPL_IEC_MRP_OUI_TYPE_DEFAULT.
     */
    vtss_appl_iec_mrp_oui_type_t oui_type;

    /**
     * OUI used when \p oui_type == VTSS_APPL_IEC_MRP_OUI_TYPE_CUSTOM.
     */
    uint8_t custom_oui[3];

    /**
     * Ring port1 and port2 configurations.
     *
     * Index 0 is for port1 (fits VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1)
     * Index 1 is for port2 (fits VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2)
     * Index 2 is for innterconnection port (first
     *         VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION)
     *
     * See vtss_appl_iec_mrp_port_conf_t for details.
     */
    vtss_appl_iec_mrp_port_conf_t ring_port[3];

    /**
     * The VLAN ID on which MRP PDUs are transmitted and received on the ring
     * ports.
     *
     * Make sure to add this VLAN ID to both ring ports, or pass-through of MRP
     * PDUs from one ring port to the other will not work.
     *
     * If the VLAN ID corresponds to the port's port VLAN ID (PVID) or it is
     * zero or the port is not a member of this VLAN ID, untagged MRP PDUs will
     * be transmitted.
     *
     * If the frames do get VLAN tagged, DEI will be set to 0 and PCP will be
     * set to 7, which is in accordance with the standard's clause 8.1.5.
     */
    mesa_vid_t vlan;

    /**
     * Recovery time profile.
     *
     * Selects the recovery time profile in accordance with the standard's Table
     * 59 (MRM) and Table 60 (MRC) on p. 129.
     *
     * These are what comes out of selecting a particular profile if the role
     * is MRM (or MRA, which eventually becomes an MRM):
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_10MS:
     *  - MRP_TOPchgT     = 0.5 msecs
     *  - MRP_TOPNRmax    = 3
     *  - MRP_TSTshortT   = 0.5 msecs
     *  - MRP_TSTdefaultT = 1 msec
     *  - MRP_TSTNRmax    = 3
     *  - MRPTSTExtNRmax  = N/A
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_30MS:
     *  - MRP_TOPchgT     = 0.5 msecs
     *  - MRP_TOPNRmax    = 3
     *  - MRP_TSTshortT   = 1 msec
     *  - MRP_TSTdefaultT = 3.5 msecs
     *  - MRP_TSTNRmax    = 3
     *  - MRPTSTExtNRmax  = N/A
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_200MS:
     *  - MRP_TOPchgT     = 10 msecs
     *  - MRP_TOPNRmax    = 3
     *  - MRP_TSTshortT   = 10 msecs
     *  - MRP_TSTdefaultT = 20 msecs
     *  - MRP_TSTNRmax    = 3
     *  - MRPTSTExtNRmax  = N/A
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_500MS:
     *  - MRP_TOPchgT     = 20 msecs
     *  - MRP_TOPNRmax    = 3
     *  - MRP_TSTshortT   = 30 msecs
     *  - MRP_TSTdefaultT = 50 msecs
     *  - MRP_TSTNRmax    = 5
     *  - MRPTSTExtNRmax  = 15
     *
     * These are what comes out of selecting a particular profile if the role
     * is MRC (or MRA, which eventually becomes an MRC):
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_10MS and
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_30MS:
     *  - MRP_LNKdownT   = 1 msec
     *  - MRP_LNKupT     = 1 msec
     *  - MRP_LNKNRmax   = 4
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_200MS and
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_500MS:
     *  - MRP_LNKdownT   = 20 msecs
     *  - MRP_LNKupT     = 20 msecs
     *  - MRP_LNKNRmax   = 4
     *
     * Default is VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_500MS
     */
    vtss_appl_iec_mrp_recovery_profile_t recovery_profile;

    /**
     * Configuration used when \p role is VTSS_APPL_IEC_MRP_ROLE_MRM or
     * VTSS_APPL_IEC_MRP_ROLE_MRA.
     *
     * See vtss_appl_iec_mrp_mrm_conf_t for details.
     */
    vtss_appl_iec_mrp_mrm_conf_t mrm;

    // Below is all interconnection related configuration (except for
    // ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION]

    /**
     * The interconnect ring role of this instance.
     *
     * Defaults to VTSS_APPL_IEC_MRP_IN_ROLE_NONE, that is, this is
     * not an interconnect node.
     *
     * If set to VTSS_APPL_IEC_MRP_IN_ROLE_NONE, the remaining
     * parameters of this struct are not used, but will be set to default.
     */
    vtss_appl_iec_mrp_in_role_t in_role;

    /**
     * Controls whether the MIM or MIC is in LC- or RC-mode.
     *
     * In LC-mode, link changes are used to figure out interconnection topology.
     * In RC-mode, MRP_InTest PDUs are used to figure out the topology.
     *
     * Default is LC-mode.
     */
    vtss_appl_iec_mrp_in_mode_t in_mode;

    /**
     * The interconnection ID used to identify this interconnection domain.
     *
     * The same ID must be used on all nodes that are part of this
     * interconnection domain.
     *
     * Default is 0.
     */
    uint16_t in_id;

    /**
     * Interconnection name
     *
     * Specifies the name of the interconnection domain. It is only used to help
     * identifying the interconnection nodes.
     *
     * Allowed chars is in the range [32; 126] (isprint()).
     *
     * Default is empty.
     */
    char in_name[241];

    /**
     * The VLAN ID on which MRP PDUs are transmitted and received on the ring
     * ports.
     *
     * Make sure to add this VLAN ID to the interconnection port.
     *
     * If \p vlan corresponds to the port's port VLAN ID (PVID) or it is zero
     * or the port is not a member of this VLAN ID, untagged MRP PDUs will
     * be transmitted.
     *
     * If the frames do get VLAN tagged, DEI will be set to 0 and PCP will be
     * set to 7, which is in accordance with the standard's clause 8.1.5.
     */
    mesa_vid_t in_vlan;

    /**
     * Interconnection recovery time profile.
     *
     * Selects the recovery time profile in accordance with the standard's Table
     * 61 (MIM) on p. 129 and Table 62 (MIC) on p. 130.
     *
     * Notice: Only the _200MS and _500MS fixed set profiles are supported for
     * the interconnection role.
     *
     * These are what comes out of selecting a particular profile if the in_role
     * is MIM:
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_10MS and
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_30MS:
     *  - Not supported
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_200MS:
     *  - MRP_IN_TOPchgT      = 10 msecs
     *  - MRP_IN_TOPNRmax     = 3
     *  - MRP_IN_TSTdefaultT  = 20 msecs
     *  - MRP_IN_TSTNRmax     = 3
     *  - MRP_IN_LNKSTATchgT  = 20 msecs
     *  - MRP_IN_LNKSTATNRmax = 8
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_500MS:
     *  - MRP_IN_TOPchgT      = 20 msecs
     *  - MRP_IN_TOPNRmax     = 3
     *  - MRP_IN_TSTdefaultT  = 50 msecs
     *  - MRP_IN_TSTNRmax     = 8
     *  - MRP_IN_LNKSTATchgT  = 20 msecs
     *  - MRP_IN_LNKSTATNRmax = 8
     *
     * These are what comes out of selecting a particular profile if the in_role
     * is MIC:
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_10MS and
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_30MS:
     *  - Not supported
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_200MS and
     * VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_500MS:
     *  - MRP_IN_LNKdownT   = 20 msecs
     *  - MRP_IN_LNKupT     = 20 msecs
     *  - MRP_IN_LNKNRmax   = 4
     *
     * Default is VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_500MS
     */
    vtss_appl_iec_mrp_recovery_profile_t in_recovery_profile;

    /**
     * The administrative state of this MRP instance (default false).
     * Set to true to make it function normally and false to make it cease
     * functioning (in which case all H/W entries are removed).
     */
    mesa_bool_t admin_active;
} vtss_appl_iec_mrp_conf_t;

/**
 * \brief Get a default MRP configuration.
 *
 * \param *conf [OUT] Default MRP configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_iec_mrp_conf_default_get(vtss_appl_iec_mrp_conf_t *conf);

/**
 * \brief Get MRPinstance configuration.
 *
 * \param instance [IN]  MRP instance
 * \param conf     [OUT] Current configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_iec_mrp_conf_get(uint32_t instance, vtss_appl_iec_mrp_conf_t *conf);

/**
 * \brief Set MRP instance configuration.
 *
 * \param instance [IN] MRP instance
 * \param conf     [IN] MRP new configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_iec_mrp_conf_set(uint32_t instance, const vtss_appl_iec_mrp_conf_t *conf);

/**
 * \brief Delete an MRP instance.
 *
 * \param instance [IN] Instance to be deleted
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_iec_mrp_conf_del(uint32_t instance);

/**
 * \brief Instance iterator.
 *
 * This function returns the next defined MRP instance number. The end is
 * reached when VTSS_RC_ERROR is returned.
 * The search for an enabled instance will start with 'prev_instance' + 1.
 * If 'prev_instance' pointer is NULL, the search start with the lowest possible
 * instance number.
 *
 * \param prev_instance [IN]  Instance number
 * \param next_instance [OUT] Next instance
 *
 * \return VTSS_RC_OK as long as next is valid.
 */
mesa_rc vtss_appl_iec_mrp_itr(const uint32_t *prev_instance, uint32_t *next_instance);

/**
 * Notifications possibly generated by an MRP instance.
 * Corresponds to the standard's clause 5.9 (Ring Diagnosis).
 */
typedef struct {
    /**
     * Indicates whether the ring is open or not.
     * Can only be raised by nodes in the MRM operating role.
     *
     * Corresponds to the standard's RING_OPEN.
     */
    mesa_bool_t ring_open;

    /**
     * Indicates whether there is more than one MRP manager on the ring.
     *
     * It is normal to have this raised shortly during manager negotiation when
     * multiple ring devices are configured as MRAs.
     *
     * If no MRP_Test PDUs have been seen for 10 seconds, this event falls back
     * to 'false'.
     *
     * When this is set, VTSS_APPL_IEC_MRP_OPER_WARNING_MULTIPLE_MRMS will be
     * set in vtss_appl_iec_mrp_status_t::oper_warnings.
     *
     * Can only be raised by nodes in the MRM operating role.
     *
     * Corresponds to the standard's MULTIPLE_MANAGERS.
     */
    mesa_bool_t multiple_mrms;

    /**
     * Indicates whether the interconnection ring is open or not.
     * Can only be raised by MIMs.
     *
     * Corresponds to the standard's INTERCONNECTION_OPEN.
     */
    mesa_bool_t in_open;

    /**
     * Indicates whether there is more than one MIM on the interconnection ring.
     * This event is only raised if the MRP_InTest PDU's Interconnection ID
     * field contains the same value as the one configured for this instance
     * (see vtss_appl_iec_mrp_conf_t::in_id).
     *
     * If no MRP_InTest PDUs have been seen for 10 seconds, this event falls
     * back to 'false'.
     *
     * When this is set, VTSS_APPL_IEC_MRP_OPER_WARNING_MULTIPLE_MIMS will be
     * set in vtss_appl_iec_mrp_status_t::oper_warnings.
     *
     * No correspoding event in the standard.
     */
    mesa_bool_t multiple_mims;
} vtss_appl_iec_mrp_notification_status_t;

/**
 * Get the notification status of an MRP instance.
 *
 * \param instance     [IN]  MRP instance.
 * \param notif_status [OUT] Pointer to structure receiving MRP instance's notification status.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_iec_mrp_notification_status_get(uint32_t instance, vtss_appl_iec_mrp_notification_status_t *const notif_status);

/**
 * MRP defines a number of different MRP PDU types. This is an enumeration of
 * these types.
 */
typedef enum {
    VTSS_APPL_IEC_MRP_PDU_TYPE_TEST,                  /**< MRP_Test   PDUs                                    */
    VTSS_APPL_IEC_MRP_PDU_TYPE_TOPOLOGY_CHANGE,       /**< MRP_TopologyChange PDUs                            */
    VTSS_APPL_IEC_MRP_PDU_TYPE_LINK_DOWN,             /**< MRP_LinkDown PDUs                                  */
    VTSS_APPL_IEC_MRP_PDU_TYPE_LINK_UP,               /**< MRP_LinkUp PDUs                                    */
    VTSS_APPL_IEC_MRP_PDU_TYPE_OPTION_TEST_MGR_NACK,  /**< MRP_Option PDUs with SubTLV type MRP_TestMgrNAck   */
    VTSS_APPL_IEC_MRP_PDU_TYPE_OPTION_TEST_PROPAGATE, /**< MRP_Option PDUs with SubTLV type MRP_TestPropagate */
    VTSS_APPL_IEC_MRP_PDU_TYPE_OPTION,                /**< MRP_Option PDUs not of the two previous types      */
    VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TEST,               /**< MRP_InTest PDUs                                    */
    VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TOPOLOGY_CHANGE,    /**< MRP_InTopologyChange PDUs                          */
    VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_DOWN,          /**< MRP_InLinkDown PDUs                                */
    VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_UP,            /**< MRP_InLinkUp PDUs                                  */
    VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_STATUS_POLL,   /**< MRP_InLinkStatusPoll PDUs                          */
    VTSS_APPL_IEC_MRP_PDU_TYPE_UNKNOWN,               /**< An MRP PDU not matching any of the above           */
    VTSS_APPL_IEC_MRP_PDU_TYPE_LAST,                  /**< Place holder to get number of different PDU types  */
} vtss_appl_iec_mrp_pdu_type_t;

/**
 * MRP per-port statistics
 */
typedef struct {
    uint64_t tx_cnt[VTSS_APPL_IEC_MRP_PDU_TYPE_LAST]; /**< Number of transmitted MRP PDUs per type                                     */
    uint64_t rx_cnt[VTSS_APPL_IEC_MRP_PDU_TYPE_LAST]; /**< Number of received MRP PDUs per type                                        */
    uint64_t rx_error_cnt;                            /**< Number of received erroneous MRP PDUs - possibly counted in rx_cnt[unknown] */
    uint64_t rx_unhandled_cnt;                        /**< Number of received unhandled MRP PDUs - also counted in \p rx_cnt           */
    uint64_t rx_own_cnt;                              /**< Number of received MRP PDUs with our own SMAC                               */
    uint64_t rx_others_cnt;                           /**< Number of received MRP PDUs with someone else's SMAC                        */
    uint64_t sf_cnt;                                  /**< Number of local signal fails                                                */
} vtss_appl_iec_mrp_statistics_t;

/**
 * Status per MRP port.
 */
typedef struct {
    /**
     * Specifies whether the port is forwarding or blocked.
     *
     * True if forwarding, false if blocked.
     */
    mesa_bool_t forwarding;

    /**
     * Indicates whether the port currently has signal fail or not.
     */
    mesa_bool_t sf;

    /**
     * Statistics. Can be cleared separately with a call to
     * vtss_appl_iec_mrp_statistics_clear().
     */
    vtss_appl_iec_mrp_statistics_t statistics;
} vtss_appl_iec_mrp_port_status_t;

/**
 * The operational state of an MRP instance.
 * There are a few ways of not having the instance active. Each of them has its
 * own enumeration. Only when the state is VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE,
 * will the MRP instance be active and up and running.
 * However, there may still be operational warnings that may cause the instance
 * not to run optimally. See vtss_appl_iec_mrp_oper_warnings_t for a list of
 * possible warnings.
 *
 * The reason for having operational warnings rather than just an operational
 * state is that we only want an MRP instance to be inactive if a true error
 * has occurred, or if the user has selected it to be inactive, because
 * if one of the warnings also resulted in making the MRP instance inactive, we
 * might get loops in our network.
 */
typedef enum {
    VTSS_APPL_IEC_MRP_OPER_STATE_ADMIN_DISABLED, /**< Instance is inactive, because it is administratively disabled. */
    VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE,         /**< The instance is active and up and running.                     */
    VTSS_APPL_IEC_MRP_OPER_STATE_INTERNAL_ERROR, /**< Instance is inactive, because an internal error has occurred.  */
} vtss_appl_iec_mrp_oper_state_t;

/**
 * Bitmask of operational warnings of an MRP instance.
 *
 * If the operational state is VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE, the MRP
 * instance is indeed active, but it may be that it doesn't run as the
 * administrator thinks, because of configuration errors, which are reflected in
 * the warnings below.
 *
 * Notice, this should have been a 64-bit enum, but not all C++ compilers
 * support that. Also, the special C++11 syntaxes:
 *     enum vtss_appl_iec_mrp_oper_warnings_t : uint64_t {
 *         ....
 *     };
 * and
 *     enum class vtss_appl_iec_mrp_oper_warnings_t : uint64_t {
 *         ....
 *     };

 * don't work.
 */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_NONE                          = 0x0000000000LLU; /**< No warnings. If oper. state is VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE, everything is fine.                   */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_RING_VLAN_PORT1 = 0x0000000001LLU; /**< Port1 is not member of the ring VLAN (which is configured for tagged operation)                           */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_RING_VLAN_PORT2 = 0x0000000002LLU; /**< Port2 is not member of the ring VLAN (which is configured for tagged operation)                           */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_VLAN_PORT1   = 0x0000000004LLU; /**< Port1 is not member of the interconnection VLAN (which is configured for tagged operation)                */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_VLAN_PORT2   = 0x0000000008LLU; /**< Port2 is not member of the interconnection VLAN (which is configured for tagged operation)                */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_VLAN_IN      = 0x0000000010LLU; /**< Interconnection port is not member of the interconnection VLAN (which is configured for tagged operation) */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_RING_PVID_PORT1 = 0x0000000020LLU; /**< Port1 is not member of its PVID (ring VLAN is configured for untagged operation)                          */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_RING_PVID_PORT2 = 0x0000000040LLU; /**< Port2 is not member of its PVID (ring VLAN is configured for untagged operation)                          */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_PVID_DIFFER_RING_PORT1_2      = 0x0000000080LLU; /**< Port1 and Port2's PVID differ (ring VLAN is configured for untagged operation)                            */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_PVID_PORT1   = 0x0000000100LLU; /**< Port1 is not member of the interconnection port's PVID (I/C VLAN is configured for untagged operation)    */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_PVID_PORT2   = 0x0000000200LLU; /**< Port2 is not member of the interconnection port's PVID (I/C VLAN is configured for untagged operation)    */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_PVID_IN      = 0x0000000400LLU; /**< Interconnection port is not member of its PVID (I/C VLAN is configured for untagged operation)            */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_UNTAGS_RING_VLAN_PORT1        = 0x0000000800LLU; /**< Port1 untags ring VLAN (which is configured for tagged operation)                                         */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_UNTAGS_RING_VLAN_PORT2        = 0x0000001000LLU; /**< Port2 untags ring VLAN (which is configured for tagged operation)                                         */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_UNTAGS_IN_VLAN_PORT1          = 0x0000002000LLU; /**< Port1 untags interconnection VLAN (which is configured for tagged operation)                              */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_UNTAGS_IN_VLAN_PORT2          = 0x0000004000LLU; /**< Port2 untags interconnection VLAN (which is configured for tagged operation)                              */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_UNTAGS_IN_VLAN_IN             = 0x0000008000LLU; /**< Interconnection port untags interconnection VLAN (which is configured for tagged operation)               */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_TAGS_RING_PVID_PORT1          = 0x0000010000LLU; /**< Port1 tags port1's PVID (which is configured for untagged operation)                                      */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_TAGS_RING_PVID_PORT2          = 0x0000020000LLU; /**< Port2 tags port2's PVID (which is configured for untagged operation)                                      */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_TAGS_IN_PVID_PORT1            = 0x0000040000LLU; /**< Port1 tags interconnection port's PVID (which is configured for untagged operation)                       */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_TAGS_IN_PVID_PORT2            = 0x0000080000LLU; /**< Port2 tags interconnection port's PVID (which is configured for untagged operation)                       */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_TAGS_IN_PVID_IN               = 0x0000100000LLU; /**< Interconnection port tags interconnection VLAN (which is configured for untagged operation)               */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_FOUND_PORT1           = 0x0000200000LLU; /**< Port1 MEP is not found. Using link-state for SF instead.                                                  */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_FOUND_PORT2           = 0x0000400000LLU; /**< Port2 MEP is not found. Using link-state for SF instead.                                                  */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_FOUND_IN              = 0x0000800000LLU; /**< Interconnection MEP is not found. Using link-state for SF instead.                                        */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_ADMIN_DISABLED_PORT1      = 0x0001000000LLU; /**< Port1 MEP is administratively disabled. Using link-state for SF instead.                                  */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_ADMIN_DISABLED_PORT2      = 0x0002000000LLU; /**< Port2 MEP is administratively disabled. Using link-state for SF instead.                                  */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_ADMIN_DISABLED_IN         = 0x0004000000LLU; /**< Interconnection MEP is administratively disabled. Using link-state for SF instead.                        */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_DOWN_MEP_PORT1        = 0x0008000000LLU; /**< Port1 MEP is not a Down-MEP. Using link-state for SF instead.                                             */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_DOWN_MEP_PORT2        = 0x0010000000LLU; /**< Port2 MEP is not a Down-MEP. Using link-state for SF instead.                                             */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_DOWN_MEP_IN           = 0x0020000000LLU; /**< Interconnection MEP is not a Down-MEP. Using link-state for SF instead.                                   */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_IFINDEX_DIFFER_PORT1      = 0x0040000000LLU; /**< Port1 MEP's residence port is not that of Port1. Using link-state for SF instead.                         */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_IFINDEX_DIFFER_PORT2      = 0x0080000000LLU; /**< Port2 MEP's residence port is not that of Port2. Using link-state for SF instead.                         */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_IFINDEX_DIFFER_IN         = 0x0100000000LLU; /**< Interconnection MEP's residence port is not that of I(/C port. Using link-state for SF instead.           */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_STP_ENABLED_PORT1             = 0x0200000000LLU; /**< Port1 has spanning tree enabled                                                                           */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_STP_ENABLED_PORT2             = 0x0400000000LLU; /**< Port2 has spanning tree enabled                                                                           */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_STP_ENABLED_IN                = 0x0800000000LLU; /**< Interconnection port has spanning tree enabled                                                            */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_MULTIPLE_MRMS                 = 0x1000000000LLU; /**< Multiple MRMs are detected on the ring. See also vtss_appl_iec_mrp_notification_status_t::multiple_mrms   */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_MULTIPLE_MIMS                 = 0x2000000000LLU; /**< Multiple MIMs are detected on the ring. See also vtss_appl_iec_mrp_notification_status_t::multiple_mims   */
const uint64_t VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR                = 0x4000000000LLU; /**< An internal error has occurred. A code update is required. Please check console for trace output.         */

/**
 * See description above the const uint64_t list above.
 */
typedef uint64_t vtss_appl_iec_mrp_oper_warnings_t;

/**
 * Enumeration of the node states.
 * These are used both for the ring as such and for a possible interconnection
 * port.
 *
 * For ring state, this corresponds to "Real Ring State" (p. 33).
 * For interconnection state, this corresponds to "Interconnection Topology
 * State" (p. 35).
 */
typedef enum {
    /**
     * If used for ring state:
     *   The MRP instance is disabled.
     *
     * If used for interconnection state:
     *   The interconnection is not in use.
     */
    VTSS_APPL_IEC_MRP_RING_STATE_DISABLED,

    /**
     * If used for ring state:
     *   Ring is open due to link or MRC failure in the ring.
     *
     * If used for interconnection state:
     *   Interconnection topology is open due to link or MIC failure in
     *   interconnection topology.
     */
    VTSS_APPL_IEC_MRP_RING_STATE_OPEN,

    /**
     * If used for ring state:
     *   Ring is closed (normal operation, no error)
     *
     * If used for interconnection state:
     *   Interconnection topology is closed (normal operation, no error).
     */
    VTSS_APPL_IEC_MRP_RING_STATE_CLOSED,

    /**
     * If used for ring state:
     *   Is used if Real Role State is MRC.
     *
     * If used for interconnection state:
     *   Device has no information (yet) about the interconnection topology.
     */
    VTSS_APPL_IEC_MRP_RING_STATE_UNDEFINED,
} vtss_appl_iec_mrp_ring_state_t;

/**
 * Contains the min/max/last round-trip times of MRP_Test or MRP_InTest PDUs.
 */
typedef struct {
    /**
     * On many platforms, round-trip time cannot be computed, because the
     * transmitted MRP_Test/MRP_InTest PDUs are not timestamped, because they
     * are sent by H/W, which doesn't always have the option of timestamping,
     * so the following timestamp round-trip times are only valid if this
     * parameter is true.
     */
    mesa_bool_t valid;

    /**
     * Minimum round-trip delay in milliseconds of own MRP_Test or MRP_InTest
     * PDUs.
     */
    uint32_t msec_min;

    /**
     * Maximum round-trip delay in milliseconds of own MRP_Test or MRP_InTest
     * PDUs.
     */
    uint32_t msec_max;

    /**
     * Latest round-trip delay in milliseconds of own MRP_Test or MRP_InTest
     * PDUs.
     */
    uint32_t msec_last;

    /**
     * Time since boot in seconds of last update of this structure.
     *
     * If it's zero, it has never been updated, so the remaining members of this
     * structure are invalid.
     */
    uint64_t last_update_secs;
} vtss_appl_iec_mrp_round_trip_statistics_t;

/**
 * Status per MRP instance
 */
typedef struct {
    /**
     * Operational state of this MRP instance.
     *
     * The MRP instance is inactive unless oper_state is
     * VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE.
     *
     * When inactive, non of the remaining members of this struct are valid.
     * When active, the MRP instance may, however, still have warnings. See
     * \p warning_state for a bitmask of possible warnings.
     */
    vtss_appl_iec_mrp_oper_state_t oper_state;

    /**
     * Operational warnings of this MRP instance.
     *
     * The MRP instance is error and warning free if \p oper_state is
     * VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE and \p oper_warning is
     * VTSS_APPL_IEC_MRP_OPER_WARNING_NONE.
     */
    vtss_appl_iec_mrp_oper_warnings_t oper_warnings;

    /**
     * Specifies current MRP instance ring state
     */
    vtss_appl_iec_mrp_ring_state_t ring_state;

    /**
     * The actual ring role of this instance.
     */
    vtss_appl_iec_mrp_role_t oper_role;

    /**
     * MRP Port status
     * Index == 0 for ring port1
     * Index == 1 for ring port2
     * Index == 2 for interconnection port
     *
     * The vtss_appl_iec_mrp_port_type_t enumeration may be used directly when
     * dereferencing this.
     */
    vtss_appl_iec_mrp_port_status_t port_status[3];

    /**
     * MRP transitions
     *
     * Number of transitions between ring open and ring closed state.
     * Corresponds to the standard's MRP_Transition for ring ports.
     */
    uint64_t transitions;

    /**
     * MRM-MRC transitions
     *
     * Number of transitions between MRM and MRC when configured role is MRA.
     * Always 0 when configured role is MRM or MRC.
     *
     * No corresponding counter in the standard.
     */
    uint64_t mrm_mrc_transitions;

    /**
     * Number of FDB flushes.
     */
    uint64_t flush_cnt;

    /**
     * Round-trip time statistics of own MRP_Test PDUs.
     * Only used if operational role (oper_role) is MRM.
     */
    vtss_appl_iec_mrp_round_trip_statistics_t round_trip_time;

    /**
     * The interconnection ring state.
     */
    vtss_appl_iec_mrp_ring_state_t in_ring_state;

    /**
     * MRP interconnection transitions
     *
     * Number of transitions between interconnection toplogy open state and
     * interconnection topology closed state.
     * Corresponds to the standard's MRP_Transition for interconnection ports.
     */
    uint64_t in_transitions;

    /**
     * Round-trip time statistics of own MRP_InTest PDUs.
     * Only used if in_role is MIM and in_mode is RC.
     */
    vtss_appl_iec_mrp_round_trip_statistics_t in_round_trip_time;
} vtss_appl_iec_mrp_status_t;

/**
 * \brief Get MRP instance status.
 *
 * \param instance [IN]  MRP instance
 * \param status   [OUT] MRP status to be updated
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_iec_mrp_status_get(uint32_t instance, vtss_appl_iec_mrp_status_t *status);

/**
 * \brief Clear MRP instance statistics.
 *
 * \param instance [IN] MRP instance
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_iec_mrp_statistics_clear(uint32_t instance);

#endif  /* _VTSS_APPL_IEC_MRP_H_ */

