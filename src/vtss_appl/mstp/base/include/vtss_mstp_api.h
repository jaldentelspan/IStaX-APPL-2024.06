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

#ifndef _VTSS_MSTP_API_H_
#define _VTSS_MSTP_API_H_

#include "vtss/appl/mstp.h"

/* Type compatibility sections */

typedef vtss_appl_mstp_msti_t                mstp_msti_t;
typedef vtss_appl_mstp_p2p_t                 mstp_p2p_t;
typedef vtss_appl_mstp_portstate_t           mstp_portstate_t;
typedef vtss_appl_mstp_forceversion_t        mstp_forceversion_t;
typedef vtss_appl_mstp_map_t                 mstp_map_t;
typedef vtss_appl_mstp_bridge_status_t       mstp_bridge_status_t;
typedef vtss_appl_mstp_bridge_param_t        mstp_bridge_param_t;
typedef vtss_appl_mstp_port_status_t         mstp_port_status_t;
typedef vtss_appl_mstp_bridge_vector_t       mstp_bridge_vector_t;
typedef vtss_appl_mstp_port_mgmt_status_t    mstp_port_mgmt_status_t;
typedef vtss_appl_mstp_msti_config_t         mstp_msti_config_t;
typedef vtss_appl_mstp_port_vectors_t        mstp_port_vectors_t;
typedef vtss_appl_mstp_port_statistics_t     mstp_port_statistics_t;
typedef vtss_appl_mstp_port_param_t          mstp_port_param_t;
typedef vtss_appl_mstp_msti_port_param_t     mstp_msti_port_param_t;

/* Define Compatibility section */
#define MSTI_CIST                       VTSS_APPL_MSTI_CIST
#define N_MSTI_MAX                      VTSS_APPL_MSTP_MAX_MSTI
#define P2P_FORCETRUE                   VTSS_APPL_MSTP_P2P_FORCETRUE
#define P2P_FORCEFALSE                  VTSS_APPL_MSTP_P2P_FORCEFALSE
#define P2P_AUTO                        VTSS_APPL_MSTP_P2P_AUTO
#define PORTSTATE_DISABLED              VTSS_APPL_MSTP_PORTSTATE_DISABLED
#define PORTSTATE_DISCARDING            VTSS_APPL_MSTP_PORTSTATE_DISCARDING
#define PORTSTATE_LEARNING              VTSS_APPL_MSTP_PORTSTATE_LEARNING
#define PORTSTATE_FORWARDING            VTSS_APPL_MSTP_PORTSTATE_FORWARDING
#define MSTP_PORT_PATHCOST_AUTO         VTSS_APPL_MSTP_PORT_PATHCOST_AUTO
#define MSTP_TIMESINCE_NEVER            VTSS_APPL_MSTP_TIMESINCE_NEVER
#define MSTP_PROTOCOL_VERSION_COMPAT    VTSS_APPL_MSTP_PROTOCOL_VERSION_COMPAT
#define MSTP_PROTOCOL_VERSION_RSTP      VTSS_APPL_MSTP_PROTOCOL_VERSION_RSTP
#define MSTP_PROTOCOL_VERSION_MSTP      VTSS_APPL_MSTP_PROTOCOL_VERSION_MSTP
#define MSTP_MAX_VID                    VTSS_APPL_MSTP_MAX_VID
#define MSTP_MIN_VID                    VTSS_APPL_MSTP_MIN_VID
#define MSTP_NULL_VID                   VTSS_APPL_MSTP_NULL_VID
#define MSTP_BRIDGEID_LEN               VTSS_APPL_MSTP_BRIDGEID_LEN
#define MSTP_CONFIG_NAME_MAXLEN         VTSS_APPL_MSTP_CONFIG_NAME_MAXLEN
#define MSTP_DIGEST_LEN                 VTSS_APPL_MSTP_DIGEST_LEN

/** Opaque MSTP handle */
typedef struct mstp_bridge mstp_bridge_t;

/** Basic MAC address type */
typedef struct {
    u8 mac[6];
} mstp_macaddr_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \file vtss_mstp_api.h
 * \brief MSTP main API header file
 *
 * This file contain the definitions of API functions
 *
 * \author Lars Povlsen <lpovlsen@vitesse.com>
 *
 */

/**
 * Create MSTP bridge
 *
 * \return (opaque) instance data reference or NULL.
 *
 * \param bridge_id A unique 48-bit Universally Administered MAC
 * Address, termed the Bridge Address. (7.12.5 Unique identification
 * of a bridge).
 *
 * \param n_ports (maximum) number of ports in bridge. This defined
 * the valid port numbers for this instance to lie in the <em>
 * [1; n_ports] </em> range (in accordance with 14.3 bullet j). Ports
 * are specifically added/deleted with vtss_mstp_add_port() and
 * vtss_mstp_delete_port() interfaces.
 */
mstp_bridge_t *_vtss_mstp_create_bridge(const mstp_macaddr_t *bridge_id, uint n_ports);

/**
 * Delete MSTP bridge.
 *
 * \param mstp The MSTP instance data.
 *
 */
mesa_rc _vtss_mstp_delete_bridge(mstp_bridge_t *mstp);

/**
 * 12.12.3 Set MST Configuration Table
 *
 * \param mstp The MSTP instance data.
 *
 * \param map A table mapping each of the 4096 VLANs to a
 * corresponding MSTI. (802.1Q - 8.9). Entry 0 is skipped. The
 * remainder of entries must map to a valid MSTI index, with the CIST
 * having the value \e '0'. Initially, all VID's map to the CIST.
 *
 * \return TRUE if the \e map contain valid mappings, FALSE
 * otherwise.
 *
 * \note The current implementation do not distinguish between FID's
 * and VID's, i.e. there is a 1:1 mapping between them.
 *
 * \note MSTI instances in the bridge will be created and deleted as a
 * side-effect of setting the mapping table.
 *
 * Likewise, active CIST ports will be probed for MSTI membership and
 * MSTI ports will be added respectively removed to synchronize
 * instantiated ports in the active MSTIs.
 */
mesa_rc _vtss_mstp_set_mapping(mstp_bridge_t *mstp, mstp_map_t *map);

/**
 * 12.12.3.4 Set MST Configuration Identifier Elements
 *
 * Purpose: To change the current values of the modifiable elements of
 * the MST Configuration Identifier for the Bridge (13.7).
 *
 * \param mstp The MSTP instance data.
 *
 * \param name The Configuration Name (max MSTP_CONFIG_NAME_MAXLEN
 * characters).
 *
 * \param revision The Revision Level [0; 65535]
 */
mesa_rc _vtss_mstp_set_config_id(mstp_bridge_t *mstp, const char *name, u16 revision);

/**
 * Get MST Configuration Identifier Elements
 *
 * Purpose: To retreive the current values of the modifiable elements
 * of the MST Configuration Identifier for the Bridge (13.7), as well
 * as the configuration digest.
 *
 * \param mstp The MSTP instance data.
 *
 * \param name The Configuration Name (size MSTP_CONFIG_NAME_MAXLEN
 * characters). The pointer may be NULL if the name is not desired.
 *
 * \param revision The Revision Level. The pointer may be NULL if the
 * revision is not desired.
 *
 * \param digest The current MSTI map calculated digest (size
 * MSTP_DIGEST_LEN bytes). The pointer may be NULL if the digest is
 * not desired.
 */
mesa_rc _vtss_mstp_get_config_id(mstp_bridge_t *mstp, char name[MSTP_CONFIG_NAME_MAXLEN], u16 *revision, u8 digest[MSTP_DIGEST_LEN]);

/**
 * Re-initialize port instance. The CIST and applicable MSTI's for the
 * port will be initialized. Port operational properties and
 * configuration is retained.
 *
 * \param mstp The MSTP instance data.
 *
 * \param portnum The bridge port to initialize
 *
 * \return TRUE if the operation succeeded. The operation will fail if
 * the given port number is larger than the maximum allowed port
 * number (see _vtss_mstp_create_bridge()) - or if the port has not
 * been added.
 */
mesa_rc _vtss_mstp_reinit_port(mstp_bridge_t *mstp, uint portnum);

/**
 * Add port instance to bridge. The CIST and applicable MSTI's for the
 * port will be created - according to the VLAN mapping
 * table.
 *
 * \param mstp The MSTP instance data.
 *
 * \param portnum The bridge port to add
 *
 * \return TRUE if the operation succeeded. The operation will fail if
 * the given port number is larger than the maximum allowed port
 * number (see _vtss_mstp_create_bridge()) - or if the port has already
 * been added.
 */
mesa_rc _vtss_mstp_add_port(mstp_bridge_t *mstp, uint portnum);

/**
 * Delete port instance from bridge.
 *
 * \param mstp The MSTP instance data.
 *
 * \param portnum The bridge port to delete
 *
 * \return TRUE if the operation succeeded. The operation will fail if
 * the given port number is larger than the maximum allowed port
 * number (see _vtss_mstp_create_bridge()) - or if the port has not
 * been added.
 */
mesa_rc _vtss_mstp_delete_port(mstp_bridge_t *mstp, uint portnum);

/**
 * Query whether a port has been added to bridge
 *
 * \param mstp The MSTP instance data.
 *
 * \param portnum The bridge port to query
 *
 * \return TRUE if the port number is valid and the port has been
 * added to the bridge. FALSE otherwise.
 */
mesa_rc _vtss_mstp_port_added(const mstp_bridge_t *mstp, uint portnum);

/**
 * Control bridge port state.
 *
 * \param mstp The MSTP instance data.
 *
 * \param portnum The bridge port to control
 *
 * \param enable Whether the port should be enabled or not. This is
 * the aggregate state of MAC_Enabled, MAC_Operational, Administrative
 * Port State and any 802.1X autorization. As such this controls \e
 * portEnabled (17.19.18) directly.
 *
 * \param linkspeed The link speed of the interface, in MB/s. The
 * speed will be converted into a path cost according to Table 17-3.
 *
 * \param fdx Full duplex operation of physical link
 * (operPointToPointMAC)
 *
 * \return TRUE if the operation succeeded. The operation will fail if
 * the given port number is not a valid port number.
 */
mesa_rc _vtss_mstp_port_enable(mstp_bridge_t *mstp, uint portnum, BOOL enable, u32 linkspeed, BOOL fdx);

/**
 * 12.8.1.1 Read CIST Bridge Protocol Parameters
 * 12.8.1.2 Read MSTI Bridge Protocol Parameters
 *
 * Purpose: To obtain information regarding the Bridge's Spanning Tree
 * Protocol Entity.
 *
 * \param mstp The MSTP instance data.
 *
 * \param msti The MSTP port instance number. The CIST has instance
 * number zero.
 *
 * \param status The bridge status data (12.8.1.1.3 Outputs)
 *
 * \return VTSS_RC_OK if the operation succeeded. The operation will fail if
 * the given \e msti is not a valid instance number.
 *
 * \note Some outputs are only defined for the CIST - see 802.1Q
 * 12.8.1.2.
 */
mesa_rc _vtss_mstp_get_bridge_status(mstp_bridge_t *mstp, uint msti, mstp_bridge_status_t *status);

/**
 * 12.8.1.1 Get Bridge Protocol Parameters
 *
 * Purpose: To read current parameters in the Bridge's Bridge Protocol
 * Entity.
 *
 * \param mstp The MSTP instance data.
 *
 * \param param The bridge configuration data
 *
 * \return TRUE if the operation succeeded.
 */
mesa_rc _vtss_mstp_get_bridge_parameters(mstp_bridge_t *mstp, mstp_bridge_param_t *param);

/**
 * 12.8.1.3 Set Bridge Protocol Parameters
 *
 * To modify parameters in the Bridge's Bridge Protocol Entity, in
 * order to force a configuration of the spanning tree and/or tune the
 * reconfiguration time to suit a specific topology. In RSTP and MSTP
 * implementations, this operation causes these values to be set for
 * all Ports of the Bridge.
 *
 * \param mstp The MSTP instance data.
 *
 * \param param The bridge configuration data
 *
 * \return TRUE if the operation succeeded.
 */
mesa_rc _vtss_mstp_set_bridge_parameters(mstp_bridge_t *mstp, const mstp_bridge_param_t *param);

/**
 * 12.8.1.2 Get MSTI Bridge Protocol Parameters
 *
 * Purpose: To read parameters in the Bridge's Bridge Protocol Entity
 * for the specified Spanning Tree instance.
 *
 * \param mstp The MSTP instance data.
 *
 * \param msti The MSTP instance number. The CIST has instance number
 * zero.
 *
 * \param bridgePriority - the current value of the priority part of the
 * Bridge Identifier (13.23.2) for the MSTI (Output).
 *
 * \return TRUE if the operation succeeded. The operation will fail if
 * the given \e msti is not a valid instance number.
 */
mesa_rc _vtss_mstp_get_bridge_priority(mstp_bridge_t *mstp, uint msti, u8 *bridgePriority);

/**
 * 12.8.1.4 Set MSTI Bridge Protocol Parameters
 *
 * To modify parameters in the Bridge's Bridge Protocol Entity for the
 * specified Spanning Tree instance, in order to force a configuration
 * of the spanning tree and/or tune the reconfiguration time to suit a
 * specific topology.
 *
 * \param mstp The MSTP instance data.
 *
 * \param msti The MSTP instance number. The CIST has instance number
 * zero.
 *
 * \param bridgePriority - the new value of the priority part of the
 * Bridge Identifier (13.23.2) for the MSTI
 *
 * \return TRUE if the operation succeeded. The operation will fail if
 * the given \e msti is not a valid instance number.
 */
mesa_rc _vtss_mstp_set_bridge_priority(mstp_bridge_t *mstp, uint msti, u8 bridgePriority);

/**
 * 12.8.2.1 Read CIST Port Parameters
 * 12.8.2.2 Read MSTI Port Parameters
 *
 * Purpose: To obtain information regarding a specific Port within the
 * Bridge's Bridge Protocol Entity, for the CIST/MSTI.
 *
 * \param mstp The MSTP instance data.
 *
 * \param msti The MSTP port instance number. The CIST has instance
 * number zero.
 *
 * \param portnum Port Number - the number of the Bridge Port
 *
 * \param status The port status data (12.8.2.1.3 Outputs)
 *
 * \return TRUE if the operation succeeded. The operation will fail if
 * the given port number is not a valid port number or \e msti is not
 * a valid instance number.
 *
 * \note Some outputs are only defined for the CIST - see 802.1Q
 * 12.8.2.1.3/12.8.2.2.3 Outputs
 */
mesa_rc _vtss_mstp_get_port_status(mstp_bridge_t *mstp, uint msti, uint portnum, mstp_port_status_t *status);

/**
 * Read Port Vectors
 *
 * Purpose: To obtain information regarding a specific Port within the
 * Bridge's Bridge Protocol Entity, for the CIST/MSTI.
 *
 * \param mstp The MSTP instance data.
 *
 * \param msti The MSTP port instance number. The CIST has instance
 * number zero.
 *
 * \param portnum Port Number - the number of the Bridge Port
 *
 * \param vectors The port vectors
 *
 * \return TRUE if the operation succeeded. The operation will fail if
 * the given port number is not a valid port number or \e msti is not
 * a valid instance number.
 *
 * \note This is for debug and troubleshooting only.
 */
mesa_rc _vtss_mstp_get_port_vectors(const mstp_bridge_t *mstp, uint msti, uint portnum, mstp_port_vectors_t *vectors);

/**
 * Read Port Statistics
 *
 * Purpose: To obtain information regarding a specific Port's PDU
 * reception and transmission within the Bridge's Spanning Tree
 * Protocol Entity.
 *
 * \param mstp The MSTP instance data.
 *
 * \param portnum Port Number - the number of the Bridge Port
 *
 * \param statistics The port statistics data
 *
 * \return TRUE if the operation succeeded. The operation will fail if
 * the given port number is not a valid port number.
 */
mesa_rc _vtss_mstp_get_port_statistics(const mstp_bridge_t *mstp, uint portnum, mstp_port_statistics_t *statistics);

/**
 * Clear Port Statistics
 *
 * Purpose: To clear information regarding a specific Port's PDU
 * reception and transmission within the Bridge's Spanning Tree
 * Protocol Entity.
 *
 * \param mstp The MSTP instance data.
 *
 * \param portnum Port Number - the number of the Bridge Port
 *
 * \return TRUE if the operation succeeded. The operation will fail if
 * the given port number is not a valid port number.
 */
mesa_rc _vtss_mstp_clear_port_statistics(const mstp_bridge_t *mstp, uint portnum);

/**
 * 12.8.2.1 Get CIST port parameters
 *
 * Purpose: To read parameters for a CIST Port in the bridge's Bridge
 * Protocol Entity.
 *
 * \param mstp The MSTP instance data.
 *
 * \param portnum Port Number - the number of the Bridge Port
 *
 * \param param The current port configuration data (12.8.2.3.2
 * Inputs)
 *
 * \return TRUE if the operation succeeded. The operation will fail if
 * the given port number is not a valid port number.
 *
 * \note To set Port Priority or Port Path Cost, use
 * vtss_mstp_set_msti_port_parameters() with the \e msti parameter set
 * to zero (implying the CIST).
 */
mesa_rc _vtss_mstp_get_port_parameters(mstp_bridge_t *mstp, uint portnum, mstp_port_param_t *param);

/**
 * 12.8.2.3 Set CIST port parameters
 *
 * Purpose: To modify parameters for a Port in the bridge's Bridge
 * Protocol Entity in order to force a configuration of the spanning
 * tree for the CIST.
 *
 * Procedure: In RSTP and MSTP Bridges, the Path Cost (13.37.1 of this
 * standard, 17.16.5 of IEEE Std 802.1D) and Port Priority (17.18.7 of
 * IEEE Std 802.1D) parameters for the Port are updated using the
 * supplied values. The reselect parameter value for the CIST for the
 * Port (13.24 of this standard, 17.18.29 of IEEE Std 802.1D) is set
 * TRUE, and the selected parameter for the CIST for the Port (13.24
 * of this standard, 17.18.31 of IEEE Std 802.1D) is set FALSE.
 *
 * \param mstp The MSTP instance data.
 *
 * \param portnum Port Number - the number of the Bridge Port
 *
 * \param param The port configuration data (12.8.2.3.2 Inputs)
 *
 * \return TRUE if the operation succeeded. The operation will fail if
 * the given port number is not a valid port number.
 *
 * \note To set Port Priority or Port Path Cost, use
 * vtss_mstp_set_msti_port_parameters() with the \e msti parameter set
 * to zero (implying the CIST).
 */
mesa_rc _vtss_mstp_set_port_parameters(mstp_bridge_t *mstp, uint portnum, const mstp_port_param_t *param);

/**
 * 12.8.2.2 Get MSTI port parameters
 *
 * Purpose: To read parameters for a MSTI/CIST Port in the Bridge's
 * Bridge Protocol Entity.
 *
 * \param mstp The MSTP instance data.
 *
 * \param msti The MSTP port instance number. The CIST has instance
 * number zero.
 *
 * \param portnum Port Number - the number of the MSTI Port
 *
 * \param param The current CIST/MSTI port configuration data
 * (12.8.2.4.2 Inputs)
 *
 * \return TRUE if the operation succeeded. The operation will fail if
 * the given port number is not a valid port number.
 */
mesa_rc _vtss_mstp_get_msti_port_parameters(mstp_bridge_t *mstp, uint msti, uint portnum, mstp_msti_port_param_t *param);

/**
 * 12.8.2.4 Set MSTI port parameters
 *
 * Purpose: To modify parameters for a Port in the Bridge's Bridge
 * Protocol Entity in order to force a configuration of the spanning
 * tree for the specified Spanning Tree instance.
 *
 * Procedure: The Path Cost (13.37.1 of this standard, 17.16.5 of IEEE
 * Std 802.1D) and Port Priority (17.18.7 of IEEE Std 802.1D)
 * parameters for the specified MSTI and Port are updated using the
 * supplied values. The reselect parameter value for the MSTI for the
 * Port (13.24) is set TRUE, and the selected parameter for the MSTI
 * for the Port () is set FALSE.
 *
 * \param mstp The MSTP instance data.
 *
 * \param msti The MSTP port instance number. The CIST has instance
 * number zero.
 *
 * \param portnum Port Number - the number of the MSTI Port
 *
 * \param param The port configuration data (12.8.2.4.2 Inputs)
 *
 * \return TRUE if the operation succeeded. The operation will fail if
 * the given port number is not a valid port number.
 */
mesa_rc _vtss_mstp_set_msti_port_parameters(mstp_bridge_t *mstp, uint msti, uint portnum, const mstp_msti_port_param_t *param);

/**
 * 12.8.2.5 Force BPDU Migration Check
 *
 * Purpose: To force the specified Port to transmit RST or MST BPDUs
 * (see 13.29 of this standard and 17.26 of IEEE Std 802.1D).
 *
 * \param mstp The MSTP instance data.
 *
 * \param portnum The bridge port to control
 *
 * \return TRUE if the operation succeeded. The operation will fail if
 * the given port number is not a valid port number.
 */
mesa_rc _vtss_mstp_port_mcheck(const mstp_bridge_t *mstp, uint portnum);

/**
 * Tick MSTP state-event machines. This shall be called every 1000msec
 * by the host system.
 *
 * The function drives state for the MSTP instance.
 */
void _vtss_mstp_tick(mstp_bridge_t *mstp);

/**
 * BPDU receive. This is called when a port receives a PDU for the
 * bridge group address.
 *
 * \param mstp The MSTP instance data.
 *
 * \param portnum The physical port on which the frames was received.
 *
 * \param buffer The received BPDU.
 *
 * \param size The length of the BPDU buffer.
 */
void _vtss_mstp_rx(const mstp_bridge_t *mstp, uint portnum, const void *buffer, size_t size);

/**
 * Stringification support - return a printable representation of a
 * bridge id.
 *
 * \param buffer The output buffer (must be at least 24 bytes long for
 * an full bridge display)
 *
 * \param size The length of the output \e buffer.
 *
 * \param bridgeid the bridge identifier - a MSTP_BRIDGEID_LEN long
 * byte array.
 *
 * \return the number of characters put into \e buffer. (Not including
 * the trailing '\\0'). A return value of size or more means that the
 * output was truncated. A negative value signal general failure.
 */
int vtss_mstp_bridge2str(void *buffer, size_t size, const u8 *bridgeid);

/**
 * Lock MSTP state-event machines. Used to optimize bulk port changes
 * (during system initialization etc). The locking can be nested.
 *
 * \param mstp The MSTP instance data.
 *
 */
void _vtss_mstp_stm_lock(mstp_bridge_t *mstp);

/**
 * Unlock MSTP state-event machines. Any pending state-event machine
 * updates will be performed (when nested unlocking reaches the top
 * level).
 *
 * \param mstp The MSTP instance data.
 *
 */
void _vtss_mstp_stm_unlock(mstp_bridge_t *mstp);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_MSTP_API_H_ */
