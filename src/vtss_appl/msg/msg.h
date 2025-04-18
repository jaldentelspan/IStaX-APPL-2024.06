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

#ifndef _VTSS_MSG_H_
#define _VTSS_MSG_H_

#define MSG_ASSERT(expr, fmt, ...) { \
    if (!(expr)) {                   \
        T_E("ASSERTION FAILED");     \
        T_E(fmt, ##__VA_ARGS__);     \
        VTSS_ASSERT(expr);           \
    }                                \
}

/****************************************************************************/
// Includes
/****************************************************************************/
#include "main.h"         /* For init_cmd_t def */
#include "critd_api.h"
#include "msg_api.h"
#include "topo_api.h"
#include "conf_api.h"      /* For MAC address */
#include "control_api.h"   /* For control_dbg_latest_init_modules_get() */
#include "version.h"       /* For version_string and product_name */
#include <time.h>          /* For time_t typedef */
#include <misc_api.h>      /* For misc_time2str() */
#include "packet_api.h"    /* For Rx and Tx of frames */

/****************************************************************************/
// Useful macros
/****************************************************************************/
#undef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b) ((a) > (b) ? (b) : (a))

/****************************************************************************/
// Run-time default configuration values
/****************************************************************************/
#define MSG_CFG_DEFAULT_MSYN_TIMEOUT_MS              1500
#define MSG_CFG_DEFAULT_MD_TIMEOUT_MS                3500
#define MSG_CFG_DEFAULT_MD_RETRANSMIT_LIMIT             2
#define MSG_CFG_DEFAULT_SEC_WINSZ                       4
#define MSG_CFG_DEFAULT_PRI_WINSZ_PER_SEC              40
#define MSG_CFG_DEFAULT_HTL_LIMIT                      31

/****************************************************************************/
// Compile-time configuration values:
/****************************************************************************/
#define MSG_CFG_CONN_CNT                                1

/****************************************************************************/
// Sample every MSG_SAMPLE_TIME_MS msecs to see if it's time to retransmit
// frames.
/****************************************************************************/
#define MSG_SAMPLE_TIME_MS 500

/****************************************************************************/
// Min/max definitions.
/****************************************************************************/
#define MSG_CFG_MIN_WINSZ               1 /* MDs   */
#define MSG_CFG_MAX_WINSZ             256 /* MDs   */
#define MSG_CFG_MIN_TIMEOUT_MS       MSG_SAMPLE_TIME_MS
#define MSG_CFG_MAX_TIMEOUT_MS       5000
#define MSG_CFG_MAX_RETRANSMIT_LIMIT   20
#define MSG_CFG_MIN_HTL_LIMIT           1
#define MSG_CFG_MAX_HTL_LIMIT         255

// The number of available sequence numbers.
#define MSG_SEQ_CNT 0x10000UL

// MSG_MIN_FRAGSZ: Minimum fragment size, when fragmenting a message.
// By defining a minimum fragment size, we ensure that a message cannot
// span more than MSG_MAX_LEN_BYTES / MSG_MIN_FRAGSZ MDs.
// In the actual case, this is
//   round_up(4,194,303 / 250) = 16778 MDs
// and thereby 16778 SEQ numbers. The following equation must hold:
//   16778 + MSG_CFG_MAX_WINSZ < MSG_SEQ_CNT/2
// or with numbers:
//   16778 + 256 = 17034 < 32768 which holds.
// If this wasn't the case, it would be impossible for the Message Module
// to re-order incoming MDs, because it wouldn't know whether to insert
// the message before of after a previously received, but not yet forwarded,
// message.
#define MSG_MIN_FRAGSZ    250 /* Bytes */

/****************************************************************************/
// Event flags used to wake up the TX_thread and RX_thread.
// There is no real difference between the flags, because the threads treat
// all events the same, but for debugging purposes it's nice to be able to
// see thre reason for waking up the thread.
// Use power-of-2 values only, since they are bits in a 32-bit mask.
/****************************************************************************/
#define MSG_FLAG_TX_MSYN      0x01
#define MSG_FLAG_TX_DONE_MSG  0x02
#define MSG_FLAG_TX_DONE_MD   0x04
#define MSG_FLAG_TX_MORE_MDS  0x08
#define MSG_FLAG_TX_MSG       0x10
#define MSG_FLAG_TX_MORE_WORK 0x20
#define MSG_FLAG_RX_MSG       0x40

/****************************************************************************/
// msg_conn_state_t
// Defines the states that a single connection can live in.
/****************************************************************************/
typedef enum {
    MSG_CONN_STATE_SEC_NO_PRI,                  // State after boot
    MSG_CONN_STATE_SEC_WAIT_FOR_MSYNACKS_MACK,
    MSG_CONN_STATE_SEC_ESTABLISHED,
    MSG_CONN_STATE_PRI_NO_SEC,                  // Secondary switch doesn't exist
    MSG_CONN_STATE_PRI_RDY,                     // Secondary switch exists, MSYNs must be sent
    MSG_CONN_STATE_PRI_WAIT_FOR_MSYNACK,
    MSG_CONN_STATE_PRI_ESTABLISHED,
    MSG_CONN_STATE_PRI_STOP,
} msg_conn_state_t;

/****************************************************************************/
// msg_mod_state_t
// Defines the states that the Message Module as a whole can live in.
/****************************************************************************/
typedef enum {
    MSG_MOD_STATE_SEC, // Wake-up state after boot.
    MSG_MOD_STATE_PRI,

    // IMPORTANT: THIS MUST BE THE LAST ENTRY IN THIS ENUM, BECAUSE IT'S USED TO SIZE AN ARRAY.
    MSG_MOD_STATE_LAST_ENTRY
} msg_mod_state_t;

/****************************************************************************/
// msg_pdu_type_t
// PDU types defined for the message protocol
/****************************************************************************/
typedef enum {
    MSG_PDU_TYPE_MSYN    = 0x1,
    MSG_PDU_TYPE_MSYNACK = 0x2,
    MSG_PDU_TYPE_MD      = 0x3,
    MSG_PDU_TYPE_MACK    = 0x4,
    MSG_PDU_TYPE_MRST    = 0x5,

    // IMPORTANT: THIS MUST BE THE LAST ENTRY IN THIS ENUM, BECAUSE IT'S USED TO SIZE AN ARRAY.
    MSG_PDU_TYPE_LAST_ENTRY
} msg_pdu_type_t;

/****************************************************************************/
/****************************************************************************/
typedef enum {
    MSG_MSYNACK_STATUS_OK                            = 0x00,
    MSG_MSYNACK_STATUS_OK_BUT_ALREADY_CONNECTED_SAME = 0x01,
    MSG_MSYNACK_STATUS_OK_BUT_ALREADY_CONNECTED_DIFF = 0x02,
    MSG_MSYNACK_STATUS_ERR_CID                       = 0x80
} msg_msynack_status_t;

// Values below 0x01 indicate OK. Values above (but less than 0x80) indicate warning.
#define MSG_MSYNACK_STATUS_WRN_LVL 0x01

// Values below 0x80 indicate OK status. Values above indicate error.
#define MSG_MSYNACK_STATUS_ERR_LVL 0x80

/****************************************************************************/
/****************************************************************************/
typedef enum {
    MSG_MACK_STATUS_OK                      = 0x00,
    MSG_MACK_STATUS_OK_BUT_ALREADY_RECEIVED = 0x01,
    MSG_MACK_STATUS_OK_BUT_OUT_OF_ORDER     = 0x02,
    MSG_MACK_STATUS_ERR_OUT_OF_MEMORY       = 0x80  // Can be sent by Secondary switch only
} msg_mack_status_t;

// Values below 0x01 indicate OK. Values above (but less than 0x80) indicate warning.
#define MSG_MACK_STATUS_WRN_LVL 0x01

// Values below 0x80 indicate OK or warning status. Values at or above indicate error.
#define MSG_MACK_STATUS_ERR_LVL 0x80

/****************************************************************************/
// msg_conn_stat_t
// Per-connection statistics counters.
// These are cleared when a new connection is being negotiated.
/****************************************************************************/
typedef struct {
    // -------------------------- RX ------------------------------
    // Number of PDUs received on this connection.
    // The first dimension is the PDU type that was received, of which the
    // first index is for unknown PDU types.
    // The second dimension use the first index ([][0]) for good frames, and
    // the last index ([][1]) for bad frames.
    u32 rx_pdu[MSG_PDU_TYPE_LAST_ENTRY][2];
    u32 rx_msg; // Successfully received user messages.

    // -------------------------- TX ------------------------------
    // Number of PDUs transmitted on this connection.
    // The first dimension is the PDU type that was received, of which
    // index == 0 is unused.
    // The second dimension use the first index ([][0]) for OK TXed frames,
    // and the last index ([][1]) for frames that weren't transmitted
    // because Topo returned invalid DMAC or stack port number, probably
    // due to topology changes.
    u32 tx_pdu[MSG_PDU_TYPE_LAST_ENTRY][2];
    // Likewise, index 0 is for OK Tx, 1 for bad Tx:
    u32 tx_msg[2]; // Successfully/Unsuccessfully transmitted user messages.

    // Payload-only bytes.
    // Retransmissions are also counted.
    u64 tx_md_bytes;

    // Longest roundtrip time, i.e. the max time it took from an MD was
    // sent to the corresponding MACK was received.
    vtss_tick_count_t max_roundtrip_tick_cnt;
} msg_conn_stat_t;

/****************************************************************************/
// msg_mod_per_state_pdu_stat_t
// Module statistics counters. Instantiated once per state (pri/sec).
// Counts PDUs - not User Messages.
// Is only cleared by a CLI debug command.
/****************************************************************************/
typedef struct {
    // -------------------------- RX ------------------------------
    // Number of frames destined for this switch.
    // The first dimension is the PDU type that was received, of which the
    // last index is for unknown PDU types.
    // The second dimension use the first index ([][0]) for good frames, and
    // the last index ([][1]) for bad frames.
    u32 rx[MSG_PDU_TYPE_LAST_ENTRY + 1][2];

    // Payload-only bytes. Index 0 for OK and index 1 for bad MDs.
    // Retransmissions are also counted.
    u64 rx_md_bytes[2]; // Payload-only.

    // -------------------------- TX ------------------------------
    // Number of frames transmitted from this switch.
    // First dimension is the PDU type.
    // Second dimension is for retransmission. Index [][0][] is
    // incremented the first time the PDU is transmitted, Index[][1][]
    // is incremented on subsequent transmissions.
    // The third dimension's first index ([][][0]) is for success, and
    // last index ([][][1]) is for error.
    u32 tx[MSG_PDU_TYPE_LAST_ENTRY][2][2];

    // Payload-only bytes.
    // Retransmissions are also counted.
    u64 tx_md_bytes;

    // If no response to an MD was received before the last retransmission
    // ran out, this one is increased. For a primary switch, the connection is
    // thereafter re-negotiated, for a secondary switch, we simply await the
    // primary switch to transmit another MSYN.
    u32 tx_md_timeouts;
} msg_mod_per_state_pdu_stat_t;

/****************************************************************************/
// msg_mod_per_state_usr_stat_t
// Module statistics counters. Instantiated once per state (pri/sec).
// Counts User Messages - not PDUs.
// Is only cleared by a CLI debug command.
/****************************************************************************/
typedef struct {
    // Maintain a counter per module ID of received and transmitted frames.

    // -------------------------- RX ------------------------------
    // The VTSS_MODULE_ID_NONE is the required length of the array, since only
    // module IDs below this value are assigned. If a frame with a module ID
    // greater than this value is received, it's count is saved in the
    // VTSS_MODULE_ID_NONE entry.
    // The reception is divided into Rx where a callback function was assigned
    // for that module ID, and Rx without that callback function.
    // Entry [][0] are with subscribers, [][1] without.
    u32 rx[VTSS_MODULE_ID_NONE + 1][2];

    // -------------------------- TX ------------------------------
    // Entry [][0] are the good ones, [][1] are the bad ones.
    u32 tx[VTSS_MODULE_ID_NONE + 1][2];

    u32 rxb[VTSS_MODULE_ID_NONE + 1];
    u32 txb[VTSS_MODULE_ID_NONE + 1];

    // The following contains an entry per possible result of a call to
    // msg_tx(). It is not per user module ID, since that is too expensive
    // in terms of RAM.
    u32 tx_per_return_code[MSG_TX_RC_LAST_ENTRY];
} msg_mod_per_state_usr_stat_t;

/****************************************************************************/
// msg_relay_stat_t
// Module statistics counters. Instantiated once.
/****************************************************************************/
typedef struct {
    u32 rx;           // Total number of frames received for relaying (good and bad)
    u32 tx_ok;        // Number of frames successfully transmitted.
    u32 inv_port_err; // While relaying a frame: Topo returned an invalid stack port number. Frame dropped.
    u32 tx_err;       // While relaying a frame: Packet Module couldn't transmit the frame. Frame dropped.
    u32 htl_err;      // While relaying a frame: HTL field of incoming frame was 1. Frame dropped.
} msg_relay_stat_t;

#define SSP_HDR_SZ_BYTES              8 /* Includes 0x8880, EPID, SSPID, and RESERVED */
#define MSYN_HDR_SZ_BYTES            17 /* Starting with PDU type */
// 3 (for PDU_TYPE, HTL, CID) + 6 * 2 (TLV 0x0-0x5) + 9 * 2 (TL 0x80-0x88) + 274 (V 0x80-0x82 and 0x85-0x88) + max string lengths (V 0x83-0x84) = 3 + 12 + 18 + 274 + strs = 307 + strs
#define MSYNACK_MAX_HDR_SZ_BYTES    (307 + MSG_MAX_VERSION_STRING_LEN + MSG_MAX_PRODUCT_NAME_LEN) /* Starting with PDU type */

#define DONT_USE_MD_MAX_HDR_SZ_BYTES 21 /* Starting with PDU type, without padding */
#define MACK_HDR_SZ_BYTES            11 /* Starting with PDU type */

// Size of whole frame (excluding VTSS_FDMA_HDR_SIZE_BYTES and FCS)
#define MSYN_MAX_SZ_BYTES     ((2 * VTSS_MAC_ADDR_SZ_BYTES) + SSP_HDR_SZ_BYTES + MSYN_HDR_SZ_BYTES)
#define MSYNACK_MAX_SZ_BYTES  ((2 * VTSS_MAC_ADDR_SZ_BYTES) + SSP_HDR_SZ_BYTES + MSYNACK_MAX_HDR_SZ_BYTES)
#define MACK_SZ_BYTES         ((2 * VTSS_MAC_ADDR_SZ_BYTES) + SSP_HDR_SZ_BYTES + MACK_HDR_SZ_BYTES)

#define MRST_HDR_SZ_BYTES             5 /* Starting with PDU type, i.e. PDU type, HTL, CID, and two-byte NULL-TLV */
// Size of whole MRST frame (excluding VTSS_FDMA_HDR_SIZE_BYTES and FCS)
#define MRST_MAX_SZ_BYTES     ((2 * VTSS_MAC_ADDR_SZ_BYTES) + SSP_HDR_SZ_BYTES + MRST_HDR_SZ_BYTES)

// Maximum frame size including various protocol headers (leave a little margin
// to the well-known 1518 for safety).
#define MSG_MTU 1400

// Fragmented MDs should use a multiple of 4 bytes payload to
// allow for faster re-assembly
#define MD_OFFSET_ALIGN (4)

// MDs are divided into encapsulation protocol, MD header, and user data (payload).
// The size of the MD header and protocol must take a multiple of four bytes,
// so that the first byte of the message payload is located on a four byte
// boundary, so that the receiver can directly map a structure onto it.
// The "DONT_USE_" prefix indicates that the code should not use these defines

#define DONT_USE_MD_UNALIGNED_PROTO_SZ_BYTES       ((2 * VTSS_MAC_ADDR_SZ_BYTES) + SSP_HDR_SZ_BYTES + DONT_USE_MD_MAX_HDR_SZ_BYTES)
#define MD_MAX_PROTO_SZ_BYTES                      (4 * ((DONT_USE_MD_UNALIGNED_PROTO_SZ_BYTES + 3) / 4))
#define DONT_USE_MD_PAYLOAD_OFFSET                 MD_MAX_PROTO_SZ_BYTES
#define DONT_USE_MD_MAX_UNALIGNED_PAYLOAD_SZ_BYTES (MSG_MTU - DONT_USE_MD_PAYLOAD_OFFSET)
#define MD_MAX_PAYLOAD_SZ_BYTES                    (MD_OFFSET_ALIGN*(DONT_USE_MD_MAX_UNALIGNED_PAYLOAD_SZ_BYTES/MD_OFFSET_ALIGN))

// Offset of DMAC in message protocol frames.
#define MSG_DMAC_OFFSET 0
#define MSG_SMAC_OFFSET (MSG_DMAC_OFFSET + VTSS_MAC_ADDR_SZ_BYTES)

// Offset of PDU Type in message protocol frames. Does not include VTSS_FDMA_HDR_SIZE
#define MSG_PDU_TYPE_OFFSET ((2 * VTSS_MAC_ADDR_SZ_BYTES) + SSP_HDR_SZ_BYTES)
#define MSG_HTL_OFFSET      (MSG_PDU_TYPE_OFFSET + 1)
#define MSG_CID_OFFSET      (MSG_HTL_OFFSET + 1)
#define MSG_TLV_OFFSET      (MSG_CID_OFFSET + 1)

// MSYN TLV definitions
#define MSG_MSYN_TLV_TYPE_NULL              0x00
#define MSG_MSYN_TLV_TYPE_MWINSZ            0x01
#define MSG_MSYN_TLV_TYPE_MUPSID            0x02
#define MSG_MSYN_TLV_TYPE_MSEQ              0x80
#define MSG_MSYN_TLV_TYPE_MFRAGSZ           0x81

// MSYNACK TLV definitions
#define MSG_MSYNACK_TLV_TYPE_NULL           0x00
#define MSG_MSYNACK_TLV_TYPE_SWINSZ         0x01
#define MSG_MSYNACK_TLV_TYPE_STATUS         0x02
#define MSG_MSYNACK_TLV_TYPE_PORT_CNT       0x03
#define MSG_MSYNACK_TLV_TYPE_STACK_PORT_0   0x04
#define MSG_MSYNACK_TLV_TYPE_STACK_PORT_1   0x05
#define MSG_MSYNACK_TLV_TYPE_MSEQ           0x80
#define MSG_MSYNACK_TLV_TYPE_SFRAGSZ        0x81
#define MSG_MSYNACK_TLV_TYPE_SSEQ           0x82
#define MSG_MSYNACK_TLV_TYPE_VERSION_STRING 0x83
#define MSG_MSYNACK_TLV_TYPE_PRODUCT_NAME   0x84
#define MSG_MSYNACK_TLV_TYPE_UPTIME_SECS    0x85
#define MSG_MSYNACK_TLV_TYPE_BOARD_TYPE     0x86
#define MSG_MSYNACK_TLV_TYPE_API_INST_ID    0x87
#define MSG_MSYNACK_TLV_TYPE_PHYS_PORT_MAP  0x88

// MD TLV definitions
#define MSG_MD_TLV_TYPE_NULL                0x00
#define MSG_MD_TLV_TYPE_DMODID              0x01
#define MSG_MD_TLV_TYPE_SEQ                 0x80
#define MSG_MD_TLV_TYPE_TOTLEN              0x81
#define MSG_MD_TLV_TYPE_OFFSET              0x82

// MACK TLV definitions
#define MSG_MACK_TLV_TYPE_NULL              0x00
#define MSG_MACK_TLV_TYPE_STATUS            0x01
#define MSG_MACK_TLV_TYPE_SEQ               0x80

// MRST TLV definitions
#define MSG_MRST_TLV_TYPE_NULL              0x00

/****************************************************************************/
// msg_list_item_t
// Each instance of this type is used to hold one single user message.
// It can be linked together with user items of the same type to
// form a list of user messages.
/****************************************************************************/
typedef struct tag_msg_item_t {

    // TRUE if this item is used for Tx, FALSE if for Rx.
    BOOL is_tx_msg;

    // Pointer to the user message
    u8 *usr_msg;

    // Length in bytes of the user message.
    u32 len;

    // Destination module ID
    u8 dmodid;

    // If @this is used as Rx on sec: The "arbitrarily" chosen connection ID
    // If @this is used as Rx on pri: The corresponding secondary switch's ISID
    // If @this is used as Tx on pri: Only used when looping back, and is then the Primary Switch ISID.
    // If @this is used as Tx on sec: Unused.
    u32 connid;

    // Tx: The message module state that this message was sent in.
    // Rx: The message module state that this message was received in.
    // These are needed so that the correct statistics counters can be
    // updated.
    // If a Tx Msg is looped back, the Rx will be counted in the same
    // state as it was transmitted, even though a state change may have
    // occurred in the meanwhile.
    msg_mod_state_t state;

    union {
        struct {
            // Sequence number handling
            u16 left_seq;
            u16 right_seq;
            u32 frags_received;
        } rx;

        struct {
            // User options
            msg_tx_opt_t opt;

            // User Tx Done callback
            msg_tx_cb_t cb;

            // Return code passed to msg_thread. If msg_tx_adv() was OK, it contains
            // MSG_TX_RC_OK, otherwise an error code, which is passed to the callback
            // function (if any).
            msg_tx_rc_t rc;

            // User-defined context
            void *contxt;

        } tx;
    } u;

    // Pointer to the next user message in the list.
    struct tag_msg_item_t *next;
} msg_item_t;

/****************************************************************************/
// msg_mcb_t
// Message Control Block. Holds the state of one connection.
/****************************************************************************/
typedef struct {
    //****************************************************
    // State common to both primary and secondary switches
    //****************************************************

    // Current state for this connection.
    msg_conn_state_t state;

    // Time that the connection got established
    time_t estab_time;

    // Source's SEQ number. Initialized to 0 at boot and then never more.
    // Used by source when sending MSYNACK and MDs and increased by one for
    // every such frame transmitted.
    u16 next_available_sseq;

    // Determines the left-most index of the sliding window that we haven't
    // yet received.
    // Pri: Initialized to MSYNACK.SSEQ + 1.
    // Sec: Initialized to MSYN.MSEQ + 1.
    u16 dseq_leftmost_unreceived;

    // Initialized as @dseq_leftmost_unreceived. Determines the leftmost
    // position in the sliding window that we accept MDs within.
    // @dseq_leftmost_allowed starts moving to the right when swinsz
    // MDs have been received, and then it moves once everytime
    // @dseq_leftmost_unreceived moves.
    u16 dseq_leftmost_allowed;

    // Array of already received MDs, 1 bit per MD.
    // The array is indexed in the following way:
    //   Initially this array and dseq_ackd_idx are cleared to 0.
    //   dseq_ackd_idx follows the dseq_leftmost_unreceived,
    //   thus incrementing whenever dseq_leftmost_unreceived increments.
    //   Bits in the interval
    //     [dseq_ackd_idx; dseq_ack_idx+swinsz[ (taking wrap around into account)
    //   are '1' if an MD is received. When dseq_ackd_idx increments by one
    //   (modulo 8*sizeof(dseq_ackd)) the corresponding position
    //   bit in dseq_ackd+swinsz gets cleared.
    u8  dseq_ackd[VTSS_BF_SIZE(MSG_CFG_MAX_WINSZ)];
    u32 dseq_ackd_idx;

    // Source's window size.
    // Initialized from global state's msg_cfg_sec_winsz if secondary switch
    // and receiving an MSYN.
    // Initialized from global state's msg_cfg_pri_winsz_per_sec if
    // primary switch and a new secondary switch is added.
    u16 swinsz;

    // Destination's window size.
    // Initialized by reception of MSYN (if secondary switch) or MSYNACK (if primary switch).
    u16 dwinsz;

    // Destination's fragment size. When the destination transmits fragmented
    // user messages, all-but-the-end MD must have a payload size of this
    // many bytes:
    u32 fragsz;

    // Pointer to a list of user TX messages currently in progress
    msg_item_t *tx_msg_list;

    // Pointer to the last user message in the tx_msg_list
    // This is only to speed up the insertion of messages.
    msg_item_t *tx_msg_list_last;

    // Pointer to a list of RX'd messages yet not forwarded to a User Module.
    msg_item_t *rx_msg_list;

    // In primary switch mode: Secondary switch's MAC address.
    // In secondary switch mode: Primary switch's MAC address.
    u8 dmac[VTSS_MAC_ADDR_SZ_BYTES];

    // Connection ID, "arbitrarily" chosen by secondary switch, and only used in interface
    // between User Modules and Msg Module. Valid in secondary switch mode only.
    u32 connid;

    // Per-connection statistics counters
    msg_conn_stat_t stat;

    // Primary switches use the msyn member, secondary switches the msynack.
    union {
        struct {

            // Various secondary switch info
            msg_switch_info_t switch_info;
        } primary_switch;

        struct {
        } secondary_switch;
    } u;
} msg_mcb_t;

/****************************************************************************/
// msg_glbl_state_t
// Holds the global state of the Msg Module. There is only one such instance
// per switch.
/****************************************************************************/
typedef struct {
    // The state that the Message Module as a whole is in. This is a sort of
    // super-state compared with the per-connection state, msg_mcb_t.state.
    msg_mod_state_t state;

    // Primary switch: Holds the current primary switch ISID.
    // Secondary switch: Unused.
    vtss_isid_t misid;

    // Primary switch: Unused.
    // Secondary switch: The message module must choose a new connection ID
    // for every connection that is opened towards it. The following variable is
    // initialized to a number that is one higher than the maximum ISID, so that
    // the Msg Module can discern between ISIDs and connids.
    // The 'connid' is only used in the interface between User and Msg Modules.
    u32 sec_next_connid;

    // Initialized to MSG_CFG_DEFAULT_MSYN_TIMEOUT_MS at boot.
    // May be changed using CLI at run-time.
    u32 msg_cfg_msyn_timeout_ms;

    // Initialized to MSG_CFG_DEFAULT_MD_TIMEOUT_MS at boot.
    // May be changed using CLI at run-time.
    u32 msg_cfg_md_timeout_ms;

    // Initialized to MSG_CFG_DEFAULT_MD_RETRANSMIT_LIMIT at boot.
    // May be changed using CLI at run-time.
    u32 msg_cfg_md_retransmit_limit;

    // Initialized to MSG_CFG_DEFAULT_SEC_WINSZ at boot.
    // May be changed using CLI at run-time.
    u16 msg_cfg_sec_winsz;

    // Initialized to MSG_CFG_DEFAULT_PRI_WINSZ_PER_SEC at boot.
    // May be changed using CLI at run-time.
    u16 msg_cfg_pri_winsz_per_sec;

    // Initialized to MSG_CFG_DEFAULT_HTL_LIMIT at boot.
    // May be changed using CLI at run-time.
    u8 msg_cfg_htl_limit;

    // Module-level statistics counters - one per possible module state.
    // Counts User Messages - not PDUs.
    msg_mod_per_state_usr_stat_t usr_stat[MSG_MOD_STATE_LAST_ENTRY];

    // Module-level statistics counters - one per possible module state.
    // Counts PDUs - not User Messages.
    msg_mod_per_state_pdu_stat_t pdu_stat[MSG_MOD_STATE_LAST_ENTRY];

    // Relay statistics counters, one per stack port
    msg_relay_stat_t relay_stat[2];
} msg_glbl_state_t;

/****************************************************************************/
// msg_rx_filter_item_t
// One single filter. Connected in a list to form all registrants for msgs.
/****************************************************************************/
typedef struct tag_msg_rx_filter_item {
    msg_rx_filter_t filter;
    struct tag_msg_rx_filter_item *next;
} msg_rx_filter_item_t;

#endif /* _VTSS_MSG_H_ */

