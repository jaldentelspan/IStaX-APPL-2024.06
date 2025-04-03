/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_APPL_MEBA_H_
#define _VTSS_APPL_MEBA_H_

#include <vtss/basics/type_traits.hxx>
#include <microchip/ethernet/board/api.h>

/**
 * \brief Retreive the board instance capabilities..
 *
 * \param inst [IN] The board instance.
 * \param cap  [IN] The capability to query for.
 *
 * \return the capability value.
 **/
static inline uint32_t meba_capability(meba_inst_t inst, int cap)
{
    return inst->api.meba_capability(inst, cap);
}

#define MEBA_WRAP(name)                                                 \
    template <typename ...Args>                                         \
    mesa_rc name(meba_inst_t inst, Args&&... args) {                    \
        if (inst->api.name) {                                           \
            mesa_rc rc;                                                 \
            inst->iface.debug(MEBA_TRACE_LVL_RACKET, __FUNCTION__, __LINE__, "Calling %s", #name); \
            rc = inst->api.name(inst, vtss::forward<Args>(args)...);    \
            inst->iface.debug(MEBA_TRACE_LVL_NOISE, __FUNCTION__, __LINE__, "%s returns %d", #name, rc); \
            return rc;                                                  \
        } else {                                                        \
            return MESA_RC_NOT_IMPLEMENTED;                             \
        }                                                               \
    }

#define MEBA_WRAP_SYNCE(name)                                           \
    template <typename ...Args>                                         \
    mesa_rc name(meba_inst_t inst, Args&&... args) {                    \
        if (inst->api_synce && inst->api_synce->name) {                 \
            mesa_rc rc;                                                 \
            inst->iface.debug(MEBA_TRACE_LVL_RACKET, __FUNCTION__, __LINE__, "Calling %s", #name); \
            rc = inst->api_synce->name(inst, vtss::forward<Args>(args)...);    \
            inst->iface.debug(MEBA_TRACE_LVL_NOISE, __FUNCTION__, __LINE__, "%s returns %d", #name, rc); \
            return rc;                                                  \
        } else {                                                        \
            return MESA_RC_NOT_IMPLEMENTED;                             \
        }                                                               \
    }

#define MEBA_WRAP_POE(name)                                           \
    template <typename ...Args>                                         \
    mesa_rc name(meba_inst_t inst, Args&&... args) {                    \
        if (inst->api_poe && inst->api_poe->name) {                 \
            mesa_rc rc;                                                 \
            inst->iface.debug(MEBA_TRACE_LVL_RACKET, __FUNCTION__, __LINE__, "Calling %s", #name); \
            rc = inst->api_poe->name(inst, vtss::forward<Args>(args)...);    \
            inst->iface.debug(MEBA_TRACE_LVL_NOISE, __FUNCTION__, __LINE__, "%s returns %d", #name, rc); \
            return rc;                                                  \
        } else {                                                        \
            return MESA_RC_NOT_IMPLEMENTED;                             \
        }                                                               \
    }

#define MEBA_WRAP_TOD(name)                                           \
    template <typename ...Args>                                         \
    mesa_rc name(meba_inst_t inst, Args&&... args) {                    \
        if (inst->api_tod && inst->api_tod->name) {                 \
            mesa_rc rc;                                                 \
            inst->iface.debug(MEBA_TRACE_LVL_RACKET, __FUNCTION__, __LINE__, "Calling %s", #name); \
            rc = inst->api_tod->name(inst, vtss::forward<Args>(args)...);    \
            inst->iface.debug(MEBA_TRACE_LVL_NOISE, __FUNCTION__, __LINE__, "%s returns %d", #name, rc); \
            return rc;                                                  \
        } else {                                                        \
            return MESA_RC_NOT_IMPLEMENTED;                             \
        }                                                               \
    }

#define X(name) MEBA_WRAP(name)
    MEBA_LIST_OF_API_CALLS
#undef X

#define X(name) MEBA_WRAP_SYNCE(name)
    MEBA_LIST_OF_API_SYNCE_CALLS
#undef X

#define X(name) MEBA_WRAP_POE(name)
    MEBA_LIST_OF_API_POE_CALLS
#undef X

#define X(name) MEBA_WRAP_TOD(name)
    MEBA_LIST_OF_API_TOD_CALLS
#undef X

#endif  // _VTSS_APPL_MEBA_H_
