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

#ifndef _VTSS_MAIN_H_
#define _VTSS_MAIN_H_

#include <new>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

#include "vtss_os_wrapper.h"

#include "vtss_cpp_porting_macros.h"
#include "main_types.h"
#include "primitives.hxx"
#include "microchip/ethernet/switch/api.h"
#include "vtss_phy_api.h"
#include <vtss/basics/clear.hxx>
#include "critd_api.h"

#define VTSS_MEMCMP_ELEMENT(A, B, M) \
    { \
        int res = memcmp(&A.M, &B.M, sizeof(A.M)); \
        if (res != 0) return res; \
    }

#define VTSS_MEMCMP_ELEMENT_ARRAY(A, B, M) \
    { \
        int res = memcmp(A.M, B.M, sizeof(A.M)); \
        if (res != 0) return res; \
    }

#define VTSS_MEMCMP_ELEMENT_ARRAY_RECURSIVE(A, B, M) \
    for (int i = 0; i < vtss_array_size(A.M); ++i) { \
        int res = vtss_memcmp(A.M[i], B.M[i]); \
        if (res != 0) return res; \
    }

#define VTSS_MEMCMP_ELEMENT_RECURSIVE(A, B, M) \
    { \
        int res = vtss_memcmp(A.M, B.M); \
        if (res != 0) return res; \
    }

#define VTSS_MEMCMP_ELEMENT_INT(A, B, M) \
    { \
        if (A.M < B.M) return -1; \
        if (A.M > B.M) return 1;  \
    }

#define VTSS_MEMCMP_ELEMENT_CAP(A, B, M) \
    { \
        int res = memcmp(A.M.data(), B.M.data(), A.M.mem_size()); \
        if (res != 0) return res; \
    }

// Like memset, but works with NON-POD data
template<typename TYPE>
void vtss_clear(TYPE &type) {
    vtss::clear(type);
}

template<typename TYPE, int CNT>
void vtss_clear(TYPE (&type)[CNT]) {
    vtss::clear(type);
}

/* - Error codes ---------------------------------------------------- */
#define VTSS_UNSPECIFIED_ERROR VTSS_RC_ERROR
#define VTSS_INVALID_PARAMETER VTSS_RC_ERROR
#define VTSS_INCOMPLETE        VTSS_RC_INCOMPLETE  /* Operation incomplete */

#define VTSS_RC(expr) { mesa_rc __rc__ = (expr); if (__rc__ < VTSS_RC_OK) return __rc__; }

// Macro for printing return code errors (When the return code is not OK)
#define VTSS_RC_ERR_PRINT(expr) {mesa_rc __rc__ = (expr);  if (__rc__ != VTSS_RC_OK) T_E("%s", error_txt(__rc__));}

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#include <vtss_trace_api.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define ARRSZ(t) /*lint -e{574} */ (sizeof(t)/sizeof(t[0])) /* Suppress Lint Warning 574: Signed-unsigned mix with relational */

#undef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b) ((a) > (b) ? (b) : (a))

extern critd_t vtss_appl_api_crit;

/* Macros for stringification */
#define vtss_xstr(s) vtss_str(s)
#define vtss_str(s) #s

VTSS_BEGIN_HDR

/* Lock/unlock functions for get-modify-set API operations */
struct vtss_appl_api_lock
{
    vtss_appl_api_lock(const char *file, int line) {
        critd_enter(&vtss_appl_api_crit, file, line);
    }

    ~vtss_appl_api_lock() {
        critd_exit(&vtss_appl_api_crit, __FILE__, 0);
    }
};

#define VTSS_APPL_API_LOCK_SCOPE() vtss_appl_api_lock __appl_api_lock(__FILE__, __LINE__)

// Global recursive mutex Lock/unlock functions for application level mutex
void vtss_global_lock(const char *file, unsigned int line);
void vtss_global_unlock(const char *file, unsigned int line);

/* API lock/unlock macros */
#define VTSS_API_ENTER(...) { mesa_api_lock_t lock; lock.function = __FUNCTION__; lock.file = __FILE__; lock.line = __LINE__; mesa_callout_lock(&lock); }
#define VTSS_API_EXIT(...) { mesa_api_lock_t lock; lock.function = __FUNCTION__; lock.file = __FILE__; lock.line = __LINE__; mesa_callout_unlock(&lock); }

void mepa_callout_lock(const mepa_lock_t *const lock);
void mepa_callout_unlock(const mepa_lock_t *const lock);

/* ================================================================= *
 *  Bit field macros           
 * ================================================================= */

/* Bit field macros */
#define VTSS_BF_SIZE(n)      (((n)+7)/8)
#define VTSS_BF_GET(a, n)    (((a)[(n)/8] & (1<<((n)%8))) ? 1 : 0)
#define VTSS_BF_SET(a, n, v) { if (v) { a[(n)/8] |= (1U<<((n)%8)); } else { a[(n)/8] &= ~(1U<<((n)%8)); }}
#define VTSS_BF_CLR(a, n)    (memset(a, 0, VTSS_BF_SIZE(n)))

/* Port member bit field macros */
#define VTSS_PORT_BF_SIZE                VTSS_BF_SIZE(64)
#define VTSS_PORT_BF_GET(a, port_no)     VTSS_BF_GET(a, port_no - VTSS_PORT_NO_START)
#define VTSS_PORT_BF_SET(a, port_no, v)  VTSS_BF_SET(a, port_no - VTSS_PORT_NO_START, v)
#define VTSS_PORT_BF_CLR(a)                   VTSS_BF_CLR(a, 64)

#ifndef VTSS_BITOPS_DEFINED
#ifdef __ASSEMBLER__
#define VTSS_BIT(x)                   (1 << (x))
#define VTSS_BITMASK(x)               ((1 << (x)) - 1)
#else
#define VTSS_BIT(x)                   (1U << (x))
#define VTSS_BITMASK(x)               ((1U << (x)) - 1)
#endif
#define VTSS_EXTRACT_BITFIELD(x,o,w)  (((x) >> (o)) & VTSS_BITMASK(w))
#define VTSS_ENCODE_BITFIELD(x,o,w)   (((x) & VTSS_BITMASK(w)) << (o))
#define VTSS_ENCODE_BITMASK(o,w)      (VTSS_BITMASK(w) << (o))
#define VTSS_BITOPS_DEFINED
#endif /* VTSS_BITOPS_DEFINED */

/* ================================================================= *
 *  Error codes                
 * ================================================================= */

/* Error code interpretation */
const char *error_txt(mesa_rc);

/* ================================================================= *
 *  Initialization             
 * ================================================================= */

/* Flags for INIT_CMD_CONF_DEF */
#define INIT_CMD_PARM2_FLAGS_IP                0x00000001 /* If set, attempt to restore VLAN1 IP configuration */
#define INIT_CMD_PARM2_FLAGS_ME_PRIO           0x00000002 /* If set, restore Primary Switch Election Priority (topo) */
#define INIT_CMD_PARM2_FLAGS_SID               0x00000004 /* If set, restore USID mappings (topo) */
#define INIT_CMD_PARM2_FLAGS_NO_DEFAULT_CONFIG 0x00000008 /* If set, don't apply 'default-config' */

/* Initialize all modules */
mesa_rc init_modules(vtss_init_data_t *data);

/* ================================================================= *
 *  Other useful, yet fundamental, constants and types
 * ================================================================= */
void vtss_api_trace_update(void);

#ifndef VTSS_SW_OPTION_PHY
#define PHY_INST NULL
#endif

#define VTSS_MALLOC(_s_)                            VTSS_MALLOC_MODID (VTSS_ALLOC_MODULE_ID, _s_,      __FILE__, __LINE__)
#define VTSS_CALLOC(_n_, _s_)                       VTSS_CALLOC_MODID (VTSS_ALLOC_MODULE_ID, _n_, _s_, __FILE__, __LINE__)
#define VTSS_REALLOC(_p_, _s_)                      VTSS_REALLOC_MODID(VTSS_ALLOC_MODULE_ID, _p_, _s_, __FILE__, __LINE__)
#define VTSS_STRDUP(_s_)                            VTSS_STRDUP_MODID (VTSS_ALLOC_MODULE_ID, _s_,      __FILE__, __LINE__)
#define VTSS_MALLOC_CAST(X, _sz_)                   (X = ((__typeof__(X)) VTSS_MALLOC(_sz_)))
#define VTSS_CALLOC_CAST(X, _nm_, _sz_)             (X = ((__typeof__(X)) VTSS_CALLOC(_nm_, _sz_)))
#define VTSS_REALLOC_CAST(X, _ptr_, _sz_)           (X = ((__typeof__(X)) VTSS_REALLOC(_ptr_, _sz_)))

/* System File Paths */
#define VTSS_MAIN_SD_PATH     "/mnt/media"

#define VTSS_MAX_VERSION_STRING_SIZE 255

// The possible Zarlink Servo Modules
typedef enum {
    VTSS_ZARLINK_SERVO_NONE = 0,            /// No Zarlink Servo module
    VTSS_ZARLINK_SERVO_ZLS30380,            /// Using the zls30380 Servo module
    VTSS_ZARLINK_SERVO_ZLS30387             /// Using the zls30387 Servo module
} vtss_zarlink_servo_t;

// Application capabilities */
typedef enum {
    VTSS_APPL_CAP_DUMMY = 10000,         // Avoid clashing with MESA capabilities

    VTSS_APPL_CAP_ISID_CNT,              // Number of ISIDs
    VTSS_APPL_CAP_ISID_END,              // End of ISIDs
    VTSS_APPL_CAP_PORT_USER_CNT,         // Number of port module users

    VTSS_APPL_CAP_L2_PORT_CNT,           // Number of ports
    VTSS_APPL_CAP_L2_LLAG_CNT,           // Number of LLAGs
    VTSS_APPL_CAP_L2_GLAG_CNT,           // Number of GLAGs
    VTSS_APPL_CAP_L2_POAG_CNT,           // Number of ports and LLAGs and GLAGs
    VTSS_APPL_CAP_MSTI_CNT,              // Number of MSTI

    VTSS_APPL_CAP_PACKET_RX_PORT_CNT,    // Number of packet Rx statistics ports
    VTSS_APPL_CAP_PACKET_RX_PRIO_CNT,    // Number of packet Tx statistics priorities
    VTSS_APPL_CAP_PACKET_TX_PORT_CNT,    // Number of packet Tx statistics ports

    VTSS_APPL_CAP_VLAN_VID_CNT,          // VLAN ID space
    VTSS_APPL_CAP_VLAN_COUNTERS,         // VLAN counters supported
    VTSS_APPL_CAP_VLAN_FLOODING,         // Management of VLAN flooding supported
    VTSS_APPL_CAP_VLAN_USER_INT_CNT,     // Number of internal VLAN users

    VTSS_APPL_CAP_AFI_SINGLE_CNT,        // Number of simultaneous single-frame flows
    VTSS_APPL_CAP_AFI_MULTI_CNT,         // Number of simultaneous multi-frame flows
    VTSS_APPL_CAP_AFI_MULTI_LEN_MAX,     // Maximum number of frames in one multi-frame sequence

    VTSS_APPL_CAP_MSTP_PORT_CONF_CNT,    // Number of internal MSTP port config
    VTSS_APPL_CAP_MSTP_MSTI_CNT,         // Number of MSTP instances

    VTSS_APPL_CAP_QOS_PORT_QUEUE_CNT,
    VTSS_APPL_CAP_QOS_WRED_DPL_CNT,
    VTSS_APPL_CAP_QOS_WRED_GRP_CNT,

    VTSS_APPL_CAP_NAS_STATE_MACHINE_CNT, // Number of NAS state machines

    VTSS_APPL_CAP_ACL_ACE_CNT,           // Maximum number of ACEs

    VTSS_APPL_CAP_MAX_ACL_RULES_PR_PTP_CLOCK, // The maximum number of ACL rules per PTP clock

    VTSS_APPL_CAP_LLDP_REMOTE_ENTRY_CNT, // Number of LLDP neighbors
    VTSS_APPL_CAP_LLDPMED_POLICIES_CNT,  // Number of LLDPMED policies

    VTSS_APPL_CAP_IP_INTERFACE_CNT,      // Max number of L3 interfaces
    VTSS_APPL_CAP_IP_ROUTE_CNT,          // Max number of static configured IP routes

    VTSS_APPL_CAP_DHCP_HELPER_USER_CNT,

    VTSS_APPL_CAP_AGGR_MGMT_GLAG_END,

    VTSS_APPL_CAP_AGGR_MGMT_GROUPS,

    VTSS_APPL_CAP_LACP_MAX_PORTS_IN_AGGR,

    VTSS_APPL_CAP_VOICE_VLAN_LLDP_TELEPHONY_MAC_ENTRIES_CNT,

    // Number of ports plus one. This is becasue the station clock is treated as a port, and there can be only one.
    VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT,

    // Number of PTP clocks
    VTSS_APPL_CAP_PTP_CLOCK_CNT,

    // Number of ports, plus number of station clocks, plus number of ptp clocks
    VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_AND_PTP_CNT,

    // Number of SYNCE clock sources that can be nominated
    VTSS_APPL_CAP_SYNCE_NOMINATED_CNT,

    // Number of SYNCE clock selections 
    VTSS_APPL_CAP_SYNCE_SELECTED_CNT,

    // The packet RX MTU size capability
    VTSS_APPL_CAP_PACKET_RX_MTU,

    // Indicate if the applicaiton AFI module is working or not
    VTSS_APPL_CAP_AFI,

    // The Zarlink Servo variant (vtss_zarlink_servo_t)
    VTSS_APPL_CAP_ZARLINK_SERVO_TYPE,

    // Does this platform have a loop port meant for up-injections?
    VTSS_APPL_CAP_LOOP_PORT_UP_INJ,

    // Number of ports on board plus the number of virtual ports that can be used for PTP
    VTSS_APPL_CAP_PORT_CNT_PTP_PHYS_AND_VIRT,
} vtss_appl_cap_t;

VTSS_END_HDR

// Allocate and construct (with placement new operator) a class/struct of TYPE.
// Returns a pointer to the constructed object.
template<typename TYPE, typename... Args>
TYPE *vtss_create(int modid, const char *f, int l, Args &&... args) {
    // We have a ctidy checker that looks for places where sizeof(x) is called
    // on objects/types that have a non-trivial destructor. Normally sizeof is
    // used "just" before we are doing something that does not work with data
    // with non-trivial destructors (like sending it using the message module).
    // But here it is needed and save to do.
    void *buf = VTSS_CALLOC_MODID(modid, 1,  sizeof(TYPE), f, l);  // NOLINT

    if (!buf) {
        return nullptr;
    }

    // Construct non-PoD data
    return new (buf) TYPE(vtss::forward<Args>(args)...);
}

#define VTSS_CREATE(TYPE, ...) \
    vtss_create<TYPE>(VTSS_ALLOC_MODULE_ID, __FILE__, __LINE__, ##__VA_ARGS__)

template<typename TYPE>
void vtss_destroy(TYPE *type) {
    type->~TYPE();
    VTSS_FREE(type);
}

template<typename TYPE, size_t SIZE>
constexpr size_t vtss_array_size(const TYPE (&)[SIZE]) {
    return SIZE;
}

uint32_t vtss_appl_port_cnt(mesa_inst_t inst);
uint32_t vtss_appl_capability(const void *_inst_unused_, int cap);

#endif /* _VTSS_MAIN_H_ */

