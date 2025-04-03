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

/**
 * \file
 * \brief Public VCL API
 * \details This header file describes VCL public functions and types, applicable to VCL management.
 *          VCL stands for VLAN Classification List, and in simple terms, is a list of user-defined entries
 *          that describe how incoming packets should be classified (i.e. which VID they should be tagged with).
 *          The module supports three types of classifications; MAC-based, IP Subnet-based and Protocol-based.
 *          The VID tag of the incoming packet will be therefore decided based on its Source MAC address, the Subnet
 *          it belongs to or its Protocol type. Only one rule can apply at a given packet and the priority (high to low)
 *          is: 1)MAC, 2) IP Subnet, 3) Protocol and 4)Regulal Port-based VLAN rules.
 *          Finally, the user also selects the ports where the above rules will be enabled on, so a rule can be active on
 *          certain stack ports at any given time.
 */

#ifndef _VTSS_APPL_VCL_H_
#define _VTSS_APPL_VCL_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/vcap_types.h>
#include <vtss/basics/enum-descriptor.h>

/**
 * Maximum length - including terminating NULL - of a Protocol Group name.
 */
#define VTSS_APPL_VCL_MAX_GROUP_NAME_LEN 17

/**
 * Maximum length of an Organizationally Unique ID.
 */
#define VTSS_APPL_VCL_OUI_SIZE 3

/**
 * VLAN ID and Port Structure required in vtss_appl_vcl_mac_table_conf_set()
 * and vtss_appl_vcl_mac_table_conf_get() functions,
 * describing the mapping to be added or retrieved.
 * The same structure is also used by vtss_appl_vcl_ip_table_conf_set(), vtss_appl_vcl_proto_table_conf_set()
 * and vtss_appl_vcl_ip_table_conf_get(), vtss_appl_vcl_proto_table_conf_get() functions
 * in the same manner.
 */
typedef struct {
    /**
     * VLAN ID.
     *
     * The VLAN ID of the MAC/Subnet to VLAN ID mapping.
     */
    mesa_vid_t vid;

    /**
     * List of port memberships.
     *
     * The port list of the entire stack, indicating where the MAC/Subnet to VLAN ID mapping is active.
     */
    vtss_port_list_stackable_t ports;
} vtss_appl_vcl_generic_conf_global_t;

/**
 * \brief Set/add a static MAC to VLAN ID mapping (entry) into the VCL MAC table.
 * The entries are stored in a global format and the user provides a port list (that can either include one switch or multiple switches
 * in a stack), specifying where the entry will be enabled on.
 *
 * This function can be used to add a new entry, or update an existing one.
 * In any case, the user always provides the full port list (stack/switch) of the entry with the enabled ports
 * marked by '1' and the disabled ports marked by '0'.
 * So, in order to edit an existing entry in a switch/stack, a combination of vtss_appl_vcl_mac_table_conf_get() and
 * vtss_appl_vcl_mac_table_conf_set() is required so that the existing entry is retrieved before the
 * eddited entry can be added through this function. That way, the user will have the previous version of the entry available
 * and will change only the desired part of the port list, leaving the rest unaffected.
 *
 * The provided VLAN ID must be in the range 1 - 4095.
 *
 * \param mac   [IN] 6 byte MAC address.
 * \param entry [IN] Contains the VLAN ID that the MAC address will be mapped to and the port mask where this mapping will be enabled on.
 *
 * \return VTSS_RC_OK if the operation succeeded, otherwise it might return various error codes corresponding to invalid input parameters
 *  (e.g. broadcast MAC address), VCL_ERROR_ENTRY_DIFF_VID if the MAC address is already mapped to a different VID and VCL_ERROR_MAC_TABLE_FULL
 *  when the maximum number of MAC VCL entries (VCL_MAC_VCE_MAX) has been exceeded.
 **/
mesa_rc vtss_appl_vcl_mac_table_conf_set(mesa_mac_t mac, const vtss_appl_vcl_generic_conf_global_t *const entry);

/**
 * \brief Delete a configured MAC to VLAN ID mapping (entry) from the VCL MAC table.
 * The entries are stored in a global format, therefore the entire entry will be deleted when this function is called.
 *
 * If the mapping does not exist in the VCL module, then the operation will fail.
 *
 * \param mac  [IN] 6 byte MAC address - Since the mappings are unique, the address alone is enough to identify the mapping to be deleted.
 *
 * \return VTSS_RC_OK if the operation succeeded, otherwise it will return various error codes corresponding to invalid input parameters
 *  (e.g. broadcast MAC address).
 **/
mesa_rc vtss_appl_vcl_mac_table_conf_del(mesa_mac_t mac);

/**
 * \brief Get a MAC to VLAN ID mapping (entry) (added through vtss_appl_vcl_mac_table_conf_set()) from the VCL MAC table.
 * If the mapping does not exist in the VCL module, then the operation will fail.
 *
 * Since the mappings of this module are global, the returned entry will also contain all information stored in the stack
 * and is up to the user to select the portion he sees fit (e.g. the local port list of a single switch).
 *
 * Furthermore, the user can either use the iterator function in order to select a entry from the table or directly call
 * this function with a proper MAC address and retrieve that entry.
 *
 * When searching for an entry with the iterator (the MAC address of the entry is thus provided), then this function will
 * always be successful and return VTSS_RC_OK.
 *
 * \param mac   [IN]  6 byte MAC address - Since the mappings are unique, the address alone is enough to identify the mapping to be retrieved.
 *
 * \param entry [OUT] The retrieved entry.
 *
 * \return VTSS_RC_OK if the operation succeeded, VCL_ERROR_ENTRY_NOT_FOUND is the entry is not present in the stack/switch
 * or some other error code depending on the invalid input parameter.
 **/
mesa_rc vtss_appl_vcl_mac_table_conf_get(mesa_mac_t mac, vtss_appl_vcl_generic_conf_global_t *entry);

/**
 * \brief Iterate function of the VCL MAC table.
 * This function will iterate through all MAC to VLAN ID mappings (entries)
 * found in the table.
 *
 * Use the NULL pointer as an input in order to get the MAC address of the first entry,
 * otherwise the iterator will provide the MAC of the next entry.
 *
 * \param prev_mac [IN]  previous MAC.
 * \param next_mac [OUT] next MAC.
 *
 * \return VTSS_RC_OK if the next MAC address was found, otherwise VCL_ERROR_ENTRY_NOT_FOUND is
 * returned to signal the end of the VCL MAC table.
 **/
mesa_rc vtss_appl_vcl_mac_table_conf_itr(const mesa_mac_t *const prev_mac, mesa_mac_t *const next_mac);

/**
 * \brief Function for setting the <key, value> pair to defaults.
 * This is way to fill the RowEditor with values that are within
 * the expected range (e.g. VlanId between 1 - 4095). Without this,
 * fields are set to zero which is outside the expected range.
 * Since there are no real defaults for the <key, value> pair, the
 * function simply sets these fields to '1', which is within
 * the expected range.
 *
 * \param mac   [OUT] 6 byte MAC address.
 * \param entry [OUT] Contains the VLAN ID that the MAC address will be mapped to and the port mask where this mapping will be enabled on.
 *
 * \return VTSS_RC_OK all times, since this is a simple value assignment.
 **/
mesa_rc vtss_appl_vcl_mac_table_conf_def(mesa_mac_t *mac, vtss_appl_vcl_generic_conf_global_t *entry);

/**
 * \brief Set/add a static IP Subnet to VLAN ID mapping (entry) into the VCL IP table.
 * The entries are stored in a global format and the user provides a port list (that can either include one switch or multiple switches
 * in a stack), specifying where the entry will be enabled on.
 *
 * This function can be used to add a new entry, or update an existing one.
 * In any case, the user always provides the full port list (stack/switch) of the entry with the enabled ports
 * marked by '1' and the disabled ports marked by '0'.
 * So, in order to edit an existing entry in a switch/stack, a combination of vtss_appl_vcl_ip_table_conf_get() and
 * vtss_appl_vcl_ip_table_conf_set() is required so that the existing entry is retrieved before the
 * eddited entry can be added through this function. That way, the user will have the previous version of the entry available
 * and will change only the desired part of the port list, leaving the rest unaffected.
 *
 * The user can either specify the IP Subnet directly or through the address of one of the subnet's hosts.
 * E.g. 192.168.1.1/24 will be automatically converted into 192.168.1.0/24.
 *
 * The provided VLAN ID must be in the range 1 - 4095.
 *
 * \param subnet   [IN] Subnet IP Address and Mask Length.
 * \param entry    [IN] Contains the VLAN ID that the IP Subnet will be mapped to and the port mask where this mapping will be enabled on.
 *
 * \return VTSS_RC_OK if the operation succeeded, otherwise it might return various error codes corresponding to invalid input parameters
 * (e.g. mask length 33), VCL_ERROR_ENTRY_DIFF_VID if the IP Subnet is already mapped to a different VID and VCL_ERROR_IP_TABLE_FULL
 * when the maximum number of IP Subnet VCL entries (VCL_IP_VCE_MAX) has been exceeded.
 **/
mesa_rc vtss_appl_vcl_ip_table_conf_set(mesa_ipv4_network_t subnet, const vtss_appl_vcl_generic_conf_global_t *const entry);

/**
 * \brief Delete a configured IP Subnet to VLAN ID mapping (entry) from the VCL IP table.
 * The entries are stored in a global format, therefore the entire entry will be deleted when this function is called.
 *
 * If the mapping does not exist in the VCL module, then the operation will fail.
 *
 * \param subnet [IN] Subnet IP Address and Mask Length - Since the mappings are unique, the IP Subnet alone is enough
 * to identify the mapping to be deleted.
 *
 * \return VTSS_RC_OK if the operation succeeded, otherwise it will return various error codes corresponding to invalid input parameters
 * (e.g. invalid mask length).
 **/
mesa_rc vtss_appl_vcl_ip_table_conf_del(mesa_ipv4_network_t subnet);

/**
 * \brief Get an IP Subnet to VLAN ID mapping (entry) (added through vtss_appl_vcl_ip_table_conf_set()) from the VCL IP table.
 * If the mapping does not exist in the VCL module, then the operation will fail.
 *
 * Since the mappings of this module are global, the returned entry will also contain all information stored in the stack
 * and is up to the user to select the portion he sees fit (e.g. the local port list of a single switch).
 *
 * Furthermore, the user can either use the iterator function in order to select a entry from the table or directly call
 * this function with a proper Subnet address and retrieve that entry.
 *
 * When searching for an entry with the iterator (the Subnet address of the entry is thus provided), then this function will
 * always be successful and return VTSS_RC_OK.
 *
 * \param subnet [IN]  Subnet IP Address and Mask Length - Since the mappings are unique, the IP Subnet alone is enough
 * to identify the mapping to be retrieved.
 *
 * \param entry  [OUT] The retrieved entry.
 *
 * \return VTSS_RC_OK if the operation succeeded, VCL_ERROR_ENTRY_NOT_FOUND is the entry is not present in the stack/switch (only when
 * searching without using the iterator) or some other error code depending on the invalid input parameter.
 **/
mesa_rc vtss_appl_vcl_ip_table_conf_get(mesa_ipv4_network_t subnet, vtss_appl_vcl_generic_conf_global_t *entry);

/**
 * \brief Iterate function of the VCL IP table.
 * This function will iterate through all IP Subnet to VLAN ID mappings (entries)
 * found in the table.
 *
 * Use the NULL pointer as an input in order to get the IP Subnet of the first entry,
 * otherwise the iterator will provide the IP Subnet of the next entry.
 *
 * \param prev_subnet [IN]  previous Subnet IP Address and Mask Length.
 *
 * \param next_subnet [OUT] next Subnet IP Address and Mask Length.
 *
 * \return VTSS_RC_OK if the next IP Subnet was found, otherwise VCL_ERROR_ENTRY_NOT_FOUND is
 * returned to signal the end of the VCL IP table.
 **/
mesa_rc vtss_appl_vcl_ip_table_conf_itr(const mesa_ipv4_network_t *const prev_subnet, mesa_ipv4_network_t *const next_subnet);

/**
 * \brief Function for setting the <key, value> pair to defaults.
 * This is way to fill the RowEditor with values that are within
 * the expected range (e.g. VlanId between 1 - 4095). Without this,
 * fields are set to zero which is outside the expected range.
 * Since there are no real defaults for the <key, value> pair, the
 * function simply sets these fields to '1', which is within
 * the expected range.
 *
 * \param subnet [OUT] Subnet IP Address and Mask Length.
 * \param entry  [OUT] Contains the VLAN ID that the MAC address will be mapped to and the port mask where this mapping will be enabled on.
 *
 * \return VTSS_RC_OK all times, since this is a simple value assignment.
 **/
mesa_rc vtss_appl_vcl_ip_table_conf_def(mesa_ipv4_network_t *subnet, vtss_appl_vcl_generic_conf_global_t *entry);

/**
 * Enumeration of all three Frame Types used by the Protocol-based VCL part
 */
typedef enum {
    /**
     * Ethernet II encapsulation
     */
    VTSS_APPL_VCL_PROTO_ENCAP_ETH2 = 1,

    /**
     * LLC SNAP encapsulation
     */
    VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP,

    /**
     * LLC encapsulations other than SNAP
     */
    VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER
} vtss_appl_vcl_proto_encap_type_t;

/**
 * Protocol field structure for ETH2 encapsulation type
 **/
typedef struct {
    /**
     * Ethernet Protocol Type.
     **/
    uint16_t eth_type;
} vtss_appl_vcl_proto_eth2_t;

/**
 * Protocol field structure for LLC SNAP encapsulation type
 **/
typedef struct {
    /**
     * Organizationally Unique ID.
     */
    uint8_t  oui[VTSS_APPL_VCL_OUI_SIZE];

    /**
     * Protocol ID.
     */
    uint16_t pid;
} vtss_appl_vcl_proto_llc_snap_t;

/**
 * Protocol field structure for other LLC encapsulation types
 **/
typedef struct {
    /**
     * Destination Service Access Point
     */
    uint8_t dsap;

    /**
     * Source Service Access Point
     */
    uint8_t ssap;
} vtss_appl_vcl_proto_llc_other_t;

/**
 * Union of all three possible Protocol field types supported by the Protocol-based VCL part
 */
typedef union {
    /**
     * The protocol field of the Ethernet II encapsulation type
     */
    vtss_appl_vcl_proto_eth2_t eth2_proto;

    /**
     * The protocol fields of the LLC SNAP encapsulation type
     */
    vtss_appl_vcl_proto_llc_snap_t  llc_snap_proto;

    /**
     * The protocol fields of all other LLC encapsulation types other than SNAP
     */
    vtss_appl_vcl_proto_llc_other_t llc_other_proto;
} vtss_appl_vcl_proto_encap_t;

/**
 * Protocol structure including the protocol encapsulation type and the respective protocol field
 */
typedef struct {
    /**
     * Protocol Encapsulation Type of the Protocol to Protocol Group mapping.
     */
    vtss_appl_vcl_proto_encap_type_t proto_encap_type;

    /**
     * Protocol field of the Protocol to Protocol Group mapping.
     */
    vtss_appl_vcl_proto_encap_t proto;
} vtss_appl_vcl_proto_t;

/**
 * Protocol to Protocol Group Structure required in vtss_appl_vcl_proto_table_proto_set()
 * and vtss_appl_vcl_proto_table_proto_get() functions,
 * describing the mapping to be added or retrieved.
 */
typedef struct {
    /**
     * Protocol Group Name of the Protocol to Protocol Group mapping.
     */
    uint8_t name[VTSS_APPL_VCL_MAX_GROUP_NAME_LEN];
} vtss_appl_vcl_proto_group_conf_proto_t;

/**
 * \brief Set/add a static Protocol to Protocol Group mapping (entry) into the VCL Protocol table.
 * These entries are stored only in the primary switch of the stack.
 *
 * This function can only be used to add a new entry in the stack, provided that the specified protocol is not
 * already part of a protocol group (unique mappings).
 *
 * The provided Protocol field value must be in the range 0x600 - 0xffff (Ethernet II), 0x0 - 0xffffff (LLC - SNAP)
 * or 0x0 - 0xff (LLC - Other).
 *
 * \param protocol [IN] Protocol Encapsulation Type and Protocol Field value.
 * \param entry    [IN] Contains the protocol group name of the mapping.
 *
 * \return VTSS_RC_OK if the operation succeeded, otherwise it might return various error codes corresponding to invalid input parameters
 * (e.g. invalid group name), VCL_ERROR_PROTOCOL_ALREADY_CONF if the specified protocol is already mapped to a group name
 * and VCL_ERROR_GROUP_PROTO_TABLE_FULL when the maximum number of Protocol VCL entries (VCL_PROTO_PROTOCOL_MAX) has been exceeded.
 **/
mesa_rc vtss_appl_vcl_proto_table_proto_set(vtss_appl_vcl_proto_t protocol, const vtss_appl_vcl_proto_group_conf_proto_t *const entry);

/**
 * \brief Delete a static Protocol to Protocol Group mapping (entry) from the VCL Protocol table.
 * These entries are stored only in the primary switch of the stack.
 *
 * This function completely deletes the protocol from its protocol group and this will be applied to all configuarations in the stack.
 *
 * The provided Protocol field value must be in the range 0x600 - 0xffff.
 *
 * \param protocol [IN] Protocol Encapsulation Type and Protocol Field value.
 *
 * \return VTSS_RC_OK if the operation succeeded, otherwise it might return various error codes corresponding to invalid input parameters
 * (e.g. invalid group name).
 **/
mesa_rc vtss_appl_vcl_proto_table_proto_del(vtss_appl_vcl_proto_t protocol);

/**
 * \brief Get a Protocol to Protocol Group mapping (entry) (added through vtss_appl_vcl_proto_table_proto_set()) from the VCL Protocol table.
 * If the mapping does not exist in the VCL module, then the operation will fail.
 *
 * These entries are stored only in the primary switch of the stack.
 *
 * \param protocol [IN]  Protocol Encapsulation Type and Protocol Field value.
 *
 * \param entry    [OUT] The retrieved entry.
 *
 * \return VTSS_RC_OK if the operation succeeded, VCL_ERROR_ENTRY_NOT_FOUND is the entry is not present in the stack (only when
 * searching without using the iterator) or some other error code depending on the invalid input parameter.
 **/
mesa_rc vtss_appl_vcl_proto_table_proto_get(vtss_appl_vcl_proto_t protocol, vtss_appl_vcl_proto_group_conf_proto_t *entry);

/**
 * \brief Iterate function of the VCL Protocol table.
 * This function will iterate through all Protocol to Protocol Group mappings (entries)
 * found in the table.
 *
 * These entries are stored only in the primary switch of the stack.
 *
 * Use the NULL pointer as an input in order to get the Protocol of the first entry,
 * otherwise the iterator will provide the Protocol of the next entry.
 *
 * \param prev_protocol [IN]  previous Protocol Encapsulation Type and Protocol Field value.
 *
 * \param next_protocol [OUT] next Protocol Encapsulation Type and Protocol Field value.
 *
 * \return VTSS_RC_OK if the next Protocol was found, otherwise VCL_ERROR_ENTRY_NOT_FOUND is
 * returned to signal the end of the VCL Protocol table.
 **/
mesa_rc vtss_appl_vcl_proto_table_proto_itr(const vtss_appl_vcl_proto_t *const prev_protocol, vtss_appl_vcl_proto_t *const next_protocol);

/**
 * \brief Function for setting the <key, value> pair to defaults.
 * This is way to fill the RowEditor with values that are within
 * the expected range (e.g. ProtocolGroupName: 'default'). Without this,
 * fields are set to zero which is outside the expected range.
 * Since there are no real defaults for the <key, value> pair, the
 * function simply sets the protocol to IP and the group name to 'default'.
 *
 * \param protocol [OUT]  Protocol Encapsulation Type and Protocol Field value.
 * \param entry [OUT] Contains the Protocol Group name where this protocol will be mapped to.
 *
 * \return VTSS_RC_OK all times, since this is a simple value assignment.
 **/
mesa_rc vtss_appl_vcl_proto_table_proto_def(vtss_appl_vcl_proto_t *protocol, vtss_appl_vcl_proto_group_conf_proto_t *entry);

/**
 * \brief Set/add a static Protocol Group to VLAN ID mapping (entry) into the VCL Protocol Group table.
 * The entries are stored in a global format and the user provides a port list (that can either include one switch or multiple switches
 * in a stack), specifying where the entry will be enabled on.
 *
 * This function can only be used to add a new entry.
 * So, in order to edit an existing entry in a switch/stack, a combination of vtss_appl_vcl_proto_table_conf_get() and
 * vtss_appl_vcl_proto_table_conf_del() is required so that the existing entry is saved temporarily before being deleted
 * and then the new/eddited entry can be added through this function.
 *
 * The provided Protocol Group name must be maximum 16 characters long.
 *
 * The provided VLAN ID must be in the range 1 - 4095.
 *
 * \param group [IN] Protocol Group name.
 * \param entry [IN] Contains the port mask where this mapping will be enabled on.
 *
 * \return VTSS_RC_OK if the operation succeeded, otherwise it might return various error codes corresponding to invalid input parameters
 * (e.g. invalid group name), VCL_ERROR_ENTRY_OVERLAPPING_PORTS if the Protocol Group is already mapped with a different VID to a
 * set of ports that is overlapping with this one and VCL_ERROR_GROUP_ENTRY_TABLE_FULL when the maximum number of
 * Protocol to VLAN ID entries (VCL_PROTO_VCE_MAX) has been exceeded.
 **/
mesa_rc vtss_appl_vcl_proto_table_conf_set(vtss_appl_vcl_proto_group_conf_proto_t group, const vtss_appl_vcl_generic_conf_global_t *const entry);

/**
 * \brief Delete a configured Protocol Group to VLAN ID mapping (entry) from the VCL Protocol Group table.
 * The entries are stored in a global format, therefore the entire entry will be deleted when this function is called.
 *
 * If the mapping does not exist in the VCL module, then the operation will fail.
 *
 * \param group [IN] Protocol Group name.
 *
 * \return VTSS_RC_OK if the operation succeeded, otherwise it will return various error codes corresponding to invalid input parameters
 *  (e.g. invalid group name).
 **/
mesa_rc vtss_appl_vcl_proto_table_conf_del(vtss_appl_vcl_proto_group_conf_proto_t group);

/**
 * \brief Get a Protocol Group to VLAN ID mapping (entry) (added through vtss_appl_vcl_proto_table_conf_set()) from the VCL Protocol Group table.
 * If the mapping does not exist in the VCL module, then the operation will fail.
 *
 * Since the mappings of this module are global, the returned entry will also contain all information stored in the stack
 * and is up to the user to select the portion he sees fit (e.g. the local port list of a single switch).
 *
 * Furthermore, the user can either use the iterator function in order to select a entry from the table or directly call
 * this function with a proper Protocol Group name and retrieve that entry.
 *
 * When searching for an entry with the iterator (the Protocol Group name of the entry is thus provided), then this function will
 * always be successful and return VTSS_RC_OK.
 *
 * \param group [IN]  Protocol Group name.
 *
 * \param entry [OUT] The retrieved entry.
 *
 * \return VTSS_RC_OK if the operation succeeded, VCL_ERROR_ENTRY_NOT_FOUND is the entry is not present in the stack/switch (only when
 * searching without using the iterator) or some other error code depending on the invalid input parameter.
 **/
mesa_rc vtss_appl_vcl_proto_table_conf_get(vtss_appl_vcl_proto_group_conf_proto_t group, vtss_appl_vcl_generic_conf_global_t *entry);

/**
 * \brief Iterate function of the VCL Protocol Group table.
 * This function will iterate through all Protocol Group to VLAN ID mappings (entries)
 * found in the table.
 *
 * Use the NULL pointer as an input in order to get the Protocol Group and VID of the first entry,
 * otherwise the iterator will provide the Protocol Group and VID of the next entry.
 *
 * \param prev_group [IN]  previous Protocol Group name.
 *
 * \param next_group [OUT] next Protocol Group name.
 *
 * \return VTSS_RC_OK if the next Protocol Group and VID were found, otherwise VCL_ERROR_ENTRY_NOT_FOUND is
 * returned to signal the end of the VCL Protocol Group table.
 **/
mesa_rc vtss_appl_vcl_proto_table_conf_itr(const vtss_appl_vcl_proto_group_conf_proto_t *const prev_group, vtss_appl_vcl_proto_group_conf_proto_t *const next_group);

/**
 * \brief Function for setting the <key, value> pair to defaults.
 * This is way to fill the RowEditor with values that are within
 * the expected range (e.g. VlanId between 1 - 4095). Without this,
 * fields are set to zero which is outside the expected range.
 * Since there are no real defaults for the <key, value> pair, the
 * function simply sets these fields to '1', which is within
 * the expected range.
 *
 * \param group [OUT] Protocol Group name.
 * \param entry [OUT] Contains the port mask where this mapping will be enabled on.
 *
 * \return VTSS_RC_OK all times, since this is a simple value assignment.
 **/
mesa_rc vtss_appl_vcl_proto_table_conf_def(vtss_appl_vcl_proto_group_conf_proto_t *group, vtss_appl_vcl_generic_conf_global_t *entry);

#endif  // _VTSS_APPL_VCL_H_

