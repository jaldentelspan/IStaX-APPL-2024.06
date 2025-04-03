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

#ifndef _PACKET_API_H_
#define _PACKET_API_H_

/****************************************************************************/
/*  Public                                                                  */
/****************************************************************************/
#include "microchip/ethernet/switch/api.h"
#include "main.h"

#define SSP_PROT_EPID  2
#define ETYPE_IPV4     0x0800
#define ETYPE_IPV6     0x86DD
#define ETYPE_ARP      0x0806
#define ETYPE_RARP     0x8035

#define IP_PROTO_ICMP      1
#define IP_PROTO_IGMP      2
#define IP_PROTO_TCP       6
#define IP_PROTO_UDP      17

#define TCP_PROTO_SSH     22
#if defined(CLI_TELNET_PORT)
#define TCP_PROTO_TELNET  CLI_TELNET_PORT
#else
#define TCP_PROTO_TELNET  23
#endif
#define TCP_PROTO_DNS     53
#define TCP_PROTO_HTTP    80
#define TCP_PROTO_HTTPS  443

#define UDP_PROTO_DNS     53
#define UDP_PROTO_SNMP   161

#define FCS_SIZE_BYTES         4              /* Frame Check Sequence - FCS        */
#define PACKET_RX_MTU_DEFAULT  (1518 + 2 * 4) /* Max Etherframe + a couple of tags */

/* The highest queue number has the highest priority */

#if VTSS_PACKET_RX_QUEUE_CNT < 8
#error "Code expects at least 8 frame extraction queues"
#endif

#define PACKET_XTR_QU_LOWEST  (VTSS_PACKET_RX_QUEUE_START + 0)
#define PACKET_XTR_QU_LOWER   (VTSS_PACKET_RX_QUEUE_START + 1)
#define PACKET_XTR_QU_LOW     (VTSS_PACKET_RX_QUEUE_START + 2)
#define PACKET_XTR_QU_NORMAL  (VTSS_PACKET_RX_QUEUE_START + 3)
#define PACKET_XTR_QU_MEDIUM  (VTSS_PACKET_RX_QUEUE_START + 4)
#define PACKET_XTR_QU_HIGH    (VTSS_PACKET_RX_QUEUE_START + 5)
#define PACKET_XTR_QU_HIGHER  (VTSS_PACKET_RX_QUEUE_START + 6)
#define PACKET_XTR_QU_HIGHEST (VTSS_PACKET_RX_QUEUE_START + 7)

// Extraction Queue Allocation
#define PACKET_XTR_QU_LRN_ALL   PACKET_XTR_QU_LOWEST  /* Only Learn-All frames end up in this queue (JR Stacking + JR-48 standalone). */
#define PACKET_XTR_QU_SFLOW     PACKET_XTR_QU_LOWER   /* Only sFlow-marked frames must be forwarded on this queue. If not, other modules will not be able to receive the frame. */
#define PACKET_XTR_QU_ACL_COPY  PACKET_XTR_QU_LOW     /* For ACEs with CPU copy                          */
#define PACKET_XTR_QU_LEARN     PACKET_XTR_QU_LOW     /* For the sake of MAC-based Authentication        */
#define PACKET_XTR_QU_BC        PACKET_XTR_QU_LOW     /* For Broadcast MAC address frames                */
#define PACKET_XTR_QU_MAC       PACKET_XTR_QU_LOW     /* For other MAC addresses that require CPU copies */
#define PACKET_XTR_QU_L2CP      PACKET_XTR_QU_NORMAL  /* For L2CP frames                                 */
#define PACKET_XTR_QU_OAM       PACKET_XTR_QU_NORMAL  /* For OAM frames                                  */
#define PACKET_XTR_QU_L3_OTHER  PACKET_XTR_QU_NORMAL  /* For L3 frames with errors or TTL expiration     */
#define PACKET_XTR_QU_REDBOX    PACKET_XTR_QU_NORMAL  /* For RedBox frames                               */
#define PACKET_XTR_QU_MGMT_MAC  PACKET_XTR_QU_MEDIUM  /* For the switch's own MAC address                */
#define PACKET_XTR_QU_IGMP      PACKET_XTR_QU_HIGH    /* For IP multicast frames                         */
#define PACKET_XTR_QU_BPDU      PACKET_XTR_QU_HIGHER  /* For BPDUs                                       */
#define PACKET_XTR_QU_STACK     PACKET_XTR_QU_HIGHEST /* For Message module frames                       */
#define PACKET_XTR_QU_SPROUT    PACKET_XTR_QU_HIGHEST /* For SPROUT frames on archs not using super-prio */

// Use the same queue as frames that hit an ACL rule that caused
// the frame to be copied (not redirected) to the CPU
#define PACKET_XTR_QU_ACL_REDIR PACKET_XTR_QU_ACL_COPY  /* For ACEs with CPU redirection */

// Extraction rate for each of 4 policers in pps.
// The total rate to the CPU must be less than approximaly 200 Mbps.
// With 4000 pps and maximum size frames on all 4 policers,
// we will get 4*1518*8*4000 bps = 194 Mbps
#define PACKET_XTR_POLICER_RATE 4000

#ifdef __cplusplus
extern "C" {
#endif

/* Packet Module error codes (mesa_rc) */
typedef enum {
    PACKET_RC_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_PACKET),  // Generic error
    PACKET_RC_PARAM,                                            // Illegal parameter
    PACKET_RC_TX_CHECK,                                         // packet_tx() was called with invalid tx_props
    PACKET_RC_FDMA_TX,                                          // vtss_fdma_tx() failed.
    PACKET_RC_FDMA_AFI_CANCEL_FRAME_NOT_FOUND,                  // packet_afi_cancel(): FDMA didn't find this frame to cancel.
    PACKET_RC_THROTTLING_NOT_SUPPORTED,                         // packet_rx_throttle_cfg_get/set() not supported on this platform (external CPU without CPU queue shaper H/W support)
} packet_error_t;

/****************************************************************************/
// PACKET_RX_FILTER_PRIO_xxx definitions
/****************************************************************************/
#define PACKET_RX_FILTER_PRIO_SUPER            10
#define PACKET_RX_FILTER_PRIO_HIGH            100
#define PACKET_RX_FILTER_PRIO_NORMAL         1000
#define PACKET_RX_FILTER_PRIO_BELOW_NORMAL  10000
#define PACKET_RX_FILTER_PRIO_LOW          100000

/****************************************************************************/
// Fields to match on are:
//   * IFH.source_port
//   * IFH.frame_type (for ACLs)
//   * IFH.vid
//   * IFH.learn
//   * DMAC
//   * EtherType
//   * IP.Proto (for EtherType == 0x0800)
//   * SSPID (for EtherType == 0x8880 and EPID == 0x0002)
//   * UDP/TCP Port number
// This results in the following match flags for the API, that may be ORed
// together to form a complex match. Some of the flags are mutually exclusive
// and some are implied by the use of others.
// Special note about PACKET_RX_FILTER_MATCH_ANY:
//   If this is used, no other flags may be used (obviously). A subscriber
//   using this flag will get all packets (can be used by e.g. Ethereal).
// Special note about PACKET_RX_FILTER_MATCH_DEFAULT:
//   A subscriber using this flag will get all packets not matching any other
//   subscriptions. This flag may only be used alone.
// Special note about PACKET_RX_FILTER_MATCH_VLAN_TAG_ANY
//   This only works when PACKET_RX_FILTER_MATCH_IP_ANY is also set.
//   When set, it will allow zero or one C, S-, or S-custom tags in the frame
//   before an IPv4, IPv6, or ARP ethertype. The FDMA driver must have
//   stripped one outer tag in order for this to take effect. The frame will
//   be handed over to the subscriber with both tags stripped.
//   This flag can be used to manage the switch through a tunnel, where
//   management frames will be double tagged and classified to outer tag.
//   The flag will not work with S-Custom-tagged frames on secondary switches in
//   a stack.
/****************************************************************************/
#define PACKET_RX_FILTER_MATCH_ANY             0x00000000 /* packet_rx_filter_t member(s): None                               */
#define PACKET_RX_FILTER_MATCH_SRC_PORT        0x00000001 /* packet_rx_filter_t member(s): src_port_mask                      */
#define PACKET_RX_FILTER_MATCH_ACL             0x00000002 /* packet_rx_filter_t member(s): None                               */
#define PACKET_RX_FILTER_MATCH_VID             0x00000004 /* packet_rx_filter_t member(s): vid, vid_mask                      */
#define PACKET_RX_FILTER_MATCH_FREE_TO_USE     0x00000008 /* Unused                                                           */
#define PACKET_RX_FILTER_MATCH_DMAC            0x00000010 /* packet_rx_filter_t member(s): dmac, dmac_mask                    */
#define PACKET_RX_FILTER_MATCH_SMAC            0x00000020 /* packet_rx_filter_t member(s): smac, smac_mask                    */
#define PACKET_RX_FILTER_MATCH_ETYPE           0x00000040 /* packet_rx_filter_t member(s): etype, etype_mask                  */
#define PACKET_RX_FILTER_MATCH_IP_PROTO        0x00000080 /* packet_rx_filter_t member(s): ip_proto, ip_proto_mask            */
#define PACKET_RX_FILTER_MATCH_SSPID           0x00000100 /* packet_rx_filter_t member(s): sspid, sspid_mask                  */
#define PACKET_RX_FILTER_MATCH_SFLOW           0x00000200 /* packet_rx_filter_t member(s): None                               */
#define PACKET_RX_FILTER_MATCH_IP_ANY          0x00000400 /* packet_rx_filter_t member(s): None. Matches IPv4, IPv6, ARP      */
#define PACKET_RX_FILTER_MATCH_UDP_SRC_PORT    0x00000800 /* packet_rx_filter_t member(s): udp_src_port_min, udp_src_port_max */
#define PACKET_RX_FILTER_MATCH_UDP_DST_PORT    0x00001000 /* packet_rx_filter_t member(s): udp_dst_port_min, udp_dst_port_max */
#define PACKET_RX_FILTER_MATCH_TCP_SRC_PORT    0x00002000 /* packet_rx_filter_t member(s): tcp_src_port_min, tcp_src_port_max */
#define PACKET_RX_FILTER_MATCH_TCP_DST_PORT    0x00004000 /* packet_rx_filter_t member(s): tcp_dst_port_min, tcp_dst_port_max */
#define PACKET_RX_FILTER_MATCH_VLAN_TAG_ANY    0x00008000 /* packet_rx_filter_t member(s): None. Only works with IP_ANY       */
#define PACKET_RX_FILTER_MATCH_DEFAULT         0x80000000 /* packet_rx_filter_t member(s): None.                              */

// The following structure contains masks and what to match against.
// It MUST be initialized with a call to packet_rx_filter_init().
typedef struct {
    //
    // GENERAL CONFIGURATION
    //

    // Module ID. Only used to find the name of the subscription module when
    // printing statistics.
    // Validity:
    //   Luton26: Y
    //   Jaguar1: Y
    //   Serval : Y
    vtss_module_id_t modid;

    // Thread priority. Determines the thread priority on which the frame will be
    // dispatched to the user module.
    vtss_thread_prio_t thread_prio;

    // Maximum frame size (incl. FCS, but excluding IFH) that the caller can
    // accept. When using MSCC's build-root SDK, this can be set up to 16Kbytes.
    // When not using MSCC's build-root SDK, it can be set to at most
    // PACKET_RX_MTU_DEFAULT bytes.
    u32 mtu;

    // Match flags. Bitwise OR the PACKET_RX_FILTER_MATCH_xxx to form the final
    // filter.
    // Validity:
    //   Luton26: Y
    //   Jaguar1: Y
    //   Serval : Y
    u32 match;

    // Priority. The lower this number, the higher priority. Use
    // the PACKET_RX_FILTER_PRIO_xxx definitions.
    // Do not set this to 0. This will cause the packet module to issue an
    // error. You really need to think about a real priority.
    // Validity:
    //   Luton26: Y
    //   Jaguar1: Y
    //   Serval : Y
    u32 prio;

    // User-defined info. Supplied when calling back.
    //   Luton26: Y
    //   Jaguar1: Y
    //   Serval : Y
    void *contxt;

    // Callback function called when a match occurs.
    // Arguments:
    //   #contxt  : User-defined (the value of this structure's .contxt member).
    //   #frm     : Points to the first byte of the DMAC of the frame.
    //   #rx_info : Various frame properties, a.o. length, source port, VID.
    // The function MUST return
    //   TRUE : If the frame is consumed for good and shouldn't be dispatched
    //          to other subscribers.
    //   FALSE: If the frame is allowed to be dispatched to other subscribers.
    // NOTE 1): The callback function is NOT allowed in anyway to modify the packet.
    // NOTE 2): The callback function *is* allowed to call packet_rx_filter_XXX().
    //   Luton26: Y
    //   Jaguar1: Y
    //   Serval : Y
    BOOL (*cb)(void *contxt, const u8 *const frm, const mesa_packet_rx_info_t *const rx_info);

    //
    // FILTER CONFIGURATION
    //
    // Note that most members have a corresponding mask. If this mask is
    // all-zeros, all bits must match (inverse polarity). The rule is as
    // follows:
    // There's a match if (packet->fld & ~.fld_mask) == .fld,
    // where fld is either of the following that have a corresponding mask.
    // The reason for the inverse polarity is that most modules will ask
    // for a particular match, not a wild card match, so most modules will
    // have the compiler insert zeros for uninitialized fields.
    // Furthermore, since we have the .match member, future extensions to
    // this structure will not affect present code.

    // If PACKET_RX_FILTER_MATCH_SRC_PORT is set, a match against .src_port
    // is made. .src_port is actually a bitmask with a bit set for each
    // logical port it wishes to match. You may use the VTSS_PORT_BF_xxx macros
    // defined in main.h for this.
    //   Luton26: Y
    //   Jaguar1: Y
    //   Serval : Y
    u8 src_port_mask[VTSS_PORT_BF_SIZE];

    // If PACKET_RX_FILTER_MATCH_VID is set, a match against .vid is made.
    // At most the 12 LSBits are used in the match.
    //   Luton26: Y
    //   Jaguar1: Y
    //   Serval : Y
    u16 vid;
    u16 vid_mask;

    // If PACKET_RX_FILTER_MATCH_DMAC is set, a match like the following is made:
    // if ((packet->dmac & ~.dmac_mask) == .dmac) hit();
    //   Luton26: Y
    //   Jaguar1: Y
    //   Serval : Y
    u8 dmac[6];
    u8 dmac_mask[6];

    // If PACKET_RX_FILTER_MATCH_SMAC is set, a match like the following is made:
    // if ((packet->smac & ~.smac_mask) == .smac) hit();
    //   Luton26: Y
    //   Jaguar1: Y
    //   Serval : Y
    u8 smac[6];
    u8 smac_mask[6];

    // If PACKET_RX_FILTER_MATCH_ETYPE is set, a match against .etype is made.
    //   Luton26: Y
    //   Jaguar1: Y
    //   Serval : Y
    u16 etype;
    u16 etype_mask;

    // If PACKET_RX_FILTER_MATCH_IP_PROTO is set (this requires a PACKET_RX_FILTER_MATCH_ETYPE
    // with .etype == ETYPE_IPV4 or .etype == ETYPE_IPV6), then a match against
    // this protocol is made. For IPv6, only the first NEXT_HDR field is matched against, that is,
    // possible extension headers are NOT traversed.
    //   Luton26: Y
    //   Jaguar1: Y
    //   Serval : Y
    u8 ip_proto;
    u8 ip_proto_mask;

    // If PACKET_RX_FILTER_MATCH_UDP_SRC_PORT is set then a match against the following range
    // of UDP source port numbers is made (both ends included). Set both min and max to the
    // same value to match on only one port number.
    // Note: Setting PACKET_RX_FILTER_MATCH_UDP_SRC_PORT causes the packet module to automatically insert
    //   PACKET_RX_FILTER_MATCH_IP_PROTO with .ip_proto == 17 (IP_PROTO_UDP)
    // Supported platforms:
    //   Luton26: Y
    //   Jaguar1: Y
    //   Serval : Y
    u16 udp_src_port_min;
    u16 udp_src_port_max;

    // If PACKET_RX_FILTER_MATCH_UDP_DST_PORT is set then a match against the following range
    // of UDP destination port numbers is made (both ends included). Set both min and max to the
    // same value to match on only one port number.
    // Note: Setting PACKET_RX_FILTER_MATCH_UDP_DST_PORT causes the packet module to automatically insert
    //   PACKET_RX_FILTER_MATCH_IP_PROTO with .ip_proto == 17 (IP_PROTO_UDP)
    // Supported platforms:
    //   Luton26: Y
    //   Jaguar1: Y
    //   Serval : Y
    u16 udp_dst_port_min;
    u16 udp_dst_port_max;

    // If PACKET_RX_FILTER_MATCH_TCP_SRC_PORT is set then a match against the following range
    // of TCP source port numbers is made (both ends included). Set both min and max to the
    // same value to match on only one port number.
    // Note: Setting PACKET_RX_FILTER_MATCH_TCP_SRC_PORT causes the packet module to automatically insert
    //   PACKET_RX_FILTER_MATCH_IP_PROTO with .ip_proto == 6 (IP_PROTO_TCP)
    // Supported platforms:
    //   Luton26: Y
    //   Jaguar1: Y
    //   Serval : Y
    u16 tcp_src_port_min;
    u16 tcp_src_port_max;

    // If PACKET_RX_FILTER_MATCH_TCP_DST_PORT is set then a match against the following range
    // of TCP destination port numbers is made (both ends included). Set both min and max to the
    // same value to match on only one port number.
    // Note: Setting PACKET_RX_FILTER_MATCH_TCP_DST_PORT causes the packet module to automatically insert
    //   PACKET_RX_FILTER_MATCH_IP_PROTO with .ip_proto == 6 (IP_PROTO_TCP)
    // Supported platforms:
    //   Luton26: Y
    //   Jaguar1: Y
    //   Serval : Y
    u16 tcp_dst_port_min;
    u16 tcp_dst_port_max;

    // If PACKET_RX_FILTER_MATCH_SSPID is set (this implicitly implies
    // PACKET_RX_FILTER_MATCH_ETYPE with .etype=0x8880 and EPID == 0x0002)
    //   Luton26: N
    //   Jaguar1: Y
    //   Serval : Y
    u16 sspid;
    u16 sspid_mask;

} packet_rx_filter_t;

// Initialize a filter. It can only fail if #filter is NULL in which case a trace
// error will be printed, so don't bother about a mesa_rc return code, since it'll
// only clutter the code.
void packet_rx_filter_init(packet_rx_filter_t *filter);

// Register a filter. The filter may be allocated on the
// stack, because packet_rx_filter_register() makes a copy
// of it, so that the caller cannot change the filter
// behind this file's back. The function returns an ID
// that may be used later to unregister or change
// the subscription.
mesa_rc packet_rx_filter_register(const packet_rx_filter_t *filter, void **filter_id);

// Unregister an existing filter.
mesa_rc packet_rx_filter_unregister(void *filter_id);

// Change an existing filter
mesa_rc packet_rx_filter_change(const packet_rx_filter_t *filter, void **filter_id);

/**
 * \brief Packet Tx Filter.
 *
 * Describes ingress and egress properties
 * to be used when the packet_tx() function
 * must assess whether or not to insert a VLAN tag
 * in the frame prior to transmission on a given
 * front port. In some cases, the packet_tx() must
 * even discard the frame (if for instance the ingress
 * aggregation is equal to the egress aggregation).
 */
typedef struct {
    /**
     * Set this member to TRUE to enable filtering
     * and destination-port tag info lookup.
     * The remaining members of this structure are not
     * are not used if this is set to FALSE.
     *
     * The lookup is a pretty expensive operation, so use
     * this capability with care.
     */
    BOOL enable;

    /**
     * The port that this frame originally arrived on.
     * This is used to discard the frame in case the ingress
     * port is not in forwarding mode, or if the port is not
     * a member of the VLAN ID specified with
     * packet_tx_props_t::vid (!= VTSS_VID_NULL),
     * or if the ingress port equals the egress port specified
     * with packet_tx_props_t::dst_port, or if the ingress
     * and egress ports are members of the same LLAG.
     *
     * Ingress filtering may be turned off by setting #src_port
     * to VTSS_PORT_NO_NONE.
     */
    mesa_port_no_t src_port;

} packet_tx_filter_t;

/**
 * \brief Packet Tx Info
 */
typedef struct {
    /**
     * Set your module's ID here, or leave it as is if your
     * module doesn't have a module ID.
     * The module ID is used for providing per-module
     * statistics, and is mainly for debugging.
     */
    vtss_module_id_t modid;

    /**
     * Pointer to the actual frame data.
     *
     * frm is expected to point to the first of the DMAC.
     */
    u8 *frm;

    /**
     * This member contains lengths of the frame specified with the #frm member
     * array. This is excluding IFH and FCS and must be >= 14 bytes.
     */
    size_t len;

    /**
     * Controls whether or not packet_tx() invokes packet_tx_free() upon
     * injection.
     * FALSE if packet_tx() calls packet_tx_free(). In this case, the buffer
     * must not be used afterwards.
     * TRUE if packet_tx() does not call packet_tx_free(). In this case, the
     * caller of packet_tx() may alter the buffer afterwards and make sure he
     * frees it with packet_tx_free().
     */
    BOOL no_free;

    /**
     * Under certain circumstances, a user module may wish to send a frame
     * directed to a front port, but with a VLAN tag that matches the front
     * port's VLAN properties (tag type, membership, untagged VID, etc.).
     * This #filter property allows for that.
     *
     * #filter.enable must be set to TRUE to enable this feature.
     * If enabled, #switch_frm must be FALSE and #dst_port_mask must be 0, that is,
     * only one single frame can be transmitted at a time.
     * Furthermore, #dst_port must not be a stack port, and #contains_stack_hdr must be FALSE.
     *
     * The lookup of egress properties is based on the port given by this structure's
     * #dst_port and using the egress VID given by this structure's #vid member.
     *
     * For further use of the filter, please refer to its definition above.
     *
     * NOTICE: If the filter gives rise to discarding of the frame, packet_tx() will
     *         return VTSS_RC_INV_STATE. The caller must remember to deallocate
     *         resources if this happens.
     *
     * NOTICE: If packet_tx() returns VTSS_RC_OK, there is a chance that the frame
     *         has been modified prior to transmission (insertion of a VLAN tag).
     *         This means that you cannot use the same copy of the frame in a tight
     *         loop that transmits to multiple ports. You *must* allocate a new copy
     *         for each port you transmit to.
     */
    packet_tx_filter_t filter;

    /**
     * Internal flags. Don't use.
     */
    u32 internal_flags;

} packet_tx_info_t;

/**
 * \brief Packet Tx Properties.
 *
 * Properties on how to transmit a frame using packet_tx().
 * This must be initialized by a call to packet_tx_props_init().
 * The structure can be allocated on the stack.
 */
typedef struct {

    /**
     * Tx Properties as defined by the underlying VTSS API
     */
    mesa_packet_tx_info_t tx_info;

    /**
     * Tx properties needed by the packet module
     */
    packet_tx_info_t packet_info;

} packet_tx_props_t;

/******************************************************************************/
// packet_tx_props_init()
// Initialize a packet_tx_props_t structure.
/******************************************************************************/
void packet_tx_props_init(packet_tx_props_t *tx_props);

/**
 * \brief Transmit frame.
 *
 * Transmit a frame using the propeties set in #tx_props.
 * Refer to definition of #packet_tx_props_t for a description of
 * transmission options.
 * #tx_props may be allocated on the stack, but it must be initialized by a
 * call to packet_tx_props_init().
 *
 * \return
 *    VTSS_RC_OK on success\n
 *    VTSS_RC_ERROR on invalid combination of #tx_props or if FIFO is full.
 *    VTSS_RC_INV_STATE if #packet_tx_props_t::filter gave rise to a discard of this frame.
 */
mesa_rc packet_tx(packet_tx_props_t *tx_props);

/******************************************************************************/
// Tx buffer alloc & free.
// Args:
//   #size : Size excluding IFH, possible stack header, and FCS
// Returns:
//   Pointer to location that the DMAC should be stored or NULL on out-of-memory.
/******************************************************************************/
u8 *packet_tx_alloc(size_t size);
void packet_tx_free(u8 *buffer);

/******************************************************************************/
// packet_tx_alloc_extra()
// The difference between this function and the packet_tx_alloc() function is
// that the user is able to reserve a number of 32-bit words at the beginning
// of the packet, which is useful when some state must be saved between the call
// to the packet_tx() function and the user-defined callback function.
// Args:
//   #size              : Size exluding IFH and FCS
//   #extra_size_dwords : Number of 32-bit words to reserve room for.
//   #extra_ptr         : Pointer that after the call will contain the pointer to the additional space.
// Returns:
//   Pointer to location that the DMAC should be stored or NULL on out-of-space.
// Use packet_tx_free_extra() when freeing the packet rather than packet_tx_free().
/******************************************************************************/
u8 *packet_tx_alloc_extra(size_t size, size_t extra_size_dwords, u8 **extra_ptr);

/******************************************************************************/
// packet_tx_free_extra()
// This function is the counter-part to the packet_tx_alloc_extra() function.
// It must be called with the value returned in packet_tx_alloc_extra()'s
// extra_ptr argument.
/******************************************************************************/
void packet_tx_free_extra(u8 *extra_ptr);

/******************************************************************************/
// packet_rx_queue_usage()
/******************************************************************************/
char *packet_rx_queue_usage(u32 xtr_qu, char *buf, size_t size);

typedef int (*packet_dbg_printf_t)(const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));

/****************************************************************************/
// packet_dbg()
// Entry point to Packet Module Debug features. Should be called from
// cli only.
/****************************************************************************/
void packet_dbg(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms);

#if defined(MSCC_BRSDK)
// When running on the internal CPU, we have a throttling interface at hand.

// Throttling configuration
typedef struct {
    /**
     * Controls - per extraction queue - the maximum number of frames
     * extracted between two calls to vtss_fdma_throttle_tick() without
     * suspending extraction from that queue.
     *
     * If 0, frame count throttling is disabled for that extraction queue.
     */
    u32 frm_limit_per_tick[VTSS_PACKET_RX_QUEUE_CNT];

    /**
     * Controls - per extraction queue - the maximum number of bytes
     * extracted between two calls to vtss_fdma_throttle_tick() without
     * suspending extraction from that queue.
     *
     * If 0, byte count throttling is disabled for that extraction queue.
     */
    u32 byte_limit_per_tick[VTSS_PACKET_RX_QUEUE_CNT];

    /**
     * Controls - per extraction queue - the number of invocations of
     * vtss_fdma_throttle_tick() that must happen before an extraction queue
     * that has been disabled, gets re-enabled.
     *
     * For instance,
     *   a value of 0 means: re-enable the extraction queue on the next tick.
     *   a value of 1 means: re-enable the extraction queue two ticks from when it was suspended.
     */
    u32 suspend_tick_cnt[VTSS_PACKET_RX_QUEUE_CNT];
} packet_throttle_cfg_t;

/******************************************************************************/
// packet_rx_throttle_cfg_get/set()
// Functions only intended for use by CLI debug.
/******************************************************************************/
mesa_rc packet_rx_throttle_cfg_get(      packet_throttle_cfg_t *cfg);
mesa_rc packet_rx_throttle_cfg_set(const packet_throttle_cfg_t *cfg);

// Also, these two defines are not so public, but nice to see in CLI.
#define PACKET_THROTTLE_PERIOD_MS 100
#define PACKET_THROTTLE_FREQ_HZ   (1000 / PACKET_THROTTLE_PERIOD_MS)
#endif /* defined(MSCC_BRSDK) */

#if defined(MSCC_BRSDK)
// When running on the internal CPU, we can change the Rx MTU

// Rx configuration
typedef struct {
    /**
     * Controls the Rx MTU of the uFDMA including FCS, but excluding IFH.
     *
     * The MTU does not affect the MTU of the IP stack, only whether the uFDMA
     * driver forwards the frame to its network device or not.
     *
     * Range of valid values is [64; 16384] bytes.
     */
    u32 mtu;
} packet_rx_cfg_t;

/******************************************************************************/
// packet_rx_cfg_get/set()
// Functions only intended for use by CLI debug.
/******************************************************************************/
mesa_rc packet_rx_cfg_get(packet_rx_cfg_t *cfg);
mesa_rc packet_rx_cfg_set(packet_rx_cfg_t *cfg);

void packet_rx_process_set(bool val);
#endif /* defined(MSCC_BRSDK) */

/******************************************************************************/
// packet_rx_shaping_cfg_get/set()
// Functions only intended for use by CLI debug.
// A bitrate of 0 kbps disables shaping.
/******************************************************************************/
mesa_rc packet_rx_shaping_cfg_get(u32 *l1_rate_kbps);
mesa_rc packet_rx_shaping_cfg_set(u32 l1_rate_kbps);

#if defined(MSCC_BRSDK)
// We can enable trace when the uFDMA is included in the kernel.
void packet_ufdma_trace_update(void);
#endif /* defined(MSCC_BRSDK) */

void packet_debug_tx_props_print(vtss_module_id_t trace_mod, u32 trace_grp, u32 trace_lvl, const packet_tx_props_t *props);
void packet_debug_rx_packet_trace(bool enable);
void packet_debug_tx_packet_trace(bool enable);

/****************************************************************************/
// Module init
/****************************************************************************/
mesa_rc packet_init(vtss_init_data_t *data);

/******************************************************************************/
//
// UTILITY FUNCTIONS
//
/******************************************************************************/

/**
 * Function for converting a packet module error (see PACKETRX_xxx above) to a textual string.
 * Only errors in the packet module's range can be converted.
 *
 * \param rc [IN] Binary form of error
 *
 * \return Static string containing textual representation of #rc.
 */
const char *packet_error_txt(mesa_rc rc);

#ifdef __cplusplus
}
#endif

/******************************************************************************/
// packet_tx_props_t::operator<<()
// Used for tracing
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const packet_tx_props_t &props);

/******************************************************************************/
// packet_tx_props_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static inline size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const packet_tx_props_t *props)
{
    o << *props;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_vlan_tag_t::operator<<()
// Used for tracing
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_vlan_tag_t &tag);

/******************************************************************************/
// mesa_vlan_tag_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static inline size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_vlan_tag_t *tag)
{
    o << *tag;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_packet_rx_info_t::operator<<()
// Used for tracing
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_packet_rx_info_t &info);

/******************************************************************************/
// mesa_packet_rx_info_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static inline size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_packet_rx_info_t *info)
{
    o << *info;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

#endif /* _PACKET_API_H_ */

