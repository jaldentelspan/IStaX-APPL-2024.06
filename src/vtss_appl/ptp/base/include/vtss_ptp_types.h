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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#ifndef _VTSS_PTP_TYPES_H_
#define _VTSS_PTP_TYPES_H_
/**
 * \file vtss_ptp_types.h
 * \brief PTP protocol engine type definitions file
 *
 * This file contain the definitions of types used by the API interface and
 * implementation.
 *
 */


#include "vtss/appl/ptp.h"
#ifdef __cplusplus
        extern "C" {
#endif

#if defined(_lint)
/* This is mostly for lint */
#define offsetof(s,m) ((size_t)(unsigned long)&(((s *)0)->m))
#endif /* offsetof() */

#if !defined(PTP_ATTRIBUTE_PACKED)
#define PTP_ATTRIBUTE_PACKED __attribute__((packed)) /* GCC defined */
#endif

#define ARR_SZ(a) (sizeof(a)/sizeof(a[0]))

/**
 * \brief PTP IP multicast Addresses
 *
 */
// "224.0.1.129"
#define PTP_PRIMARY_DEST_IP (((((((in_addr_t)224<<8)+ (in_addr_t)0)<<8) +(in_addr_t)1)<<8) + 129)
// "224.0.0.107"
#define PTP_PDELAY_DEST_IP  (((((((in_addr_t)224<<8)+ (in_addr_t)0)<<8) +(in_addr_t)0)<<8) + 107)

/**
 * \brief Convert a time stamp to a text.
 *
 * format %10d s %11d ns.
 *
 * \param t [IN]  pointer to time stamp
 * \param str [IN/OUT]  buffer to store the text string in
 */
char *TimeStampToString (const mesa_timestamp_t *t, char* str);


/**
 * \brief Returns accuracy hex decimal equivalent string
 *
 * \param t [IN]  accuracy
 * \param str [IN/OUT]  buffer to store the text string in
 */
const char * ClockAccuracyToString (u8 clockAccuracy);

/**
 * \brief Convert a clock identifier to text.
 *
 * \param clockIdentity [IN]  pointer to clock identifier
 * \param str [IN/OUT]  buffer to store the text string in
 */
char *ClockIdentityToString (const vtss_appl_clock_identity clockIdentity, char *str);

/**
 * \brief Convert a clock profile to text.
 *
 * \param profile [IN]  profile
 */
const char *ClockProfileToString(const int profile);

/**
 * \brief Convert a clock quality to text.
 *
 * format: "Cl:%03d Ac:%03d Va:%05d"
 *                           ^-offsetScaledLogVariance
 *                   ^---------clockAccuracy
 *           ^-----------------clockClass
 * \param clockQuality [IN]  pointer to clock identifier
 * \param str [IN/OUT]  buffer to store the text string in
 */
char *ClockQualityToString (const vtss_appl_clock_quality *clockQuality, char *str);

/**
 * \brief compare two PortIdentities.
 *
 * Like memcmp.
 */
int PortIdentitycmp(const vtss_appl_ptp_port_identity* a, const vtss_appl_ptp_port_identity* b);

/**
 * \brief main PTP Filter types
 */
enum {
    PTP_FILTERTYPE_ACI_DEFAULT,
    PTP_FILTERTYPE_ACI_FREQ_XO,
    PTP_FILTERTYPE_ACI_PHASE_XO,
    PTP_FILTERTYPE_ACI_FREQ_TCXO,
    PTP_FILTERTYPE_ACI_PHASE_TCXO,
    PTP_FILTERTYPE_ACI_FREQ_OCXO_S3E,
    PTP_FILTERTYPE_ACI_PHASE_OCXO_S3E,
    PTP_FILTERTYPE_ACI_BC_PARTIAL_ON_PATH_FREQ,
    PTP_FILTERTYPE_ACI_BC_PARTIAL_ON_PATH_PHASE,
    PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_FREQ,
    PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE,
    PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE,
    PTP_FILTERTYPE_ACI_FREQ_ACCURACY_FDD,
    PTP_FILTERTYPE_ACI_FREQ_ACCURACY_XDSL,
    PTP_FILTERTYPE_ACI_ELEC_FREQ,
    PTP_FILTERTYPE_ACI_ELEC_PHASE,
    PTP_FILTERTYPE_ACI_PHASE_RELAXED_C60W,
    PTP_FILTERTYPE_ACI_PHASE_RELAXED_C150,
    PTP_FILTERTYPE_ACI_PHASE_RELAXED_C180,
    PTP_FILTERTYPE_ACI_PHASE_RELAXED_C240,
    PTP_FILTERTYPE_ACI_PHASE_OCXO_S3E_R4_6_1,
    PTP_FILTERTYPE_ACI_BASIC_PHASE,
    PTP_FILTERTYPE_ACI_BASIC_PHASE_LOW,
    PTP_FILTERTYPE_BASIC,
    PTP_FILTERTYPE_MAX_TYPE
};

/**
 * \brief Convert a device type to text.
 *
 * \param state [IN]  device type
 * \return buffer with the text string
 */
const char *DeviceTypeToString(vtss_appl_ptp_device_type_t state);


/**
 * \brief Convert a port state to text.
 *
 * \param state [IN]  port state
 * \param str [IN/OUT]  buffer to store the text string in
 */
const char *PortStateToString(u8 state);

#define MAX_UNICAST_MASTERS_PR_SLAVE 5
#define MAX_UNICAST_SLAVES_PR_MASTER 256

/*
 * used to initialize the run time options default
 */
#define DEFAULT_CLOCK_VARIANCE       (0xffff) /* indicates that the variance is not computed (spec 7.6.3.3) */
#define DEFAULT_802_1_AS_CLOCK_VARIANCE (0x436A) /* IEEE 802.1AS-2020 subclause 8.6.2.4 */
#define DEFAULT_CLOCK_CLASS          248      /* Default. This clockClass shall be used if none of the other clockClass definitions apply */
#define G8275PRTC_GM_CLOCK_CLASS       6      /* This clockClass shall be used if GM is locked to a 1PPS input */
#define G8275PRTC_GM_HO_CLOCK_CLASS    7      /* This clockClass shall be used if GM is in holdoverafter 1PPS input */
#define G8275PRTC_BC_HO_CLOCK_CLASS  135
#define G8275PRTC_BC_OUT_OF_HO_CLOCK_CLASS  165
#define G8275PRTC_GM_OUT_OF_HO_CLOCK_CLASS_CAT1  140
#define G8275PRTC_GM_OUT_OF_HO_CLOCK_CLASS_CAT2  150
#define G8275PRTC_GM_OUT_OF_HO_CLOCK_CLASS_CAT3  160
#define G8275PRTC_TSC_CLOCK_CLASS       255
/* From IEEE-1588-2008, This clockClass is used when grandmaster is not locked to primary reference time source and in holdover mode. By default, it is assumed to be not locked to primary reference.*/
/* changing clockClass 7 to 187, to support virtual port for default profile.*/
#define DEFAULT_GM_CLOCK_CLASS         187
#define G8275PRTC_GM_ACCURACY          0x21

#define DEFAULT_NO_RESET_CLOCK       TRUE
/* spec defined constants  */

/* features, only change to reflect changes in implementation */
#define CLOCK_FOLLOWUP    TRUE

/**
 * \brief delayMechanism values
 */
enum { DELAY_MECH_E2E = 1, DELAY_MECH_P2P, DELAY_MECH_COMMON_P2P, DELAY_MECH_DISABLED = 0xfe };

/* Number of missed Pdelay_resp's before the Peer delay measurement is disqualified (in 802.1AS called allowedLostResponses) */
#define DEFAULT_MAX_CONTIGOUS_MISSED_PDELAY_RESPONSE 9

/* In 802.1as, number of allowed instances where the computed mean PropagationDelay exceeds threshold meanLinkDelay and/or neighborRateration is invalid. */
#define DEFAULT_MAX_PDELAY_ALLOWED_FAULTS 9

/* In 802.1as, number of gPTP-capable message intervals to wait without receiving a Signaling message from its neighbor containing a gPTP-capable TLV */
#define DEFAULT_GPTP_CAPABLE_RECEIPT_TIMEOUT 9

/* features, only change to reflect changes in implementation */
/* used in initData */
#define VERSION_PTP       2

#define MINOR_VERSION_PTP 1
#define MINOR_VERSION_PTP_2011 0
/**
 * \brief message header messageType field values
 */
enum {
    PTP_MESSAGE_TYPE_SYNC=0,  PTP_MESSAGE_TYPE_DELAY_REQ,  PTP_MESSAGE_TYPE_P_DELAY_REQ,
    PTP_MESSAGE_TYPE_P_DELAY_RESP,
    PTP_MESSAGE_TYPE_FOLLOWUP=8,  PTP_MESSAGE_TYPE_DELAY_RESP,
    PTP_MESSAGE_TYPE_P_DELAY_RESP_FOLLOWUP, PTP_MESSAGE_TYPE_ANNOUNCE, PTP_MESSAGE_TYPE_SIGNALLING,
    PTP_MESSAGE_TYPE_MANAGEMENT,
    PTP_MESSAGE_TYPE_ALL_OTHERS
};

/* define ptp packet header fields */
#define PTP_MESSAGE_MESSAGE_TYPE_OFFSET      0       /* messageType's offset in the ptp packet */
#define PTP_MESSAGE_VERSION_PTP_OFFSET       1       /* versionPTP's offset in the ptp packet */
#define PTP_MESSAGE_MESSAGE_LENGTH_OFFSET    2       /* messageLength's offset in the ptp packet */
#define PTP_MESSAGE_DOMAIN_OFFSET            4       /* domain's offset in the ptp packet */
#define PTP_MESSAGE_RESERVED_BYTE_OFFSET     5       /* first reserved byte's offset in the ptp packet */
#define PTP_MESSAGE_FLAG_FIELD_OFFSET        6       /* flag field's offset in the ptp packet */
#define PTP_MESSAGE_CORRECTION_FIELD_OFFSET  8       /* correction field's offset in the ptp packet */
#define PTP_MESSAGE_RESERVED_FOR_TS_OFFSET   16      /* reserved field's offset in the ptp packet */
#define PTP_MESSAGE_SOURCE_PORT_ID_OFFSET    20      /* sourcePortIdentity field's offset in the ptp packet */
#define PTP_MESSAGE_SEQUENCE_ID_OFFSET       30      /* sequenceID's offset in the ptp packet */
#define PTP_MESSAGE_CONTROL_FIELD_OFFSET     32      /* controlField's offset in the ptp packet */
#define PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET   33      /* logMessageInterval's offset in the ptp packet */


/* define ptp packet sync/follow_up/announce fields */
#define PTP_MESSAGE_ORIGIN_TIMESTAMP_OFFSET  34      /* originTimestamp's offset in the ptp packet */

/* define ptp packet delay_Resp fields */
#define PTP_MESSAGE_RECEIVE_TIMESTAMP_OFFSET  34     /* receiveTimestamp's offset in the ptp delay_resp packet */
#define PTP_MESSAGE_REQ_PORT_ID_OFFSET        44     /* requestingPortIdentity's offset in the ptp packet */

/* define ptp packet Pdelay_xxx fields */
#define PTP_MESSAGE_PDELAY_TIMESTAMP_OFFSET   34     /* receiveTimestamp's offset in the ptp delay_resp packet */
#define PTP_MESSAGE_PDELAY_PORT_ID_OFFSET     44     /* requestingPortIdentity's offset in the ptp packet */

/* define ptp packet announce fields */
#define PTP_MESSAGE_CURRENT_UTC_OFFSET        44     /* current utc's offset in the ptp packet */
#define PTP_MESSAGE_RESERVED_OFFSET           45     /* Reserved field in the announce messages ptp packet */
#define PTP_MESSAGE_GM_PRI1_OFFSET            47
#define PTP_MESSAGE_GM_CLOCK_Q_OFFSET         48
#define PTP_MESSAGE_GM_PRI2_OFFSET            52
#define PTP_MESSAGE_GM_IDENTITY_OFFSET        53
#define PTP_MESSAGE_STEPS_REMOVED_OFFSET      61
#define PTP_MESSAGE_TIME_SOURCE_OFFSET        63

/* define ptp packet Signalling fields */
#define PTP_MESSAGE_SIGNAL_TARGETPORTIDENTITY 34     /* targetPortIdentity's offset in the ptp signalling packet */

/* define ptp 802.1as followup message fields */
#define PTP_MESSAGE_FOLLOWUP_CUM_RATE_OFFSET  54     /* cumulativeScaledRateOffset in 802.1as Followup TLV */
#define PTP_MESSAGE_FOLLOWUP_GM_PHASE_CHANGE  60     // lastGmPhaseChange
#define PTP_MESSAGE_FOLLOWUP_GM_FREQ_CHANGE   72     // scaledLastGmFreqChange

/**
 * \brief Local Clock operational mode
 */
typedef enum {
    VTSS_PTP_CLOCK_FREERUN,
    VTSS_PTP_CLOCK_LOCKING,
    VTSS_PTP_CLOCK_LOCKED,
} vtss_ptp_clock_mode_t;

/**
 * \brief Clock Default Data Set structure
 */
typedef struct {
    vtss_appl_ptp_clock_status_default_ds_t status;
    //vtss_appl_ptp_clock_config_default_ds_t cfg;
} ptp_clock_default_ds_t;

typedef struct {
    mesa_timeinterval_t master_to_slave_max;
    mesa_timeinterval_t master_to_slave_min;
    mesa_timeinterval_t master_to_slave_mean;
    u32                 master_to_slave_mean_count;
    mesa_timeinterval_t master_to_slave_cur;
    mesa_timeinterval_t slave_to_master_max;
    mesa_timeinterval_t slave_to_master_min;
    mesa_timeinterval_t slave_to_master_mean;
    u32                 slave_to_master_mean_count;
    mesa_timeinterval_t slave_to_master_cur;
    u32                 sync_pack_rx_cnt;
    u32                 sync_pack_timeout_cnt;
    u32                 delay_req_pack_tx_cnt;
    u32                 delay_resp_pack_rx_cnt;
    u32                 sync_pack_seq_err_cnt;
    u32                 follow_up_pack_loss_cnt;
    u32                 delay_resp_seq_err_cnt;
    u32                 delay_req_not_saved_cnt;
    u32                 delay_req_intr_not_rcvd_cnt;
    bool                enable;
} vtss_ptp_slave_statistics_t;

#define PTP_PATH_TRACE_MAX_SIZE 179         /* max number of path traces that can be handled in this node */
typedef struct ptp_path_trace_s{
    int size;                                   /* actual number of entries in the path sequence */
    vtss_appl_clock_identity pathSequence[PTP_PATH_TRACE_MAX_SIZE];   /* sequence of clock identities */
} ptp_path_trace_t;

/**
 * \brief SystemIdentity.
 * This structure is used to Define the SystemIdentity as described in 802.1AS clause 10.3.2.
 */
typedef struct {
    u8                          priority1;      /**< Priority 1 value */
    vtss_appl_clock_quality     clockQuality;   /**< Quality of the PTP clock */
    u8                          priority2;      /**< Priority 2 value */
    vtss_appl_clock_identity    clockIdentity;  /**< Identity of the PTP clock */
} vtss_ptp_system_identity_t;

/**
 * \brief an enum that takes the values Received, Mine, Aged, or Disabled, to indicate the origin and state of the port's time-synchronization spanning tree information
 */
typedef enum {
    VTSS_PTP_INFOIS_RECEIVED,   /**< the port has received current information (i.e., announce receipt timeout has not occurred and, if gmPresent is TRUE, sync receipt timeout also has not occurred) from the master time-aware system for the attached communication path */
    VTSS_PTP_INFOIS_MINE,       /**< information for the port has been derived from the SlavePort for the time-aware system (with the addition of SlavePort stepsRemoved). This includes the possibility that the SlavePort is the port whose portNumber is 0, i.e., the time-aware system is the root of the gPTP domain */
    VTSS_PTP_INFOIS_AGED,       /**< announce receipt timeout or, in the case where gmPresent is TRUE, sync receipt timeout have occurred */
    VTSS_PTP_INFOIS_DISABLED,   /**< If portOper, ptpPortEnabled, and asCapable are not all TRUE */
} vtss_ptp_infoIs_t;


/**
 * \brief PriorityVector.
 * This structure is used to Define the PriorityVector as described in 802.1AS clause 10.3.5.
 */
typedef struct {
    vtss_ptp_system_identity_t      systemIdentity;      /**< system identity */
    u16                             stepsRemoved;        /**< Number of communications path between the clock and the Grandmaster clock */
    vtss_appl_ptp_port_identity     sourcePortIdentity;  /**< Port identity of master to which this slave is connected */
    u16                             portNumber;          /**< local port number for the priority vector */
} vtss_ptp_priority_vector_t;

/**
 * \brief clock instance variables for the 802.1as BMCA.
 * This structure is used to pr clock cariables for the 802.1AS BMCA.
 */
typedef struct {
    vtss_appl_ptp_clock_timeproperties_ds_t timeProperties;         // The Time properties to be sent out in announce messages.
    vtss_appl_ptp_clock_timeproperties_ds_t sysTimeProperties;      // The time properties configured for this node.
    vtss_ptp_priority_vector_t              systemPriorityVector;   // Ny own priorityVector.
    vtss_ptp_priority_vector_t              gmPriorityVector;       // Received priorityVector from current grandmaster.
    bool                                    gmPresent;              // indicates whether a grandmaster-capable time-aware system is presentin the domain
} vtss_ptp_clock_802_1as_bmca_t;

/**
 * \brief port instance variables for the 802.1as BMCA.
 * This structure is used to port cariables for the 802.1AS BMCA.
 */
typedef struct {
    vtss_ptp_priority_vector_t              masterPriority; // priority vector that holds the data used in announce when the port is in master state.
    vtss_ptp_priority_vector_t              portPriority; // latest received announce information or masterPriority in case of timeout.
    vtss_appl_ptp_clock_timeproperties_ds_t annTimeProperties; //latest received announce information.
    vtss_ptp_infoIs_t                       infoIs; // an enum that takes the values Received, Mine, Aged, or Disabled, to indicate the origin and state of the port's time-synchronization spanning tree information
    u16                                     portStepsRemoved; // the value of stepsRemoved for the port.
    bool                                    bmca_enabled;     // true if the ptp port state is != DISABLED or INITIALIZING
    //portDS.status.s_802_1as.portRole: seems to be the same as selectedRole in the port_role_Selection state machine
} vtss_ptp_port_802_1as_bmca_t;

char * vtss_ptp_SystemIdentityToString(const vtss_ptp_system_identity_t *systemIdentity, char *str, size_t size);
char * vtss_ptp_TimePropertiesToString(const vtss_appl_ptp_clock_timeproperties_ds_t * properties, char *str, size_t size);
char * vtss_ptp_PriorityVectorToString(const vtss_ptp_priority_vector_t * priority, char *str, size_t size);
const char *vtss_ptp_InfoIsToString(vtss_ptp_infoIs_t i);
const char *vtss_ptp_PortRoleToString(vtss_appl_ptp_802_1as_port_role_t i);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_PTP_TYPES_H_ */
