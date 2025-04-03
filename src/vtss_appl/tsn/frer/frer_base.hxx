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

#ifndef _FRER_BASE_HXX_
#define _FRER_BASE_HXX_

#include "main_types.h"
#include <vtss/appl/frer.hxx>
#include <vtss/basics/map.hxx> /* For vtss::Map()                */
#include "packet_api.h"        /* For packet_tx_props_t          */
#include "acl_api.h"           /* For acl_entry_conf_t           */
#include <vtss/appl/vlan.h>    /* For vtss_appl_vlan_port_type_t */
#include "tsn_timer.hxx"       /* For tsn_timer_t                */

#define VTSS_MALLOC_MODULE_ID VTSS_MODULE_ID_FRER

typedef struct {
    mesa_port_no_t port_no;
    bool           link;
} frer_port_state_t;

// All members of this struct are updated by frer_base.cxx and some of them are
// used by frer.cxx.
// Stream is an overloaded word in FRER. This structure's name refers to the
// ingress streams defined in the stream module. The user may add and remove
// (ingress) streams dynamically without changing the configuration of FRER, so
// we can subscribe and unsubscribe from a particular stream and we keep an eye
// on what we have done with this state.
typedef struct {
    // true if stream exists.
    bool exists;

    // true if we couldn't take over the stream.
    bool attach_failed;

    // Holds the stream's list of ingress ports. Only valid if #exists is true.
    // This is only used by frer.cxx to update operational warnings.
    mesa_port_list_t ingress_ports;
} frer_stream_state_t;

// Latent error detection state.
typedef struct {
    // We monitor the compound counters on all egress ports.
    // CurBaseDifference as defined in 802.1CB, clause 7.4.4.2.1.
    // From the standard:
    //    CurBaseDifference is an unsigned integer of the same size as the
    //    counters from which its value is computed - hence uint64_t.
    uint64_t cur_base_difference[VTSS_APPL_FRER_EGRESS_PORT_CNT_MAX];

    // This is just a status for a particular egress port.
    bool latent_error[VTSS_APPL_FRER_EGRESS_PORT_CNT_MAX];

    // Number of times the LatentErrorReset() function is invoked. Corresponds
    // to frerCpsSeqRcvyLatentErrorResets (clause 10.8.10 of 802.1CB).
    uint64_t led_resets;

    // The Latent error detection requires two timers, a test timer and a
    // reset-timer. We use the functionality of the parent (TSN) module.

    // Timeout of the timer corresponds to RESET_LATENT_ERROR event from clause
    // 7.4.4.1.
    tsn_timer_t reset_timer;

    // Timeout of the timer corresponds to TRIGGER_LATENT_ERROR_TEST from
    // clause 7.4.4.1.
    tsn_timer_t test_timer;
} frer_led_state_t;

// This structure contains all the configuration and status pertaining to one
// FRER instance.
typedef struct {
    vtss_appl_frer_conf_t   conf;
    vtss_appl_frer_status_t status;
    uint32_t                inst; // For internal trace

    // Return true if recovery mode and individual recovery is enabled.
    bool individual_recovery(void)
    {
        return conf.mode == VTSS_APPL_FRER_MODE_RECOVERY && conf.rcvy_individual;
    }

    bool using_stream_collection(void) const
    {
        return conf.stream_collection_id != VTSS_APPL_STREAM_COLLECTION_ID_NONE;
    }

    // We support both a stream ID list and stream collections.
    // Stream Collections:
    //   When using stream collections, only idx 0 is used and the stream state
    //   indicates whether the stream collection exists or not and if it exists,
    //   whether we could attach to it.
    //   We do not support stream collections in individual recovery mode.
    //
    // Stream ID list:
    //   Whether entries are used or not depends on the configured mode.
    //   Generation:
    //     We only support one single ingress stream.
    //
    //   Recovery:
    //     Up to ARRSZ(conf.stream_ids) ingress streams are supported.
    //     Well, in fact it's only FRER_cap.egress_port_cnt_max, but let's go
    //     for the worst-case here.
    frer_stream_state_t stream_states[ARRSZ(conf.stream_ids)];

    // ID of Tag Control Element. Used to apply rules on egress ports.
    mesa_tce_id_t tce_id;

    // Member Stream IDs. These are the base mstream IDs. If individual recovery
    // is enabled, we need one set of base mstream IDs per ingress stream.
    // If individual recovery is disabled, we only use index 0.
    mesa_frer_mstream_id_t mstream_base_ids[ARRSZ(conf.stream_ids)];

    // Compound Stream ID. One per possible egress port. Used in recovery.
    mesa_frer_cstream_id_t cstream_ids[VTSS_APPL_FRER_EGRESS_PORT_CNT_MAX];

    // S/W-based counters for number of manual resets in generation mode.
    uint64_t gen_resets;

    // Latent-error-detection state.
    frer_led_state_t led_state;
} frer_state_t;

typedef vtss::Map<uint32_t, frer_state_t> frer_map_t;
typedef frer_map_t::iterator frer_itr_t;
extern  frer_map_t           FRER_map;

void    frer_base_init(void);
mesa_rc frer_base_state_init(      frer_state_t *frer_state);
mesa_rc frer_base_activate(        frer_state_t *frer_state);
mesa_rc frer_base_deactivate(      frer_state_t *frer_state);
mesa_rc frer_base_led_conf_change( frer_state_t *frer_state);
mesa_rc frer_base_control_reset(   frer_state_t *frer_state, const vtss_appl_frer_control_t *ctrl);
void    frer_base_stream_update(   frer_state_t *frer_state, int idx);
mesa_rc frer_base_statistics_get  (frer_state_t *frer_state, mesa_port_no_t port_no, vtss_appl_stream_id_t stream_id, vtss_appl_frer_statistics_t *statistics);
mesa_rc frer_base_statistics_clear(frer_state_t *frer_state);

#endif /* _FRER_BASE_HXX_ */

