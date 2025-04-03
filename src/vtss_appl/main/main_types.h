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

#ifndef _VTSS_MAIN_TYPES_H_
#define _VTSS_MAIN_TYPES_H_

#include <errno.h>
#include <sys/time.h>
#include <microchip/ethernet/common.h>      /* For mesa_port_no_t */
#include "caparray.hxx"
#include "board_if.h" /* For vtss_board_type_t */

#define VPRIlu "%lu"
#define VPRIld "%ld"
#define VPRIlx "%lx"
#define VPRIFlx(format) "%" format "lx"
#define VPRIlX "%lX"
#define VPRIFld(format) "%" format "ld"
#if __INTPTR_MAX__ == __INT32_MAX__
#    define VPRIz   "%u"   /* to print size_t. */
#    define VPRIsz  "%d"   /* to print ssize_t. */

#    define VPRI64u "%llu"
#    define VPRI64x "%llx"
#    define VPRI64d "%lld"
#    define VPRI64Fu(format) "%" format "llu"
#    define VPRI64Fx(format) "%" format "llx"
#    define VPRI64Fd(format) "%" format "lld"

#elif __INTPTR_MAX__ == __INT64_MAX__
#    define VPRIz   "%lu"  /* to print size_t. */
#    define VPRIsz  "%ld"  /* to print ssize_t. */

#    define VPRI64u "%lu"
#    define VPRI64x "%lx"
#    define VPRI64d "%ld"
#    define VPRI64Fu(format) "%" format "lu"
#    define VPRI64Fx(format) "%" format "lx"
#    define VPRI64Fd(format) "%" format "ld"
#else
#    error "Environment not 32 or 64-bit."
#endif

namespace vtss {
struct ostream;
} /* vtss  */ 

// TODO, this need to be cleanup when the API split is done.
#ifndef _VTSS_API_TYPES_H_
typedef int8_t             i8;   /**<  8-bit signed */
typedef int16_t            i16;  /**< 16-bit signed */
typedef int32_t            i32;  /**< 32-bit signed */
typedef int64_t            i64;  /**< 64-bit signed */
typedef uint8_t            u8;   /**<  8-bit unsigned */
typedef uint16_t           u16;  /**< 16-bit unsigned */
typedef uint32_t           u32;  /**< 32-bit unsigned */
typedef uint64_t           u64;  /**< 64-bit unsigned */
typedef uint8_t            BOOL; /**< Boolean implemented as 8-bit unsigned */
#endif  // _VTSS_API_TYPES_H_

/* - Integer types -------------------------------------------------- */
typedef unsigned int        uint;
typedef unsigned short      ushort;
typedef unsigned char       uchar;
typedef signed char         schar;
typedef long long           longlong;
typedef unsigned long long  ulonglong;

/* ================================================================= *
 *  User Switch IDs (USIDs), used by management modules (CLI, Web)
 * ================================================================= */
#define VTSS_USID_START  1
#define VTSS_USID_CNT    VTSS_ISID_CNT
#define VTSS_USID_END    (VTSS_USID_START + VTSS_USID_CNT)
#define VTSS_USID_LOCAL  0xff /* Special value for selecting all switches. Only valid in selected contexts! */
#define VTSS_USID_ALL    0  /* Special value for selecting all switches. Only valid in selected contexts! */
#define VTSS_USID_LEGAL(usid) (usid >= VTSS_USID_START && usid < VTSS_USID_END)

typedef uint vtss_usid_t;

/* ================================================================= *
 *  Internal Switch IDs (ISIDs)
 * ================================================================= */
#define VTSS_ISID_START   1
#define VTSS_ISID_CNT     1
#define VTSS_ISID_END     (VTSS_ISID_START + VTSS_ISID_CNT)
#define VTSS_ISID_LOCAL   0             /* Special value for local switch. Only valid in selected contexts! */
#define VTSS_ISID_UNKNOWN 0xff          /* Special value only used in selected contexts!                    */
#define VTSS_ISID_GLOBAL VTSS_ISID_END /* INIT_CMD_CONF_DEF: Reset global parameters */

#define VTSS_ISID_LEGAL(isid) (isid >= VTSS_ISID_START && isid < VTSS_ISID_END)

typedef uint vtss_isid_t;

/* ================================================================= *
 *  Unique IDs
 * ================================================================= */
// This is the interval between each switch.
#define SWITCH_INTERVAL 1000

// Macro for getting a unique port ID based on the switch isid (internal switch id), and the iport (internal port number) for each port in a stack.
// When the unique port ID is generates, the isid and iport are converted to usid (user switch number) and uport (user port number).
// Example: usid == 2 and uport == 3 gives a unique port id of 1003 (not 2003).
// This is inline with the way that SNMP assigns ifIndex based on switch ID and port numbers.
#define GET_UNIQUE_PORT_ID(isid, iport) (topo_isid2usid(isid) - VTSS_USID_START) * SWITCH_INTERVAL + iport2uport(iport)

/* Init command structure */
/* ================================================================= *
 *  Initialization             
 * ================================================================= */

/* Init command */
typedef enum {
    INIT_CMD_EARLY_INIT,        /* Called right after application start. Mostly used to initialize trace */
    INIT_CMD_INIT,              /* Initialize module. Called before scheduler is started. */
    INIT_CMD_START,             /* Start module. Called after scheduler is started. */
    INIT_CMD_CONF_DEF,          /* Create and activate factory defaults. 
                                   The 'flags' field is used for special exceptions.
                                   When creating factory defaults, each module may
                                   be called multiple times with different parameters.
                                   The 'isid' field may take one of the following values:
                                   VTSS_ISID_LOCAL : Create defaults in local section.
                                   VTSS_ISID_GLOBAL: Create global defaults in global section.
                                   Specific isid   : Create switch defaults in global section. */
    INIT_CMD_ICFG_LOADING_PRE,  /* ICFG will soon read startup-config and apply it to the individual modules (so it's time to default the configuration) */
    INIT_CMD_OBSOLETE,          /* OBSOLETE. Cannot be generated */
    INIT_CMD_ICFG_LOADING_POST, /* ICFG has now applied the startup-config. Time to apply the configuration to MESA. */
    INIT_CMD_ICFG_LOADED_POST,  /* All modules should now have applied their startup config to MESA, and can do other things. */
    INIT_CMD_OBSOLETE_2,        /* OBSOLETE. Cannot be generated */
    INIT_CMD_SUSPEND_RESUME,    /* Suspend/resume port module (resume valid) */
    INIT_CMD_WARMSTART_QUERY,   /* Query if a module is ready for warm start (warmstart is output parameter) */
} init_cmd_t;

vtss::ostream& operator<<(vtss::ostream& o, init_cmd_t c);

typedef struct {
    BOOL              configurable;   // TRUE if switch has been seen before
    u32               port_cnt;       // Port count of switch
    mesa_port_no_t    stack_ports[2]; // The stack port numbers of the switch
    vtss_board_type_t board_type;     // Board type enumeration, which uniquely identifies the  board (SFP, Cu. 24-ported, 48-ported, etc.).
    unsigned int      api_inst_id;    // The ID used to instantiate the API of the switch.
} init_switch_info_t;

typedef struct {
    i32            chip_port;         // Set to -1 if not used
    mesa_chip_no_t chip_no;           // Chip number for multi-chip targets.
} init_port_map_t;

typedef struct {
    init_cmd_t     cmd;            /* Command */
    vtss_isid_t    isid;           /* CONF_DEF/ICFG_LOADING_POST */
    u32            flags;          /* CONF_DEF */
    u8             resume;         /* SUSPEND_RESUME */
    u8             warmstart;      /* WARMSTART_QUERY - Module must set warmstart to FALSE if it is not ready for warmstart yet */

    // The following structure is valid as follows:
    // In ICFG_LOADING_PRE event:   Entries with switch_info[]::configurable == TRUE contain valid info
    //                              in the remaining members.
    // In ICFG_LOADING_POST events: Only index given by #isid is guaranteed to contain valid info.
    // In INIT_CONF_DEF events:     Only valid for legal isids [VTSS_ISID_START; VTSS_ISID_END[, and only the #.configurable
    //                              member is valid. #.configurable has the following semantics:
    //                              If FALSE, the switch has been deleted through SPROUT. This means: Forget everything about it.
    //                              If TRUE, it doesn't necessarily mean that the switch has been seen before.
    init_switch_info_t switch_info[VTSS_ISID_END]; // isid-indexed. VTSS_ISID_LOCAL is unused.
} vtss_init_data_t;

/**
 * \brief Type used for passing contexts that may either be a pointer or an integer
 */
    typedef union  {
        void *ptr;
        u32 u;
    } ctx_t;

#define NULL_CTX ({ NULL })
#ifdef __cplusplus
extern "C" {
#endif
const char *control_init_cmd2str(init_cmd_t cmd);
#ifdef __cplusplus
}
#endif


#ifndef _VTSS_API_TYPES_H_

typedef i64 vtss_timeinterval_t;

/** \brief Max/min values for 64 signed integer */
#define VTSS_I64_MAX  0x7FFFFFFFFFFFFFFFLL  /**<  Max value for 64 bit signed integer */
#define VTSS_I64_MIN -0x8000000000000000LL  /**<  Min value for 64 bit signed integer */

#if __INTPTR_MAX__ == __INT32_MAX__
#    if !defined(PRIu64)
#        define PRIu64 "llu"           /**< Fallback un-signed 64-bit formatting string */
#    endif

#    if !defined(PRIi64)
#        define PRIi64 "lli"           /**< Fallback signed 64-bit formatting string */
#    endif

#    if !defined(PRIx64)
#        define PRIx64 "llx"           /**< Fallback hex 64-bit formatting string */
#    endif

#elif __INTPTR_MAX__ == __INT64_MAX__
#    if !defined(PRIu64)
#        define PRIu64 "lu"           /**< Fallback un-signed 64-bit formatting string */
#    endif

#    if !defined(PRIi64)
#        define PRIi64 "li"           /**< Fallback signed 64-bit formatting string */
#    endif

#    if !defined(PRIx64)
#        define PRIx64 "lx"           /**< Fallback hex 64-bit formatting string */
#    endif
#else
#    error "Environment not 32 or 64-bit."
#endif

#define VTSS_BIT64(x)                  (1ULL << (x))                           /**< Set one bit in a 64-bit mask               */
#define VTSS_BITMASK64(x)              ((1ULL << (x)) - 1)                     /**< Get a bitmask consisting of x ones         */
#define VTSS_EXTRACT_BITFIELD64(x,o,w) (((x) >> (o)) & VTSS_BITMASK64(w))      /**< Extract w bits from bit position o in x    */
#define VTSS_ENCODE_BITFIELD64(x,o,w)  (((u64)(x) & VTSS_BITMASK64(w)) << (o)) /**< Place w bits of x at bit position o        */
#define VTSS_ENCODE_BITMASK64(o,w)     (VTSS_BITMASK64(w) << (o))              /**< Create a bitmask of w bits positioned at o */

#if !defined(TRUE)
#define TRUE  1 /**< True boolean value */
#endif
#if !defined(FALSE)
#define FALSE 0 /**< False boolean value */
#endif

typedef BOOL vtss_event_t;
typedef u32 vtss_isdx_t;   /**< Ingress Service Index type */

#define VTSS_PACKET_RATE_DISABLED     MESA_PACKET_RATE_DISABLED
#define VTSS_PORT_NO_NONE             MESA_PORT_NO_NONE
#define VTSS_PORT_NO_CPU              MESA_PORT_NO_CPU
#define VTSS_PORT_NO_START            (0)
#define VTSS_PRIOS                    8
#define VTSS_PRIO_NO_NONE             0xffffffff
#define VTSS_PRIO_START               0
#define VTSS_PRIO_END                 (VTSS_PRIO_START + VTSS_PRIOS)
#define VTSS_PRIO_ARRAY_SIZE          VTSS_PRIO_END
#define VTSS_QUEUES                   VTSS_PRIOS
#define VTSS_QUEUE_START              0
#define VTSS_QUEUE_END                (VTSS_QUEUE_START + VTSS_QUEUES)
#define VTSS_QUEUE_ARRAY_SIZE         VTSS_QUEUE_END
#define VTSS_PCPS                     MESA_PCP_CNT
#define VTSS_PCP_START                0
#define VTSS_PCP_END                  MESA_PCP_ARRAY_SIZE
#define VTSS_PCP_ARRAY_SIZE           MESA_PCP_ARRAY_SIZE
#define VTSS_DEIS                     2
#define VTSS_DEI_START                0
#define VTSS_DEI_END                  (VTSS_DEI_START + VTSS_DEIS)
#define VTSS_DEI_ARRAY_SIZE           VTSS_DEI_END
#define VTSS_BITRATE_DISABLED         MESA_BITRATE_DISABLED
#define VTSS_QOS_MAP_ID_NONE          MESA_QOS_MAP_ID_NONE
#define VTSS_VID_NULL                 MESA_VID_NULL
#define VTSS_VID_DEFAULT              MESA_VID_DEFAULT
#define VTSS_VID_RESERVED             MESA_VID_RESERVED
#define VTSS_VIDS                     MESA_VIDS
#define VTSS_VID_ALL                  MESA_VID_ALL
#define VTSS_MAC_ADDR_SZ_BYTES        MESA_MAC_ADDR_SZ_BYTES
#define VTSS_ISDX_NONE                MESA_ISDX_NONE
#define VTSS_VSI_NONE                 MESA_VSI_NONE
#define VTSS_AGGR_NO_NONE             0xffffffff
#define VTSS_AGGR_NO_START            0
#define VTSS_GLAGS                    32
#define VTSS_GLAG_NO_NONE             0xffffffff
#define VTSS_GLAG_NO_START            0
#define VTSS_GLAG_NO_END              (VTSS_GLAG_NO_START+VTSS_GLAGS)
#define VTSS_GLAG_PORTS               8
#define VTSS_GLAG_PORT_START          0
#define VTSS_GLAG_PORT_END            (VTSS_GLAG_PORT_START+VTSS_GLAG_PORTS)
#define VTSS_GLAG_PORT_ARRAY_SIZE     VTSS_GLAG_PORT_END
#define VTSS_PACKET_RX_QUEUE_CNT      8
#define VTSS_PACKET_RX_GRP_CNT        2
#define VTSS_PACKET_TX_GRP_CNT        2
#define VTSS_PACKET_RX_QUEUE_NONE     (0xffffffff)
#define VTSS_PACKET_RX_QUEUE_START    (0)
#define VTSS_PACKET_RX_QUEUE_END      (VTSS_PACKET_RX_QUEUE_START + VTSS_PACKET_RX_QUEUE_CNT)
#define VTSS_COSIDS                   8
#define VTSS_ONE_MIA                  MESA_ONE_MIA
#define VTSS_ONE_MILL                 MESA_ONE_MILL
#define VTSS_MAX_TIMEINTERVAL         MESA_MAX_TIMEINTERVAL
#define VTSS_INTERVAL_SEC(t)          ((i32)(((t) >> 16) / VTSS_ONE_MIA))
#define VTSS_INTERVAL_MS(t)           ((i32)(((t) >> 16) / VTSS_ONE_MILL))
#define VTSS_INTERVAL_US(t)           ((i32)(((t) >> 16) / 1000))
#define VTSS_INTERVAL_NS(t)           ((i32)(((t) >> 16) % (VTSS_ONE_MIA)))
#define VTSS_INTERVAL_PS(t)           (((((i32)(t & 0xffff)) * 1000) + 0x8000) / 0x10000)
#define VTSS_SEC_NS_INTERVAL(s,n)     (((vtss_timeinterval_t)(n) + (vtss_timeinterval_t)(s) * VTSS_ONE_MIA) << 16)
#define VTSS_CLOCK_IDENTITY_LENGTH    MESA_CLOCK_IDENTITY_LENGTH

#endif // _VTSS_API_TYPES_H_


#ifndef _VTSS_OS_LINUX_H_
#define VTSS_NSLEEP(nsec) {                                     \
    struct timespec ts;                                         \
    ts.tv_sec = 0;                                              \
    ts.tv_nsec = nsec;                                          \
    while(nanosleep(&ts, &ts) == -1 && errno == EINTR) {        \
    }                                                           \
}

/** Sleep for \param msec milliseconds */
#define VTSS_MSLEEP(msec) {                                     \
    struct timespec ts;                                         \
    ts.tv_sec = (msec) / 1000;                                  \
    ts.tv_nsec = ((msec) % 1000) * 1000000;                     \
    while(nanosleep(&ts, &ts) == -1 && errno == EINTR) {        \
    }                                                           \
}

typedef struct {
    struct timeval timeout;   /**< Timeout */
    struct timeval now;       /**< Time right now */
} vtss_mtimer_t;

#define VTSS_MTIMER_START(timer,msec) { \
    (void) gettimeofday(&((timer)->timeout),NULL);   \
    (timer)->timeout.tv_usec+=msec*1000; \
    if ((timer)->timeout.tv_usec>=1000000) { (timer)->timeout.tv_sec+=(timer)->timeout.tv_usec/1000000; (timer)->timeout.tv_usec%=1000000; } \
} /**< Start timer */

#define VTSS_MTIMER_TIMEOUT(timer) (gettimeofday(&((timer)->now),NULL)==0 && timercmp(&((timer)->now),&((timer)->timeout),>)) /**< Timer timeout */

#define VTSS_OS_CTZ(val32) ((val32) == 0 ? 32 : __builtin_ctzl((unsigned long)val32))
#define VTSS_OS_CLZ(val32) __builtin_clz(val32)

#endif // _VTSS_OS_LINUX_H_

#ifndef _VTSS_MISC_API_H_
#define VTSS_CHIP_NO_ALL 0xffffffff
#endif

#define VTSS_PORTS_ ((int)fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT))

#define VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD 64

#endif /* _VTSS_MAIN_TYPES_H_ */
