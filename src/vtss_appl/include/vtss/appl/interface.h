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

#ifndef _VTSS_APPL_INTERFACE_H_
#define _VTSS_APPL_INTERFACE_H_

#include <microchip/ethernet/switch/api/types.h>
#include <vtss/appl/types.h>

#ifdef __cplusplus
#include <vtss/basics/stream.hxx>
#include <vtss/basics/print_fmt.hxx>
#endif

/**
 * \file
 *
 * \brief Public (network) interfaces indexes API
 *
 * \details This header file describes the functions and
 * associated types and defines.
 *
 */
#define VTSS_IFINDEX_NONE_                                0 /**< No interface */
#define VTSS_IFINDEX_START_                               1 /**< First valid interface index  */
#define VTSS_IFINDEX_VLAN_OFFSET_                         0 /**< Offset for VLAN interfaces */
#define VTSS_IFINDEX_UNIT_MULTP_                    1000000 /**< Extent of per-unit interfaces */
#define VTSS_IFINDEX_GLAG_OFFSET_                 100000000 /**< Offset for Global link aggregations */
#define VTSS_IFINDEX_REDBOX_NEIGHBOR_             200000000 /**< RedBox neighbor interface index */
#define VTSS_IFINDEX_EVC_OBSOLETE_OFFSET_         200000000 /**< Offset for EVC interfaces (obsolete) */
#define VTSS_IFINDEX_MPLS_LINK_OBSOLETE_OFFSET_   300000000 /**< Offset for MPLS-TP link interfaces (obsolete) */
#define VTSS_IFINDEX_MPLS_TUNNEL_OBSOLETE_OFFSET_ 400000000 /**< Offset for MPLS tunnel interfaces (obsolete) */
#define VTSS_IFINDEX_MPLS_PW_OBSOLETE_OFFSET_     500000000 /**< Offset for MPLS pseudo-wire interfaces (obsolete) */
#define VTSS_IFINDEX_MPLS_LSP_OBSOLETE_OFFSET_    600000000 /**< Offset for MPLS LSP interfaces (obsolete) */
#define VTSS_IFINDEX_FOREIGN_OFFSET_              700000000 /**< Offset for Foreign interfaces */
#define VTSS_IFINDEX_FRR_VLINK_OFFSET_            800000000 /**< Offset for Frr vlink interfaces */
#define VTSS_IFINDEX_CPU_OFFSET_                  900000000 /**< Offset for Cpu interfaces */
#define VTSS_IFINDEX_PORT_UNIT_OFFSET_                    0 /**< Offset for switch ports */
#define VTSS_IFINDEX_LLAG_UNIT_OFFSET_                 5000 /**< Offset for Local link aggregations */

#define VTSS_IFINDEX_CONVERT_OS_IF_NAME_FAILED_  (VTSS_APPL_VLAN_ID_MAX + 1 + 9000000) /**< Returned as a foreign sub-ifindex when unable to conver an OS name to a VTSS ifindex                   */
#define VTSS_IFINDEX_CONVERT_OS_IF_ID_FAILED_    (VTSS_APPL_VLAN_ID_MAX + 1 + 3000000) /**< Returned as a foreign sub-ifindex when unable to convert an OS ifindex to an OS name                   */
#define VTSS_IFINDEX_CONVERT_VLINK_FAILED_       (1 + 2000000)                         /**< Returned as a foreign sub-ifindex when unable to convert VLINK.xxx OS interface to a VLINK interface   */
#define VTSS_IFINDEX_CONVERT_VLAN_FAILED_        (1000000)                             /**< Returned as a foreign sub-ifindex when unable to convert vtss.ifh.xxx OS interface to a VLAN interface */

/** Base interface index type */
typedef struct {
    /** Internal data used to store the interface index. */
    uint32_t private_ifindex_data_do_not_use_directly;
} vtss_ifindex_t;

/** Print a ifindex using printf
 * \param X [IN] Interface index
 */
#define VTSS_IFINDEX_PRINTF_ARG(X) (X).private_ifindex_data_do_not_use_directly

#ifdef __cplusplus
#define VTSS_IMPL_INT_LIKE_CMP_OPERATOR(OP)                       \
    inline bool operator OP(vtss_ifindex_t a, vtss_ifindex_t b) { \
        return a.private_ifindex_data_do_not_use_directly OP      \
                b.private_ifindex_data_do_not_use_directly;       \
    }                                                             \
    inline bool operator OP(uint32_t a, vtss_ifindex_t b) {       \
        return a OP b.private_ifindex_data_do_not_use_directly;   \
    }                                                             \
    inline bool operator OP(vtss_ifindex_t a, uint32_t b) {       \
        return a.private_ifindex_data_do_not_use_directly OP b;   \
    }
VTSS_IMPL_INT_LIKE_CMP_OPERATOR(== )
VTSS_IMPL_INT_LIKE_CMP_OPERATOR(!= )
VTSS_IMPL_INT_LIKE_CMP_OPERATOR(< )
VTSS_IMPL_INT_LIKE_CMP_OPERATOR(> )
VTSS_IMPL_INT_LIKE_CMP_OPERATOR(<= )
VTSS_IMPL_INT_LIKE_CMP_OPERATOR(>= )
#undef VTSS_IMPL_INT_LIKE_CMP_OPERATOR

#define VTSS_IMPL_INT_LIKE_ARIT_OPERATOR(OP)                                \
    inline vtss_ifindex_t operator OP(vtss_ifindex_t a, vtss_ifindex_t b) { \
        return vtss_ifindex_t{                                              \
                a.private_ifindex_data_do_not_use_directly OP               \
                        b.private_ifindex_data_do_not_use_directly};        \
    }                                                                       \
    inline vtss_ifindex_t operator OP(int a, vtss_ifindex_t b) {            \
        return vtss_ifindex_t{                                              \
                a OP b.private_ifindex_data_do_not_use_directly};           \
    }                                                                       \
    inline vtss_ifindex_t operator OP(vtss_ifindex_t a, int b) {            \
        return vtss_ifindex_t{                                              \
                a.private_ifindex_data_do_not_use_directly OP b};           \
    }
VTSS_IMPL_INT_LIKE_ARIT_OPERATOR(+)
VTSS_IMPL_INT_LIKE_ARIT_OPERATOR(-)
vtss::ostream &operator<<(vtss::ostream &o, const vtss_ifindex_t &i);
#undef VTSS_IMPL_INT_LIKE_ARIT_OPERATOR

#define DECL(X) static constexpr vtss_ifindex_t X = {X ## _}
/** No interface */
DECL(VTSS_IFINDEX_NONE);

/** First valid interface index */
DECL(VTSS_IFINDEX_START);

/** Offset for VLAN interfaces */
DECL(VTSS_IFINDEX_VLAN_OFFSET);

/**  Extent of per-unit interfaces */
DECL(VTSS_IFINDEX_UNIT_MULTP);

/** Offset for Global link aggregations */
DECL(VTSS_IFINDEX_GLAG_OFFSET);

/** RedBox neighbor interface index */
DECL(VTSS_IFINDEX_REDBOX_NEIGHBOR);

/** Offset for switch ports */
DECL(VTSS_IFINDEX_PORT_UNIT_OFFSET);

/** Offset for Local link aggregations */
DECL(VTSS_IFINDEX_LLAG_UNIT_OFFSET);

/** Offset for Foreign interfaces */
DECL(VTSS_IFINDEX_FOREIGN_OFFSET);

/** Offset for Frr vlink interfaces */
DECL(VTSS_IFINDEX_FRR_VLINK_OFFSET);

/** Offset for Cpu interfaces */
DECL(VTSS_IFINDEX_CPU_OFFSET);

#undef DECL
#endif  // __cplusplus

/** Interface types
 */
typedef enum {
    VTSS_IFINDEX_TYPE_NONE,                 /**< No interface */
    VTSS_IFINDEX_TYPE_PORT,                 /**< Switch port type */
    VTSS_IFINDEX_TYPE_LLAG,                 /**< Local link aggregation type */
    VTSS_IFINDEX_TYPE_GLAG,                 /**< Global link aggregation type */
    VTSS_IFINDEX_TYPE_VLAN,                 /**< VLAN interface type */
    VTSS_IFINDEX_TYPE_REDBOX_NEIGHBOR,      /**< RedBox neighbor interface */
    VTSS_IFINDEX_TYPE_EVC_OBSOLETE,         /**< EVC interface type (obsolete) */
    VTSS_IFINDEX_TYPE_MPLS_LINK_OBSOLETE,   /**< MPLS link interface type (obsolete) */
    VTSS_IFINDEX_TYPE_MPLS_TUNNEL_OBSOLETE, /**< MPLS tunnel interface type (obsolete) */
    VTSS_IFINDEX_TYPE_MPLS_PW_OBSOLETE,     /**< MPLS pseudo-wire interface type (obsolete) */
    VTSS_IFINDEX_TYPE_MPLS_LSP_OBSOLETE,    /**< MPLS lsp interface type (obsolete) */
    VTSS_IFINDEX_TYPE_FOREIGN,              /**< Foreign interface type */
    VTSS_IFINDEX_TYPE_FRR_VLINK,            /**< Frr VLINK type */
    VTSS_IFINDEX_TYPE_CPU,                  /**< Cpu interface type */
    VTSS_IFINDEX_TYPE_ILLEGAL,              /**< Invalid ifindex type */

    // NOTE: When adding new interfaces in this schema please remember to update
    // the documentation in the vtss_appl/snmp/mibs/VTSS-TC.mib (and derived
    // TC's). Also, update the serialize(<any>, AsInterfaceIndex) functions in
    // vtss_appl/util/vtss_appl_formatting_tags.cxx
} vtss_ifindex_type_t;

/* Enumeration flags */
#define VTSS_IFINDEX_GETNEXT_PORTS \
    (1 << VTSS_IFINDEX_TYPE_PORT) /**< Enumerate ports */
#define VTSS_IFINDEX_GETNEXT_LLAGS \
    (1 << VTSS_IFINDEX_TYPE_LLAG) /**< Enumerate LLAGs */
#define VTSS_IFINDEX_GETNEXT_GLAGS \
    (1 << VTSS_IFINDEX_TYPE_GLAG) /**< Enumerate GLAGs */
#define VTSS_IFINDEX_GETNEXT_VLANS \
    (1 << VTSS_IFINDEX_TYPE_VLAN) /**< Enumerate VLANs */
#define VTSS_IFINDEX_GETNEXT_FOREIGNS \
    (1 << VTSS_IFINDEX_TYPE_FOREIGN) /**< Enumerate Foreigns */
#define VTSS_IFINDEX_GETNEXT_FRR_VLINK \
    (1 << VTSS_IFINDEX_TYPE_FRR_VLINK) /**< Enumerate FRR vlink */
#define VTSS_IFINDEX_GETNEXT_CPU \
    (1 << VTSS_IFINDEX_TYPE_CPU) /**< Enumerate CPU ports */
#define VTSS_IFINDEX_GETNEXT_ALL \
    0xFFFFFFFF /**< Enumerate for get-next on all interface types */

/** Interface represented by {type, isid, ordinal}
 */
typedef struct {
    vtss_ifindex_type_t iftype; /**< Interface type */
    vtss_isid_t isid;           /**< Isid (defined for port, LLAG) */
    vtss_usid_t usid;           /**< Usid (defined for port, LLAG) */
    uint32_t ordinal;                /**< Base interface index (zero-based) */
} vtss_ifindex_elm_t;

/** Formats the ifindex how to be added in the stream
 *
 * \param stream [OUT] Buffer where result will be added
 * \param fmt    [IN]  Format of the ifindex
 * \param *p     [IN]  Pointer to the interface
 *
 * \return The number of bytes added to stream
 */
size_t fmt(vtss::ostream &stream, const vtss::Fmt &fmt, const vtss_ifindex_t *p);

#ifdef __cplusplus
extern "C" {
#endif

/** Cast a ifindex to an integer
 *
 * \param X [IN] Interface index
 *
 * \return The integer representation of of X
 */
#define vtss_ifindex_cast_to_u32(X) \
    ((X).private_ifindex_data_do_not_use_directly)

/** Un-safe cast a uint32_t to an vtss_ifindex_t.
 *
 * \param i [IN] uint32_t to be cast
 *
 * \return The vtss_ifindex_t representation of of i
 */
vtss_ifindex_t vtss_ifindex_cast_from_u32_0(uint32_t i);

/** Cast a uint32_t to an vtss_ifindex_t.
 *
 * This cast expect that the resulting vtss_ifindex_t is of type 'a'. If this is
 * not the case print an error.
 *
 * \param i [IN] uint32_t to be cast
 * \param a [IN] Expected type 'a'
 *
 * \return The vtss_ifindex_t representation of of i
 */
vtss_ifindex_t vtss_ifindex_cast_from_u32(uint32_t i, vtss_ifindex_type_t a);

/** Cast a uint32_t to an vtss_ifindex_t.
 *
 * This cast expect that the resulting vtss_ifindex_t is of type 'a' or 'b'. If
 * this is not the case print an error.
 *
 * \param i [IN] uint32_t to be cast
 * \param a [IN] Expected type 'a'
 * \param b [IN] Expected type 'b'
 *
 * \return The vtss_ifindex_t representation of of i
 */
vtss_ifindex_t vtss_ifindex_cast_from_u32_2(uint32_t i, vtss_ifindex_type_t a,
                                            vtss_ifindex_type_t b);

/** Cast a uint32_t to an vtss_ifindex_t.
 *
 * This cast expect that the resulting vtss_ifindex_t is of type 'a', 'b' or
 * 'c'. If this is not the case print an error.
 *
 * \param i [IN] uint32_t to be cast
 * \param a [IN] Expected type 'a'
 * \param b [IN] Expected type 'b'
 * \param c [IN] Expected type 'c'
 *
 * \return The vtss_ifindex_t representation of of i
 */
vtss_ifindex_t vtss_ifindex_cast_from_u32_3(uint32_t i, vtss_ifindex_type_t a,
                                            vtss_ifindex_type_t b,
                                            vtss_ifindex_type_t c);

/** Cast a uint32_t to an vtss_ifindex_t.
 *
 * This cast expect that the resulting vtss_ifindex_t is of type 'a', 'b', 'c'
 * or 'd'. If this is not the case print an error.
 *
 * \param i [IN] uint32_t to be cast
 * \param a [IN] Expected type 'a'
 * \param b [IN] Expected type 'b'
 * \param c [IN] Expected type 'c'
 * \param d [IN] Expected type 'd'
 *
 * \return The vtss_ifindex_t representation of of i
 */
vtss_ifindex_t vtss_ifindex_cast_from_u32_4(uint32_t i, vtss_ifindex_type_t a,
                                            vtss_ifindex_type_t b,
                                            vtss_ifindex_type_t c,
                                            vtss_ifindex_type_t d);

/** Check if an interface is set to the NONE constant
 *
 * \param ifindex [IN] Interface index
 *
 * \return TRUE if the interface is a switch port
 */
mesa_bool_t vtss_ifindex_is_none(vtss_ifindex_t ifindex);

/** Check if an interface is a switch port
 *
 * \param ifindex [IN] Interface index
 *
 * \return TRUE if the interface is a switch port
 */
mesa_bool_t vtss_ifindex_is_port(vtss_ifindex_t ifindex);

/** Check if an interface is a local link aggregation
 *
 * \param ifindex [IN] Interface index
 *
 * \return TRUE if the interface is an LLAG
 */
mesa_bool_t vtss_ifindex_is_llag(vtss_ifindex_t ifindex);

/** Check if an interface is a global link aggregation
 *
 * \param ifindex [IN] Interface index
 *
 * \return TRUE if the interface is an GLAG
 */
mesa_bool_t vtss_ifindex_is_glag(vtss_ifindex_t ifindex);

/** Check if an interface is a VLAN interface
 *
 * \param ifindex [IN] Interface index
 *
 * \return TRUE if the interface is a VLAN interface
 */
mesa_bool_t vtss_ifindex_is_vlan(vtss_ifindex_t ifindex);

/** Check if an interface is set to the VTSS_IFINDEX_REDBOX_NEIGHBOR constant
 *
 * \param ifindex [IN] Interface index
 *
 * \return TRUE if the interface is RedBox-Neighbor
 */
mesa_bool_t vtss_ifindex_is_redbox_neighbor(vtss_ifindex_t ifindex);

/** Check if an interface is a Foreign interface
 *
 * \param ifindex [IN] Interface index
 *
 * \return TRUE if the interface is a Foreign interface
 */
mesa_bool_t vtss_ifindex_is_foreign(vtss_ifindex_t ifindex);

/** Check if an interface is a Frr vlink
 *
 * \param ifindex [IN] Interface index
 *
 * \return TRUE if the interface is a Frr vlink interface
 */
mesa_bool_t vtss_ifindex_is_frr_vlink(vtss_ifindex_t ifindex);

/** Check if an interface is a Cpu interface
 *
 * \param ifindex [IN] Interface index
 *
 * \return TRUE if the interface is a Cpu interface
 */
mesa_bool_t vtss_ifindex_is_cpu(vtss_ifindex_t ifindex);

/** Convert a switch port number to an interface index
 *
 * \param usid [IN] Switch unit (use 1 for non-stacked configurations)
 *
 * \param uport [IN] User view port number
 *
 * \param ifindex [OUT] Interface index
 *
 * \return VTSS_RC_OK if conversion occurred
 */
mesa_rc vtss_ifindex_from_usid_uport(vtss_usid_t usid, vtss_uport_no_t uport,
                                     vtss_ifindex_t *ifindex);


/** Convert a switch port number to an interface index
 *
 * \param isid [IN] Switch unit (ignored for non-stacked configurations)
 *
 * \param port_no [IN] Port number
 *
 * \param ifindex [OUT] Interface index
 *
 * \return VTSS_RC_OK if conversion occurred
 */
mesa_rc vtss_ifindex_from_port(vtss_isid_t isid, mesa_port_no_t port_no,
                               vtss_ifindex_t *ifindex);

/** Convert a local link aggregation to an interface index
 *
 * \param usid [IN] Switch unit (use 1 for non-stacked configurations)
 *
 * \param llag_no [IN] LLAG number
 *
 * \param ifindex [OUT] Interface index
 *
 * \return VTSS_RC_OK if conversion occurred
 */
mesa_rc vtss_ifindex_from_usid_llag(vtss_usid_t usid, uint32_t llag_no,
                                    vtss_ifindex_t *ifindex);

/** Convert a local link aggregation to an interface index
 *
 * \param isid [IN] Switch unit (ignored for non-stacked configurations)
 *
 * \param llag_no [IN] LLAG number
 *
 * \param ifindex [OUT] Interface index
 *
 * \return VTSS_RC_OK if conversion occurred
 */
mesa_rc vtss_ifindex_from_llag(vtss_isid_t isid, uint32_t llag_no,
                               vtss_ifindex_t *ifindex);

/** Convert a global link aggregation to an interface index
 *
 * \param glag_no [IN] GLAG number
 *
 * \param ifindex [OUT] Interface index
 *
 * \return VTSS_RC_OK if conversion occurred
 */
mesa_rc vtss_ifindex_from_glag(uint32_t glag_no, vtss_ifindex_t *ifindex);

/** Convert a VLAN id to an interface index
 *
 * \param vlan_no [IN] VLAN number
 *
 * \param ifindex [OUT] Interface index
 *
 * \return VTSS_RC_OK if conversion occurred
 */
mesa_rc vtss_ifindex_from_vlan(mesa_vid_t vlan_no, vtss_ifindex_t *ifindex);

/** Convert a Foreign id to an interface index
 *
 * \param foreign_no [IN] Foreign number
 *
 * \param ifindex [OUT] Interface index
 *
 * \return VTSS_RC_OK if conversion occurred
 */
mesa_rc vtss_ifindex_from_foreign(uint32_t foreign_no, vtss_ifindex_t *ifindex);

/** Convert a Frr vlink id to an interface index
 *
 * \param frr_vlink_no [IN] Frr vlink number
 *
 * \param ifindex [OUT] Interface index
 *
 * \return VTSS_RC_OK if conversion occured
 */
mesa_rc vtss_ifindex_from_frr_vlink(uint32_t frr_vlink_no, vtss_ifindex_t *ifindex);

/** Convert a Cpu id to an interface index
 *
 * \param cpu_no [IN] Cpu number
 *
 * \param ifindex [OUT] Interface index
 *
 * \return VTSS_RC_OK if conversion occurred
 */
mesa_rc vtss_ifindex_from_cpu(uint32_t cpu_no, vtss_ifindex_t *ifindex);

/**
 * Convert an os interface name (the name you get by doing a 'ip link' from a
 * shell) to an ifindex.
 * See also vtss_ifindex_from_os_ifindex()
 *
 * \param &os_ifname [IN] The name as shown by 'ip link'
 *
 * \return ifindex if things went smooth
 * ifindex == 709004096 - The failure due to unknown OS interface name, else if
 * ifindex >= 702000000 - The failure due to VLINK type conversion, else
 * ifindex >= 701000000 - The failure due to VLAN type conversion.
 */
vtss_ifindex_t vtss_ifindex_from_os_ifname(const std::string &os_ifname);

/**
 * Convert an os_ifindex (the number you get by doing a 'ip link' from a shell)
 * to an ifindex.
 * If os_ifindex is 0, the function will return an ifindex of type
 * VTSS_IFINDEX_TYPE_NONE.
 * If this application knows the interface (e.g. vtss.vlan.1), it will be
 * converted to the known type (e.g. VTSS_IFINDEX_TYPE_VLAN).
 * If this application doesn't know the interface, it will be converted to an
 * ifindex of type VTSS_IFINDEX_TYPE_FOREIGN with the os_ifindex as index.
 *
 * \param os_ifindex [IN] The index as shown by 'ip link'
 *
 * \return ifindex if things went smooth
 * ifindex == 709004096 - The failure due to unknown OS interface name, else if
 * ifindex >= 702000000 - The failure due to VLINK type conversion, else
 * ifindex >= 701000000 - The failure due to VLAN type conversion.
 */
vtss_ifindex_t vtss_ifindex_from_os_ifindex(unsigned int os_ifindex);

/** Return the interface type for a given an interface index
 *
 * \param ifindex [IN] Interface index
 *
 * \return VTSS_IFINDEX_TYPE_ILLEGAL for illegal input values, or the
 * approriate type for valid input values. See \e
 * vtss_ifindex_type_t.
 */
vtss_ifindex_type_t vtss_ifindex_type(vtss_ifindex_t ifindex);

/** Decompose a given interface index into discrete components.
 *
 * \param ifindex [IN] Interface index
 *
 * \param ife [OUT] Decomposed interface components: {type, isid, ordinal}
 *
 * \return VTSS_RC_OK if conversion occurred
 */
mesa_rc vtss_ifindex_decompose(vtss_ifindex_t ifindex, vtss_ifindex_elm_t *ife);

/** Convert an ifindex to a vlan id provided that the ifindex is of vlan type
 *
 * \param ifindex [IN] Interface index
 *
 * \return vlan id if ok, otherwise 0
 */
mesa_vid_t vtss_ifindex_as_vlan(vtss_ifindex_t ifindex);

/** Given an interface index return the next valid interface
 * (in numerical order).
 *
 * \param prev_ifindex [IN] Previous interface index. \e previous may be
 * given as a NULL, in which case the first interface of the desired
 * types will be returned.
 *
 * \param next_ifindex [OUT] the next valid interface numerically larger than
 *the input.
 *
 * \param check_exist [IN] Set to true, the next_ifindex will skip none exist
 *interface.
 *
 * \return VTSS_RC_OK if \em next has been assigned a value (i.e. a
 * next interface exist).
 */
mesa_rc vtss_ifindex_getnext(const vtss_ifindex_t *prev_ifindex,
                             vtss_ifindex_t *next_ifindex, mesa_bool_t check_exist);

/** Given an interface index return the next valid interface
 * (in numerical order).
 *
 * \param previous [IN] Previous interface index. \e previous may be
 * given as a NULL, in which case the first interface of the desired
 * types will be returned.
 *
 * \param next [OUT] the next valid interface numerically larger than the input.
 *
 * \param enumerate_types [IN] A bitmask of the interfaces to
 * enumerate. See VTSS_IFINDEX_GETNEXT_PORTS,
 * VTSS_IFINDEX_GETNEXT_LLAGS, VTSS_IFINDEX_GETNEXT_GLAGS and
 * VTSS_IFINDEX_GETNEXT_VLANS;
 *
 * \return VTSS_RC_OK if \em next has been assigned a value (i.e. a
 * next interface exist).
 */
mesa_rc vtss_ifindex_getnext_by_type(const vtss_ifindex_t *previous,
                                     vtss_ifindex_t *next, uint32_t enumerate_types);

/** Given an interface index return the next valid port interface index
 * (in numerical order).
 *
 * \param previous [IN] Previous interface index. \e previous may be
 * given as a NULL, in which case the first interface of the desired
 * types will be returned.
 *
 * \param next [OUT] the next valid interface numerically larger than the input.
 *
 * \return VTSS_RC_OK if \em next has been assigned a value
 */
mesa_rc vtss_ifindex_getnext_port(const vtss_ifindex_t *previous,
                                  vtss_ifindex_t *next);

/** Given an interface index return the next exist port interface index
 * (in numerical order).
 *
 * \param previous [IN] Previous interface index. \e previous may be
 * given as a NULL, in which case the first interface of the desired
 * types will be returned.
 *
 * \param next [OUT] the next valid interface numerically larger than the input.
 *
 * \return VTSS_RC_OK if \em next has been assigned a value
 */
mesa_rc vtss_ifindex_getnext_port_exist(const vtss_ifindex_t *previous,
                                        vtss_ifindex_t *next);


/** Given an interface index return the next configurable port interface index
 * (in numerical order).
 *
 * \param previous [IN] Previous interface index. \e previous may be
 * given as a NULL, in which case the first interface of the desired
 * types will be returned.
 *
 * \param next [OUT] the next valid interface numerically larger than the input.
 *
 * \return VTSS_RC_OK if \em next has been assigned a value
 */
mesa_rc vtss_ifindex_getnext_port_configurable(const vtss_ifindex_t *previous,
                                               vtss_ifindex_t *next);

/** Interface iterator function.
 *
 * \param prev_ifindex [IN] Previous interface index
 *
 * \param next_ifindex [OUT] next interface index
 *
 * \param enumerate_types [IN] Specify VTSS_IFINDEX_GETNEXT_xxx to fileter by
 *interface type when iterates interface index.
 *
 * \param check_exist [IN] TRUE - will verify the interface index is currently
 *present or not.
 *                         The interface index will be skipped if it is not
 *present in the system.
 *
 * \return VTSS_RC_OK if the next interface index is found.
 */
mesa_rc vtss_ifindex_iterator(const vtss_ifindex_t *prev_ifindex,
                              vtss_ifindex_t *next_ifindex, uint32_t enumerate_types,
                              mesa_bool_t check_exist);

/** Given an interface,queue index return the next valid port interface,queue
 *index
 * (in numerical order).
 *
 * \param prev_ifindex [IN] Previous interface index. \e previous may be
 * given as a NULL, in which case the first interface of the desired
 * types will be returned.
 *
 * \param next_ifindex [OUT] the next valid interface numerically larger than
 *the input.
 *
 * \param prev_queue [IN] Previous priority queue index.
 *
 * \param next_queue [OUT] the next valid priority queue numerically larger than
 *the input.
 *
 * \return VTSS_RC_OK if \em next has been assigned a value
 */
mesa_rc vtss_ifindex_getnext_port_queue(const vtss_ifindex_t *prev_ifindex,
                                        vtss_ifindex_t *next_ifindex,
                                        const mesa_prio_t *prev_queue,
                                        mesa_prio_t *next_queue);

/**

 * Convert an interface index to a printable string.
 *
 * \param buffer [OUT] The returned human readable representation of
 * the provided bridge identifier.
 *
 * \param size [IN] The size of the output buffer
 *
 * \param ifindex [IN] The interface index
 *
 * \return Returns the first argument, \e buffer,
 */
char *vtss_ifindex2str(char *buffer, size_t size, vtss_ifindex_t ifindex);

#ifdef VTSS_SW_OPTION_WEB
/**
 * Convert a string to an interface index.
 *
 * \param buffer [IN] The human readable representation of the index.
 * \param len    [IN] The length of the human readable string. If 0, use entire string.
 *
 * \param ifindex [OUT] The interface index
 *
 * \return Returns VTSS_RC_OK if the string successfully is translated to an interface index.
 */
mesa_rc vtss_str2ifindex(const char *buffer, int len, vtss_ifindex_t *ifindex);
#endif //VTSS_SW_OPTION_WEB

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace vtss {
inline vtss_ifindex_t ifindex_cast_from_u32(uint32_t i, vtss_ifindex_type_t a) { return vtss_ifindex_cast_from_u32(i, a); }
inline vtss_ifindex_t ifindex_cast_from_u32(uint32_t i, vtss_ifindex_type_t a, vtss_ifindex_type_t b) { return vtss_ifindex_cast_from_u32_2(i, a, b); }
inline vtss_ifindex_t ifindex_cast_from_u32(uint32_t i, vtss_ifindex_type_t a, vtss_ifindex_type_t b, vtss_ifindex_type_t c) { return vtss_ifindex_cast_from_u32_3(i, a, b, c); }
inline vtss_ifindex_t ifindex_cast_from_u32(uint32_t i, vtss_ifindex_type_t a, vtss_ifindex_type_t b, vtss_ifindex_type_t c, vtss_ifindex_type_t d) { return vtss_ifindex_cast_from_u32_4(i, a, b, c, d); }

inline mesa_rc ifindex_decompose(vtss_ifindex_t ifindex, vtss_ifindex_elm_t *ife) { return vtss_ifindex_decompose(ifindex, ife); }
inline vtss_ifindex_elm_t ifindex_decompose(vtss_ifindex_t ifindex) {
    vtss_ifindex_elm_t e;
    if (vtss::ifindex_decompose(ifindex, &e) != MESA_RC_OK)
        e.iftype = VTSS_IFINDEX_TYPE_ILLEGAL;
    return e;
}
inline mesa_rc ifindex_decompose(uint32_t ifindex, vtss_ifindex_elm_t *ife) { return vtss_ifindex_decompose(vtss_ifindex_t{ifindex}, ife); }
inline vtss_ifindex_elm_t ifindex_decompose(uint32_t ifindex) { return ifindex_decompose(vtss_ifindex_t{ifindex}); }


inline mesa_bool_t ifindex_is_none(vtss_ifindex_t ifindex) { return vtss_ifindex_is_none(ifindex); }
inline mesa_bool_t ifindex_is_port(vtss_ifindex_t ifindex) { return vtss_ifindex_is_port(ifindex); }
inline mesa_bool_t ifindex_is_llag(vtss_ifindex_t ifindex) { return vtss_ifindex_is_llag(ifindex); }
inline mesa_bool_t ifindex_is_glag(vtss_ifindex_t ifindex) { return vtss_ifindex_is_glag(ifindex); }
inline mesa_bool_t ifindex_is_vlan(vtss_ifindex_t ifindex) { return vtss_ifindex_is_vlan(ifindex); }
inline mesa_bool_t ifindex_is_redbox_neighbor(vtss_ifindex_t ifindex) { return vtss_ifindex_is_redbox_neighbor(ifindex); }
inline mesa_bool_t ifindex_is_foreign(vtss_ifindex_t ifindex) { return vtss_ifindex_is_foreign(ifindex); }
inline mesa_bool_t ifindex_is_frr_vlink(vtss_ifindex_t ifindex) { return vtss_ifindex_is_frr_vlink(ifindex); }
inline mesa_bool_t ifindex_is_cpu(vtss_ifindex_t ifindex) { return vtss_ifindex_is_cpu(ifindex); }

inline mesa_bool_t ifindex_is_none(uint32_t ifindex) { return vtss_ifindex_is_none(vtss_ifindex_t{ifindex}); }
inline mesa_bool_t ifindex_is_port(uint32_t ifindex) { return vtss_ifindex_is_port(vtss_ifindex_t{ifindex}); }
inline mesa_bool_t ifindex_is_llag(uint32_t ifindex) { return vtss_ifindex_is_llag(vtss_ifindex_t{ifindex}); }
inline mesa_bool_t ifindex_is_glag(uint32_t ifindex) { return vtss_ifindex_is_glag(vtss_ifindex_t{ifindex}); }
inline mesa_bool_t ifindex_is_vlan(uint32_t ifindex) { return vtss_ifindex_is_vlan(vtss_ifindex_t{ifindex}); }
inline mesa_bool_t ifindex_is_redbox_neighbor(uint32_t ifindex) { return vtss_ifindex_is_redbox_neighbor(vtss_ifindex_t{ifindex}); }
inline mesa_bool_t ifindex_is_foreign(uint32_t ifindex) { return vtss_ifindex_is_foreign(vtss_ifindex_t{ifindex}); }
inline mesa_bool_t ifindex_is_frr_vlink(uint32_t ifindex) { return vtss_ifindex_is_frr_vlink(vtss_ifindex_t{ifindex}); }
inline mesa_bool_t ifindex_is_cpu(uint32_t ifindex) { return vtss_ifindex_is_cpu(vtss_ifindex_t{ifindex}); }
}  // namespace vtss
#endif

#endif /* _VTSS_APPL_INTERFACE_H_ */
