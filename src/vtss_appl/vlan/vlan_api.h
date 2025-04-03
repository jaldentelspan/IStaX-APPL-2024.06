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

#ifndef _VLAN_API_H_
#define _VLAN_API_H_

#include "vtss/appl/vlan.h" /**< Our public header file */

// Shared VLAN Learning supported if capability MESA_CAP_L2_SVL_FID_CNT is non-zero
// FIDs are numbered [1; FID_CNT]
#define VTSS_APPL_VLAN_FID_CNT fast_cap(MESA_CAP_L2_SVL_FID_CNT)

/**
 * Maximum number of VLANs that can be created in the system.
 * The number does not restrict the VLAN IDs that can be
 * created - only the number of VLANs. Changing the number
 * of VLANs that can be created affects the memory usage.
 * See VTSS_APPL_VLAN_ID_MIN and VTSS_APPL_VLAN_ID_MAX for VLAN ID range.
 * By default, we create the number of VLANs required
 * by the VLAN ID range.
 */
#define VLAN_ENTRY_CNT (VTSS_APPL_VLAN_ID_MAX - VTSS_APPL_VLAN_ID_MIN + 1)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * VLAN module error codes (mesa_rc)
 */
enum {
    VLAN_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_VLAN), /**< Generic error code                               */
    VLAN_ERROR_ISID,                                          /**< Invalid ISID                                     */
    VLAN_ERROR_PORT,                                          /**< Invalid port number                              */
    VLAN_ERROR_IFINDEX,                                       /**< Invalid Interface Index                          */
    VLAN_ERROR_MUST_BE_PRIMARY_SWITCH,                        /**< Operation only valid on primary switch           */
    VLAN_ERROR_NOT_CONFIGURABLE,                              /**< Switch not configurable                          */
    VLAN_ERROR_USER,                                          /**< Invalid user                                     */
    VLAN_ERROR_VID,                                           /**< Invalid VLAN ID                                  */
    VLAN_ERROR_FID,                                           /**< Invalid Filter ID                                */
    VLAN_ERROR_ALLOWED_HYBRID_VID,                            /**< Invalid VLAN ID selected in allowed hybrid VLANs */
    VLAN_ERROR_ALLOWED_TRUNK_VID,                             /**< Invalid VLAN ID selected in allowed trunk VLANs  */
    VLAN_ERROR_FLAGS,                                         /**< Invalid vtss_appl_vlan_port_conf_t::flags        */
    VLAN_ERROR_PVID,                                          /**< Invalid vtss_appl_vlan_port_conf_t::pvid         */
    VLAN_ERROR_FRAME_TYPE,                                    /**< Invalid vtss_appl_vlan_port_conf_t::frame_type   */
    VLAN_ERROR_TX_TAG_TYPE,                                   /**< Invalid vtss_appl_vlan_port_conf_t::tx_tag_type  */
    VLAN_ERROR_UVID,                                          /**< Invalid vtss_appl_vlan_port_conf_t::untagged_vid */
    VLAN_ERROR_TPID,                                          /**< Invalid TPID                                     */
    VLAN_ERROR_PORT_MODE,                                     /**< Invalid port mode parameter                      */
    VLAN_ERROR_PARM,                                          /**< Illegal parameter                                */
    VLAN_ERROR_ENTRY_NOT_FOUND,                               /**< VLAN not found                                   */
    VLAN_ERROR_VLAN_TABLE_FULL,                               /**< VLAN table full                                  */
    VLAN_ERROR_USER_PREVIOUSLY_CONFIGURED,                    /**< A volatile user is configuring a port that is already configured */
    VLAN_ERROR_FLOODING_NOT_MANAGEABLE,                       /**< Flooding can not be managed                      */
#if defined(VTSS_SW_OPTION_VLAN_NAMING)
    VLAN_ERROR_NAME_ALREADY_EXISTS,                           /**< VLAN name is already configured                  */
    VLAN_ERROR_NAME_RESERVED,                                 /**< The VLAN name is reserved for another VLAN ID    */
    VLAN_ERROR_NAME_INVALID,                                  /**< The VLAN name contains invalid chars             */
    VLAN_ERROR_NAME_DOES_NOT_EXIST,                           /**< VLAN name does not exist in table                */
    VLAN_ERROR_NAME_DEFAULT_VLAN,                             /**< The default VLAN's name cannot be changed        */
    VLAN_ERROR_NAME_VLAN_NOT_CREATED,                         /**< Cannot set name of non-access VLANs              */
#endif
}; // Leave it anonymous

/**
 * Index into vlan_port_conflicts_t::users[] used to
 * get the VLAN user that caused a conflict for a particular
 * port configuration parameter.
 */
typedef enum {
    VLAN_PORT_FLAGS_IDX_PVID = 0,    /**< Index into vlan_port_conflicts_t::users[] that gives PVID conflicting users                    */
    VLAN_PORT_FLAGS_IDX_INGR_FILT,   /**< Index into vlan_port_conflicts_t::users[] that gives ingress filter conflicting users          */
    VLAN_PORT_FLAGS_IDX_RX_TAG_TYPE, /**< Index into vlan_port_conflicts_t::users[] that gives acceptable frame type conflicting users   */
    VLAN_PORT_FLAGS_IDX_TX_TAG_TYPE, /**< Index into vlan_port_conflicts_t::users[] that gives egress tagging conflicting users          */
    VLAN_PORT_FLAGS_IDX_AWARE,       /**< Index into vlan_port_conflicts_t::users[] that gives awareness and port type conflicting users */
    VLAN_PORT_FLAGS_IDX_CNT          /**< Must come last. Used to size arrays and stop iteration                                         */
} vlan_port_flags_idx_t;

/**
 * This structure is used for VLAN port configuration
 * conflict displaying in the user interface.
 */
typedef struct {
    /**
     * These flags indicate type of the conflict. For example, if there
     * is an Ingress filter conflict, the VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT bit will
     * be set in the mask.
     */
    u8 port_flags;

    /**
     * Each entry is indexed by a VTSS_APPL_VLAN_PORT_FLAGS_IDX_xxx and
     * contains a bitmask of conflicting VLAN users (VTSS_APPL_VLAN_USER_xxx).
     */
    u32 users[VLAN_PORT_FLAGS_IDX_CNT];
} vlan_port_conflicts_t;

/******************************************************************************/
//
// PORT FUNCTIONS ET AL
//
/******************************************************************************/

/**
 * Get VLAN port configuration.
 *
 * The function returns the VLAN port configuration
 * for #isid:#port. #port must be a normal, non-stack port.
 *
 * The function can get info directly from the switch API
 * and from a S/W state. Which of these depends on the
 * value of #isid, as follows:
 *
 * #isid == VTSS_ISID_LOCAL:
 *   Can be called on both the primary and on secondary switches.
 *   Either way, it will return the values currently
 *   stored in H/W on the local switch, and will therefore
 *   not be retrievable for a given VLAN user, which means
 *   that you must specify #user == VTSS_APPL_VLAN_USER_ALL, which
 *   is a synonym for the VLAN port configuration combined
 *   for all users. If you fail to specify VTSS_APPL_VLAN_USER_ALL,
 *   this function will return VLAN_ERROR_USER.
 *
 * #isid == [VTSS_ISID_START; VTSS_ISID_END[ (i.e. a legal ISID):
 *   Can only be called on the primary switch.
 *   In this case, #user may  be in range
 *   [VTSS_APPL_VLAN_USER_STATIC; VTSS_APPL_VLAN_USER_ALL], where VTSS_APPL_VLAN_USER_ALL
 *   causes this function to retrieve the combined VLAN port
 *   configuration.
 *
 * #isid == VTSS_ISID_GLOBAL:
 *   Illegal.
 *
 * \p details controls whether to get the high-level port configuration (when FALSE)
 * or a detailed-out configuration (when TRUE). It has only effect when
 * \p user == VTSS_APPL_VLAN_USER_STATIC. All other values of \p user assumes that \p details is TRUE.
 * When details are requested, the only valid part of \p conf is what is in \p hybrid.
 *
 * NOTICE: For now, vtss_appl_vlan_port_conf_t::forbidden_vids will not be filled in
 * when using vlan_mgmt_port_conf_get(). Use vtss_appl_vlan_interface_conf_get()
 * to have it filled in.
 *
 * \param isid    [IN]  VTSS_ISID_LOCAL or legal ISID. Functionality as specified above.
 * \param port    [IN]  Valid, non-stacking port number.
 * \param conf    [OUT] Pointer to structure retrieving the current port configuration.
 * \param user    [IN]  VLAN user to obtain configuration for. See also description above.
 * \param details [IN]  Only applicable for \p user VTSS_APPL_VLAN_USER_STATIC. If TRUE, hybrid-part of \p conf is filled in with details, rather than the remaining members of \p conf.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt(return code)
 *         to get a textual representation of the error code.
 */
mesa_rc vlan_mgmt_port_conf_get(vtss_isid_t isid, mesa_port_no_t port, vtss_appl_vlan_port_conf_t *conf, vtss_appl_vlan_user_t user, BOOL details);

/**
 * VLAN user VLAN port configuration change.
 *
 * Change the VLAN port configuration for #user. Any #user execpt for
 * VTSS_APPL_VLAN_USER_STATIC causes a volatile change that will not be persisted.
 * #user VTSS_APPL_VLAN_USER_ALL is not allowed.
 *
 * There is no guarantee that the configuration will take effect, because
 * it could happen that a higher prioritized user has already configured
 * the features that are attempted configured now.
 * Please check vlan_mgmt_port_conflicts_get() if this function returns
 * VTSS_RC_OK and you don't see your changes take effect.
 *
 * NOTICE: For now, vtss_appl_vlan_port_conf_t::forbidden_vids will not be used when
 * when calling vlan_mgmt_port_conf_set(). Use vtss_appl_vlan_interface_conf_set()
 * to make it take effect through the #conf structure, or vlan_mgmt_vlan_add()/del().
 *
 * \param isid [IN] ISID of a configurable switch. Must be in interval [VTSS_ISID_START; VTSS_ISID_END[.
 * \param port [IN] Port number (iport) to change configuration for. Stack ports not allowed.
 * \param conf [IN] New configuration.
 * \param user [IN] All VLAN users except VTSS_APPL_VLAN_USER_ALL.
 *
 * \return VTSS_RC_OK on success (but still no guarantee that the changes will take effect),
 *         anything else on error. Use error_txt(return code) to get a description.
 **/
mesa_rc vlan_mgmt_port_conf_set(vtss_isid_t isid, mesa_port_no_t port, const vtss_appl_vlan_port_conf_t *conf, vtss_appl_vlan_user_t user);

/**
 * Get a default port configuration.
 *
 * \param conf [OUT] Pointer receiving default port configuration.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt(return code)
 *         to get a textual representation of the error code. Errors can only
 *         occur if #conf == NULL.
 **/
mesa_rc vlan_mgmt_port_conf_default_get(vtss_appl_vlan_port_conf_t *conf);

/**
 * Get VLAN port conflicts for a given port.
 *
 * \param isid      [IN]  Legal ISID to switch to get conflicts for.
 * \param port      [IN]  Port number to get conflicts for. Stack ports not allowed.
 * \param conflicts [OUT] Pointer receiving conflicts.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt(return code)
 *         to get a textual representation of the error code.
 **/
mesa_rc vlan_mgmt_port_conflicts_get(vtss_isid_t isid, mesa_port_no_t port, vlan_port_conflicts_t *conflicts);

/******************************************************************************/
//
// MEMBERSHIP FUNCTIONS
//
/******************************************************************************/

/**
 * A more complex way of looking at VLAN membership registrations.
 */
typedef enum {
    VLAN_REGISTRATION_TYPE_NORMAL = 0, /**< Not member */
    VLAN_REGISTRATION_TYPE_FIXED,      /**< Member     */
    VLAN_REGISTRATION_TYPE_FORBIDDEN,  /**< Forbidden  */
} vlan_registration_type_t;

/**
 * Set membership of a range of ports to a specific VID.
 *
 * This function sets the membership for #user on #membership::vid,
 * i.e. it overwrites any previous configuration that #user may have
 * had on that VID.
 *
 * #user must be in range [VTSS_APPL_VLAN_USER_STATIC; VTSS_APPL_VLAN_USER_ALL[.
 *
 * If #user == VTSS_APPL_VLAN_USER_STATIC:
 *   The values of #isid and #membership don't matter, since the
 *   function will automatically add membership for ports on all
 *   switches that are going into this VID.
 *
 * If #user == VTSS_APPL_VLAN_USER_FORBIDDEN:
 *   Ports set in #membership will override all other users'
 *   membership by disallowing these ports on a given VID,
 *   so that the final membership value written to H/W always
 *   will have zeroes for forbidden ports.
 *
 * If #user != VTSS_APPL_VLAN_USER_STATIC && #user != VTSS_APPL_VLAN_USER_FORBIDDEN:
 *   #isid may be legal or VTSS_ISID_GLOBAL. Ports will become
 *   members of whatever is specified in #membership.
 *   If the VLAN doesn't exist prior to the call, it will be
 *   added to all switches in the stack. This means that on
 *   some switches it will be added with no ports as members.
 *   In order to be backward compatible and ease the module
 *   implementation, a VLAN will automatically get deleted
 *   when all switches have a zero memberset. This means that
 *   you may call this function with #membership->ports set to
 *   all-zeros.
 *
 * VTSS_APPL_VLAN_USER_STATIC and VTSS_APPL_VLAN_USER_FORBIDDEN should only be used
 * from administrative interfaces, i.e. Web, CLI, SNMP.
 *
 * The final port membership written to hardware is a bitwise OR
 * of all user modules' requests, with the exception that forbidden
 * ports are cleared.
 *
 * \param isid       [IN] Legal ISID or VTSS_ISID_GLOBAL.
 * \param membership [IN] The membership.
 * \param user       [IN] The VLAN user ([VTSS_APPL_VLAN_USER_STATIC; VTSS_APPL_VLAN_USER_ALL[) to change membership for.
 *
 * \return VTSS_RC_OK in most cases. If something different from VTSS_RC_OK is returned,
 *         it's because of parameters passed to the function or because the switch is
 *         currently not the primary switch.
 */
mesa_rc vlan_mgmt_vlan_add(vtss_isid_t isid, vtss_appl_vlan_entry_t *membership, vtss_appl_vlan_user_t user);

/**
 * Delete VLAN membership for #user on #vid.
 *
 * This function backs out #user's contribution to VLAN membership for
 * VLAN ID #vid on the switch given by #isid.
 *
 * #isid must be a legal ISID or VTSS_ISID_GLOBAL. If VTSS_ISID_GLOBAL,
 * membership is removed on all switches for #user.
 *
 * #user may be any VLAN user, including VTSS_APPL_VLAN_USER_ALL, i.e. in
 * range [VTSS_APPL_VLAN_USER_STATIC; VTSS_APPL_VLAN_USER_ALL].
 *
 * If invoked with VTSS_APPL_VLAN_USER_ALL, all users membership will be
 * deleted, except for the VLAN_FORBIDDEN_USER.
 *
 * To change the forbidden user's "anti-membership", this function
 * must be invoked directly with #user == VLAN_FORBIDDEN_USER.
 *
 * The function returns VTSS_RC_OK even if membership for a given
 * user doesn't exist (that is, even if the function has done nothing).
 *
 * VTSS_APPL_VLAN_USER_STATIC and VTSS_APPL_VLAN_USER_FORBIDDEN should only be used
 * from administrative interfaces, i.e. Web, CLI, SNMP.
 *
 * \param isid [IN] Legal ISID or VTSS_ISID_GLOBAL.
 * \param vid  [IN] The VID to delete membership for ([VTSS_APPL_VLAN_ID_MIN; VTSS_APPL_VLAN_ID_MAX]).
 * \param user [IN] The user ([VTSS_APPL_VLAN_USER_STATIC; VTSS_APPL_VLAN_USER_ALL]) to unregister membership for.
 *
 * \return VTSS_RC_OK in most cases. If something different from VTSS_RC_OK is returned,
 *         it's because of parameters passed to the function or because the switch is
 *         currently not the primary switch.
 */
mesa_rc vlan_mgmt_vlan_del(vtss_isid_t isid, mesa_vid_t vid, vtss_appl_vlan_user_t user);

/**
 * Get membership info for a given port and VLAN user.
 *
 * This function looks up #user's contribution to the
 * resulting VLAN mask on a given #isid:#port.
 * If a bit is set, the #user has added membership
 * for the corresponding VID on that #isid:#port.
 *
 * Use VTSS_BF_GET(vid_mask, vid) to traverse #vid_mask.
 *
 * Call with #user set to VTSS_APPL_VLAN_USER_FORBIDDEN to get the
 * forbidden VLANs (a '1' in bit-positions that are forbidden).
 *
 * Call with #user set to VTSS_APPL_VLAN_USER_ALL to get the memberships
 * as written to hardware.
 *
 * \param isid     [IN]  Legal ISID.
 * \param port     [IN]  Valid non-stack port.
 * \param user     [IN]  Must be in range [VTSS_APPL_VLAN_USER_STATIC; VTSS_APPL_VLAN_USER_ALL].
 * \param vid_mask [OUT] Pointer to an array of VTSS_APPL_VLAN_BITMASK_LEN_BYTES receiving the resulting per-VID memberships.
 *
 * \return VTSS_RC_OK on success. Anything else means that the caller
 *         has passed erroneous parameters to the function.
 */
mesa_rc vlan_mgmt_membership_per_port_get(vtss_isid_t isid, mesa_port_no_t port, vtss_appl_vlan_user_t user, u8 vid_mask[VTSS_APPL_VLAN_BITMASK_LEN_BYTES]);

/**
 * Get complex registration for a given port and VLAN user.
 *
 * This function looks up VTSS_APPL_VLAN_USER_STATIC and VTSS_APPL_VLAN_USER_FORBIDDEN's
 * contribution to the resulting VLAN mask and enumerates per
 * VLAN ID the high-level registration type like this:
 *
 * If a VID is forbidden, set #reg[vid] to VLAN_REGISTRATION_TYPE_FORBIDDEN.
 * Otherwise if #port is member of VID, set #reg[vid] to VLAN_REGISTRATION_TYPE_FIXED,
 * Otherwise set #reg[vid] to VLAN_REGISTRATION_TYPE_NORMAL.
 *
 * Notice that #reg possibly requires either static or dynamic allocation (that is
 * normally you will not want to allocate it on the stack).
 *
 * \param isid [IN]  Legal ISID.
 * \param port [IN]  Valid non-stack port.
 * \param reg  [OUT] Pointer to an array of VTSS_APPL_VLAN_ID_MAX +1 entries receiving the registration type.
 *
 * \return VTSS_RC_OK on success. Anything else means that the caller
 *         has passed erroneous parameters to the function.
 */
mesa_rc vlan_mgmt_registration_per_port_get(vtss_isid_t isid, mesa_port_no_t port, vlan_registration_type_t reg[VTSS_APPL_VLAN_ID_MAX + 1]);

#if defined(VTSS_SW_OPTION_VLAN_NAMING)
/******************************************************************************/
//
// VLAN NAMING FUNCTIONS
//
/******************************************************************************/

/**
 * Get a VLAN ID given a VLAN name.
 *
 * \param name [IN]  Name to look up.
 * \param vid  [OUT] Pointer to resulting VLAN ID.
 *
 * \return VTSS_RC_OK if entry was found and \p name filled in.
 *         VLAN_ERROR_NAME_DOES_NOT_EXIST if no such name matched. \p vid will be VTSS_VID_NULL in that case.
 *         Anything else means input parameter error.
 */
mesa_rc vlan_mgmt_name_to_vid(const char name[VTSS_APPL_VLAN_NAME_MAX_LEN], mesa_vid_t *vid);
#endif /* defined(VTSS_SW_OPTION_VLAN_NAMING) */

/******************************************************************************/
//
// UTILITY FUNCTIONS
//
/******************************************************************************/

/**
 * Function for converting a VLAN error
 * (see VLAN_ERROR_xxx above) to a textual string.
 * Only errors in the VLAN module's range can
 * be converted.
 *
 * \param rc [IN] Binary form of error
 *
 * \return Static string containing textual representation of #rc.
 */
const char *vlan_error_txt(mesa_rc rc);

/******************************************************************************/
//
// CONFIGURATION CHANGE CALLBACKS
//
/******************************************************************************/

/**
 * Signature of function to call back upon port configuration changes.
 *
 * Below, [IN] is seen from the called back function's p.o.v.
 * If called back on the primary switch, #isid is a legal ISID ([VTSS_ISID_START; VTSS_ISID_END[),
 * whereas it is VTSS_ISID_LOCAL if called back on the local switch (see
 * description under vlan_port_conf_change_register() for details).
 *
 * The callback function may call into the VLAN module again if it likes, without
 * risking deadlocks.
 *
 * The callback function is only invoked if #isid exists in the stack.
 *
 * \param isid     [IN] Switch ID on which a configuration change is about to occur (on primary switch) or occurred (on local switch, in which case it is VTSS_ISID_LOCAL).
 * \param port_no  [IN] Port number on which a configuration change is about to occur (on primary switch) or occurred (on local switch).
 * \param new_conf [IN] Pointer to the new port configuration.
 *
 * \return Nothing.
 */
typedef void (*vlan_port_conf_change_callback_t)(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_vlan_port_detailed_conf_t *new_conf);

/**
 * Register for VLAN port configuration changes.
 *
 * The caller may choose between getting called back on the local switch after
 * a change has just happened or on the primary switch. If called back on the
 * primary switch, the change may or may not already have happened on the switch
 * in question.
 *
 * Currently, there is no support for unregistering once registered.
 *
 * \param modid         [IN] Module ID of registrant. Only used for debug purposes.
 * \param cb            [IN] Pointer to a function to call back when port configuration changes.
 * \param cb_on_primary [IN] If TRUE, only call #cb on the primary switch, otherwise only call #cb on the local switch on which change has just occurred.
 *
 * \return Nothing.
 */
void vlan_port_conf_change_register(vtss_module_id_t modid, vlan_port_conf_change_callback_t cb, BOOL cb_on_primary);

/**
 * Signature of function to call back upon SVL configuration changes.
 *
 * Below, [IN] is seen from the called back function's p.o.v.
 *
 * The VLAN mutex is taken during the callback.
 *
 * \param vid     [IN] VLAN ID in question.
 * \param new_fid [IN] New FID in question. 0 if FID == VID.
 *
 * \return Nothing.
 */
typedef void (*vlan_svl_conf_change_callback_t)(mesa_vid_t vid, mesa_vid_t new_fid);

/**
 * Register for SVL configuration changes.
 *
 * Currently, there is no support for unregistering once registered.
 *
 * \param modid  [IN] Module ID of registrant. Only used for debug purposes.
 * \param cb     [IN] Pointer to a function to call back when SVL configuration changes.
 *
 * \return Nothing.
 */
void vlan_svl_conf_change_register(vtss_module_id_t modid, vlan_svl_conf_change_callback_t cb);

/**
 * Structure used to pass bit-arrays of ports
 * back and forth between VLAN module and users of it.
 */
typedef struct {
    /**
     * Port list.
     */
    mesa_port_list_t ports;
} vlan_ports_t;

/**
 * Structure used in membership change callbacks.
 *
 * Use VTSS_BF_GET() to access bits in the bit arrays.
 */
typedef struct {
    /**
     * This one indicates whether VTSS_APPL_VLAN_USER_STATIC has
     * added member-ports to a given VLAN.
     * It is not necessarily possible to
     * derive from #static_ports whether the static user
     * has added a VLAN, because it may contain no members.
     */
    BOOL static_vlan_exists;

    /**
     * Contains a bit per port that tells whether a change has occurred in
     * static user's or forbidden user's VLAN membership on that port (TRUE if so).
     */
    vlan_ports_t changed_ports;

    /**
     * Current (new) static port membership.
     */
    vlan_ports_t static_ports;

    /**
     * Current (new) forbidden port "membership".
     */
    vlan_ports_t forbidden_ports;
} vlan_membership_change_t;

/**
 * Signature of callback function invoked when VLAN membership changes on a given switch.
 *
 * [IN] is seen from the callback function's perspective.
 *
 * The callback function is only invoked for administrative changes, that is, when
 * the underlying VLAN user is VTSS_APPL_VLAN_USER_STATIC or VTSS_APPL_VLAN_USER_FORBIDDEN, so any changes made
 * by other volatile VLAN users (e.g. GVRP) are not reported anywhere.
 *
 * The callback function is only invoked on the primary switch.
 * The callback function may call into the VLAN module again if it likes, without risking deadlocks.
 * The callback function is only invoked if #isid exists in the stack.
 *
 * \param isid    [IN] A legal ISID identifying the switch on which the change is about to occur.
 * \param vid     [IN] A legal VLAN ID for which membership is about to occur.
 * \param changes [IN] The changes that have occurred. See structure for more details.
 *
 * \return Nothing.
 */
typedef void (*vlan_membership_change_callback_t)(vtss_isid_t isid, mesa_vid_t vid, vlan_membership_change_t *changes);

/**
 * Register for VLAN membership changes.
 *
 * The callback function will only be invoked on the primary switch.
 * There is no guarantee that the changes have propagated all the way
 * to hardware when called back.
 *
 * \param cb    [IN] Function to call back upon VLAN membership changes.
 * \param modid [IN] Module ID of registrant. Only used for debug purposes.
 *
 * \return Nothing.
 */
void vlan_membership_change_register(vtss_module_id_t modid, vlan_membership_change_callback_t cb);

/**
 * Signature of callback function invoked when VLAN membership changes on a given switch.
 *
 * The function may be invoked immediately when the changes occur, or it may be invoked
 * after a while. There is no indication of which changes have occurred.
 *
 * \return Nothing.
 */
typedef void (*vlan_membership_bulk_change_callback_t)(void);

/**
 * Register for VLAN membership changes.
 *
 * The callback function will only be invoked on the primary switch. It may take
 * a while for the callback function to be invoked. Only after a given management
 * interface is done updating VLAN memberships will the callback be invoked.
 *
 * \param cb    [IN] Function to call back upon VLAN membership changes.
 * \param modid [IN] Module ID of registrant. Only used for debug purposes.
 *
 * \return Nothing.
 */
void vlan_membership_bulk_change_register(vtss_module_id_t modid, vlan_membership_bulk_change_callback_t cb);

/* Start/Stop bulk updates.
 * Every call to vlan_bulk_update_begin() must be balanced with a call to vlan_bulk_update_end().
 * Useful e.g. when a large number of VLANs are changed
 */
void vlan_bulk_update_begin(void);
void vlan_bulk_update_end(void);
u32  vlan_bulk_update_ref_cnt_get(void);

/**
 * Signature of callback function invoked when S-custom tag EtherType changes.
 *
 * [IN] is seen from the callback function's perspective.
 *
 * The callback function will only be invoked on the primary switch.
 *
 * \param tpid [IN] The new custom S-tag EtherType set by management.
 *
 * \return Nothing.
 */
typedef void (*vlan_s_custom_etype_change_callback_t)(mesa_etype_t tpid);

/**
 * Register for S-custom EtherType changes.
 *
 * \param cb    [IN] Function to call back upon S-custom tag EtherType changes.
 * \param modid [IN] Module ID of registrant. Only used for debug purposes.
 *
 * \return Nothing.
 */
void vlan_s_custom_etype_change_register(vtss_module_id_t modid, vlan_s_custom_etype_change_callback_t cb);

/******************************************************************************/
//
// UTILITY FUNCTIONS
//
/******************************************************************************/

/**
 * Maximum buffer size needed in order to
 * convert a VLAN bit mask to a textual representation.
 *
 * Worst case is if every other VLAN is not defined, so
 * that the resulting string is something along these lines:
 * "1,3,5,...,4093,4095".
 *
 * Such a string is at most:
 *   [   1;    9]:    5 * (1 digit  + 1 comma) =   10 bytes
 *   [  10;   99]:   45 * (2 digits + 1 comma) =  135 bytes
 *   [ 100;  999]:  450 * (3 digits + 1 comma) = 1800 bytes
 *   [1000; 4095]: 1548 * (4 digits + 1 comma) = 7740 bytes
 * --------------------------------------------------------
 * Total                                         9685 bytes
 *
 * One could think that room for a terminating '\0' is
 * needed, but actually, it is not, because a comma is not
 * required after the last "4095" string, but such a comma
 * was already included in the compuatations above.
 */
#define VLAN_VID_LIST_AS_STRING_LEN_BYTES 9685

/**
 * Utility function that determines whether a given VID gets tagged or not on egress.
 *
 * \param p   [IN] Pointer to a port configuration previously obtained with a call to vtss_appl_vlan_interface_conf_get()
 * \param vid [IN] VID to check.
 *
 * \return TRUE if #vid gets tagged on egress, FALSE otherwise.
 */
BOOL vlan_mgmt_vid_gets_tagged(vtss_appl_vlan_port_detailed_conf_t *p, mesa_vid_t vid);

/**
 * Utility function that converts a VLAN bitmask to a textual representation.
 *
 * #bitmask is a mask of VTSS_APPL_VLAN_BITMASK_LEN_BYTES bytes, and
 * #txt is a string of at least VLAN_VID_LIST_AS_STRING_LEN_BYTES
 * bytes (which should normally be VTSS_MALLOC()ed).
 *
 * The resulting #txt could contain e.g. "1-4095" or "1,17-23,45".
 *
 * \param bitmask [IN]  Pointer to the binary bitmask.
 * \param txt     [OUT] Pointer to resulting text string.
 *
 * \return Pointer to txt, so that this can be used directly in a printf()-like function.
 */
char *vlan_mgmt_vid_bitmask_to_txt(const u8 bitmask[VTSS_APPL_VLAN_BITMASK_LEN_BYTES], char txt[VLAN_VID_LIST_AS_STRING_LEN_BYTES]);

/**
 * Utility function that checks if two VLAN bitmasks are identical.
 *
 * Both #bitmask1 and #bitmask2 are arrays of VTSS_APPL_VLAN_BITMASK_LEN_BYTES bytes.
 *
 * The reason that you should call this function rather than
 * memcmp() is that not all bits of #bitmask1/#bitmask2 need to be
 * valid, and may therefore have an arbitrary value.
 *
 * \param bitmask1 [IN] Pointer to the first binary bitmask.
 * \param bitmask2 [IN] Pointer to the second binary bitmask.
 *
 * \return TRUE if #bitmask1 and #bitmask2 are VLAN-wise identical,
 *         FALSE otherwise.
 */
BOOL vlan_mgmt_bitmasks_identical(u8 *bitmask1, u8 *bitmask2);

/**
 * Utility function that sets all significant bits of a VLAN bitmask to 1.
 *
 * #bitmask must be an array of VTSS_APPL_VLAN_BITMASK_LEN_BYTES bytes.
 *
 * \param bitmask [OUT] Pointer to the binary bitmask.
 *
 * \return VTSS_RC_OK on success. Can only return VLAN_ERROR_PARM otherwise.
 */
mesa_rc vlan_mgmt_bitmask_all_ones_set(u8 bitmask[VTSS_APPL_VLAN_BITMASK_LEN_BYTES]);

/**
 * Get a textual representation of a vtss_appl_vlan_user_t.
 *
 * \param user [IN] Binary form of VLAN user.
 *
 * \return Static string containing textual representation of #user.
 */
const char *vlan_mgmt_user_to_txt(vtss_appl_vlan_user_t user);

/**
 * Get a textual representation of a vtss_appl_vlan_port_type_t.
 *
 * \param port_type [IN] Binary form of port type.
 *
 * \return Static string containing textual representation of #port_type
 */
const char *vlan_mgmt_port_type_to_txt(vtss_appl_vlan_port_type_t port_type);

/**
 * Get a textual representation of a mesa_vlan_frame_t.
 *
 * \param frame_type [IN] Binary form of frame type.
 *
 * \return Static string containing textual representation of #frame_type
 */
const char *vlan_mgmt_frame_type_to_txt(mesa_vlan_frame_t frame_type);

/**
 * Get a textual representation of a vtss_appl_vlan_tx_tag_type_t.
 *
 * \param tx_tag_type     [IN] Binary form of Tx tag type.
 * \param can_be_any_uvid [IN] Used only to show non-static overrides of Tx tag type, which can tag or untag any particular VID, not just PVID.
 *
 * \return Static string containing textual representation of #tx_tag_type
 */
const char *vlan_mgmt_tx_tag_type_to_txt(vtss_appl_vlan_tx_tag_type_t tx_tag_type, BOOL can_be_any_uvid);

/**
 * Get to know whether #user is a valid caller of vlan_mgmt_port_conf_set()
 *
 * \param user [IN] VLAN user to ask for.
 *
 * \return TRUE if #user really may call vlan_mgmt_port_conf_set(), FALSE otherwise.
 */
BOOL vlan_mgmt_user_is_port_conf_changer(vtss_appl_vlan_user_t user);

/**
 * Get to know whether #user is a valid caller of vtss_appl_vlan_add().
 *
 * \param user [IN] VLAN user to ask for.
 *
 * \return TRUE if #user really may call vtss_appl_vlan_add(), FALSE otherwise.
 */
BOOL vlan_mgmt_user_is_membership_changer(vtss_appl_vlan_user_t user);

// Number of internal VLAN users
u32 vlan_user_int_cnt(void);

/******************************************************************************/
//
// OTHER NON-MANAGEMENT FUNCTIONS
//
/******************************************************************************/

/**
 * Module initialization function.
 *
 * \param data [IN] Pointer to state
 *
 * \return VTSS_RC_OK unless something serious is wrong.
 */
mesa_rc vlan_init(vtss_init_data_t *data);

#ifdef __cplusplus
}
#endif

// For tracing
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_vlan_port_detailed_conf_t *conf);

#endif /* _VLAN_API_H_ */

