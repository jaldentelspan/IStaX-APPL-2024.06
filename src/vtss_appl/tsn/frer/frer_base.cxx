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

#include <vtss/appl/frer.hxx>
#include "frer_base.hxx"
#include "frer_expose.hxx"
#include "frer_trace.h"
#include "frer_lock.hxx"
#include "frer_api.h"
#include "tsn_lock.hxx"
#include "stream_api.h" // For stream_util_counters_XXX()

/*
 * Symbol for individual recovery instance (mstream) with individual recovery
 * disabled, assuming two streams, X and Y.
 *   /\
 *   XY
 *   --
 *
 * Symbol for two individual recovery instances (mstreams) with individual
 * recovery enabled, assuming two streams, X and Y:
 *   /\  /\
 *   XX  YY
 *   --  --
 *
 * Symbol for compound recovery instance (cstream):
 *   /-\
 *   | |
 *   \-/
 *
 * Symbol for VCE, IFLOW, and TCE (only used on older chips like SparX-5i):
 *   ---
 *   | |
 *   ---
 *
 * Both of the following examples have two input streams and three egress
 * interfaces.
 *
 * Example 1a: Individual recovery disabled. Using a list of stream_ids.
 * When a list of stream_ids is used, each stream has its own IFLOW (maintained
 * by the Stream module).
 * Each stream's IFLOW point the same mstream. The number of mstreams
 * corresponds to the number of egress interfaces, so in this example there are
 * three mstreams in total (the IDs of the last two are hidden in the
 * API, so the application only has the ID of the first).
 * Also three cstreams are allocated - one per egress port, and the mstreams
 * point to their own cstream.
 *
 * On older chips (SparX-5i), one single TCE is allocated, which is installed on
 * all egress ports. The drawing is not 100% correct, because the cstream does
 * not point to the TCE. Instead the TCE matches frames that egress the
 * specified egress list of ports on the FRER VLAN. The TCE is used to either
 * pop (terminating recovery mode) or push an R-tag (non-terminating recovery
 * mode and generation mode).
 *
 * On newer chips (LAN966x), such a TCE is not allocated, because control of
 * pushing and popping of R-tags occur in the IFLOW.
 *
 *               VCE     IFLOW       mstream  cstream    TCE (SparX-5i, only)
 *
 *                                      /\      /-\
 *                             -------> XY ---> | | --        --> Recovered stream
 *                             |   |    --      \-/  |        |
 *               ---      ---  |   |                 |        |
 * Stream X ---> | | ---> | | --   |                 |        |
 *               ---      ---      |                 |        |
 *                                 |                 |        |
 *                                 |    /\      /-\  |   ---  |
 *                                 |    XY ---> | | ---> | | ---> Recovered stream
 *                                 |    --      \-/  |   ---  |
 *                                 |                 |        |
 *               ---      ---      |                 |        |
 * Stream Y ---> | | ---> | | ------                 |        |
 *               ---      ---                        |        |
 *                                      /\      /-\  |        |
 *                                      XY ---> | | --        --> Recovered stream
 *                                      --      \-/
 *
 * Example 1b: Individual recovery disabled. Using a stream-collection.
 * When using a stream-collection, the stream module allocates one single IFLOW
 * and use that for all streams in the collection.
 * This one IFLOW points to one mstream. The actual number of mstreams
 * corresponds to the number of egress interfaces, so in this example there are
 * three mstreams in total (the IDs of the last two are hidden in the
 * API, so the application only has the ID of the first).
 * Also three cstreams are allocated - one per egress port, and the mstreams
 * point to their own cstream.
 *
 * The story about the TCE is the same as in example 1a).
 *
 *               VCE     IFLOW       mstream  cstream    TCE (SparX-5i, only)
 *
 *                                      /\      /-\
 *                             -------> XY ---> | | --        --> Recovered stream
 *                             |        --      \-/  |        |
 *               ---      ---  |                     |        |
 * Stream X ---> | | ---> | | --                     |        |
 *               ---      ---                        |        |
 *                         |                         |        |
 *                         |            /\      /-\  |   ---  |
 *                         |            XY ---> | | ---> | | ---> Recovered stream
 *                         |            --      \-/  |   ---  |
 *                         |                         |        |
 *               ---       |                         |        |
 * Stream Y ---> | | ------|                         |        |
 *               ---                                 |        |
 *                                      /\      /-\  |        |
 *                                      XY ---> | | --        --> Recovered stream
 *                                      --      \-/
 *
 * Example 2a: Individual recovery enabled. Using a list of stream_ids.
 * Each ingress stream requires its own IFLOW.
 * Each IFLOW points to its own set of mstreams. A set consists of an mstream
 * per egress port.
 * Each mstream points to a cstream allocated for the egress port.
 *
 * Indiviual recovery therefore requires ingress-stream-cnt x egress-port-cnt
 * mstreams, so with support of up to 8 ingress streams and 8 egress ports, one
 * recovery instance with individual recovery enabled requires 64 mstreams. A
 * similar setup with individual recovery disabled only requires one mstream per
 * egress port.
 *
 * The story about the TCE is the same as in example 1a).
 *
 *               VCE     IFLOW    mstream    cstream    TCE (SparX-5i, only)
 *
 *                                   /\
 *                              ---> XX ---
 *               ---      ---   |    --   |
 * Stream X ---> | | ---> | | ---         ---> /-\
 *               ---      ---                  | | ---        --> Recovered stream
 *                                        ---> \-/   |        |
 *                                   /\   |          |        |
 *                              ---> YY ---          |        |
 *                              |    --              |        |
 *                              |                    |        |
 *                              |                    |        |
 *                              |    /\              |        |
 *                              |    XX ---          |        |
 *                              |    --   |          |        |
 *               ---      ---   |         ---> /-\   |   ---  |
 * Stream Y ---> | | ---> | | ---              | | ----> | | ---> Recovered stream
 *               ---      ---             ---> \-/   |   ---  |
 *                                   /\   |          |        |
 *                                   YY ---          |        |
 *                                   --              |        |
 *                                                   |        |
 *                                                   |        |
 *                                   /\              |        |
 *                                   XX ---          |        |
 *                                   --   |          |        |
 *                                        ---> /-\   |        |
 *                                             | | ---        --> Recovered stream
 *                                        ---> \-/
 *                                   /\   |
 *                                   YY ---
 *                                   --
 *
 * Example 2b: Individual recovery enabled. Using a stream-collection.
 *
 * This is not possible, because with individual recovery, we need to have one
 * IFLOW per stream, and a stream-collection only has one. We don't support a
 * list of stream-collections.
 *
 * Example 3a: Generation mode. Using one stream.
 * The thing with FRER generation is that we need one single IFLOW, because the
 * IFLOW is tightly coupled with the sequence number generator (SNG).
 *
 * The IFLOW generates the sequence number, and we let the user decide how to
 * get the frames to the egress ports (FRER VLAN membership or MAC-table
 * lookups) on which a TCE adds the R-Tag on older chips SparX-5i). On newer
 * chips, the IFLOW is used to both push and pop R-tags, so a TCE is not
 * required.
 *
 * It looks like this:
 *               VCE     IFLOW                           TCE (SparX-5i only)
 *
 *
 *                                                            --> R-tagged stream
 *                                                            |
 *                                                            |
 *               ---      ---                            ---  |
 * Stream X ---> | | ---> | |--------------------------> | | ---> R-tagged stream
 *               ---      ---                            ---  |
 *                                                            |
 *                                                            |
 *                                                            --> R-tagged stream
 *
 * Example 3b: Generation mode. Using a stream-collection.
 *
 * This is the actual reason for supporting stream-collections. As said in
 * example 3a), is that we need one single IFLOW, because of the SNG's tightly
 * coupling with IFLOWs. A stream-collection is exactly that. All streams in a
 * stream collection are using the same IFLOW, and can therefore use the same
 * SNG.
 *
 * The TCE story is the same as in example 1a and 3a).
 *
 * It looks like this:
 *               VCE     IFLOW                           TCE (SparX-5i only)
 *
 *
 *                                                            --> R-tagged stream
 *               ---                                          |
 * Stream X ---> | | --                                       |
 *               ---  |   ---                            ---  |
 *                    --> | |--------------------------> | | ---> R-tagged stream
 *               ---  |   ---                            ---  |
 * Stream Y -->  | | --                                       |
 *               ---                                          |
 *                                                            --> R-tagged stream
 */

// Compound streams are controlled without API allocation, so we must keep track
// of the allocations ourselves. This value indicates that a cstream is not
// allocated.
// Compound stream IDs are 16-bit unsigned integers numbered in range
// [0; FRER_BASE_cap_cstream_cnt[.
#define FRER_CSTREAM_ID_NONE  (mesa_frer_cstream_id_t)-1
static mesa_frer_cstream_id_t FRER_BASE_cstream_id = -1;
static uint32_t               FRER_BASE_cap_cstream_cnt;

// This capability determines whether pushing and popping of R-tags is done in
// the IFLOW(s) or in a specially crafted TCE. On older chips (e.g. FA), we need
// to have TCEs per configured egress port in order to push and pop R-Tags.
// On newer chips (e.g. LAN966x), pushing occurs when sequence generation is
// enabled in the IFLOW and popping is also configured in the IFLOW, so on these
// chips, no TCEs are required.
static bool FRER_BASE_cap_iflow_pop;

// Member streams are controlled through API allocation, so we just get a base
// mstream ID, but we need a way to indicate whether this base mstream ID is in
// use or not.
#define FRER_MSTREAM_BASE_ID_NONE (mesa_frer_mstream_id_t)-1

// TCEs are only used in older chips (FA):
// TCEs are ordered in H/W. The application can choose where to insert it by
// using an ID in the calls to mesa_tce_add(). If using MESA_TCE_ID_LAST, the
// TCE will come last in H/W. Otherwise, we can choose to insert it just before
// another ID (mesa_tce_id_t). That's fine as long as only one module is using
// TCEs, but if we must ensure that the TCEs come in correct order, we should
// have a central module that knows about the modules that want to install TCEs
// and the order that these modules want to install them in relationship to each
// other, so that it can provide a correct ID to use.
// Since we currently don't have such a module, the first FRER instance will
// always have to use MESA_TCE_ID_LAST, and the TCEs will eventually be mixed
// inter-module-wise. However, a given module can always control the insertion
// order of its own rules.
// Besides these problems, we also need our own set of IDs.
// As of now, only CFM uses TCEs and assigns IDs in the following range:
//     [0xFFFF0000; 0xFFFFFFFF[ (up to 65534 TCEs)
// So let's pick IDs in this range:
//     [0xFFFE0000; 0xFFFEFFFF[ (up to 65534 TCEs).
#define FRER_TCE_ID_NONE  0xFFFEFFFF
#define FRER_TCE_ID_START 0xFFFE0000
#define FRER_TCE_ID_END   FRER_TCE_ID_NONE
static mesa_tce_id_t FRER_BASE_tce_id = FRER_TCE_ID_START - 1;

// Other global variables
static uint32_t FRER_BASE_cap_port_cnt; // Number of ports.

// Some mesa_frer_XXX() functions are supposed to be called from multiple places
// in this file. In order to minimize the trace that comes out of it, many of
// these calls are replaced by a call to a single function that performs them.
// This requires that function to know whether to get, set, or both, which this
// enum is for.
typedef enum {
    FRER_BASE_ACCESS_METHOD_GET     = 0x01,
    FRER_BASE_ACCESS_METHOD_SET     = 0x02,
    FRER_BASE_ACCESS_METHOD_GET_SET = FRER_BASE_ACCESS_METHOD_GET | FRER_BASE_ACCESS_METHOD_SET
} frer_base_access_method_t;

/******************************************************************************/
// FRER_BASE_cstream_conf_access()
/******************************************************************************/
static mesa_rc FRER_BASE_cstream_conf_access(frer_state_t *frer_state, mesa_frer_cstream_id_t cstream_id, mesa_frer_stream_conf_t *cstream_conf, frer_base_access_method_t access_method, int idx, const char *func, int line)
{
    mesa_rc rc;

    if (access_method & FRER_BASE_ACCESS_METHOD_GET) {
        if ((rc = mesa_frer_cstream_conf_get(nullptr, cstream_id, cstream_conf)) != VTSS_RC_OK) {
            T_EG(FRER_TRACE_GRP_BASE, "%u (%s#%d): mesa_frer_cstream_conf_get(%d, %u) failed: %s", frer_state->inst, func, line, idx, cstream_id, error_txt(rc));

            // Convert to something sensible
            return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
        }

        T_IG(FRER_TRACE_GRP_BASE, "%u (%s#%d): mesa_frer_cstream_conf_get(%d, %u) succeeded", frer_state->inst, func, line, idx, cstream_id);
    }

    if (access_method & FRER_BASE_ACCESS_METHOD_SET) {
        if ((rc = mesa_frer_cstream_conf_set(nullptr, cstream_id, cstream_conf)) != VTSS_RC_OK) {
            T_EG(FRER_TRACE_GRP_BASE, "%u (%s#%d): mesa_frer_cstream_conf_set(%d, %u) failed: %s", frer_state->inst, func, line, idx, cstream_id, error_txt(rc));

            // Convert to something sensible
            return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
        }

        T_IG(FRER_TRACE_GRP_BASE, "%u (%s#%d): mesa_frer_cstream_conf_set(%d, %u) succeeded", frer_state->inst, func, line, idx, cstream_id);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_mstream_conf_access()
/******************************************************************************/
static mesa_rc FRER_BASE_mstream_conf_access(frer_state_t *frer_state, mesa_frer_mstream_id_t mstream_id, mesa_port_no_t port_no, mesa_frer_stream_conf_t *mstream_conf, frer_base_access_method_t access_method, int idx, const char *func, int line)
{
    mesa_rc rc;

    if (access_method & FRER_BASE_ACCESS_METHOD_GET) {
        if ((rc = mesa_frer_mstream_conf_get(nullptr, mstream_id, port_no, mstream_conf)) != VTSS_RC_OK) {
            T_EG(FRER_TRACE_GRP_BASE, "%u (%s#%d): mesa_frer_mstream_conf_get(%d, %u, %u) failed: %s", frer_state->inst, func, line, idx, port_no, mstream_id, error_txt(rc));

            // Convert to something sensible
            return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
        }

        T_IG(FRER_TRACE_GRP_BASE, "%u (%s#%d): mesa_frer_mstream_conf_get(%d, %u, %u) succeeded", frer_state->inst, func, line, idx, port_no, mstream_id);
    }

    if (access_method & FRER_BASE_ACCESS_METHOD_SET) {
        if ((rc = mesa_frer_mstream_conf_set(nullptr, mstream_id, port_no, mstream_conf)) != VTSS_RC_OK) {
            T_EG(FRER_TRACE_GRP_BASE, "%u (%s#%d): mesa_frer_mstream_conf_set(%d, %u, %u) failed: %s", frer_state->inst, func, line, idx, port_no, mstream_id, error_txt(rc));

            // Convert to something sensible
            return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
        }

        T_IG(FRER_TRACE_GRP_BASE, "%u (%s#%d): mesa_frer_mstream_conf_set(%d, %u, %u) succeeded", frer_state->inst, func, line, idx, port_no, mstream_id);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_cstream_cnt_get()
/******************************************************************************/
static mesa_rc FRER_BASE_cstream_cnt_get(frer_state_t *frer_state, mesa_frer_cstream_id_t cstream_id, mesa_frer_counters_t *ctrs, int idx, const char *func, int line)
{
    mesa_rc rc;

    if ((rc = mesa_frer_cstream_cnt_get(nullptr, cstream_id, ctrs)) != VTSS_RC_OK) {
        T_EG(FRER_TRACE_GRP_BASE, "%u (%s#%d): mesa_frer_cstream_cnt_get(%d, %u) failed: %s", frer_state->inst, func, line, idx, cstream_id, error_txt(rc));

        // Convert to something sensible
        return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
    }

    T_IG(FRER_TRACE_GRP_BASE, "%u (%s#%d): mesa_frer_cstream_cnt_get(%d, %u) succeeded", frer_state->inst, func, line, idx, cstream_id);

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_cstream_cnt_clear()
/******************************************************************************/
static mesa_rc FRER_BASE_cstream_cnt_clear(frer_state_t *frer_state, mesa_frer_cstream_id_t cstream_id, int idx, const char *func, int line)
{
    mesa_rc rc;

    if ((rc = mesa_frer_cstream_cnt_clear(nullptr, cstream_id)) != VTSS_RC_OK) {
        T_EG(FRER_TRACE_GRP_BASE, "%u (%s#%d): mesa_frer_cstream_cnt_clear(%d, %u) failed: %s", frer_state->inst, func, line, idx, cstream_id, error_txt(rc));

        // Convert to something sensible
        return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
    }

    T_IG(FRER_TRACE_GRP_BASE, "%u (%s#%d): mesa_frer_cstream_cnt_clear(%d, %u) succeeded", frer_state->inst, func, line, idx, cstream_id);

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_mstream_cnt_clear()
/******************************************************************************/
static mesa_rc FRER_BASE_mstream_cnt_clear(frer_state_t *frer_state, mesa_frer_mstream_id_t mstream_id, mesa_port_no_t port_no, int idx, const char *func, int line)
{
    mesa_rc rc;

    if ((rc = mesa_frer_mstream_cnt_clear(nullptr, mstream_id, port_no)) != VTSS_RC_OK) {
        T_EG(FRER_TRACE_GRP_BASE, "%u (%s#%d): mesa_frer_mstream_cnt_clear(%d, %u, %u) failed: %s", frer_state->inst, func, line, idx, port_no, mstream_id, error_txt(rc));

        // Convert to something sensible
        return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
    }

    T_IG(FRER_TRACE_GRP_BASE, "%u (%s#%d): mesa_frer_mstream_cnt_clear(%d, %u, %u) succeeded", frer_state->inst, func, line, idx, port_no, mstream_id);

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_tce_id_alloc()
// We take the simple solution and go through all currently active FRER
// instances to find an unused TCE ID.
// We don't have a requirement on insertion order, so let's just insert them
// last.
/******************************************************************************/
static mesa_rc FRER_BASE_tce_id_alloc(frer_state_t *frer_state)
{
    frer_itr_t itr;
    bool       already_used, already_wrapped;

    if (frer_state->tce_id != FRER_TCE_ID_NONE) {
        // Re-use current.
        return VTSS_RC_OK;
    }

    already_wrapped = false;
    while (1) {
        if (++FRER_BASE_tce_id == FRER_TCE_ID_END) {
            if (already_wrapped) {
                T_EG(FRER_TRACE_GRP_BASE, "%u: Internal error: No free TCE IDs in our range (range size = %u)", frer_state->inst, FRER_TCE_ID_END - FRER_TCE_ID_START);
                return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
            }

            FRER_BASE_tce_id = FRER_TCE_ID_START;
            already_wrapped = true;
        }

        already_used = false;
        for (itr = FRER_map.begin(); itr != FRER_map.end(); ++itr) {
            if (itr->second.tce_id == FRER_BASE_tce_id) {
                already_used = true;
                break;
            }
        }

        if (!already_used) {
            frer_state->tce_id = FRER_BASE_tce_id;
            break;
        }
    }

    T_IG(FRER_TRACE_GRP_BASE, "%u: tce-id-alloc() => 0x%x", frer_state->inst, frer_state->tce_id);

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_tce_id_free()
/******************************************************************************/
static void FRER_BASE_tce_id_free(frer_state_t *frer_state)
{
    T_IG(FRER_TRACE_GRP_BASE, "%u: tce-id-free(0x%x)", frer_state->inst, frer_state->tce_id);
    frer_state->tce_id = FRER_TCE_ID_NONE;
}

/******************************************************************************/
// FRER_BASE_cstream_id_alloc()
// We take the simple solution and go through all currently active FRER
// instances to find an unused Compound Stream ID.
/******************************************************************************/
static mesa_rc FRER_BASE_cstream_id_alloc(frer_state_t *frer_state, int idx)
{
    mesa_frer_cstream_id_t *cstream_id = &frer_state->cstream_ids[idx];
    frer_itr_t             itr;
    bool                   already_used, already_wrapped;
    int                    i;

    if (*cstream_id != FRER_CSTREAM_ID_NONE) {
        T_EG(FRER_TRACE_GRP_BASE, "%u: Internal error: Attempting to allocate a cstream ID while the old state indicates that it's already in use (%u)", frer_state->inst, *cstream_id);
        return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
    }

    already_wrapped = false;
    while (1) {
        if (++FRER_BASE_cstream_id == FRER_BASE_cap_cstream_cnt) {
            if (already_wrapped) {
                T_EG(FRER_TRACE_GRP_BASE, "%u: Internal error: No free cstream IDs in our range ([0; %u[)", frer_state->inst, FRER_BASE_cap_cstream_cnt);
                return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
            }

            FRER_BASE_cstream_id = 0;
            already_wrapped = true;
        }

        already_used = false;
        for (itr = FRER_map.begin(); itr != FRER_map.end(); ++itr) {
            for (i = 0; i < ARRSZ(itr->second.cstream_ids); i++) {
                if (itr->second.cstream_ids[i] == FRER_BASE_cstream_id) {
                    already_used = true;
                    break;
                }
            }

            if (already_used) {
                break;
            }
        }

        if (!already_used) {
            *cstream_id = FRER_BASE_cstream_id;
            break;
        }
    }

    T_IG(FRER_TRACE_GRP_BASE, "%u: cstream-id-alloc(%d) => %u", frer_state->inst, idx, *cstream_id);

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_cstream_id_free()
/******************************************************************************/
static void FRER_BASE_cstream_id_free(frer_state_t *frer_state, int idx)
{
    mesa_frer_cstream_id_t  *cstream_id = &frer_state->cstream_ids[idx];
    mesa_frer_stream_conf_t cstream_conf;

    if (*cstream_id == FRER_CSTREAM_ID_NONE) {
        // Nothing to free, but avoid the T_IG() below.
        return;
    }

    // 'debug api vxlat' prints out all cstreams unless they have recovery
    // disabled, so in order to minimize the output, we write a new, disabled
    // cstream configuration for this ID.
    vtss_clear(cstream_conf);
    (void)FRER_BASE_cstream_conf_access(frer_state, *cstream_id, &cstream_conf, FRER_BASE_ACCESS_METHOD_SET, idx, __FUNCTION__, __LINE__);

    T_IG(FRER_TRACE_GRP_BASE, "%u: cstream-id-free(%d, %u)", frer_state->inst, idx, *cstream_id);
    *cstream_id = FRER_CSTREAM_ID_NONE;
}

/******************************************************************************/
// FRER_BASE_mstream_id_alloc()
// May only be invoked in recovery mode.
/******************************************************************************/
static mesa_rc FRER_BASE_mstream_id_alloc(frer_state_t *frer_state, int idx)
{
    mesa_frer_mstream_id_t *mstream_base_id = &frer_state->mstream_base_ids[idx];
    mesa_rc                rc;

    if (*mstream_base_id != FRER_MSTREAM_BASE_ID_NONE) {
        T_EG(FRER_TRACE_GRP_BASE, "%u: Internal error: Attempting to allocate a base mstream ID while the old state indicates that it's already in use (%u)", frer_state->inst, *mstream_base_id);
        return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
    }

    if ((rc = mesa_frer_mstream_alloc(nullptr, &frer_state->conf.egress_ports, mstream_base_id)) != VTSS_RC_OK) {
        // The maximum number of FRER instances is higher than the worst-
        // case number of mstreams needed (because generation doesn't
        // require mstreams and non-individual recovery instances only
        // require one set of mstreams. This means that we cannot throw a
        // trace error here - hence the trace info (and not a trace warning,
        // because I hate them).
        // Furthermore the API requires N mstreams in sequence so the
        // mstream allocation table may become fragmented, so that it can't
        // find them. N is the number of egress ports.
        T_IG(FRER_TRACE_GRP_BASE, "%u: mesa_frer_mstream_alloc(%d, %s) failed: %s", frer_state->inst, idx, frer_state->conf.egress_ports, error_txt(rc));

        // Convert the error code to something sensible
        return VTSS_APPL_FRER_RC_HW_RESOURCES;
    }

    T_IG(FRER_TRACE_GRP_BASE, "%u: mesa_frer_mstream_alloc(%d, %s) => %u", frer_state->inst, idx, frer_state->conf.egress_ports, *mstream_base_id);
    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_mstream_id_free()
// May only be invoked in recovery mode.
/******************************************************************************/
static mesa_rc FRER_BASE_mstream_id_free(frer_state_t *frer_state, int idx)
{
    mesa_frer_mstream_id_t *mstream_base_id = &frer_state->mstream_base_ids[idx];
    mesa_rc                rc               = VTSS_RC_OK;

    if (*mstream_base_id == FRER_MSTREAM_BASE_ID_NONE) {
        // Nothing to free
        return VTSS_RC_OK;
    }

    if ((rc = mesa_frer_mstream_free(nullptr, *mstream_base_id)) != VTSS_RC_OK) {
        T_EG(FRER_TRACE_GRP_BASE, "%u: mesa_frer_mstream_free(%d, %u) failed: %s", frer_state->inst, idx, *mstream_base_id, error_txt(rc));
    } else {
        T_IG(FRER_TRACE_GRP_BASE, "%u: mesa_frer_mstream_free(%d, %u) succceeded", frer_state->inst, idx, *mstream_base_id);
    }

    *mstream_base_id = FRER_MSTREAM_BASE_ID_NONE;
    return rc;
}

/******************************************************************************/
// FRER_BASE_stream_action_get()
/******************************************************************************/
static mesa_rc FRER_BASE_stream_action_get(frer_state_t *frer_state, vtss_appl_stream_action_t &action, int idx)
{
    mesa_frer_mstream_id_t mstream_base_id;

    vtss_clear(action);

    action.enable    = true;
    action.client_id = frer_state->inst; // To identify ourselves

    if (frer_state->conf.mode == VTSS_APPL_FRER_MODE_GENERATION) {
        // We need to enable sequence number generation on the IFLOW.

        // In generation mode, we don't have to disable cut-through, because
        // we are sure the frame exits the switch.
        action.frer.frer.generation = true;

        // Count the number of resets.
        frer_state->gen_resets = 1;
    } else {
        if (FRER_BASE_cap_iflow_pop && frer_state->conf.rcvy_terminate) {
            // On newer chips, we pop the R-Tag using the IFLOW.
            action.frer.frer.pop = true;
        }

        // Connect the IFLOW to an mstream base ID. If individual recovery is
        // enabled, we have an mstream ID per ingress stream. If individual
        // recovery is disabled, we use the same mstream ID for all IFLOWs. This
        // is located in index 0.
        mstream_base_id = frer_state->mstream_base_ids[frer_state->individual_recovery() ? idx : 0];

        if (mstream_base_id == FRER_MSTREAM_BASE_ID_NONE) {
            T_EG(FRER_TRACE_GRP_BASE, "%u: mstream not allocated (idx = %d)", frer_state->inst, idx);
            return VTSS_RC_ERROR;
        }

        // We must disable cut-through in recovery mode, because we can't be
        // sure that the frame reaches its egress port.
        action.cut_through_override     = true;
        action.cut_through_disable      = true;
        action.frer.frer.mstream_enable = true;
        action.frer.frer.mstream_id     = mstream_base_id;
    }

    // Get frames classified to the FRER VLAN
    action.frer.vid = frer_state->conf.frer_vlan;

    if (frer_state->conf.mode == VTSS_APPL_FRER_MODE_GENERATION) {
        // Preserve user's tags. pop_enable just enables me to override the
        // pop-cnt, which is set to 0.
        action.frer.pop_enable = true;
        action.frer.pop_cnt    = frer_state->conf.outer_tag_pop ? 1 : 0;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_stream_detach()
/******************************************************************************/
static mesa_rc FRER_BASE_stream_detach(frer_state_t *frer_state, int idx)
{
    frer_stream_state_t              *stream_state = &frer_state->stream_states[idx];
    vtss_appl_stream_id_t            stream_id;
    vtss_appl_stream_collection_id_t stream_collection_id;
    vtss_appl_stream_action_t        action = {};
    mesa_rc                          rc;

    if (stream_state->exists && !stream_state->attach_failed) {
        if (frer_state->using_stream_collection()) {
            stream_collection_id = frer_state->conf.stream_collection_id;
            rc = vtss_appl_stream_collection_action_set(stream_collection_id, VTSS_APPL_STREAM_CLIENT_FRER, &action);
            T_IG(FRER_TRACE_GRP_BASE, "%u: vtss_appl_stream_collection_action_set(%u, enable = false): %s", frer_state->inst, stream_collection_id, error_txt(rc));
        } else {
            // The stream (used) to exist and we have attached to it.
            stream_id = frer_state->conf.stream_ids[idx];
            rc = vtss_appl_stream_action_set(stream_id, VTSS_APPL_STREAM_CLIENT_FRER, &action);
            T_IG(FRER_TRACE_GRP_BASE, "%u: vtss_appl_stream_action_set(%u, enable = false): %s", frer_state->inst, stream_id, error_txt(rc));
        }
    }

    stream_state->exists        = false;
    stream_state->attach_failed = false;
    stream_state->ingress_ports.clear_all();
    return rc;
}

/******************************************************************************/
// FRER_BASE_stream_attach()
/******************************************************************************/
static void FRER_BASE_stream_attach(frer_state_t *frer_state, int idx)
{
    frer_stream_state_t       *stream_state = &frer_state->stream_states[idx];
    vtss_appl_stream_id_t     stream_id     = frer_state->conf.stream_ids[idx];
    vtss_appl_stream_action_t action;
    vtss_appl_stream_conf_t   stream_conf;
    vtss_appl_stream_status_t stream_status;
    mesa_rc                   rc1, rc2;

    if ((rc1 = vtss_appl_stream_conf_get(stream_id, &stream_conf)) != VTSS_RC_OK) {
        T_IG(FRER_TRACE_GRP_BASE, "%u: idx = %d: vtss_appl_stream_conf_get(%u) failed: %s", frer_state->inst, idx, stream_id, error_txt(rc1));
    }

    if ((rc2 = vtss_appl_stream_status_get(stream_id, &stream_status)) != VTSS_RC_OK) {
        T_IG(FRER_TRACE_GRP_BASE, "%u: idx = %d: vtss_appl_stream_status_get(%u) failed: %s", frer_state->inst, idx, stream_id, error_txt(rc2));
    }

    if (rc1 != VTSS_RC_OK || rc2 != VTSS_RC_OK) {
        // Stream doesn't exist. This is not an error, since it might not have
        // been created yet (or has been deleted).
        stream_state->exists        = false;
        stream_state->attach_failed = false;
        return;
    }

    // Stream exists.
    stream_state->exists = true;
    stream_state->ingress_ports = stream_conf.port_list;

    if (stream_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_FRER].enable) {
        // We are already attached to the stream. Check that the ID is indeed
        // this instance's. Otherwise we have a problem, Houston, that requires
        // a code update. We shouldn't be able to get into this situation,
        // because of the checks at the bottom of vtss_appl_frer_conf_set():
        // Two administratively enabled instances cannot use the same stream
        // ID.
        T_IG(FRER_TRACE_GRP_BASE, "%u: We are already attached to stream %u", frer_state->inst, stream_id);

        if (stream_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_FRER].client_id != frer_state->inst) {
            // This requires a code update, and is not an operational warning.
            T_EG(FRER_TRACE_GRP_BASE, "%u: Stream (%u) attached to by us, but on another FRER instance (%u)", frer_state->inst, stream_id, stream_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_FRER].client_id);
        }

        return;
    }

    // Currently, we are not attached to the stream. Do it!
    if (FRER_BASE_stream_action_get(frer_state, action, idx) != VTSS_RC_OK) {
        stream_state->attach_failed = true;
        return;
    }

    if (frer_state->conf.mode == VTSS_APPL_FRER_MODE_GENERATION) {
        // When attaching to a stream for the first time, clear the stream's
        // counters (in generation mode only, because they are not used in
        // recovery mode).
        (void)stream_util_counters_clear(stream_id);
    }

    rc1 = vtss_appl_stream_action_set(stream_id, VTSS_APPL_STREAM_CLIENT_FRER, &action);
    T_IG(FRER_TRACE_GRP_BASE, "%u: vtss_appl_stream_action_set(%u, enable = true): %s", frer_state->inst, stream_id, error_txt(rc1));

    stream_state->attach_failed = rc1 != VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_stream_collection_attach()
/******************************************************************************/
static void FRER_BASE_stream_collection_attach(frer_state_t *frer_state)
{
    frer_stream_state_t                  *stream_state         = &frer_state->stream_states[0]; // Only index 0 is used with stream collections
    vtss_appl_stream_collection_id_t     stream_collection_id  = frer_state->conf.stream_collection_id;
    vtss_appl_stream_action_t            action;
    vtss_appl_stream_collection_conf_t   stream_collection_conf;
    vtss_appl_stream_collection_status_t stream_collection_status;
    vtss_appl_stream_id_t                stream_id;
    vtss_appl_stream_conf_t              stream_conf;
    int                                  idx;
    mesa_rc                              rc1, rc2;

    if ((rc1 = vtss_appl_stream_collection_conf_get(stream_collection_id, &stream_collection_conf)) != VTSS_RC_OK) {
        T_IG(FRER_TRACE_GRP_BASE, "%u: vtss_appl_stream_collection_conf_get(%u) failed: %s", frer_state->inst, stream_collection_id, error_txt(rc1));
    }

    if ((rc2 = vtss_appl_stream_collection_status_get(stream_collection_id, &stream_collection_status)) != VTSS_RC_OK) {
        T_IG(FRER_TRACE_GRP_BASE, "%u: vtss_appl_stream_collection_status_get(%u) failed: %s", frer_state->inst, stream_collection_id, error_txt(rc2));
    }

    if (rc1 != VTSS_RC_OK || rc2 != VTSS_RC_OK) {
        // Stream collection doesn't exist. This is not an error, since it might
        // not have been created yet (or has been deleted).
        stream_state->exists        = false;
        stream_state->attach_failed = false;
        return;
    }

    // Stream collection exists.
    stream_state->exists = true;

    // Gotta go through all streams in the collection to get an aggregated
    // ingress port list (for the sake of operational warnings).
    stream_state->ingress_ports.clear_all();
    for (idx = 0; idx < ARRSZ(stream_collection_conf.stream_ids); idx++) {
        stream_id = stream_collection_conf.stream_ids[idx];

        if (stream_id == VTSS_APPL_STREAM_ID_NONE) {
            // This marks the end of the list.
            break;
        }

        if ((rc1 = vtss_appl_stream_conf_get(stream_id, &stream_conf)) == VTSS_RC_OK) {
            stream_state->ingress_ports |= stream_conf.port_list;
        } else {
            T_IG(FRER_TRACE_GRP_BASE, "%u: vtss_appl_stream_conf_get(%u) failed: %s", frer_state->inst, stream_id, error_txt(rc1));
        }
    }

    if (stream_collection_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_FRER].enable) {
        // We are already attached to the stream collection. Check that the ID
        // is indeed this instance's. Otherwise we have a problem, Houston, that
        // requires a code update. We shouldn't be able to get into this
        // situation, because of the checks at the bottom of
        // vtss_appl_frer_conf_set(): Two administratively enabled instances
        // cannot use the same stream collection ID.
        T_IG(FRER_TRACE_GRP_BASE, "%u: We are already attached to stream collection %u", frer_state->inst, stream_collection_id);

        if (stream_collection_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_FRER].client_id != frer_state->inst) {
            // This requires a code update, and is not an operational warning.
            T_EG(FRER_TRACE_GRP_BASE, "%u: Stream collection (%u) attached to by us, but on another FRER instance (%u)", frer_state->inst, stream_collection_id, stream_collection_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_FRER].client_id);
        }

        return;
    }

    // Currently, we are not attached to the stream collection. Do it!
    if (FRER_BASE_stream_action_get(frer_state, action, 0 /* doesn't matter */) != VTSS_RC_OK) {
        stream_state->attach_failed = true;
        return;
    }

    if (frer_state->conf.mode == VTSS_APPL_FRER_MODE_GENERATION) {
        // When attaching to a stream collection for the first time, clear the
        // stream collection's counters (in generation mode only, because they
        // are not used in recovery mode).
        (void)stream_collection_util_counters_clear(stream_collection_id);
    }

    rc1 = vtss_appl_stream_collection_action_set(stream_collection_id, VTSS_APPL_STREAM_CLIENT_FRER, &action);
    T_IG(FRER_TRACE_GRP_BASE, "%u: vtss_appl_stream_collection_action_set(%u, enable = true): %s", frer_state->inst, stream_collection_id, error_txt(rc1));

    stream_state->attach_failed = rc1 != VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_stream_or_collection_attach()
// The reason that this function doesn't return a mesa_rc is that it's not an
// error if the stream (collection) doesn't exist.
// However, it will be flagged as a warning by the upper layer (frer.cxx), who
// uses stream_state->exists and stream_state->attach_failed for operational
// warnings.
/******************************************************************************/
static void FRER_BASE_stream_or_collection_attach(frer_state_t *frer_state, int idx)
{
    if (frer_state->using_stream_collection()) {
        FRER_BASE_stream_collection_attach(frer_state);
    } else {
        FRER_BASE_stream_attach(frer_state, idx);
    }
}

/******************************************************************************/
// FRER_BASE_tce_activate()
/******************************************************************************/
static mesa_rc FRER_BASE_tce_activate(frer_state_t *frer_state)
{
    mesa_tce_t tce_conf;
    mesa_rc    rc;

    VTSS_RC(FRER_BASE_tce_id_alloc(frer_state));

    if ((rc = mesa_tce_init(nullptr, &tce_conf)) != VTSS_RC_OK) {
        T_EG(FRER_TRACE_GRP_BASE, "%u: mesa_tce_init() failed: %s", frer_state->inst, error_txt(rc));
        // Convert to something sensible
        return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
    }

    tce_conf.id = frer_state->tce_id;

    // Could have been nice to be able to match on IFLOW as well, but that only
    // works for CPU-based CCM injection.
    //
    // In generation mode, we create a TCE for the user-specified egress ports.
    // This means that an R-Tag will be added on these ports only. If another
    // port is member of the FRER VLAN, it will also send the frame, but without
    // an R-Tag.
    //
    // In recovery mode, we also create a TCE for the user-specified egress
    // ports and add an R-Tag - unless we are terminating the flows, in which
    // case we pop the R-Tag.
    //
    // Note that on FA, one TCE maps to an ES0 rule per egress port, so we want
    // to keep the number of egress ports to a minimum.
    tce_conf.key.port_list = frer_state->conf.egress_ports;
    tce_conf.key.vid       = frer_state->conf.frer_vlan;

    tce_conf.action.tag.pcp_sel = MESA_PCP_SEL_PORT;    // Let the port decide PCP
    tce_conf.action.tag.dei_sel = MESA_DEI_SEL_PORT;    // Let the port decide DEI
    tce_conf.action.tag.map_id  = MESA_QOS_MAP_ID_NONE; // Don't use a QoS map.
    tce_conf.action.tag.tpid    = MESA_TPID_SEL_PORT;   // Let the port choose.

    if (frer_state->conf.mode == VTSS_APPL_FRER_MODE_RECOVERY && frer_state->conf.rcvy_terminate) {
        tce_conf.action.rtag.pop = true;
    } else {
        // In generation and non-terminating recovery mode, we push an R-Tag.
        // Note that if using MESA_RTAG_SEL_OUTER we cannot get a C- or S-tag on
        // the frame.
        tce_conf.action.rtag.sel = MESA_RTAG_SEL_INNER;
    }

    if ((rc = mesa_tce_add(nullptr, MESA_TCE_ID_LAST, &tce_conf)) != VTSS_RC_OK) {
        // All places where we may run dry of resources, we don't use T_E(), but
        // T_I()
        T_IG(FRER_TRACE_GRP_BASE, "%u: mesa_tce_add(0x%x) failed: %s", frer_state->inst, frer_state->tce_id, error_txt(rc));

        // Avoid deleting this ID in the API later on, since we haven't added a
        // new entry.
        FRER_BASE_tce_id_free(frer_state);

        // And convert error code to something sensible
        return VTSS_APPL_FRER_RC_HW_RESOURCES;
    }

    T_IG(FRER_TRACE_GRP_BASE, "%u: mesa_tce_add(0x%x) succeeded", frer_state->inst, frer_state->tce_id);

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_tce_deactivate()
/******************************************************************************/
static mesa_rc FRER_BASE_tce_deactivate(frer_state_t *frer_state)
{
    mesa_rc rc;

    if (frer_state->tce_id == FRER_TCE_ID_NONE) {
        return VTSS_RC_OK;
    }

    if ((rc = mesa_tce_del(nullptr, frer_state->tce_id)) != VTSS_RC_OK) {
        T_EG(FRER_TRACE_GRP_BASE, "%u: mesa_tce_del(0x%x) failed: %s", frer_state->inst, frer_state->tce_id, error_txt(rc));

        // Convert to something sensible
        rc = VTSS_APPL_FRER_RC_INTERNAL_ERROR;
    } else {
        T_IG(FRER_TRACE_GRP_BASE, "%u: mesa_tce_del(0x%x) succeeded", frer_state->inst, frer_state->tce_id);
    }

    FRER_BASE_tce_id_free(frer_state);
    return rc;
}

/******************************************************************************/
// FRER_BASE_cstreams_activate()
/******************************************************************************/
static mesa_rc FRER_BASE_cstreams_activate(frer_state_t *frer_state)
{
    mesa_frer_stream_conf_t cstream_conf;
    mesa_port_no_t          port_no;
    int                     cnt;

    if (frer_state->conf.mode != VTSS_APPL_FRER_MODE_RECOVERY) {
        // No compound streams needed when not in recovery mode.
        return VTSS_RC_OK;
    }

    vtss_clear(cstream_conf);
    cstream_conf.recovery    = true; // Always enable recovery on cstreams
    cstream_conf.alg         = frer_state->conf.rcvy_algorithm;
    cstream_conf.hlen        = frer_state->conf.rcvy_history_len;
    cstream_conf.reset_time  = frer_state->conf.rcvy_reset_timeout_ms;
    cstream_conf.take_no_seq = frer_state->conf.rcvy_take_no_sequence;

    // We need a cstream per egress port.
    cnt = 0;
    for (port_no = 0; port_no < FRER_BASE_cap_port_cnt; port_no++) {
        if (!frer_state->conf.egress_ports.get(port_no)) {
            continue;
        }

        // Allocate a compound stream for this egress port.
        VTSS_RC(FRER_BASE_cstream_id_alloc(frer_state, cnt));

        // Before assigning this cstream a configuration, clear its counters,
        // because after setting recovery = true, we need to have the reset
        // count incremented to 1 (which happens in S/W in the API on some
        // platforms).
        VTSS_RC(FRER_BASE_cstream_cnt_clear(frer_state, frer_state->cstream_ids[cnt], cnt, __FUNCTION__, __LINE__));

        // Set the configuration
        VTSS_RC(FRER_BASE_cstream_conf_access(frer_state, frer_state->cstream_ids[cnt], &cstream_conf, FRER_BASE_ACCESS_METHOD_SET, cnt, __FUNCTION__, __LINE__));

        cnt++;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_cstreams_deactivate()
/******************************************************************************/
static mesa_rc FRER_BASE_cstreams_deactivate(frer_state_t *frer_state)
{
    int idx;

    // There's no free() or del() function involved with cstream_ids.
    for (idx = 0; idx < ARRSZ(frer_state->cstream_ids); idx++) {
        // Loop through all of them to ensure a consistent state - should this
        // instance be reused.
        FRER_BASE_cstream_id_free(frer_state, idx);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_mstreams_activate()
/******************************************************************************/
static mesa_rc FRER_BASE_mstreams_activate(frer_state_t *frer_state)
{
    mesa_frer_mstream_id_t  mstream_base_id;
    mesa_frer_cstream_id_t  cstream_id;
    mesa_frer_stream_conf_t mstream_conf;
    mesa_port_no_t          port_no;
    int                     idx, cstream_idx;

    if (frer_state->conf.mode != VTSS_APPL_FRER_MODE_RECOVERY) {
        // No compound streams needed when not in recovery mode.
        return VTSS_RC_OK;
    }

    memset(&mstream_conf, 0, sizeof(mstream_conf));

    mstream_conf.recovery    = frer_state->conf.rcvy_individual;
    mstream_conf.alg         = frer_state->conf.rcvy_algorithm;
    mstream_conf.hlen        = frer_state->conf.rcvy_history_len;
    mstream_conf.reset_time  = frer_state->conf.rcvy_reset_timeout_ms;
    mstream_conf.take_no_seq = frer_state->conf.rcvy_take_no_sequence;

    // We leave mstream_conf.cstream_id for the loop below.

    // We need:
    // - One set of mstreams (one call to mesa_frer_mstream_alloc(), which
    //   allocates N mstreams under the hood, where N is the number of egress
    //   ports).
    // - If individual recovery is enabled, we need to do this for each ingress
    //   stream ID.
    for (idx = 0; idx < ARRSZ(frer_state->mstream_base_ids); idx++) {
        if (frer_state->using_stream_collection()) {
            // We allocate only one mstream and we cannot use individual
            // recovery, so we exit the loop below when idx > 0.
        } else {
            if (frer_state->conf.stream_ids[idx] == VTSS_APPL_STREAM_ID_NONE) {
                // Invariant: First occurrence of VTSS_APPL_STREAM_ID_NONE
                // marks the end of the list.
                break;
            }
        }

        if (idx > 0 && !frer_state->individual_recovery()) {
            // We only need one set of mstreams in non-individual recovery
            break;
        }

        VTSS_RC(FRER_BASE_mstream_id_alloc(frer_state, idx));

        mstream_base_id = frer_state->mstream_base_ids[idx];

        // Connect the mstreams to the cstreams. Remember, there is one cstream
        // per egress port, so we iterate across egress ports, because that's
        // what the API expects.
        cstream_idx = 0;
        for (port_no = 0; port_no < FRER_BASE_cap_port_cnt; port_no++) {
            if (!frer_state->conf.egress_ports.get(port_no)) {
                continue;
            }

            cstream_id = frer_state->cstream_ids[cstream_idx];

            if (cstream_id == FRER_CSTREAM_ID_NONE) {
                T_EG(FRER_TRACE_GRP_BASE, "%u: cstream not allocated for port %u", frer_state->inst, port_no);
                return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
            }

            mstream_conf.cstream_id = cstream_id;

            // Before assigning this mstream a configuration, clear its
            // counters, because after setting recovery = true, we need to have
            // the reset count incremented to 1 (which happens in S/W in the API
            // on some platforms).
            VTSS_RC(FRER_BASE_mstream_cnt_clear(frer_state, mstream_base_id, port_no, idx, __FUNCTION__, __LINE__));

            // We have allocated N (number of egress ports) mstreams that we
            // need to configure. They are allocated in order, but we only got
            // the base mstream ID back from the API.
            VTSS_RC(FRER_BASE_mstream_conf_access(frer_state, mstream_base_id, port_no, &mstream_conf, FRER_BASE_ACCESS_METHOD_SET, idx, __FUNCTION__, __LINE__));

            cstream_idx++;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_mstreams_deactivate()
/******************************************************************************/
static mesa_rc FRER_BASE_mstreams_deactivate(frer_state_t *frer_state)
{
    int     idx;
    mesa_rc rc, first_encountered_rc = VTSS_RC_OK;

    // Free all mstreams
    for (idx = 0; idx < ARRSZ(frer_state->mstream_base_ids); idx++) {
        // Loop through all of them to ensure a consistent state - should this
        // instance be reused.
        if ((rc = FRER_BASE_mstream_id_free(frer_state, idx)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    }

    return first_encountered_rc;
}

/******************************************************************************/
// FRER_BASE_vces_activate()
/******************************************************************************/
static mesa_rc FRER_BASE_vces_activate(frer_state_t *frer_state)
{
    int idx;

    // Attach to stream collection or all streams
    for (idx = 0; idx < ARRSZ(frer_state->stream_states); idx++) {
        if (frer_state->using_stream_collection()) {
            // If using stream collections, only idx 0 is valid.
            if (idx > 0) {
                break;
            }
        } else {
            if (frer_state->conf.stream_ids[idx] == VTSS_APPL_STREAM_ID_NONE) {
                // Invariant: First occurrence of VTSS_APPL_STREAM_ID_NONE marks
                // the end of the list.
                break;
            }
        }

        FRER_BASE_stream_or_collection_attach(frer_state, idx);
    }

    // This function cannot fail, because
    // FRER_BASE_stream_or_collection_attach() doesn't return a mesa_rc, because
    // it just sets some state variables that can be used by frer.cxx to issue
    // an operational warning.
    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_vces_deactivate()
/******************************************************************************/
static mesa_rc FRER_BASE_vces_deactivate(frer_state_t *frer_state)
{
    int idx;
    mesa_rc rc, first_encountered_rc = VTSS_RC_OK;

    // Detach from stream collection or all streams.
    for (idx = 0; idx < ARRSZ(frer_state->stream_states); idx++) {
        if (frer_state->using_stream_collection()) {
            // If using stream collections, only idx 0 is valid.
            if (idx > 0) {
                break;
            }
        } else {
            if (frer_state->conf.stream_ids[idx] == VTSS_APPL_STREAM_ID_NONE) {
                // Invariant: First occurrence of VTSS_APPL_STREAM_ID_NONE marks the
                // end of the list.
                break;
            }
        }

        if ((rc = FRER_BASE_stream_detach(frer_state, idx)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                rc = first_encountered_rc;
            }
        }
    }

    return first_encountered_rc;
}

/******************************************************************************/
// FRER_BASE_led_reset()
// Corresponds to LatentErrorReset() as defined in 802.1CB, clause 7.4.4.3.
/******************************************************************************/
static mesa_rc FRER_BASE_led_reset(frer_state_t *frer_state)
{
    mesa_frer_counters_t   ctrs;
    mesa_frer_cstream_id_t cstream_id;
    frer_led_state_t       &led_state = frer_state->led_state;
    uint32_t               paths      = frer_state->conf.rcvy_latent_error_detection.paths;
    int                    cstream_idx;

    for (cstream_idx = 0; cstream_idx < ARRSZ(frer_state->cstream_ids); cstream_idx++) {
        cstream_id = frer_state->cstream_ids[cstream_idx];

        if (cstream_id == FRER_CSTREAM_ID_NONE) {
            // No more cstreams to monitor
            break;
        }

        VTSS_RC(FRER_BASE_cstream_cnt_get(frer_state, cstream_id, &ctrs, cstream_idx, __FUNCTION__, __LINE__));

        // Let's hope the IEEE-guys have thought about wrap-around. I'm not yet
        // convinced...
        led_state.cur_base_difference[cstream_idx] = ctrs.passed_packets * (paths - 1) - ctrs.discarded_packets;
    }

    led_state.led_resets++;
    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_led_notif_status_update()
/******************************************************************************/
static mesa_rc FRER_BASE_led_notif_status_update(frer_state_t *frer_state, bool new_latent_error, bool get_first = true)
{
    vtss_appl_frer_notification_status_t notif_status;
    mesa_rc                              rc;

    T_IG(FRER_TRACE_GRP_NOTIF, "%u: Setting latent error to %d", frer_state->inst, new_latent_error);

    frer_state->status.notif_status.latent_error = new_latent_error;

    if (get_first) {
        // Update the notification status.
        if ((rc = frer_notification_status.get(frer_state->inst, &notif_status)) != VTSS_RC_OK) {
            T_EG(FRER_TRACE_GRP_NOTIF, "%u: Unable to get notification status: %s", frer_state->inst, error_txt(rc));
        }
    } else {
        vtss_clear(notif_status);
    }

    notif_status.latent_error = new_latent_error;

    if ((rc = frer_notification_status.set(frer_state->inst, &notif_status)) != VTSS_RC_OK) {
        T_EG(FRER_TRACE_GRP_NOTIF, "%u: Unable to set notification status to %d: %s", frer_state->inst, new_latent_error, error_txt(rc));
    }

    return rc;
}

/******************************************************************************/
// FRER_BASE_led_test()
// Corresponds to LatentErrorTest() as defined in 802.1CB, clause 7.4.4.4.
/******************************************************************************/
static mesa_rc FRER_BASE_led_test(frer_state_t *frer_state)
{
    mesa_frer_counters_t   ctrs;
    mesa_frer_cstream_id_t cstream_id;
    frer_led_state_t       &led_state = frer_state->led_state;
    uint32_t               paths      = frer_state->conf.rcvy_latent_error_detection.paths;
    uint32_t               conf_diff  = frer_state->conf.rcvy_latent_error_detection.difference;
    int64_t                diff;
    int                    cstream_idx;
    bool                   latent_error = false;

    for (cstream_idx = 0; cstream_idx < ARRSZ(frer_state->cstream_ids); cstream_idx++) {
        cstream_id = frer_state->cstream_ids[cstream_idx];

        if (cstream_id == FRER_CSTREAM_ID_NONE) {
            // No more cstreams to monitor
            break;
        }

        VTSS_RC(FRER_BASE_cstream_cnt_get(frer_state, cstream_id, &ctrs, cstream_idx, __FUNCTION__, __LINE__));

        // Let's hope the IEEE-guys have thought about wrap-around. I'm not yet
        // convinced...
        diff = led_state.cur_base_difference[cstream_idx] - ((ctrs.passed_packets * (paths - 1)) - ctrs.discarded_packets);

        if (diff < 0) {
            diff = -diff;
        }

        if (diff > conf_diff) {
            // SIGNAL_LATENT_ERROR
            // This is the combined latent error status for all egress ports.
            latent_error = true;

            // Just for debug
            led_state.latent_error[cstream_idx] = true;
        } else {
            // Just for debug
            led_state.latent_error[cstream_idx] = false;
        }
    }

    // notif_status.latent_error is sticky, so we can only set it in here, not
    // clear it.
    if (latent_error) {
        return FRER_BASE_led_notif_status_update(frer_state, latent_error);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_control_latent_error_clear()
/******************************************************************************/
static mesa_rc FRER_BASE_control_latent_error_clear(frer_state_t *frer_state)
{
    if (frer_state->conf.mode != VTSS_APPL_FRER_MODE_RECOVERY) {
        return VTSS_RC_OK;
    }

    if (!frer_state->conf.rcvy_latent_error_detection.enable) {
        return VTSS_RC_OK;
    }

    return FRER_BASE_led_notif_status_update(frer_state, false);
}

/******************************************************************************/
// FRER_BASE_led_reset_timeout()
// Corresponds to RESET_LATENT_ERROR signal from 802.1CB, clause 7.4.4.1.
/******************************************************************************/
static void FRER_BASE_led_reset_timeout(tsn_timer_t &timer, void *context)
{
    frer_state_t *frer_state = (frer_state_t *)context;

    VTSS_ASSERT(frer_state);

    FRER_LOCK_SCOPE();

    T_DG(FRER_TRACE_GRP_BASE, "%u: Reset timeout", frer_state->inst);
    (void)FRER_BASE_led_reset(frer_state);
}

/******************************************************************************/
// FRER_BASE_led_test_timeout()
// Corresponds to TRIGGER_LATENT_ERROR_TEST signal from 802.1CB, clause 7.4.4.1.
/******************************************************************************/
static void FRER_BASE_led_test_timeout(tsn_timer_t &timer, void *context)
{
    frer_state_t *frer_state = (frer_state_t *)context;

    VTSS_ASSERT(frer_state);

    FRER_LOCK_SCOPE();

    T_DG(FRER_TRACE_GRP_BASE, "%u: Test timeout", frer_state->inst);
    (void)FRER_BASE_led_test(frer_state);
}

/******************************************************************************/
// FRER_BASE_led_activate()
// In this context, led stands for "latent error detection".
// This corresponds to the BEGIN signal in clause 7.4.4.1.
/******************************************************************************/
static mesa_rc FRER_BASE_led_activate(frer_state_t *frer_state)
{
    vtss_appl_frer_latent_error_detection_t &led_conf = frer_state->conf.rcvy_latent_error_detection;

    // Create an entry in the global notification status table - independent of
    // mode and whether latent error detection is enabled.
    VTSS_RC(FRER_BASE_led_notif_status_update(frer_state, false, false));

    frer_state->led_state.led_resets = 0;

    if (frer_state->conf.mode == VTSS_APPL_FRER_MODE_GENERATION) {
        // No error detection in generation mode.
        return VTSS_RC_OK;
    }

    if (!led_conf.enable) {
        // It's not enabled. Nothing to do.
        return VTSS_RC_OK;
    }

    // Start the latent error reset and latent error test timers.
    // Unfortunately, this requires us to take a TSN mutex.
    {
        TSN_LOCK_SCOPE();

        tsn_timer_start(frer_state->led_state.reset_timer, led_conf.reset_period_ms, true);
        tsn_timer_start(frer_state->led_state.test_timer,  led_conf.period_ms,       true);
    }

    // BEGIN signal
    return FRER_BASE_led_reset(frer_state);
}

/******************************************************************************/
// FRER_BASE_led_deactivate()
/******************************************************************************/
static mesa_rc FRER_BASE_led_deactivate(frer_state_t *frer_state)
{
    {
        TSN_LOCK_SCOPE();

        tsn_timer_stop(frer_state->led_state.reset_timer);
        tsn_timer_stop(frer_state->led_state.test_timer);
    }

    // Remove this instance's entry from the global notification table.
    // It's not an error if we can't because it might be that it has never been
    // created.
    (void)frer_notification_status.del(frer_state->inst);

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_control_reset()
/******************************************************************************/
static mesa_rc FRER_BASE_control_reset(frer_state_t *frer_state)
{
    vtss_appl_stream_conf_t stream_conf;
    mesa_frer_mstream_id_t  mstream_base_id;
    mesa_frer_cstream_id_t  cstream_id;
    mesa_frer_stream_conf_t mstream_conf, cstream_conf;
    mesa_port_no_t          port_no;
    int                     mstream_idx, cstream_idx;
    mesa_rc                 rc;

    if (frer_state->conf.mode == VTSS_APPL_FRER_MODE_GENERATION) {
        // In order to reset the sequence number generator, we need to re-apply
        // the IFLOW configuration. This is done with a call to the stream
        // module.

        if (frer_state->using_stream_collection()) {
            if ((rc = vtss_appl_stream_collection_action_set(frer_state->conf.stream_collection_id, VTSS_APPL_STREAM_CLIENT_FRER, nullptr, true /* Re-apply IFLOW configuration */)) != VTSS_RC_OK) {
                // This is not an error, since it might be that the stream
                // collection is not created, so only print a trace info, not a
                // trace error.
                T_IG(FRER_TRACE_GRP_BASE, "%u: vtss_appl_stream_collection_action_set(%u) failed: %s", frer_state->inst, frer_state->conf.stream_collection_id, error_txt(rc));
            }
        } else {
            if ((rc = vtss_appl_stream_action_set(frer_state->conf.stream_ids[0], VTSS_APPL_STREAM_CLIENT_FRER, nullptr, true /* Re-apply IFLOW configuration */)) != VTSS_RC_OK) {
                // This is not an error, since it might be that the stream is
                // not created, so only print a trace info, not a trace error.
                T_IG(FRER_TRACE_GRP_BASE, "%u: vtss_appl_stream_action_set(%u) failed: %s", frer_state->inst, frer_state->conf.stream_ids[0], error_txt(rc));
            }
        }

        // Increase the S/W-controlled reset counter.
        frer_state->gen_resets++;

        return VTSS_RC_OK;
    }

    // In recovery mode, we have cstreams and possibly also mstreams to reset.

    // First mstreams
    if (frer_state->individual_recovery()) {
        for (mstream_idx = 0; mstream_idx < ARRSZ(frer_state->mstream_base_ids); mstream_idx++) {
            mstream_base_id = frer_state->mstream_base_ids[mstream_idx];

            if (mstream_base_id == FRER_MSTREAM_BASE_ID_NONE) {
                // No more mstreams to clear.
                break;
            }

            for (port_no = 0; port_no < FRER_BASE_cap_port_cnt; port_no++) {
                if (!frer_state->conf.egress_ports.get(port_no)) {
                    continue;
                }

                // Getting and setting the same mstream configuration causes
                // apparently the recovery functions to reset. I'm not 100% sure
                // that traffic won't get lost or not get counted.
                VTSS_RC(FRER_BASE_mstream_conf_access(frer_state, mstream_base_id, port_no, &mstream_conf, FRER_BASE_ACCESS_METHOD_GET_SET, mstream_idx, __FUNCTION__, __LINE__));
            }
        }
    }

    // Then cstreams
    for (cstream_idx = 0; cstream_idx < ARRSZ(frer_state->cstream_ids); cstream_idx++) {
        cstream_id = frer_state->cstream_ids[cstream_idx];

        if (cstream_id == FRER_CSTREAM_ID_NONE) {
            // No more cstreams to clear
            break;
        }

        // Getting and setting the same cstream configuration causes apparently
        // the recovery functions to reset. I'm not 100% sure that traffic won't
        // get lost or not get counted.
        VTSS_RC(FRER_BASE_cstream_conf_access(frer_state, cstream_id, &cstream_conf, FRER_BASE_ACCESS_METHOD_GET_SET, cstream_idx, __FUNCTION__, __LINE__));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_BASE_general_dump()
/******************************************************************************/
static void FRER_BASE_general_dump(uint32_t session_id, frer_icli_pr_t pr)
{
    frer_itr_t   itr;
    frer_state_t *frer_state;
    char         tce_buf[15];
    bool         first = true;

    for (itr = FRER_map.begin(); itr != FRER_map.end(); ++itr) {
        frer_state = &itr->second;

        if (first) {
            pr(session_id, "General:\n");
            pr(session_id, "Inst Mode       Oper. State    TCE ID\n");
            pr(session_id, "---- ---------- -------------- ----------\n");
            first = false;
        }

        if (frer_state->tce_id != FRER_TCE_ID_NONE) {
            sprintf(tce_buf, "0x%08x", frer_state->tce_id);
        } else {
            strcpy(tce_buf, "<NONE>");
        }

        pr(session_id, "%4u %-10s %-14s %10s\n",
           frer_state->inst,
           frer_util_mode_to_str(frer_state->conf.mode, true),
           frer_util_oper_state_to_str(frer_state->status.oper_state),
           tce_buf);
    }
}

/******************************************************************************/
// FRER_BASE_stream_dump()
/******************************************************************************/
static void FRER_BASE_stream_dump(uint32_t session_id, frer_icli_pr_t pr)
{
    frer_itr_t                           itr;
    frer_state_t                         *frer_state;
    vtss_appl_stream_collection_status_t stream_collection_status;
    vtss_appl_stream_status_t            stream_status;
    vtss_appl_stream_id_t                stream_id;
    int                                  idx;
    bool                                 first = true, exists, attached;

    for (itr = FRER_map.begin(); itr != FRER_map.end(); ++itr) {
        frer_state = &itr->second;

        if (frer_state->status.oper_state != VTSS_APPL_FRER_OPER_STATE_ACTIVE) {
            continue;
        }

        if (frer_state->using_stream_collection()) {
            exists   = false;
            attached = false;

            if (vtss_appl_stream_collection_status_get(frer_state->conf.stream_collection_id, &stream_collection_status) != VTSS_RC_OK) {
                // Stream collection doesn't exist. That's not an error, but an
                // operational warning.
            } else {
                exists = true;

                if (stream_collection_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_FRER].enable) {
                    attached = true;

                    if (stream_collection_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_FRER].client_id != frer_state->inst) {
                        T_EG(FRER_TRACE_GRP_BASE, "%u: Internal error: Mismatch in client ID on Stream Collection ID %u: In Stream conf: %u", frer_state->inst, frer_state->conf.stream_collection_id, stream_collection_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_FRER].client_id);
                    }
                }
            }

            if (first) {
                pr(session_id, "\nVCEs:\n");
                pr(session_id, "Inst Stream ID Stream Coll ID Exists Attached\n");
                pr(session_id, "---- --------- -------------- ------ --------\n");
                first = false;
            }

            pr(session_id, "%4u %9s %14u %6s %8s\n",
               frer_state->inst,
               "-",
               frer_state->conf.stream_collection_id,
               frer_util_yes_no_str(exists),
               frer_util_yes_no_str(attached));
        } else {
            for (idx = 0; idx < ARRSZ(frer_state->stream_states); idx++) {
                stream_id = frer_state->conf.stream_ids[idx];

                if (stream_id == VTSS_APPL_STREAM_ID_NONE) {
                    // Invariant: First occurrence of VTSS_APPL_STREAM_ID_NONE marks
                    // the end of the list.
                    break;
                }

                exists   = false;
                attached = false;

                if (vtss_appl_stream_status_get(stream_id, &stream_status) != VTSS_RC_OK) {
                    // Stream doesn't exist. That's not an error, but an operational
                    // warning.
                } else {
                    exists = true;

                    if (stream_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_FRER].enable) {
                        attached = true;

                        if (stream_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_FRER].client_id != frer_state->inst) {
                            T_EG(FRER_TRACE_GRP_BASE, "%u: Internal error: Mismatch in client ID on Stream ID %u: In Stream conf: %u", frer_state->inst, stream_id, stream_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_FRER].client_id);
                        }
                    }
                }

                if (first) {
                    pr(session_id, "\nVCEs:\n");
                    pr(session_id, "Inst Stream ID Stream Coll ID Exists Attached\n");
                    pr(session_id, "---- --------- -------------- ------ --------\n");
                    first = false;
                }

                pr(session_id, "%4u %9u %14s %6s %8s\n",
                   frer_state->inst,
                   stream_id,
                   "-",
                   frer_util_yes_no_str(exists),
                   frer_util_yes_no_str(attached));
            }
        }
    }
}

/******************************************************************************/
// FRER_BASE_mstream_dump()
/******************************************************************************/
static void FRER_BASE_mstream_dump(uint32_t session_id, frer_icli_pr_t pr)
{
    frer_itr_t              itr;
    frer_state_t            *frer_state;
    mesa_frer_stream_conf_t mstream_conf;
    mesa_frer_mstream_id_t  mstream_id;
    mesa_port_no_t          port_no;
    int                     idx;
    bool                    first = true;

    for (itr = FRER_map.begin(); itr != FRER_map.end(); ++itr) {
        frer_state = &itr->second;

        for (idx = 0; idx < ARRSZ(frer_state->mstream_base_ids); idx++) {
            mstream_id = frer_state->mstream_base_ids[idx];
            if (mstream_id == FRER_MSTREAM_BASE_ID_NONE) {
                break;
            }

            for (port_no = 0; port_no < FRER_BASE_cap_port_cnt; port_no++) {
                if (!frer_state->conf.egress_ports.get(port_no)) {
                    continue;
                }

                if (FRER_BASE_mstream_conf_access(frer_state, mstream_id, port_no, &mstream_conf, FRER_BASE_ACCESS_METHOD_GET, idx, __FUNCTION__, __LINE__) != VTSS_RC_OK) {
                    continue;
                }

                if (first) {
                    pr(session_id, "\nMSTREAMS:\n");
                    pr(session_id, "Inst MSTREAM ID port_no Enabled Algorithm hlen reset_time take_no_seq CSTREAM ID\n");
                    pr(session_id, "---- ---------- ------- ------- --------- ---- ---------- ----------- ----------\n");
                    first = false;
                }

                pr(session_id, "%4u %10u %7u %-7s %-9s %4u %10u %11s %10u\n",
                   frer_state->inst, mstream_id, port_no, frer_util_yes_no_str(mstream_conf.recovery),
                   frer_util_rcvy_alg_to_str(mstream_conf.alg, true), mstream_conf.hlen, mstream_conf.reset_time,
                   frer_util_yes_no_str(mstream_conf.take_no_seq), mstream_conf.cstream_id);
            }
        }
    }
}

/******************************************************************************/
// FRER_BASE_cstream_dump()
/******************************************************************************/
static void FRER_BASE_cstream_dump(uint32_t session_id, frer_icli_pr_t pr)
{
    frer_itr_t              itr;
    frer_state_t            *frer_state;
    mesa_frer_stream_conf_t cstream_conf;
    mesa_frer_cstream_id_t  cstream_id;
    int                     idx;
    bool                    first = true;

    for (itr = FRER_map.begin(); itr != FRER_map.end(); ++itr) {
        frer_state = &itr->second;

        for (idx = 0; idx < ARRSZ(frer_state->cstream_ids); idx++) {
            cstream_id = frer_state->cstream_ids[idx];
            if (cstream_id == FRER_CSTREAM_ID_NONE) {
                break;
            }

            if (FRER_BASE_cstream_conf_access(frer_state, cstream_id, &cstream_conf, FRER_BASE_ACCESS_METHOD_GET, idx, __FUNCTION__, __LINE__) != VTSS_RC_OK) {
                continue;
            }

            if (first) {
                pr(session_id, "\nCSTREAMS:\n");
                pr(session_id, "Inst CSTREAM ID Enabled Algorithm hlen reset_time take_no_seq\n");
                pr(session_id, "---- ---------- ------- --------- ---- ---------- -----------\n");
                first = false;
            }

            pr(session_id, "%4u %10u %-7s %-9s %4u %10u %11s\n",
               frer_state->inst, cstream_id, frer_util_yes_no_str(cstream_conf.recovery),
               frer_util_rcvy_alg_to_str(cstream_conf.alg, true), cstream_conf.hlen, cstream_conf.reset_time,
               frer_util_yes_no_str(cstream_conf.take_no_seq));
        }
    }
}

/******************************************************************************/
// FRER_BASE_led_dump()
/******************************************************************************/
static void FRER_BASE_led_dump(uint32_t session_id, frer_icli_pr_t pr)
{
    frer_itr_t             itr;
    frer_state_t           *frer_state;
    mesa_frer_cstream_id_t cstream_id;
    int                    idx;
    bool                   first = true;
    char                   reset_time_buf[100], test_time_buf[100];
    uint64_t               now_ms;

    for (itr = FRER_map.begin(); itr != FRER_map.end(); ++itr) {
        frer_state = &itr->second;

        if (frer_state->status.oper_state != VTSS_APPL_FRER_OPER_STATE_ACTIVE) {
            continue;
        }

        if (frer_state->conf.mode != VTSS_APPL_FRER_MODE_RECOVERY) {
            continue;
        }

        if (!frer_state->conf.rcvy_latent_error_detection.enable) {
            continue;
        }

        if (first) {
            pr(session_id, "\nLatent Error Detection:\n");
            pr(session_id, "Inst CSTREAM ID CurBaseDiff    Latent Error Reset time left [ms] Test time left [ms]\n");
            pr(session_id, "---- ---------- -------------- ------------ -------------------- -------------------\n");
            first = false;
        }

        // First print the overall latent error status.
        now_ms = vtss::uptime_milliseconds();

        sprintf(reset_time_buf, VPRI64d "/%u", (int64_t)frer_state->led_state.reset_timer.timeout_ms - (int64_t)now_ms, frer_state->led_state.reset_timer.period_ms);
        sprintf(test_time_buf,  VPRI64d "/%u", (int64_t)frer_state->led_state.test_timer.timeout_ms  - (int64_t)now_ms, frer_state->led_state.test_timer.period_ms);

        pr(session_id, "%4u %10s %14s %-12s %20s %19s\n",
           frer_state->inst, "", "",
           frer_util_yes_no_str(frer_state->status.notif_status.latent_error),
           reset_time_buf, test_time_buf);

        // Then print the per-egress-port CurBaseDifference and latent error
        // status.
        for (idx = 0; idx < ARRSZ(frer_state->cstream_ids); idx++) {
            cstream_id = frer_state->cstream_ids[idx];
            if (cstream_id == FRER_CSTREAM_ID_NONE) {
                break;
            }

            pr(session_id, "%4u %10u " VPRI64Fu("14") " %s\n",
               frer_state->inst, cstream_id, frer_state->led_state.cur_base_difference[idx],
               frer_util_yes_no_str(frer_state->led_state.latent_error[idx]));
        }
    }
}

/******************************************************************************/
// frer_base_activate()
/******************************************************************************/
mesa_rc frer_base_activate(frer_state_t *frer_state)
{
    mesa_rc rc;

    // Order of activation for generation mode:
    // TCE    (matches <egress_ports, FRER VLAN>). Only needed on older chips.
    // VCE    (key = user-decided through streams. action = <iflow, FRER VLAN>)

    // Order of activation for recovery mode:
    // TCE      (matches <egress_ports, FRER_VLAN>). Only needed on older chips.
    // cstreams (compound stream - one per egress port).
    // mstreams (member stream - one set per iflow, a set being number of egress ports)
    // VCEs     (key = user-decided through streams. action = <iflow, FRER VLAN>)

    T_IG(FRER_TRACE_GRP_BASE, "%u: Activating, mode = %s", frer_state->inst, frer_util_mode_to_str(frer_state->conf.mode));

    if (!FRER_BASE_cap_iflow_pop) {
        if ((rc = FRER_BASE_tce_activate(frer_state)) != VTSS_RC_OK) {
            goto do_exit;
        }
    }

    if ((rc = FRER_BASE_cstreams_activate(frer_state)) != VTSS_RC_OK) {
        goto do_exit;
    }

    if ((rc = FRER_BASE_mstreams_activate(frer_state)) != VTSS_RC_OK) {
        goto do_exit;
    }

    if ((rc = FRER_BASE_vces_activate(frer_state)) != VTSS_RC_OK) {
        goto do_exit;
    }

    if ((rc = FRER_BASE_led_activate(frer_state)) != VTSS_RC_OK) {
        goto do_exit;
    }

do_exit:
    if (rc != VTSS_RC_OK) {
        (void)frer_base_deactivate(frer_state);
    }

    return rc;
}

/******************************************************************************/
// frer_base_deactivate()
/******************************************************************************/
mesa_rc frer_base_deactivate(frer_state_t *frer_state)
{
    mesa_rc rc, first_encountered_rc = VTSS_RC_OK;

    // Order of deactivation for generation mode:
    // VCEs, IFLOWs, TCE (older chips)

    // Order of deactivation for recovery mode:
    // VCEs, IFLOWs, mstreams, cstreams, TCE (older chips)

    T_IG(FRER_TRACE_GRP_BASE, "%u: Deactivating, mode = %s", frer_state->inst, frer_util_mode_to_str(frer_state->conf.mode));

    // We must go through all steps, whether an individual step fails or not.
    if ((rc = FRER_BASE_vces_deactivate(frer_state)) != VTSS_RC_OK) {
        if (first_encountered_rc == VTSS_RC_OK) {
            first_encountered_rc = rc;
        }
    }

    if ((rc = FRER_BASE_mstreams_deactivate(frer_state)) != VTSS_RC_OK) {
        if (first_encountered_rc == VTSS_RC_OK) {
            first_encountered_rc = rc;
        }
    }

    if ((rc = FRER_BASE_cstreams_deactivate(frer_state)) != VTSS_RC_OK) {
        if (first_encountered_rc == VTSS_RC_OK) {
            first_encountered_rc = rc;
        }
    }

    if (!FRER_BASE_cap_iflow_pop) {
        if ((rc = FRER_BASE_tce_deactivate(frer_state)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    }

    if ((rc = FRER_BASE_led_deactivate(frer_state)) != VTSS_RC_OK) {
        if (first_encountered_rc == VTSS_RC_OK) {
            first_encountered_rc = rc;
        }
    }

    return first_encountered_rc;
}

/******************************************************************************/
// frer_base_led_conf_change()
// Only the Latent Error Detection configuration has changed (and it's already
// saved in frer_state->conf).
// Caller only invokes us if the instance is operational active.
/******************************************************************************/
mesa_rc frer_base_led_conf_change( frer_state_t *frer_state)
{
    VTSS_RC(FRER_BASE_led_deactivate(frer_state));
    return FRER_BASE_led_activate(frer_state);
}

/******************************************************************************/
// frer_base_control_reset()
// Caller only invokes us if the instance is operational active.
/******************************************************************************/
mesa_rc frer_base_control_reset(frer_state_t *frer_state, const vtss_appl_frer_control_t *ctrl)
{
    mesa_rc rc, first_encountered_rc = VTSS_RC_OK;

    if (ctrl->latent_error_clear) {
        if ((rc = FRER_BASE_control_latent_error_clear(frer_state)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    }

    if (ctrl->reset) {
        if ((rc = FRER_BASE_control_reset(frer_state)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    }

    return first_encountered_rc;
}

/******************************************************************************/
// frer_base_stream_update()
// Will be called by frer.cxx when a stream gets added, released by someone else
// or deleted and the FRER instance is active and up and running.
/******************************************************************************/
void frer_base_stream_update(frer_state_t *state, int idx)
{
    FRER_BASE_stream_or_collection_attach(state, idx);
}

/******************************************************************************/
// frer_base_state_init()
/******************************************************************************/
mesa_rc frer_base_state_init(frer_state_t *frer_state)
{
    int idx;

    for (idx = 0; idx < ARRSZ(frer_state->stream_states); idx++) {
        frer_stream_state_t *stream_state = &frer_state->stream_states[idx];

        stream_state->exists        = false;
        stream_state->attach_failed = false;
        stream_state->ingress_ports.clear_all();
    }

    frer_state->tce_id = FRER_TCE_ID_NONE;

    for (idx = 0; idx < ARRSZ(frer_state->cstream_ids); idx++) {
        frer_state->cstream_ids[idx] = FRER_CSTREAM_ID_NONE;
    }

    for (idx = 0; idx < ARRSZ(frer_state->mstream_base_ids); idx++) {
        frer_state->mstream_base_ids[idx] = FRER_MSTREAM_BASE_ID_NONE;
    }

    vtss_clear(frer_state->led_state);

    {
        // Prepare the two Latent Error Detection timers. This requires us to
        // take the TSN mutex :-(
        TSN_LOCK_SCOPE();
        tsn_timer_init(frer_state->led_state.reset_timer, "FRER Reset", frer_state->inst, FRER_BASE_led_reset_timeout, frer_state);
        tsn_timer_init(frer_state->led_state.test_timer,  "FRER Test",  frer_state->inst, FRER_BASE_led_test_timeout,  frer_state);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// frer_base_statistics_get()
// All arguments are already checked and \p statistics cleared by caller.
// Caller only invokes us if the instance is operational active.
/******************************************************************************/
mesa_rc frer_base_statistics_get(frer_state_t *frer_state, mesa_port_no_t port_no, vtss_appl_stream_id_t stream_id, vtss_appl_frer_statistics_t *statistics)
{
    vtss_appl_stream_conf_t stream_conf;
    mesa_frer_counters_t    ctrs;
    mesa_ingress_counters_t ingress_counters;
    mesa_port_no_t          port_no_itr;
    int                     cstream_idx, mstream_idx;
    bool                    found;
    mesa_rc                 rc;

    if (frer_state->conf.mode == VTSS_APPL_FRER_MODE_GENERATION) {
        // We only have the number of ingress port matches and resets in
        // generation mode.

        if (frer_state->using_stream_collection()) {
            T_IG(FRER_TRACE_GRP_BASE, "%u: stream_collection_util_counters_get(%u)", frer_state->inst, frer_state->conf.stream_collection_id);
            if ((rc = stream_collection_util_counters_get(frer_state->conf.stream_collection_id, ingress_counters)) != VTSS_RC_OK) {
                // This typically happens when the stream collection doesn't
                // exist or has no attached streams.
                T_IG(FRER_TRACE_GRP_BASE, "%u: stream_collection_util_counters_get(%u) failed: %s", frer_state->inst, frer_state->conf.stream_collection_id, error_txt(rc));
                return VTSS_RC_OK;
            }
        } else {
            T_IG(FRER_TRACE_GRP_BASE, "%u: stream_util_counters_get(%u)", frer_state->inst, frer_state->conf.stream_ids[0]);
            if ((rc = stream_util_counters_get(frer_state->conf.stream_ids[0], ingress_counters)) != VTSS_RC_OK) {
                // This typically happens when the stream doesn't exist.
                T_IG(FRER_TRACE_GRP_BASE, "%u: stream_util_counters_get(%u) failed: %s", frer_state->inst, frer_state->conf.stream_ids[0], error_txt(rc));
                return VTSS_RC_OK;
            }
        }

        // The rx_<color> counters always count, whereas rx_match only count if
        // a PSFP filter is enabled (on LAN966x).
        statistics->gen_matches = ingress_counters.rx_green.frames + ingress_counters.rx_yellow.frames + ingress_counters.rx_red.frames;
        statistics->gen_resets  = frer_state->gen_resets;
        return VTSS_RC_OK;
    }

    if (stream_id == VTSS_APPL_STREAM_ID_NONE) {
        // Getting compound (cstream) counters, which are per egress port.
        // Find the cstream ID.
        cstream_idx = 0;
        found = false;
        for (port_no_itr = 0; port_no_itr < FRER_BASE_cap_port_cnt; port_no_itr++) {
            if (!frer_state->conf.egress_ports.get(port_no_itr)) {
                continue;
            }

            if (port_no == port_no_itr) {
                found = true;
                break;
            }

            cstream_idx++;
        }

        if (!found) {
            // This should be checked by caller of us.
            T_EG(FRER_TRACE_GRP_BASE, "%u: Unable to find port_no = %u in list of egress ports", frer_state->inst, port_no);
            return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
        }

        if ((rc = mesa_frer_cstream_cnt_get(nullptr, frer_state->cstream_ids[cstream_idx], &ctrs)) != VTSS_RC_OK) {
            T_EG(FRER_TRACE_GRP_BASE, "%u: mesa_frer_cstream_cnt_get(%d, %u, %u) failed: %s", frer_state->inst, cstream_idx, port_no, frer_state->cstream_ids[cstream_idx], error_txt(rc));
            return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
        } else {
            T_IG(FRER_TRACE_GRP_BASE, "%u: mesa_frer_cstream_cnt_get(%d, %u, %u) succeeded",  frer_state->inst, cstream_idx, port_no, frer_state->cstream_ids[cstream_idx]);
        }
    } else {
        // Getting invididual (mstream) counters. Individual recovery is only
        // possible with a list of stream IDs, not a stream collection.
        mstream_idx = 0;
        found = false;
        for (mstream_idx = 0; mstream_idx < ARRSZ(frer_state->conf.stream_ids); mstream_idx++) {
            // Look for the correct index.
            if (stream_id == frer_state->conf.stream_ids[mstream_idx]) {
                found = true;
                break;
            }
        }

        if (!found) {
            // This should be checked by caller of us.
            T_EG(FRER_TRACE_GRP_BASE, "%u: Unable to find stream ID = %u in list of Stream IDs", frer_state->inst, stream_id);
            return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
        }

        if ((rc = mesa_frer_mstream_cnt_get(nullptr, frer_state->mstream_base_ids[mstream_idx], port_no, &ctrs)) != VTSS_RC_OK) {
            T_EG(FRER_TRACE_GRP_BASE, "%u: mesa_frer_mstream_cnt_get(%d, %u, %u) failed: %s", frer_state->inst, mstream_idx, port_no, frer_state->mstream_base_ids[mstream_idx], error_txt(rc));
            return VTSS_APPL_FRER_RC_INTERNAL_ERROR;
        } else {
            T_IG(FRER_TRACE_GRP_BASE, "%u: mesa_frer_mstream_cnt_get(%d, %u, %u) succeeded",  frer_state->inst, mstream_idx, port_no, frer_state->mstream_base_ids[mstream_idx]);
        }
    }

    // Transfer counters to our public counter structure.
    // Remaining counters in public structure are S/W-counted and already set.
    statistics->rcvy_out_of_order_packets = ctrs.out_of_order_packets;
    statistics->rcvy_rogue_packets        = ctrs.rogue_packets;
    statistics->rcvy_passed_packets       = ctrs.passed_packets;
    statistics->rcvy_discarded_packets    = ctrs.discarded_packets;
    statistics->rcvy_lost_packets         = ctrs.lost_packets;
    statistics->rcvy_tagless_packets      = ctrs.tagless_packets;
    statistics->rcvy_resets               = ctrs.resets;
    statistics->rcvy_latent_error_resets  = frer_state->led_state.led_resets;

    return VTSS_RC_OK;
}

/******************************************************************************/
// frer_base_statistics_clear()
// Caller only invokes us if the instance is operational active.
/******************************************************************************/
mesa_rc frer_base_statistics_clear(frer_state_t *frer_state)
{
    mesa_frer_mstream_id_t mstream_base_id;
    mesa_frer_cstream_id_t cstream_id;
    mesa_port_no_t         port_no;
    int                    mstream_idx, cstream_idx;

    if (frer_state->conf.mode == VTSS_APPL_FRER_MODE_GENERATION) {
        frer_state->gen_resets = 0;

        if (frer_state->using_stream_collection()) {
            (void)stream_collection_util_counters_clear(frer_state->conf.stream_collection_id);
        } else {
            // Only one stream in stream_ids[] in recovery mode. That's the
            // whole reason for having stream collections.
            (void)stream_util_counters_clear(frer_state->conf.stream_ids[0]);
        }

        return VTSS_RC_OK;
    }

    // In recovery mode, we have cstream and possibly also mstream counters that
    // we need to clear. Even though mstream counters aren't used unless we are
    // in individual recovery mode, we clear them anyway.

    // Clear the number of times the LatentErrorReset() function has been
    // invoked.
    frer_state->led_state.led_resets = 0;

    // First mstream counters.
    for (mstream_idx = 0; mstream_idx < ARRSZ(frer_state->mstream_base_ids); mstream_idx++) {
        mstream_base_id = frer_state->mstream_base_ids[mstream_idx];

        if (mstream_base_id == FRER_MSTREAM_BASE_ID_NONE) {
            // No more mstreams to clear.
            break;
        }

        for (port_no = 0; port_no < FRER_BASE_cap_port_cnt; port_no++) {
            if (!frer_state->conf.egress_ports.get(port_no)) {
                continue;
            }

            VTSS_RC(FRER_BASE_mstream_cnt_clear(frer_state, mstream_base_id, port_no, mstream_idx, __FUNCTION__, __LINE__));
        }
    }

    // The cstream counters
    for (cstream_idx = 0; cstream_idx < ARRSZ(frer_state->cstream_ids); cstream_idx++) {
        cstream_id = frer_state->cstream_ids[cstream_idx];

        if (cstream_id == FRER_CSTREAM_ID_NONE) {
            // No more cstreams to clear
            break;
        }

        VTSS_RC(FRER_BASE_cstream_cnt_clear(frer_state, cstream_id, cstream_idx, __FUNCTION__, __LINE__));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// frer_rules_debug_dump()
// Dumps active rules.
/******************************************************************************/
void frer_rules_debug_dump(uint32_t session_id, frer_icli_pr_t pr)
{
    FRER_LOCK_SCOPE();

    FRER_BASE_general_dump(session_id, pr);
    FRER_BASE_stream_dump (session_id, pr);
    FRER_BASE_mstream_dump(session_id, pr);
    FRER_BASE_cstream_dump(session_id, pr);
    FRER_BASE_led_dump    (session_id, pr);
}

/******************************************************************************/
// frer_base_init()
// Invoked during ICFG_LOADING_PRE.
/******************************************************************************/
void frer_base_init(void)
{
    if (!vtss_appl_frer_supported()) {
        T_EG(FRER_TRACE_GRP_BASE, "Ouch");
        return;
    }

    FRER_BASE_cap_port_cnt    = fast_cap(MEBA_CAP_BOARD_PORT_COUNT);
    FRER_BASE_cap_cstream_cnt = fast_cap(MESA_CAP_L2_FRER_CSTREAM_CNT);
    FRER_BASE_cap_iflow_pop   = fast_cap(MESA_CAP_L2_FRER_IFLOW_POP);
}

