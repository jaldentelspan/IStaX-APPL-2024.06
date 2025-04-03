/*

 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 * \file
 * \brief Public VLAN TRANSLATION API
 * \details This header file describes VLAN translation public functions and types, applicable to VLAN translation management.
 *          This module is responsible for creating VLAN translation mappings and configuring switch interfaces to use these
 *          mappings. VLAN translation is the act of modifying the VLAN tag of an incoming or outgoing packet according to a set
 *          rule, thus 'translating' the VLAN ID of the tag. A VLAN translation rule x->y is applied to incoming packets by replacing
 *          VID x with VID y, and in a reverse manner for outgoing packets, by replacing VID y with VID x (translation works in both
 *          directions). VLAN translations like the above are grouped together in VLAN translation groups identified by a Group ID (gid).
 *          Therefore this module keeps a table of VLAN translation mappings that include the Group ID, the source VID (vid) and the
 *          translated VID (tvid). Then, when a user wants to enable a set (group) of translation rules, it does so by configuring an
 *          interface to use a certain group (gid). Each interface can be mapped to only one group, and this will activate all translations
 *          stored within the group for that interface.
 */

#ifndef _VTSS_APPL_VLAN_TRANSLATION_H_
#define _VTSS_APPL_VLAN_TRANSLATION_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>
#include <vtss/basics/enum-descriptor.h>

extern const vtss_enum_descriptor_t mesa_vlan_trans_dir_txt[]; /**< Enum descriptor text */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief A collections of capability properties of the VLAN TRANSLATION module.
 **/
typedef struct {
    /**
     * Maximum number of translations.
     *
     * This is the maximum number of VLAN translation mappings the user can store in the VLAN Translation mapping table.
     **/
    uint32_t max_number_of_translations;
} vtss_appl_vlan_translation_capabilities_t;

/**
 * Group ID and VLAN ID Structure required in vtss_appl_vlan_translation_group_conf_set(),
 * vtss_appl_vlan_translation_group_conf_del() and vtss_appl_vlan_translation_group_conf_get() functions,
 * acting as the key of the mapping to be added, deleted or retrieved.
 * The same structure is also used by vtss_appl_vlan_translation_group_conf_itr() function
 * used for iterating between VLAN translation mappings.
 **/
typedef struct {
    /**
     * Group ID.
     *
     * The Group ID of the VLAN translation mapping key.
     */
    uint16_t gid;

    /**
     * Translation Direction.
     *
     * The Translation Direction of the VLAN Translation mapping key.
     */
    mesa_vlan_trans_dir_t dir;

    /**
     * VLAN ID.
     *
     * The VLAN ID of the VLAN translation mapping key.
     */
    mesa_vid_t vid;
} vtss_appl_vlan_translation_group_mapping_key_t;

/**
 * Group ID Structure required in  vtss_appl_vlan_translation_if_conf_set()
 * and  vtss_appl_vlan_translation_if_conf_get() functions,
 * acting as the value of the configuration to be set, or retrieved.
 **/
typedef struct {
    /**
     * Group ID.
     *
     * The Group ID of the group that the interface is  configured to use.
     */
    uint16_t gid;
} vtss_appl_vlan_translation_if_conf_value_t;

/* Global configuration ---------------------------------------------------- */

/**
 * \brief Get the capabilities of the device.
 *
 * \param c [OUT] Buffer to receive the result in.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_vlan_translation_global_capabilities_get(vtss_appl_vlan_translation_capabilities_t *c);

/**
 * \brief Set/add a static VLAN translation mapping (entry) into the VLAN translation mapping table.
 *
 * This function can be used to add a new entry, or update an existing one (i.e. change the translated VLAN ID).
 * In any case, the user always provides the key of the mapping, which is the Group ID and the source VLAN ID,
 * and the translated VLAN ID.
 *
 * The provided Group ID must be in the range 1 - max number of VLAN translation groups, which is equal to the
 * total number of interfaces of the switch. This is a result of the limitation that each interface can be configured
 * to use only one VLAN translation group at a given time.
 * The provided source and translated VLAN IDs must be in the range 1 - 4095 and should not match, otherwise they will be rejected
 * (makes no sense to translate to the existing VLAN ID).
 *
 * \param mapping [IN] Group ID and source VLAN ID of the mapping.
 * \param tvid    [IN] Contains the VLAN ID that the source VLAN ID will be translated to.
 *
 * \return VTSS_RC_OK if the operation succeeded, otherwise it might return various error codes corresponding to invalid input parameters
 * (e.g. translated VID is the same as the provided source VID), or indicating that the maximum number of VLAN translation mapping entries
 * (max_number_of_translations) has been exceeded.
 **/
mesa_rc vtss_appl_vlan_translation_group_conf_set(vtss_appl_vlan_translation_group_mapping_key_t mapping,
                                                  const mesa_vid_t *const tvid);

/**
 * \brief Delete a static VLAN translation mapping (entry) from the VLAN translation mapping table.
 *
 * This function completely deletes the VLAN translation mapping (VID->TVID) from its VLAN translation group
 * and this will be applied to all active configurations in the switch.
 *
 * Since the mappings are unique, only the VLAN translation mapping key is required to identify the entry.
 *
 * \param mapping [IN] Group ID and source VLAN ID of the mapping.
 * 
 * \return VTSS_RC_OK if the operation succeeded, otherwise it might return various error codes corresponding to invalid input parameters
 * (e.g. invalid Group ID or source VLAN ID).
 **/
mesa_rc vtss_appl_vlan_translation_group_conf_del(vtss_appl_vlan_translation_group_mapping_key_t mapping);

/**
 * \brief Get a VLAN translation mapping (entry) from the VLAN translation mapping table.
 * If the mapping does not exist in the VLAN translation module, then the operation will fail.
 *
 * \param mapping [IN]  Group ID and source VLAN ID of the mapping.
 *
 * \param tvid    [OUT] The retrieved translated VLAN ID of the mapping.
 *
 * \return VTSS_RC_OK if the operation succeeded, VT_ERROR_ENTRY_NOT_FOUND if the entry is not present in the switch (only when
 * searching without using the iterator) or some other error code depending on the invalid input parameter.
 **/
mesa_rc vtss_appl_vlan_translation_group_conf_get(vtss_appl_vlan_translation_group_mapping_key_t mapping,
                                                  mesa_vid_t *tvid);

/**
 * \brief Iterator function of the VLAN translation mapping table.
 * This function will iterate through all VLAN translation mappings (entries)
 * found in the table.
 *
 * Use the NULL pointer as an input in order to get the key of the first entry,
 * otherwise the iterator will provide the key of the next entry (if there is one).
 *
 * \param prev_mapping [IN]  Group ID and source VLAN ID of the previous mapping.
 *
 * \param next_mapping [OUT] Group ID and source VLAN ID of the next mapping.
 *
 * \return VTSS_RC_OK if the next mapping key was found, otherwise VT_ERROR_ENTRY_NOT_FOUND is
 * returned to signal the end of the VLAN translation mapping table.
 **/
mesa_rc vtss_appl_vlan_translation_group_conf_itr(const vtss_appl_vlan_translation_group_mapping_key_t *const prev_mapping,
                                                  vtss_appl_vlan_translation_group_mapping_key_t *const next_mapping);

/**
 * \brief Function for setting the <key, value> pair to defaults.
 * This is way to fill the RowEditor with values that are within
 * the expected range (e.g. VlanId between 1 - 4095). Without this,
 * fields are set to zero which is outside the expected range.
 * Since there are no real defaults for the <key, value> pair, the
 * function simply sets all of their fields to '1', which is within
 * the expected range.
 *
 * \param mapping [OUT] Group ID and source VLAN ID of the mapping.
 * \param tvid    [OUT] The translated VLAN ID of the mapping.
 *
 * \return VTSS_RC_OK all times, since this is a simple value assignment.
 **/
mesa_rc vtss_appl_vlan_translation_group_conf_def(vtss_appl_vlan_translation_group_mapping_key_t *mapping, mesa_vid_t *tvid);

/* Interface functions ----------------------------------------------------- */

/**
 * \brief Set an interface to use a certain set of VLAN translation mappings, declared in a given VLAN translation Group.
 *
 * The provided interface ID must of course be a valid interface of the switch, and the provided Group ID must be in the
 * range 1 - max number of VLAN translation groups, which is equal to the total number of interfaces of the switch.
 *
 * If there are VLAN translation mappings stored in the provided VLAN translation Group, then these will be activated
 * on the specified interface, otherwise they will become active as soon as they are added (the interface to Group ID mapping is kept).
 *
 * The user can set the interface to use the default VLAN translation Group by providing the corresponding Group ID;
 * i.e. the interface ID, so by default switch port #1 is set to use mappings that belong to Group ID #1.
 *
 * \param ifindex [IN] The ID of the interface that is being configured.
 * \param group   [IN] Group ID that identifies the set of VLAN translation mappings that will be activated on the above interface.
 *
 * \return VTSS_RC_OK if the operation succeeded, otherwise it might return various error codes corresponding to invalid input parameters
 *  (e.g. non-existing interface or invalid Group ID).
 **/
mesa_rc vtss_appl_vlan_translation_if_conf_set(vtss_ifindex_t ifindex,
                                               const vtss_appl_vlan_translation_if_conf_value_t *const group);

/**
 * \brief Get an interface configuration, i.e. which VLAN translation Group the interface is set to use.
 *
 * Furthermore, the user can either use the iterator function in order to select a valid interface from the switch or directly call
 * this function with a proper interface ID and retrieve its configuration.
 *
 * When searching for an interface with the iterator (the interface ID is thus provided), then this function will
 * always be successful and return VTSS_RC_OK.
 *
 * \param ifindex [IN]  The ID of the interface that is requested.
 *
 * \param group   [OUT] Group ID that identifies the set of VLAN translation mappings that will be activated on the above interface.
 *
 * \return VTSS_RC_OK if the operation succeeded, or some error code depending on the invalid input parameter.
 **/
mesa_rc vtss_appl_vlan_translation_if_conf_get(vtss_ifindex_t ifindex,
                                               vtss_appl_vlan_translation_if_conf_value_t *group);

/**
 * \brief Iterator function of the VLAN translation interface configuration.
 * This function will iterate through all interfaces available in the switch so they can be used to fetch their configuration.
 *
 * Use the NULL pointer as an input in order to get the index of the first available interface,
 * otherwise the iterator will provide the index of the next interface.
 *
 * \param prev_ifindex [IN]  ID of the previous switch interface.
 *
 * \param next_ifindex [OUT] ID of the next switch inteface.
 *
 * \return VTSS_RC_OK if the next interface ID was found, otherwise VT_ERROR_ENTRY_NOT_FOUND is
 * returned to signal the end of the interface list.
 **/
mesa_rc vtss_appl_vlan_translation_if_conf_itr(const vtss_ifindex_t *const prev_ifindex,
                                               vtss_ifindex_t *const next_ifindex);

#ifdef __cplusplus
}
#endif
#endif  // _VTSS_APPL_VLAN_TRANSLATION_H_
