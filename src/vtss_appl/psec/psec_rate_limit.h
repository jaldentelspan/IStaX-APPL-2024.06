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

#ifndef _PSEC_RATE_LIMIT_H_
#define _PSEC_RATE_LIMIT_H_

#include <microchip/ethernet/switch/api.h> /* For u8, u16, u32, u64, mesa_vid_mac_t, etc */

#define PSEC_RATE_LIMIT_FILTER_CNT 10 /**< The number of memorized historical MAC addresses */

/**
 * \brief Rate limiter configuration
 */
typedef struct {
    u32 fill_level_min; /**< Hysteresis: After the burst capacity has been reached, do not allow new frames until it reaches this amount.   */
    u32 fill_level_max; /**< Burst capacity. At most this amount of frames in a burst.                                                      */
    u32 rate;           /**< At most this amount of frames per second over time (better to pick a power-of-two for this number).            */
    u64 drop_age_ms;    /**< Drop the frame if it is less than this amount of milliseconds since last frame with this MAC address was seen. */
} psec_rate_limit_conf_t;

/**
 * \brief Rate limiter statistics
 */
typedef struct {
    u64 forward_cnt;     /**< Number of not-dropped packets.                  */
    u64 drop_cnt;        /**< Number of dropped packets due to rate-limiting  */
    u64 filter_drop_cnt; /**< Number of dropped packets due to the MAC filter */
} psec_rate_limit_stat_t;

/**
 * \brief Check to see if we should drop this frame.
 *
 * Check to see if we should drop this frame from a rate-limiter perspective.
 *
 * - Reentrant: No. Always call it from the same thread (e.g. Packet Rx thread).
 *
 * \param port    [IN]: Port on which the frame was received.
 * \param vid_mac [IN]: Frame's MAC address and classified VID.
 *
 * \return TRUE if frame should be dropped.\n
 *         FALSE if it's OK - from the rate-limiter's perspective - to send the frame to the primary switch.
 */
BOOL psec_rate_limit_drop(mesa_port_no_t port, mesa_vid_mac_t *vid_mac);

/**
 * \brief Get current rate-limiter configuration - Debug only
 *
 * \param conf [OUT]: Pointer to structure receiving current rate limiter configuration.
 */
mesa_rc psec_rate_limit_conf_get(psec_rate_limit_conf_t *conf);

/**
 * \brief Configure rate-limiter - Debug only
 *
 * \param isid [IN]: Set to VTSS_ISID_LOCAL to configure local switch or VTSS_ISID_GLOBAL to configure all switches in stack.
 * \param conf [IN]: The rate limiter configuration to use from now on.
 */
mesa_rc psec_rate_limit_conf_set(vtss_isid_t isid, psec_rate_limit_conf_t *conf);

/**
 * \brief Read statistics counters - Debug only
 *
 * \param stat                  [OUT]: Pointer to structure receiving the current rate limiter statistics.
 * \param rate_limit_fill_level [OUT]: Pointer to u64 receiving the current rate-limiter fill-level
 */
mesa_rc psec_rate_limit_stat_get(psec_rate_limit_stat_t *stat, u64 *rate_limit_fill_level);

/**
 * \brief Clear statistics counters - Debug only
 */
mesa_rc psec_rate_limit_stat_clr(void);

/**
 * \brief Clear current filter for \@port.
 *
 * Use VTSS_PORTS for \@port, if you wish to clear the whole filter.
 */
mesa_rc psec_rate_limit_filter_clr(mesa_port_no_t port);

/**
 * \brief Initialize rate-limiter
 *
 * Expected to be called only once.
 */
void psec_rate_limit_init(void);

/**
 * \brief Function to send rate limit configuration to another switch.
 *
 * This is implemented in psec.c, not psec_rate_limit.c.
 */
void psec_msg_tx_rate_limit_conf(vtss_isid_t isid, psec_rate_limit_conf_t *conf);

#endif /* _PSEC_RATE_LIMIT_H_ */

