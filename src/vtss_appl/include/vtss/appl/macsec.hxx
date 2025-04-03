/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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
* \brief Public MACsec API
* \details This header file describes IEEE802.1AE MACsec control functions and
* types.
*/

#ifndef _VTSS_APPL_MACSEC_HXX_
#define _VTSS_APPL_MACSEC_HXX_

#include <vtss/appl/interface.h>
#include <microchip/ethernet/phy/api.h>
#include <vtss/basics/enum_macros.hxx> /* For VTSS_ENUM_BITWISE() */

/**
 * Definition of error return codes.
 * See also macsec_error_txt() in macsec.cxx.
 */
enum {
    VTSS_APPL_MACSEC_RC_INVALID_PARAMETER = MODULE_ERROR_START(VTSS_MODULE_ID_MACSEC), /**< Invalid parameter                                                   */
    VTSS_APPL_MACSEC_RC_NOT_SUPPORTED,                                                 /**< MACsec functionality is not supported on this platform              */
    VTSS_APPL_MACSEC_RC_NOT_SUPPORTED_ON_PORT,                                         /**< MACsec functionality is not supported on this port                  */
    VTSS_APPL_MACSEC_RC_NOT_ENABLED_ON_PORT,                                           /**< MACsec is not enabled on this port                                  */
    VTSS_APPL_MACSEC_RC_INTERNAL_ERROR,                                                /**< Internal error. Requires code update                                */
    VTSS_APPL_MACSEC_RC_OUT_OF_MEMORY,                                                 /**< Out of memory                                                       */
    VTSS_APPL_MACSEC_RC_INVALID_IFINDEX,                                               /**< Invalid ifindex                                                     */
    VTSS_APPL_MACSEC_RC_IFINDEX_IS_NOT_A_PORT,                                         /**< Ifindex does not represent a port                                   */
    VTSS_APPL_MACSEC_RC_IFINDEX_PORT_INVALID,                                          /**< The port number that the interface index represents is out of range */
    VTSS_APPL_MACSEC_RC_INVALID_SECY_ID,                                               /**< Invalid SecY ID for the chosen interface                            */
    VTSS_APPL_MACSEC_RC_NO_SUCH_SECY_ID,                                               /**< The referenced SecY does not exist                                  */
    VTSS_APPL_MACSEC_RC_INVALID_RX_SC_IDX,                                             /**< Invalid Rx SC index for the chosen SecY                             */
    VTSS_APPL_MACSEC_RC_INVALID_FRAME_VALIDATION,                                      /**< Invalid value of the frame validation parameter                     */
    VTSS_APPL_MACSEC_RC_INVALID_TX_SC_MAC_TYPE,                                        /**< Invalid Tx SC MAC type                                              */
    VTSS_APPL_MACSEC_RC_RX_SC_PEER_MAC_MUST_BE_UNICAST,                                /**< The Rx SC's Peer MAC address must be a unicast MAC address          */
    VTSS_APPL_MACSEC_RC_RX_SC_PEER_MAC_MUST_BE_NON_ZERO,                               /**< The Rx SC's Peer MAC address must not be the all-zeros MAC address  */
    VTSS_APPL_MACSEC_RC_TX_SC_CUSTOM_MAC_MUST_BE_UNICAST,                              /**< Custom Tx SC MAC address must be a unicast MAC address              */
    VTSS_APPL_MACSEC_RC_TX_SC_MAC_MUST_BE_NON_ZERO,                                    /**< Custom Tx SC MAC address must not be all-zeros                      */
    VTSS_APPL_MACSEC_RC_TX_SC_IDENTICAL_MAC,                                           /**< Another SecY uses the same Tx SC MAC address as this one            */
    VTSS_APPL_MACSEC_RC_NO_CIPHER_SUITE_SELECTED,                                      /**< No cipher suite is selected                                         */
    VTSS_APPL_MACSEC_RC_MORE_THAN_ONE_CIPHER_SUITE_SELECTED,                           /**< More than one cipher suites are selected                            */
    VTSS_APPL_MACSEC_RC_INVALID_CIPHER_SUITE,                                          /**< No such cipher suite                                                */
    VTSS_APPL_MACSEC_RC_CIPHER_SUITE_NOT_SUPPORTED_ON_PORT,                            /**< Selected cipher suite is not supported on this port                 */
    VTSS_APPL_MACSEC_RC_INVALID_CONFIDENTIALITY_OFFSET,                                /**< Invalid confidentiality offset                                      */
    VTSS_APPL_MACSEC_RC_INVALID_TX_SA_INDEX,                                           /**< The Tx SA index is too high                                         */
    VTSS_APPL_MACSEC_RC_INVALID_RX_SA_INDEX,                                           /**< The Rx SA index is too high                                         */
    VTSS_APPL_MACSEC_RC_TX_SA_NEXT_PN_CANNOT_BE_ZERO,                                  /**< Next-PN cannot be zero in Tx SA                                     */
    VTSS_APPL_MACSEC_RC_TX_SA_NEXT_PN_CANNOT_EXCEED_32BITS_NON_XPN,                    /**< Next-PN cannot exceed 2^32 - 1 when not using XPN                   */
    VTSS_APPL_MACSEC_RC_RX_SA_LOWEST_PN_CANNOT_EXCEED_32BITS_NON_XPN,                  /**< Lowest-PN cannot exceed 2^32 - 1 when not using XPN                 */
    VTSS_APPL_MACSEC_RC_TX_SA_EXACTLY_ONE_MUST_BE_ACTIVE,                              /**< Exactly one Tx SA must be active                                    */
    VTSS_APPL_MACSEC_RC_RX_SA_EXACTLY_ONE_MUST_BE_ACTIVE,                              /**< Exactly one Rx SA of every enabled Rx SC must be active             */
    VTSS_APPL_MACSEC_RC_RX_SC_IDENTICAL_PEER_MACS,                                     /**< Two Rx SCs cannot use the same Peer MAC address on the same SecY    */
    VTSS_APPL_MACSEC_RC_RX_SC_IDENTICAL_VIRTUAL_PORT_ID_AND_PEER_MAC,                  /**< Another SecY uses the same <Virtual Port ID, Peer MAC> Rx SC        */
    VTSS_APPL_MACSEC_RC_TX_SAK_128_WRONG_LENGTH,                                       /**< Tx SA: A 128 bit cipher requires a key of 32 hexadecimal characters */
    VTSS_APPL_MACSEC_RC_RX_SAK_128_WRONG_LENGTH,                                       /**< Rx SA: A 128 bit cipher requires a key of 32 hexadecimal characters */
    VTSS_APPL_MACSEC_RC_TX_SAK_256_WRONG_LENGTH,                                       /**< Tx SA: A 256 bit cipher requires a key of 64 hexadecimal characters */
    VTSS_APPL_MACSEC_RC_RX_SAK_256_WRONG_LENGTH,                                       /**< Rx SA: A 256 bit cipher requires a key of 64 hexadecimal characters */
    VTSS_APPL_MACSEC_RC_TX_SAK_KEY_NOT_ONLY_HEX_CHARS,                                 /**< Tx SA: Key must contain hexadecimal characters only                 */
    VTSS_APPL_MACSEC_RC_RX_SAK_KEY_NOT_ONLY_HEX_CHARS,                                 /**< Rx SA: Key must contain hexadecimal characters only                 */
    VTSS_APPL_MACSEC_RC_TX_SAK_SALT_WRONG_LEN,                                         /**< Tx SA: An XPN cipher requires 24 hexadecimal characters of salt     */
    VTSS_APPL_MACSEC_RC_RX_SAK_SALT_WRONG_LEN,                                         /**< Rx SA: An XPN cipher requires 24 hexadecimal characters of salt     */
    VTSS_APPL_MACSEC_RC_TX_SAK_SALT_NOT_ONLY_HEX_CHARS,                                /**< Tx SA: Salt must contain hexadecimal characters only                */
    VTSS_APPL_MACSEC_RC_RX_SAK_SALT_NOT_ONLY_HEX_CHARS,                                /**< Rx SA: Salt must contain hexadecimal characters only                */
    VTSS_APPL_MACSEC_RC_INVALID_STATISTICS_CLEAR_REQUEST,                              /**< Invalid statistics clear request                                    */
};

/**
 * Structure containing overall MACsec capabilities.
 */
typedef struct {
    /**
     * If not a single port on this device has MACsec capabilities, the MACsec
     * module will not enable itself.
     */
    mesa_bool_t macsec_supported;
} vtss_appl_macsec_capabilities_t;

/**
 * Get MACsec capabilities
 *
 * \param cap [OUT] Capabilities
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_macsec_capabilities_get(vtss_appl_macsec_capabilities_t *cap);

/**
 * This is a bitwise enumeration of supported cipher suites.
 * If used to select a given cipher suite, exactly one bit must be set.
 */
typedef enum {
    VTSS_APPL_MACSEC_CIPHER_SUITE_GCM_AES_128     = 0x01, /**< GCM-AES-128       */
    VTSS_APPL_MACSEC_CIPHER_SUITE_GCM_AES_256     = 0x02, /**< GCM-AES-256       */
    VTSS_APPL_MACSEC_CIPHER_SUITE_GCM_AES_XPN_128 = 0x04, /**< GCM-AES-128 (XPN) */
    VTSS_APPL_MACSEC_CIPHER_SUITE_GCM_AES_XPN_256 = 0x08, /**< GCM-AES-256 (XPN) */
} vtss_appl_macsec_cipher_suite_t;

/**
 * Operators for vtss_appl_macsec_cipher_suite_t flags.
 */
VTSS_ENUM_BITWISE(vtss_appl_macsec_cipher_suite_t);

/**
 * Structure containing capabilities for a given port
 */
typedef struct {
    /**
     * True if PHY is MACsec capable, false if not.
     */
    mesa_bool_t macsec_capable;

    /**
     * Supported cipher suites. Notice that the enum is a bit-wise enumerator,
     * so if a bit is set, the corresponding cipher suite is supported.
     */
    vtss_appl_macsec_cipher_suite_t cipher_suites;

    /**
     * Total number of Rx SAs (Secure Associations)
     */
    uint32_t total_rx_sa_cnt;

    /**
     * Total number of Tx SAs (Secure Associations)
     */
    uint32_t total_tx_sa_cnt;

    /**
     * Maximum number of SecYs.
     */
    uint32_t secy_cnt_max;

    /**
     * Maximum number of Tx SCs (Secure Channels) per SecY.
     * This is always 1.
     */
    uint32_t tx_sc_cnt_per_secy_max;

    /**
     * The maxmium number of Rx SCs (Secure Channels) per SecY.
     */
    uint32_t rx_sc_cnt_per_secy_max;

    /**
     * Maximum number of Tx SAs per SC.
     * Notice, that it might be that the PHY runs out of Tx SAs if you configure
     * this number of SAs per allowed Tx SC per allowed SecY.
     */
    uint32_t tx_sa_cnt_per_sc_max;

    /**
     * Maximum number of Rx SAs per SC
     */
    uint32_t rx_sa_cnt_per_sc_max;
} vtss_appl_macsec_interface_capabilities_t;

/**
 * Get MACsec capabilities for a given interface.
 *
 * \param ifindex [IN]  Port to get capabilities for
 * \param if_caps [OUT] Interface capabilities
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_macsec_interface_capabilities_get(vtss_ifindex_t ifindex, vtss_appl_macsec_interface_capabilities_t *if_caps);

/**
 * Per-port MACsec configuration
 */
typedef struct {
    /**
     * Enable or disable the MACsec block on the port.
     */
    mesa_bool_t enable;
} vtss_appl_macsec_interface_conf_t;

/**
 * Get a default global per-port MACsec configuration
 *
 * \param conf [OUT] Default per-port MACsec configuration
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_macsec_interface_conf_default_get(vtss_appl_macsec_interface_conf_t *conf);

/**
 * Get the global per-port MACsec configuration
 *
 * \param ifindex [IN]  Port to get MACsec configuration for
 * \param conf    [OUT] Configuration
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_macsec_interface_conf_get(vtss_ifindex_t ifindex, vtss_appl_macsec_interface_conf_t *conf);

/**
 * Set the global per-port MACsec configuration
 *
 * \param ifindex [IN]  Port to set MACsec configuration for
 * \param conf    [OUT] Configuration
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_macsec_interface_conf_set(vtss_ifindex_t ifindex, const vtss_appl_macsec_interface_conf_t *conf);

/**
 * Iterate across all ports that support MACsec.
 *
 * \param prev_ifindex [IN]  Pointer to the ifindex to get the next from - or nullptr or VTSS_IFINDEX_NONE to get the first
 * \param next_ifindex [OUT] If return value is VTSS_RC_OK, it will contain the ifindex of the next port supporting MACsec.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_macsec_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex);

/**
 * Index of a SecY within a MACsec port.
 */
typedef uint32_t vtss_appl_macsec_secy_id_t;

/**
 * This may be used to start an iteration across defined SecYs.
 */
#define VTSS_APPL_MACSEC_SECY_ID_NONE 0

/**
 * Use this key structure to access a SecY within a given MACsec-enabled port.
 */
struct vtss_appl_macsec_secy_key_t {
    /**
     * Default constructor
     */
    vtss_appl_macsec_secy_key_t(void) : ifindex(VTSS_IFINDEX_NONE), secy_id(VTSS_APPL_MACSEC_SECY_ID_NONE) {};

    /**
     * Needed by Private MIB generator (row editor)
     * \return A reference to a SecY key.
     */
    const vtss_appl_macsec_secy_key_t &operator*(void);

    /**
     * Interface index of port to access.
     */
    vtss_ifindex_t ifindex;

    /**
     * SecY ID.
     */
    vtss_appl_macsec_secy_id_t secy_id;
};

/**
 * Specialized function to output contents of a SecY key to a stream
 *
 * \param o        [OUT] Stream
 * \param secy_key [IN]  Key to print
 *
 * \return The stream itself
 */
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_macsec_secy_key_t &secy_key);

/**
 * Specialized fmt() function usable in trace commands.
 *
 * Suppose you want to trace a SecY key. You *could* do it like this:
 *   T_I("SecY = %s::%u", secy_key.ifindex, secy_key.secy_id);
 *
 * The presence of the following function ensures that you can do it like this:
 *   T_I("SecY = %s", secy_key);
 *
 * You do not need to know about the parameters, so they won't be documented
 * very well.
 *
 * \param o        [OUT] Stream
 * \param fmt      [IN]  Format
 * \param secy_key [IN]  Key to print
 *
 * \return Number of bytes written.
 */
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_macsec_secy_key_t *secy_key);

/**
 * The key to be used with a particular Secure Association (SA);
 * This can be an either 128 bit (16 bytes) or 256 bits (32 bytes) AES key.
 */
typedef struct {
    /**
     * The 16 or 32 bytes key, expressed as a character array of only
     * hexadecimal characters + a terminating null.
     * The chosen cipher determines the length.
     *
     * To generate a random 16-byte key, use e.g.
     *   dd if=/dev/urandom count=16 bs=1 2>/dev/null | hexdump | cut -c 9- | tr -d ' \n'
     *
     * Likewise, to generate a random 32-byte key, use e.g.
     *   dd if=/dev/urandom count=32 bs=1 2>/dev/null | hexdump | cut -c 9- | tr -d ' \n'
     */
    char key[65];

    /**
     * The salt is a 96-bit (12 bytes) parameter provided to the current cipher
     * suite for subsequent protection and validation operations.
     *
     * This is used for XPN ciphers only.
     *
     * From 802.1AEbw:
     * The 64 least significant bits of the salt are the 64 least significant
     * bits of the MKA key server's Member Identifier (MI).
     * The 16 next most significant bits of the salt comprise the XOR of the 16
     * next most significant bits of that MI with the 16 most significant bits
     * of the 32-bit MKA Key Number (KN).
     * The 16 most significant bits of the salt comprise the XOR of the 16 most
     * significant bits of the MI with the 16 least significant bits of the KN.
     *
     * Since we don't have an MKA, the user must, however, specify it herself as
     * 24 hexadecimal characters plus a terminating null.
     *
     * To generate a random 12-byte salt, use e.g.
     *   dd if=/dev/urandom count=12 bs=1 2>/dev/null | hexdump | cut -c 0 | tr -d ' \n'
     */
    char salt[25];
} vtss_appl_macsec_sak_t;

/**
 * Tx Secure Association configuration
 */
typedef struct {
    /**
     * Set this to true to install this Tx SA in the PHY, and false to remove
     * it.
     */
    mesa_bool_t configured;

    /**
     * Set this to true to activate this Tx SA instance. Exactly one Tx SA must
     * be active if the SecY is administratively enabled.
     *
     * The thing is that this one should have been an action to apply to an
     * administratively enabled SecY, but the MACsec API contains a grave bug
     * that goes like this:
     * If link is up while a SecY and its SCs and SAs get configured, everything
     * is fine and the SecY will not really become active until an Rx/Tx SA has
     * been activated.
     * If, however, link is down while configuring the SecY, and link goes up
     * afterwards, the MACsec API thinks that all configured SAs (both Rx and
     * Tx) are in use and it automatically activates the first.
     */
    mesa_bool_t active;

    /**
     * Packet number of the first packet sent using the new SA.
     *
     * Must be > 0 and less than 2^32 - 1 for 32-bit packet numbering and less
     * than 2^64 -1 for 64-bit packet numbering (XPN).
     */
    uint64_t next_pn;

    /**
     * If true, packets are encrypted, otherwise only integrity protected.
     */
    mesa_bool_t encrypt;

    /**
     * The key (SAK) used with this SA.
     */
    vtss_appl_macsec_sak_t sak;
} vtss_appl_macsec_tx_sa_conf_t;

/**
 * Upper bound for the number of Secure Associations to have per Tx SC.
 * The minimum we must support is 2. If you ever adjust this, the code in
 * MACSEC_capabilities_set() may have to be changed to a value actually
 * supported by the PHY.
 */
#define VTSS_APPL_MACSEC_TX_SA_CNT_PER_SC_MAX 4

#if VTSS_APPL_MACSEC_TX_SA_CNT_PER_SC_MAX < 2
#error "VTSS_APPL_MACSEC_TX_SA_CNT_PER_SC_MAX must be at least 2"
#endif

/**
 * Indicates which MAC address to use in a SecY's Tx SC.
 */
typedef enum {
    VTSS_APPL_MACSEC_TX_SC_MAC_TYPE_MGMT,   /**< Use the CPU's management MAC address in the Tx SC  */
    VTSS_APPL_MACSEC_TX_SC_MAC_TYPE_PORT,   /**< Use the port's management MAC address in the Tx SC */
    VTSS_APPL_MACSEC_TX_SC_MAC_TYPE_CUSTOM, /**< Use a custom MAC address in the Tx SC              */
} vtss_appl_macsec_tx_sc_mac_type_t;

/**
 * Tx Secure Channel configuration
 */
typedef struct {
    /**
     * If true, add this Tx SC to hardware.
     * If false, remove it from hardware.
     */
    mesa_bool_t configured;

    /**
     * Selects which MAC address to use in the Tx SC.
     * Default is the CPU's management MAC address.
     */
    vtss_appl_macsec_tx_sc_mac_type_t mac_type;

    /**
     * MAC address to use in Tx SC. This is only used when \p mac_type is
     * VTSS_APPL_MACSEC_TX_SC_MAC_TYPE_CUSTOM.
     *
     * Only non-zero, unicast MAC addresses are allowed.
     */
    mesa_mac_t custom_mac;

    /**
     * The Secure Association configuration for the Tx SC.
     * The number of SAs per SC may vary from device to device, so the
     * VTSS_APPL_MACSEC_TX_SA_CNT_PER_SC_MAX constitutes an upper bound. Use
     * vtss_appl_macsec_interface_capabilities_t::tx_sa_cnt_per_sc_max to find
     * the actual.
     */
    vtss_appl_macsec_tx_sa_conf_t sa[VTSS_APPL_MACSEC_TX_SA_CNT_PER_SC_MAX];
} vtss_appl_macsec_tx_sc_conf_t;

/**
 * Rx Secure Association configuration
 */
typedef struct {
    /**
     * Set this to true to install this SA in the PHY, and false to remove it.
     */
    mesa_bool_t configured;

    /**
     * Set this to true to activate this Rx SA instance. Exactly one Rx SA must
     * be active per Rx SC if the SecY is administratively enabled.
     *
     * The thing is that this one should have been an action to apply to an
     * administratively enabled SecY, but the MACsec API contains a grave bug
     * that goes like this:
     * If link is up while a SecY and its SCs and SAs get configured, everything
     * is fine and the SecY will not really become active until an Rx/Tx SA has
     * been activated.
     * If, however, link is down while configuring the SecY, and link goes up
     * afterwards, the MACsec API thinks that all configured SAs (both Rx and
     * Tx) are in use and it automatically activates the first.
     */
    mesa_bool_t active;

    /**
     * The lowest packet number associated with the SA.
     */
    uint64_t lowest_pn;

    /**
     * The key (SAK) used with this SA.
     */
    vtss_appl_macsec_sak_t sak;
} vtss_appl_macsec_rx_sa_conf_t;

/**
 * Upper bound for the number of Secure Associations to have per Rx SC.
 * The minimum we must support is 2. If you ever adjust this, the code in
 * MACSEC_capabilities_set() may have to be changed to a value actually
 * supported by the PHY.
 */
#define VTSS_APPL_MACSEC_RX_SA_CNT_PER_SC_MAX 4

#if VTSS_APPL_MACSEC_RX_SA_CNT_PER_SC_MAX < 2
#error "VTSS_APPL_MACSEC_RX_SA_CNT_PER_SC_MAX must be at least 2"
#endif

/**
 * Rx Secure Channel configuration
 */
typedef struct {
    /**
     * Set this to true to install this Rx SC in the PHY, and false to remove
     * it.
     */
    mesa_bool_t configured;

    /**
     * Peer MAC address.
     * Only non-zero, unicast MAC addresses are allowed.
     */
    mesa_mac_t peer_mac;

    /**
     * The Secure Association configuration for this Rx SC.
     * The number of SAs per SC may vary from device to device, so the
     * VTSS_APPL_MACSEC_RX_SA_CNT_PER_SC_MAX constitutes an upper bound. Use
     * vtss_appl_macsec_interface_capabilities_t::rx_sa_cnt_per_sc_max to find
     * the actual.
     */
    vtss_appl_macsec_rx_sa_conf_t sa[VTSS_APPL_MACSEC_RX_SA_CNT_PER_SC_MAX];
} vtss_appl_macsec_rx_sc_conf_t;

/**
 * This indicates that no Rx SC is chosen.
 */
#define VTSS_APPL_MACSEC_RX_SC_IDX_NONE 0xFFFFFFFF

/**
 * This is used to size Rx SC arrays. It defines an upper limit. Use
 * vtss_appl_macsec_interface_capabilities_t::rx_sc_cnt_per_secy_max to find the
 * actual number of supported Rx SCs per SecY for a given port.
 */
#define VTSS_APPL_MACSEC_RX_SC_CNT_MAX 8

/**
 * Configuration of how MACsec handles the ICV of incoming frames.
 */
typedef enum {
    VTSS_APPL_MACSEC_FRAME_VALIDATION_DISABLED, /**< Integrity check is not performed, but SecTAG and ICV are removed if present. */
    VTSS_APPL_MACSEC_FRAME_VALIDATION_CHECK,    /**< Validation is enabled, but invalid frames are not discarded.                 */
    VTSS_APPL_MACSEC_FRAME_VALIDATION_STRICT,   /**< Validation is enabled and invalid frames are discarded.                      */
} vtss_appl_macsec_frame_validation_t;

/**
 * MACsec configuration of a PHY's SecY.
 */
typedef struct {
    /**
     * Virtual port ID
     * Used in the SCI tag if present.
     * Changing the virtual port ID causes all SCs and their SAs to be
     * recreated.
     */
    uint16_t virtual_port_id;

    /**
     * Selects the cipher suite to use - and implicitly also whether running
     * normal or extended packet numbering (XPN).
     *
     * Changing the cipher suite from a 128- to a 256-bit or vice versa requires
     * all SAs to be removed before-hand, because the key length isn't valid.
     * All SCs will be recreated.
     *
     * Default is VTSS_APPL_MACSEC_CIPHER_SUITE_GCM_AES_128.
     *
     * Corresponds to currentCipherSuite.
     */
    vtss_appl_macsec_cipher_suite_t cipher_suite;

    /**
     * Selects the offset into the frames where to begin/end encryption.
     *
     * In the Tx SC, the value is only used if encryption is enabled.
     *
     * Valid values are [0; 64]. Default is 0.
     *
     * Corresponds to secyCipherSuiteProtectionOffset, which was deprecated in
     * 802.1AEcg-2017.
     */
    uint32_t confidentiality_offset;

    /**
     * Enables or disables MACsec encapsulation of egressing frames.
     *
     * When enabled, frames will be MACsec encapsulated with an ICV. If \p
     * encrypt is also set, the data will be encrypted.
     *
     * If the SA or the next SA after a PN rollover doesn't contain a key, both
     * Rx and Tx of frames will stop (MAC_Operational will go to false).
     *
     * When disabled, frames will pass through the crypto-core to the controlled
     * port without modification. This is useful when deploying MACsec in a
     * network.
     *
     * Default is true.
     *
     * Corresponds to protectFrames in 802.1AE.
     */
    mesa_bool_t tx_protect_frames;

    /**
     * Indicates whether to include the SCI in the SecTAG.
     *
     * Default is false.
     *
     * Corresponds to alwaysIncludeSCI.
     */
    mesa_bool_t tx_always_include_sci;

    /**
     * Enables use of the ES (End Station) bit in the SecTAG when transmitting
     * protected frames.
     *
     * Default is false.
     *
     * Corresponds to useES.
     */
    mesa_bool_t tx_use_es;

    /**
     * Enables use of the SCB (Single Copy Broadcast) bit in the SecTAG when
     * transmitting protected frames.
     *
     * Default is false.
     *
     * Corresponds to useSCB.
     */
    mesa_bool_t tx_use_scb;

    /**
     * Controls how MACsec handles the ICV of incoming frames.
     *
     * Default is strict.
     *
     * Corresponds to validateFrames.
     */
    vtss_appl_macsec_frame_validation_t rx_frame_validation;

    /**
     * If true, frames arriving with a packet number outside the replay protect
     * window are discarded.  If false, such frames are allowed.
     *
     * See also \p replay_window.
     *
     * Default is true.
     *
     * Corresponds to replayProtect.
     */
    mesa_bool_t rx_replay_protect;

    /**
     * Controls the window size within which a frame's packet number is
     * accepted.
     *
     * This is only used if \p replay_protect is true.
     *
     * Default is 0, indicating that frames must come in order.
     *
     * Corresponds to replayWindow.
     *
     * Notice, that even if using 64-bit packet numbers (XPN), the replay window
     * is an uint32_t.
     */
    uint32_t rx_replay_window;

    /**
     * SecY's Tx Secure Channel configuration, including SAs.
     */
    vtss_appl_macsec_tx_sc_conf_t tx_sc;

    /**
     * SecY's Rx Secure Channel configuration, including SAs.
     * The number of Rx SCs may vary from PHY to PHY, so the
     * VTSS_APPL_MACSEC_RX_SC_CNT_MAX constitutes an upper bound. Use
     * vtss_appl_macsec_interface_capabilities_t::rx_sc_cnt_per_secy_max to find
     * the actual.
     */
    vtss_appl_macsec_rx_sc_conf_t rx_sc[VTSS_APPL_MACSEC_RX_SC_CNT_MAX];

    /**
     * The administrative state of this SecY instance (default false).
     * Set to true to apply its configuration to H/W and false to remove it from
     * H/W (but keep it in S/W).
     */
    mesa_bool_t admin_active;
} vtss_appl_macsec_secy_conf_t;

/**
 * Get a default SecY configuration.
 *
 * \param conf [OUT] Pointer to structure receiving a default SecY configuration.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_macsec_secy_conf_default_get(vtss_appl_macsec_secy_conf_t *conf);

/**
 * Get the current SecY configuration for a particular port.
 *
 * \param secy_key [IN]  The SecY key to get the SecY configuration for.
 * \param conf     [OUT] Pointer to structure receiving the port's SecY configuration.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_macsec_secy_conf_get(const vtss_appl_macsec_secy_key_t &secy_key, vtss_appl_macsec_secy_conf_t *conf);

/**
 * Set the SecY configuration for a particular port.
 *
 * Notice that only SCs created after this call will get this configuration.
 * Existing SCs will keep their current configuration.
 *
 * \param secy_key [IN] The SecY key to set the SecY configuration for.
 * \param conf     [IN] Pointer to structure with the new SecY configuration for that port.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_macsec_secy_conf_set(const vtss_appl_macsec_secy_key_t &secy_key, const vtss_appl_macsec_secy_conf_t *conf);

/**
 * Delete an existing SecY configuration for a particular port.
 *
 * \param secy_key [IN] The SecY key to delete along with its SCs and SAs.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_macsec_secy_conf_del(const vtss_appl_macsec_secy_key_t &secy_key);

/**
 * Iterate across SecYs.
 *
 * To iterate across all SecYs on all interfaces, set prev_key to nullptr or
 * clear its contents.
 *
 * To iterate across all SecYs on a given interface, start by setting
 *   prev_key.ifindex to the interface
 *   prev_key.secy_id = VTSS_APPL_MACSEC_SECY_ID_NONE
 * Then iterate with \p stay_on_this_interface set to true. This will cause the
 * function to return something != VTSS_RC_OK before entering this next port.
 *
 * \param prev_secy_key          [IN]  Previous SecY
 * \param next_secy_key          [OUT] Next SecY
 * \param stay_on_this_interface [IN] See text above.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_macsec_secy_itr(const vtss_appl_macsec_secy_key_t *prev_secy_key, vtss_appl_macsec_secy_key_t *next_secy_key, bool stay_on_this_interface = false);

/**
 * Value indicating no Secure Association.
 */
#define VTSS_APPL_MACSEC_SA_IDX_NONE 0xffffffff

/**
 * Timestamp of various events.
 */
typedef struct {
    /**
     * Time when this was created measured in seconds since boot.
     */
    uint32_t created_time;

    /**
     * Time when this was started measured in seconds since boot. Identical to
     * \p created_time if not started.
     */
    uint32_t started_time;

    /**
     * Time when this was stopped measured in seconds since boot. Identical to
     * \p created_time if not stopped.
     */
    uint32_t stopped_time;
} vtss_appl_macsec_timestamps_t;

/**
 * Status of one Tx SA.
 * See 802.1AE clause 10.7.22.
 */
typedef struct {
    /**
     * True if installed in H/W, false if not.
     * If not, the remaining fields are invalid.
     */
    mesa_bool_t installed;

    /**
     * If true, this SA is currently in use.
     */
    mesa_bool_t in_use;

    /**
     * Next Packet Number.
     */
    uint64_t next_pn;

    /**
     * Timestamps of events for this Tx SA
     */
    vtss_appl_macsec_timestamps_t timestamps;
} vtss_appl_macsec_tx_sa_status_t;

/**
 * Status of a particular Tx Secure Channel
 * See 802.1AE clause 10.7.20.
 */
typedef struct {
    /**
     * True if installed in H/W, false if not.
     * If not, the remaining fields are invalid.
     */
    mesa_bool_t installed;

    /**
     * True if at least one of the SAs are in use.
     */
    mesa_bool_t transmitting;

    /**
     * Index (AN) of the encoding SA. See 802.1AE clause 10.5.1.
     */
    uint32_t encoding_sa_idx;

    /**
     * Index (AN) of the encyphering SA. See 802.1AE clause 10.5.4.
     */
    uint32_t enciphering_sa_idx;

    /**
     * Indicates which - if any - SA within this Tx SC is active.
     *
     * The value VTSS_APPL_MACSEC_SA_IDX_NONE indicates that no SA is active.
     */
    uint32_t active_sa_idx;

    /**
     * Timestamps of events for this Tx SC
     */
    vtss_appl_macsec_timestamps_t timestamps;

    /**
     * Status of this SecY's Tx SC's SAs.
     * There is one such for each possible SA. Use
     * vtss_appl_macsec_interface_capabilities_t::tx_sa_cnt_per_sc_max to find
     * the upper limit for this particular port.
     */
    vtss_appl_macsec_tx_sa_status_t sa[VTSS_APPL_MACSEC_TX_SA_CNT_PER_SC_MAX];
} vtss_appl_macsec_tx_sc_status_t;

/**
 * Status of one Rx SA.
 * See 802.1AE clause 10.7.14.
 */
typedef struct {
    /**
     * True if installed in H/W, false if not.
     * If not, the remaining fields are invalid.
     */
    mesa_bool_t installed;

    /**
     * If true, this SA is currently in use.
     */
    mesa_bool_t in_use;

    /**
     * Next packet number.
     */
    uint64_t next_pn;

    /**
     * Lowest packet number.
     */
    uint64_t lowest_pn;

    /**
     * Timestamps of events for this Tx SA
     */
    vtss_appl_macsec_timestamps_t timestamps;
} vtss_appl_macsec_rx_sa_status_t;

/**
 * Status of a particular Rx Secure Channel
 */
typedef struct {
    /**
     * True if installed in H/W, false if not.
     * If not, the remaining fields are invalid.
     */
    mesa_bool_t installed;

    /**
     * True if at least one of the SAs are in use.
     */
    mesa_bool_t receiving;

    /**
     * Indicates which - if any - SA within this Rx SC is active.
     *
     * The value VTSS_APPL_MACSEC_SA_IDX_NONE indicates that no SA is active.
     */
    uint32_t active_sa_idx;

    /**
     * Timestamps of events for this Rx SC
     */
    vtss_appl_macsec_timestamps_t timestamps;

    /**
     * Status of this SecY's Rx SC's SAs.
     * There is one such for each possible SA. Use
     * vtss_appl_macsec_interface_capabilities_t::rx_sa_cnt_per_sc_max to find
     * the upper limit for this particular port.
     */
    vtss_appl_macsec_rx_sa_status_t sa[VTSS_APPL_MACSEC_RX_SA_CNT_PER_SC_MAX];
} vtss_appl_macsec_rx_sc_status_t;

/**
 * Status of a particular SecY's controlled port.
 */
typedef struct {
    /**
     * This one contains the number of seconds the switch has been up since
     * boot.
     * This is useful if you wish to convert vtss_appl_macsec_timestamps_t
     * members to a relative time.
     */
    uint32_t uptime_seconds;

    /**
     * True if installed in H/W, false if not.
     *
     * If false, the remaining fields are invalid.
     */
    mesa_bool_t installed;

    /**
     * Corresponds to 802.1AE MAC_Enabled (clause 6.4 and 10.7.4).
     *
     * This is true if the controlled port is enabled and a Tx SA and an Rx SA
     * are in use.
     */
    mesa_bool_t mac_enabled;

    /**
     * Corresponds to 802.1AE MAC_Operational (clause 6.4 and 10.7.4).
     *
     * This is true if \p mac_enabled is true and the Tx SA's next packet number
     * is not 0 or 0xFFFFFFFF.
     */
    mesa_bool_t mac_operational;

    /**
     * Corresponds to 802.1AE operPointToPointMAC (clause 6.5 and 10.7.4).
     *
     * This is true if and only if
     * 1) We are running strict frame validation and at most one Rx SC is in use,
     *    or
     * 2) We are not running strict frame validation, but running full duplex on
     *    the link.
     */
    mesa_bool_t oper_point_to_point_mac;

    /**
     * Status of this SecY's Tx SC.
     */
    vtss_appl_macsec_tx_sc_status_t tx_sc;

    /**
     * Status of this SecY's Rx SCs.
     * Use vtss_appl_macsec_interface_capabilities_t::rx_sc_cnt_per_secy_max to
     * find the upper limit for this particular port.
     */
    vtss_appl_macsec_rx_sc_status_t rx_sc[VTSS_APPL_MACSEC_RX_SC_CNT_MAX];
} vtss_appl_macsec_secy_status_t;

/**
 * Get status of a SecY.
 *
 * \param secy_key    [IN]  SecY to get status for
 * \param secy_status [OUT] Pointer to structure receiving SecY's status.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_macsec_secy_status_get(const vtss_appl_macsec_secy_key_t &secy_key, vtss_appl_macsec_secy_status_t *secy_status);

/**
 * MACsec counters.
 * Unfortunately, the MACsec API defines separate structures with identical
 * members. We unify them in the application.
 */
typedef struct {
    uint64_t if_in_pkts;            /**< In packets     */
    uint64_t if_in_octets;          /**< In octets      */
    uint64_t if_in_ucast_pkts;      /**< In unicasts    */
    uint64_t if_in_multicast_pkts;  /**< In multicasts  */
    uint64_t if_in_broadcast_pkts;  /**< In broadcasts  */
    uint64_t if_in_discards;        /**< In discards    */
    uint64_t if_in_errors;          /**< In errors      */
    uint64_t if_out_pkts;           /**< Out packets    */
    uint64_t if_out_octets;         /**< Out octets     */
    uint64_t if_out_ucast_pkts;     /**< Out unicasts   */
    uint64_t if_out_multicast_pkts; /**< Out multicasts */
    uint64_t if_out_broadcast_pkts; /**< Out broadcasts */
    uint64_t if_out_errors;         /**< Out errors     */
} vtss_appl_macsec_counters_t;

/**
 * Port statistics
 */
typedef struct {
    /**
     * Uncontrolled port statistics
     */
    vtss_appl_macsec_counters_t uncontrolled;

    /**
     * Common port statistics
     */
    vtss_appl_macsec_counters_t common;
} vtss_appl_macsec_interface_statistics_t;

/**
 * Get port statistics.
 *
 * \param ifindex    [IN]  Port to get MACsec statistics for.
 * \param statistics [OUT] Pointer to structure receiving the port's statistics
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_macsec_interface_statistics_get(vtss_ifindex_t ifindex, vtss_appl_macsec_interface_statistics_t *statistics);

/**
 * Tx SC statistics
 */
typedef struct {
    /**
     * SecY's Tx SC statistics.
     */
    mepa_macsec_tx_sc_counters_t sc;

    /**
     * SecY's Tx SC's SA statistics.
     * There is one such for each possible SA. Use
     * vtss_appl_macsec_interface_capabilities_t::tx_sa_cnt_per_sc_max to find
     * the upper limit for this particular port.
     */
    mepa_macsec_tx_sa_counters_t sa[VTSS_APPL_MACSEC_TX_SA_CNT_PER_SC_MAX];
} vtss_appl_macsec_tx_sc_statistics_t;

/**
 * Rx SC statistics
 */
typedef struct {
    /**
     * SecY's Rx SC statistics.
     * There is one such for each possible Rx SC. Use
     * vtss_appl_macsec_interface_capabilities_t::rx_sc_cnt_per_secy_max to find
     * the upper limit for this particular port.
     */
    mepa_macsec_rx_sc_counters_t sc;

    /**
     * SecY's Rx SC's SA statistics.
     * There is one such for each possible SA. Use
     * vtss_appl_macsec_interface_capabilities_t::rx_sa_cnt_per_sc_max to find
     * the upper limit for this particular port.
     */
    mepa_macsec_rx_sa_counters_t sa[VTSS_APPL_MACSEC_RX_SA_CNT_PER_SC_MAX];
} vtss_appl_macsec_rx_sc_statistics_t;

/**
 * SecY statistics.
 */
typedef struct {
    /**
     * Controlled statistics
     */
    vtss_appl_macsec_counters_t controlled;

    /**
     * SecY statistics
     */
    mepa_macsec_secy_counters_t secy;

    /**
     * SecY's Tx SC statistics
     */
    vtss_appl_macsec_tx_sc_statistics_t tx_sc;

    /**
     * SecY's Rx SC statistics.
     * There is one such for each possible Rx SC. Use
     * vtss_appl_macsec_interface_capabilities_t::rx_sc_cnt_per_secy_max to find
     * the upper limit for this particular port.
     */
    vtss_appl_macsec_rx_sc_statistics_t rx_sc[VTSS_APPL_MACSEC_RX_SC_CNT_MAX];
} vtss_appl_macsec_secy_statistics_t;

/**
 * Get SecY statistics.
 *
 * \param secy_key   [IN]  The SecY key to get the SecY configuration for.
 * \param statistics [OUT] Pointer to structure receiving the port's SecY statistics.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_macsec_secy_statistics_get(const vtss_appl_macsec_secy_key_t &secy_key, vtss_appl_macsec_secy_statistics_t *statistics);

/**
 * Enumeration for controlling which counters to clear.
 */
typedef enum {
    VTSS_APPL_MACSEC_STATISTICS_CLEAR_CONTROLLED,    /**< Clear controlled port Tx and Rx statistics, only                */
    VTSS_APPL_MACSEC_STATISTICS_CLEAR_CONTROLLED_TX, /**< Clear controlled port Tx statistics, only                       */
    VTSS_APPL_MACSEC_STATISTICS_CLEAR_CONTROLLED_RX, /**< Clear controlled port Rx statistics, only                       */
    VTSS_APPL_MACSEC_STATISTICS_CLEAR_UNCONTROLLED,  /**< Clear uncontrolled port statistics, only                        */
    VTSS_APPL_MACSEC_STATISTICS_CLEAR_COMMON,        /**< Clear common port statistics, only                              */
    VTSS_APPL_MACSEC_STATISTICS_CLEAR_ALL,           /**< Clear both controlled, uncontrolled, and common port statistics */
} vtss_appl_macsec_statistics_clear_what_t;

/**
 * Statistics may be cleared in several ways - controlled by this structure.
 */
struct vtss_appl_macsec_statistics_clear_control_t {

    /**
     * Default constructor, selecting clearing of all counters.
     */
    vtss_appl_macsec_statistics_clear_control_t(void) :
        what(VTSS_APPL_MACSEC_STATISTICS_CLEAR_ALL),
        secy_id(VTSS_APPL_MACSEC_SECY_ID_NONE),
        rx_sc_idx(VTSS_APPL_MACSEC_RX_SC_IDX_NONE),
        sa_idx(VTSS_APPL_MACSEC_SA_IDX_NONE) {};

    /**
     * Controls which counters to clear.
     */
    vtss_appl_macsec_statistics_clear_what_t what;

    /**
     * This one is only used if \p what is
     * VTSS_APPL_MACSEC_STATISTICS_CLEAR_CONTROLLED,
     * VTSS_APPL_MACSEC_STATISTICS_CLEAR_CONTROLLED_TX, or
     * VTSS_APPL_MACSEC_STATISTICS_CLEAR_CONTROLLED_RX.
     *
     * In that case, it indicates the SecY to clear statistics for. Use
     * VTSS_APPL_MACSEC_SECY_ID_NONE to clear all SecYs.
     *
     * Per-SA granularity can be achieved by combining this with a particular
     * SA index (see \p sa_idx).
     */
    vtss_appl_macsec_secy_id_t secy_id;

    /**
     * This one is only used if \p what is
     * VTSS_APPL_MACSEC_STATISTICS_CLEAR_CONTROLLED_RX.
     *
     * In that case, it indicates the Rx SC to clear statistics for. Use
     * VTSS_APPL_MACSEC_RX_SC_IDX_NONE to clear all Rx SCs.
     *
     * Per-SA granularity can be achieved by combining this with a particular
     * SA index (see \p sa_idx).
     */
    size_t rx_sc_idx;

    /**
     * This one is only used if \p what is
     * VTSS_APPL_MACSEC_STATISTICS_CLEAR_CONTROLLED_TX or
     * VTSS_APPL_MACSEC_STATISTICS_CLEAR_CONTROLLED_RX.
     *
     * In that case, it indicates the SA to clear counters for.
     */
    size_t sa_idx;
};

/**
 * Clear port statistics.
 *
 * \param ifindex [IN] Port to clear MACsec statistics for.
 * \param control [IN] Selects what to clear.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_macsec_statistics_clear(vtss_ifindex_t ifindex, const vtss_appl_macsec_statistics_clear_control_t *control);

#endif /* _VTSS_APPL_MACSEC_HXX_ */

